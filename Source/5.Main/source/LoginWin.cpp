//*****************************************************************************
// File: LoginWin.cpp
//*****************************************************************************

#include "stdafx.h"
#include "LoginWin.h"
#include "Input.h"
#include "UIMng.h"
#include "ZzzBMD.h"
#include "ZzzInfomation.h"
#include "ZzzObject.h"
#include "ZzzCharacter.h"
#include "ZzzInterface.h"
#include "UIControls.h"
#include "ZzzScene.h"
#include "wsclientinline.h"
#include "DSPlaySound.h"
#include "./Utilities/Log/muConsoleDebug.h"

#include "ProtocolSend.h"
#include "ServerListManager.h"
#include "Reconnect.h"
#include "./ExternalObject/leaf/regkey.h"
#include "CB_DangKyInGame.h"
#include "CBInterface.h"
#if(CB_AUTOLOGINWIN)
#include "CB_AutoLogin.h"
#endif
#include "GameConfig/GameConfig.h"
#include "Platform/MobilePlatform.h"
#define	LIW_ACCOUNT		0
#define	LIW_PASSWORD	1

#define LIW_OK			0
#define LIW_CANCEL		1

extern float g_fScreenRate_x;
extern float g_fScreenRate_y;
extern bool MouseLButtonPop;
extern bool MouseLButtonPush;
extern int MouseX;
extern int MouseY;
extern int g_iChatInputType;
extern char m_Username[11];
extern char m_Password[11];
extern bool IsEnterPressed();
extern void SetEnterPressed(bool enterpressed);

namespace
{
#if defined(__ANDROID__) || defined(MU_IOS)
float GetLoginUiScale()
{
	if (g_fScreenRate_y > 1.0f)
	{
		return g_fScreenRate_y;
	}

	return 1.0f;
}
#else
float GetLoginUiScale()
{
	return 1.0f;
}
#endif

int ScaleLoginMetric(int value)
{
	return static_cast<int>(std::lround(static_cast<double>(value) * GetLoginUiScale()));
}
}

CLoginWin::CLoginWin()
{
	m_pIDInputBox = NULL;
	m_pPassInputBox = NULL;
#if(CB_AUTOLOGINWIN)
	gCB_AutoLogin = new CB_AutoLogin;
#endif
}

CLoginWin::~CLoginWin()
{
	SAFE_DELETE(m_pIDInputBox);
	SAFE_DELETE(m_pPassInputBox);

#if(CB_AUTOLOGINWIN)
	SAFE_DELETE(gCB_AutoLogin);
#endif
}

void CLoginWin::Create()
{
	CWin::Create(329, 245, BITMAP_LOG_IN + 7);
	CWin::SetSize(ScaleLoginMetric(329), ScaleLoginMetric(245));

	m_asprInputBox[LIW_ACCOUNT].Create(156, 23, BITMAP_LOG_IN + 8);
	m_asprInputBox[LIW_ACCOUNT].SetSize(ScaleLoginMetric(156), ScaleLoginMetric(23));
	m_asprInputBox[LIW_PASSWORD].Create(156, 23, BITMAP_LOG_IN + 8);
	m_asprInputBox[LIW_PASSWORD].SetSize(ScaleLoginMetric(156), ScaleLoginMetric(23));

	for (int i = 0; i < 2; ++i)
	{
		m_aBtn[i].Create(54, 30, BITMAP_BUTTON + i, 3, 2, 1);
		m_aBtn[i].SetSize(ScaleLoginMetric(54), ScaleLoginMetric(30));
		CWin::RegisterButton(&m_aBtn[i]);
	}
#if(CB_DANGKYINGAME)
	//=====
	m_DangKy.Create(54, 30, gInterface.Data[IMG_71620].ModelID, 3, 2, 1);
	m_DangKy.SetSize(ScaleLoginMetric(54), ScaleLoginMetric(30));
	CWin::RegisterButton(&m_DangKy);
#endif

	SAFE_DELETE(m_pIDInputBox);

	m_pIDInputBox = new CUITextInputBox;
	m_pIDInputBox->Init(g_hWnd, 140, 14, MAX_ID_SIZE);
	m_pIDInputBox->SetBackColor(0, 0, 0, 255);
	m_pIDInputBox->SetTextColor(255, 255, 230, 210);
	m_pIDInputBox->SetFont(g_hFixFont);
	m_pIDInputBox->SetState(UISTATE_NORMAL);
	if (m_ID[0] == '\0' && m_Username[0] != '\0')
	{
		strncpy(m_ID, m_Username, sizeof(m_ID) - 1);
		m_ID[sizeof(m_ID) - 1] = '\0';
	}
	m_pIDInputBox->SetText(m_ID[0] ? m_ID : m_Username);

	SAFE_DELETE(m_pPassInputBox);

	m_pPassInputBox = new CUITextInputBox;
	m_pPassInputBox->Init(g_hWnd, 140, 14, MAX_PASSWORD_SIZE, TRUE);
	m_pPassInputBox->SetBackColor(0, 0, 0, 25);
	m_pPassInputBox->SetTextColor(255, 255, 230, 210);
	m_pPassInputBox->SetFont(g_hFixFont);
	m_pPassInputBox->SetState(UISTATE_NORMAL);
	m_pPassInputBox->SetOption(UIOPTION_NOLOCALIZEDCHARACTERS);
	if (m_Password[0] != '\0')
	{
		m_pPassInputBox->SetText(m_Password);
	}
	m_pIDInputBox->SetTabTarget(m_pPassInputBox);
#if defined(__ANDROID__) || defined(MU_IOS)
	m_pPassInputBox->SetTabTarget(NULL);
#else
	m_pPassInputBox->SetTabTarget(m_pIDInputBox);
#endif
	
	this->FirstLoad = 1;

#if(CB_AUTOLOGINWIN)
	if (gCB_AutoLogin)
	{
		gCB_AutoLogin->ReadConfigs();
		gCB_AutoLogin->SetShowListAccount(false);
		if (gCB_AutoLogin->totalSavedAcc > 0)
		{
			gCB_AutoLogin->SetSelectedAccount(0);
		}
	}
#endif
}

