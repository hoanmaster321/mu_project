///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN

#include <locale.h>
#if !defined(__ANDROID__) && !defined(MU_IOS)
#include <zmouse.h>
#endif
#include "UIWindows.h"
#include "UIManager.h"
#include "ZzzOpenglUtil.h"
#include "ZzzTexture.h"
#include "ZzzOpenData.h"
#include "ZzzScene.h"
#include "VulkanSDL3Context.h"
#include "GPUContext.h"
#include "VulkanTextureManager.h"
#include "ZzzBMD.h"
#include "ZzzInfomation.h"
#include "ZzzObject.h"
#include "ZzzCharacter.h"
#include "ZzzInterface.h"
#include "ZzzInventory.h"
#include "zzzLodTerrain.h"
#include "DSPlaySound.h"
#include "wsclientinline.h"
#include "Resource.h"
#include "zzzpath.h"
#include "Nprotect.h"
#include "Local.h"
#include "PersonalShopTitleImp.h"
#include "./Utilities/Log/ErrorReport.h"
#include "UIMapName.h"		// rozy
#include "./Utilities/Log/muConsoleDebug.h"
#include "ProtocolSend.h"
#include "CBTMessageBox.h"
#include "CSChaosCastle.h"
#include "GMHellas.h"
#include "Input.h"
#include "./Time/Timer.h"
#include "UIMng.h"
//==Load BCustom
#include "Protect.h"
#include "GameCensorship.h"
#include "NewUIMainFrameMobile.h"

#if !defined(__ANDROID__) && !defined(MU_IOS)
#include <imm.h>
#include <io.h>
#include "./ExternalObject/leaf/ExceptionHandler.h"
#include "./Utilities/Dump/CrashReporter.h"
#include "ProtectSysKey.h"
#include "./ExternalObject/leaf/regkey.h"
#include "nvapi.h"
#include "NvApiDriverSettings.h"
#endif

#include "w_MapHeaders.h"

#include "w_PetProcess.h"

#include <ThemidaInclude.h>

#include "MultiLanguage.h"

#include "PacketManager.h"
#include "BuffIcon.h"
#if(UseStackLog)
#include "Stack.h"
#endif
#include "MainLoad.h"
#include "TrayMode.h"
#include "CB_MUHelper.h"


CUIMercenaryInputBox * g_pMercenaryInputBox = NULL;
CUITextInputBox * g_pSingleTextInputBox = NULL;
CUITextInputBox * g_pSinglePasswdInputBox = NULL;
int g_iChatInputType = 1;
extern BOOL g_bIMEBlock;

CChatRoomSocketList * g_pChatRoomSocketList = NULL;

CMultiLanguage *pMultiLanguage = NULL;

extern DWORD g_dwTopWindow;

#ifdef MOVIE_DIRECTSHOW
CMovieScene*	g_pMovieScene = NULL;
#endif // MOVIE_DIRECTSHOW

CUIManager* g_pUIManager = NULL;
CUIMapName* g_pUIMapName = NULL;		// rozy

int Time_Effect = 0;
bool ashies = false;
int weather = rand()%3;

HWND      g_hWnd  = NULL;
HINSTANCE g_hInst = NULL;
HDC       g_hDC   = NULL;
HGLRC     g_hRC   = NULL;
HFONT     g_hFont = NULL;
HFONT     g_hFontBold = NULL;
HFONT     g_hFontBoldName = NULL;
HFONT     g_hFontBig = NULL;
HFONT     g_hFixFont = NULL;
HFONT     g_hFontMini= NULL;
CTimer*		g_pTimer = NULL;	// performance counter.
bool      Destroy = false;
bool      ActiveIME = false;

BYTE*				RendomMemoryDump;
ITEM_ATTRIBUTE*		ItemAttRibuteMemoryDump;
CHARACTER*			CharacterMemoryDump;

int       RandomTable[100];

char TextMu[]       = "mu.exe";

extern CErrorReport g_ErrorReport;

BOOL g_bMinimizedEnabled = FALSE;
int g_iScreenSaverOldValue = 60*15;

extern float g_fScreenRate_x;	// ¡Ø
extern float g_fScreenRate_y;
extern int DisplayWinCDepthBox;
extern int DisplayWin;
extern int DisplayHeight;
extern int DisplayWinMid;
extern int DisplayHeightExt;
extern int DisplayWinExt;
extern int DisplayWinReal;

#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
BOOL g_bUseWindowMode = TRUE;
#endif

char Mp3FileName[256];

#if !defined(__ANDROID__) && !defined(MU_IOS)
#pragma comment(lib, "wzAudio.lib")
#include <wzAudio.h>
#else
inline int wzAudioCreate(HWND) { return 1; }
inline int wzAudioDestroy() { return 1; }
inline int wzAudioPlay(const char*, int) { return 1; }
inline int wzAudioStop() { return 1; }
inline int wzAudioOption(int, int) { return 1; }
inline int wzAudioGetStreamOffsetRange() { return 0; }
inline int wzAudioSetVolume(int) { return 1; }
#define WZAOPT_STOPBEFOREPLAY 0
#endif

void StopMp3(char *Name, BOOL bEnforce)
{
    if(!m_MusicOnOff && !bEnforce) return;

	if(Mp3FileName[0] != NULL)
	{
		if(strcmp(Name, Mp3FileName) == 0) {
			wzAudioStop();
			Mp3FileName[0] = NULL;
		}
	}
}

void PlayMp3(char *Name, BOOL bEnforce )
{
	if(Destroy) return;
    if(!m_MusicOnOff && !bEnforce) return;

	if(strcmp(Name,Mp3FileName) == 0) 
	{
		return;
	}
	else 
	{
		wzAudioPlay(Name, 1);
		strcpy(Mp3FileName,Name);
	}
}

bool IsEndMp3()
{
	if (100 == wzAudioGetStreamOffsetRange())
		return true;
	return false;
}

int GetMp3PlayPosition()
{
	return wzAudioGetStreamOffsetRange();
}

extern int  LogIn;
extern char LogInID[];
extern BOOL g_bGameServerConnected;

void CheckHack( void)
{
	if (SceneFlag != MAIN_SCENE || !g_bGameServerConnected)
	{
		return;
	}
	#ifdef NEW_PROTOCOL_SYSTEM
		gProtocolSend.SendCheckOnline();
	#else
		SendCheck();
	#endif
#if defined(__ANDROID__) || defined(MU_IOS)
	g_ErrorReport.Write("[Heartbeat] Sent keepalive packet 0x0E to GameServer\r\n");
#endif
}

GLvoid KillGLWindow(GLvoid)								
{
	//if (g_hRC)
	//{
	//	if (!wglMakeCurrent(NULL,NULL))
	//	{
	//		g_ErrorReport.Write( "GL - Release Of DC And RC Failed\r\n");
	//		MessageBox(NULL,"Release Of DC And RC Failed.","Error",MB_OK | MB_ICONINFORMATION);
	//	}

	//	if (!wglDeleteContext(g_hRC))
	//	{
	//		g_ErrorReport.Write( "GL - Release Rendering Context Failed\r\n");
	//		MessageBox(NULL,"Release Rendering Context Failed.","Error",MB_OK | MB_ICONINFORMATION);
	//	}

	//	g_hRC=NULL;
	//}
	if (g_hRC)
	{
		g_hRC = NULL;
	}
	if (g_hDC && !ReleaseDC(g_hWnd,g_hDC))
	{
		g_ErrorReport.Write( "Release DC Error\r\n");
		MessageBox(NULL,"Release DC Error.","Error",MB_OK | MB_ICONINFORMATION);
		g_hDC=NULL;
	}

#if (defined WINDOWMODE)
	if (g_bUseWindowMode == FALSE)
	{
		ChangeDisplaySettings(NULL,0);
		ShowCursor(TRUE);
	}
#else
#ifdef ENABLE_FULLSCREEN
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
	if (g_bUseWindowMode == FALSE)
#endif	// USER_WINDOW_MODE
	{
		ChangeDisplaySettings(NULL,0);
		ShowCursor(TRUE);
	}
#endif //ENABLE_FULLSCREEN
#endif	//WINDOWMODE(#else)
}


BOOL GetFileNameOfFilePath( char *lpszFile, char *lpszPath)
{
	int iFind = ( int)'\\';
	char *lpFound = lpszPath;
	char *lpOld = lpFound;
	while ( lpFound)
	{
		lpOld = lpFound;
		lpFound = strchr( lpFound + 1, iFind);
	}

	if ( strchr( lpszPath, iFind))
	{
		strcpy( lpszFile, lpOld + 1);
	}
	else
	{
		strcpy( lpszFile, lpOld);
	}

	BOOL bCheck = TRUE;
	for ( char *lpTemp = lpszFile; bCheck; ++lpTemp)
	{
		switch ( *lpTemp)
		{
		case '\"':
		case '\\':
		case '/':
		case ' ':
			*lpTemp = '\0';
		case '\0':
			bCheck = FALSE;
			break;
		}
	}

	return ( TRUE);
}

HANDLE g_hMainExe = INVALID_HANDLE_VALUE;

BOOL OpenMainExe( void)
{
#if defined(_DEBUG) || defined(__ANDROID__) || defined(MU_IOS)
	return ( TRUE);
#else
	char lpszFile[MAX_PATH];
	char *lpszCommandLine = GetCommandLine();
	GetFileNameOfFilePath( lpszFile, lpszCommandLine);

	g_hMainExe = CreateFile( ( char*)lpszFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, 0);
	
	return ( INVALID_HANDLE_VALUE != g_hMainExe);
#endif
}

void CloseMainExe( void)
{
#if !defined(__ANDROID__) && !defined(MU_IOS)
	CloseHandle( g_hMainExe);
#endif
}

WORD DecryptCheckSumKey( WORD wSource)
{
	WORD wAcc = wSource ^ 0xB479;
	return ( ( wAcc >> 10) << 4) | ( wAcc & 0xF);
}

DWORD GenerateCheckSum( BYTE *pbyBuffer, DWORD dwSize, WORD wKey)
{
	DWORD dwKey = ( DWORD)wKey;
	DWORD dwResult = dwKey << 9;
	for ( DWORD dwChecked = 0; dwChecked <= dwSize - 4; dwChecked += 4)
	{
		DWORD dwTemp;
		memcpy( &dwTemp, pbyBuffer + dwChecked, sizeof ( DWORD));

		switch ( ( dwChecked / 4 + wKey) % 3)
		{
		case 0:
			dwResult ^= dwTemp;
			break;
		case 1:
			dwResult += dwTemp;
			break;
		case 2:
			dwResult <<= ( dwTemp % 11);
			dwResult ^= dwTemp;
			break;
		}

		if ( 0 == ( dwChecked % 4))
		{
			dwResult ^= ( ( dwKey + dwResult) >> ( ( dwChecked / 4) % 16 + 3));
		}
	}

	return ( dwResult);
}

