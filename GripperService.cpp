#include "GripperService.h"
#include "common.h"
#include <string>
#include <iostream>
#include <thread>

static int parseIndex = 0;
static std::vector<uint8_t> recvData(128);
static std::vector<uint8_t> recvDataLast(128);

// 全局变量
static SOCKET client = INVALID_SOCKET;
static std::string Ip = "192.168.144.30"; // 必须使用std::string，不然C#传过来的ip后半部分会乱码
static int Port = 8519;
static bool IsConned = false;

static uint8_t EMITTER_GRAB_OR_RELEASE = 0x35;

// 清理函数
void GripperService_Cleanup() {
    if (client != INVALID_SOCKET) closesocket(client);
    CleanupWinsock();
}

// 设置IP
void GripperService_SetIp(const char* ip) {
    Ip = std::string(ip);
}

static void resetBuffer() {
    std::cerr << "清空recvData: " << std::endl;
    recvData.clear();
    std::cerr << "重置recvData: " << std::endl;
    recvData.resize(128, 0); // 初始化128字节缓冲区
}

// 连接
bool GripperService_Connection() {
    if (IsConned) return true;

    client = connection(Ip, Port);
    if (client == INVALID_SOCKET) {
        IsConned = false;
        return !(client == INVALID_SOCKET);
    }

    IsConned = true;
    return true;
}

// 断开连接
void GripperService_DisConnected() {
    if (!IsConned) return;
    closesocket(client);
    IsConned = false;
}

// 检查连接状态
bool GripperService_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// 发送数据
void GripperService_SendData(const char* data, int length) {
    if (!GripperService_IsConnected()) return;

    int result = send(client, data, length, 0);
    if (result == SOCKET_ERROR) {
        // 获取错误码并处理
        int error = WSAGetLastError();
        std::cerr << "机械爪消息发送失败，错误码: " << error << std::endl;
        GripperService_DisConnected();
    }
}

// 抓取
void GripperService_Grab() {
    Msg msg;
    msg.SetMsgId(EMITTER_GRAB_OR_RELEASE);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(0x00);
    msg.SetPayload(payload);
    GripperService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 松开
void GripperService_Release() {
    Msg msg;
    msg.SetMsgId(EMITTER_GRAB_OR_RELEASE);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(0x01);
    msg.SetPayload(payload);
    GripperService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}