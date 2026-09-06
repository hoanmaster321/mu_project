#include "StdAfx.h"
#include "CB_DangKyInGame.h"
#include "NewUISystem.h"
#include "CBInterface.h"
#include "CUIController.h"
#include "CharacterManager.h"
#include "Util.h"
#include "Protocol.h"
#include "NewUIBase.h"
#include "Other.h"
#include "ZzzInterface.h"
#include "UIMng.h"
#include "Input.h"
#include "CustomEventTime.h"

CB_DangKyInGame* gCB_DangKyInGame;

namespace
{
	const DWORD kRegisterTextColor = 0xFFFFFFFF;
	const float kRegisterWindowWidth = 262.0f;
	const float kRegisterWindowHeight = 250.0f;
	const float kInputWidth = 110.0f;
	const float kInputSpacing = 20.0f;
	const float kCaptchaWidth = 120.0f;
	const float kCaptchaHeight = 17.0f;
	const float kCaptchaCodeWidth = 50.0f;
	const float kCaptchaInputOffsetX = 54.0f;
	const float kCaptchaInputOffsetY = 6.5f;
	const float kCaptchaInputWidth = kCaptchaWidth - kCaptchaInputOffsetX - 2.0f;
	const float kCaptchaInputHeight = 14.0f;
	const char* kDefaultRegisterPhone = "0000000000";

	bool IsRegisterInputTap()
	{
#if defined(__ANDROID__) || defined(MU_IOS)
		return SEASON3B::IsPress(VK_LBUTTON)
			|| CInput::Instance().IsLBtnDn()
			|| ((GetKeyState(VK_LBUTTON) & 0x8000) != 0);
#else
		return ((GetKeyState(VK_LBUTTON) & 0x8000) != 0);
#endif
	}

	const char* ResolveRegisterLabel(const char* text, const char* fallback)
	{
		if (text == NULL || text[0] == '\0' || strcmp(text, "Null") == 0 || strcmp(text, "NULL") == 0)
		{
			return fallback;
		}

		return text;
	}

	void RenderRegisterText(HFONT font, float boxX, float boxY, float boxW, float boxH, int align, const char* text)
	{
		if (text == NULL || text[0] == '\0')
		{
			return;
		}

		DWORD backupTextColor = g_pRenderText->GetTextColor();
		DWORD backupBgColor = g_pRenderText->GetBgColor();

		gInterface.DrawBarForm(boxX, boxY, boxW, boxH, 0.0f, 0.0f, 0.0f, 0.60f);
		g_pRenderText->SetFont(font != NULL ? font : g_hFont);
		g_pRenderText->SetShadowText(0);
		g_pRenderText->SetTextColor(kRegisterTextColor);
		g_pRenderText->SetBgColor(0);
		g_pRenderText->RenderText((int)boxX, (int)boxY + 1, text, (int)boxW, 0, align);
		g_pRenderText->SetFont(g_hFont);
		g_pRenderText->SetTextColor(backupTextColor);
		g_pRenderText->SetBgColor(backupBgColor);
	}

	void RenderRegisterInput(
		CUITextInputBox*& input,
		float posX,
		float posY,
		float width,
		float height,
		UIOPTIONS option,
		int maxLength,
		bool isPassword)
	{
		if (input == NULL)
		{
			input = new CUITextInputBox;
			input->Init(g_hWnd, static_cast<int>(width), static_cast<int>(height), maxLength, isPassword ? TRUE : FALSE);
			input->SetTextColor(255, 255, 255, 255);
			input->SetBackColor(255, 0, 0, 0);
			input->SetFont(g_hFont);
			input->SetState(UISTATE_NORMAL);
			input->SetOption(option);
			input->SetText("");
		}

		input->SetPosition(static_cast<int>(posX), static_cast<int>(posY));
		input->Render();
#if !defined(__ANDROID__) && !defined(MU_IOS)
		input->DoAction();
#endif
	}