DWORD GetCheckSum( WORD wKey)
{
	wKey = DecryptCheckSumKey( wKey);

	char lpszFile[MAX_PATH];

	strcpy( lpszFile, "data\\local\\Gameguard.csr");

	HANDLE hFile = CreateFile( ( char*)lpszFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if ( INVALID_HANDLE_VALUE == hFile)
	{
		return ( 0);
	}
	
	DWORD dwSize = GetFileSize( hFile, NULL);
	BYTE *pbyBuffer = new BYTE [dwSize];
	DWORD dwNumber;
	ReadFile( hFile, pbyBuffer, dwSize, &dwNumber, 0);
	CloseHandle( hFile);
	
	DWORD dwCheckSum = GenerateCheckSum(pbyBuffer, dwSize, wKey);
	delete [] pbyBuffer;
	
	return (dwCheckSum);
}


BOOL GetFileVersion( char *lpszFileName, WORD *pwVersion)
{
	DWORD dwHandle;
	DWORD dwLen = GetFileVersionInfoSize( lpszFileName, &dwHandle);
	if ( dwLen <= 0)
	{
		return ( FALSE);
	}

	BYTE *pbyData = new BYTE [dwLen];
	if ( !GetFileVersionInfo( lpszFileName, dwHandle, dwLen, pbyData))
	{
		delete [] pbyData;
		return ( FALSE);
	}

	VS_FIXEDFILEINFO *pffi;
	UINT uLen;
	if ( !VerQueryValue( pbyData, "\\", ( LPVOID*)&pffi, &uLen))
	{
		delete [] pbyData;
		return ( FALSE);
	}

	pwVersion[0] = HIWORD( pffi->dwFileVersionMS);
	pwVersion[1] = LOWORD( pffi->dwFileVersionMS);
	pwVersion[2] = HIWORD( pffi->dwFileVersionLS);
	pwVersion[3] = LOWORD( pffi->dwFileVersionLS);

	delete [] pbyData;
	return ( TRUE);
}

extern PATH     *path;

void DestroyWindow()
{
	LogOut = true;
	//. save volume level
#if !defined(__ANDROID__) && !defined(MU_IOS)
	leaf::CRegKey regkey;
	regkey.SetKey(leaf::CRegKey::_HKEY_CURRENT_USER, "SOFTWARE\\Webzen\\Mu\\Config");
	regkey.WriteDword("VolumeLevel", g_pOption->GetVolumeLevel());
#endif
	char string[10];
	WritePrivateProfileStringA("Custom", "ShowName", itoa(mShowName, string, 10) , "./config.ini");
	WritePrivateProfileStringA("Custom", "ShowHPBar", itoa(mShowHPBar, string, 10), "./config.ini");
	WritePrivateProfileStringA("Custom", "ShowMiniMap", itoa(mShowMiniMap, string, 10), "./config.ini");
	WritePrivateProfileStringA("Custom", "ShowDanhHieu", itoa(mShowDanhHieu, string, 10), "./config.ini");
	//===
	WritePrivateProfileStringA("Graphics", "GlowEffect", itoa(g_pNewUISystem->GetUI_NewOptionWindow()->OnOffGrap[g_pNewUISystem->GetUI_NewOptionWindow()->eGlowEffect], string, 10), "./config.ini");
	WritePrivateProfileStringA("Graphics", "EffectDynamic", itoa(g_pNewUISystem->GetUI_NewOptionWindow()->OnOffGrap[g_pNewUISystem->GetUI_NewOptionWindow()->eEffectDynamic], string, 10), "./config.ini");
	WritePrivateProfileStringA("Graphics", "EffectStatic", itoa(g_pNewUISystem->GetUI_NewOptionWindow()->OnOffGrap[g_pNewUISystem->GetUI_NewOptionWindow()->eEffectStatic], string, 10), "./config.ini");
	WritePrivateProfileStringA("Graphics", "BMDPlayer", itoa(g_pNewUISystem->GetUI_NewOptionWindow()->OnOffGrap[g_pNewUISystem->GetUI_NewOptionWindow()->eBMDPlayer], string, 10), "./config.ini");
	WritePrivateProfileStringA("Graphics", "BMDWings", itoa(g_pNewUISystem->GetUI_NewOptionWindow()->OnOffGrap[g_pNewUISystem->GetUI_NewOptionWindow()->eBMDWings], string, 10), "./config.ini");
	WritePrivateProfileStringA("Graphics", "BMDWeapons", itoa(g_pNewUISystem->GetUI_NewOptionWindow()->OnOffGrap[g_pNewUISystem->GetUI_NewOptionWindow()->eBMDWeapons], string, 10), "./config.ini");
	WritePrivateProfileStringA("Graphics", "BMDImg", itoa(g_pNewUISystem->GetUI_NewOptionWindow()->OnOffGrap[g_pNewUISystem->GetUI_NewOptionWindow()->eBMDImg], string, 10), "./config.ini");
	WritePrivateProfileStringA("Graphics", "BMDMonter", itoa(g_pNewUISystem->GetUI_NewOptionWindow()->OnOffGrap[g_pNewUISystem->GetUI_NewOptionWindow()->eBMDMonter], string, 10), "./config.ini");
	WritePrivateProfileStringA("Graphics", "RenderObjects", itoa(g_pNewUISystem->GetUI_NewOptionWindow()->OnOffGrap[g_pNewUISystem->GetUI_NewOptionWindow()->eRenderObjects], string, 10), "./config.ini");
	WritePrivateProfileStringA("Graphics", "RenderTerrain", itoa(g_pNewUISystem->GetUI_NewOptionWindow()->OnOffGrap[g_pNewUISystem->GetUI_NewOptionWindow()->eRenderTerrain], string, 10), "./config.ini");
	WritePrivateProfileStringA("Graphics", "ExcellentEffect", itoa(g_pNewUISystem->GetUI_NewOptionWindow()->OnOffGrap[g_pNewUISystem->GetUI_NewOptionWindow()->eExcellentEffect], string, 10), "./config.ini");
	WritePrivateProfileStringA("Graphics", "BMDZen", itoa(g_pNewUISystem->GetUI_NewOptionWindow()->OnOffGrap[g_pNewUISystem->GetUI_NewOptionWindow()->eBMDZen], string, 10), "./config.ini");


	CUIMng::Instance().Release();

#ifdef MOVIE_DIRECTSHOW
	if(g_pMovieScene)
	{
		g_pMovieScene->Destroy();
	}
#endif // MOVIE_DIRECTSHOW

	//. release font handle
	if(g_hFont)::DeleteObject((HGDIOBJ)g_hFont);

	if(g_hFontBold)::DeleteObject((HGDIOBJ)g_hFontBold);

	if(g_hFontBig)::DeleteObject((HGDIOBJ)g_hFontBig);

	if(g_hFixFont)::DeleteObject((HGDIOBJ)g_hFixFont);

	if (g_hFontMini)::DeleteObject((HGDIOBJ)g_hFixFont);

	if (g_hFontBoldName)::DeleteObject((HGDIOBJ)g_hFontBoldName);
	
	ReleaseCharacters();

    if ( path!=NULL )
    {
	    delete path;
    }
	SAFE_DELETE(GateAttribute);

	//for ( int i = 0; i < MAX_SKILLS; ++i)
	//{
	//}
	SAFE_DELETE(SkillAttribute);

	SAFE_DELETE(CharacterMachine);

    DeleteWaterTerrain ();

#ifdef MOVIE_DIRECTSHOW
	if(SceneFlag != MOVIE_SCENE)
#endif // MOVIE_DIRECTSHOW
	{
		gMapManager.DeleteObjects();

		// Object.
		for(int i=MODEL_LOGO;i<MAX_MODELS;i++)
		{
			Models[i].Release();
		}

		// Bitmap
		Bitmaps.UnloadAllImages();
	}

	SAFE_DELETE_ARRAY( CharacterMemoryDump );
	SAFE_DELETE_ARRAY( ItemAttRibuteMemoryDump );
	SAFE_DELETE_ARRAY( RendomMemoryDump );
	SAFE_DELETE_ARRAY( ModelsDump );
	
#ifdef DYNAMIC_FRUSTRUM
	DeleteAllFrustrum();
#endif //DYNAMIC_FRUSTRUM

	SAFE_DELETE(g_pMercenaryInputBox);
	SAFE_DELETE(g_pSingleTextInputBox);
	SAFE_DELETE(g_pSinglePasswdInputBox);

	SAFE_DELETE(g_pChatRoomSocketList);
	SAFE_DELETE(g_pUIMapName);	// rozy
	SAFE_DELETE( g_pTimer );
	SAFE_DELETE(g_pUIManager);
	 
#ifdef MOVIE_DIRECTSHOW
	SAFE_DELETE(g_pMovieScene);
#endif // MOVIE_DIRECTSHOW

	SAFE_DELETE(pMultiLanguage);
	BoostRest( g_BuffSystem );
	BoostRest( g_MapProcess );
	BoostRest( g_petProcess );

	g_ErrorReport.Write( "Destroy" );
	 
	HWND shWnd = FindWindow(NULL, "MuPlayer");
	if(shWnd)
		SendMessage(shWnd, WM_DESTROY, 0, 0);
}
void DestroySound()
{
	for(int i=0;i<MAX_BUFFER;i++)
		ReleaseBuffer(i);

	FreeDirectSound();
	wzAudioDestroy();
}

int g_iInactiveTime = 0;
int g_iNoMouseTime = 0;
int g_iInactiveWarning = 0;
bool g_bWndActive = false;
bool HangulDelete = false;
int Hangul = 0;
bool g_bEnterPressed = false;

int g_iMousePopPosition_x = 0;
int g_iMousePopPosition_y = 0;

extern int TimeRemain;
extern bool EnableFastInput;
void MainScene(HDC hDC);

LONG FAR PASCAL WndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	MOUSEHOOKSTRUCTEX* Mouse = (MOUSEHOOKSTRUCTEX*)lParam;
    switch (msg)
    {
	case WM_SYSKEYDOWN:
		{
			return 0;
		}
		break;
#if defined PROTECT_SYSTEMKEY && defined NDEBUG
#ifndef FOR_WORK
	case WM_SYSCOMMAND:
		{
			if(wParam == SC_KEYMENU || wParam == SC_SCREENSAVE)
			{
				return 0;
			}
		}
		break;
#endif // !FOR_WORK
#endif // PROTECT_SYSTEMKEY && NDEBUG
    case WM_ACTIVATE:
		if(LOWORD(wParam) == WA_INACTIVE)
		{
#ifdef ACTIVE_FOCUS_OUT
			if (g_bUseWindowMode == FALSE)
#endif	// ACTIVE_FOCUS_OUT
				g_bWndActive = false;
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
			if (g_bUseWindowMode == TRUE)
			{
				MouseLButton = false;
				MouseLButtonPop = false;
				//MouseLButtonPush = false;
				MouseRButton = false;
				MouseRButtonPop = false;
				MouseRButtonPush = false;
				MouseLButtonDBClick = false;
				MouseMButton = false;
				MouseMButtonPop = false;
				MouseMButtonPush = false;
				MouseWheel = 0;
			}
#endif
		}
		else
		{
			g_bWndActive = true;
		}
		break;
	case WM_TIMER :
		//MessageBox(NULL,GlobalText[16],"Error",MB_OK);
		switch( wParam )
		{
		case HACK_TIMER:
			// PKD_ADD_BINARY_PROTECTION
			VM_START
			CheckHack();
			VM_END
			break;
		case WINDOWMINIMIZED_TIMER:
			PostMessage(g_hWnd, WM_CLOSE, 0, 0);
			break;
		case CHATCONNECT_TIMER:
			g_pFriendMenu->SendChatRoomConnectCheck();
			break;
		case SLIDEHELP_TIMER:
			if(g_bWndActive)
			{
				if(g_pSlideHelpMgr)
					g_pSlideHelpMgr->CreateSlideText();
			}
			break;
		}
		break;
	case WM_USER_MEMORYHACK:
		//SetTimer( g_hWnd, WINDOWMINIMIZED_TIMER, 1*1000, NULL);
		KillGLWindow();
		break;
	case WM_NPROTECT_EXIT_TWO:
		SendHackingChecked( 0x04, 0);
		SetTimer( g_hWnd, WINDOWMINIMIZED_TIMER, 1*1000, NULL);
		MessageBox(NULL,GlobalText[16],"Error",MB_OK);
		break;
	case WM_ASYNCSELECTMSG :
		switch( WSAGETSELECTEVENT( lParam ) )
		{
		case FD_CONNECT :
			break;
		case FD_READ :
			SocketClient.nRecv();
			break;
		case FD_WRITE :
			SocketClient.FDWriteSend();
			break;
		case FD_CLOSE :
			g_pChatListBox->AddText("", GlobalText[3], SEASON3B::TYPE_SYSTEM_MESSAGE);
#ifdef CONSOLE_DEBUG
			switch(WSAGETSELECTERROR(lParam))
			{
			case WSAECONNRESET:
				g_ConsoleDebug->Write(MCD_ERROR, "The connection was reset by the remote side.");
				g_ErrorReport.Write("The connection was reset by the remote side.\r\n");
				g_ErrorReport.WriteCurrentTime();
				break;
			case WSAECONNABORTED:
				g_ConsoleDebug->Write(MCD_ERROR, "The connection was terminated due to a time-out or other failure.");
				g_ErrorReport.Write("The connection was terminated due to a time-out or other failure.\r\n");
				g_ErrorReport.WriteCurrentTime();
				break;
			}
#endif // CONSOLE_DEBUG
			SocketClient.Close();

			#ifdef NEW_PROTOCOL_SYSTEM
				gProtocolSend.DisconnectServer();
			#endif	

			CUIMng::Instance().PopUpMsgWin(MESSAGE_SERVER_LOST);
			break;
		}
		break;
	case WM_CTLCOLOREDIT:
		SetBkColor((HDC)wParam, RGB(0, 0, 0));
		SetTextColor((HDC)wParam, RGB(255, 255, 255));
		return (LRESULT)GetStockObject(BLACK_BRUSH);
		break;
	case WM_ERASEBKGND:
		return TRUE;
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps ;
			HDC hDC = BeginPaint(hwnd, &ps);
			EndPaint(hwnd, &ps) ;
		}
		return 0;
		break;
	case WM_DESTROY:
		{
			Destroy = true;
			SocketClient.Close();

			#ifdef NEW_PROTOCOL_SYSTEM
				gProtocolSend.DisconnectServer();
			#endif	

			DestroySound();
			DestroyWindow();
			KillGLWindow();	
			CloseMainExe();
			ExitProcess(0);
			//PostQuitMessage(0);
		}
		break;
    case WM_SETCURSOR:
        ShowCursor(false);
		break;
#if (defined WINDOWMODE)
	case WM_SIZE:
		if ( SIZE_MINIMIZED == wParam && g_bUseWindowMode == FALSE )
		{
			if ( !( g_bMinimizedEnabled))
			{
				DWORD dwMess[SIZE_ENCRYPTION_KEY];
				for ( int i = 0; i < SIZE_ENCRYPTION_KEY; ++i)
				{
					dwMess[i] = GetTickCount();
				}
				g_SimpleModulusCS.LoadKeyFromBuffer( ( BYTE*)dwMess, FALSE, FALSE, FALSE, TRUE);
			}
		}
		break;
#else
#ifdef NDEBUG
#ifndef FOR_WORK
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
	case WM_SIZE:
		if ( SIZE_MINIMIZED == wParam
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
			&& g_bUseWindowMode == FALSE
#endif
			)
		{
			if ( !( g_bMinimizedEnabled))
			{
				SendHackingChecked( 0x05, 0);
				DWORD dwMess[SIZE_ENCRYPTION_KEY];
				for ( int i = 0; i < SIZE_ENCRYPTION_KEY; ++i)
				{
					dwMess[i] = GetTickCount();
				}
				g_SimpleModulusCS.LoadKeyFromBuffer( ( BYTE*)dwMess, FALSE, FALSE, FALSE, TRUE);
			}
		}
		break;
#endif
#endif
#endif
#endif	//WINDOWMODE(#else)
//-----------------------------
	default:
		if (msg >= WM_CHATROOMMSG_BEGIN && msg < WM_CHATROOMMSG_END)
			g_pChatRoomSocketList->ProcessSocketMessage(msg - WM_CHATROOMMSG_BEGIN, WSAGETSELECTEVENT(lParam));
		break;
	}

	MouseLButtonDBClick = false;
	if (MouseLButtonPop == true && (g_iMousePopPosition_x != MouseX || g_iMousePopPosition_y != MouseY)) 
		MouseLButtonPop = false;
    switch (msg)
	{
	case WM_MOUSEMOVE:
		{
			MouseX = (float)LOWORD(lParam) / g_fScreenRate_x; //g_fScreenRate_x
			MouseY = (float)HIWORD(lParam) / g_fScreenRate_y;
			if(MouseX < 0) 
				MouseX = 0;
			if(MouseX > DisplayWin) //Fix Wide
				MouseX = DisplayWin;
			if(MouseY < 0) 
				MouseY = 0;
			if(MouseY > DisplayHeight)
				MouseY = DisplayHeight;
		}
		break;
	case WM_LBUTTONDOWN:
		g_iNoMouseTime = 0;
		MouseLButtonPop = false;
		if(!MouseLButton) 
			MouseLButtonPush = true;
		MouseLButton = true;
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
		SetCapture(g_hWnd);
#endif
		break;
	case WM_LBUTTONUP: //514
		g_iNoMouseTime = 0;
		MouseLButtonPush = false;
		//if(MouseLButton) MouseLButtonPop = true;
		MouseLButtonPop = true;
		MouseLButton = false;
		g_iMousePopPosition_x = MouseX;
		g_iMousePopPosition_y = MouseY;
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
		ReleaseCapture();
#endif
		break;
	case WM_RBUTTONDOWN:
		g_iNoMouseTime = 0;
		MouseRButtonPop = false;
		if(!MouseRButton) MouseRButtonPush = true;
		MouseRButton = true;
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
		SetCapture(g_hWnd);
#endif
		break;
	case WM_RBUTTONUP:
		g_iNoMouseTime = 0;
		MouseRButtonPush = false;
		if(MouseRButton) MouseRButtonPop = true;
		MouseRButton = false;
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
		ReleaseCapture();
#endif
		break;
    case WM_LBUTTONDBLCLK:
		g_iNoMouseTime = 0;
		MouseLButtonDBClick = true;
		break;
	case WM_MBUTTONDOWN:
		Camera3DSetMove = true;
		g_iNoMouseTime = 0;
		MouseMButtonPop = false;
		if(!MouseMButton) MouseMButtonPush = true;
		MouseMButton = true;
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
		SetCapture(g_hWnd);
#endif
		break;
	case WM_MBUTTONUP:
		Camera3DSetMove = false;
		g_iNoMouseTime = 0;
		MouseMButtonPush = false;
		if(MouseMButton) MouseMButtonPop = true;
		MouseRButton = false;
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
		ReleaseCapture();
#endif
		break;
	case WM_MOUSEWHEEL:
        {
            
			//==Camera 3D

			//if(!Camera3DEnable)
			
			MouseWheel = (short)HIWORD(wParam) / WHEEL_DELTA;
			
			if (Camera3DEnable && Zoom3D == 0 && GetTickCount() > BlockMouseWheel)
			{
				if (((short)HIWORD(wParam) / WHEEL_DELTA) > 0)
				{
					//if(CameraFOV < 95.f) Zoom3D += 5;
					if (CameraFOV < gProtect.m_MainInfo.ZoomMax) Zoom3D += 5;
				}
				else
				{
					if (CameraFOV > gProtect.m_MainInfo.ZoomMin) Zoom3D -= 5;
				}
			}
        }
        break;
	case WM_IME_NOTIFY:
		{
			if (g_iChatInputType == 1)
			{
				switch (wParam)
				{
				case IMN_SETCONVERSIONMODE:
					if (GetFocus() == g_hWnd)
					{
						CheckTextInputBoxIME(IME_CONVERSIONMODE);
					}
					break;
				case IMN_SETSENTENCEMODE:
					if (GetFocus() == g_hWnd)
					{
						CheckTextInputBoxIME(IME_SENTENCEMODE);
					}
					break;
				default:
					break;
				}
			}
		}
		break;
	case WM_CHAR:
		{
			switch(wParam)
			{
			case VK_RETURN:
				{
					SetEnterPressed( true );
				}
				break;
			}
		}
		break;
    }
	if( g_BuffSystem ) {
		LRESULT result;
		TheBuffStateSystem().HandleWindowMessage( msg, wParam, lParam, result );
	}

    return DefWindowProc(hwnd,msg,wParam,lParam);
}

