#include "SlowDescentDevice200Service.h"
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
static std::string Ip = "192.168.144.39"; // 必须使用std::string，不然C#传过来的ip后半部分会乱码
static int Port = 8519;
static bool IsConned = false;

/* 缓降器使能控制
* 发送1个字节
* byte0: 0: Disable缓降器，1: Enable缓降器
* */
static uint8_t DESCENT200_CONTROL = 0x01;

/* 缓降器红蓝警示灯
* 发送1个字节
* byte0: 0: 关，1: 开
* */
static uint8_t DESCENT200_WARNING_LIGHT_CONTROL = 0x02;

/* 缓降器紧急控制
* 发送1个字节
* byte0: 0: 复位，1：急停，2：熔断
* */
static uint8_t DESCENT200_URGENT_CONTROL = 0x03;

/* 缓降器速度控制（-100~100cm/s）
* 发送1个字节
* byte0: 速度（-100~100cm/s）
* */
static uint8_t DESCENT200_SPEED_CONTROL = 0x04;

/* 缓降器长度控制（0~100m）
* 发送1个字节
* byte0: 放线长度（0~100m）
* */
static uint8_t DESCENT200_LENGTH_CONTROL = 0x05;

/* 缓降器挂钩开关
* 发送1个字节
* byte0: 0: 关，1: 开
* */
static uint8_t DESCENT200_HOOK_CONTROL = 0x06;

/* 缓降器重量清零
* 发送1个字节
* byte0: 留空
* */
static uint8_t DESCENT200_RESET_WEIGHT_CONTROL = 0x30;

/* 缓降器状态返回
* 无需发送，返回16个字节
* byte0: 安全开关状态，0：关，1: 开
* byte1: 触顶状态，0: 未触顶，1: 触顶
* byte2: 红蓝指示灯状态，0：关，1: 开
* byte3: 吊载重量高位（0-300kg）
* byte4: 吊载重量低位
* byte5: 水平摆角高位（0-359°）
* byte6: 水平摆角低位
* byte7: 缓降钩速度（-100~100cm/s），负值上升，正值下降，0停止
* byte8: 释放绳长高位（单位是0.1m）
* byte9: 释放绳长低位
* byte10: 缓降钩开关状态，0：关，1: 开
* byte11: 缓降钩通信状态，0：正常，1: 断连
* byte12: 缓降钩电压高位（单位0.01伏）
* byte13: 缓降钩电压低位
* byte14: 主板温度（-40℃~150℃）
* byte15: 保留
* */
static uint8_t DESCENT200_STATE_GET = 0x90;

static std::vector<SlowDescentDevice200Callback> slowDescentDevice200Callbacks;

// 清理函数
void SlowDescentDevice200Service_Cleanup() {
    if (client != INVALID_SOCKET) closesocket(client);
    CleanupWinsock();
}

