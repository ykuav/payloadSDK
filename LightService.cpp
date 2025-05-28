#include "LightService.h"
#include "common.h"
#include <string>
#include <iostream>
#include <thread>
#include <mstcpip.h>  // 关键头文件
#pragma comment(lib, "Ws2_32.lib")  // 链接库

static int parseIndex = 0;
static std::vector<uint8_t> recvData(128);
static std::vector<uint8_t> recvDataLast(128);

// 全局变量
static SOCKET client = INVALID_SOCKET;
static std::string Ip = "192.168.144.24"; // 必须使用std::string，不然C#传过来的ip后半部分会乱码
static int Port = 8519;
static bool IsConned = false;

static uint8_t OPEN_CLOSE_LIGHT = 0x01; // 开关灯
static uint8_t LUMINANCE_CHANGE = 0x02; // 亮度
static uint8_t SHARP_FLASH = 0x03;      // 爆闪
static uint8_t FETCH_TEMPERATURE = 0x04;// 获取温度
static uint8_t TRIPOD_HEAD = 0x06;      // 云台控制

static std::vector<TemperatureCallback> temperatureCallbacks;

// 清理函数
void LightService_Cleanup() {
    if (client != INVALID_SOCKET) closesocket(client);
    CleanupWinsock();
}

// 设置IP
void LightService_SetIp(const char* ip) {
    Ip = std::string(ip);
}

static void resetBuffer() {
    std::cerr << "清空recvData: " << std::endl;
    recvData.clear();
    std::cerr << "重置recvData: " << std::endl;
    recvData.resize(128, 0); // 初始化128字节缓冲区
}

static bool parseByte(uint8_t b) {
    switch (parseIndex) {
        case 0: // Header检查
            if (b != 0x8D) {
                parseIndex = 0;
                resetBuffer();
                return false;
            }
            recvData[0] = b;
            parseIndex++;
            return false;

        case 1: // Length字段
            recvData[1] = b;
            parseIndex++;
            return false;

        case 2: // MsgID字段
            recvData[2] = b;
            parseIndex++;
            return false;

        default: { // 数据负载
            const int expectedLength = static_cast<int>(recvData[1]) + 4;
            // 动态扩容（如果需要）
            if (parseIndex >= recvData.size()) {
                recvData.resize(parseIndex + 16); // 每次扩展16字节
            }
            recvData[parseIndex] = b;
            parseIndex++;

            if (parseIndex >= expectedLength) {
                // 复制有效数据段
                recvDataLast.assign(
                    recvData.begin(),
                    recvData.begin() + expectedLength
                );

                parseIndex = 0;
                resetBuffer();
                return true;
            }
            return false;
        }
    }
}

// 数据接收
static void dataReceive() {
    try {
        std::cerr << LightService_IsConnected() << std::endl;
        while (LightService_IsConnected())
        {
            uint8_t recvBuffer[1024];
            const int bytesReceived = recv(client,
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
                if (parseByte(recvBuffer[i])) {
                    // 打印获取到的数据
                    printHex(recvDataLast);
                    for (TemperatureCallback temperatureCallback : temperatureCallbacks) {
                        if (recvDataLast[0] != 0x8D) {
                            continue;
                        }
                        if (recvDataLast[2] == FETCH_TEMPERATURE) {
                            temperatureCallback((int)(recvDataLast[3] & 0xFF) - 50, (int)(recvDataLast[4] & 0xFF) - 50);
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "探照灯消息接收错误: " << e.what() << std::endl;
        LightService_DisConnected();
    }
}

// 连接
bool LightService_Connection() {
    if (IsConned) return true;

    client = connection(Ip, Port);
    if (client == INVALID_SOCKET) {
        IsConned = false;
        return !(client == INVALID_SOCKET);
    }

    IsConned = true;
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
void LightService_DisConnected() {
    if (!IsConned) return;
    closesocket(client);
    IsConned = false;
}

// 检查连接状态
bool LightService_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// 发送数据
void LightService_SendData(const char* data, int length) {
    if (!LightService_IsConnected()) return;

    int result = send(client, data, length, 0);
    if (result == SOCKET_ERROR) {
        // 获取错误码并处理
        int error = WSAGetLastError();
        std::cerr << "探照灯消息发送失败，错误码: " << error << std::endl;
        LightService_DisConnected();
    }
}

// 1 开灯，0 关灯
void LightService_OpenLight(int open) {
    Msg msg;
    msg.SetMsgId(OPEN_CLOSE_LIGHT);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(open);
    msg.SetPayload(payload);
    LightService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());

    // 如果是关灯，也要把爆闪模式关掉，防止开着爆闪时关灯导致下次开灯还会是爆闪模式
    if (open == 0) {
        Sleep(100);
        LightService_SharpFlash(0);
    }
}

// 亮度调整
void LightService_LuminanceChange(int lum) {
    Msg msg;
    msg.SetMsgId(LUMINANCE_CHANGE);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(lum);
    msg.SetPayload(payload);
    LightService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 1 开爆闪，0 关爆闪
void LightService_SharpFlash(int open) {
    Msg msg;
    msg.SetMsgId(SHARP_FLASH);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(open);
    msg.SetPayload(payload);
    LightService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 获取温度
void LightService_FetchTemperature() {
    Msg msg;
    msg.SetMsgId(FETCH_TEMPERATURE);
    std::vector<uint8_t> payload(0);
    msg.SetPayload(payload);
    LightService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 云台控制，pitch 俯仰（0 - 12000），roll 横滚（暂时不起作用），yaw 偏航（0 - 34000）
void LightService_GimbalControl(int16_t pitch, int16_t roll, int16_t yaw) {
    Msg msg;
    msg.SetMsgId(TRIPOD_HEAD);
    std::vector<uint8_t> payload(6); // 创建长度为6的vector
    // 使用 memcpy 将数据按小端序写入 vector
    memcpy(payload.data(), &pitch, 2);      // 复制 pitch 到 [0, 1]
    memcpy(payload.data() + 2, &roll, 2);   // 复制 roll 到 [2, 3]
    memcpy(payload.data() + 4, &yaw, 2);    // 复制 yaw 到 [4, 5]
    msg.SetPayload(payload);
    LightService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 注册温度获取回调函数
void LightService_RegisterCallback(TemperatureCallback callback)
{
    temperatureCallbacks.push_back(callback);
}