#if defined(__ANDROID__) || defined(MU_IOS)
static void UpdateScreenMetrics(int screenW, int screenH)
{
	if (screenW < screenH)
	{
		std::swap(screenW, screenH);
	}
	WindowWidth = static_cast<unsigned int>(screenW);
	WindowHeight = static_cast<unsigned int>(screenH);
	const float scale = (screenH > 0) ? (static_cast<float>(screenH) / 480.0f) : 1.0f;
	g_fScreenRate_x = scale;
	g_fScreenRate_y = scale;
	DisplayHeight = 480;
	DisplayHeightExt = 0;
	DisplayWin = (scale > 0.0f) ? static_cast<int>(static_cast<float>(WindowWidth) / scale) : 640;
	DisplayWinMid = DisplayWin / 2;
	DisplayWinExt = static_cast<int>((DisplayWin * 0.5f) - 320.0f);
	DisplayWinReal = static_cast<int>(DisplayWin / 640.0f);
	DisplayWinCDepthBox = DisplayWin - 640;
}
#endif

bool CreateOpenglWindow()
{
#if !defined(__ANDROID__) && !defined(MU_IOS)
	if (!(g_hDC=GetDC(g_hWnd)))
	{
		g_ErrorReport.Write( "Get DC Error - ErrorCode : %d\r\n", GetLastError());
		KillGLWindow();
		MessageBox(NULL,GlobalText[4],"Get DC Error.",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	ShowWindow(g_hWnd,SW_SHOW);
	SetForegroundWindow(g_hWnd);
	SetFocus(g_hWnd);
#else
	g_hDC = (HDC)1;
#endif

	// Initialize Native SDL3 + Vulkan Context
#if defined(__ANDROID__)
	if (WindowWidth < WindowHeight)
	{
		std::swap(WindowWidth, WindowHeight);
	}
#endif
	if (VulkanSDL3Context::Instance().Init("MU Online", WindowWidth, WindowHeight, false))
	{
#if defined(__ANDROID__)
		int pxW = 0, pxH = 0;
		if (SDL_GetWindowSizeInPixels(VulkanSDL3Context::Instance().GetWindow(), &pxW, &pxH) && pxW > 0 && pxH > 0)
		{
			UpdateScreenMetrics(pxW, pxH);
			g_ErrorReport.Write("[Android] Screen resolution detected: %dx%d (scale: %.2f, DisplayWin: %d)\r\n", WindowWidth, WindowHeight, g_fScreenRate_x, DisplayWin);
		}
#endif
		GPUContext::Instance().Init(VulkanSDL3Context::Instance().GetWindow(), WindowWidth, WindowHeight);
#if defined(__ANDROID__)
		const auto& swapExtent = GPUContext::Instance().GetSwapchainExtent();
		if (swapExtent.width > 0 && swapExtent.height > 0)
		{
			UpdateScreenMetrics(swapExtent.width, swapExtent.height);
			g_ErrorReport.Write("[Android] Synced resolution with Vulkan swapchain: %dx%d (scale: %.2f, DisplayWin: %d)\r\n", WindowWidth, WindowHeight, g_fScreenRate_x, DisplayWin);
		}
#endif
	}
	return true;
}
#if !defined(__ANDROID__) && !defined(MU_IOS)
#include <shobjidl.h>
#endif
void SetUniqueAppID() {
#if !defined(__ANDROID__) && !defined(MU_IOS)
	srand(static_cast<unsigned int>(time(nullptr)));
	int randomID = rand();
	std::wstringstream appIDStream;
	appIDStream << L"MyApp.UniqueWindowID" << randomID;
	SetCurrentProcessExplicitAppUserModelID(appIDStream.str().c_str());
#endif
}
HWND StartWindow(HINSTANCE hCurrentInst,int nCmdShow)
{
#if defined(__ANDROID__) || defined(MU_IOS)
    return (HWND)1;
#else
   char *windowName = gProtect.m_MainInfo.WindowName;
   //SetUniqueAppID(); //

    WNDCLASS wndClass;
    HWND hWnd;
	
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc   = WndProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = 0;
    wndClass.hInstance     = hCurrentInst;
    wndClass.hIcon		   = LoadIcon(hCurrentInst, (LPCTSTR)IDI_ICON1);
    wndClass.hCursor	   = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndClass.lpszMenuName  = NULL;
    wndClass.lpszClassName = windowName;
    RegisterClass(&wndClass);

	gTrayMode.Init(g_hInst);

#ifdef ENABLE_FULLSCREEN
	{
#ifndef FOR_WORK
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
		if (g_bUseWindowMode == TRUE)
		{
			RECT rc = { 0, 0, WindowWidth, WindowHeight };
#if defined WINDOWMODE
			if (g_bUseWindowMode == TRUE)
				AdjustWindowRect(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_BORDER | WS_CLIPCHILDREN, NULL);
			else
#endif	//WINDOWMODE
			AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, NULL);
			rc.right -= rc.left;
			rc.bottom -= rc.top;

#if defined WINDOWMODE
			if (g_bUseWindowMode == TRUE)
			{
			hWnd = CreateWindow(
				windowName, windowName,
				WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_BORDER | WS_CLIPCHILDREN,
				(GetSystemMetrics(SM_CXSCREEN) - rc.right) / 2,
				(GetSystemMetrics(SM_CYSCREEN) - rc.bottom) / 2,
				rc.right,
				rc.bottom,
				NULL, NULL, hCurrentInst, NULL);
			}
			else
#endif//WINDOWMODE
			{
			hWnd = CreateWindow(
				windowName, windowName,
				WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
				(GetSystemMetrics(SM_CXSCREEN) - rc.right) / 2,
				(GetSystemMetrics(SM_CYSCREEN) - rc.bottom) / 2,
				rc.right,
				rc.bottom,
				NULL, NULL, hCurrentInst, NULL);
			}
		}
		else
#endif//WINDOWMODE
		{
			hWnd = CreateWindowEx( WS_EX_TOPMOST | WS_EX_APPWINDOW,
				windowName, windowName,
				WS_POPUP,
				0, 0,
				WindowWidth,
				WindowHeight,
				NULL, NULL, hCurrentInst, NULL);
		}
#else //FOR_WORK
		hWnd = CreateWindow(
			windowName, windowName,
			WS_POPUP,
			0, 0,
			WindowWidth,
			WindowHeight,
			NULL, NULL, hCurrentInst, NULL);
#endif
	}
#else //ENABLE_FULLSCREEN
	{
		RECT rc = { 0, 0, WindowWidth, WindowHeight };
#if defined WINDOWMODE
		if (g_bUseWindowMode == TRUE)
			AdjustWindowRect(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_BORDER | WS_CLIPCHILDREN, NULL);
		else
#endif
		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, NULL);
		rc.right -= rc.left;
		rc.bottom -= rc.top;
#if defined WINDOWMODE
		if (g_bUseWindowMode == TRUE)
		{
		hWnd = CreateWindow(
			windowName, windowName,
			WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_BORDER | WS_CLIPCHILDREN,
			(GetSystemMetrics(SM_CXSCREEN) - rc.right) / 2,
			(GetSystemMetrics(SM_CYSCREEN) - rc.bottom) / 2,
			rc.right,
			rc.bottom,
			NULL, NULL, hCurrentInst, NULL);
		}
		else
#endif
		{
		hWnd = CreateWindow(
			windowName, windowName,
			WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
			(GetSystemMetrics(SM_CXSCREEN) - rc.right) / 2,
			(GetSystemMetrics(SM_CYSCREEN) - rc.bottom) / 2,
			rc.right,
			rc.bottom,
			NULL, NULL, hCurrentInst, NULL);
		}
	}
#endif //ENABLE_FULLSCREEN
	
    return hWnd;
#endif
}

