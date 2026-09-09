#include "stdafx.h"
#include "GameConfig.h"
#include "GameConfigConstants.h"

#if !defined(__ANDROID__) && !defined(MU_IOS)

#include <imagehlp.h>
#include "GameConfigConstants.h"
#include <windows.h>

GameConfig& GameConfig::GetInstance()
{
    static GameConfig instance;
    return instance;
}

GameConfig::GameConfig()
{
    // Get executable directory and construct config path
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);

    // Find last backslash to get directory
    wchar_t* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash)
    {
        *(lastSlash + 1) = L'\0';  // Keep the trailing backslash
    }

    m_configPath = exePath;
    m_configPath += L"config.ini";

    Load();
}

void GameConfig::Load()
{
    using namespace CfgSections;
    using namespace CfgKeys;
    using namespace CfgDefaults;

    m_windowWidth  = ReadInt(CfgSectionWindow, CfgKeyWidth, CfgDefaultWindowWidth);
    m_windowHeight = ReadInt(CfgSectionWindow, CfgKeyHeight, CfgDefaultWindowHeight);
    m_windowMode   = ReadBool(CfgSectionWindow, CfgKeyWindowed, CfgDefaultWindowed);

    m_colorDepth = ReadInt(CfgSectionGraphics, CfgKeyColorDepth, CfgDefaultColorDepth);

    m_soundEnabled = ReadBool(CfgSectionAudio, CfgKeySoundEnabled, CfgDefaultSoundEnabled);
    m_musicEnabled = ReadBool(CfgSectionAudio, CfgKeyMusicEnabled, CfgDefaultMusicEnabled);
    m_volumeLevel  = ReadInt(CfgSectionAudio, CfgKeyVolumeLevel, CfgDefaultVolumeLevel);

    m_renderTextType = ReadInt(CfgSectionGraphics, CfgKeyRenderTextType, CfgDefaultRenderTextType);

    m_rememberMe        = ReadBool(CfgSectionLogin, CfgKeyRememberMe, CfgDefaultRememberMe);
    m_languageSelection = ReadString(CfgSectionLogin, CfgKeyLanguage, CfgDefaultLanguage);
    m_encryptedUsername = ReadString(CfgSectionLogin, CfgKeyEncryptedUsername, CfgDefaultEncryptedUsername);
    m_encryptedPassword = ReadString(CfgSectionLogin, CfgKeyEncryptedPassword, CfgDefaultEncryptedPassword);

    m_serverIP   = ReadString(CfgSectionConnectionSettings, CfgKeyServerIP, CfgDefaultServerIP);
    m_serverPort = ReadInt(CfgSectionConnectionSettings, CfgKeyServerPort, CfgDefaultServerPort);
}

void GameConfig::Save()
{
    using namespace CfgSections;
    using namespace CfgKeys;

    WriteInt(CfgSectionWindow, CfgKeyWidth, m_windowWidth);
    WriteInt(CfgSectionWindow, CfgKeyHeight, m_windowHeight);
    WriteBool(CfgSectionWindow, CfgKeyWindowed, m_windowMode);

    WriteInt(CfgSectionGraphics, CfgKeyColorDepth, m_colorDepth);
    WriteInt(CfgSectionGraphics, CfgKeyRenderTextType, m_renderTextType);

    WriteBool(CfgSectionAudio, CfgKeySoundEnabled, m_soundEnabled);
    WriteBool(CfgSectionAudio, CfgKeyMusicEnabled, m_musicEnabled);
    WriteInt(CfgSectionAudio, CfgKeyVolumeLevel, m_volumeLevel);

    WriteBool(CfgSectionLogin, CfgKeyRememberMe, m_rememberMe);
    WriteString(CfgSectionLogin, CfgKeyLanguage, m_languageSelection);
    WriteString(CfgSectionLogin, CfgKeyEncryptedUsername, m_encryptedUsername);
    WriteString(CfgSectionLogin, CfgKeyEncryptedPassword, m_encryptedPassword);

    WriteString(CfgSectionConnectionSettings, CfgKeyServerIP, m_serverIP);
    WriteInt(CfgSectionConnectionSettings, CfgKeyServerPort, m_serverPort);
}

void GameConfig::SetWindowSize(int width, int height)
{
    m_windowWidth = width;
    m_windowHeight = height;
}

void GameConfig::SetWindowMode(bool windowed)
{
    m_windowMode = windowed;
}

