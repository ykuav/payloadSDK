#include "ResqmeService.h"
#include "common.h"
#include <string>
#include <iostream>
#include <thread>

static int parseIndex = 0;
static std::vector<uint8_t> recvData(128);
static std::vector<uint8_t> recvDataLast(128);

// 全局变量
static SOCKET client = INVALID_SOCKET;
static std::string Ip = "192.168.144.31"; // 必须使用std::string，不然C#传过来的ip后半部分会乱码
static int Port = 8519;
static bool IsConned = false;

static uint8_t RESQME_STATUS = 0x31;        // 状态
static uint8_t RESQME_LUNCH = 0x32;         // 发射
static uint8_t RESQME_SAFETY_SWITCH = 0x33; // 安全开关

static std::vector<ResqmeStateCallback> resqmeStateCallbacks;

// 清理函数
void ResqmeService_Cleanup() {
    if (client != INVALID_SOCKET) closesocket(client);
    CleanupWinsock();
}

// 设置IP
void ResqmeService_SetIp(const char* ip) {
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
        std::cerr << ResqmeService_IsConnected() << std::endl;
        while (ResqmeService_IsConnected())
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
                    for (ResqmeStateCallback resqmeStateCallback : resqmeStateCallbacks) {
                        if (recvDataLast[0] != 0x8D) {
                            continue;
                        }
                        if (recvDataLast[2] == RESQME_STATUS) {
                            resqmeStateCallback(recvDataLast.data(), recvDataLast.size());
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "破窗器消息接收错误: " << e.what() << std::endl;
        ResqmeService_DisConnected();
    }
}

// 连接
bool ResqmeService_Connection() {
    if (IsConned) return true;

    client = connection(Ip, Port);
    if (client == INVALID_SOCKET) {
        IsConned = false;
        return false;
    }

    IsConned = true;
    std::thread t(dataReceive);
    t.detach();
    return true;
}

// 断开连接
void ResqmeService_DisConnected() {
    if (!IsConned) return;
    closesocket(client);
    IsConned = false;
}

// 检查连接状态
bool ResqmeService_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// 发送数据
void ResqmeService_SendData(const char* data, int length) {
    if (!ResqmeService_IsConnected()) return;
    try {
        send(client, data, length, 0);
    }
    catch (const std::exception& e) {
        std::cerr << "破窗器消息发送失败: " << e.what() << std::endl;
        ResqmeService_DisConnected();
    }
}

// 发射，index: 1：1号口，2：2号口，3：全部
void ResqmeService_Launch(int index) {
    Msg msg;
    msg.SetMsgId(RESQME_LUNCH);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(index);
    payload[1] = static_cast<uint8_t>(0x01);
    msg.SetPayload(payload);
    ResqmeService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 安全开关，state=true时打开
void ResqmeService_SafetySwitch(BOOL state) {
    Msg msg;
    msg.SetMsgId(RESQME_SAFETY_SWITCH);
    std::vector<uint8_t> payload(4);
    if (state) {
        payload[1] = static_cast<uint8_t>(0x01);
    }
    else {
        payload[1] = static_cast<uint8_t>(0x00);
    }
    msg.SetPayload(payload);
    ResqmeService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 注册破窗器发射口状态回调函数
void ResqmeService_RegisterCallback(ResqmeStateCallback callback)
{
    resqmeStateCallbacks.push_back(callback);
}