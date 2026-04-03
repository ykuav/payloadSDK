#include "FourInOne2Service.h"
#include "common.h"
#include <iostream>
#include <thread>
#include "http_client.h"
#include <mstcpip.h>  // 关键头文件
#include <sstream>    // 需要包含这个头文件来使用 std::stringstream
#include <iomanip>    // 需要包含这个头文件来使用 std::setw, std::setfill, std::hex, std::uppercase
#pragma comment(lib, "Ws2_32.lib")  // 链接库

// 全局变量
static SOCKET client_megaphone = INVALID_SOCKET;
static SOCKET client_light = INVALID_SOCKET;
static std::string Ip = "192.168.144.39"; // 必须使用std::string，不然C#传过来的ip后半部分会乱码
static int port_megaphone = 8519;
static int port_light = 8529;
static bool isConnected_megaphone = false;
static bool isConnected_light = false;
static std::string HttpServerUrl = "http://" + Ip + ":8222";
static std::vector<RadioCallback> radioCallbacks; // 收音回调函数

static uint8_t FOURINONE2_LIGHT_SWITCH = 0x01; // 四合一2照明开关
static uint8_t FOURINONE2_LIGHT_LUMINANCE = 0x02; // 四合一2灯光亮度(0-30)
static uint8_t FOURINONE2_FLASH_SWITCH = 0x03; // 四合一2爆闪开关
static uint8_t FOURINONE2_RED_AND_BLUE_CONTROL = 0x07; // 四合一2红蓝控制

static uint8_t FOURINONE2_STATE = 0x25; // 四合一2灯光状态返回

static int parseIndex_light = 0;
static std::vector<uint8_t> recvData_light(128);
static std::vector<uint8_t> recvDataLast_light(128);
static std::vector<LightCallback> lightCallbacks; // 灯回调函数

void FourInOne2Service_Cleanup()
{
    if (client_megaphone != INVALID_SOCKET) closesocket(client_megaphone);
    if (client_light != INVALID_SOCKET) closesocket(client_light);
    CleanupWinsock();
}

void FourInOne2Service_SetIp(const char* ip)
{
    Ip = std::string(ip);
    HttpServerUrl = "http://" + Ip + ":8222";
}