	void FocusRegisterInputOnTap(float posX, float posY, float width, float height, CUITextInputBox* input, const char* debugName = NULL)
	{
#if defined(__ANDROID__) || defined(MU_IOS)
		if (input == NULL)
		{
			return;
		}

		if (SEASON3B::CheckMouseIn((int)posX, (int)posY, (int)width, (int)height) == 1)
		{
			if (IsRegisterInputTap())
			{
				if (debugName != NULL)
				{
					char debugText[256];
					std::snprintf(
						debugText,
						sizeof(debugText),
						"REGISTER_FOCUS hit=%s mouse=%d,%d rect=%.1f,%.1f,%.1f,%.1f",
						debugName,
						MouseX,
						MouseY,
						posX,
						posY,
						width,
						height);
					OutputDebugStringA(debugText);
				}

				input->GiveFocus(TRUE);
				pSetCursorFocus = true;
				PlayBuffer(25, 0, 0);
			}
		}
#else
		(void)posX;
		(void)posY;
		(void)width;
		(void)height;
		(void)input;
#endif
	}

	void FocusRegisterCaptchaInput(CUITextInputBox* input)
	{
		if (input == NULL)
		{
			return;
		}

		input->GiveFocus(FALSE);

		HWND inputHandle = input->GetHandle();
		if (inputHandle == NULL)
		{
			return;
		}

		char currentText[8] = { 0 };
		input->GetText(currentText, sizeof(currentText));
		const int caretPos = (int)std::strlen(currentText);
		SendMessageW(inputHandle, EM_SETSEL, (WPARAM)caretPos, (LPARAM)caretPos);
	}

	void RenderRegisterCaptcha(float posX, float posY, CUITextInputBox* input, const char* text)
	{
		if (input == NULL || text == NULL)
		{
			return;
		}

		gInterface.DrawBarForm(posX, posY + 3.5f, kCaptchaWidth, kCaptchaHeight, 0.0f, 0.0f, 0.0f, 1.0f);
		gInterface.DrawBarForm(posX + 2.0f, posY + 4.0f, kCaptchaCodeWidth, 15.0f, 1.0f, 0.2167f, 0.0f, 1.0f);
		TextDraw((HFONT)g_hFontBold, posX + 2.0f, posY + 6.0f, 0xFFFFFFB8, 0x0, (int)kCaptchaCodeWidth, 0, 3, text);

		input->SetPosition(posX + kCaptchaInputOffsetX, posY + kCaptchaInputOffsetY);
		input->Render();
		input->DoAction();

#if defined(__ANDROID__) || defined(MU_IOS)
		const float inputTapX = posX + kCaptchaInputOffsetX - 4.0f;
		const float inputTapY = posY - 5.0f;
		const float inputTapWidth = kCaptchaWidth - kCaptchaInputOffsetX + 8.0f;
		const float inputTapHeight = 30.0f;

		FocusRegisterInputOnTap(inputTapX, inputTapY, inputTapWidth, inputTapHeight, input, "captcha_input");

		if (SEASON3B::CheckMouseIn((int)inputTapX, (int)inputTapY, (int)inputTapWidth, (int)inputTapHeight) == 1
			&& IsRegisterInputTap())
		{
			FocusRegisterCaptchaInput(input);
			pSetCursorFocus = true;
		}
#endif
	}
}

CB_DangKyInGame::CB_DangKyInGame()
{
	for (int i = 0; i < TYPE_INPUT_DKTK::eMaxINPUT; ++i)
	{
		CInputData[i] = NULL;
	}

	CInputCaptCha = NULL;
	TimeSendRegTK = GetTickCount();
	OpenDKTK = false;
}

CB_DangKyInGame::~CB_DangKyInGame()
{
	Clear();
}

void CB_DangKyInGame::Clear()
{
	for (int i = 0; i < TYPE_INPUT_DKTK::eMaxINPUT; ++i)
	{
		SAFE_DELETE(CInputData[i]);
	}

	SAFE_DELETE(CInputCaptCha);

	TimeSendRegTK = GetTickCount();
	OpenDKTK = false;
}

