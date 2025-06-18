#include "ThrowerService.h"
#include "common.h"
#include <string>
#include <iostream>
#include <thread>
#include <mstcpip.h>  // �ؼ�ͷ�ļ�
#pragma comment(lib, "Ws2_32.lib")  // ���ӿ�

static int parseIndex = 0;
static std::vector<uint8_t> recvData(128);
static std::vector<uint8_t> recvDataLast(128);

// ȫ�ֱ���
static SOCKET client = INVALID_SOCKET;
static std::string Ip = "192.168.144.99"; // ����ʹ��std::string����ȻC#��������ip��벿�ֻ�����
static int Port = 8519;
static bool IsConned = false;

static uint8_t THROWER_UPDATE = 0x0A; // ��Ͷ���ӳ���������4�ֽڣ��ֱ��� �ա��ա��ա���
static uint8_t THROWER_CONTROL_ONE = 0x21; // ������������ƣ�4�ֽڣ��ֱ��� ����ţ�0-7�����ر�/������0/1�����ա���
static uint8_t THROWER_CONTROL_ALL = 0x22; // ȫ������������ƣ�4�ֽڣ��ֱ��� �ر�/������0/1�����ա��ա���
static uint8_t THROWER_CHARGING = 0x23; // ���ŵ磬4�ֽڣ��ֱ��� �ŵ�/��磨0/1�����ա��ա���
static uint8_t THROWER_ALLOW_DETONATION = 0x24; // �����𱬣�4�ֽڣ��ֱ��� ȡ��/�����𱬣�0/1�����ա��ա���
static uint8_t THROWER_STATE = 0x25; // ���״̬�����跢�ͣ�ÿ1���Զ��ϱ�һ�Σ�6�ֽڣ��ֱ��� �߶ȡ���״̬�����״̬���¶ȡ���״̬���𱬸߶�
/* �°��޸ģ�0x25����40���ֽ�
* ��8�ֽڣ����״�߶ȡ��𱬸߶ȡ�����������������������������������
* 1�ŵ���8�ֽڣ����𱬳��״̬��0���������𱬣�1�������𱬣�2�����ڳ�磬127��δ���ӣ����߶�״̬��0���߶Ȳ��㣬1���߶��㹻������������������״̬��0���޷�������1������������������������������
* 2�ŵ���8�ֽڣ����𱬳��״̬��0���������𱬣�1�������𱬣�2�����ڳ�磬127��δ���ӣ����߶�״̬��0���߶Ȳ��㣬1���߶��㹻������������������״̬��0���޷�������1������������������������������
* 3�ŵ���8�ֽڣ����𱬳��״̬��0���������𱬣�1�������𱬣�2�����ڳ�磬127��δ���ӣ����߶�״̬��0���߶Ȳ��㣬1���߶��㹻������������������״̬��0���޷�������1������������������������������
* 4�ŵ���8�ֽڣ����𱬳��״̬��0���������𱬣�1�������𱬣�2�����ڳ�磬127��δ���ӣ����߶�״̬��0���߶Ȳ��㣬1���߶��㹻������������������״̬��0���޷�������1������������������������������
* */

static uint8_t THROWER_CONNECT_TEST = 0x26; // ���Ӳ��ԣ�����������ʱ���ͣ�4�ֽڣ��ֱ��� �ա��ա��ա���
static uint8_t THROWER_DETONATE_HEIGHT = 0x27; // �����𱬸߶ȣ�4�ֽڣ��ֱ��� �𱬸߶ȡ��ա��ա���
static uint8_t THROWER_CONTROL_TWO_CENTER = 0x28; // ˫�����������(�м���)��4�ֽڣ��ֱ��� �ر�/������0/1�����ա��ա���
static uint8_t THROWER_CONTROL_TWO_LEFT = 0x29; // ˫�����������(�����1/2)��4�ֽڣ��ֱ��� �ر�/������0/1�����ա��ա���
static uint8_t THROWER_CONTROL_TWO_RIGHT = 0x2A; // ˫�����������(�Ҳ���7/8)��4�ֽڣ��ֱ��� �ر�/������0/1�����ա��ա���
static uint8_t THROWER_CHARGING_AND_ALLOW = 0x31; // ���ŵ��������һ��4�ֽڣ��ֱ��� ���ţ�1-4����״̬��0/1�����ա���

static std::vector<ThrowerStateCallback> throwerStateCallbacks;

// ������
void ThrowerService_Cleanup() {
    if (client != INVALID_SOCKET) closesocket(client);
    CleanupWinsock();
}

// ����IP
void ThrowerService_SetIp(const char* ip) {
    Ip = std::string(ip);
}

static void resetBuffer() {
    std::cerr << "���recvData: " << std::endl;
    recvData.clear();
    std::cerr << "����recvData: " << std::endl;
    recvData.resize(128, 0); // ��ʼ��128�ֽڻ�����
}