// 数据接收
static void dataReceive() {
    try {
        while (FourInOne2Service_Megaphone_IsConnected()) {
            uint8_t recvBuffer[1024];
            const int bytesReceived = recv(client_megaphone,
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
                tmp.clear();
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "四合一2喊话器消息接收错误: " << e.what() << std::endl;
        FourInOne2Service_Megaphone_DisConnected();
    }
}

bool FourInOne2Service_Megaphone_Connection()
{
    if (isConnected_megaphone) return true;

    client_megaphone = connection(Ip, port_megaphone);
    if (client_megaphone == INVALID_SOCKET) {
        isConnected_megaphone = false;
        return !(client_megaphone == INVALID_SOCKET);
    }

    isConnected_megaphone = true;
    FourInOne2Service_SetVolumn(30);

    std::thread t(dataReceive);
    t.detach();
    return true;
}

void FourInOne2Service_Megaphone_DisConnected()
{
    if (!client_megaphone) return;
    closesocket(client_megaphone);
    isConnected_megaphone = false;
}

bool FourInOne2Service_Megaphone_IsConnected()
{
    return isConnected_megaphone && client_megaphone != INVALID_SOCKET;
}

// 喊话器发送数据
void FourInOne2Service_Megaphone_SendData(const char* data, int length) {
    if (!FourInOne2Service_Megaphone_IsConnected()) return;

    int result = send(client_megaphone, data, length, 0);
    if (result == SOCKET_ERROR) {
        // 获取错误码并处理
        int error = WSAGetLastError();
        std::cerr << "四合一2喊话器消息发送失败，错误码: " << error << std::endl;
        FourInOne2Service_Megaphone_DisConnected();
    }
}

// 实时喊话
void FourInOne2Service_RealTimeShout(uint8_t* data, int length) {

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
    FourInOne2Service_Megaphone_SendData(
        reinterpret_cast<const char*>(combinedData.data()),
        combinedData.size()
    );
}

// 设置音量
void FourInOne2Service_SetVolumn(int volumn) {
    // 1. 命令前缀
    std::string setVolume = "[14]"; // 请确认此前缀与Kotlin中的SET_VOLUME常量一致

    // 2. 将音量值转换为2位的十六进制字符串（大写），不足两位时前面补零
    std::stringstream hexStream;
    hexStream << std::uppercase << std::setw(2) << std::setfill('0') << std::hex << volumn;
    std::string hexVolumn = hexStream.str();

    // 3. 拼接前缀和十六进制字符串
    std::string combinedData = setVolume + hexVolumn;

    // 4. 发送数据
    FourInOne2Service_Megaphone_SendData(combinedData.c_str(), combinedData.length());
}

// 播放TTS
void FourInOne2Service_PlayTTS(int voiceType, int loop, const char* text) {
    std::string ttsPlay = (loop == 0) ? "[31]" : "[32]";
    std::string combinedData = ttsPlay + std::to_string(voiceType) + std::string(text);
    FourInOne2Service_Megaphone_SendData(combinedData.c_str(), combinedData.length());
}

// 停止TTS
void FourInOne2Service_StopTTS() {
    std::string stopTTS = "[17]";
    FourInOne2Service_Megaphone_SendData(stopTTS.c_str(), stopTTS.length());
}


// 上传音频文件
bool FourInOne2Service_UploadFile(const wchar_t* filePath) {
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
const char* FourInOne2Service_GetFileList() {
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
bool FourInOne2Service_DelFile(const wchar_t* audioName) {
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
void FourInOne2Service_PlayAudio(const char* audioName, int loop) {
    std::string audioPlay = "[12]" + std::to_string(loop) + std::string(audioName);
    FourInOne2Service_Megaphone_SendData(audioPlay.c_str(), audioPlay.length());
}

// 停止播放音频
void FourInOne2Service_StopPlayAudio() {
    std::string stopAudioPlay = "[13]";
    FourInOne2Service_Megaphone_SendData(stopAudioPlay.c_str(), stopAudioPlay.length());
}

// 播放警报
void FourInOne2Service_PlayAlarm() {
    std::string alarmPlay = "[18]";
    FourInOne2Service_Megaphone_SendData(alarmPlay.c_str(), alarmPlay.length());
}

// 停止播放警报
void FourInOne2Service_StopPlayAlarm() {
    std::string stopAlarmPlay = "[19]";
    FourInOne2Service_Megaphone_SendData(stopAlarmPlay.c_str(), stopAlarmPlay.length());
}

// 开始收音
void FourInOne2Service_StartRadio() {
    std::string startRadio = "[40]";
    FourInOne2Service_Megaphone_SendData(startRadio.c_str(), startRadio.length());
}

// 结束收音
void FourInOne2Service_StopRadio() {
    std::string stopRadio = "[41]";
    FourInOne2Service_Megaphone_SendData(stopRadio.c_str(), stopRadio.length());
}

void FourInOne2Service_RegisterRadioCallback(RadioCallback callback) {
    radioCallbacks.push_back(callback);
}

// 灯消息接收缓冲区重置初始化
static void resetBuffer_light() {
    std::cerr << "清空recvData_light: " << std::endl;
    recvData_light.clear();
    std::cerr << "重置recvData_light: " << std::endl;
    recvData_light.resize(128, 0); // 初始化128字节缓冲区
}
// 灯数据处理
static bool parseByte_light(uint8_t b) {
    switch (parseIndex_light) {
        case 0: // Header检查
            if (b != 0x8D) {
                parseIndex_light = 0;
                resetBuffer_light();
                return false;
            }
            recvData_light[0] = b;
            parseIndex_light++;
            return false;

        case 1: // Length字段
            recvData_light[1] = b;
            parseIndex_light++;
            return false;

        case 2: // MsgID字段
            recvData_light[2] = b;
            parseIndex_light++;
            return false;

        default: { // 数据负载
            const int expectedLength = static_cast<int>(recvData_light[1]) + 4;
            // 动态扩容（如果需要）
            if (parseIndex_light >= recvData_light.size()) {
                recvData_light.resize(parseIndex_light + 16); // 每次扩展16字节
            }
            recvData_light[parseIndex_light] = b;
            parseIndex_light++;

            if (parseIndex_light >= expectedLength) {
                // 复制有效数据段
                recvDataLast_light.assign(
                    recvData_light.begin(),
                    recvData_light.begin() + expectedLength
                );

                parseIndex_light = 0;
                resetBuffer_light();
                return true;
            }
            return false;
        }
    }
}
// 灯数据接收
static void dataReceive_light() {
    try {
        std::cerr << FourInOne2Service_Light_IsConnected() << std::endl;
        while (FourInOne2Service_Light_IsConnected())
        {
            uint8_t recvBuffer[1024];
            const int bytesReceived = recv(client_light,
                reinterpret_cast<char*>(recvBuffer),
                sizeof(recvBuffer), 0);

            if (bytesReceived == SOCKET_ERROR) {
                const int error = WSAGetLastError();
                if (error == WSAEWOULDBLOCK) continue;
                throw std::runtime_error("接收失败，错误码: " + std::to_string(error));
            }

            if (bytesReceived == 0) {
                continue;
            }
            // 处理每个字节
            for (int i = 0; i < bytesReceived; ++i) {
                if (parseByte_light(recvBuffer[i])) {
                    // 打印获取到的数据
                    printHex(recvDataLast_light);
                    for (LightCallback lightCallback : lightCallbacks) {
                        if (recvDataLast_light[0] != 0x8D) {
                            continue;
                        }
                        if (recvDataLast_light[2] == FOURINONE2_STATE) {
                            lightCallback(recvDataLast_light.data(), recvDataLast_light.size());
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "灯消息接收错误: " << e.what() << std::endl;
        FourInOne2Service_Light_DisConnected();
    }
}
// 连接灯
bool FourInOne2Service_Light_Connection() {
    if (isConnected_light) return true;

    client_light = connection(Ip, port_light);
    if (client_light == INVALID_SOCKET) {
        isConnected_light = false;
        return !(client_light == INVALID_SOCKET);
    }

    isConnected_light = true;
    std::thread t(dataReceive_light); // 灯消息接收线程
    t.detach();

    return true;
}

// 断开灯连接
void FourInOne2Service_Light_DisConnected() {
    if (!client_light) return;
    closesocket(client_light);
    isConnected_light = false;
}

// 检查灯连接状态
bool FourInOne2Service_Light_IsConnected() {
    return isConnected_light && client_light != INVALID_SOCKET;
}

// 灯发送数据
void FourInOne2Service_Light_SendData(const char* data, int length) {
    if (!FourInOne2Service_Light_IsConnected()) return;

    int result = send(client_light, data, length, 0);
    if (result == SOCKET_ERROR) {
        // 获取错误码并处理
        int error = WSAGetLastError();
        std::cerr << "四合一2灯消息发送失败，错误码: " << error << std::endl;
        FourInOne2Service_Light_DisConnected();
    }
}

// 开灯关灯
void FourInOne2Service_OpenLight(int open) {
    Msg msg;
    msg.SetMsgId(FOURINONE2_LIGHT_SWITCH);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(open);
    msg.SetPayload(payload);
    FourInOne2Service_Light_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}
// 亮度调整
void FourInOne2Service_LuminanceChange(int lum) {
    Msg msg;
    msg.SetMsgId(FOURINONE2_LIGHT_LUMINANCE);
    std::vector<uint8_t> payload(1);
    if (lum > 30) {
        payload[0] = static_cast<uint8_t>(30);
    }
    else {
        payload[0] = static_cast<uint8_t>(lum);
    }
    msg.SetPayload(payload);
    FourInOne2Service_Light_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}
// 爆闪关灯
void FourInOne2Service_SharpFlash(int open) {
    Msg msg;
    msg.SetMsgId(FOURINONE2_FLASH_SWITCH);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(open);
    msg.SetPayload(payload);
    FourInOne2Service_Light_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}
// 红蓝控制
void FourInOne2Service_RedBlueLedControl(int model) {
    Msg msg;
    msg.SetMsgId(FOURINONE2_RED_AND_BLUE_CONTROL);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(model);
    msg.SetPayload(payload);
    FourInOne2Service_Light_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}
// 注册灯的回调函数
void FourInOne2Service_RegisterLightCallback(LightCallback callback) {
    lightCallbacks.push_back(callback);
}