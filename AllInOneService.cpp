#include "AllInOneService.h"
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
static SOCKET client_main = INVALID_SOCKET;
static SOCKET client_ptz = INVALID_SOCKET;
static std::string Ip = "192.168.144.27"; // 必须使用std::string，不然C#传过来的ip后半部分会乱码
static int port_megaphone = 8519;
static int port_main = 8529;
static int port_ptz = 12345;
static bool isConnected_megaphone = false;
static bool isConnected_main = false;
static bool isConnected_ptz = false;
static std::string HttpServerUrl = "http://" + Ip + ":8222";

static uint8_t ALLINONE_SAFETY_SWITCH = 0x00; // 多合一安全开关
static uint8_t ALLINONE_LIGHT_SWITCH = 0x01; // 多合一照明开关
static uint8_t ALLINONE_LIGHT_LUMINANCE = 0x02; // 多合一灯光亮度(0-30)
static uint8_t ALLINONE_FLASH_SWITCH = 0x03; // 多合一爆闪开关
static uint8_t ALLINONE_THROWER_CONTROL = 0x04; // 多合一抛投控制
static uint8_t ALLINONE_RED_AND_BLUE_CONTROL = 0x07; // 多合一红蓝控制
static uint8_t ALLINONE_ALLOW_DETONATE = 0x31; // 多合一灭火弹充电放电
static uint8_t ALLINONE_DETONATE_HEIGHT = 0x27; // 设置引爆高度

static uint8_t ALLINONE_STATE = 0x25; // 多合一灯光和抛投状态返回

// 俯仰控制，端口：12345
static uint8_t ALLINONE_PITCH_CONTROL = 0x10; // 俯仰控制
// 每500ms上报一次，payload包含两个字节，高位在前，数值是0-900，对应0-90度
static uint8_t ALLINONE_PITCH_STATE = 0x11; // 俯仰角度上报（500ms）

static int parseIndex_main = 0;
static std::vector<uint8_t> recvData_main(128);
static std::vector<uint8_t> recvDataLast_main(128);
static std::vector<MainCallback> mainCallbacks; // 灯和抛投回调函数

static int parseIndex_ptz = 0;
static std::vector<uint8_t> recvData_ptz(128);
static std::vector<uint8_t> recvDataLast_ptz(128);
static std::vector<PtzCallback> ptzCallbacks; // 俯仰回调函数

// 清理函数
void AllInOneService_Cleanup() {
    if (client_megaphone != INVALID_SOCKET) closesocket(client_megaphone);
    if (client_main != INVALID_SOCKET) closesocket(client_main);
    if (client_ptz != INVALID_SOCKET) closesocket(client_ptz);
    CleanupWinsock();
}

// 设置IP
void AllInOneService_SetIp(const char* ip) {
    Ip = std::string(ip);
    HttpServerUrl = "http://" + Ip + ":8222";
}

// 连接喊话器
bool AllInOneService_Megaphone_Connection() {
    if (isConnected_megaphone) return true;

    client_megaphone = connection(Ip, port_megaphone);
    if (client_megaphone == INVALID_SOCKET) {
        isConnected_megaphone = false;
        return !(client_megaphone == INVALID_SOCKET);
    }

    isConnected_megaphone = true;
    AllInOneService_SetVolumn(30);

    return true;
}

// 断开喊话器连接
void AllInOneService_Megaphone_DisConnected() {
    if (!client_megaphone) return;
    closesocket(client_megaphone);
    isConnected_megaphone = false;
}

// 检查喊话器连接状态
bool AllInOneService_Megaphone_IsConnected() {
    return isConnected_megaphone && client_megaphone != INVALID_SOCKET;
}

// 喊话器发送数据
void AllInOneService_Megaphone_SendData(const char* data, int length) {
    if (!AllInOneService_Megaphone_IsConnected()) return;

    int result = send(client_megaphone, data, length, 0);
    if (result == SOCKET_ERROR) {
        // 获取错误码并处理
        int error = WSAGetLastError();
        std::cerr << "多合一喊话器消息发送失败，错误码: " << error << std::endl;
        AllInOneService_Megaphone_DisConnected();
    }
}

// 实时喊话
void AllInOneService_RealTimeShout(uint8_t* data, int length) {

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
    AllInOneService_Megaphone_SendData(
        reinterpret_cast<const char*>(combinedData.data()),
        combinedData.size()
    );
}

