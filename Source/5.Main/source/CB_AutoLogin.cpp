#include "StdAfx.h"
#include "CB_AutoLogin.h"
#include "NewUISystem.h"
#include "CBInterface.h"
#include "CUIController.h"
#include "CharacterManager.h"
#include "Util.h"
#include "Protocol.h"
#include "NewUIBase.h"
#include "Other.h"
#include "ZzzInterface.h"
#include "./ExternalObject/leaf/regkey.h"
#include "UIMng.h"
CB_AutoLogin* gCB_AutoLogin;
extern char m_ID[11];
extern char m_Username[11];
extern char m_Password[11];

CB_AutoLogin::CB_AutoLogin()
{
	this->showListAccount = false;
	this->selectedAccount = 0;
	this->totalSavedAcc = 0;
	this->TickCount = 0;
}

CB_AutoLogin::~CB_AutoLogin()
{
}
void CB_AutoLogin::DrawInfo(int XPos, int YPos)
{
	if (g_pBCustomMenuInfo->RenderCheckBoxMini(int((XPos + 124) / g_fScreenRate_x),
		int((YPos + 158) / g_fScreenRate_y), 0xFFFFFFFF, m_SavePassOnOff == 1 ? TRUE : FALSE, "Save password") && !this->showListAccount)
	{
		m_SavePassOnOff ^= 1;

	}

	if (this->showListAccount)
	{
		if (this->totalSavedAcc == 0)
		{
			this->showListAccount = 0;
		}
		int KC = 15;
		
		for (int i = 0; i < this->totalSavedAcc; i++)
		{
			DWORD BGColor = 0x000000FF;
			if (SEASON3B::CheckMouseIn(int((XPos + 114) / g_fScreenRate_x), int((YPos + 111 + (KC * i)) / g_fScreenRate_y), 105, KC) == 1)
			{
				BGColor = 0xA5A100FF;
				if (GetKeyState(VK_LBUTTON) & 0x8000 && GetTickCount() > this->TickCount +500)
				{
					PlayBuffer(25, 0, 0);
					this->SetSelectedAccount(i);
					this->showListAccount = 0;
					this->TickCount = GetTickCount();
				}
			}
			TextDraw(g_hFont, int((XPos + 114) / g_fScreenRate_x), int((YPos + 111 + (KC*i)) / g_fScreenRate_y), 0xFFFFFFFF, BGColor, 115, 0, 1, this->saved_acc[i].ID); //ID
			//delete
			if (g_pBCustomMenuInfo->DrawButtonGUI(CNewUIPartyInfoWindow::IMAGE_PARTY_EXIT, int((XPos + 245) / g_fScreenRate_x), int((YPos + 111 + (KC * i)) / g_fScreenRate_y), 12, 12))
			{
				this->RemoveAccount(i);
			}
		}
	}
	else
	{
		if (g_pBCustomMenuInfo->DrawButtonGUI(CNewUICastleWindow::IMAGE_CASTLEWINDOW_SCROLL_DOWN_BTN, int((XPos + 245) / g_fScreenRate_x), int((YPos + 110) / g_fScreenRate_y), 14, 14))
		{
			this->showListAccount ^= 1;
			this->TickCount = GetTickCount();
		}
	}
}
void CB_AutoLogin::SetShowListAccount(bool show)
{
	this->showListAccount = show;
}
void CB_AutoLogin::SetSelectedAccount(int Index)
{
	this->selectedAccount = Index;
	CUIMng& rUIMng = CUIMng::Instance();
	if (m_SavePassOnOff)
	{
		rUIMng.m_LoginWin.GetIDInputBox()->SetText(this->saved_acc[this->selectedAccount].ID);
		rUIMng.m_LoginWin.GetPassInputBox()->SetText(this->saved_acc[this->selectedAccount].PW);
	}
	else
	{
		rUIMng.m_LoginWin.GetIDInputBox()->SetText(this->saved_acc[this->selectedAccount].ID);
		rUIMng.m_LoginWin.GetPassInputBox()->SetText("");
	}
#if defined(__ANDROID__) || defined(MU_IOS)
	if (this->totalSavedAcc > 0 && Index >= 0 && Index < this->totalSavedAcc)
	{
		strncpy(m_ID, this->saved_acc[Index].ID, sizeof(m_ID) - 1);
		m_ID[sizeof(m_ID) - 1] = '\0';
		strncpy(m_Username, this->saved_acc[Index].ID, sizeof(m_Username) - 1);
		m_Username[sizeof(m_Username) - 1] = '\0';
		if (m_SavePassOnOff)
		{
			strncpy(m_Password, this->saved_acc[Index].PW, sizeof(m_Password) - 1);
			m_Password[sizeof(m_Password) - 1] = '\0';
		}
	}
#endif
}
void CB_AutoLogin::RemoveAccount(int Index)
{
	if (Index < 0 && Index >= this->totalSavedAcc)
	{
		return;
	}
#if defined(__ANDROID__) || defined(MU_IOS)
	FILE* fp = fopen("autologin.dat", "w");
	if (fp)
	{
		fprintf(fp, "%d\n", m_SavePassOnOff ? 1 : 0);
		for (int i = 0; i < this->totalSavedAcc; ++i)
		{
			if (i == Index) continue;
			fprintf(fp, "%s %s\n", this->saved_acc[i].ID, this->saved_acc[i].PW);
		}
		fclose(fp);
	}
	this->ReadConfigs();
#else
	leaf::CRegKey regkey;
	regkey.SetKey(leaf::CRegKey::_HKEY_CURRENT_USER, "SOFTWARE\\Webzen\\Mu\\Config");
	if (Index == 0)
	{
		regkey.WriteString("ID", "");
		regkey.WriteString("PW", "");
	}
	else {
		char zKey[50];

		sprintf(zKey, "ID_%d", Index + 1);
		regkey.WriteString(zKey, "");
		sprintf(zKey, "PW_%d", Index + 1);
		regkey.WriteString(zKey, "");
	}
	this->ReadConfigs();
#endif
}
void CB_AutoLogin::ReadConfigs()
{
#if defined(__ANDROID__) || defined(MU_IOS)
	this->totalSavedAcc = 0;
	m_SavePassOnOff = true;
	FILE* fp = fopen("autologin.dat", "r");
	if (fp)
	{
		int savePass = 1;
		if (fscanf(fp, "%d\n", &savePass) == 1)
		{
			m_SavePassOnOff = (savePass != 0);
		}
		char line[128];
		while (this->totalSavedAcc < MAX_ACCOUNT_SAVE && fgets(line, sizeof(line), fp))
		{
			char id[32] = {0};
			char pw[32] = {0};
			if (sscanf(line, "%31s %31s", id, pw) == 2)
			{
				strncpy(this->saved_acc[this->totalSavedAcc].ID, id, 10);
				this->saved_acc[this->totalSavedAcc].ID[10] = '\0';
				strncpy(this->saved_acc[this->totalSavedAcc].PW, pw, 10);
				this->saved_acc[this->totalSavedAcc].PW[10] = '\0';
				this->saved_acc[this->totalSavedAcc].index = this->totalSavedAcc;
				this->totalSavedAcc++;
			}
		}
		fclose(fp);
	}
	if (this->totalSavedAcc == 0)
	{
		FILE* cfp = fopen("credentials.dat", "r");
		if (cfp)
		{
			char u[128] = {0};
			char p[128] = {0};
			if (fgets(u, sizeof(u), cfp))
			{
				u[strcspn(u, "\r\n")] = '\0';
				if (fgets(p, sizeof(p), cfp))
				{
					p[strcspn(p, "\r\n")] = '\0';
				}
				if (u[0] != '\0')
				{
					strncpy(this->saved_acc[0].ID, u, 10);
					this->saved_acc[0].ID[10] = '\0';
					strncpy(this->saved_acc[0].PW, p, 10);
					this->saved_acc[0].PW[10] = '\0';
					this->saved_acc[0].index = 0;
					this->totalSavedAcc = 1;
					m_SavePassOnOff = 1;
				}
			}
			fclose(cfp);
		}
	}
	if (this->totalSavedAcc > 0)
	{
		strncpy(m_ID, this->saved_acc[0].ID, sizeof(m_ID) - 1);
		m_ID[sizeof(m_ID) - 1] = '\0';
		strncpy(m_Username, this->saved_acc[0].ID, sizeof(m_Username) - 1);
		m_Username[sizeof(m_Username) - 1] = '\0';
		if (m_SavePassOnOff)
		{
			strncpy(m_Password, this->saved_acc[0].PW, sizeof(m_Password) - 1);
			m_Password[sizeof(m_Password) - 1] = '\0';
		}
	}
#else
	HKEY hKey;
	DWORD dwDisp;
	DWORD dwSize;
	if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Webzen\\Mu\\Config", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisp))
	{
		//==============
		this->totalSavedAcc = 0;
		char zKey[50];
		for (int i = 0; i < MAX_ACCOUNT_SAVE; i++) {
			if (i) {
				sprintf(zKey, "ID_%d", i + 1);
			}
			else {
				sprintf(zKey, "ID");
			}
			dwSize = 11;
			if (RegQueryValueEx(hKey, zKey, 0, NULL, (LPBYTE)this->saved_acc[this->totalSavedAcc].ID, &dwSize) != ERROR_SUCCESS ||
				dwSize < 1)
			{
				ZeroMemory(this->saved_acc[this->totalSavedAcc].ID, sizeof(this->saved_acc[this->totalSavedAcc].ID));
				continue;
			}
			if (i) {
				sprintf(zKey, "PW_%d", i + 1);
			}
			else {
				sprintf(zKey, "PW");
			}
			dwSize = 11;
			if (RegQueryValueEx(hKey, zKey, 0, NULL, (LPBYTE)this->saved_acc[this->totalSavedAcc].PW, &dwSize) != ERROR_SUCCESS ||
				dwSize < 1)
			{
				ZeroMemory(this->saved_acc[this->totalSavedAcc].PW, sizeof(this->saved_acc[this->totalSavedAcc].PW));
				continue;
			}
			this->saved_acc[this->totalSavedAcc].index = i;
			this->totalSavedAcc++;
		}
		dwSize = sizeof(int);
		if (RegQueryValueEx(hKey, "SavePass", 0, NULL, (LPBYTE)& m_SavePassOnOff, &dwSize) != ERROR_SUCCESS)
		{
			m_SavePassOnOff = false;
		}
	}

	m_SavePassOnOff = true;
#endif
}

