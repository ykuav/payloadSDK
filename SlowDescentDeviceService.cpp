#include "SlowDescentDeviceService.h"
#include "common.h"
#include <string>
#include <iostream>
#include <thread>

static int parseIndex = 0;
static std::vector<uint8_t> recvData(128);
static std::vector<uint8_t> recvDataLast(128);

// 全局变量
static SOCKET client = INVALID_SOCKET;
static std::string Ip = "192.168.144.29"; // 必须使用std::string，不然C#传过来的ip后半部分会乱码
static int Port = 8519;
static bool IsConned = false;

/* 缓降器使能控制
* 发送1个字节
* byte0: 0: Disable缓降器，1: Enable缓降器
* */
static uint8_t DESCENT_CONTROL = 0x12;

/* 缓降器紧急控制
* 发送1个字节
* byte0: 0: 解除紧急状态，1: 紧急停车，2: 紧急熔断
* */
static uint8_t DESCENT_URGENT_CONTROL = 0x13;

/* 缓降器动作控制
* 发送3个字节
* byte0: 0: 长度控制模式(0 ~ 300分米)，1: 速度控制模式(-20 ~ +20米/分钟)（实际传0~40，0是-20米/分钟，40是20米/分钟）
* byte1: 目标长度或速度高8位
* byte2: 目标长度或速度低8位
* */
static uint8_t DESCENT_ACTION_CONTROL = 0x14;

/* 缓降器状态返回
* 无需发送，返回8个字节
* byte0: 0：缓降器已Disable，1: 缓降器已Enable
* byte1: 0: 长度控制模式，1: 速度控制模式
* byte2: 当前速度m/min
* byte3: 已释放长度高8位
* byte4: 已释放长度低8位
* byte5: 0: 缓降器未到限位，1: 缓降器已到顶，2: 缓降器已到底
* byte6: 载重 kg/LSB
* byte7:
*   0: 已解除紧急状态
*   1: 紧急刹车
*   2: 紧急熔断
*   3: 紧急刹车+熔断
* */
static uint8_t DESCENT_STATE_GET = 0x15;

static std::vector<SlowDescentDeviceCallback> slowDescentDeviceCallbacks;

// 清理函数
void SlowDescentDeviceService_Cleanup() {
    if (client != INVALID_SOCKET) closesocket(client);
    CleanupWinsock();
}

// 设置IP
void SlowDescentDeviceService_SetIp(const char* ip) {
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
        std::cerr << SlowDescentDeviceService_IsConnected() << std::endl;
        while (SlowDescentDeviceService_IsConnected())
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
                    for (SlowDescentDeviceCallback slowDescentDeviceCallback : slowDescentDeviceCallbacks) {
                        if (recvDataLast[0] != 0x8D) {
                            continue;
                        }
                        if (recvDataLast[2] == DESCENT_STATE_GET) {
                            slowDescentDeviceCallback(recvDataLast.data(), recvDataLast.size());
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "缓降器消息接收错误: " << e.what() << std::endl;
        SlowDescentDeviceService_DisConnected();
    }
}

// 连接
bool SlowDescentDeviceService_Connection() {
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
void SlowDescentDeviceService_DisConnected() {
    if (!IsConned) return;
    closesocket(client);
    IsConned = false;
}

// 检查连接状态
bool SlowDescentDeviceService_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// 发送数据
void SlowDescentDeviceService_SendData(const char* data, int length) {
    if (!SlowDescentDeviceService_IsConnected()) return;
    try {
        send(client, data, length, 0);
    }
    catch (const std::exception& e) {
        std::cerr << "缓降器消息发送失败: " << e.what() << std::endl;
        SlowDescentDeviceService_DisConnected();
    }
}

// 设置缓降器是否可用，true可用，false不可用
void SlowDescentDeviceService_Enable(BOOL flag) {
    Msg msg;
    msg.SetMsgId(DESCENT_CONTROL);
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
    SlowDescentDeviceService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 缓降器紧急控制
void SlowDescentDeviceService_EmergencyControl(int command) {
    Msg msg;
    msg.SetMsgId(DESCENT_URGENT_CONTROL);
    std::vector<uint8_t> payload(1);
    switch (command) {
        // 解除紧急状态（在发送了紧急停车或紧急熔断命令后，突然想取消停车和熔断的时候使用）
        case 0:
            payload[0] = static_cast<uint8_t>(0x00);
            break;
        // 紧急停车，紧急制动
        case 1:
            payload[0] = static_cast<uint8_t>(0x01);
            break;
        // 紧急熔断
        case 2:
            payload[0] = static_cast<uint8_t>(0x02);
            break;
    }
    msg.SetPayload(payload);
    SlowDescentDeviceService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

/* 缓降器动作控制
* mode: 0：按长度，1：按速度
* speedOrLength: 速度或长度
* */
void SlowDescentDeviceService_actionControl(int mode, int speedOrLength) {
    Msg msg;
    msg.SetMsgId(DESCENT_ACTION_CONTROL);
    std::vector<uint8_t> payload(3);
    payload[0] = static_cast<uint8_t>(mode);
    // 速度传值：0~40
    // 长度传值：0~300
    payload[1] = static_cast<uint8_t>(speedOrLength / 256);
    payload[2] = static_cast<uint8_t>(speedOrLength % 256);
    msg.SetPayload(payload);
    SlowDescentDeviceService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 注册抛投状态回调函数
void SlowDescentDeviceService_RegisterCallback(SlowDescentDeviceCallback callback)
{
    slowDescentDeviceCallbacks.push_back(callback);
}