#include "WaterGunService.h"
#include "common.h"
#include <string>
#include <iostream>
#include <thread>

static int parseIndex = 0;
static std::vector<uint8_t> recvData(128);
static std::vector<uint8_t> recvDataLast(128);

// 全局变量
static SOCKET client = INVALID_SOCKET;
static std::string Ip = "192.168.144.33"; // 必须使用std::string，不然C#传过来的ip后半部分会乱码
static int Port = 8519;
static bool IsConned = false;

static uint8_t WATERGUN_TOLEFT = 0x33; // 往左
static uint8_t WATERGUN_TORIGHT = 0x34; // 往右
static uint8_t WATERGUN_MODESWITCH = 0x35;       // 模式切换
static uint8_t WATERGUN_STATE_SEND = 0x26;       // 发送心跳包
static uint8_t WATERGUN_STATE_RECEIVE = 0x25;    // 接收心跳包

static std::vector<WaterGunServiceStateCallback> waterGunStateCallbacks;

// 清理函数
void WaterGunService_Cleanup() {
    if (client != INVALID_SOCKET) closesocket(client);
    CleanupWinsock();
}

// 设置IP
void WaterGunService_SetIp(const char* ip) {
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
        std::cerr << WaterGunService_IsConnected() << std::endl;
        while (WaterGunService_IsConnected())
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
                    for (WaterGunServiceStateCallback waterGunStateCallback : waterGunStateCallbacks) {
                        if (recvDataLast[0] != 0x8D) {
                            continue;
                        }
                        if (recvDataLast[2] == WATERGUN_STATE_RECEIVE) {
                            waterGunStateCallback(recvDataLast[3], recvDataLast[4]);
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "水枪消息接收错误: " << e.what() << std::endl;
        WaterGunService_DisConnected();
    }
}

// 连接
bool WaterGunService_Connection() {
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
void WaterGunService_DisConnected() {
    if (!IsConned) return;
    closesocket(client);
    IsConned = false;
}

// 检查连接状态
bool WaterGunService_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// 发送数据
void WaterGunService_SendData(const char* data, int length) {
    if (!WaterGunService_IsConnected()) return;
    try {
        send(client, data, length, 0);
    }
    catch (const std::exception& e) {
        std::cerr << "水枪消息发送失败: " << e.what() << std::endl;
        WaterGunService_DisConnected();
    }
}

// 水枪往左
void WaterGunService_ToLeft() {
    Msg msg;
    msg.SetMsgId(WATERGUN_TOLEFT);
    std::vector<uint8_t> payload(4);
    msg.SetPayload(payload);
    WaterGunService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 水枪往右
void WaterGunService_ToRight() {
    Msg msg;
    msg.SetMsgId(WATERGUN_TORIGHT);
    std::vector<uint8_t> payload(4);
    msg.SetPayload(payload);
    WaterGunService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

/* 操作水枪模式切换
* 状态为0时，不可用
* 状态为1时，发送该命令会变成自动模式，即水枪自动左右转，状态会变为2
* 状态为2时，发送该命令，水枪停止左右转动，状态变为3，水枪慢慢回归中点，然后自动变为手动模式，状态变为4
* 状态为4时，发送该命令，状态变为5，与状态1一致
* */
void WaterGunService_ModeSwitch() {
    Msg msg;
    msg.SetMsgId(WATERGUN_MODESWITCH);
    std::vector<uint8_t> payload(4);
    msg.SetPayload(payload);
    WaterGunService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 发送心跳包
void WaterGunService_Heartbeat() {
    Msg msg;
    msg.SetMsgId(WATERGUN_STATE_SEND);
    std::vector<uint8_t> payload(4);
    msg.SetPayload(payload);
    WaterGunService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 注册消防水枪状态回调函数
void WaterGunService_RegisterCallback(WaterGunServiceStateCallback callback)
{
    waterGunStateCallbacks.push_back(callback);
}