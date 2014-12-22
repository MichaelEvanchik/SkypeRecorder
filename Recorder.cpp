// SkypeRecorderDlg.cpp : implementation file
//

#include <windows.h>
#include "recorder.h"
#include "portmixer.h"
#include <fstream.h>

#pragma comment(lib,"winmm.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DWORD_PTR DWORD
#define DWORDPTR DWORD


BOOL m_bRun;
HWAVEIN m_hWaveIn;
WAVEFORMATEX m_stWFEX;
WAVEHDR m_stWHDR[MAX_BUFFERS];
HANDLE m_hThread;
HMMIO m_hOPFile;
MMIOINFO m_stmmIF;
MMCKINFO m_stckOut,m_stckOutRIFF; 

CRecorder::CRecorder()
{
	FillDevices();
}



UINT CRecorder::FillDevices()
{
	UINT nDevices,nC1;
	WAVEINCAPS stWIC={0};
	MMRESULT mRes;

	nDevices=waveInGetNumDevs();

	for(nC1=0;nC1<1;++nC1)
	{
		ZeroMemory(&stWIC,sizeof(WAVEINCAPS));
		mRes=waveInGetDevCaps(nC1,&stWIC,sizeof(WAVEINCAPS));
	}
	getCaps();
	return nDevices;
}

int CRecorder::getCaps()
{
	int nSel=0;
	WAVEINCAPS stWIC={0};
	MMRESULT mRes;


	if(nSel!=-1)
	{
		ZeroMemory(&stWIC,sizeof(WAVEINCAPS));
		mRes=waveInGetDevCaps(nSel,&stWIC,sizeof(WAVEINCAPS));
		if(mRes==0)
		{
			/*if(WAVE_FORMAT_1M08==(stWIC.dwFormats&WAVE_FORMAT_1M08))
				MessageBox("11 kHz, mono, 8-bit");
			if(WAVE_FORMAT_1M16==(stWIC.dwFormats&WAVE_FORMAT_1M16))
				MessageBox("11 kHz, mono, 16-bit");
			if(WAVE_FORMAT_2M16==(stWIC.dwFormats&WAVE_FORMAT_2M08))
				MessageBox("22.05 kHz, mono, 8-bit");
			if(WAVE_FORMAT_2M16==(stWIC.dwFormats&WAVE_FORMAT_2M16))
				MessageBox("22.05 kHz, mono, 16-bit");
			if(WAVE_FORMAT_4M08==(stWIC.dwFormats&WAVE_FORMAT_4M08))
				MessageBox("44.1 kHz, mono, 8-bit");
			if(WAVE_FORMAT_4M16==(stWIC.dwFormats&WAVE_FORMAT_4M16))
				MessageBox("44.1 kHz, mono, 16-bit");*/
		}
	}

	return 0;
}


DWORD WINAPI ThFunc(LPVOID pDt)
{
	CRecorder *pOb=(CRecorder*)pDt;
	pOb->StartRecording();
	return 0;
}

void CALLBACK waveInProc(HWAVEIN hwi,UINT uMsg,DWORD dwInstance,DWORD dwParam1,DWORD dwParam2)
{
	WAVEHDR *pHdr=NULL;
	switch(uMsg)
	{
		case WIM_CLOSE:
			break;

		case WIM_DATA:
			{
				CRecorder *pDlg=(CRecorder*)dwInstance;
				pDlg->ProcessHeader((WAVEHDR *)dwParam1);
			}
			break;

		case WIM_OPEN:
			break;

		default:
			break;
	}
}

VOID CRecorder::ProcessHeader(WAVEHDR * pHdr)
{
	MMRESULT mRes=0;

	if(WHDR_DONE==(WHDR_DONE &pHdr->dwFlags))
	{
		mmioWrite(m_hOPFile,pHdr->lpData,pHdr->dwBytesRecorded);
		mRes=waveInAddBuffer(m_hWaveIn,pHdr,sizeof(WAVEHDR));
	}
}

