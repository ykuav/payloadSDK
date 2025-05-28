#include "GripperService.h"
#include "common.h"
#include <string>
#include <iostream>
#include <thread>
#include <mstcpip.h>  // 关键头文件
#include <thread>
#pragma comment(lib, "Ws2_32.lib")  // 链接库

static int parseIndex = 0;
static std::vector<uint8_t> recvData(128);
static std::vector<uint8_t> recvDataLast(128);

// 全局变量
static SOCKET client = INVALID_SOCKET;
static std::string Ip = "192.168.144.30"; // 必须使用std::string，不然C#传过来的ip后半部分会乱码
static int Port = 8519;
static bool IsConned = false;
static WSAEVENT g_networkEvent = WSACreateEvent();

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

// 断开连接
void GripperService_DisConnected() {
    if (!IsConned) return;
    closesocket(client);
    IsConned = false;
}

// 独立线程监听事件，在没有定时调用recv和send方法的情况下使用，以便及时获取到KeepAlive状态
static void MonitorConnection() {
    while (IsConned) {
        DWORD ret = WSAWaitForMultipleEvents(1, &g_networkEvent, FALSE, 1000, FALSE);
        if (ret == WSA_WAIT_EVENT_0) {
            WSANETWORKEVENTS events;
            WSAEnumNetworkEvents(client, g_networkEvent, &events);
            if (events.lNetworkEvents & FD_CLOSE) {
                std::cerr << "连接被远端关闭（KeepAlive 失效）" << std::endl;
                GripperService_DisConnected();
                break;
            }
        }
    }
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

    // 启用 KeepAlive
    int keepAlive = 1;
    setsockopt(client, SOL_SOCKET, SO_KEEPALIVE, (char*)&keepAlive, sizeof(keepAlive));

    // 设置参数：5秒无活动开始探测
    DWORD bytesReturned;
    tcp_keepalive keepaliveOpts{ 1, 5000, 500 }; // 开启/5秒空闲/0.5秒间隔
    WSAIoctl(client, SIO_KEEPALIVE_VALS, &keepaliveOpts, sizeof(keepaliveOpts), NULL, 0, &bytesReturned, NULL, NULL);
    return true;
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