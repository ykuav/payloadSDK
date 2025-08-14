#include "FourInOneService.h"
#include "common.h"
#include <iostream>
#include <thread>
#include "http_client.h"
#include <mstcpip.h>  // �ؼ�ͷ�ļ�
#pragma comment(lib, "Ws2_32.lib")  // ���ӿ�

// ȫ�ֱ���
static SOCKET client = INVALID_SOCKET;
static SOCKET servoControlClient = INVALID_SOCKET;
static std::string Ip = "192.168.144.27"; // ����ʹ��std::string����ȻC#��������ip��벿�ֻ�����
static int Port = 8519;
static bool IsConned = false;
static std::string HttpServerUrl = "http://" + Ip + ":8222";

static uint8_t OPEN_CLOSE_LIGHT = 0x01; // ���ص�
static uint8_t LUMINANCE_CHANGE = 0x02; // ����
static uint8_t SHARP_FLASH = 0x03;      // ����
static uint8_t FETCH_TEMPERATURE = 0x04;// ��ȡ�¶�
static uint8_t RED_BLUE_FLASHES = 0x07; // ����
static uint8_t SEROV_CONTROL = 0x09;	// ������������

static std::vector<RadioCallback> radioCallbacks; // �����ص�����

// ������
void FourInOneService_Cleanup() {
    if (client != INVALID_SOCKET) closesocket(client);
    if (servoControlClient != INVALID_SOCKET) closesocket(servoControlClient);
    CleanupWinsock();
}

// ����IP
void FourInOneService_SetIp(const char* ip) {
    Ip = std::string(ip);
    HttpServerUrl = "http://" + Ip + ":8222";
}