static bool parseByte(uint8_t b) {
    switch (parseIndex) {
    case 0: // Header���
        if (b != 0x8D) {
            parseIndex = 0;
            resetBuffer();
            return false;
        }
        recvData[0] = b;
        parseIndex++;
        return false;

    case 1: // Length�ֶ�
        recvData[1] = b;
        parseIndex++;
        return false;

    case 2: // MsgID�ֶ�
        recvData[2] = b;
        parseIndex++;
        return false;

    default: { // ���ݸ���
        const int expectedLength = static_cast<int>(recvData[1]) + 4;
        // ��̬���ݣ������Ҫ��
        if (parseIndex >= recvData.size()) {
            recvData.resize(parseIndex + 16); // ÿ����չ16�ֽ�
        }
        recvData[parseIndex] = b;
        parseIndex++;

        if (parseIndex >= expectedLength) {
            // ������Ч���ݶ�
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

// ���ݽ���
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
                throw std::runtime_error("����ʧ�ܣ�������: " + std::to_string(error));
            }

            if (bytesReceived == 0) {
                continue;
            }
            // ����ÿ���ֽ�
            for (int i = 0; i < bytesReceived; ++i) {
                if (parseByte(recvBuffer[i])) {
                    // ��ӡ��ȡ��������
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
        std::cerr << "��Ͷ��Ϣ���մ���: " << e.what() << std::endl;
        ThrowerService_DisConnected();
    }
}

// ����
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

    // ���� KeepAlive
    int keepAlive = 1;
    setsockopt(client, SOL_SOCKET, SO_KEEPALIVE, (char*)&keepAlive, sizeof(keepAlive));

    // ���ò�����5���޻��ʼ̽��
    DWORD bytesReturned;
    tcp_keepalive keepaliveOpts{ 1, 5000, 500 }; // ����/5�����/0.5����
    WSAIoctl(client, SIO_KEEPALIVE_VALS, &keepaliveOpts, sizeof(keepaliveOpts), NULL, 0, &bytesReturned, NULL, NULL);
    return true;
}

// �Ͽ�����
void ThrowerService_DisConnected() {
    if (!IsConned) return;
    closesocket(client);
    IsConned = false;
}

// �������״̬
bool ThrowerService_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// ��������
void ThrowerService_SendData(const char* data, int length) {
    if (!ThrowerService_IsConnected()) return;

    int result = send(client, data, length, 0);
    if (result == SOCKET_ERROR) {
        // ��ȡ�����벢����
        int error = WSAGetLastError();
        std::cerr << "��Ͷ��Ϣ����ʧ�ܣ�������: " << error << std::endl;
        ThrowerService_DisConnected();
    }
}

// �򿪶����index�Ƕ����ţ�0-5
void ThrowerService_Open(int index) {
    Msg msg;
    msg.SetMsgId(THROWER_CONTROL_ONE);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(index);
    payload[1] = static_cast<uint8_t>(0x01);
    msg.SetPayload(payload);
    ThrowerService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// �����λ���رգ���index�Ƕ����ţ�0-5
void ThrowerService_Close(int index) {
    Msg msg;
    msg.SetMsgId(THROWER_CONTROL_ONE);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(index);
    payload[1] = static_cast<uint8_t>(0x00);
    msg.SetPayload(payload);
    ThrowerService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// ���м��������
void ThrowerService_OpenCenter() {
    Msg msg;
    msg.SetMsgId(THROWER_CONTROL_TWO_CENTER);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(0x01);
    msg.SetPayload(payload);
    ThrowerService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// �ر��м��������
void ThrowerService_CloseCenter() {
    Msg msg;
    msg.SetMsgId(THROWER_CONTROL_TWO_CENTER);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(0x00);
    msg.SetPayload(payload);
    ThrowerService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// ������������
void ThrowerService_OpenLeft() {
    Msg msg;
    msg.SetMsgId(THROWER_CONTROL_TWO_LEFT);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(0x01);
    msg.SetPayload(payload);
    ThrowerService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// �ر�����������
void ThrowerService_CloseLeft() {
    Msg msg;
    msg.SetMsgId(THROWER_CONTROL_TWO_LEFT);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(0x00);
    msg.SetPayload(payload);
    ThrowerService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// ���Ҳ��������
void ThrowerService_OpenRight() {
    Msg msg;
    msg.SetMsgId(THROWER_CONTROL_TWO_RIGHT);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(0x01);
    msg.SetPayload(payload);
    ThrowerService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// �ر��Ҳ��������
void ThrowerService_CloseRight() {
    Msg msg;
    msg.SetMsgId(THROWER_CONTROL_TWO_RIGHT);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(0x00);
    msg.SetPayload(payload);
    ThrowerService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// ��ȫ�����
void ThrowerService_OpenAll() {
    Msg msg;
    msg.SetMsgId(THROWER_CONTROL_ALL);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(0x01);
    msg.SetPayload(payload);
    ThrowerService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// �ر�ȫ�����
void ThrowerService_CloseAll() {
    Msg msg;
    msg.SetMsgId(THROWER_CONTROL_ALL);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(0x00);
    msg.SetPayload(payload);
    ThrowerService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// ���ŵ磬type=true�ǳ�磬type=false�Ƿŵ�(�°�������)
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

// �Ƿ�����������type=true������������type=false�ǽ�ֹ����(�°�������)
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

// �����𱬺Ϳ�ʼ��磨�°�ʹ�ã�
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

// ���������߶ȣ�������䵽ָ���߶Ⱥ���Զ�����
void ThrowerService_SetDetonateHeight(int height) {
    Msg msg;
    msg.SetMsgId(THROWER_DETONATE_HEIGHT);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(height);
    msg.SetPayload(payload);
    ThrowerService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// ����������Ͷ���ӳɹ����趨ʱ����
// ��Ͷ���15s����δ�յ������������Զ�����
void ThrowerService_Heartbeat() {
    Msg msg;
    msg.SetMsgId(THROWER_CONNECT_TEST);
    std::vector<uint8_t> payload(4);
    msg.SetPayload(payload);
    ThrowerService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// ���°��ӳ���
void ThrowerService_Update() {
    Msg msg;
    msg.SetMsgId(THROWER_UPDATE);
    std::vector<uint8_t> payload(4);
    msg.SetPayload(payload);
    ThrowerService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// ע����Ͷ״̬�ص�����
void ThrowerService_RegisterCallback(ThrowerStateCallback callback)
{
    throwerStateCallbacks.push_back(callback);
}