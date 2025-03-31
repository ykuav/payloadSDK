#include "MegaphoneService.h"
#include "http_client.h"
#include "common.h"
#include <iostream>
#include <string>
#include <map>

// 全局变量
static SOCKET client = INVALID_SOCKET;
static SOCKET servoControlClient = INVALID_SOCKET;
static std::string Ip = "192.168.144.23"; // 必须使用std::string，不然C#传过来的ip后半部分会乱码
static int Port = 8519;
static bool IsConned = false;
static std::string HttpServerUrl = "http://" + Ip + ":8222";

// 清理函数
void MegaphoneService_Cleanup() {
    if (client != INVALID_SOCKET) closesocket(client);
    if (servoControlClient != INVALID_SOCKET) closesocket(servoControlClient);
    CleanupWinsock();
}

// 设置IP
void MegaphoneService_SetIp(const char* ip) {
    Ip = std::string(ip);
    HttpServerUrl = "http://" + Ip + ":8222";
}

// 连接
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

// 断开连接
void MegaphoneService_DisConnected() {
    if (!IsConned) return;
    closesocket(client);
    IsConned = false;
}

// 检查连接状态
bool MegaphoneService_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// 发送数据
void MegaphoneService_SendData(const char* data, int length) {
    if (!MegaphoneService_IsConnected()) return;
    try {
        send(client, data, length, 0);
    }
    catch (const std::exception& e) {
        std::cerr << "喊话器消息发送失败: " << e.what() << std::endl;
        MegaphoneService_DisConnected();
    }
}

// 实时喊话
void MegaphoneService_RealTimeShout(uint8_t* data, int length) {
    std::string realTimeShout = "[42]";
    std::string combinedData = realTimeShout + reinterpret_cast<const char*>(data);
    MegaphoneService_SendData(combinedData.c_str(), combinedData.length());
}

// 设置音量
void MegaphoneService_SetVolumn(int volumn) {
    std::string setVolume = "[14]";
    std::string combinedData = setVolume + std::to_string(volumn);
    MegaphoneService_SendData(combinedData.c_str(), combinedData.length());
}

// 播放TTS
void MegaphoneService_PlayTTS(int voiceType, int loop, const char* text) {
    std::string ttsPlay = (loop == 0) ? "[31]" : "[32]";
    std::string combinedData = ttsPlay + std::to_string(voiceType) + std::string(text);
    MegaphoneService_SendData(combinedData.c_str(), combinedData.length());
}

// 停止TTS
void MegaphoneService_StopTTS() {
    std::string stopTTS = "[17]";
    MegaphoneService_SendData(stopTTS.c_str(), stopTTS.length());
}

// 上传音频文件
BOOL MegaphoneService_UploadFile() {
    std::string url = HttpServerUrl + "/upload-file";

    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
    BOOL result = FALSE;
    /******************************* POST请求示例 ******************************/
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

// 获取音频文件列表
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

// 删除音频文件
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

// 播放音频
void MegaphoneService_PlayAudio(const char* audioName, int loop) {
    std::string audioPlay = "[12]" + std::to_string(loop) + std::string(audioName);
    MegaphoneService_SendData(audioPlay.c_str(), audioPlay.length());
}

// 停止播放音频
void MegaphoneService_StopPlayAudio() {
    std::string stopAudioPlay = "[13]";
    MegaphoneService_SendData(stopAudioPlay.c_str(), stopAudioPlay.length());
}

// 播放警报
void MegaphoneService_PlayAlarm() {
    MegaphoneService_StopPlayAlarm();
    std::string alarmPlay = "[18]";
    MegaphoneService_SendData(alarmPlay.c_str(), alarmPlay.length());
}

// 停止播放警报
void MegaphoneService_StopPlayAlarm() {
    std::string stopAlarmPlay = "[19]";
    MegaphoneService_SendData(stopAlarmPlay.c_str(), stopAlarmPlay.length());
}

// 俯仰控制
void MegaphoneService_PitchControl(unsigned int pitch) {
    if (servoControlClient == INVALID_SOCKET) {
        // 连接喊话器俯仰控制的舵机
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
        std::cerr << "喊话器俯仰控制失败: " << ex.what() << std::endl;
    }
}