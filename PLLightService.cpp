#include "PLLightService.h"
#include "common.h"
#include <string>
#include <iostream>
#include <thread>
#include <mstcpip.h>  // 关键头文件
#include <array>     // 提供 std::array
#include <cstdint>   // 提供 uint8_t, int32_t
#include <cstddef>   // 提供 size_t
#include "PLMsg.h"
#include <iomanip>
#pragma comment(lib, "Ws2_32.lib")  // 链接库

static int parseIndex = 0;
static std::vector<uint8_t> recvData(128);
static std::vector<uint8_t> recvDataLast(128);

// 全局变量
static SOCKET client = INVALID_SOCKET;
static std::string Ip = "192.168.144.36"; // 必须使用std::string，不然C#传过来的ip后半部分会乱码
static int Port = 8519;
static bool IsConned = false;

static uint8_t PTZCONTROL = 0x30; // 云台控制
static uint8_t LEDCONTROL = 0x2C; // 灯控制

static std::vector<Callback> callbacks;

// 清理函数
void PLLightService_Cleanup() {
    if (client != INVALID_SOCKET) closesocket(client);
    CleanupWinsock();
}

// 设置IP
void PLLightService_SetIp(const char* ip) {
    Ip = std::string(ip);
}

static void resetBuffer() {
    std::cerr << "清空recvData: " << std::endl;
    recvData.clear();
    std::cerr << "重置recvData: " << std::endl;
    recvData.resize(128, 0); // 初始化128字节缓冲区
}