void CB_DangKyInGame::OpenOnOff()
{
	if (GetTickCount() - gInterface.Data[eWindow_DangKyInGame].EventTick <= 300)
	{
		return;
	}

	gInterface.Data[eWindow_DangKyInGame].EventTick = GetTickCount();

	if (gInterface.Data[eWindow_DangKyInGame].OnShow)
	{
		gInterface.Data[eWindow_DangKyInGame].Close();
		Clear();
		return;
	}

	gInterface.Data[eWindow_DangKyInGame].Open();
	OpenDKTK = true;
	gInterface.vCaptcha = gInterface.generateCaptcha(4);
}

bool CB_DangKyInGame::RenderWindow(int X, int Y)
{
	(void)X;
	(void)Y;

	if (!gInterface.Data[eWindow_DangKyInGame].OnShow || gCB_DangKyInGame == NULL)
	{
		if (OpenDKTK)
		{
			Clear();
		}

		return false;
	}

	OpenDKTK = true;

	const char* labels[TYPE_INPUT_DKTK::eMaxINPUT] =
	{
		"TAI KHOAN :",
		"MAT KHAU :",
		"7 SO BAO MAT :",
		"SO DIEN THOAI :"
	};

	const int inputOptions[TYPE_INPUT_DKTK::eMaxINPUT] =
	{
		UIOPTION_NOLOCALIZEDCHARACTERS,
		UIOPTION_NOLOCALIZEDCHARACTERS,
		UIOPTION_NUMBERONLY,
		UIOPTION_NUMBERONLY
	};

	const int maxInput[TYPE_INPUT_DKTK::eMaxINPUT] =
	{
		MAX_ID_SIZE,
		MAX_PASSWORD_SIZE,
		7,
		11
	};

	float startX = (MAX_WIN_WIDTH / 2) - (kRegisterWindowWidth / 2);
	float startY = 40.0f;
	float inputPosX = 0.0f;
	float inputPosY[TYPE_INPUT_DKTK::eMaxINPUT] = { 0.0f };
	gInterface.Data[eWindow_DangKyInGame].AllowMove = false;

	g_pBCustomMenuInfo->gDrawWindowCustom(
		&startX,
		&startY,
		kRegisterWindowWidth,
		kRegisterWindowHeight,
		eWindow_DangKyInGame,
		"Dang Ky Tai Khoan");

	if (!gInterface.Data[eWindow_DangKyInGame].OnShow)
	{
		Clear();
		return false;
	}

	inputPosX = startX + 120.0f;

	auto submitRegister = [&]()
	{
		if (CInputCaptCha == NULL)
		{
			return;
		}

		char captchaText[8] = { 0 };
		CInputCaptCha->GetText(captchaText, sizeof(captchaText));
		std::string captchaInput(captchaText);

		if (!gInterface.check_Captcha(gInterface.vCaptcha, captchaInput))
		{
			gInterface.OpenMessageBox("Error", "Invalid Captcha");
			return;
		}

		RequsetDKTK();
	};

	RenderRegisterText(g_hFontBold, startX + 20.0f, startY + 48.0f, kRegisterWindowWidth - 40.0f, 14.0f, 3, "Tai khoan chi duoc su dung");
	RenderRegisterText(g_hFontBold, startX + 20.0f, startY + 62.0f, kRegisterWindowWidth - 40.0f, 14.0f, 3, "cac ky tu 0-9, a-z");

	startY += 30.0f;

	for (int i = Account; i <= Snonumber; ++i)
	{
		RenderRegisterText(g_hFontBold, startX + 18.0f, startY + 48.0f, 96.0f, 16.0f, 1, labels[i]);
		inputPosY[i] = startY + 50.0f;
		gInterface.DrawBarForm(inputPosX - 3.0f, inputPosY[i] - 3.0f, kInputWidth, 16.0f, 0.0f, 0.0f, 0.0f, 1.0f);
		RenderRegisterInput(
			CInputData[i],
			inputPosX,
			inputPosY[i],
			kInputWidth,
			14.0f,
			(UIOPTIONS)inputOptions[i],
			maxInput[i],
			(i == Pass));

		startY += kInputSpacing;
	}

	startY += 10.0f;
	startY += 10.0f;

	const float captchaX = startX + 105.0f;
	const float captchaY = startY + 45.0f;
	RenderRegisterText(
		(HFONT)g_hFontBold,
		startX + 18.0f,
		startY + 48.0f,
		96.0f,
		16.0f,
		1,
		ResolveRegisterLabel(gOther.TextVN_NAPGAME[13], "Captcha :"));

	if (CInputCaptCha == NULL)
	{
		CInputCaptCha = new CUITextInputBox;
		CInputCaptCha->Init(g_hWnd, (int)kCaptchaInputWidth, (int)kCaptchaInputHeight, 4);
		CInputCaptCha->SetBackColor(0, 0, 0, 0);
		CInputCaptCha->SetTextColor(255, 255, 255, 255);
		CInputCaptCha->SetFont((HFONT)g_hFont);
		CInputCaptCha->SetState(UISTATE_NORMAL);
		CInputCaptCha->SetOption(UIOPTION_NUMBERONLY);
		CInputCaptCha->SetText("");
	}

	if (CInputData[Account] != NULL && CInputData[Pass] != NULL)
	{
		CInputData[Account]->SetTabTarget(CInputData[Pass]);
	}

	if (CInputData[Pass] != NULL && CInputData[Snonumber] != NULL)
	{
		CInputData[Pass]->SetTabTarget(CInputData[Snonumber]);
	}

	if (CInputData[Snonumber] != NULL && CInputData[Phone] != NULL)
	{
		CInputData[Snonumber]->SetTabTarget(CInputData[Phone]);
	}

	if (CInputData[Snonumber] != NULL && CInputCaptCha != NULL)
	{
		CInputData[Snonumber]->SetTabTarget(CInputCaptCha);
	}

	if (CInputCaptCha != NULL)
	{
		CInputCaptCha->SetTabTarget(NULL);
	}

	RenderRegisterCaptcha(captchaX, captchaY, CInputCaptCha, gInterface.vCaptcha.c_str());

	for (int i = Account; i <= Snonumber; ++i)
	{
		FocusRegisterInputOnTap(inputPosX - 3.0f, inputPosY[i] - 3.0f, kInputWidth + 6.0f, 20.0f, CInputData[i]);
	}

	startY += 30.0f;

	if (g_pBCustomMenuInfo->DrawButton(
		startX + 100.0f,
		startY + 45.0f,
		100.0f,
		12,
		const_cast<char*>(ResolveRegisterLabel(gOther.TextVN_NAPGAME[14], "Dang Ky")),
		80.0f,
		true))
	{
		submitRegister();
	}
#if defined(__ANDROID__) || defined(MU_IOS)
	else if (CInputCaptCha != NULL && CInputCaptCha->HaveFocus() && SEASON3B::IsPress(VK_RETURN))
	{
		submitRegister();
	}
#endif

	gInterface.DrawMessageBox();
	return true;
}

