#include "WaterBranchService.h"
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
static std::string Ip = "192.168.144.35"; // 必须使用std::string，不然C#传过来的ip后半部分会乱码
static int Port = 8519;
static bool IsConned = false;

static uint8_t WATERBRANCH_STATE_RECEIVE = 0x25; // 接收心跳包
static uint8_t WATERBRANCH_STATE_SEND = 0x26; // 发送心跳包
static uint8_t WATERBRANCH_HOSE_RELEASE = 0x2B; // 释放水带
static uint8_t WATERBRANCH_HOSE_DETACHMENT = 0x2C; // 水带脱困

static std::vector<WaterBranchServiceStateCallback> bucketStateCallbacks;

// 清理函数
void WaterBranchService_Cleanup() {
    if (client != INVALID_SOCKET) closesocket(client);
    CleanupWinsock();
}

// 设置IP
void WaterBranchService_SetIp(const char* ip) {
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
        std::cerr << WaterBranchService_IsConnected() << std::endl;
        while (WaterBranchService_IsConnected())
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
                    for (WaterBranchServiceStateCallback WaterBranchStateCallback : bucketStateCallbacks) {
                        if (recvDataLast[0] != 0x8D) {
                            continue;
                        }
                        if (recvDataLast[2] == WATERBRANCH_STATE_RECEIVE) {
                            WaterBranchStateCallback(recvDataLast.data(), recvDataLast.size());
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "消防水枪消息接收错误: " << e.what() << std::endl;
        WaterBranchService_DisConnected();
    }
}

// 连接
bool WaterBranchService_Connection() {
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
void WaterBranchService_DisConnected() {
    if (!IsConned) return;
    closesocket(client);
    IsConned = false;
}

// 检查连接状态
bool WaterBranchService_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// 发送数据
void WaterBranchService_SendData(const char* data, int length) {
    if (!WaterBranchService_IsConnected()) return;

    int result = send(client, data, length, 0);
    if (result == SOCKET_ERROR) {
        // 获取错误码并处理
        int error = WSAGetLastError();
        std::cerr << "消防水枪消息发送失败，错误码: " << error << std::endl;
        WaterBranchService_DisConnected();
    }
}

// 释放水带，0关，1开
void WaterBranchService_HoseRelease(int operateType) {
    Msg msg;
    msg.SetMsgId(WATERBRANCH_HOSE_RELEASE);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(operateType);
    msg.SetPayload(payload);
    WaterBranchService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 水带脱困，0关，1开
void WaterBranchService_HoseDetachment(int operateType) {
    Msg msg;
    msg.SetMsgId(WATERBRANCH_HOSE_DETACHMENT);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(operateType);
    msg.SetPayload(payload);
    WaterBranchService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 发送心跳包
void WaterBranchService_Heartbeat() {
    Msg msg;
    msg.SetMsgId(WATERBRANCH_STATE_SEND);
    std::vector<uint8_t> payload(4);
    msg.SetPayload(payload);
    WaterBranchService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 注册消防水枪状态回调函数
void WaterBranchService_RegisterCallback(WaterBranchServiceStateCallback callback)
{
    bucketStateCallbacks.push_back(callback);
}