// ���ݽ���
static void dataReceive() {
    try {
        while (FourInOneService_IsConnected()) {
            uint8_t recvBuffer[1024];
            const int bytesReceived = recv(client,
                reinterpret_cast<char*>(recvBuffer),
                sizeof(recvBuffer), 0);

            if (bytesReceived == SOCKET_ERROR) {
                const int error = WSAGetLastError();
                if (error == WSAEWOULDBLOCK) continue;
                throw std::runtime_error("����ʧ�ܣ�������: " + std::to_string(error));
            }
            if (bytesReceived == 0) continue;

            static std::vector<uint8_t> tmp;
            const uint8_t radioTarget[4] = { '[', '4', '0', ']' };

            for (int i = 0; i < bytesReceived; ++i) {
                if (recvBuffer[i] == '[') {
                    // ��⵽�±���ͷ����tmp��������ʱ�����ص�
                    if (!tmp.empty() && tmp.size() >= 4 &&
                        std::equal(tmp.begin(), tmp.begin() + 4, radioTarget))
                    {
                        for (RadioCallback cb : radioCallbacks) {
                            cb(tmp.data(), tmp.size());
                        }
                    }
                    tmp.clear();
                }
                tmp.push_back(recvBuffer[i]);
            }

            // ��������ĩβ�Ĳ������ݣ���ѡ��
            if (!tmp.empty() && tmp.size() >= 4 &&
                std::equal(tmp.begin(), tmp.begin() + 4, radioTarget))
            {
                for (RadioCallback cb : radioCallbacks) {
                    cb(tmp.data(), tmp.size());
                }
                tmp.clear();
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "�ĺ�һ��Ϣ���մ���: " << e.what() << std::endl;
        FourInOneService_DisConnected();
    }
}

// ����
bool FourInOneService_Connection() {
    if (IsConned) return true;

    client = connection(Ip, Port);
    if (client == INVALID_SOCKET) {
        IsConned = false;
        return !(client == INVALID_SOCKET);
    }

    IsConned = true;
    FourInOneService_SetVolumn(30);
    std::thread t(dataReceive);
    t.detach();

    // ���� KeepAlive
    int keepAlive = 1;
    setsockopt(client, SOL_SOCKET, SO_KEEPALIVE, (char*)&keepAlive, sizeof(keepAlive));

    // ���ò�����5���޻��ʼ̽��
    DWORD bytesReturned;
    tcp_keepalive keepaliveOpts{ 1, 5000, 500 }; // ����/5�����/0.5����
    WSAIoctl(client, SIO_KEEPALIVE_VALS, &keepaliveOpts, sizeof(keepaliveOpts), NULL, 0, &bytesReturned, NULL, NULL);
    return true;
}

// �Ͽ�����
void FourInOneService_DisConnected() {
    if (!IsConned) return;
    closesocket(client);
    IsConned = false;
}

// �������״̬
bool FourInOneService_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// ��������
void FourInOneService_SendData(const char* data, int length) {
    if (!FourInOneService_IsConnected()) return;

    int result = send(client, data, length, 0);
    if (result == SOCKET_ERROR) {
        // ��ȡ�����벢����
        int error = WSAGetLastError();
        std::cerr << "�ĺ�һ��Ϣ����ʧ�ܣ�������: " << error << std::endl;
        FourInOneService_DisConnected();
    }
}

// ʵʱ����
void FourInOneService_RealTimeShout(uint8_t* data, int length) {

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
    FourInOneService_SendData(
        reinterpret_cast<const char*>(combinedData.data()),
        combinedData.size()
    );
}

// ��������
void FourInOneService_SetVolumn(int volumn) {
    std::string setVolume = "[14]";
    std::string combinedData = setVolume + std::to_string(volumn);
    FourInOneService_SendData(combinedData.c_str(), combinedData.length());
}

// ����TTS
void FourInOneService_PlayTTS(int voiceType, int loop, const char* text) {
    std::string ttsPlay = (loop == 0) ? "[31]" : "[32]";
    std::string combinedData = ttsPlay + std::to_string(voiceType) + std::string(text);
    FourInOneService_SendData(combinedData.c_str(), combinedData.length());
}

// ֹͣTTS
void FourInOneService_StopTTS() {
    std::string stopTTS = "[17]";
    FourInOneService_SendData(stopTTS.c_str(), stopTTS.length());
}


// �ϴ���Ƶ�ļ�
bool FourInOneService_UploadFile(const wchar_t* filePath) {
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
const char* FourInOneService_GetFileList() {
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
bool FourInOneService_DelFile(const wchar_t* audioName) {
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
void FourInOneService_PlayAudio(const char* audioName, int loop) {
    std::string audioPlay = "[12]" + std::to_string(loop) + std::string(audioName);
    FourInOneService_SendData(audioPlay.c_str(), audioPlay.length());
}

// ֹͣ������Ƶ
void FourInOneService_StopPlayAudio() {
    std::string stopAudioPlay = "[13]";
    FourInOneService_SendData(stopAudioPlay.c_str(), stopAudioPlay.length());
}

// ���ž���
void FourInOneService_PlayAlarm() {
    FourInOneService_StopPlayAlarm();
    std::string alarmPlay = "[18]";
    FourInOneService_SendData(alarmPlay.c_str(), alarmPlay.length());
}

// ֹͣ���ž���
void FourInOneService_StopPlayAlarm() {
    std::string stopAlarmPlay = "[19]";
    FourInOneService_SendData(stopAlarmPlay.c_str(), stopAlarmPlay.length());
}

// ��������
void FourInOneService_PitchControl(unsigned int pitch) {
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

// ��ʼ����
void FourInOneService_StartRadio() {
    std::string startRadio = "[40]";
    FourInOneService_SendData(startRadio.c_str(), startRadio.length());
}

// ��������
void FourInOneService_StopRadio() {
    std::string stopRadio = "[41]";
    FourInOneService_SendData(stopRadio.c_str(), stopRadio.length());
}

// 1 ���ƣ�0 �ص�
void FourInOneService_OpenLight(int open) {
    Msg msg;
    msg.SetMsgId(OPEN_CLOSE_LIGHT);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(open);
    msg.SetPayload(payload);
    FourInOneService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
    // ����ǹصƣ�ҲҪ�ѱ���ģʽ�ص�����ֹ���ű���ʱ�صƵ����´ο��ƻ����Ǳ���ģʽ
    if (open == 0) {
        Sleep(100);
        FourInOneService_SharpFlash(0);
    }
}

// ���ȵ���
void FourInOneService_LuminanceChange(int lum) {
    Msg msg;
    msg.SetMsgId(LUMINANCE_CHANGE);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(lum);
    msg.SetPayload(payload);
    FourInOneService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 1 ��������0 �ر���
void FourInOneService_SharpFlash(int open) {
    Msg msg;
    msg.SetMsgId(SHARP_FLASH);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(open);
    msg.SetPayload(payload);
    FourInOneService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// ��������, 0-100��ʵ�ʴ����ʱ����100 - 200
void FourInOneService_ControlServo(int pitch) {
    Msg msg;
    msg.SetMsgId(SEROV_CONTROL);
    std::vector<uint8_t> payload(2);
    payload[0] = 0xFF;
    payload[1] = static_cast<uint8_t>(pitch + 100);
    msg.SetPayload(payload);
    FourInOneService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// �������ƣ�model 0 �أ�1-16���ǿ�����16�ֺ���ģʽ
void FourInOneService_RedBlueLedControl(int model) {
    Msg msg;
    msg.SetMsgId(RED_BLUE_FLASHES);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(model);
    msg.SetPayload(payload);
    FourInOneService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// ע�������ص�����
void FourInOneService_RegisterCallback(RadioCallback callback)
{
    radioCallbacks.push_back(callback);
}