void CLoginWin::PreRelease()
{
	for (int i = 0; i < 2; ++i)
		m_asprInputBox[i].Release();
}

void CLoginWin::SetPosition(int nXCoord, int nYCoord)
{
	CWin::SetPosition(nXCoord, nYCoord);

	m_asprInputBox[LIW_ACCOUNT].SetPosition(nXCoord + ScaleLoginMetric(109), nYCoord + ScaleLoginMetric(106));
	m_asprInputBox[LIW_PASSWORD].SetPosition(nXCoord + ScaleLoginMetric(109), nYCoord + ScaleLoginMetric(131));

	if (g_iChatInputType == 1)
	{
		m_pIDInputBox->SetPosition(int((m_asprInputBox[LIW_ACCOUNT].GetXPos() + ScaleLoginMetric(6)) / g_fScreenRate_x),
			int((m_asprInputBox[LIW_ACCOUNT].GetYPos() + ScaleLoginMetric(5)) / g_fScreenRate_y));

		m_pPassInputBox->SetPosition(int((m_asprInputBox[LIW_PASSWORD].GetXPos() + ScaleLoginMetric(6)) / g_fScreenRate_x),
			int((m_asprInputBox[LIW_PASSWORD].GetYPos() + ScaleLoginMetric(5)) / g_fScreenRate_y));
	}

	m_aBtn[LIW_OK].SetPosition(nXCoord + ScaleLoginMetric(150), nYCoord + ScaleLoginMetric(178));
	m_aBtn[LIW_CANCEL].SetPosition(nXCoord + ScaleLoginMetric(211), nYCoord + ScaleLoginMetric(178));
	//===
#if(CB_DANGKYINGAME)
	m_DangKy.SetPosition(nXCoord + ScaleLoginMetric(60), nYCoord + ScaleLoginMetric(178));
#endif
}

