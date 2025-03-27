#include "MegaphoneService.h"
#include "common.h"
#include <iostream>
#include <string>

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
        return false;
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
BOOL MegaphoneService_UploadFile(LPCWSTR filePath, LPWSTR errorMessage, DWORD errorSize) {
    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
    BOOL result = FALSE;

    // 1. 初始化WinHTTP会话
    /*hSession = WinHttpOpen(
        L"WinHTTP Example/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );
    if (!hSession) {
        swprintf_s(errorMessage, errorSize, L"WinHttpOpen失败: %d", GetLastError());
        goto cleanup;
    }

    // 2. 解析URL
    URL_COMPONENTS urlComp = { sizeof(URL_COMPONENTS) };
    WCHAR host[256] = { 0 }, path[1024] = { 0 };
    urlComp.lpszHostName = host;
    urlComp.dwHostNameLength = ARRAYSIZE(host);
    urlComp.lpszUrlPath = path;
    urlComp.dwUrlPathLength = ARRAYSIZE(path);

    if (!WinHttpCrackUrl(url, wcslen(url), 0, &urlComp)) {
        swprintf_s(errorMessage, errorSize, L"URL解析失败: %d", GetLastError());
        goto cleanup;
    }

    // 3. 连接到服务器
    hConnect = WinHttpConnect(
        hSession,
        host,
        urlComp.nPort,
        0
    );
    if (!hConnect) {
        swprintf_s(errorMessage, errorSize, L"WinHttpConnect失败: %d", GetLastError());
        goto cleanup;
    }

    // 4. 创建HTTP请求
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
        swprintf_s(errorMessage, errorSize, L"WinHttpOpenRequest失败: %d", GetLastError());
        goto cleanup;
    }

    // 5. 读取文件内容
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        swprintf_s(errorMessage, errorSize, L"文件打开失败: %ls", filePath);
        goto cleanup;
    }
    DWORD fileSize = static_cast<DWORD>(file.tellg());
    file.seekg(0, std::ios::beg);
    char* buffer = new char[fileSize];
    file.read(buffer, fileSize);
    file.close();

    // 6. 设置请求头（示例：上传二进制文件）
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
        swprintf_s(errorMessage, errorSize, L"WinHttpSendRequest失败: %d", GetLastError());
        delete[] buffer;
        goto cleanup;
    }
    delete[] buffer;

    // 7. 接收响应（可选）
    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        swprintf_s(errorMessage, errorSize, L"WinHttpReceiveResponse失败: %d", GetLastError());
        goto cleanup;
    }

    result = TRUE;

cleanup:
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);*/
    return result;
}

// 获取音频文件列表
void MegaphoneService_GetFileList() {
    std::string url = HttpServerUrl + "/fetch-files";
}

// 删除音频文件
void MegaphoneService_DelFile() {
    std::string url = HttpServerUrl + "/del-file";
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