#include "ThrowerService.h"
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
static std::string Ip = "192.168.144.99"; // 必须使用std::string，不然C#传过来的ip后半部分会乱码
static int Port = 8519;
static bool IsConned = false;

static uint8_t THROWER_UPDATE = 0x0A; // 抛投板子程序升级，4字节，分别是 空、空、空、空
static uint8_t THROWER_CONTROL_ONE = 0x21; // 单舵机动作控制，4字节，分别是 舵机号（0-7）、关闭/开启（0/1）、空、空
static uint8_t THROWER_CONTROL_ALL = 0x22; // 全部舵机动作控制，4字节，分别是 关闭/开启（0/1）、空、空、空
static uint8_t THROWER_CHARGING = 0x23; // 充电放电，4字节，分别是 放电/充电（0/1）、空、空、空
static uint8_t THROWER_ALLOW_DETONATION = 0x24; // 允许起爆，4字节，分别是 取消/允许起爆（0/1）、空、空、空
static uint8_t THROWER_STATE = 0x25; // 舵机状态，无需发送，每1秒自动上报一次，6字节，分别是 高度、起爆状态、充电状态、温度、总状态、起爆高度
/* 新版修改，0x25返回40个字节
* （8字节）：雷达高度、起爆高度、保留、保留、保留、保留、保留、保留
* 1号弹（8字节）：起爆充电状态（0：不允许起爆，1：允许起爆，2：正在充电，127：未连接）、高度状态（0：高度不足，1：高度足够）、保留、保留、总状态（0：无法引爆，1：可以引爆）、保留、保留、保留
* 2号弹（8字节）：起爆充电状态（0：不允许起爆，1：允许起爆，2：正在充电，127：未连接）、高度状态（0：高度不足，1：高度足够）、保留、保留、总状态（0：无法引爆，1：可以引爆）、保留、保留、保留
* 3号弹（8字节）：起爆充电状态（0：不允许起爆，1：允许起爆，2：正在充电，127：未连接）、高度状态（0：高度不足，1：高度足够）、保留、保留、总状态（0：无法引爆，1：可以引爆）、保留、保留、保留
* 4号弹（8字节）：起爆充电状态（0：不允许起爆，1：允许起爆，2：正在充电，127：未连接）、高度状态（0：高度不足，1：高度足够）、保留、保留、总状态（0：无法引爆，1：可以引爆）、保留、保留、保留
* */

static uint8_t THROWER_CONNECT_TEST = 0x26; // 连接测试，心跳包，定时发送，4字节，分别是 空、空、空、空
static uint8_t THROWER_DETONATE_HEIGHT = 0x27; // 设置起爆高度，4字节，分别是 起爆高度、空、空、空
static uint8_t THROWER_CONTROL_TWO_CENTER = 0x28; // 双舵机动作控制(中间俩)，4字节，分别是 关闭/开启（0/1）、空、空、空
static uint8_t THROWER_CONTROL_TWO_LEFT = 0x29; // 双舵机动作控制(左侧俩1/2)，4字节，分别是 关闭/开启（0/1）、空、空、空
static uint8_t THROWER_CONTROL_TWO_RIGHT = 0x2A; // 双舵机动作控制(右侧俩7/8)，4字节，分别是 关闭/开启（0/1）、空、空、空
static uint8_t THROWER_CHARGING_AND_ALLOW = 0x31; // 充电放电和允许起爆一起，4字节，分别是 弹号（1-4）、状态（0/1）、空、空

static std::vector<ThrowerStateCallback> throwerStateCallbacks;

// 清理函数
void ThrowerService_Cleanup() {
    if (client != INVALID_SOCKET) closesocket(client);
    CleanupWinsock();
}