void CLoginWin::Show(bool bShow)
{
	CWin::Show(bShow);

	for (int i = 0; i < 2; ++i)
	{
		m_asprInputBox[i].Show(bShow);
		m_aBtn[i].Show(bShow);
	}
#if(CB_DANGKYINGAME)
	m_DangKy.Show(bShow);
#endif

#if defined(__ANDROID__) || defined(MU_IOS)
	if (bShow)
	{
		CInput& rInput = CInput::Instance();
		int loginY = (rInput.GetScreenHeight() - CWin::GetHeight()) / 4;
		if (loginY < 20) loginY = 20;
		SetPosition((rInput.GetScreenWidth() - CWin::GetWidth()) / 2, loginY);

#if(CB_AUTOLOGINWIN)
		if (gCB_AutoLogin && gCB_AutoLogin->totalSavedAcc > 0)
		{
			gCB_AutoLogin->SetSelectedAccount(0);
		}
#endif
		if (m_ID[0] == '\0' && m_Username[0] != '\0')
		{
			strncpy(m_ID, m_Username, sizeof(m_ID) - 1);
			m_ID[sizeof(m_ID) - 1] = '\0';
		}
		if (m_pIDInputBox != nullptr)
		{
			char curId[MAX_ID_SIZE + 1] = {0};
			m_pIDInputBox->GetText(curId, MAX_ID_SIZE);
			if (curId[0] == '\0' && m_ID[0] != '\0')
			{
				m_pIDInputBox->SetText(m_ID);
			}
		}
		if (m_pPassInputBox != nullptr)
		{
			char curPass[MAX_PASSWORD_SIZE + 1] = {0};
			m_pPassInputBox->GetText(curPass, MAX_PASSWORD_SIZE);
			if (curPass[0] == '\0' && m_Password[0] != '\0')
			{
				m_pPassInputBox->SetText(m_Password);
			}
		}

		if (strlen(m_ID) > 0 && m_pPassInputBox != nullptr)
		{
			m_pPassInputBox->GiveFocus(TRUE);
			MU_MobileStartTextInput();
		}
		else if (m_pIDInputBox != nullptr)
		{
			m_pIDInputBox->GiveFocus(TRUE);
			MU_MobileStartTextInput();
		}
	}
#endif
}

bool CLoginWin::FocusInputAt(float uiX, float uiY)
{
	if (!IsShow())
	{
		return false;
	}

	const float paddingX = 12.0f;
	const float paddingY = 8.0f;

	auto hitSprite = [uiX, uiY, paddingX, paddingY](CSprite& sprite)
	{
		float spriteX = static_cast<float>(sprite.GetXPos());
		float spriteY = static_cast<float>(sprite.GetYPos());
		float spriteW = static_cast<float>(sprite.GetWidth());
		float spriteH = static_cast<float>(sprite.GetHeight());
		if (g_fScreenRate_x > 0.0f)
		{
			spriteX /= g_fScreenRate_x;
			spriteW /= g_fScreenRate_x;
		}
		if (g_fScreenRate_y > 0.0f)
		{
			spriteY /= g_fScreenRate_y;
			spriteH /= g_fScreenRate_y;
		}
		return sprite.IsShow()
			&& uiX >= (spriteX - paddingX)
			&& uiX <= (spriteX + spriteW + paddingX)
			&& uiY >= (spriteY - paddingY)
			&& uiY <= (spriteY + spriteH + paddingY);
	};

	auto hitInput = [uiX, uiY, paddingX, paddingY](CUITextInputBox* input)
	{
		return input != NULL
			&& input->GetState() == UISTATE_NORMAL
			&& uiX >= (input->GetPosition_x() - paddingX)
			&& uiX <= (input->GetPosition_x() + input->GetWidth() + paddingX)
			&& uiY >= (input->GetPosition_y() - paddingY)
			&& uiY <= (input->GetPosition_y() + input->GetHeight() + paddingY);
	};

	if (hitInput(m_pIDInputBox) || hitSprite(m_asprInputBox[LIW_ACCOUNT]))
	{
		m_pIDInputBox->GiveFocus(TRUE);
#if defined(__ANDROID__) || defined(MU_IOS)
		MU_MobileStartTextInput();
#endif
		return true;
	}

	if (hitInput(m_pPassInputBox) || hitSprite(m_asprInputBox[LIW_PASSWORD]))
	{
		m_pPassInputBox->GiveFocus(TRUE);
#if defined(__ANDROID__) || defined(MU_IOS)
		MU_MobileStartTextInput();
#endif
		return true;
	}

	return false;
}

bool CLoginWin::CursorInWin(int nArea)
{
	if (!CWin::m_bShow)
		return false;

	switch (nArea)
	{
	case WA_MOVE:
		return false;
	}

	return CWin::CursorInWin(nArea);
}