bool CB_DangKyInGame::RequsetDKTK()
{
	if (CInputData[Account] == NULL
		|| CInputData[Pass] == NULL
		|| CInputData[Snonumber] == NULL)
	{
		return false;
	}

	char szID[MAX_ID_SIZE + 1] = { 0 };
	char szPass[MAX_PASSWORD_SIZE + 1] = { 0 };
	char szSno[7 + 1] = { 0 };
	char szSDT[11 + 1] = { 0 };

	CInputData[Account]->GetText(szID, MAX_ID_SIZE + 1);
	CInputData[Pass]->GetText(szPass, MAX_PASSWORD_SIZE + 1);
	CInputData[Snonumber]->GetText(szSno, sizeof(szSno));
	std::memcpy(szSDT, kDefaultRegisterPhone, min(sizeof(szSDT) - 1, std::strlen(kDefaultRegisterPhone)));

	if (TimeSendRegTK > GetTickCount())
	{
		gInterface.OpenMessageBox("Error", "Thao tac cham lai");
		return false;
	}

	if (strlen(szID) < 1)
	{
		gInterface.OpenMessageBox("Error", "Vui long nhap tai khoan");
		return false;
	}

	if (strlen(szPass) < 1)
	{
		gInterface.OpenMessageBox("Error", "Vui long nhap mat khau");
		return false;
	}

	if (strlen(szSno) < 7)
	{
		gInterface.OpenMessageBox("Error", "Vui long nhap 7 so bao mat");
		return false;
	}

	if (!CheckChuoiKyTuDacBiet(szID) || !CheckChuoiKyTuDacBiet(szPass))
	{
		gInterface.OpenMessageBox("Error", "Tai khoan hoac mat khau co ky tu khong hop le");
		return false;
	}

	PMSG_REGISTER_MAIN_SEND pMsg;
	pMsg.header.set(0xD3, 0x05, sizeof(pMsg));
	pMsg.TypeSend = 0x01;

	memset(pMsg.account, 0, sizeof(pMsg.account));
	memset(pMsg.password, 0, sizeof(pMsg.password));
	memset(pMsg.numcode, 0, sizeof(pMsg.numcode));
	memset(pMsg.sodienthoai, 0, sizeof(pMsg.sodienthoai));

	memcpy(pMsg.account, szID, min(sizeof(pMsg.account) - 1, strlen(szID)));
	memcpy(pMsg.password, szPass, min(sizeof(pMsg.password) - 1, strlen(szPass)));
	memcpy(pMsg.numcode, szSno, min(sizeof(pMsg.numcode) - 1, strlen(szSno)));
	memcpy(pMsg.sodienthoai, szSDT, min(sizeof(pMsg.sodienthoai) - 1, strlen(szSDT)));

	DataSend((LPBYTE)&pMsg, pMsg.header.size);

	gInterface.vCaptcha = gInterface.generateCaptcha(4);
	TimeSendRegTK = GetTickCount() + 5000;

	return true;
}

