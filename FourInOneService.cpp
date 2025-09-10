#include "FourInOneService.h"
#include "common.h"
#include <iostream>
#include <thread>
#include "http_client.h"
#include <mstcpip.h>  // 关键头文件
#include <sstream>    // 需要包含这个头文件来使用 std::stringstream
#include <iomanip>    // 需要包含这个头文件来使用 std::setw, std::setfill, std::hex, std::uppercase
#pragma comment(lib, "Ws2_32.lib")  // 链接库

// 全局变量
static SOCKET client = INVALID_SOCKET;
static SOCKET servoControlClient = INVALID_SOCKET;
static std::string Ip = "192.168.144.27"; // 必须使用std::string，不然C#传过来的ip后半部分会乱码
static int Port = 8519;
static bool IsConned = false;
static std::string HttpServerUrl = "http://" + Ip + ":8222";

static uint8_t OPEN_CLOSE_LIGHT = 0x01; // 开关灯
static uint8_t LUMINANCE_CHANGE = 0x02; // 亮度
static uint8_t SHARP_FLASH = 0x03;      // 爆闪
static uint8_t FETCH_TEMPERATURE = 0x04;// 获取温度
static uint8_t RED_BLUE_FLASHES = 0x07; // 红蓝
static uint8_t SEROV_CONTROL = 0x09;	// 照明俯仰控制

static std::vector<RadioCallback> radioCallbacks; // 收音回调函数

static std::vector<OtherCallback> otherCallbacks; // 其他回调函数

// 清理函数
void FourInOneService_Cleanup() {
    if (client != INVALID_SOCKET) closesocket(client);
    if (servoControlClient != INVALID_SOCKET) closesocket(servoControlClient);
    CleanupWinsock();
}

// 设置IP
void FourInOneService_SetIp(const char* ip) {
    Ip = std::string(ip);
    HttpServerUrl = "http://" + Ip + ":8222";
}

// 数据接收
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
                throw std::runtime_error("接收失败，错误码: " + std::to_string(error));
            }
            if (bytesReceived == 0) continue;

            static std::vector<uint8_t> tmp;
            const uint8_t radioTarget[4] = { '[', '4', '0', ']' };

            for (int i = 0; i < bytesReceived; ++i) {
                if (recvBuffer[i] == '[') {
                    // 检测到新报文头，且tmp已有数据时触发回调
                    if (!tmp.empty() && tmp.size() >= 4)
                    {
                        // 收音回调
                        if (std::equal(tmp.begin(), tmp.begin() + 4, radioTarget)) {
                            for (RadioCallback cb : radioCallbacks) {
                                cb(tmp.data(), tmp.size());
                            }
                        }
                        else { // 其他回调
                            for (OtherCallback cb : otherCallbacks) {
                                cb(tmp.data(), tmp.size());
                            }
                        }
                    }
                    tmp.clear();
                }
                tmp.push_back(recvBuffer[i]);
            }

            // 处理缓冲区末尾的残留数据（可选）
            if (!tmp.empty() && tmp.size() >= 4)
            {
                // 收音回调
                if (std::equal(tmp.begin(), tmp.begin() + 4, radioTarget)) {
                    for (RadioCallback cb : radioCallbacks) {
                        cb(tmp.data(), tmp.size());
                    }
                }
                else { // 其他回调
                    for (OtherCallback cb : otherCallbacks) {
                        cb(tmp.data(), tmp.size());
                    }
                }
                tmp.clear();
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "四合一消息接收错误: " << e.what() << std::endl;
        FourInOneService_DisConnected();
    }
}

// 连接
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

    // 启用 KeepAlive
    int keepAlive = 1;
    setsockopt(client, SOL_SOCKET, SO_KEEPALIVE, (char*)&keepAlive, sizeof(keepAlive));

    // 设置参数：5秒无活动开始探测
    DWORD bytesReturned;
    tcp_keepalive keepaliveOpts{ 1, 5000, 500 }; // 开启/5秒空闲/0.5秒间隔
    WSAIoctl(client, SIO_KEEPALIVE_VALS, &keepaliveOpts, sizeof(keepaliveOpts), NULL, 0, &bytesReturned, NULL, NULL);
    return true;
}