void GameConfig::SetColorDepth(int depth)
{
    m_colorDepth = depth;
}

void GameConfig::SetSoundEnabled(bool enabled)
{
    m_soundEnabled = enabled;
}

void GameConfig::SetMusicEnabled(bool enabled)
{
    m_musicEnabled = enabled;
}

void GameConfig::SetVolumeLevel(int level)
{
    m_volumeLevel = level;
}

void GameConfig::SetRenderTextType(int type)
{
    m_renderTextType = type;
}

void GameConfig::SetRememberMe(bool remember)
{
    m_rememberMe = remember;
}

void GameConfig::SetLanguageSelection(const std::wstring& lang)
{
    m_languageSelection = lang;
}

void GameConfig::SetEncryptedUsername(const std::wstring& encryptedUsername)
{
    m_encryptedUsername = encryptedUsername;
}

void GameConfig::SetEncryptedPassword(const std::wstring& encryptedPassword)
{
    m_encryptedPassword = encryptedPassword;
}

void GameConfig::SetServerIP(const std::wstring& ip)
{
    m_serverIP = ip;
}

void GameConfig::SetServerPort(int port)
{
    m_serverPort = port;
}

// Helper function to convert binary data to hex string
std::wstring GameConfig::BinaryToHex(const BYTE* data, DWORD size)
{
    std::wstring hex;
    hex.reserve(size * 2);

    const wchar_t hexChars[] = L"0123456789ABCDEF";
    for (DWORD i = 0; i < size; ++i)
    {
        hex += hexChars[(data[i] >> 4) & 0x0F];
        hex += hexChars[data[i] & 0x0F];
    }

    return hex;
}

// Helper function to convert hex string to binary data
std::vector<BYTE> GameConfig::HexToBinary(const std::wstring& hex)
{
    std::vector<BYTE> binary;

    if (hex.empty() || hex.length() % 2 != 0)
        return binary;

    binary.reserve(hex.length() / 2);

    auto hex_char_to_byte = [](wchar_t c) -> BYTE {
        if (c >= L'0' && c <= L'9') return (c - L'0');
        if (c >= L'a' && c <= L'f') return (c - L'a' + 10);
        return (c - L'A' + 10);
    };

    for (size_t i = 0; i < hex.length(); i += 2)
    {
        wchar_t high = hex[i];
        wchar_t low = hex[i + 1];

        if (!iswxdigit(high) || !iswxdigit(low))
        {
            // Invalid hex character detected, return empty vector
            return {};
        }

        binary.push_back((hex_char_to_byte(high) << 4) | hex_char_to_byte(low));
    }

    return binary;
}

void GameConfig::DecryptCredentials(wchar_t* outUser, wchar_t* outPass, size_t userBufSize, size_t passBufSize)
{
    // Decrypt Username
    std::wstring user = DecryptSetting(GetEncryptedUsername());
    if (!user.empty()) {
        wcsncpy_s(outUser, userBufSize, user.c_str(), _TRUNCATE);
    }

    // Decrypt Password
    std::wstring pass = DecryptSetting(GetEncryptedPassword());
    if (!pass.empty()) {
        wcsncpy_s(outPass, passBufSize, pass.c_str(), _TRUNCATE);
    }
}

// Helper functions using Windows INI API
int GameConfig::ReadInt(const wchar_t* section, const wchar_t* key, int defaultValue)
{
    return GetPrivateProfileIntW(section, key, defaultValue, m_configPath.c_str());
}

void GameConfig::WriteInt(const wchar_t* section, const wchar_t* key, int value)
{
    wchar_t buffer[32];
    swprintf_s(buffer, L"%d", value);

    WritePrivateProfileStringW(section, key, buffer, m_configPath.c_str());
}

bool GameConfig::ReadBool(const wchar_t* section, const wchar_t* key, bool defaultValue)
{
    return GetPrivateProfileIntW(section, key, defaultValue ? 1 : 0, m_configPath.c_str()) != 0;
}

void GameConfig::WriteBool(const wchar_t* section, const wchar_t* key, bool value)
{
    WritePrivateProfileStringW(section, key, value ? L"1" : L"0", m_configPath.c_str());
}

std::wstring GameConfig::ReadString(const wchar_t* section, const wchar_t* key, const std::wstring& defaultValue)
{
    std::vector<wchar_t> buffer(2048);
    while (true)
    {
        DWORD charsRead = GetPrivateProfileStringW(section, key, defaultValue.c_str(), buffer.data(), static_cast<DWORD>(buffer.size()), m_configPath.c_str());
        if (charsRead < buffer.size() - 1)
        {
            return std::wstring(buffer.data());
        }
        buffer.resize(buffer.size() * 2);
    }
}

