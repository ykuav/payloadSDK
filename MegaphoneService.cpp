#include "MegaphoneService.h"
#include "http_client.h"
#include "common.h"
#include <iostream>
#include <string>
#include <map>

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
        return !(client == INVALID_SOCKET);
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
BOOL MegaphoneService_UploadFile() {
    std::string url = HttpServerUrl + "/upload-file";

    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
    BOOL result = FALSE;
    /******************************* POST����ʾ�� ******************************/
    HttpClient client;
    if (!client.Initialize(L"My WinHttp Client/1.0")) {
        std::wcout << L"Failed to initialize HTTP client: " << client.GetLastErrorMessage() << std::endl;
        return FALSE;
    }

    // Set custom headers (optional)
    std::map<std::wstring, std::wstring> headers;
    headers[L"Accept"] = L"application/json";
    headers[L"User-Agent"] = L"My Custom User Agent";
    client.SetHeaders(headers);
    std::string requestBody = "{\"name\":\"John\",\"age\":30}";
    std::string responseBody;
    DWORD statusCode = 0;

    std::wcout << L"\nPerforming POST request..." << std::endl;

    if (client.Post(Utf8ToWide(url), requestBody, L"multipart/form-data", responseBody, statusCode)) {
        std::cout << "Status code: " << statusCode << std::endl;
        std::cout << "Response body: " << responseBody << std::endl;
    }
    else {
        std::wcout << L"POST request failed: " << client.GetLastErrorMessage() << std::endl;
    }
    return result;
}

// ��ȡ��Ƶ�ļ��б�
void MegaphoneService_GetFileList() {
    std::string url = HttpServerUrl + "/fetch-files";

    HttpClient client;
    if (!client.Initialize(L"My WinHttp Client/1.0")) {
        std::wcout << L"Failed to initialize HTTP client: " << client.GetLastErrorMessage() << std::endl;
        return;
    }

    // Set custom headers (optional)
    std::map<std::wstring, std::wstring> headers;
    headers[L"Accept"] = L"application/json";
    headers[L"User-Agent"] = L"My Custom User Agent";
    client.SetHeaders(headers);

    std::string responseBody;
    DWORD statusCode = 0;

    std::wcout << L"Performing GET request..." << std::endl;

    if (client.Get(Utf8ToWide(url), responseBody, statusCode)) {
        std::cout << "Status code: " << statusCode << std::endl;
        std::cout << "Response body: " << responseBody << std::endl;
    }
    else {
        std::wcout << L"GET request failed: " << client.GetLastErrorMessage() << std::endl;
    }
}

// ɾ����Ƶ�ļ�
BOOL MegaphoneService_DelFile(std::string fileName) {
    std::string url = HttpServerUrl + "/del-file";

    HttpClient client;
    if (!client.Initialize(L"My WinHttp Client/1.0")) {
        std::wcout << L"Failed to initialize HTTP client: " << client.GetLastErrorMessage() << std::endl;
        return FALSE;
    }

    // Set custom headers (optional)
    std::map<std::wstring, std::wstring> headers;
    headers[L"Accept"] = L"application/json";
    headers[L"User-Agent"] = L"My Custom User Agent";
    client.SetHeaders(headers);
    std::string requestBody = "{\"filename\":"+ fileName +"}";
    std::string responseBody;
    DWORD statusCode = 0;

    std::wcout << L"\nPerforming POST request..." << std::endl;

    if (client.Post(Utf8ToWide(url), requestBody, L"application/json", responseBody, statusCode)) {
        std::cout << "Status code: " << statusCode << std::endl;
        std::cout << "Response body: " << responseBody << std::endl;
    }
    else {
        std::wcout << L"POST request failed: " << client.GetLastErrorMessage() << std::endl;
    }
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