// 断开连接
void FourInOneService_DisConnected() {
    if (!IsConned) return;
    closesocket(client);
    IsConned = false;
}

// 检查连接状态
bool FourInOneService_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// 发送数据
void FourInOneService_SendData(const char* data, int length) {
    if (!FourInOneService_IsConnected()) return;

    int result = send(client, data, length, 0);
    if (result == SOCKET_ERROR) {
        // 获取错误码并处理
        int error = WSAGetLastError();
        std::cerr << "四合一消息发送失败，错误码: " << error << std::endl;
        FourInOneService_DisConnected();
    }
}

// 实时喊话
void FourInOneService_RealTimeShout(uint8_t* data, int length) {

    // [1] 将字符串前缀"[42]"转换为字节序列
    const std::string realTimeShout = "[42]";

    // [2] 直接构造二进制数据块，包含前缀和原始数据（含0x00）
    std::vector<uint8_t> combinedData;
    combinedData.reserve(realTimeShout.size() + length);

    // 添加前缀字节
    combinedData.insert(combinedData.end(),
        reinterpret_cast<const uint8_t*>(realTimeShout.data()),
        reinterpret_cast<const uint8_t*>(realTimeShout.data() + realTimeShout.size()));

    // 添加原始数据（含0x00）
    combinedData.insert(combinedData.end(), data, data + length);

    // [3] 发送完整二进制数据（无截断）
    FourInOneService_SendData(
        reinterpret_cast<const char*>(combinedData.data()),
        combinedData.size()
    );
}

// 设置音量
void FourInOneService_SetVolumn(int volumn) {
    // 1. 命令前缀
    std::string setVolume = "[14]"; // 请确认此前缀与Kotlin中的SET_VOLUME常量一致

    // 2. 将音量值转换为2位的十六进制字符串（大写），不足两位时前面补零
    std::stringstream hexStream;
    hexStream << std::uppercase << std::setw(2) << std::setfill('0') << std::hex << volumn;
    std::string hexVolumn = hexStream.str();

    // 3. 拼接前缀和十六进制字符串
    std::string combinedData = setVolume + hexVolumn;

    // 4. 发送数据
    FourInOneService_SendData(combinedData.c_str(), combinedData.length());
}

// 播放TTS
void FourInOneService_PlayTTS(int voiceType, int loop, const char* text) {
    std::string ttsPlay = (loop == 0) ? "[31]" : "[32]";
    std::string combinedData = ttsPlay + std::to_string(voiceType) + std::string(text);
    FourInOneService_SendData(combinedData.c_str(), combinedData.length());
}

// 停止TTS
void FourInOneService_StopTTS() {
    std::string stopTTS = "[17]";
    FourInOneService_SendData(stopTTS.c_str(), stopTTS.length());
}


// 上传音频文件
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

// 获取音频文件列表
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
        char* result = new char[responseBody.size() + 1]; // 分配内存
        strcpy_s(result, responseBody.size() + 1, responseBody.c_str());
        return result; // 返回 C 风格字符串
    }
    else {
        std::wcout << L"GET request failed: " << client.GetLastErrorMessage() << std::endl;
        std::string data = WideToUtf8(client.GetLastErrorMessage());
        std::string reslutStr = "{\"code\": -1, \"data\": \"" + data + "\"}";
        char* result = new char[reslutStr.size() + 1];

        // 复制数据
        strcpy_s(result, reslutStr.size() + 1, reslutStr.c_str());
        return result;
    }
}