// 设置音量
void AllInOneService_SetVolumn(int volumn) {
    // 1. 命令前缀
    std::string setVolume = "[14]"; // 请确认此前缀与Kotlin中的SET_VOLUME常量一致

    // 2. 将音量值转换为2位的十六进制字符串（大写），不足两位时前面补零
    std::stringstream hexStream;
    hexStream << std::uppercase << std::setw(2) << std::setfill('0') << std::hex << volumn;
    std::string hexVolumn = hexStream.str();

    // 3. 拼接前缀和十六进制字符串
    std::string combinedData = setVolume + hexVolumn;

    // 4. 发送数据
    AllInOneService_Megaphone_SendData(combinedData.c_str(), combinedData.length());
}

// 播放TTS
void AllInOneService_PlayTTS(int voiceType, int loop, const char* text) {
    std::string ttsPlay = (loop == 0) ? "[31]" : "[32]";
    std::string combinedData = ttsPlay + std::to_string(voiceType) + std::string(text);
    AllInOneService_Megaphone_SendData(combinedData.c_str(), combinedData.length());
}

// 停止TTS
void AllInOneService_StopTTS() {
    std::string stopTTS = "[17]";
    AllInOneService_Megaphone_SendData(stopTTS.c_str(), stopTTS.length());
}


