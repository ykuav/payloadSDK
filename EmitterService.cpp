#include "EmitterService.h"
#include "common.h"
#include <string>
#include <iostream>
#include <thread>

static int parseIndex = 0;
static std::vector<uint8_t> recvData(128);
static std::vector<uint8_t> recvDataLast(128);

// 全局变量
static SOCKET client = INVALID_SOCKET;
static std::string Ip = "192.168.144.26"; // 必须使用std::string，不然C#传过来的ip后半部分会乱码
static int Port = 8519;
static bool IsConned = false;

static uint8_t EMITTER_LAUNCH = 0x11; // 发射控制
static uint8_t EMITTER_STATUS = 0x10; // 获取发射口状态

static std::vector<EmitterStatusCallback> emitterStatusCallbacks;

// 清理函数
void EmitterService_Cleanup() {
    if (client != INVALID_SOCKET) closesocket(client);
    CleanupWinsock();
}

// 设置IP
void EmitterService_SetIp(const char* ip) {
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
        std::cerr << EmitterService_IsConnected() << std::endl;
        while (EmitterService_IsConnected())
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
                    for (EmitterStatusCallback emitterStatusCallback : emitterStatusCallbacks) {
                        if (recvDataLast[0] != 0x8D) {
                            continue;
                        }
                        if (recvDataLast[2] == EMITTER_LAUNCH) {
                            emitterStatusCallback(recvDataLast.data(), recvDataLast.size());
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "38mm发射器消息接收错误: " << e.what() << std::endl;
        EmitterService_DisConnected();
    }
}

// 连接
bool EmitterService_Connection() {
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
void EmitterService_DisConnected() {
    if (!IsConned) return;
    closesocket(client);
    IsConned = false;
}

// 检查连接状态
bool EmitterService_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// 发送数据
void EmitterService_SendData(const char* data, int length) {
    if (!EmitterService_IsConnected()) return;
    try {
        send(client, data, length, 0);
    }
    catch (const std::exception& e) {
        std::cerr << "38mm发射器消息发送失败: " << e.what() << std::endl;
        EmitterService_DisConnected();
    }
}

// 发射
void EmitterService_Launch(int index) {
    Msg msg;
    msg.SetMsgId(EMITTER_LAUNCH);
    std::vector<uint8_t> payload(16);
    payload[index] = static_cast<uint8_t>(0x01);
    msg.SetPayload(payload);
    EmitterService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 发射口状态查询
void EmitterService_GetStatus() {
    Msg msg;
    msg.SetMsgId(EMITTER_STATUS);
    std::vector<uint8_t> payload(0);
    msg.SetPayload(payload);
    EmitterService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 发射口状态回调函数
void EmitterService_RegisterCallback(EmitterStatusCallback callback)
{
    emitterStatusCallbacks.push_back(callback);
}