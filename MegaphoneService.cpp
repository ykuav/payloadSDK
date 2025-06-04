#include "MegaphoneService.h"
#include "http_client.h"
#include "common.h"
#include <iostream>
#include <string>
#include <map>
#include <mstcpip.h>  // �ؼ�ͷ�ļ�
#include <thread>
#pragma comment(lib, "Ws2_32.lib")  // ���ӿ�

// ȫ�ֱ���
static SOCKET client = INVALID_SOCKET;
static SOCKET servoControlClient = INVALID_SOCKET;
static std::string Ip = "192.168.144.23"; // ����ʹ��std::string����ȻC#��������ip��벿�ֻ�����
static int Port = 8519;
static bool IsConned = false;
static std::string HttpServerUrl = "http://" + Ip + ":8222";
static WSAEVENT g_networkEvent = WSACreateEvent();

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

// �Ͽ�����
void MegaphoneService_DisConnected() {
    if (!IsConned) return;
    closesocket(client);
    IsConned = false;
}

// �����̼߳����¼�����û�ж�ʱ����recv��send�����������ʹ�ã��Ա㼰ʱ��ȡ��KeepAlive״̬
static void MonitorConnection() {
    while (IsConned) {
        DWORD ret = WSAWaitForMultipleEvents(1, &g_networkEvent, FALSE, 1000, FALSE);
        if (ret == WSA_WAIT_EVENT_0) {
            WSANETWORKEVENTS events;
            WSAEnumNetworkEvents(client, g_networkEvent, &events);
            if (events.lNetworkEvents & FD_CLOSE) {
                std::cerr << "���ӱ�Զ�˹رգ�KeepAlive ʧЧ��" << std::endl;
                MegaphoneService_DisConnected();
                break;
            }
        }
    }
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
    MegaphoneService_SetVolumn(30);

    // ���� KeepAlive
    int keepAlive = 1;
    setsockopt(client, SOL_SOCKET, SO_KEEPALIVE, (char*)&keepAlive, sizeof(keepAlive));

    // ���ò�����5���޻��ʼ̽��
    DWORD bytesReturned;
    tcp_keepalive keepaliveOpts{ 1, 5000, 500 }; // ����/5�����/0.5����
    WSAIoctl(client, SIO_KEEPALIVE_VALS, &keepaliveOpts, sizeof(keepaliveOpts), NULL, 0, &bytesReturned, NULL, NULL);

    // ע�������¼�����
    if (WSAEventSelect(client, g_networkEvent, FD_CLOSE) == SOCKET_ERROR) {
        std::cerr << "�¼�ע��ʧ��: " << WSAGetLastError() << std::endl;
        return false;
    }
    std::thread monitor(MonitorConnection);
    monitor.detach();
    return true;
}

// �������״̬
bool MegaphoneService_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// ��������
void MegaphoneService_SendData(const char* data, int length) {
    if (!MegaphoneService_IsConnected()) return;

    int result = send(client, data, length, 0);
    if (result == SOCKET_ERROR) {
        // ��ȡ�����벢����
        int error = WSAGetLastError();
        std::cerr << "��������Ϣ����ʧ�ܣ�������: " << error << std::endl;
        MegaphoneService_DisConnected();
    }
}

// ʵʱ����
void MegaphoneService_RealTimeShout(uint8_t* data, int length) {
    // [1] ���ַ���ǰ׺"[42]"ת��Ϊ�ֽ�����
    const std::string realTimeShout = "[42]";

    // [2] ֱ�ӹ�����������ݿ飬����ǰ׺��ԭʼ���ݣ���0x00��
    std::vector<uint8_t> combinedData;
    combinedData.reserve(realTimeShout.size() + length);

    // ���ǰ׺�ֽ�
    combinedData.insert(combinedData.end(),
        reinterpret_cast<const uint8_t*>(realTimeShout.data()),
        reinterpret_cast<const uint8_t*>(realTimeShout.data() + realTimeShout.size()));

    // ���ԭʼ���ݣ���0x00��
    combinedData.insert(combinedData.end(), data, data + length);

    // [3] �����������������ݣ��޽ضϣ�
    MegaphoneService_SendData(
        reinterpret_cast<const char*>(combinedData.data()),
        combinedData.size()
    );
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
bool MegaphoneService_UploadFile(const wchar_t* filePath) {
    std::string url = HttpServerUrl + "/upload-file";
    std::wstring filePathStr(filePath);
    bool result = UploadFile(Utf8ToWide(url), filePathStr);
    if (result) {
        std::wcout << L"File uploaded successfully!" << std::endl;
    }
    else {
        std::wcout << L"Upload failed!" << std::endl;
    }
    return result;
}

// ��ȡ��Ƶ�ļ��б�
const char* MegaphoneService_GetFileList() {
    std::string url = HttpServerUrl + "/fetch-files";

    HttpClient client;
    if (!client.Initialize(L"My WinHttp Client/1.0")) {
        std::wcout << L"Failed to initialize HTTP client: " << client.GetLastErrorMessage() << std::endl;
        return "{\"code\": -1, \"data\": \"Failed to initialize HTTP client\"}";
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
        char* result = new char[responseBody.size() + 1]; // �����ڴ�
        strcpy_s(result, responseBody.size() + 1, responseBody.c_str());
        return result; // ���� C ����ַ���
    }
    else {
        std::wcout << L"GET request failed: " << client.GetLastErrorMessage() << std::endl;
        std::string data = WideToUtf8(client.GetLastErrorMessage());
        std::string reslutStr = "{\"code\": -1, \"data\": \"" + data + "\"}";
        char* result = new char[reslutStr.size() + 1];

        // ��������
        strcpy_s(result, reslutStr.size() + 1, reslutStr.c_str());
        return result;
    }
}

// ɾ����Ƶ�ļ�
bool MegaphoneService_DelFile(const wchar_t* audioName) {
    std::string url = HttpServerUrl + "/del-file";
    std::wstring fileName(audioName);

    HttpClient client;
    if (!client.Initialize(L"My WinHttp Client/1.0")) {
        std::wcout << L"Failed to initialize HTTP client: " << client.GetLastErrorMessage() << std::endl;
        return FALSE;
    }

    // ��������ֵ��
    std::vector<std::pair<std::wstring, std::wstring>> formData = {
        { L"filename", fileName }
    };
    // ����Ϊ x-www-form-urlencoded ��ʽ
    std::string encodedBody;
    for (const auto& pair : formData) {

        std::string utf8_key = WideToUtf8(pair.first);
        std::string utf8_value = WideToUtf8(pair.second);

        std::string encoded_key = UrlEncode(utf8_key);
        std::string encoded_value = UrlEncode(utf8_value);

        if (!encodedBody.empty()) encodedBody += "&";
        encodedBody += encoded_key + "=" + encoded_value;
    }
    std::string response;
    DWORD statusCode = 0;
    // ���� Post ����
    bool success = client.Post(
        Utf8ToWide(url),                        // URL
        encodedBody,                            // �����ı�����
        L"application/x-www-form-urlencoded",   // Content-Type
        response,                               // ��Ӧ����
        statusCode                              // HTTP ״̬��
    );

    std::wcout << L"\nPerforming POST request..." << std::endl;

    if (success) {
        std::cout << "Status code: " << statusCode << std::endl;
        std::cout << "Response body: " << response << std::endl;
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