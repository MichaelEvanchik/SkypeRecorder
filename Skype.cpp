//#include "stdafx.h"
#include <conio.h>
#include <stdio.h>
#include <string.h>
#include <process.h>
#include <time.h>
#include <windows.h>
#include <rpcdce.h>

#include "recorder.h"

CRecorder rec;

enum {
	SKYPECONTROLAPI_ATTACH_SUCCESS=0,								// Client is successfully attached and API window handle can be found in wParam parameter
	SKYPECONTROLAPI_ATTACH_PENDING_AUTHORIZATION=1,	// Skype has acknowledged connection request and is waiting for confirmation from the user.
																									// The client is not yet attached and should wait for SKYPECONTROLAPI_ATTACH_SUCCESS message
	SKYPECONTROLAPI_ATTACH_REFUSED=2,								// User has explicitly denied access to client
	SKYPECONTROLAPI_ATTACH_NOT_AVAILABLE=3,					// API is not available at the moment. For example, this happens when no user is currently logged in.
																									// Client should wait for SKYPECONTROLAPI_ATTACH_API_AVAILABLE broadcast before making any further
																									// connection attempts.
	SKYPECONTROLAPI_ATTACH_API_AVAILABLE=0x8001
};

int connected;
HWND hInit_MainWindowHandle;
HINSTANCE hInit_ProcessHandle;
char acInit_WindowClassName[128];
HANDLE hGlobal_ThreadShutdownEvent;
bool volatile fGlobal_ThreadRunning=true;
UINT uiGlobal_MsgID_SkypeControlAPIAttach;
UINT uiGlobal_MsgID_SkypeControlAPIDiscover;
HWND hGlobal_SkypeAPIWindowHandle=NULL;
DWORD ulGlobal_PromptConsoleMode=0;
HANDLE volatile hGlobal_PromptConsoleHandle=NULL;


/////////////////////////////////////////////////////////////////////////////
void procMessage(char *data)
{
	if(strstr(data, "CALL") != NULL)
	{
		if(strstr(data, "INPROGRESS") != NULL)
		{
			char fn[100];
			sprintf(fn,"file%d.wav", time(NULL));
			rec.Start(fn);
		}
		if(strstr(data, "FINISHED") != NULL)
		{
			rec.Stop();
		}
	}
}
//////////////////////////////////////////////////////////////////////////////

int sendmsg(const char *msg)
{
	COPYDATASTRUCT oCopyData;
				
	// send command to skype
	oCopyData.dwData=0;
	oCopyData.lpData=(void*)msg;
	oCopyData.cbData=strlen(msg)+1;
	if( oCopyData.cbData!=1 )
	{
		if( SendMessage( hGlobal_SkypeAPIWindowHandle, WM_COPYDATA, (WPARAM)hInit_MainWindowHandle, (LPARAM)&oCopyData)==FALSE )
		{
			hGlobal_SkypeAPIWindowHandle=NULL;
			
			MessageBox(NULL,"","recording",MB_OK);
			connected=0;
			return -1;
		}
	}
	return 0;
}


void GrabPage()
{
	sendmsg("OPEN OPTIONS SOUNDDEVICES");

	HWND sub, top,sub1,sub2,sub3,save,adj;

	Sleep(500);
	do
	{
		do
		{
			sub=FindWindow("TskOptionsForm.UnicodeClass", "Skype\x99 - Options");
			
		}while(sub == NULL);

		top=NULL;
		do
		{
			top=FindWindowEx(sub, top, "TTntPanel.UnicodeClass","");
			
			sub1=FindWindowEx(top, NULL, "TPanel", "");
			save=FindWindowEx(sub1, NULL, "TTntButton.UnicodeClass", "Save");
			
			sub2=FindWindowEx(top, NULL, "TTntPageControl.UnicodeClass", "");
			sub3=FindWindowEx(sub2, NULL, "TTntTabSheet.UnicodeClass", "tsAudio");
			
			adj=FindWindowEx(sub3, NULL, "TTntCheckBox.UnicodeClass", "Let Skype adjust my sound device settings");
			
		}while(((save == NULL) || (adj == NULL)) && (top != NULL));
	} while((save == NULL) && (adj == NULL));

	if(SendMessage(adj, BM_GETCHECK,0,0) == BST_CHECKED)
		SendMessage(adj, BM_CLICK, BST_UNCHECKED, 0);
//	Sleep(100);
//	SendMessage(adj, BM_SETCHECK, BST_UNCHECKED, 0);
	SendMessage(save, BM_CLICK, 0, 0);
}


void ListenIn()
{
	HWND top,sub1,sub2,b1,ok;
	do
	{
		top=FindWindow("TskACLForm.UnicodeClass", "Skype\x99");
		ok=FindWindowEx(top, NULL, "TTntButton.UnicodeClass", "OK");
		sub1=FindWindowEx(top, NULL, "TPanel", "");
		sub2=FindWindowEx(sub1, NULL, "TPanel", "");
		b1=FindWindowEx(sub2, NULL, "TTntRadioButton.UnicodeClass", "Allow this program to use Skype");
		
		Sleep(50);
	}while((b1 == NULL) && (!connected));

	SendMessage(b1, BM_CLICK,0,0);
	SendMessage(ok, BM_CLICK,0,0);
}


int connect()
{
	
	return 0;
}

int disconnect()
{
	
	return 0;
}