void CB_DangKyInGame::RecvKQRegInGame(XULY_CGPACKET* lpMsg)
{
	if (lpMsg == NULL)
	{
		return;
	}

	char szID[MAX_ID_SIZE + 1] = { 0 };
	char szPass[MAX_PASSWORD_SIZE + 1] = { 0 };

	if (CInputData[Account] != NULL)
	{
		CInputData[Account]->GetText(szID, sizeof(szID));
	}

	if (CInputData[Pass] != NULL)
	{
		CInputData[Pass]->GetText(szPass, sizeof(szPass));
	}

	switch (lpMsg->ThaoTac)
	{
	case CB_DangKyInGame::eDangKyThanhCong:
		{
			gInterface.OpenMessageBox("Ket Qua", "Dang ky thanh cong\nID : %s", szID);
			CUIMng& rUIMng = CUIMng::Instance();
			rUIMng.m_LoginWin.GetIDInputBox()->SetText(szID);
			rUIMng.m_LoginWin.GetPassInputBox()->SetText(szPass);
			gInterface.Data[eWindow_DangKyInGame].Close();
			Clear();
		}
		break;

	case CB_DangKyInGame::eTaiKhoanDaTonTai:
		gInterface.OpenMessageBox("Ket Qua", "ID %s da ton tai", szID);
		break;

	case CB_DangKyInGame::eDuLieuNhapKhongDung:
		gInterface.OpenMessageBox("Ket Qua", "Thong tin nhap khong hop le");
		break;

	default:
		break;
	}
}