char m_ID[11];
char m_Username[11] = {};
char m_Password[11] = {};
char m_Pass[21];
char m_Version[11];
char m_ExeVersion[11];
int  m_SavePassOnOff;
int  m_SoundOnOff;
int  m_MusicOnOff;
int  m_Resolution;
int	m_nColorDepth;
int	g_iRenderTextType = 0;

char m_FontName[30];
int m_FontSizePlus = 0;

int mShowName;
int mShowHPBar;
int mShowMiniMap;
int mShowDanhHieu;

char g_aszMLSelection[MAX_LANGUAGE_NAME_LENGTH] = {'\0'};
std::string g_strSelectedML = "";

BOOL OpenInitFile()
{
	//char szTemp[50];
	char szIniFilePath[256+20]="";
	char szCurrentDir[256];

	GetCurrentDirectory(256, szCurrentDir);

	strcpy(szIniFilePath, szCurrentDir);
	if( szCurrentDir[strlen(szCurrentDir)-1] == '\\' ) 
		strcat(szIniFilePath, "config.ini");
	else strcat(szIniFilePath, "\\config.ini");

	GetPrivateProfileString ("LOGIN", "Version", "", m_Version, 11, szIniFilePath);

	char *lpszCommandLine = GetCommandLine();
	char lpszFile[MAX_PATH];
	if ( GetFileNameOfFilePath( lpszFile, lpszCommandLine))
	{
		WORD wVersion[4];
		if ( GetFileVersion( lpszFile, wVersion))
		{
			sprintf( m_ExeVersion, "%d.%02d", wVersion[0], wVersion[1]);
			if ( wVersion[2] > 0)
			{
                char lpszMinorVersion[3] = "a";
                if ( wVersion[2]>26 )
                {
                    lpszMinorVersion[0] = 'A';
				    lpszMinorVersion[0] += ( wVersion[2] - 27 );
                    lpszMinorVersion[1] = '+';
                }
                else
                {
				    lpszMinorVersion[0] += ( wVersion[2] - 1);
                }
				strcat( m_ExeVersion, lpszMinorVersion);
			}
		}
		else
		{
			strcpy( m_ExeVersion, m_Version);
		}
	}
	else
	{
		strcpy( m_ExeVersion, m_Version);
	}
	//==== Custom Font
	GetPrivateProfileString("FONT", "FontName", "", m_FontName, 30, szIniFilePath);
	m_FontSizePlus = GetPrivateProfileInt("FONT", "FontSizeBonus", 0, szIniFilePath);

	//===Custom Config
	mShowName = GetPrivateProfileInt("Custom", "ShowName", 0, szIniFilePath);
	mShowHPBar = GetPrivateProfileInt("Custom", "ShowHPBar", 0, szIniFilePath);
	mShowMiniMap = GetPrivateProfileInt("Custom", "ShowMiniMap", 0, szIniFilePath);
	mShowDanhHieu = GetPrivateProfileInt("Custom", "ShowDanhHieu", 0, szIniFilePath);



	g_bGMObservation = mShowName;
//#ifdef _DEBUG

	m_ID[0] = '\0';
	m_Pass[0] = '\0';
	m_SavePassOnOff = 0;
	m_SoundOnOff = 1;
	m_MusicOnOff = 1;
	m_Resolution = 0;
	m_nColorDepth = 0;

	HKEY hKey;
	DWORD dwDisp;
	DWORD dwSize;
	if ( ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Webzen\\Mu\\Config", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, & hKey, &dwDisp))
	{
		dwSize = 11;
		if ( RegQueryValueEx (hKey, "ID", 0, NULL, (LPBYTE)m_ID, & dwSize) != ERROR_SUCCESS)
		{
		}

		dwSize = sizeof(int);
		if ( RegQueryValueEx (hKey, "SoundOnOff", 0, NULL, (LPBYTE) & m_SoundOnOff, &dwSize) != ERROR_SUCCESS)
		{
			m_SoundOnOff = true;
		}
		dwSize = sizeof ( int);
		if ( RegQueryValueEx (hKey, "MusicOnOff", 0, NULL, (LPBYTE) & m_MusicOnOff, &dwSize) != ERROR_SUCCESS)
		{
			m_MusicOnOff = false;
		}
		dwSize = sizeof ( int);
		if ( RegQueryValueEx (hKey, "Resolution", 0, NULL, (LPBYTE) & m_Resolution, &dwSize) != ERROR_SUCCESS)
			m_Resolution = 1;

		if (0 == m_Resolution)
			m_Resolution = 3;

	    if ( RegQueryValueEx (hKey, "ColorDepth", 0, NULL, (LPBYTE) & m_nColorDepth, &dwSize) != ERROR_SUCCESS)
	    {
		    m_nColorDepth = 0;
	    }
		dwSize = sizeof ( int);
		if ( RegQueryValueEx (hKey, "TextOut", 0, NULL, (LPBYTE) & g_iRenderTextType, &dwSize) != ERROR_SUCCESS)
		{
			g_iRenderTextType = 0;
		}

		g_iChatInputType = 1;

#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
		dwSize = sizeof ( int);
		if ( RegQueryValueEx (hKey, "WindowMode", 0, NULL, (LPBYTE) & g_bUseWindowMode, &dwSize) != ERROR_SUCCESS)
		{
			g_bUseWindowMode = FALSE;
		}
#endif // USER_WINDOW_MODE

		dwSize = MAX_LANGUAGE_NAME_LENGTH;
		if ( RegQueryValueEx (hKey, "LangSelection", 0, NULL, (LPBYTE)g_aszMLSelection, &dwSize) != ERROR_SUCCESS)
		{
			strcpy(g_aszMLSelection, "Eng");
		}
		g_strSelectedML = g_aszMLSelection;
	}
	RegCloseKey( hKey);

#if(WIDE_SCREEN)
	float GetPos = 0.0f;
	switch (m_Resolution) //Resolution
	{
	case 0:WindowWidth = 640; WindowHeight = 480;	GetPos = 1.25f; break;			 
	case 1:WindowWidth = 800; WindowHeight = 600;	GetPos = 1.25f; break;			 
	case 2:WindowWidth = 1024; WindowHeight = 640;	GetPos = 1.25f; break;			 
	case 3:WindowWidth = 1280; WindowHeight = 800;	GetPos = 1.50f; break;			 
	case 4:WindowWidth = 1366; WindowHeight = 768;	GetPos = 1.60f; break;			 
	case 5:WindowWidth = 1440; WindowHeight = 900;	GetPos = 1.69f; break;			 
	case 6:WindowWidth = 1536; WindowHeight = 960;	GetPos = 1.87f; break;			 
	case 7:WindowWidth = 1680; WindowHeight = 1050; GetPos = 2.10f; break;			 
	case 8:WindowWidth = 1920; WindowHeight = 1200; GetPos = 2.10f; break;			 
	case 9:WindowWidth = 2560; WindowHeight = 1600; GetPos = 2.10f; break;			 
	case 10:WindowWidth = 3840; WindowHeight = 2400; GetPos = 2.10f; break;			 
	default:WindowWidth = 1920; WindowHeight = 1200; GetPos = 1.20f; break;			 
		//case 3:WindowWidth=1920;WindowHeight=1440;break;
		//case 4:WindowWidth=1600;WindowHeight=1200;break;
	}
	g_fScreenRate_x = GetPos;
	g_fScreenRate_y = GetPos;

	DisplayWinCDepthBox = (int)WindowWidth / g_fScreenRate_y - 640;
	DisplayWin = (int)WindowWidth / g_fScreenRate_y;
	DisplayWinMid = (int)WindowWidth / g_fScreenRate_y / 2;
	//DisplayWinExt = (int)WindowWidth / g_fScreenRate_y / 2 - 320;
	DisplayWinExt = (int)((DisplayWin * 0.5) - 320.0);
	DisplayWinReal = (int)DisplayWin / 640.0;

	DisplayHeight = (int)((double)WindowHeight / g_fScreenRate_y);
	DisplayHeightExt = (int)((double)DisplayHeight - 480.0);

#else
	switch (m_Resolution)
	{
	case 0:
		WindowWidth = 640;
		WindowHeight = 480;
		break;
	case 1:
		WindowWidth = 800;
		WindowHeight = 600;
		break;
	case 2:
		WindowWidth = 1024;
		WindowHeight = 768;
		break;
	case 3:
		WindowWidth = 1280;
		WindowHeight = 768;
		break;
	case 4:
		WindowWidth = 1366;
		WindowHeight = 768;
		break;
	case 5:
		WindowWidth = 1440;
		WindowHeight = 900;
		break;
	case 6:
		WindowWidth = 1600;
		WindowHeight = 900;
		break;
	case 7:
		WindowWidth = 1600;
		WindowHeight = 1280;
		break;
	case 8:
		WindowWidth = 1680;
		WindowHeight = 1050;
		break;
	case 9:
		WindowWidth = 1920;
		WindowHeight = 1080;
		break;
	case 10:
		WindowWidth = 2560;
		WindowHeight = 1440;
		break;
	default:
		WindowWidth = 640;
		WindowHeight = 480;
		break;
	}

	g_fScreenRate_x = (float)WindowWidth / 640;
	g_fScreenRate_y = (float)WindowHeight / 480;
	DisplayWin = 640;
	DisplayHeight = 480;

	DisplayWinReal = 640;
	DisplayWinMid = 320;
	DisplayWinCDepthBox = 0;

	DisplayHeight = 480;
	DisplayHeightExt = 0;
#endif


	g_ConsoleDebug->Write(MCD_NORMAL, "[IP] %s Port %d", szServerIpAddress, g_ServerPort);
	g_ConsoleDebug->Write(MCD_NORMAL, "[Resolusion] %d %dx%d", m_Resolution, WindowWidth,WindowHeight);
	return TRUE;
}

BOOL Util_CheckOption( char *lpszCommandLine, unsigned char cOption, char *lpszString)
{
	unsigned char cComp[2];
	cComp[0] = cOption; cComp[1] = cOption;
	if ( islower( ( int)cOption))
	{
		cComp[1] = toupper( ( int)cOption);
	}
	else if ( isupper( ( int)cOption))
	{
		cComp[1] = tolower( ( int)cOption);
	}

	int nFind = ( int)'/';
	unsigned char *lpFound = ( unsigned char*)lpszCommandLine;
	while ( lpFound)
	{
		lpFound = ( unsigned char*)strchr( ( char*)( lpFound + 1), nFind);
		if ( lpFound && ( *( lpFound + 1) == cComp[0] || *( lpFound + 1) == cComp[1]))
		{	// 
			if ( lpszString)
			{
				int nCount = 0;
				for ( unsigned char *lpSeek = lpFound + 2; *lpSeek != ' ' && *lpSeek != '\0'; lpSeek++)
				{
					nCount++;
				}

				memcpy( lpszString, lpFound + 2, nCount);
				lpszString[nCount] = '\0';
			}
			return ( TRUE);
		}
	}

	return ( FALSE);
}

#if !defined(__ANDROID__) && !defined(MU_IOS)
BOOL UpdateFile( char *lpszOld, char *lpszNew)
{
	SetFileAttributes(lpszOld, FILE_ATTRIBUTE_NORMAL);
	SetFileAttributes(lpszNew, FILE_ATTRIBUTE_NORMAL);

	DWORD dwStartTickCount = ::GetTickCount();
	while(::GetTickCount() - dwStartTickCount < 5000) {
		if ( CopyFile( lpszOld, lpszNew, FALSE))
		{
			DeleteFile( lpszOld);
			return ( TRUE);
		}
	}
	g_ErrorReport.Write("%s to %s CopyFile Error : %d\r\n", lpszNew, lpszOld, GetLastError());
	return ( FALSE);
}

#include <tlhelp32.h>

BOOL KillExeProcess( char *lpszExe)
{
	HANDLE hProcessSnap = NULL; 
    BOOL bRet = FALSE; 
    PROCESSENTRY32 pe32 = { 0 }; 
 
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 
 
    if (hProcessSnap == INVALID_HANDLE_VALUE) 
        return (FALSE); 
 
    pe32.dwSize = sizeof(PROCESSENTRY32); 
 
    if (Process32First(hProcessSnap, &pe32)) 
    {
        do 
        { 
			if(stricmp(pe32.szExeFile, lpszExe) == 0)
			{
				HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);

				if(process)
				{
					TerminateProcess(process, 0);
				}
			}
        } while (Process32Next(hProcessSnap, &pe32)); 
        bRet = TRUE; 
    } 
    else 
        bRet = FALSE;
 
    CloseHandle (hProcessSnap);

	return bRet;
}
#endif