void GameConfig::WriteString(const wchar_t* section, const wchar_t* key, const std::wstring& value)
{
    WritePrivateProfileStringW(section, key, value.c_str(), m_configPath.c_str());
}

std::wstring GameConfig::DecryptSetting(const std::wstring& hexInput)
{
    if (hexInput.empty()) return L"";

    // Convert Hex String back to Binary Blob
    std::vector<BYTE> encryptedData = HexToBinary(hexInput);
    if (encryptedData.empty()) return L"";

    DATA_BLOB dataIn, dataOut;
    dataIn.pbData = encryptedData.data();
    dataIn.cbData = static_cast<DWORD>(encryptedData.size());

    // Decrypt using Windows DPAPI
    if (CryptUnprotectData(&dataIn, nullptr, nullptr, nullptr, nullptr, 0, &dataOut))
    {
        std::wstring result(reinterpret_cast<wchar_t*>(dataOut.pbData), dataOut.cbData / sizeof(wchar_t));
        LocalFree(dataOut.pbData); // Safety: Windows allocated this, we free it
        // The decrypted string might contain the null terminator, let's remove it if it exists.
        if (!result.empty() && result.back() == L'\0') {
            result.pop_back();
        }
        return result;
    }

    return L"";
}

std::wstring GameConfig::EncryptSetting(const wchar_t* input)
{
    if (!input || wcslen(input) == 0) return L"";

    DATA_BLOB dataIn, dataOut;
    dataIn.cbData = static_cast<DWORD>((wcslen(input) + 1) * sizeof(wchar_t));
    dataIn.pbData = reinterpret_cast<BYTE*>(const_cast<wchar_t*>(input));

    if (CryptProtectData(&dataIn, nullptr, nullptr, nullptr, nullptr, 0, &dataOut))
    {
        std::wstring hexResult = BinaryToHex(dataOut.pbData, dataOut.cbData);
        LocalFree(dataOut.pbData);
        return hexResult;
    }
    return L"";
}

void GameConfig::EncryptAndSaveCredentials(const wchar_t* user, const wchar_t* pass)
{
    std::wstring encUser = EncryptSetting(user);
    std::wstring encPass = EncryptSetting(pass);

    if (!encUser.empty() && !encPass.empty())
    {
        SetEncryptedUsername(encUser);
        SetEncryptedPassword(encPass);
        Save(); // Actually write to the .ini file
    }
}

#else // __ANDROID__
// Android: GameConfig implementation with credentials persistence
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>

GameConfig& GameConfig::GetInstance() {
    static GameConfig instance;
    return instance;
}

GameConfig::GameConfig()
    : m_windowWidth(800), m_windowHeight(600), m_windowMode(true),
      m_colorDepth(32), m_soundEnabled(true), m_musicEnabled(true),
      m_volumeLevel(10), m_renderTextType(0), m_rememberMe(false),
      m_languageSelection(L"Eng"), m_serverIP(CfgDefaults::CfgDefaultServerIP),
      m_serverPort(CfgDefaults::CfgDefaultServerPort)
{
    Load();
}

void GameConfig::Load() {
    FILE* fp = fopen("credentials.dat", "r");
    if (fp) {
        char u[128] = {0};
        char p[128] = {0};
        if (fgets(u, sizeof(u), fp)) {
            u[strcspn(u, "\r\n")] = '\0';
            if (fgets(p, sizeof(p), fp)) {
                p[strcspn(p, "\r\n")] = '\0';
            }
            if (u[0] != '\0') {
                wchar_t wUser[128] = {0};
                wchar_t wPass[128] = {0};
                std::mbstowcs(wUser, u, 127);
                std::mbstowcs(wPass, p, 127);
                m_encryptedUsername = wUser;
                m_encryptedPassword = wPass;
                m_rememberMe = true;
            }
        }
        fclose(fp);
    }

    if (m_encryptedUsername.empty()) {
        FILE* afp = fopen("autologin.dat", "r");
        if (afp) {
            int savePass = 1;
            fscanf(afp, "%d\n", &savePass);
            char line[128];
            if (fgets(line, sizeof(line), afp)) {
                char id[32] = {0};
                char pw[32] = {0};
                if (sscanf(line, "%31s %31s", id, pw) >= 1) {
                    wchar_t wUser[32] = {0};
                    wchar_t wPass[32] = {0};
                    std::mbstowcs(wUser, id, 31);
                    std::mbstowcs(wPass, pw, 31);
                    m_encryptedUsername = wUser;
                    m_encryptedPassword = wPass;
                    m_rememberMe = true;
                }
            }
            fclose(afp);
        }
    }
}

