// CustomMessage.h: interface for the CCustomMessage class.
//
//////////////////////////////////////////////////////////////////////

#pragma once



struct CUSTOM_MESSAGE_INFO
{
	int Index;
	char Message[128];
};

class CCustomMessage
{
public:
	CCustomMessage();
	virtual ~CCustomMessage();
	void Init();
	void LoadEng(CUSTOM_MESSAGE_INFO* info);
	void LoadVtm(CUSTOM_MESSAGE_INFO* info);
	void SetInfoEng(CUSTOM_MESSAGE_INFO info);
	void SetInfoVtm(CUSTOM_MESSAGE_INFO info);
	CUSTOM_MESSAGE_INFO* GetInfoEng(int index);
	CUSTOM_MESSAGE_INFO* GetInfoVtm(int index);
	char* GetMessage(int index);
#ifndef GetMessage
	char* GetMessageA(int index) { return GetMessage(index); }
#endif
	char * GetMessageB(int index); // Text.bmd
public:
	char m_DefaultMessage[128];
	CUSTOM_MESSAGE_INFO m_EngCustomMessageInfo[MAX_CUSTOM_MESSAGE];
	CUSTOM_MESSAGE_INFO m_VtmCustomMessageInfo[MAX_CUSTOM_MESSAGE];
};

extern CCustomMessage gCustomMessage;
extern std::string g_strSelectedML;