// 设置IP
void SlowDescentDevice200Service_SetIp(const char* ip) {
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
        std::cerr << SlowDescentDevice200Service_IsConnected() << std::endl;
        while (SlowDescentDevice200Service_IsConnected())
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
                    for (SlowDescentDevice200Callback slowDescentDevice200Callback : slowDescentDevice200Callbacks) {
                        if (recvDataLast[0] != 0x8D) {
                            continue;
                        }
                        if (recvDataLast[2] == DESCENT200_STATE_GET) {
                            slowDescentDevice200Callback(recvDataLast.data(), recvDataLast.size());
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "200kg缓降器消息接收错误: " << e.what() << std::endl;
        SlowDescentDevice200Service_DisConnected();
    }
}

// 连接
bool SlowDescentDevice200Service_Connection() {
    if (IsConned) return true;

    client = connection(Ip, Port);
    if (client == INVALID_SOCKET) {
        IsConned = false;
        return !(client == INVALID_SOCKET);
    }

    IsConned = true;
    std::thread t(dataReceive);
    t.detach();

    return true;
}

// 断开连接
void SlowDescentDevice200Service_DisConnected() {
    if (!IsConned) return;
    closesocket(client);
    IsConned = false;
}

// 检查连接状态
bool SlowDescentDevice200Service_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// 发送数据
void SlowDescentDevice200Service_SendData(const char* data, int length) {
    if (!SlowDescentDevice200Service_IsConnected()) return;

    int result = send(client, data, length, 0);
    if (result == SOCKET_ERROR) {
        // 获取错误码并处理
        int error = WSAGetLastError();
        std::cerr << "缓降器消息发送失败，错误码: " << error << std::endl;
        SlowDescentDevice200Service_DisConnected();
    }
}

// 设置缓降器是否可用，true可用，false不可用
void SlowDescentDevice200Service_Enable(BOOL flag) {
    Msg msg;
    msg.SetMsgId(DESCENT200_CONTROL);
    std::vector<uint8_t> payload(1);
    // Enable缓降器
    if (flag) {
        payload[0] = static_cast<uint8_t>(0x01);
    }
    // Disable缓降器
    else {
        payload[0] = static_cast<uint8_t>(0x00);
    }
    msg.SetPayload(payload);
    SlowDescentDevice200Service_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 缓降器紧急控制
void SlowDescentDevice200Service_EmergencyControl(int command) {
    Msg msg;
    msg.SetMsgId(DESCENT200_URGENT_CONTROL);
    std::vector<uint8_t> payload(1);
    switch (command) {
        // 复位
    case 0:
        payload[0] = static_cast<uint8_t>(0x00);
        break;
        // 急停
    case 1:
        payload[0] = static_cast<uint8_t>(0x01);
        break;
        // 熔断
    case 2:
        payload[0] = static_cast<uint8_t>(0x02);
        break;
    }
    msg.SetPayload(payload);
    SlowDescentDevice200Service_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 红蓝开关
void SlowDescentDevice200Service_WarningLightControl(BOOL flag) {
    Msg msg;
    msg.SetMsgId(DESCENT200_WARNING_LIGHT_CONTROL);
    std::vector<uint8_t> payload(1);
    // 打开红蓝
    if (flag) {
        payload[0] = static_cast<uint8_t>(0x01);
    }
    // 关闭红蓝
    else {
        payload[0] = static_cast<uint8_t>(0x00);
    }
    msg.SetPayload(payload);
    SlowDescentDevice200Service_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 按速度控制
void SlowDescentDevice200Service_ControlBySpeed(int speed) {
    Msg msg;
    msg.SetMsgId(DESCENT200_SPEED_CONTROL);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(speed);
    msg.SetPayload(payload);
    SlowDescentDevice200Service_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 按长度控制
void SlowDescentDevice200Service_ControlByLength(int length) {
    Msg msg;
    msg.SetMsgId(DESCENT200_LENGTH_CONTROL);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(length);
    msg.SetPayload(payload);
    SlowDescentDevice200Service_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 挂钩开关控制，false: 关，true: 开
void SlowDescentDevice200Service_HookControl(BOOL flag) {
    Msg msg;
    msg.SetMsgId(DESCENT200_HOOK_CONTROL);
    std::vector<uint8_t> payload(1);
    // 打开挂钩
    if (flag) {
        payload[0] = static_cast<uint8_t>(0x01);
    }
    // 关闭挂钩
    else {
        payload[0] = static_cast<uint8_t>(0x00);
    }
    msg.SetPayload(payload);
    SlowDescentDevice200Service_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 重量清零
void SlowDescentDevice200Service_ResetWeight() {
    Msg msg;
    msg.SetMsgId(DESCENT200_RESET_WEIGHT_CONTROL);
    std::vector<uint8_t> payload(1);
    msg.SetPayload(payload);
    SlowDescentDevice200Service_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 注册抛投状态回调函数
void SlowDescentDevice200Service_RegisterCallback(SlowDescentDevice200Callback callback)
{
    slowDescentDevice200Callbacks.push_back(callback);
}