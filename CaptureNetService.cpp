#include "CaptureNetService.h"
#include "common.h"
#include <string>
#include <iostream>
#include <thread>
#include <mstcpip.h>  // 关键头文件
#include <thread>
#pragma comment(lib, "Ws2_32.lib")  // 链接库

// 全局变量
static SOCKET client = INVALID_SOCKET;
static std::string Ip = "192.168.144.25"; // 必须使用std::string，不然C#传过来的ip后半部分会乱码
static int Port = 8519;
static bool IsConned = false;

static uint8_t SEROV_CONTROL = 0x09; // 舵机控制

static WSAEVENT g_networkEvent = WSACreateEvent();

// 清理函数
void CaptureNetService_Cleanup() {
    if (client != INVALID_SOCKET) closesocket(client);
    CleanupWinsock();
}

// 设置IP
void CaptureNetService_SetIp(const char* ip) {
    Ip = std::string(ip);
}

// 断开连接
void CaptureNetService_DisConnected() {
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
                CaptureNetService_DisConnected();
                break;
            }
        }
    }
}

// 连接
bool CaptureNetService_Connection() {
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

    // 注册网络事件监听
    if (WSAEventSelect(client, g_networkEvent, FD_CLOSE) == SOCKET_ERROR) {
        std::cerr << "事件注册失败: " << WSAGetLastError() << std::endl;
        return false;
    }
    std::thread monitor(MonitorConnection);
    monitor.detach();
    return true;
}

// 检查连接状态
bool CaptureNetService_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// 发送数据
void CaptureNetService_SendData(const char* data, int length) {
    if (!CaptureNetService_IsConnected()) return;

    int result = send(client, data, length, 0);
    if (result == SOCKET_ERROR) {
        // 获取错误码并处理
        int error = WSAGetLastError();
        std::cerr << "捕捉网消息发送失败，错误码: " << error << std::endl;
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