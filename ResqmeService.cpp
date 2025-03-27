#include "ResqmeService.h"
#include "common.h"
#include <string>
#include <iostream>
#include <thread>

static int parseIndex = 0;
static std::vector<uint8_t> recvData(128);
static std::vector<uint8_t> recvDataLast(128);

// ȫ�ֱ���
static SOCKET client = INVALID_SOCKET;
static std::string Ip = "192.168.144.31"; // ����ʹ��std::string����ȻC#��������ip��벿�ֻ�����
static int Port = 8519;
static bool IsConned = false;

static uint8_t RESQME_STATUS = 0x31;        // ״̬
static uint8_t RESQME_LUNCH = 0x32;         // ����
static uint8_t RESQME_SAFETY_SWITCH = 0x33; // ��ȫ����

static std::vector<ResqmeStateCallback> resqmeStateCallbacks;

// ������
void ResqmeService_Cleanup() {
    if (client != INVALID_SOCKET) closesocket(client);
    CleanupWinsock();
}

// ����IP
void ResqmeService_SetIp(const char* ip) {
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
        std::cerr << ResqmeService_IsConnected() << std::endl;
        while (ResqmeService_IsConnected())
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
                    for (ResqmeStateCallback resqmeStateCallback : resqmeStateCallbacks) {
                        if (recvDataLast[0] != 0x8D) {
                            continue;
                        }
                        if (recvDataLast[2] == RESQME_STATUS) {
                            resqmeStateCallback(recvDataLast.data(), recvDataLast.size());
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "�ƴ�����Ϣ���մ���: " << e.what() << std::endl;
        ResqmeService_DisConnected();
    }
}

// ����
bool ResqmeService_Connection() {
    if (IsConned) return true;

    client = connection(Ip, Port);
    if (client == INVALID_SOCKET) {
        IsConned = false;
        return false;
    }

    IsConned = true;
    std::thread t(dataReceive);
    t.detach();
    return true;
}

// �Ͽ�����
void ResqmeService_DisConnected() {
    if (!IsConned) return;
    closesocket(client);
    IsConned = false;
}

// �������״̬
bool ResqmeService_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// ��������
void ResqmeService_SendData(const char* data, int length) {
    if (!ResqmeService_IsConnected()) return;
    try {
        send(client, data, length, 0);
    }
    catch (const std::exception& e) {
        std::cerr << "�ƴ�����Ϣ����ʧ��: " << e.what() << std::endl;
        ResqmeService_DisConnected();
    }
}

// ���䣬index: 1��1�ſڣ�2��2�ſڣ�3��ȫ��
void ResqmeService_Launch(int index) {
    Msg msg;
    msg.SetMsgId(RESQME_LUNCH);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(index);
    payload[1] = static_cast<uint8_t>(0x01);
    msg.SetPayload(payload);
    ResqmeService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// ��ȫ���أ�state=trueʱ��
void ResqmeService_SafetySwitch(BOOL state) {
    Msg msg;
    msg.SetMsgId(RESQME_SAFETY_SWITCH);
    std::vector<uint8_t> payload(4);
    if (state) {
        payload[1] = static_cast<uint8_t>(0x01);
    }
    else {
        payload[1] = static_cast<uint8_t>(0x00);
    }
    msg.SetPayload(payload);
    ResqmeService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// ע���ƴ��������״̬�ص�����
void ResqmeService_RegisterCallback(ResqmeStateCallback callback)
{
    resqmeStateCallbacks.push_back(callback);
}