void CB_AutoLogin::SaveData(char* szID, char* szPass)
{
#if defined(__ANDROID__) || defined(MU_IOS)
	if (!szID || szID[0] == '\0') return;
	FILE* fp = fopen("autologin.dat", "w");
	if (fp)
	{
		fprintf(fp, "%d\n", m_SavePassOnOff ? 1 : 0);
		fprintf(fp, "%s %s\n", szID, szPass ? szPass : "");
		int successCount = 1;
		for (int i = 0; i < this->totalSavedAcc; ++i)
		{
			if (successCount >= MAX_ACCOUNT_SAVE) break;
			if (strcmp(szID, this->saved_acc[i].ID) == 0) continue;
			fprintf(fp, "%s %s\n", this->saved_acc[i].ID, this->saved_acc[i].PW);
			successCount++;
		}
		fclose(fp);
	}
	this->ReadConfigs();
#else
	leaf::CRegKey regkey;
	regkey.SetKey(leaf::CRegKey::_HKEY_CURRENT_USER, "SOFTWARE\\Webzen\\Mu\\Config");
	char zKey[50];
	int i;
	if (this->totalSavedAcc > 0) {
		//clear all for sure, infact we can clear the rest for performance
		for (i = 1; i < MAX_ACCOUNT_SAVE; i++) {
			sprintf(zKey, "ID_%d", i + 1);
			regkey.WriteString(zKey, "");
			sprintf(zKey, "PW_%d", i + 1);
			regkey.WriteString(zKey, "");
		}
		//save new data
		int successCount = 1;
		for (i = 0; i < this->totalSavedAcc; i++) {
			if (successCount >= MAX_ACCOUNT_SAVE) {
				break;
			}
			if (strcmp(szID, this->saved_acc[i].ID) == 0) { //duplicate account
				continue;
			}
			sprintf(zKey, "ID_%d", successCount + 1);
			regkey.WriteString(zKey, this->saved_acc[i].ID);
			sprintf(zKey, "PW_%d", successCount + 1);
			regkey.WriteString(zKey, this->saved_acc[i].PW);
			successCount++;
		}
	}
	regkey.WriteString("ID", szID);
	regkey.WriteString("PW", szPass);
	regkey.WriteDword("SavePass", m_SavePassOnOff ? 1 : 0);
#endif
}