// 上传音频文件
bool AllInOneService_UploadFile(const wchar_t* filePath) {
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
const char* AllInOneService_GetFileList() {
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
bool AllInOneService_DelFile(const wchar_t* audioName) {
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
void AllInOneService_PlayAudio(const char* audioName, int loop) {
    std::string audioPlay = "[12]" + std::to_string(loop) + std::string(audioName);
    AllInOneService_Megaphone_SendData(audioPlay.c_str(), audioPlay.length());
}

// 停止播放音频
void AllInOneService_StopPlayAudio() {
    std::string stopAudioPlay = "[13]";
    AllInOneService_Megaphone_SendData(stopAudioPlay.c_str(), stopAudioPlay.length());
}

// 播放警报
void AllInOneService_PlayAlarm() {
    std::string alarmPlay = "[18]";
    AllInOneService_Megaphone_SendData(alarmPlay.c_str(), alarmPlay.length());
}

// 停止播放警报
void AllInOneService_StopPlayAlarm() {
    std::string stopAlarmPlay = "[19]";
    AllInOneService_Megaphone_SendData(stopAlarmPlay.c_str(), stopAlarmPlay.length());
}

// 抛投和灯消息接收缓冲区重置初始化
static void resetBuffer_main() {
    std::cerr << "清空recvData_main: " << std::endl;
    recvData_main.clear();
    std::cerr << "重置recvData_main: " << std::endl;
    recvData_main.resize(128, 0); // 初始化128字节缓冲区
}
// 灯和抛投数据处理
static bool parseByte_main(uint8_t b) {
    switch (parseIndex_main) {
        case 0: // Header检查
            if (b != 0x8D) {
                parseIndex_main = 0;
                resetBuffer_main();
                return false;
            }
            recvData_main[0] = b;
            parseIndex_main++;
            return false;

        case 1: // Length字段
            recvData_main[1] = b;
            parseIndex_main++;
            return false;

        case 2: // MsgID字段
            recvData_main[2] = b;
            parseIndex_main++;
            return false;

        default: { // 数据负载
            const int expectedLength = static_cast<int>(recvData_main[1]) + 4;
            // 动态扩容（如果需要）
            if (parseIndex_main >= recvData_main.size()) {
                recvData_main.resize(parseIndex_main + 16); // 每次扩展16字节
            }
            recvData_main[parseIndex_main] = b;
            parseIndex_main++;

            if (parseIndex_main >= expectedLength) {
                // 复制有效数据段
                recvDataLast_main.assign(
                    recvData_main.begin(),
                    recvData_main.begin() + expectedLength
                );

                parseIndex_main = 0;
                resetBuffer_main();
                return true;
            }
            return false;
        }
    }
}
// 灯和抛投数据接收
static void dataReceive_main() {
    try {
        std::cerr << AllInOneService_Main_IsConnected() << std::endl;
        while (AllInOneService_Main_IsConnected())
        {
            uint8_t recvBuffer[1024];
            const int bytesReceived = recv(client_main,
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
                if (parseByte_main(recvBuffer[i])) {
                    // 打印获取到的数据
                    printHex(recvDataLast_main);
                    for (MainCallback mainCallBack : mainCallbacks) {
                        if (recvDataLast_main[0] != 0x8D) {
                            continue;
                        }
                        if (recvDataLast_main[2] == ALLINONE_STATE) {
                            mainCallBack(recvDataLast_main.data(), recvDataLast_main.size());
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "灯和抛投消息接收错误: " << e.what() << std::endl;
        AllInOneService_Main_DisConnected();
    }
}
// 连接灯和抛投
bool AllInOneService_Main_Connection() {
    if (isConnected_main) return true;

    client_main = connection(Ip, port_main);
    if (client_main == INVALID_SOCKET) {
        isConnected_main = false;
        return !(client_main == INVALID_SOCKET);
    }

    isConnected_main = true;
    std::thread t(dataReceive_main); // 灯和抛投消息接收线程
    t.detach();

    return true;
}

// 断开灯和抛投连接
void AllInOneService_Main_DisConnected() {
    if (!client_main) return;
    closesocket(client_main);
    isConnected_main = false;
}

// 检查灯和抛投连接状态
bool AllInOneService_Main_IsConnected() {
    return isConnected_main && client_main != INVALID_SOCKET;
}

// 灯和抛投发送数据
void AllInOneService_Main_SendData(const char* data, int length) {
    if (!AllInOneService_Main_IsConnected()) return;

    int result = send(client_main, data, length, 0);
    if (result == SOCKET_ERROR) {
        // 获取错误码并处理
        int error = WSAGetLastError();
        std::cerr << "多合一灯或抛投消息发送失败，错误码: " << error << std::endl;
        AllInOneService_Main_DisConnected();
    }
}
// 安全开关
void AllInOneService_SafetySwitch(int open) {
    Msg msg;
    msg.SetMsgId(ALLINONE_SAFETY_SWITCH);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(open);
    msg.SetPayload(payload);
    AllInOneService_Main_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}
// 开灯关灯
void AllInOneService_OpenLight(int open) {
    Msg msg;
    msg.SetMsgId(ALLINONE_LIGHT_SWITCH);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(open);
    msg.SetPayload(payload);
    AllInOneService_Main_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}
// 亮度调整
void AllInOneService_LuminanceChange(int lum) {
    Msg msg;
    msg.SetMsgId(ALLINONE_LIGHT_LUMINANCE);
    std::vector<uint8_t> payload(1);
    if (lum > 30) {
        payload[0] = static_cast<uint8_t>(30);
    }
    else {
        payload[0] = static_cast<uint8_t>(lum);
    }
    msg.SetPayload(payload);
    AllInOneService_Main_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}
// 爆闪关灯
void AllInOneService_SharpFlash(int open) {
    Msg msg;
    msg.SetMsgId(ALLINONE_FLASH_SWITCH);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(open);
    msg.SetPayload(payload);
    AllInOneService_Main_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}
// 红蓝控制
void AllInOneService_RedBlueLedControl(int model) {
    Msg msg;
    msg.SetMsgId(ALLINONE_RED_AND_BLUE_CONTROL);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(model);
    msg.SetPayload(payload);
    AllInOneService_Main_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 抛投开关
void AllInOneService_ThrowerSwitch(int index, int isOpen) {
    Msg msg;
    msg.SetMsgId(ALLINONE_THROWER_CONTROL);
    std::vector<uint8_t> payload(2);
    payload[0] = static_cast<uint8_t>(index);
    payload[1] = static_cast<uint8_t>(isOpen);
    msg.SetPayload(payload);
    AllInOneService_Main_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}
// 充电放电，允许或禁止引爆
void AllInOneService_AllowDetonate(int index, int isAllow) {
    Msg msg;
    msg.SetMsgId(ALLINONE_ALLOW_DETONATE);
    std::vector<uint8_t> payload(2);
    payload[0] = static_cast<uint8_t>(index);
    payload[1] = static_cast<uint8_t>(isAllow);
    msg.SetPayload(payload);
    AllInOneService_Main_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}
// 设置引爆高度
void AllInOneService_SetDetonateHeight(int height) {
    Msg msg;
    msg.SetMsgId(ALLINONE_DETONATE_HEIGHT);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(height);
    msg.SetPayload(payload);
    AllInOneService_Main_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}
// 注册灯和抛投的回调函数
void AllInOneService_RegisterMainCallback(MainCallback callback) {
    mainCallbacks.push_back(callback);
}


// 俯仰消息接收缓冲区重置初始化
static void resetBuffer_ptz() {
    std::cerr << "清空recvData_ptz: " << std::endl;
    recvData_ptz.clear();
    std::cerr << "重置recvData_ptz: " << std::endl;
    recvData_ptz.resize(128, 0); // 初始化128字节缓冲区
}
// 俯仰数据处理
static bool parseByte_ptz(uint8_t b) {
    switch (parseIndex_ptz) {
        case 0: // Header检查
            if (b != 0x8D) {
                parseIndex_ptz = 0;
                resetBuffer_ptz();
                return false;
            }
            recvData_ptz[0] = b;
            parseIndex_ptz++;
            return false;

        case 1: // Length字段
            recvData_ptz[1] = b;
            parseIndex_ptz++;
            return false;

        case 2: // MsgID字段
            recvData_ptz[2] = b;
            parseIndex_ptz++;
            return false;

        default: { // 数据负载
            const int expectedLength = static_cast<int>(recvData_ptz[1]) + 4;
            // 动态扩容（如果需要）
            if (parseIndex_ptz >= recvData_ptz.size()) {
                recvData_ptz.resize(parseIndex_ptz + 16); // 每次扩展16字节
            }
            recvData_ptz[parseIndex_ptz] = b;
            parseIndex_ptz++;

            if (parseIndex_ptz >= expectedLength) {
                // 复制有效数据段
                recvDataLast_ptz.assign(
                    recvData_ptz.begin(),
                    recvData_ptz.begin() + expectedLength
                );

                parseIndex_ptz = 0;
                resetBuffer_ptz();
                return true;
            }
            return false;
        }
    }
}
// 俯仰数据接收
static void dataReceive_ptz() {
    try {
        std::cerr << AllInOneService_Ptz_IsConnected() << std::endl;
        while (AllInOneService_Ptz_IsConnected())
        {
            uint8_t recvBuffer[1024];
            const int bytesReceived = recv(client_ptz,
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
                if (parseByte_ptz(recvBuffer[i])) {
                    // 打印获取到的数据
                    printHex(recvDataLast_ptz);
                    for (PtzCallback ptzCallBack : ptzCallbacks) {
                        if (recvDataLast_ptz[0] != 0x8D) {
                            continue;
                        }
                        if (recvDataLast_ptz[2] == ALLINONE_PITCH_STATE) {
                            ptzCallBack(recvDataLast_ptz.data(), recvDataLast_ptz.size());
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "灯和抛投消息接收错误: " << e.what() << std::endl;
        AllInOneService_Main_DisConnected();
    }
}
// 连接俯仰电机
bool AllInOneService_Ptz_Connection() {
    if (isConnected_ptz) return true;

    client_ptz = connection(Ip, port_ptz);
    if (client_ptz == INVALID_SOCKET) {
        isConnected_ptz = false;
        return !(client_ptz == INVALID_SOCKET);
    }

    isConnected_ptz = true;
    std::thread t(dataReceive_ptz); // 俯仰消息接收线程
    t.detach();

    return true;
}

// 断开俯仰连接
void AllInOneService_Ptz_DisConnected() {
    if (!client_ptz) return;
    closesocket(client_ptz);
    isConnected_ptz = false;
}

// 检查俯仰连接状态
bool AllInOneService_Ptz_IsConnected() {
    return isConnected_ptz && client_ptz != INVALID_SOCKET;
}

// 俯仰发送数据
void AllInOneService_Ptz_SendData(const char* data, int length) {
    if (!AllInOneService_Ptz_IsConnected()) return;

    int result = send(client_ptz, data, length, 0);
    if (result == SOCKET_ERROR) {
        // 获取错误码并处理
        int error = WSAGetLastError();
        std::cerr << "多合一俯仰消息发送失败，错误码: " << error << std::endl;
        AllInOneService_Ptz_DisConnected();
    }
}

// 俯仰控制
void AllInOneService_PitchControl(int pitch) {
    Msg msg;
    msg.SetMsgId(ALLINONE_PITCH_CONTROL);
    std::vector<uint8_t> payload(2);
    payload[0] = (pitch >> 8) & 0xFF; // 高字节
    payload[1] = pitch & 0xFF;        // 低字节
    msg.SetPayload(payload);
    AllInOneService_Ptz_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 注册俯仰的回调函数
void AllInOneService_RegisterPtzCallback(PtzCallback callback) {
    ptzCallbacks.push_back(callback);
}