// 数据接收
static void dataReceive() {
    try {
        std::cerr << PLLightService_IsConnected() << std::endl;
        while (PLLightService_IsConnected())
        {
            uint8_t recvBuffer[1024];
            const int bytesReceived = recv(client,
                reinterpret_cast<char*>(recvBuffer),
                sizeof(recvBuffer), 0);

            if (bytesReceived == SOCKET_ERROR) {
                const int error = WSAGetLastError();
                if (error == WSAEWOULDBLOCK) continue;
                throw std::runtime_error("品灵探照灯接收失败，错误码: " + std::to_string(error));
            }

            if (bytesReceived == 0) {
                continue;
            }

            for (Callback callback : callbacks) {
                if (recvDataLast.size() > 3)
                    if (recvDataLast[0] == 0x55 && recvDataLast[1] == 0xAA && recvDataLast[2] == 0xDC) {
                        callback(recvDataLast.data(), recvDataLast.size());
                    }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "品灵探照灯消息接收错误: " << e.what() << std::endl;
        PLLightService_DisConnected();
    }
}

// 连接
bool PLLightService_Connection() {
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
void PLLightService_DisConnected() {
    if (!IsConned) return;
    closesocket(client);
    IsConned = false;
}

// 检查连接状态
bool PLLightService_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// 发送数据
void PLLightService_SendData(const char* data, int length) {
    if (!PLLightService_IsConnected()) return;

    int result = send(client, data, length, 0);
    if (result == SOCKET_ERROR) {
        // 获取错误码并处理
        int error = WSAGetLastError();
        std::cerr << "品灵探照灯消息发送失败，错误码: " << error << std::endl;
        PLLightService_DisConnected();
    }
}

// true 开灯，false 关灯
void PLLightService_OpenLight(bool open) {
    PL_Msg msg;
    msg.SetMsgId(LEDCONTROL);
    std::vector<uint8_t> payload(3);
    payload[0] = 0x74;
    if (open) {
        payload[1] = 0x01;
    }
    else {
        payload[1] = 0x02;
    }
    msg.SetPayload(payload);
    PLLightService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 亮度调整
void PLLightService_LuminanceChange(int lum) {
    PL_Msg msg;
    msg.SetMsgId(LEDCONTROL);
    std::vector<uint8_t> payload(3);
    payload[0] = 0x74;
    payload[1] = 0x0A;
    payload[2] = lum;
    msg.SetPayload(payload);
    PLLightService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 开始爆闪
void PLLightService_startFlashing()
{
    PL_Msg msg;
    msg.SetMsgId(LEDCONTROL);
    std::vector<uint8_t> payload(3);
    payload[0] = 0x74;
    payload[1] = 0x09;
    msg.SetPayload(payload);
    PLLightService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 设置爆闪频率
void PLLightService_setFlashingFrequency(int frequency)
{
    PL_Msg msg;
    msg.SetMsgId(LEDCONTROL);
    std::vector<uint8_t> payload(3);
    payload[0] = 0x74;
    payload[1] = 0x0B;
    payload[2] = frequency;
    msg.SetPayload(payload);
    PLLightService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 云台回中
void PLLightService_PTZToCenter()
{
    PL_Msg msg;
    msg.SetMsgId(PTZCONTROL);
    std::vector<uint8_t> payload(14);
    payload[0] = 0x04;
    msg.SetPayload(payload);
    PLLightService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}


// 速度转byte数组，1bit=0.01°
// 使用std::vector<uint8_t>作为返回类型
static std::vector<uint8_t> speedToTwoByteArray(int speed) {
    int32_t value = speed * 100;

    return {
        static_cast<uint8_t>((value >> 8) & 0xFF),  // 高8位
        static_cast<uint8_t>(value & 0xFF)          // 低8位
    };
}

// 角度转byte数组，1bit=360/65536°
// angle值应该在-180到179之间
// 使用std::vector<uint8_t>作为返回类型
static std::vector<uint8_t> angleToTwoByteArray(int angle) {
    int32_t value = 0;

    if (angle > 179) value = (angle - 360) * 65536 / 360;
    else if (angle < -180) value = (angle + 360) * 65536 / 360;
    else value = angle * 65536 / 360;

    return {
        static_cast<uint8_t>((value >> 8) & 0xFF),  // 高8位
        static_cast<uint8_t>(value & 0xFF)          // 低8位
    };
}

// 云台控制，速度模式
void PLLightService_PTZCtrlBySpeed(int16_t yawSpeed, int16_t pitchSpeed, int16_t rollSpeed)
{
    PL_Msg msg;
    msg.SetMsgId(PTZCONTROL);
    std::vector<uint8_t> payload(14);
    std::vector<uint8_t> yawSpeedData = speedToTwoByteArray(yawSpeed);
    std::vector<uint8_t> pitchSpeedData = speedToTwoByteArray(pitchSpeed);
    std::vector<uint8_t> rollSpeedData = speedToTwoByteArray(rollSpeed);
    payload[0] = 0x01;
    // 复制速度数据到相应位置（使用迭代器避免越界）
    if (yawSpeedData.size() >= 2) {
        std::copy(yawSpeedData.begin(), yawSpeedData.end(), payload.begin() + 1);
    }
    if (pitchSpeedData.size() >= 2) {
        std::copy(pitchSpeedData.begin(), pitchSpeedData.end(), payload.begin() + 3);
    }
    if (rollSpeedData.size() >= 2) {
        std::copy(rollSpeedData.begin(), rollSpeedData.end(), payload.begin() + 5);
    }
    msg.SetPayload(payload);
    PLLightService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 云台控制，绝对角度模式，回中位置为0点
void PLLightService_PTZCtrlByAngle(int16_t yaw, int16_t pitch, int16_t roll)
{
    PL_Msg msg;
    msg.SetMsgId(PTZCONTROL);
    std::vector<uint8_t> payload(14);
    std::vector<uint8_t> yawData = angleToTwoByteArray(yaw);
    std::vector<uint8_t> pitchData = angleToTwoByteArray(pitch);
    std::vector<uint8_t> rollData = angleToTwoByteArray(roll);
    payload[0] = 0x0B;
    // 复制速度数据到相应位置（使用迭代器避免越界）
    if (yawData.size() >= 2) {
        std::copy(yawData.begin(), yawData.end(), payload.begin() + 1);
    }
    if (pitchData.size() >= 2) {
        std::copy(pitchData.begin(), pitchData.end(), payload.begin() + 3);
    }
    if (rollData.size() >= 2) {
        std::copy(rollData.begin(), rollData.end(), payload.begin() + 5);
    }
    msg.SetPayload(payload);
    PLLightService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 注册回调函数
void PLLightService_RegisterCallback(Callback callback)
{
    callbacks.push_back(callback);
}