void CLoginWin::UpdateWhileActive(double dDeltaTick)
{
#if(CB_DANGKYINGAME)
	if (gInterface.Data[eWindow_DangKyInGame].OnShow)
	{
		return;
	}
#endif
	CInput& rInput = CInput::Instance();

	bool okClicked = m_aBtn[LIW_OK].IsClick();
	bool cancelClicked = m_aBtn[LIW_CANCEL].IsClick();
#if defined(__ANDROID__) || defined(MU_IOS)
	if (!okClicked && !cancelClicked && (rInput.IsLBtnUp() || rInput.IsLBtnDn() || MouseLButtonPop || MouseLButtonPush))
	{
		const long cx = rInput.GetCursorX();
		const long cy = rInput.GetCursorY();
		const long mx = (long)(MouseX * g_fScreenRate_x);
		const long my = (long)(MouseY * g_fScreenRate_y);
		if (m_aBtn[LIW_OK].PtInSprite(cx, cy) || m_aBtn[LIW_OK].PtInSprite(mx, my))
		{
			okClicked = true;
		}
		else if (m_aBtn[LIW_CANCEL].PtInSprite(cx, cy) || m_aBtn[LIW_CANCEL].PtInSprite(mx, my))
		{
			cancelClicked = true;
		}
	}
#endif

	if (okClicked)
	{
		RequestLogin();
	}
	else if (cancelClicked)
	{
		CancelLogin();
	}
#if(CB_DANGKYINGAME)
	else if (m_DangKy.IsClick())
	{
		if (gCB_DangKyInGame) gCB_DangKyInGame->OpenOnOff();
	}
#endif
	else if (CInput::Instance().IsKeyDown(VK_RETURN) || IsEnterPressed())
	{
		SetEnterPressed(false);
		::PlayBuffer(SOUND_CLICK01);
		RequestLogin();
	}
	else if (CInput::Instance().IsKeyDown(VK_ESCAPE))
	{
		::PlayBuffer(SOUND_CLICK01);
		CancelLogin();
		CUIMng::Instance().SetSysMenuWinShow(false);
	}
}

void CLoginWin::UpdateWhileShow(double dDeltaTick)
{
#if(CB_DANGKYINGAME)
	if (gInterface.Data[eWindow_DangKyInGame].OnShow)
	{
		if ((m_pIDInputBox != NULL && m_pIDInputBox->HaveFocus())
			|| (m_pPassInputBox != NULL && m_pPassInputBox->HaveFocus()))
		{
			SetFocus(g_hWnd);
		}

		return;
	}
#endif

	m_pIDInputBox->DoAction();
	m_pPassInputBox->DoAction();

#if defined(__ANDROID__) || defined(MU_IOS)
	if (CInput::Instance().IsLBtnDn() || MouseLButtonPush || MouseLButtonPop)
	{
		FocusInputAt(static_cast<float>(MouseX), static_cast<float>(MouseY));
	}
#endif
}

void CLoginWin::RenderControls()
{

	if (this->FirstLoad == 1)
	{
		if (strlen(m_ID) > 0)
			CUIMng::Instance().m_LoginWin.GetPassInputBox()->GiveFocus();
		else
			CUIMng::Instance().m_LoginWin.GetIDInputBox()->GiveFocus();
		this->FirstLoad = 0;
	}

	CWin::RenderButtons();

	for (int i = 0; i < 2; ++i)
		m_asprInputBox[i].Render();
#if(CB_AUTOLOGINWIN)
	if (gCB_AutoLogin)
	{
		if (!gCB_AutoLogin->showListAccount)
		{
			m_pIDInputBox->Render();
			m_pPassInputBox->Render();
		}
	}
#else
	m_pIDInputBox->Render();
	m_pPassInputBox->Render();
#endif
	g_pRenderText->SetFont(g_hFixFont);
	g_pRenderText->SetBgColor(0);
	g_pRenderText->SetTextColor(CLRDW_WHITE);
	g_pRenderText->RenderText(int((CWin::GetXPos() + ScaleLoginMetric(30)) / g_fScreenRate_x),
		int((CWin::GetYPos() + ScaleLoginMetric(113)) / g_fScreenRate_y), GlobalText[450]);
	g_pRenderText->RenderText(int((CWin::GetXPos() + ScaleLoginMetric(30)) / g_fScreenRate_x),
		int((CWin::GetYPos() + ScaleLoginMetric(139)) / g_fScreenRate_y), GlobalText[451]);	

	unicode::t_char szServerName[MAX_TEXT_LENGTH];

	const char* apszGlobalText[4]
		= { GlobalText[461], GlobalText[460], GlobalText[3130], GlobalText[3131] };
	sprintf(szServerName, apszGlobalText[g_ServerListManager->GetNonPVPInfo()],
		g_ServerListManager->GetSelectServerName(), g_ServerListManager->GetSelectServerIndex());

	g_pRenderText->RenderText(int((CWin::GetXPos() + ScaleLoginMetric(111)) / g_fScreenRate_x),
		int((CWin::GetYPos() + ScaleLoginMetric(80)) / g_fScreenRate_y), szServerName);
#if(CB_AUTOLOGINWIN)
	if (gCB_AutoLogin) gCB_AutoLogin->DrawInfo(CWin::GetXPos(), CWin::GetYPos());
#endif

}

