#include "CaptureNetService.h"
#include "common.h"
#include <string>
#include <iostream>
#include <thread>

// 全局变量
static SOCKET client = INVALID_SOCKET;
static std::string Ip = "192.168.144.25"; // 必须使用std::string，不然C#传过来的ip后半部分会乱码
static int Port = 8519;
static bool IsConned = false;

static uint8_t SEROV_CONTROL = 0x09; // 舵机控制

// 清理函数
void CaptureNetService_Cleanup() {
    if (client != INVALID_SOCKET) closesocket(client);
    CleanupWinsock();
}

// 设置IP
void CaptureNetService_SetIp(const char* ip) {
    Ip = std::string(ip);
}

// 连接
bool CaptureNetService_Connection() {
    if (IsConned) return true;

    client = connection(Ip, Port);
    if (client == INVALID_SOCKET) {
        IsConned = false;
        return false;
    }

    IsConned = true;
    return true;
}

// 断开连接
void CaptureNetService_DisConnected() {
    if (!IsConned) return;
    closesocket(client);
    IsConned = false;
}

// 检查连接状态
bool CaptureNetService_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// 发送数据
void CaptureNetService_SendData(const char* data, int length) {
    if (!CaptureNetService_IsConnected()) return;
    try {
        send(client, data, length, 0);
    }
    catch (const std::exception& e) {
        std::cerr << "捕捉网消息发送失败: " << e.what() << std::endl;
        CaptureNetService_DisConnected();
    }
}

// 发射
void CaptureNetService_Launch() {
    Msg msg;
    msg.SetMsgId(SEROV_CONTROL);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(0x01);
    msg.SetPayload(payload);
    CaptureNetService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}