// SkypeRecorderDlg.h : header file
//

#include <MMSystem.h>
#define MAX_BUFFERS	3

#if !defined(AFX_SKYPERECORDERDLG_H__5DEF6A75_BC74_4765_B1A8_58B69D841520__INCLUDED_)
#define AFX_SKYPERECORDERDLG_H__5DEF6A75_BC74_4765_B1A8_58B69D841520__INCLUDED_

/////////////////////////////////////////////////////////////////////////////
// CSkypeRecorderDlg dialog

class CRecorder
{
// Construction
public:
	CRecorder();	// standard constructor
	
	int getState() { return recstate; }
	void Start(const char *fn);
	void Stop();

public:
	int StartRecording();
	void ProcessHeader(WAVEHDR * pHdr);

public:
	int OpenDevice();
	void CloseDevice();
	int PrepareBuffers();
	void UnPrepareBuffers();
	unsigned int FillDevices();
	int getCaps();
	void setupMixerLevels();

	
	char fn_n[300];
	int recstate;
	
};

#endif // !defined(AFX_SKYPERECORDERDLG_H__5DEF6A75_BC74_4765_B1A8_58B69D841520__INCLUDED_)
