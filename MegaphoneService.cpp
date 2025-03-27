#include "MegaphoneService.h"
#include "common.h"
#include <iostream>
#include <string>

// ȫ�ֱ���
static SOCKET client = INVALID_SOCKET;
static SOCKET servoControlClient = INVALID_SOCKET;
static std::string Ip = "192.168.144.23"; // ����ʹ��std::string����ȻC#��������ip��벿�ֻ�����
static int Port = 8519;
static bool IsConned = false;
static std::string HttpServerUrl = "http://" + Ip + ":8222";

// ������
void MegaphoneService_Cleanup() {
    if (client != INVALID_SOCKET) closesocket(client);
    if (servoControlClient != INVALID_SOCKET) closesocket(servoControlClient);
    CleanupWinsock();
}

// ����IP
void MegaphoneService_SetIp(const char* ip) {
    Ip = std::string(ip);
    HttpServerUrl = "http://" + Ip + ":8222";
}

// ����
bool MegaphoneService_Connection() {
    if (IsConned) return true;

    client = connection(Ip, Port);
    if (client == INVALID_SOCKET) {
        IsConned = false;
        return false;
    }

    IsConned = true;
    MegaphoneService_SetVolumn(50);
    return true;
}

// �Ͽ�����
void MegaphoneService_DisConnected() {
    if (!IsConned) return;
    closesocket(client);
    IsConned = false;
}

// �������״̬
bool MegaphoneService_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// ��������
void MegaphoneService_SendData(const char* data, int length) {
    if (!MegaphoneService_IsConnected()) return;
    try {
        send(client, data, length, 0);
    }
    catch (const std::exception& e) {
        std::cerr << "��������Ϣ����ʧ��: " << e.what() << std::endl;
        MegaphoneService_DisConnected();
    }
}

// ʵʱ����
void MegaphoneService_RealTimeShout(uint8_t* data, int length) {
    std::string realTimeShout = "[42]";
    std::string combinedData = realTimeShout + reinterpret_cast<const char*>(data);
    MegaphoneService_SendData(combinedData.c_str(), combinedData.length());
}

// ��������
void MegaphoneService_SetVolumn(int volumn) {
    std::string setVolume = "[14]";
    std::string combinedData = setVolume + std::to_string(volumn);
    MegaphoneService_SendData(combinedData.c_str(), combinedData.length());
}

// ����TTS
void MegaphoneService_PlayTTS(int voiceType, int loop, const char* text) {
    std::string ttsPlay = (loop == 0) ? "[31]" : "[32]";
    std::string combinedData = ttsPlay + std::to_string(voiceType) + std::string(text);
    MegaphoneService_SendData(combinedData.c_str(), combinedData.length());
}

// ֹͣTTS
void MegaphoneService_StopTTS() {
    std::string stopTTS = "[17]";
    MegaphoneService_SendData(stopTTS.c_str(), stopTTS.length());
}