void CLoginWin::RequestLogin()
{
	if (CurrentProtocolState == REQUEST_JOIN_SERVER)
	{
		g_ErrorReport.Write("[AndroidLogin] request ignored while waiting join server response\r\n");
		CUIMng::Instance().PopUpMsgWin(MESSAGE_WAIT);
		return;
	}

	char szID[MAX_ID_SIZE+1] = { 0, };
	char szPass[MAX_PASSWORD_SIZE+1] = {0, };
	m_pIDInputBox->GetText(szID, MAX_ID_SIZE+1);
	m_pPassInputBox->GetText(szPass, MAX_PASSWORD_SIZE+1);
	g_ErrorReport.Write("[AndroidLogin] RequestLogin state=%d id=%s passLen=%d\r\n", CurrentProtocolState, szID, (int)strlen(szPass));
	
	if (unicode::_strlen(szID) <= 0)
		CUIMng::Instance().PopUpMsgWin(MESSAGE_INPUT_ID);
	else if (unicode::_strlen(szPass) <= 0)
		CUIMng::Instance().PopUpMsgWin(MESSAGE_INPUT_PASSWORD);
	else if (CurrentProtocolState != RECEIVE_JOIN_SERVER_SUCCESS)
	{
		g_ErrorReport.Write("[AndroidLogin] request blocked because protocol state is not ready\r\n");
		CUIMng::Instance().PopUpMsgWin(MESSAGE_WAIT);
	}
	else
	{
		CUIMng::Instance().HideWin(this);
#if defined(__ANDROID__) || defined(MU_IOS)
		strncpy(m_ID, szID, sizeof(m_ID) - 1);
		m_ID[sizeof(m_ID) - 1] = '\0';
		strncpy(m_Username, szID, sizeof(m_Username) - 1);
		m_Username[sizeof(m_Username) - 1] = '\0';
		strncpy(m_Password, szPass, sizeof(m_Password) - 1);
		m_Password[sizeof(m_Password) - 1] = '\0';

		wchar_t wUser[MAX_ID_SIZE + 1] = {0};
		wchar_t wPass[MAX_PASSWORD_SIZE + 1] = {0};
		mbstowcs(wUser, szID, MAX_ID_SIZE);
		mbstowcs(wPass, szPass, MAX_PASSWORD_SIZE);
		GameConfig::GetInstance().EncryptAndSaveCredentials(wUser, wPass);
#endif
#if(UseReconnect)
		memcpy(g_pReconnect->s_Data.ReconnectAccount, szID, 11);  //Add
		memcpy(g_pReconnect->s_Data.ReconnectPassword, szPass, 11);  //Add
#endif
		g_ConsoleDebug->Write( MCD_NORMAL, "Login with the following account: %s", szID);

		g_ErrorReport.Write("> Login Request.\r\n");
		g_ErrorReport.Write("> Try to Login \"%s\"\r\n", szID);
		// SendRequestLogIn()

#ifdef NEW_PROTOCOL_SYSTEM
		gProtocolSend.SendRequestLogInNew(szID, szPass);
#else
		SendRequestLogIn(szID, szPass);
#endif

#if(CB_AUTOLOGINWIN)
		if (gCB_AutoLogin) gCB_AutoLogin->SaveData(szID, szPass);
#endif
	}
}

void CLoginWin::CancelLogin()
{
	ConnectConnectionServer();
	CUIMng::Instance().HideWin(this);
}

void CLoginWin::ConnectConnectionServer()
{
	#ifdef NEW_PROTOCOL_SYSTEM
		gProtocolSend.DisconnectServer();
	#endif

	LogIn = 0;
	CurrentProtocolState = REQUEST_JOIN_SERVER;
	CUIMng::Instance().m_ServerSelWin.ResetAutoEnterFirstServer();
    CreateSocket(szServerIpAddress, g_ServerPort);
}