static LRESULT APIENTRY SkypeAPITest_Windows_WindowProc(
														HWND hWindow, UINT uiMessage, WPARAM uiParam, LPARAM ulParam)
{
	LRESULT lReturnCode;
	bool fIssueDefProc;
	
	lReturnCode=0;
	fIssueDefProc=false;
	switch(uiMessage)
	{
	case WM_DESTROY:
		connected=0;
		hInit_MainWindowHandle=NULL;
		PostQuitMessage(0);
		break;
	case WM_COPYDATA:
		if( hGlobal_SkypeAPIWindowHandle==(HWND)uiParam )
		{
			PCOPYDATASTRUCT poCopyData=(PCOPYDATASTRUCT)ulParam;
			//MessageBox(NULL,(char*)poCopyData->lpData,"",MB_OK);
			
			procMessage((char*)poCopyData->lpData);

			lReturnCode=1;
		}
		break;
	default:
		if( uiMessage==uiGlobal_MsgID_SkypeControlAPIAttach )
		{
			switch(ulParam)
			{
			case SKYPECONTROLAPI_ATTACH_SUCCESS:
				
				connected=1;
				hGlobal_SkypeAPIWindowHandle=(HWND)uiParam;

				GrabPage();
				break;
			case SKYPECONTROLAPI_ATTACH_PENDING_AUTHORIZATION:
				connected=0;
				ListenIn();
				
				break;
			case SKYPECONTROLAPI_ATTACH_REFUSED:
				connected=0;
				ListenIn();
				break;
			case SKYPECONTROLAPI_ATTACH_NOT_AVAILABLE:
				connected=0;
				break;
			case SKYPECONTROLAPI_ATTACH_API_AVAILABLE:
				SendMessage( HWND_BROADCAST, uiGlobal_MsgID_SkypeControlAPIDiscover, (WPARAM)hInit_MainWindowHandle, 0);
				break;
			}
			lReturnCode=1;
			break;
		}
		fIssueDefProc=true;
		break;
	}
	if( fIssueDefProc )
		lReturnCode=DefWindowProc( hWindow, uiMessage, uiParam, ulParam);
	
	return(lReturnCode);
}

int Initialize_CreateWindowClass(void)
{
	unsigned char *paucUUIDString;
	RPC_STATUS lUUIDResult;
	int fReturnStatus;
	UUID oUUID;
	
	fReturnStatus=-1;
	lUUIDResult=UuidCreate(&oUUID);
	hInit_ProcessHandle=(HINSTANCE)OpenProcess( PROCESS_DUP_HANDLE, FALSE, GetCurrentProcessId());
	if( hInit_ProcessHandle!=NULL && (lUUIDResult==RPC_S_OK || lUUIDResult==RPC_S_UUID_LOCAL_ONLY) )
	{
		if( UuidToString( &oUUID, &paucUUIDString)==RPC_S_OK )
		{
			WNDCLASS oWindowClass;
			
			strcpy( acInit_WindowClassName, "Skype-API-Test-");
			strcat( acInit_WindowClassName, (char *)paucUUIDString);
			
			oWindowClass.style=CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS;
			oWindowClass.lpfnWndProc=(WNDPROC)&SkypeAPITest_Windows_WindowProc;
			oWindowClass.cbClsExtra=0;
			oWindowClass.cbWndExtra=0;
			oWindowClass.hInstance=hInit_ProcessHandle;
			oWindowClass.hIcon=NULL;
			oWindowClass.hCursor=NULL;
			oWindowClass.hbrBackground=NULL;
			oWindowClass.lpszMenuName=NULL;
			oWindowClass.lpszClassName=acInit_WindowClassName;
			
			if( RegisterClass(&oWindowClass)!=0 )
				fReturnStatus=0;
			
			RpcStringFree(&paucUUIDString);
		}
	}
	if( fReturnStatus==-1 )
		CloseHandle(hInit_ProcessHandle),hInit_ProcessHandle=NULL;
	return(fReturnStatus);
}

void DeInitialize_DestroyWindowClass(void)
{
	UnregisterClass( acInit_WindowClassName, hInit_ProcessHandle);
	CloseHandle(hInit_ProcessHandle),hInit_ProcessHandle=NULL;
}

int Initialize_CreateMainWindow(void)
{
	hInit_MainWindowHandle=CreateWindowEx( WS_EX_APPWINDOW|WS_EX_WINDOWEDGE,
		acInit_WindowClassName, "", WS_BORDER|WS_SYSMENU|WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, 128, 128, NULL, 0, hInit_ProcessHandle, 0);
	return(hInit_MainWindowHandle!=NULL? 0:-1);
}

void DeInitialize_DestroyMainWindow(void)
{
	if( hInit_MainWindowHandle!=NULL )
		DestroyWindow(hInit_MainWindowHandle),hInit_MainWindowHandle=NULL;
}

void Global_MessageLoop(void)
{
	MSG oMessage;
	
	while(GetMessage( &oMessage, 0, 0, 0)!=FALSE)
	{
		TranslateMessage(&oMessage);
		DispatchMessage(&oMessage);
	}
}

int handler()
{
	static char acInputRow[1024];

	if( SendMessage( HWND_BROADCAST, uiGlobal_MsgID_SkypeControlAPIDiscover, (WPARAM)hInit_MainWindowHandle, 0)!=0 )
	{
	}
	return 0;
}

main()
{
	uiGlobal_MsgID_SkypeControlAPIAttach=RegisterWindowMessage("SkypeControlAPIAttach");
	uiGlobal_MsgID_SkypeControlAPIDiscover=RegisterWindowMessage("SkypeControlAPIDiscover");
	Initialize_CreateWindowClass();
	Initialize_CreateMainWindow();
	hGlobal_ThreadShutdownEvent=CreateEvent( NULL, TRUE, FALSE, NULL);

	handler();
	while(1)
		Global_MessageLoop();
}
