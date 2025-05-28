#include "WaterGunService.h"
#include "common.h"
#include <string>
#include <iostream>
#include <thread>

static int parseIndex = 0;
static std::vector<uint8_t> recvData(128);
static std::vector<uint8_t> recvDataLast(128);

// ȫ�ֱ���
static SOCKET client = INVALID_SOCKET;
static std::string Ip = "192.168.144.33"; // ����ʹ��std::string����ȻC#��������ip��벿�ֻ�����
static int Port = 8519;
static bool IsConned = false;

static uint8_t WATERGUN_TOLEFT = 0x33; // ����
static uint8_t WATERGUN_TORIGHT = 0x34; // ����
static uint8_t WATERGUN_MODESWITCH = 0x35;       // ģʽ�л�
static uint8_t WATERGUN_STATE_SEND = 0x26;       // ����������
static uint8_t WATERGUN_STATE_RECEIVE = 0x25;    // ����������

static std::vector<WaterGunServiceStateCallback> waterGunStateCallbacks;

// ������
void WaterGunService_Cleanup() {
    if (client != INVALID_SOCKET) closesocket(client);
    CleanupWinsock();
}

// ����IP
void WaterGunService_SetIp(const char* ip) {
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
        std::cerr << WaterGunService_IsConnected() << std::endl;
        while (WaterGunService_IsConnected())
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
                    for (WaterGunServiceStateCallback waterGunStateCallback : waterGunStateCallbacks) {
                        if (recvDataLast[0] != 0x8D) {
                            continue;
                        }
                        if (recvDataLast[2] == WATERGUN_STATE_RECEIVE) {
                            waterGunStateCallback(recvDataLast[3], recvDataLast[4]);
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "ˮǹ��Ϣ���մ���: " << e.what() << std::endl;
        WaterGunService_DisConnected();
    }
}

// ����
bool WaterGunService_Connection() {
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

// �Ͽ�����
void WaterGunService_DisConnected() {
    if (!IsConned) return;
    closesocket(client);
    IsConned = false;
}

// �������״̬
bool WaterGunService_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// ��������
void WaterGunService_SendData(const char* data, int length) {
    if (!WaterGunService_IsConnected()) return;
    try {
        send(client, data, length, 0);
    }
    catch (const std::exception& e) {
        std::cerr << "ˮǹ��Ϣ����ʧ��: " << e.what() << std::endl;
        WaterGunService_DisConnected();
    }
}

// ˮǹ����
void WaterGunService_ToLeft() {
    Msg msg;
    msg.SetMsgId(WATERGUN_TOLEFT);
    std::vector<uint8_t> payload(4);
    msg.SetPayload(payload);
    WaterGunService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// ˮǹ����
void WaterGunService_ToRight() {
    Msg msg;
    msg.SetMsgId(WATERGUN_TORIGHT);
    std::vector<uint8_t> payload(4);
    msg.SetPayload(payload);
    WaterGunService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

/* ����ˮǹģʽ�л�
* ״̬Ϊ0ʱ��������
* ״̬Ϊ1ʱ�����͸���������Զ�ģʽ����ˮǹ�Զ�����ת��״̬���Ϊ2
* ״̬Ϊ2ʱ�����͸����ˮǹֹͣ����ת����״̬��Ϊ3��ˮǹ�����ع��е㣬Ȼ���Զ���Ϊ�ֶ�ģʽ��״̬��Ϊ4
* ״̬Ϊ4ʱ�����͸����״̬��Ϊ5����״̬1һ��
* */
void WaterGunService_ModeSwitch() {
    Msg msg;
    msg.SetMsgId(WATERGUN_MODESWITCH);
    std::vector<uint8_t> payload(4);
    msg.SetPayload(payload);
    WaterGunService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// ����������
void WaterGunService_Heartbeat() {
    Msg msg;
    msg.SetMsgId(WATERGUN_STATE_SEND);
    std::vector<uint8_t> payload(4);
    msg.SetPayload(payload);
    WaterGunService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// ע������ˮǹ״̬�ص�����
void WaterGunService_RegisterCallback(WaterGunServiceStateCallback callback)
{
    waterGunStateCallbacks.push_back(callback);
}