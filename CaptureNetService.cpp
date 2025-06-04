#include "CaptureNetService.h"
#include "common.h"
#include <string>
#include <iostream>
#include <thread>
#include <mstcpip.h>  // �ؼ�ͷ�ļ�
#include <thread>
#pragma comment(lib, "Ws2_32.lib")  // ���ӿ�

// ȫ�ֱ���
static SOCKET client = INVALID_SOCKET;
static std::string Ip = "192.168.144.25"; // ����ʹ��std::string����ȻC#��������ip��벿�ֻ�����
static int Port = 8519;
static bool IsConned = false;

static uint8_t SEROV_CONTROL = 0x09; // �������

static WSAEVENT g_networkEvent = WSACreateEvent();

// ������
void CaptureNetService_Cleanup() {
    if (client != INVALID_SOCKET) closesocket(client);
    CleanupWinsock();
}

// ����IP
void CaptureNetService_SetIp(const char* ip) {
    Ip = std::string(ip);
}

// �Ͽ�����
void CaptureNetService_DisConnected() {
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
                CaptureNetService_DisConnected();
                break;
            }
        }
    }
}

// ����
bool CaptureNetService_Connection() {
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

    // ע�������¼�����
    if (WSAEventSelect(client, g_networkEvent, FD_CLOSE) == SOCKET_ERROR) {
        std::cerr << "�¼�ע��ʧ��: " << WSAGetLastError() << std::endl;
        return false;
    }
    std::thread monitor(MonitorConnection);
    monitor.detach();
    return true;
}

// �������״̬
bool CaptureNetService_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// ��������
void CaptureNetService_SendData(const char* data, int length) {
    if (!CaptureNetService_IsConnected()) return;

    int result = send(client, data, length, 0);
    if (result == SOCKET_ERROR) {
        // ��ȡ�����벢����
        int error = WSAGetLastError();
        std::cerr << "��׽����Ϣ����ʧ�ܣ�������: " << error << std::endl;
        CaptureNetService_DisConnected();
    }
}

// ����
void CaptureNetService_Launch() {
    Msg msg;
    msg.SetMsgId(SEROV_CONTROL);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(0x01);
    msg.SetPayload(payload);
    CaptureNetService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}