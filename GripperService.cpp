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

static uint8_t GRIPPER_RISE_OR_DECLINE = 0x35;
static uint8_t EMITTER_GRAB_OR_RELEASE = 0x36;
static uint8_t GRIPPER_STOP = 0x37;

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
    try {
        send(client, data, length, 0);
    }
    catch (const std::exception& e) {
        std::cerr << "机械爪消息发送失败: " << e.what() << std::endl;
        GripperService_DisConnected();
    }
}

// 上升
void GripperService_Rise() {
    Msg msg;
    msg.SetMsgId(GRIPPER_RISE_OR_DECLINE);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(0x00);
    msg.SetPayload(payload);
    GripperService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 下降
void GripperService_Decline() {
    Msg msg;
    msg.SetMsgId(GRIPPER_RISE_OR_DECLINE);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(0x01);
    msg.SetPayload(payload);
    GripperService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 抓取
void GripperService_Grab() {
    Msg msg;
    msg.SetMsgId(EMITTER_GRAB_OR_RELEASE);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(0x01);
    msg.SetPayload(payload);
    GripperService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 紧急制动
void GripperService_Stop() {
    Msg msg;
    msg.SetMsgId(GRIPPER_STOP);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(0x01);
    msg.SetPayload(payload);
    GripperService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 松开
void GripperService_Release() {
    GripperService_Stop();
    Sleep(100);
    Msg msg;
    msg.SetMsgId(EMITTER_GRAB_OR_RELEASE);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(0x00);
    msg.SetPayload(payload);
    GripperService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}