void GameConfig::Save() {
    if (!m_encryptedUsername.empty()) {
        EncryptAndSaveCredentials(m_encryptedUsername.c_str(), m_encryptedPassword.c_str());
    }
}

void GameConfig::SetWindowSize(int width, int height) { m_windowWidth = width; m_windowHeight = height; }
void GameConfig::SetWindowMode(bool windowed)         { m_windowMode = windowed; }
void GameConfig::SetColorDepth(int depth)             { m_colorDepth = depth; }
void GameConfig::SetSoundEnabled(bool enabled)        { m_soundEnabled = enabled; }
void GameConfig::SetMusicEnabled(bool enabled)        { m_musicEnabled = enabled; }
void GameConfig::SetVolumeLevel(int level)            { m_volumeLevel = level; }
void GameConfig::SetRenderTextType(int type)          { m_renderTextType = type; }
void GameConfig::SetRememberMe(bool remember)         { m_rememberMe = remember; }
void GameConfig::SetLanguageSelection(const std::wstring& lang) { m_languageSelection = lang; }
void GameConfig::SetEncryptedUsername(const std::wstring& u)    { m_encryptedUsername = u; }
void GameConfig::SetEncryptedPassword(const std::wstring& p)    { m_encryptedPassword = p; }
void GameConfig::SetServerIP(const std::wstring& ip)  { m_serverIP = ip; }
void GameConfig::SetServerPort(int port)              { m_serverPort = port; }

std::wstring GameConfig::BinaryToHex(const BYTE*, DWORD)         { return L""; }
std::vector<BYTE> GameConfig::HexToBinary(const std::wstring&)   { return {}; }

void GameConfig::DecryptCredentials(wchar_t* outUser, wchar_t* outPass, size_t userBufSize, size_t passBufSize) {
    if (outUser && userBufSize > 0) outUser[0] = L'\0';
    if (outPass && passBufSize > 0) outPass[0] = L'\0';

    if (!m_encryptedUsername.empty() && outUser && userBufSize > 0) {
        wcsncpy(outUser, m_encryptedUsername.c_str(), userBufSize - 1);
        outUser[userBufSize - 1] = L'\0';
    }
    if (!m_encryptedPassword.empty() && outPass && passBufSize > 0) {
        wcsncpy(outPass, m_encryptedPassword.c_str(), passBufSize - 1);
        outPass[passBufSize - 1] = L'\0';
    }

    if ((!outUser || outUser[0] == L'\0') && outUser && userBufSize > 0) {
        FILE* fp = fopen("credentials.dat", "r");
        if (fp) {
            char u[128] = {0};
            char p[128] = {0};
            if (fgets(u, sizeof(u), fp)) {
                u[strcspn(u, "\r\n")] = '\0';
                if (fgets(p, sizeof(p), fp)) {
                    p[strcspn(p, "\r\n")] = '\0';
                }
                std::mbstowcs(outUser, u, userBufSize - 1);
                outUser[userBufSize - 1] = L'\0';
                if (outPass && passBufSize > 0) {
                    std::mbstowcs(outPass, p, passBufSize - 1);
                    outPass[passBufSize - 1] = L'\0';
                }
            }
            fclose(fp);
        }
    }
}

void GameConfig::EncryptAndSaveCredentials(const wchar_t* user, const wchar_t* pass) {
    if (user) {
        m_encryptedUsername = user;
    } else {
        m_encryptedUsername.clear();
    }
    if (pass) {
        m_encryptedPassword = pass;
    } else {
        m_encryptedPassword.clear();
    }
    m_rememberMe = !m_encryptedUsername.empty();

    FILE* fp = fopen("credentials.dat", "w");
    if (fp) {
        char u[128] = {0};
        char p[128] = {0};
        if (user) std::wcstombs(u, user, sizeof(u) - 1);
        if (pass) std::wcstombs(p, pass, sizeof(p) - 1);
        fprintf(fp, "%s\n%s\n", u, p);
        fclose(fp);
    }
}

#endif // !__ANDROID__