char g_lpszCmdURL[50];
BOOL GetConnectServerInfo( PSTR szCmdLine, char *lpszURL, WORD *pwPort)
{
	char lpszTemp[256] = {0, };
	if( Util_CheckOption( szCmdLine, 'y', lpszTemp))
	{
		BYTE bySuffle[] = { 0x0C, 0x07, 0x03, 0x13 };

		for(int i=0; i<(int)strlen(lpszTemp); i++)
			lpszTemp[i] -= bySuffle[i%4];
		strcpy(lpszURL, lpszTemp);

		if( Util_CheckOption( szCmdLine, 'z', lpszTemp)) 
		{
			for(int j=0; j<(int)strlen(lpszTemp); j++)
				lpszTemp[j] -= bySuffle[j%4];
			*pwPort = atoi( lpszTemp);
		}
		g_ErrorReport.Write("[Virtual Connection] Connect IP : %s, Port : %d\r\n", lpszURL, *pwPort);
		return (TRUE);
	}
	if ( !Util_CheckOption( szCmdLine, 'u', lpszTemp))
	{
		return ( FALSE);
	}
	strcpy( lpszURL, lpszTemp);
	if ( !Util_CheckOption( szCmdLine, 'p', lpszTemp))
	{
		return ( FALSE);
	}
	*pwPort = atoi( lpszTemp);

	return ( TRUE);
}


extern int TimeRemain;
BOOL g_bInactiveTimeChecked = FALSE;
void MoveObject(OBJECT *o);
#if(!UseStackLog)
//=== MuError.dmp
#include <dbghelp.h>
#pragma comment(lib,"dbghelp.lib")
//===
bool ExceptionCallback(_EXCEPTION_POINTERS* pExceptionInfo )
{

	char path[MAX_PATH];

	SYSTEMTIME SystemTime;

	GetLocalTime(&SystemTime);

	wsprintf(path, "%d-%d-%d_%dh%dm%ds.dmp", SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay, SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);

	HANDLE file = CreateFile(path, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

	if (file != INVALID_HANDLE_VALUE)
	{
		MINIDUMP_EXCEPTION_INFORMATION mdei;

		mdei.ThreadId = GetCurrentThreadId();

		mdei.ExceptionPointers = pExceptionInfo;

		mdei.ClientPointers = 0;

		if (MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, (MINIDUMP_TYPE)(MiniDumpScanMemory + MiniDumpWithIndirectlyReferencedMemory), &mdei, 0, 0) != 0)
		{
			CloseHandle(file);
			g_ErrorReport.Write("Save DumpFile complete %s" , path);
			return EXCEPTION_EXECUTE_HANDLER;
		}
	}

	CloseHandle(file);

	return EXCEPTION_CONTINUE_SEARCH;

#ifdef ENABLE_FULLSCREEN
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
	if (g_bUseWindowMode == FALSE)
#endif	// USER_WINDOW_MODE
	{
		ChangeDisplaySettings(NULL,0);
	}
#endif //ENABLE_FULLSCREEN
	return true;
}
#endif
char* szServerIpAddress = "192.168.1.117";
WORD g_ServerPort = 63000;
BYTE Version[SIZE_PROTOCOLVERSION] = { '1' + 1, '0' + 2, '4' + 3, '0' + 4, '5' + 5 };
BYTE Serial[SIZE_PROTOCOLSERIAL + 1] = { "TbYehR2hFUPBKgZj" };
#if (GetGPUUse)
#if defined(_M_X64) || defined(__amd64__)
#define NVAPI_DLL "nvapi64.dll"
#else
#define NVAPI_DLL "nvapi.dll"
#endif

