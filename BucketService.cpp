#include "BucketService.h"
#include "common.h"
#include <string>
#include <iostream>
#include <thread>

static int parseIndex = 0;
static std::vector<uint8_t> recvData(128);
static std::vector<uint8_t> recvDataLast(128);

// 全局变量
static SOCKET client = INVALID_SOCKET;
static std::string Ip = "192.168.144.34"; // 必须使用std::string，不然C#传过来的ip后半部分会乱码
static int Port = 8519;
static bool IsConned = false;

static uint8_t BUCKET_STATE_RECEIVE = 0x25; // 接收心跳包
static uint8_t BUCKET_STATE_SEND = 0x26; // 发送心跳包
static uint8_t BUCKET_BARREL_CONTROL = 0x36; // 水桶控制
static uint8_t BUCKET_HOOK_CONTROL = 0x38; // 挂钩控制

static std::vector<BucketServiceStateCallback> bucketStateCallbacks;

// 清理函数
void BucketService_Cleanup() {
    if (client != INVALID_SOCKET) closesocket(client);
    CleanupWinsock();
}

// 设置IP
void BucketService_SetIp(const char* ip) {
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
        std::cerr << BucketService_IsConnected() << std::endl;
        while (BucketService_IsConnected())
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
                    for (BucketServiceStateCallback BucketStateCallback : bucketStateCallbacks) {
                        if (recvDataLast[0] != 0x8D) {
                            continue;
                        }
                        if (recvDataLast[2] == BUCKET_STATE_RECEIVE) {
                            BucketStateCallback(recvDataLast.data(), recvDataLast.size());
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "吊桶消息接收错误: " << e.what() << std::endl;
        BucketService_DisConnected();
    }
}

// 连接
bool BucketService_Connection() {
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
void BucketService_DisConnected() {
    if (!IsConned) return;
    closesocket(client);
    IsConned = false;
}

// 检查连接状态
bool BucketService_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// 发送数据
void BucketService_SendData(const char* data, int length) {
    if (!BucketService_IsConnected()) return;
    try {
        send(client, data, length, 0);
    }
    catch (const std::exception& e) {
        std::cerr << "吊桶消息发送失败: " << e.what() << std::endl;
        BucketService_DisConnected();
    }
}

// 操作吊桶开关，0停，1开（升），2关（降）
void BucketService_BarrelControl(int controlType) {
    Msg msg;
    msg.SetMsgId(BUCKET_BARREL_CONTROL);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(controlType);
    msg.SetPayload(payload);
    BucketService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 操作挂钩开关，0关，1开
void BucketService_HookControl(int controlType) {
    Msg msg;
    msg.SetMsgId(BUCKET_HOOK_CONTROL);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(controlType);
    msg.SetPayload(payload);
    BucketService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 发送心跳包
void BucketService_Heartbeat() {
    Msg msg;
    msg.SetMsgId(BUCKET_STATE_SEND);
    std::vector<uint8_t> payload(4);
    msg.SetPayload(payload);
    BucketService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 注册吊桶状态回调函数
void BucketService_RegisterCallback(BucketServiceStateCallback callback)
{
    bucketStateCallbacks.push_back(callback);
}