// 设置IP
void ThrowerService_SetIp(const char* ip) {
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
        std::cerr << ThrowerService_IsConnected() << std::endl;
        while (ThrowerService_IsConnected())
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
                    for (ThrowerStateCallback throwerStateCallback : throwerStateCallbacks) {
                        if (recvDataLast[0] != 0x8D) {
                            continue;
                        }
                        if (recvDataLast[2] == THROWER_STATE) {
                            throwerStateCallback(recvDataLast.data(), recvDataLast.size());
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "抛投消息接收错误: " << e.what() << std::endl;
        ThrowerService_DisConnected();
    }
}

// 连接
bool ThrowerService_Connection() {
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
void ThrowerService_DisConnected() {
    if (!IsConned) return;
    closesocket(client);
    IsConned = false;
}

// 检查连接状态
bool ThrowerService_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// 发送数据
void ThrowerService_SendData(const char* data, int length) {
    if (!ThrowerService_IsConnected()) return;

    int result = send(client, data, length, 0);
    if (result == SOCKET_ERROR) {
        // 获取错误码并处理
        int error = WSAGetLastError();
        std::cerr << "抛投消息发送失败，错误码: " << error << std::endl;
        ThrowerService_DisConnected();
    }
}

// 打开舵机，index是舵机序号，0-5
void ThrowerService_Open(int index) {
    Msg msg;
    msg.SetMsgId(THROWER_CONTROL_ONE);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(index);
    payload[1] = static_cast<uint8_t>(0x01);
    msg.SetPayload(payload);
    ThrowerService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 舵机复位（关闭），index是舵机序号，0-5
void ThrowerService_Close(int index) {
    Msg msg;
    msg.SetMsgId(THROWER_CONTROL_ONE);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(index);
    payload[1] = static_cast<uint8_t>(0x00);
    msg.SetPayload(payload);
    ThrowerService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 打开中间两个舵机
void ThrowerService_OpenCenter() {
    Msg msg;
    msg.SetMsgId(THROWER_CONTROL_TWO_CENTER);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(0x01);
    msg.SetPayload(payload);
    ThrowerService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 关闭中间两个舵机
void ThrowerService_CloseCenter() {
    Msg msg;
    msg.SetMsgId(THROWER_CONTROL_TWO_CENTER);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(0x00);
    msg.SetPayload(payload);
    ThrowerService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 打开左侧两个舵机
void ThrowerService_OpenLeft() {
    Msg msg;
    msg.SetMsgId(THROWER_CONTROL_TWO_LEFT);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(0x01);
    msg.SetPayload(payload);
    ThrowerService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 关闭左侧两个舵机
void ThrowerService_CloseLeft() {
    Msg msg;
    msg.SetMsgId(THROWER_CONTROL_TWO_LEFT);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(0x00);
    msg.SetPayload(payload);
    ThrowerService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 打开右侧两个舵机
void ThrowerService_OpenRight() {
    Msg msg;
    msg.SetMsgId(THROWER_CONTROL_TWO_RIGHT);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(0x01);
    msg.SetPayload(payload);
    ThrowerService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 关闭右侧两个舵机
void ThrowerService_CloseRight() {
    Msg msg;
    msg.SetMsgId(THROWER_CONTROL_TWO_RIGHT);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(0x00);
    msg.SetPayload(payload);
    ThrowerService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 打开全部舵机
void ThrowerService_OpenAll() {
    Msg msg;
    msg.SetMsgId(THROWER_CONTROL_ALL);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(0x01);
    msg.SetPayload(payload);
    ThrowerService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 关闭全部舵机
void ThrowerService_CloseAll() {
    Msg msg;
    msg.SetMsgId(THROWER_CONTROL_ALL);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(0x00);
    msg.SetPayload(payload);
    ThrowerService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 充电放电，type=true是充电，type=false是放电(新版已弃用)
void ThrowerService_Charging(BOOL type) {
    Msg msg;
    msg.SetMsgId(THROWER_CHARGING);
    std::vector<uint8_t> payload(4);
    if (type) {
        payload[0] = static_cast<uint8_t>(0x01);
    }
    else {
        payload[0] = static_cast<uint8_t>(0x00);
    }
    msg.SetPayload(payload);
    ThrowerService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 是否允许引爆，type=true是允许引爆，type=false是禁止引爆(新版已弃用)
void ThrowerService_AllowDetonation(BOOL type) {
    Msg msg;
    msg.SetMsgId(THROWER_ALLOW_DETONATION);
    std::vector<uint8_t> payload(4);
    if (type) {
        payload[0] = static_cast<uint8_t>(0x01);
    }
    else {
        payload[0] = static_cast<uint8_t>(0x00);
    }
    msg.SetPayload(payload);
    ThrowerService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 允许起爆和开始充电（新版使用）
void ThrowerService_chargingAndAllowDetonation(int index, BOOL _switch) {
    Msg msg;
    msg.SetMsgId(THROWER_CHARGING_AND_ALLOW);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(index);;
    if (_switch) {
        payload[1] = static_cast<uint8_t>(0x01);
    }
    else {
        payload[1] = static_cast<uint8_t>(0x00);
    }
    msg.SetPayload(payload);
    ThrowerService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 设置引爆高度，灭火弹下落到指定高度后会自动引爆
void ThrowerService_SetDetonateHeight(int height) {
    Msg msg;
    msg.SetMsgId(THROWER_DETONATE_HEIGHT);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(height);
    msg.SetPayload(payload);
    ThrowerService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 心跳包，抛投连接成功后需定时发送
// 抛投如果15s左右未收到心跳包，会自动重启
void ThrowerService_Heartbeat() {
    Msg msg;
    msg.SetMsgId(THROWER_CONNECT_TEST);
    std::vector<uint8_t> payload(4);
    msg.SetPayload(payload);
    ThrowerService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 更新板子程序
void ThrowerService_Update() {
    Msg msg;
    msg.SetMsgId(THROWER_UPDATE);
    std::vector<uint8_t> payload(4);
    msg.SetPayload(payload);
    ThrowerService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 注册抛投状态回调函数
void ThrowerService_RegisterCallback(ThrowerStateCallback callback)
{
    throwerStateCallbacks.push_back(callback);
}