int CRecorder::OpenDevice()
{
	int nT1=0;
	double dT1=0.0;
	MMRESULT mRes=0;
	
	m_stWFEX.nChannels=1;
	m_stWFEX.wBitsPerSample=8;
	m_stWFEX.nSamplesPerSec=11025;
	m_stWFEX.wFormatTag=WAVE_FORMAT_PCM;
	
	m_stWFEX.nBlockAlign=m_stWFEX.nChannels*m_stWFEX.wBitsPerSample/8;
	m_stWFEX.nAvgBytesPerSec=m_stWFEX.nSamplesPerSec*m_stWFEX.nBlockAlign;
	m_stWFEX.cbSize=sizeof(WAVEFORMATEX);
	
	mRes=waveInOpen(&m_hWaveIn,0,&m_stWFEX,(DWORDPTR)waveInProc,(DWORD_PTR)this,CALLBACK_FUNCTION);
	if(mRes!=MMSYSERR_NOERROR)
	{
		return -1;
	}

	setupMixerLevels();

	ZeroMemory(&m_stmmIF,sizeof(MMIOINFO));
	m_hOPFile=mmioOpen(fn_n,&m_stmmIF,MMIO_WRITE | MMIO_CREATE);
	if(m_hOPFile==NULL)
	{
		return -1;
	}
	ZeroMemory(&m_stckOutRIFF,sizeof(MMCKINFO));
	m_stckOutRIFF.fccType = mmioFOURCC('W', 'A', 'V', 'E'); 
	mRes=mmioCreateChunk(m_hOPFile, &m_stckOutRIFF, MMIO_CREATERIFF);
	if(mRes!=MMSYSERR_NOERROR)
	{
		return -1;
	}
	ZeroMemory(&m_stckOut,sizeof(MMCKINFO));
	m_stckOut.ckid = mmioFOURCC('f', 'm', 't', ' ');
	m_stckOut.cksize = sizeof(m_stWFEX);
	mRes=mmioCreateChunk(m_hOPFile, &m_stckOut, 0);
	if(mRes!=MMSYSERR_NOERROR)
	{
		return -1;
	}
	nT1=mmioWrite(m_hOPFile, (HPSTR) &m_stWFEX, sizeof(m_stWFEX));
	if(nT1!=sizeof(m_stWFEX))
	{
		return -1;
	}
	mRes=mmioAscend(m_hOPFile, &m_stckOut, 0);
	if(mRes!=MMSYSERR_NOERROR)
	{
		return -1;
	}
	m_stckOut.ckid = mmioFOURCC('d', 'a', 't', 'a');
	mRes=mmioCreateChunk(m_hOPFile, &m_stckOut, 0);
	if(mRes!=MMSYSERR_NOERROR)
	{
		return -1;
	}
	return 0;
}

void CRecorder::setupMixerLevels()
{
	internalPortAudioStream pa_str;
	PaWMMEStreamData pa_strd;
	PxMixer *pxm;
	PxVolume pxv;
	
	pa_str.past_DeviceData=&pa_strd;
	
	pa_strd.hWaveIn=m_hWaveIn;
	pa_strd.hWaveOut=NULL;

	pxv=1;

	pxm=Px_OpenMixer(&pa_str,0);
	
	Px_SetCurrentInputSource(pxm, 2);
	Px_SetInputVolume(pxm, pxv);
	free(pxm);
	Px_CloseMixer(&pa_str);
}

VOID CRecorder::CloseDevice()
{
	MMRESULT mRes=0;
	
	if(m_hWaveIn)
	{
		UnPrepareBuffers();
		mRes=waveInClose(m_hWaveIn);
	}
	if(m_hOPFile)
	{
		mRes=mmioAscend(m_hOPFile, &m_stckOut, 0);
		if(mRes!=MMSYSERR_NOERROR)
		{
		}
		mRes=mmioAscend(m_hOPFile, &m_stckOutRIFF, 0);
		if(mRes!=MMSYSERR_NOERROR)
		{
		}
		mmioClose(m_hOPFile,0);
		m_hOPFile=NULL;
	}
	m_hWaveIn=NULL;
}

int CRecorder::PrepareBuffers()
{
	MMRESULT mRes=0;
	int nT1=0;

	for(nT1=0;nT1<MAX_BUFFERS;++nT1)
	{
		m_stWHDR[nT1].lpData=(LPSTR)HeapAlloc(GetProcessHeap(),8,m_stWFEX.nAvgBytesPerSec);
		m_stWHDR[nT1].dwBufferLength=m_stWFEX.nAvgBytesPerSec;
		m_stWHDR[nT1].dwUser=nT1;
		mRes=waveInPrepareHeader(m_hWaveIn,&m_stWHDR[nT1],sizeof(WAVEHDR));
		if(mRes!=0)
		{
			return -1;
		}
		mRes=waveInAddBuffer(m_hWaveIn,&m_stWHDR[nT1],sizeof(WAVEHDR));
		if(mRes!=0)
		{
			return -1;
		}
	}
	return 0;
}

VOID CRecorder::UnPrepareBuffers()
{
	MMRESULT mRes=0;
	int nT1=0;

	if(m_hWaveIn)
	{
		mRes=waveInStop(m_hWaveIn);
		for(nT1=0;nT1<3;++nT1)
		{
			if(m_stWHDR[nT1].lpData)
			{
				mRes=waveInUnprepareHeader(m_hWaveIn,&m_stWHDR[nT1],sizeof(WAVEHDR));
				HeapFree(GetProcessHeap(),0,m_stWHDR[nT1].lpData);
				ZeroMemory(&m_stWHDR[nT1],sizeof(WAVEHDR));
			}
		}
	}
}

int CRecorder::StartRecording()
{	
	MMRESULT mRes;

	{
		OpenDevice();
		PrepareBuffers();
		mRes=waveInStart(m_hWaveIn);
		if(mRes!=0)
		{
			return -1;
		}
		while(m_bRun)
		{
			SleepEx(100,FALSE);
		}
	}

	CloseDevice();
	CloseHandle(m_hThread);
	m_hThread=NULL;
	return 0;
}


void CRecorder::Start(const char *fn) 
{
	// TODO: Add your control notification handler code here
	strcpy(fn_n, fn);
	//create filename!
	m_hThread=CreateThread(NULL,0,ThFunc,this,0,NULL);
	m_bRun=TRUE;
}

void CRecorder::Stop() 
{
	// TODO: Add your control notification handler code here
	m_bRun=FALSE;
	while(m_hThread)
	{
		SleepEx(100,FALSE);
	}
}