// 删除音频文件
bool FourInOneService_DelFile(const wchar_t* audioName) {
    std::string url = HttpServerUrl + "/del-file";
    std::wstring fileName(audioName);

    HttpClient client;
    if (!client.Initialize(L"My WinHttp Client/1.0")) {
        std::wcout << L"Failed to initialize HTTP client: " << client.GetLastErrorMessage() << std::endl;
        return FALSE;
    }

    // 构建表单键值对
    std::vector<std::pair<std::wstring, std::wstring>> formData = {
        { L"filename", fileName }
    };
    // 编码为 x-www-form-urlencoded 格式
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
    // 调用 Post 方法
    bool success = client.Post(
        Utf8ToWide(url),                        // URL
        encodedBody,                            // 编码后的表单数据
        L"application/x-www-form-urlencoded",   // Content-Type
        response,                               // 响应内容
        statusCode                              // HTTP 状态码
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

// 播放音频
void FourInOneService_PlayAudio(const char* audioName, int loop) {
    std::string audioPlay = "[12]" + std::to_string(loop) + std::string(audioName);
    FourInOneService_SendData(audioPlay.c_str(), audioPlay.length());
}

// 停止播放音频
void FourInOneService_StopPlayAudio() {
    std::string stopAudioPlay = "[13]";
    FourInOneService_SendData(stopAudioPlay.c_str(), stopAudioPlay.length());
}

// 播放警报
void FourInOneService_PlayAlarm() {
    FourInOneService_StopPlayAlarm();
    std::string alarmPlay = "[18]";
    FourInOneService_SendData(alarmPlay.c_str(), alarmPlay.length());
}

// 停止播放警报
void FourInOneService_StopPlayAlarm() {
    std::string stopAlarmPlay = "[19]";
    FourInOneService_SendData(stopAlarmPlay.c_str(), stopAlarmPlay.length());
}

// 俯仰控制
void FourInOneService_PitchControl(unsigned int pitch) {
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

// 开始收音
void FourInOneService_StartRadio() {
    std::string startRadio = "[40]";
    FourInOneService_SendData(startRadio.c_str(), startRadio.length());
}

// 结束收音
void FourInOneService_StopRadio() {
    std::string stopRadio = "[41]";
    FourInOneService_SendData(stopRadio.c_str(), stopRadio.length());
}

// 1 开灯，0 关灯
void FourInOneService_OpenLight(int open) {
    Msg msg;
    msg.SetMsgId(OPEN_CLOSE_LIGHT);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(open);
    msg.SetPayload(payload);
    FourInOneService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
    // 如果是关灯，也要把爆闪模式关掉，防止开着爆闪时关灯导致下次开灯还会是爆闪模式
    if (open == 0) {
        Sleep(100);
        FourInOneService_SharpFlash(0);
    }
}

// 亮度调整
void FourInOneService_LuminanceChange(int lum) {
    Msg msg;
    msg.SetMsgId(LUMINANCE_CHANGE);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(lum);
    msg.SetPayload(payload);
    FourInOneService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 1 开爆闪，0 关爆闪
void FourInOneService_SharpFlash(int open) {
    Msg msg;
    msg.SetMsgId(SHARP_FLASH);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(open);
    msg.SetPayload(payload);
    FourInOneService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 俯仰控制, 0-100，实际传输的时候是100 - 200
void FourInOneService_ControlServo(int pitch) {
    Msg msg;
    msg.SetMsgId(SEROV_CONTROL);
    std::vector<uint8_t> payload(2);
    payload[0] = 0xFF;
    payload[1] = static_cast<uint8_t>(pitch + 100);
    msg.SetPayload(payload);
    FourInOneService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 红蓝控制，model 0 关，1-16都是开，是16种红蓝模式
void FourInOneService_RedBlueLedControl(int model) {
    Msg msg;
    msg.SetMsgId(RED_BLUE_FLASHES);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(model);
    msg.SetPayload(payload);
    FourInOneService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 注册收音回调函数
void FourInOneService_RegisterCallback(RadioCallback callback)
{
    radioCallbacks.push_back(callback);
}

// 注册收音以外的回调函数
void FourInOneService_RegisterOtherCallback(OtherCallback callback)
{
    otherCallbacks.push_back(callback);
}