// �ϴ���Ƶ�ļ�
BOOL MegaphoneService_UploadFile(LPCWSTR filePath, LPWSTR errorMessage, DWORD errorSize) {
    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
    BOOL result = FALSE;

    // 1. ��ʼ��WinHTTP�Ự
    /*hSession = WinHttpOpen(
        L"WinHTTP Example/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );
    if (!hSession) {
        swprintf_s(errorMessage, errorSize, L"WinHttpOpenʧ��: %d", GetLastError());
        goto cleanup;
    }

    // 2. ����URL
    URL_COMPONENTS urlComp = { sizeof(URL_COMPONENTS) };
    WCHAR host[256] = { 0 }, path[1024] = { 0 };
    urlComp.lpszHostName = host;
    urlComp.dwHostNameLength = ARRAYSIZE(host);
    urlComp.lpszUrlPath = path;
    urlComp.dwUrlPathLength = ARRAYSIZE(path);

    if (!WinHttpCrackUrl(url, wcslen(url), 0, &urlComp)) {
        swprintf_s(errorMessage, errorSize, L"URL����ʧ��: %d", GetLastError());
        goto cleanup;
    }

    // 3. ���ӵ�������
    hConnect = WinHttpConnect(
        hSession,
        host,
        urlComp.nPort,
        0
    );
    if (!hConnect) {
        swprintf_s(errorMessage, errorSize, L"WinHttpConnectʧ��: %d", GetLastError());
        goto cleanup;
    }

    // 4. ����HTTP����
    hRequest = WinHttpOpenRequest(
        hConnect,
        L"POST",
        path,
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0
    );
    if (!hRequest) {
        swprintf_s(errorMessage, errorSize, L"WinHttpOpenRequestʧ��: %d", GetLastError());
        goto cleanup;
    }

    // 5. ��ȡ�ļ�����
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        swprintf_s(errorMessage, errorSize, L"�ļ���ʧ��: %ls", filePath);
        goto cleanup;
    }
    DWORD fileSize = static_cast<DWORD>(file.tellg());
    file.seekg(0, std::ios::beg);
    char* buffer = new char[fileSize];
    file.read(buffer, fileSize);
    file.close();

    // 6. ��������ͷ��ʾ�����ϴ��������ļ���
    LPCWSTR headers = L"Content-Type: application/octet-stream\r\n";
    if (!WinHttpSendRequest(
        hRequest,
        headers,
        -1L,
        buffer,
        fileSize,
        fileSize,
        0
    )) {
        swprintf_s(errorMessage, errorSize, L"WinHttpSendRequestʧ��: %d", GetLastError());
        delete[] buffer;
        goto cleanup;
    }
    delete[] buffer;

    // 7. ������Ӧ����ѡ��
    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        swprintf_s(errorMessage, errorSize, L"WinHttpReceiveResponseʧ��: %d", GetLastError());
        goto cleanup;
    }

    result = TRUE;

cleanup:
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);*/
    return result;
}

// ��ȡ��Ƶ�ļ��б�
void MegaphoneService_GetFileList() {
    std::string url = HttpServerUrl + "/fetch-files";
}

// ɾ����Ƶ�ļ�
void MegaphoneService_DelFile() {
    std::string url = HttpServerUrl + "/del-file";
}

// ������Ƶ
void MegaphoneService_PlayAudio(const char* audioName, int loop) {
    std::string audioPlay = "[12]" + std::to_string(loop) + std::string(audioName);
    MegaphoneService_SendData(audioPlay.c_str(), audioPlay.length());
}

// ֹͣ������Ƶ
void MegaphoneService_StopPlayAudio() {
    std::string stopAudioPlay = "[13]";
    MegaphoneService_SendData(stopAudioPlay.c_str(), stopAudioPlay.length());
}

// ���ž���
void MegaphoneService_PlayAlarm() {
    MegaphoneService_StopPlayAlarm();
    std::string alarmPlay = "[18]";
    MegaphoneService_SendData(alarmPlay.c_str(), alarmPlay.length());
}

// ֹͣ���ž���
void MegaphoneService_StopPlayAlarm() {
    std::string stopAlarmPlay = "[19]";
    MegaphoneService_SendData(stopAlarmPlay.c_str(), stopAlarmPlay.length());
}

// ��������
void MegaphoneService_PitchControl(unsigned int pitch) {
    if (servoControlClient == INVALID_SOCKET) {
        // ���Ӻ������������ƵĶ��
        servoControlClient = connection(Ip, 12345);
        if (servoControlClient == INVALID_SOCKET) {
            return;
        }
    }

    unsigned int dc = pitch + 80u;
    unsigned int data = (dc < 80u) ? 80u : (dc > 220u) ? 220u : dc;

    unsigned char sendData[2] = { 0x8d, (unsigned char)data };
    try {
        send(servoControlClient, (const char*)sendData, 2, 0);
    }
    catch (const std::exception& ex) {
        std::cerr << "��������������ʧ��: " << ex.what() << std::endl;
    }
}