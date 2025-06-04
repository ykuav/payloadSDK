#include "GripperService.h"
#include "common.h"
#include <string>
#include <iostream>
#include <thread>
#include <mstcpip.h>  // �ؼ�ͷ�ļ�
#include <thread>
#pragma comment(lib, "Ws2_32.lib")  // ���ӿ�

static int parseIndex = 0;
static std::vector<uint8_t> recvData(128);
static std::vector<uint8_t> recvDataLast(128);

// ȫ�ֱ���
static SOCKET client = INVALID_SOCKET;
static std::string Ip = "192.168.144.30"; // ����ʹ��std::string����ȻC#��������ip��벿�ֻ�����
static int Port = 8519;
static bool IsConned = false;
static WSAEVENT g_networkEvent = WSACreateEvent();

static uint8_t EMITTER_GRAB_OR_RELEASE = 0x35;

// ������
void GripperService_Cleanup() {
    if (client != INVALID_SOCKET) closesocket(client);
    CleanupWinsock();
}

// ����IP
void GripperService_SetIp(const char* ip) {
    Ip = std::string(ip);
}

static void resetBuffer() {
    std::cerr << "���recvData: " << std::endl;
    recvData.clear();
    std::cerr << "����recvData: " << std::endl;
    recvData.resize(128, 0); // ��ʼ��128�ֽڻ�����
}

// �Ͽ�����
void GripperService_DisConnected() {
    if (!IsConned) return;
    closesocket(client);
    IsConned = false;
}

// �����̼߳����¼�����û�ж�ʱ����recv��send�����������ʹ�ã��Ա㼰ʱ��ȡ��KeepAlive״̬
static void MonitorConnection() {
    while (IsConned) {
        DWORD ret = WSAWaitForMultipleEvents(1, &g_networkEvent, FALSE, 1000, FALSE);
        if (ret == WSA_WAIT_EVENT_0) {
            WSANETWORKEVENTS events;
            WSAEnumNetworkEvents(client, g_networkEvent, &events);
            if (events.lNetworkEvents & FD_CLOSE) {
                std::cerr << "���ӱ�Զ�˹رգ�KeepAlive ʧЧ��" << std::endl;
                GripperService_DisConnected();
                break;
            }
        }
    }
}

// ����
bool GripperService_Connection() {
    if (IsConned) return true;

    client = connection(Ip, Port);
    if (client == INVALID_SOCKET) {
        IsConned = false;
        return !(client == INVALID_SOCKET);
    }

    IsConned = true;

    // ���� KeepAlive
    int keepAlive = 1;
    setsockopt(client, SOL_SOCKET, SO_KEEPALIVE, (char*)&keepAlive, sizeof(keepAlive));

    // ���ò�����5���޻��ʼ̽��
    DWORD bytesReturned;
    tcp_keepalive keepaliveOpts{ 1, 5000, 500 }; // ����/5�����/0.5����
    WSAIoctl(client, SIO_KEEPALIVE_VALS, &keepaliveOpts, sizeof(keepaliveOpts), NULL, 0, &bytesReturned, NULL, NULL);
    return true;
}

// �������״̬
bool GripperService_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// ��������
void GripperService_SendData(const char* data, int length) {
    if (!GripperService_IsConnected()) return;

    int result = send(client, data, length, 0);
    if (result == SOCKET_ERROR) {
        // ��ȡ�����벢����
        int error = WSAGetLastError();
        std::cerr << "��еצ��Ϣ����ʧ�ܣ�������: " << error << std::endl;
        GripperService_DisConnected();
    }
}

// ץȡ
void GripperService_Grab() {
    Msg msg;
    msg.SetMsgId(EMITTER_GRAB_OR_RELEASE);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(0x00);
    msg.SetPayload(payload);
    GripperService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// �ɿ�
void GripperService_Release() {
    Msg msg;
    msg.SetMsgId(EMITTER_GRAB_OR_RELEASE);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(0x01);
    msg.SetPayload(payload);
    GripperService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}