#include <windows.h>
// magic numbers, do not change them
#define NVAPI_MAX_PHYSICAL_GPUS   64
#define NVAPI_MAX_USAGES_PER_GPU  34

// function pointer types
typedef int* (*NvAPI_QueryInterface_t)(unsigned int offset);
typedef int (*NvAPI_Initialize_t)();
typedef int (*NvAPI_EnumPhysicalGPUs_t)(int** handles, int* count);
typedef int (*NvAPI_GPU_GetUsages_t)(int* handle, unsigned int* usages);
extern "C" __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
extern "C" __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
extern "C" __declspec(dllexport) int GetNvOptimusEnablement() { return NvOptimusEnablement; }
extern "C" __declspec(dllexport) int GetAmdPowerXpressRequest() { return AmdPowerXpressRequestHighPerformance; }
#endif

void SleepForMilliseconds(int milliseconds) {
	LARGE_INTEGER start, end, frequency;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&start);

	do {
		QueryPerformanceCounter(&end);
	} while ((end.QuadPart - start.QuadPart) * 1000 / frequency.QuadPart < milliseconds);
}
void CALLBACK MUHelperTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

MSG MainLoop()
{
	MSG msg;
	while (1)
	{
	
#if defined(__ANDROID__) || defined(MU_IOS)
		static bool s_touchPendingUp = false;
		static int s_touchPendingUpX = 0;
		static int s_touchPendingUpY = 0;
		if (s_touchPendingUp)
		{
			s_touchPendingUp = false;
			MouseLButton = false;
			MouseLButtonPop = true;
			MouseX = (g_fScreenRate_x > 0.0f) ? std::clamp((int)((float)s_touchPendingUpX / g_fScreenRate_x), 0, DisplayWin) : s_touchPendingUpX;
			MouseY = (g_fScreenRate_y > 0.0f) ? std::clamp((int)((float)s_touchPendingUpY / g_fScreenRate_y), 0, DisplayHeight) : s_touchPendingUpY;
			CInput::Instance().SetCursorPos(s_touchPendingUpX, s_touchPendingUpY);
		}

		SDL_Event ev;
		while (SDL_PollEvent(&ev))
		{
			if (ev.type == SDL_EVENT_QUIT)
			{
				Destroy = true;
				break;
			}
			else if (ev.type == SDL_EVENT_FINGER_DOWN)
			{
				bool handled = (g_pMainFrameMobile != nullptr && g_pMainFrameMobile->OnFingerDown(ev.tfinger));
				if (!handled)
				{
					const int pxX = std::clamp((int)(ev.tfinger.x * (float)WindowWidth), 0, (int)WindowWidth - 1);
					const int pxY = std::clamp((int)(ev.tfinger.y * (float)WindowHeight), 0, (int)WindowHeight - 1);
					MouseX = (g_fScreenRate_x > 0.0f) ? std::clamp((int)((float)pxX / g_fScreenRate_x), 0, DisplayWin) : pxX;
					MouseY = (g_fScreenRate_y > 0.0f) ? std::clamp((int)((float)pxY / g_fScreenRate_y), 0, DisplayHeight) : pxY;
					CInput::Instance().SetCursorPos(pxX, pxY);
					MouseLButton = true;
					MouseLButtonPush = true;
					MouseLButtonPop = false;
					s_touchPendingUp = false;
				}
			}
			else if (ev.type == SDL_EVENT_FINGER_UP)
			{
				bool handled = (g_pMainFrameMobile != nullptr && g_pMainFrameMobile->OnFingerUp(ev.tfinger));
				if (!handled)
				{
					const int pxX = std::clamp((int)(ev.tfinger.x * (float)WindowWidth), 0, (int)WindowWidth - 1);
					const int pxY = std::clamp((int)(ev.tfinger.y * (float)WindowHeight), 0, (int)WindowHeight - 1);
					MouseX = (g_fScreenRate_x > 0.0f) ? std::clamp((int)((float)pxX / g_fScreenRate_x), 0, DisplayWin) : pxX;
					MouseY = (g_fScreenRate_y > 0.0f) ? std::clamp((int)((float)pxY / g_fScreenRate_y), 0, DisplayHeight) : pxY;
					CInput::Instance().SetCursorPos(pxX, pxY);
					if (MouseLButtonPush)
					{
						s_touchPendingUp = true;
						s_touchPendingUpX = pxX;
						s_touchPendingUpY = pxY;
					}
					else
					{
						MouseLButton = false;
						MouseLButtonPop = true;
					}
				}
			}
			else if (ev.type == SDL_EVENT_FINGER_MOTION)
			{
				bool handled = (g_pMainFrameMobile != nullptr && g_pMainFrameMobile->OnFingerMotion(ev.tfinger));
				if (!handled)
				{
					const int pxX = std::clamp((int)(ev.tfinger.x * (float)WindowWidth), 0, (int)WindowWidth - 1);
					const int pxY = std::clamp((int)(ev.tfinger.y * (float)WindowHeight), 0, (int)WindowHeight - 1);
					MouseX = (g_fScreenRate_x > 0.0f) ? std::clamp((int)((float)pxX / g_fScreenRate_x), 0, DisplayWin) : pxX;
					MouseY = (g_fScreenRate_y > 0.0f) ? std::clamp((int)((float)pxY / g_fScreenRate_y), 0, DisplayHeight) : pxY;
					CInput::Instance().SetCursorPos(pxX, pxY);
				}
			}
			else if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
			{
				const int pxX = std::clamp((int)ev.button.x, 0, (int)WindowWidth - 1);
				const int pxY = std::clamp((int)ev.button.y, 0, (int)WindowHeight - 1);
				MouseX = (g_fScreenRate_x > 0.0f) ? std::clamp((int)((float)pxX / g_fScreenRate_x), 0, DisplayWin) : pxX;
				MouseY = (g_fScreenRate_y > 0.0f) ? std::clamp((int)((float)pxY / g_fScreenRate_y), 0, DisplayHeight) : pxY;
				CInput::Instance().SetCursorPos(pxX, pxY);
				if (ev.button.button == SDL_BUTTON_LEFT)
				{
					MouseLButton = true;
					MouseLButtonPush = true;
					MouseLButtonPop = false;
					s_touchPendingUp = false;
				}
				else if (ev.button.button == SDL_BUTTON_RIGHT)
				{
					MouseRButton = true;
					MouseRButtonPush = true;
				}
			}
			else if (ev.type == SDL_EVENT_MOUSE_BUTTON_UP)
			{
				const int pxX = std::clamp((int)ev.button.x, 0, (int)WindowWidth - 1);
				const int pxY = std::clamp((int)ev.button.y, 0, (int)WindowHeight - 1);
				MouseX = (g_fScreenRate_x > 0.0f) ? std::clamp((int)((float)pxX / g_fScreenRate_x), 0, DisplayWin) : pxX;
				MouseY = (g_fScreenRate_y > 0.0f) ? std::clamp((int)((float)pxY / g_fScreenRate_y), 0, DisplayHeight) : pxY;
				CInput::Instance().SetCursorPos(pxX, pxY);
				if (ev.button.button == SDL_BUTTON_LEFT)
				{
					if (MouseLButtonPush)
					{
						s_touchPendingUp = true;
						s_touchPendingUpX = pxX;
						s_touchPendingUpY = pxY;
					}
					else
					{
						MouseLButton = false;
						MouseLButtonPop = true;
					}
				}
				else if (ev.button.button == SDL_BUTTON_RIGHT)
				{
					MouseRButton = false;
					MouseRButtonPop = true;
				}
			}
			else if (ev.type == SDL_EVENT_MOUSE_MOTION)
			{
				const int pxX = std::clamp((int)ev.motion.x, 0, (int)WindowWidth - 1);
				const int pxY = std::clamp((int)ev.motion.y, 0, (int)WindowHeight - 1);
				MouseX = (g_fScreenRate_x > 0.0f) ? std::clamp((int)((float)pxX / g_fScreenRate_x), 0, DisplayWin) : pxX;
				MouseY = (g_fScreenRate_y > 0.0f) ? std::clamp((int)((float)pxY / g_fScreenRate_y), 0, DisplayHeight) : pxY;
				CInput::Instance().SetCursorPos(pxX, pxY);
			}
			else if (ev.type == SDL_EVENT_TEXT_INPUT)
			{
				if (ev.text.text && ev.text.text[0])
				{
					AndroidInjectUtf8ToFocusedTextInput(ev.text.text);
				}
			}
			else if (ev.type == SDL_EVENT_KEY_DOWN)
			{
				if (ev.key.key == SDLK_BACKSPACE)
				{
					AndroidInjectCharToFocusedTextInput(VK_BACK);
				}
				else if (ev.key.key == SDLK_RETURN || ev.key.key == SDLK_KP_ENTER)
				{
					const HWND focusedBeforeReturn = GetFocus();
					AndroidInjectCharToFocusedTextInput(VK_RETURN);
					if (GetFocus() == focusedBeforeReturn)
					{
						SetEnterPressed(true);
					}
				}
				else if (ev.key.key == SDLK_TAB)
				{
					AndroidInjectCharToFocusedTextInput(VK_TAB);
				}
			}
		}

		if (Destroy) break;

#if defined(__ANDROID__) || defined(MU_IOS)
		// Periodic timers for Android/iOS (replaces Win32 WM_TIMER / SetTimer)
		{
			const uint32_t nowTicks = SDL_GetTicks();

			// 1. Keepalive / Heartbeat to GameServer every 10s (GameServer drops client after 60s without packet 0x0E)
			static uint32_t s_lastHackTick = 0;
			if (nowTicks - s_lastHackTick >= 10000)
			{
				s_lastHackTick = nowTicks;
				if (SceneFlag == MAIN_SCENE && g_bGameServerConnected)
				{
					CheckHack();
				}
			}

			// 2. MU Helper bot processing every 250ms
			static uint32_t s_lastHelperTick = 0;
			if (nowTicks - s_lastHelperTick >= 250)
			{
				s_lastHelperTick = nowTicks;
				MUHelperTimerProc(NULL, 0, 0, 0);
			}
		}
#endif

		if (g_pMainFrameMobile != nullptr) g_pMainFrameMobile->Update();
		Scene(g_hDC);
#if defined(__ANDROID__) || defined(MU_IOS)
		MouseLButtonPush = false;
		MouseRButtonPush = false;
		MouseMButtonPush = false;
		MouseLButtonPop = false;
		MouseRButtonPop = false;
#endif
#else
		if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (!GetMessage(&msg, NULL, 0, 0))
			{
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{

			//Scene
#if (defined WINDOWMODE)
			if (g_bUseWindowMode || g_bWndActive)
			{
				Scene(g_hDC);
			}
#ifndef FOR_WORK
			else if (g_bUseWindowMode == FALSE)
			{
				SetForegroundWindow(g_hWnd);
				SetFocus(g_hWnd);

				if (g_iInactiveWarning > 1)
				{
					SetTimer(g_hWnd, WINDOWMINIMIZED_TIMER, 1 * 1000, NULL);
					PostMessage(g_hWnd, WM_CLOSE, 0, 0);
				}
				else
				{
					g_iInactiveWarning++;
					g_bMinimizedEnabled = TRUE;
					ShowWindow(g_hWnd, SW_MINIMIZE);
					g_bMinimizedEnabled = FALSE;
					ShowWindow(g_hWnd, SW_MAXIMIZE);
				}
			}
#endif//FOR_WORK
#else//WINDOWMODE
			if (g_bWndActive)
				Scene(g_hDC);

#endif	//WINDOWMODE(#else)
		}
#endif
#ifdef NEW_PROTOCOL_SYSTEM
		if (SceneFlag < CHARACTER_SCENE)
			ProtocolCompiler();

		g_pChatRoomSocketList->ProtocolCompile();
		gProtocolSend.RecvMessage();
#else
		//=== Fix Fps Test
		ProtocolCompiler();
		g_pChatRoomSocketList->ProtocolCompile();
#endif

	} // while( 1 )

	return msg;
}

inline SpinLock* g_MUHelper_lock = new SpinLock();
void CALLBACK MUHelperTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	if (SceneFlag == 5)
	{
		g_MUHelper_lock->lock();
		if (gCB_MUHelper) gCB_MUHelper->Work();
		g_MUHelper_lock->unlock();
	}
}
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int nCmdShow)
{
#if defined(__ANDROID__)
	const char* extPath = SDL_GetAndroidExternalStoragePath();
	const char* intPath = SDL_GetAndroidInternalStoragePath();
	g_ErrorReport.Write("[Android] extPath='%s', intPath='%s'\r\n", extPath ? extPath : "null", intPath ? intPath : "null");

	auto HasDataDir = [](const char* basePath) -> bool {
		if (!basePath || !basePath[0]) return false;
		char testPath[512];
		snprintf(testPath, sizeof(testPath), "%s/Data", basePath);
		struct stat st;
		if (stat(testPath, &st) == 0 && S_ISDIR(st.st_mode)) return true;
		snprintf(testPath, sizeof(testPath), "%s/data", basePath);
		if (stat(testPath, &st) == 0 && S_ISDIR(st.st_mode)) return true;
		return false;
	};

	if (extPath && HasDataDir(extPath) && chdir(extPath) == 0)
	{
		g_ErrorReport.Write("[Android] Working directory set to external storage (Data found): %s\r\n", extPath);
	}
	else if (intPath && HasDataDir(intPath) && chdir(intPath) == 0)
	{
		g_ErrorReport.Write("[Android] Working directory set to internal storage (Data found): %s\r\n", intPath);
	}
	else if (extPath && extPath[0] && chdir(extPath) == 0)
	{
		g_ErrorReport.Write("[Android] Working directory fallback to external storage: %s\r\n", extPath);
	}
	else if (intPath && intPath[0] && chdir(intPath) == 0)
	{
		g_ErrorReport.Write("[Android] Working directory fallback to internal storage: %s\r\n", intPath);
	}
	else
	{
		chdir("/sdcard/Android/data/com.muonline.client/files");
		g_ErrorReport.Write("[Android] Fallback working directory: /sdcard/Android/data/com.muonline.client/files\r\n");
	}

	char cwdBuf[512] = {0};
	if (getcwd(cwdBuf, sizeof(cwdBuf)))
	{
		g_ErrorReport.Write("[Android] Final current working directory: %s\r\n", cwdBuf);
	}
#elif !defined(MU_IOS)
	char szModulePath[MAX_PATH] = { 0 };
	if (GetModuleFileNameA(NULL, szModulePath, MAX_PATH))
	{
		char* pSlash = strrchr(szModulePath, '\\');
		if (pSlash)
		{
			*pSlash = '\0';
			SetCurrentDirectoryA(szModulePath);
		}
	}
#endif

#if(UseStackLog)
	StartStackLogging();
#endif
#if (GetGPUUse)
	HMODULE hmod = LoadLibraryA(NVAPI_DLL);
	if (hmod == NULL)
	{

		MessageBox(0, "Couldn't find nvapi.dll", "Error", MB_OK | MB_ICONERROR);
		ExitProcess(0);
	}

	// nvapi.dll internal function pointers
	NvAPI_QueryInterface_t      NvAPI_QueryInterface = NULL;
	NvAPI_Initialize_t          NvAPI_Initialize = NULL;
	NvAPI_EnumPhysicalGPUs_t    NvAPI_EnumPhysicalGPUs = NULL;
	NvAPI_GPU_GetUsages_t       NvAPI_GPU_GetUsages = NULL;

	// nvapi_QueryInterface is a function used to retrieve other internal functions in nvapi.dll
	NvAPI_QueryInterface = (NvAPI_QueryInterface_t)GetProcAddress(hmod, "nvapi_QueryInterface");

	// some useful internal functions that aren't exported by nvapi.dll
	NvAPI_Initialize = (NvAPI_Initialize_t)(*NvAPI_QueryInterface)(0x0150E828);
	NvAPI_EnumPhysicalGPUs = (NvAPI_EnumPhysicalGPUs_t)(*NvAPI_QueryInterface)(0xE5AC921F);
	NvAPI_GPU_GetUsages = (NvAPI_GPU_GetUsages_t)(*NvAPI_QueryInterface)(0x189A1FDF);

	if (NvAPI_Initialize == NULL || NvAPI_EnumPhysicalGPUs == NULL ||
		NvAPI_EnumPhysicalGPUs == NULL || NvAPI_GPU_GetUsages == NULL)
	{
		MessageBox(0, "Couldn't get functions in nvapi.dll", "Error", MB_OK | MB_ICONERROR);
		ExitProcess(0);
	}

	// initialize NvAPI library, call it once before calling any other NvAPI functions
	(*NvAPI_Initialize)();

	int          gpuCount = 0;
	int* gpuHandles[NVAPI_MAX_PHYSICAL_GPUS] = { NULL };
	unsigned int gpuUsages[NVAPI_MAX_USAGES_PER_GPU] = { 0 };

	// gpuUsages[0] must be this value, otherwise NvAPI_GPU_GetUsages won't work
	gpuUsages[0] = (NVAPI_MAX_USAGES_PER_GPU * 4) | 0x10000;

	(*NvAPI_EnumPhysicalGPUs)(gpuHandles, &gpuCount);

	// print GPU usage every second
	//int usages[NVAPI_MAX_PHYSICAL_GPUS];
	//while (TRUE)
	//{
	//	for (int j = 0; j < gpuCount; j++)
	//	{
	//		(*NvAPI_GPU_GetUsages)(gpuHandles[j], gpuUsages);
	//		usages[j] = gpuUsages[3];
	//		//g_Console.AddMessage(1, "GPU Usage: %d", usages[j]);
	//	}
	//	Sleep(1000);
	//}
#endif
#if(!UseStackLog)
	leaf::AttachExceptionHandler(ExceptionCallback);
#endif
	char lpszExeVersion[256] = "unknown";

	char *lpszCommandLine = GetCommandLine();
	char lpszFile[MAX_PATH];
	WORD wVersion[4] = { 0,};
	if ( GetFileNameOfFilePath( lpszFile, lpszCommandLine))
	{
		if ( GetFileVersion( lpszFile, wVersion))
		{
			sprintf( lpszExeVersion, "%d.%02d", wVersion[0], wVersion[1]);
			if ( wVersion[2] > 0)
			{
				char lpszMinorVersion[2] = "a";
				lpszMinorVersion[0] += ( wVersion[2] - 1);
				strcat( lpszExeVersion, lpszMinorVersion);
			}
		}
	}

	g_ErrorReport.Write( "\r\n");
	g_ErrorReport.WriteLogBegin();
	g_ErrorReport.AddSeparator();
	g_ErrorReport.Write( "Mu online %s (%s) executed. (%d.%d.%d.%d)\r\n", lpszExeVersion, "Eng", wVersion[0], wVersion[1], wVersion[2], wVersion[3]);

	g_ConsoleDebug->Write(MCD_NORMAL, "Mu Online (Version: %d.%d.%d.%d)", wVersion[0], wVersion[1], wVersion[2], wVersion[3]);

	g_ErrorReport.WriteCurrentTime();
	ER_SystemInfo si;
	ZeroMemory( &si, sizeof ( ER_SystemInfo));
	GetSystemInfo( &si);
	g_ErrorReport.AddSeparator();
	g_ErrorReport.WriteSystemInfo( &si);
	g_ErrorReport.AddSeparator();

	if (!gMainLoad.Load())
	{
		return false;
	}


	// PKD_ADD_BINARY_PROTECTION
	VM_START
	WORD wPortNumber;	
	if ( GetConnectServerInfo( szCmdLine, g_lpszCmdURL, &wPortNumber))
	{
		szServerIpAddress = g_lpszCmdURL;
		g_ServerPort = wPortNumber;
	}
	VM_END

	if ( !OpenMainExe())
	{
		return false;
	}

	// PKD_ADD_BINARY_PROTECTION
	VM_START
	g_SimpleModulusCS.LoadEncryptionKey( "Data\\Enc1.dat");
	g_SimpleModulusSC.LoadDecryptionKey( "Data\\Dec2.dat");

	VM_END

	g_ErrorReport.Write( "> To read config.ini.\r\n");
	if( OpenInitFile() == FALSE )
	{
		g_ErrorReport.Write( "config.ini read error\r\n");
#if !defined(__ANDROID__)
		return false;
#endif
	}

	pMultiLanguage = new CMultiLanguage(g_strSelectedML);

	if (g_iChatInputType == 1)
		ShowCursor(FALSE);

	g_ErrorReport.Write( "> Enum display settings.\r\n");
	DEVMODE DevMode;
	DEVMODE* pDevmodes;
	int nModes = 0;
	while (EnumDisplaySettings(NULL, nModes, &DevMode)) nModes++;
	pDevmodes = new DEVMODE[nModes+1];
	nModes = 0;
	while (EnumDisplaySettings(NULL, nModes, &pDevmodes[nModes])) nModes++;

	DWORD dwBitsPerPel = 16;
	for(int n1=0; n1<nModes; n1++)
	{
		if(pDevmodes[n1].dmBitsPerPel == 16 && m_nColorDepth == 0) {
			dwBitsPerPel = 16; break;
		}
		if(pDevmodes[n1].dmBitsPerPel == 24 && m_nColorDepth == 1) {
			dwBitsPerPel = 24; break;
		}
		if(pDevmodes[n1].dmBitsPerPel == 32 && m_nColorDepth == 1) {
			dwBitsPerPel = 32; break;
		}
	}

#ifdef ENABLE_FULLSCREEN
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
	if (g_bUseWindowMode == FALSE)
#endif	// USER_WINDOW_MODE
	{
		for(int n2=0; n2<nModes; n2++)
		{
			if(pDevmodes[n2].dmPelsWidth==WindowWidth && pDevmodes[n2].dmPelsHeight==WindowHeight && pDevmodes[n2].dmBitsPerPel == dwBitsPerPel)
			{
				g_ErrorReport.Write( "> Change display setting %dx%d.\r\n", pDevmodes[n2].dmPelsWidth, pDevmodes[n2].dmPelsHeight);
				ChangeDisplaySettings(&pDevmodes[n2],0);
				break;
			}
		}
	}
#endif //ENABLE_FULLSCREEN

	delete [] pDevmodes;

	g_ErrorReport.Write( "> Screen size = %d x %d.\r\n", WindowWidth, WindowHeight);

    g_hInst = hInstance;
    g_hWnd = StartWindow(hInstance,nCmdShow);
	g_ErrorReport.Write( "> Start window success.\r\n");

    if ( !CreateOpenglWindow())
	{
		return FALSE;
	}

	

	g_ErrorReport.Write( "> OpenGL init success.\r\n");
	g_ErrorReport.AddSeparator();
	//g_ErrorReport.WriteOpenGLInfo();
	g_ErrorReport.AddSeparator();
	g_ErrorReport.WriteSoundCardInfo(); 

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

	//g_ErrorReport.WriteImeInfo( g_hWnd);
	g_ErrorReport.AddSeparator();


	//InitVSync();
	//if (IsVSyncAvailable())
	//{
	//	EnableVSync();
	//	//SetTargetFps(-1); // unlimited
	//}
	
	switch(WindowWidth)
	{
		case 640 :FontHeight = 12;break;
		case 800 :FontHeight = 13;break;
		case 1024:FontHeight = 14;break;
		case 1280:FontHeight = 15;break;
		default:FontHeight = 14; break;

	}
	
	int nFixFontHeight = 13 + m_FontSizePlus;
	int nFixFontSize;
	int iFontSize;
	FontHeight = FontHeight + m_FontSizePlus;
	iFontSize = FontHeight - 1;
	nFixFontSize = nFixFontHeight - 1;
#if defined(__ANDROID__) || defined(MU_IOS)
	iFontSize = 18;
	nFixFontSize = 18;
#endif

	g_hFont		= CreateFont(iFontSize,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,NONANTIALIASED_QUALITY,DEFAULT_PITCH|FF_DONTCARE, m_FontName);
	g_hFontBold = CreateFont(iFontSize,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,NONANTIALIASED_QUALITY,DEFAULT_PITCH|FF_DONTCARE, m_FontName);
	g_hFontBoldName = CreateFont(15, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, m_FontName);
	g_hFontBig	= CreateFont((iFontSize*2 > 28) ? 28 : (iFontSize*2),0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,NONANTIALIASED_QUALITY,DEFAULT_PITCH|FF_DONTCARE, m_FontName);
	g_hFixFont	= CreateFont(nFixFontSize,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,NONANTIALIASED_QUALITY,DEFAULT_PITCH | FF_DONTCARE, m_FontName);
	g_hFontMini = CreateFont(10, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, m_FontName);
	setlocale( LC_ALL, "english");

	CInput::Instance().Create(g_hWnd, WindowWidth, WindowHeight);

	g_pNewUISystem->Create();

	if(m_MusicOnOff)
	{
		wzAudioCreate(g_hWnd);
		wzAudioOption(WZAOPT_STOPBEFOREPLAY, 1);
	}

	if(m_SoundOnOff)
	{
		InitDirectSound(g_hWnd);
#if !defined(__ANDROID__) && !defined(MU_IOS)
		leaf::CRegKey regkey;
		regkey.SetKey(leaf::CRegKey::_HKEY_CURRENT_USER, "SOFTWARE\\Webzen\\Mu\\Config");
		DWORD value;
		if(!regkey.ReadDword("VolumeLevel", value))
		{
			value = 5;	//. default setting
			regkey.WriteDword("VolumeLevel", value);
		}
		if(value<0 || value>=10)
			value = 5;
		
		g_pOption->SetVolumeLevel(int(value));
		SetEffectVolumeLevel(g_pOption->GetVolumeLevel());
#endif
	}

	SetTimer(g_hWnd, HACK_TIMER, 20*1000, NULL);

	gCB_MUHelper = new CB_MUHelper;
	SetTimer(g_hWnd, MU_HELPERAUTO, 250, MUHelperTimerProc);

	srand((unsigned)time(NULL));
	for(int i=0;i<100;i++)
	{
		RandomTable[i] = rand()%360;
	}

	//memorydump[0]
	RendomMemoryDump = new BYTE [rand()%100+1];


	GateAttribute				= new GATE_ATTRIBUTE [MAX_GATES];
	SkillAttribute				= new SKILL_ATTRIBUTE[MAX_SKILLS];

	//memorydump[1]
	ItemAttRibuteMemoryDump		= new ITEM_ATTRIBUTE [MAX_ITEM+ 1024];
	ItemAttribute				= ((ITEM_ATTRIBUTE*)ItemAttRibuteMemoryDump)+rand()% 1024;

	//memorydump[2]
	CharacterMemoryDump			= new CHARACTER      [MAX_CHARACTERS_CLIENT+1+128];
	CharactersClient			= ((CHARACTER*)CharacterMemoryDump)+rand()%128;
	CharacterMachine			= new CHARACTER_MACHINE;

	memset(GateAttribute       ,0,sizeof(GATE_ATTRIBUTE   )*(MAX_GATES              ));
	//memset(ItemAttribute       ,0,sizeof(ITEM_ATTRIBUTE   )*(MAX_ITEM     + 2048));
	memset(ItemAttRibuteMemoryDump, 0, sizeof(ITEM_ATTRIBUTE)* (MAX_ITEM + 1024)); //Fix RenderItemName
	memset(SkillAttribute      ,0,sizeof(SKILL_ATTRIBUTE  )*(MAX_SKILLS             ));
	memset(CharacterMachine    ,0,sizeof(CHARACTER_MACHINE));

    CharacterAttribute   = &CharacterMachine->Character;
    CharacterMachine->Init();
	Hero = &CharactersClient[0];	

	if (g_iChatInputType == 1)
	{
		 g_pMercenaryInputBox = new CUIMercenaryInputBox;
		 g_pSingleTextInputBox = new CUITextInputBox;
		 g_pSinglePasswdInputBox = new CUITextInputBox;
	}
	
	g_pChatRoomSocketList = new CChatRoomSocketList;
	g_pUIManager = new CUIManager;
	g_pUIMapName = new CUIMapName;	// rozy
	g_pTimer = new CTimer();

#ifdef MOVIE_DIRECTSHOW
	g_pMovieScene = new CMovieScene;
#endif // MOVIE_DIRECTSHOW
	
	g_BuffSystem = BuffStateSystem::Make();

	g_MapProcess = MapProcess::Make();

	g_petProcess = PetProcess::Make();

	CUIMng::Instance().Create();

	if (g_iChatInputType == 1)
	{
		g_pMercenaryInputBox->Init(g_hWnd);
		g_pSingleTextInputBox->Init(g_hWnd, 200, 20);
		g_pSinglePasswdInputBox->Init(g_hWnd, 200, 20, 9, TRUE);
		g_pSingleTextInputBox->SetState(UISTATE_HIDE);
		g_pSinglePasswdInputBox->SetState(UISTATE_HIDE);

		g_pMercenaryInputBox->SetFont(g_hFont);
		g_pSingleTextInputBox->SetFont(g_hFont);
		g_pSinglePasswdInputBox->SetFont(g_hFont);

		g_bIMEBlock = FALSE;
		HIMC  hIMC = ImmGetContext(g_hWnd);
		ImmSetConversionStatus(hIMC, IME_CMODE_ALPHANUMERIC, IME_SMODE_NONE );
		ImmReleaseContext(g_hWnd, hIMC);
		SaveIMEStatus();
		g_bIMEBlock = TRUE;
	}
#if (defined WINDOWMODE)
	if (g_bUseWindowMode == FALSE)
	{
		int nOldVal;
		SystemParametersInfo(SPI_SCREENSAVERRUNNING, 1, &nOldVal, 0);
		SystemParametersInfo(SPI_GETSCREENSAVETIMEOUT, 0, &g_iScreenSaverOldValue, 0);
		SystemParametersInfo(SPI_SETSCREENSAVETIMEOUT, 300*60, NULL, 0);
	}
#else
#ifdef NDEBUG
#ifndef FOR_WORK
#ifdef ACTIVE_FOCUS_OUT
	if (g_bUseWindowMode == FALSE)
	{
#endif	// ACTIVE_FOCUS_OUT
		int nOldVal; // °ªÀÌ µé¾î°¥ ÇÊ¿ä°¡ ¾øÀ½
		SystemParametersInfo(SPI_SCREENSAVERRUNNING, 1, &nOldVal, 0);  // ´ÜÃàÅ°¸¦ ¸ø¾²°Ô ÇÔ
		SystemParametersInfo(SPI_GETSCREENSAVETIMEOUT, 0, &g_iScreenSaverOldValue, 0);  // ½ºÅ©¸°¼¼ÀÌ¹ö Â÷´Ü
		SystemParametersInfo(SPI_SETSCREENSAVETIMEOUT, 300*60, NULL, 0);  // ½ºÅ©¸°¼¼ÀÌ¹ö Â÷´Ü
#ifdef ACTIVE_FOCUS_OUT
	}
#endif	// ACTIVE_FOCUS_OUT
#endif
#endif
#endif	//WINDOWMODE(#else)

#ifdef SAVE_PACKET
	DeleteFile( PACKET_SAVE_FILE);
#endif

#if defined PROTECT_SYSTEMKEY && defined NDEBUG
#ifndef FOR_WORK
	ProtectSysKey::AttachProtectSysKey(g_hInst, g_hWnd);
#endif // !FOR_WORK
#endif // PROTECT_SYSTEMKEY && NDEBUG

#if defined(__ANDROID__) || defined(MU_IOS)
	g_bWndActive = true;
	g_bUseWindowMode = TRUE;
#endif

	const MSG msg = MainLoop();
	DestroyWindow();
	
    return msg.wParam;
}

#if defined(__ANDROID__) || defined(MU_IOS)
extern "C" int SDL_main(int argc, char* argv[])
{
    return WinMain(nullptr, nullptr, (char*)"", 1);
}
#endif
