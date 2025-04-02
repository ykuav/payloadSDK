#include "EmitterService.h"
#include "common.h"
#include <string>
#include <iostream>
#include <thread>

static int parseIndex = 0;
static std::vector<uint8_t> recvData(128);
static std::vector<uint8_t> recvDataLast(128);

// ȫ�ֱ���
static SOCKET client = INVALID_SOCKET;
static std::string Ip = "192.168.144.26"; // ����ʹ��std::string����ȻC#��������ip��벿�ֻ�����
static int Port = 8519;
static bool IsConned = false;

static uint8_t EMITTER_LAUNCH = 0x11; // �������
static uint8_t EMITTER_STATUS = 0x10; // ��ȡ�����״̬

static std::vector<EmitterStatusCallback> emitterStatusCallbacks;

// ������
void EmitterService_Cleanup() {
    if (client != INVALID_SOCKET) closesocket(client);
    CleanupWinsock();
}

// ����IP
void EmitterService_SetIp(const char* ip) {
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
        std::cerr << EmitterService_IsConnected() << std::endl;
        while (EmitterService_IsConnected())
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
                    for (EmitterStatusCallback emitterStatusCallback : emitterStatusCallbacks) {
                        if (recvDataLast[0] != 0x8D) {
                            continue;
                        }
                        if (recvDataLast[2] == EMITTER_LAUNCH) {
                            emitterStatusCallback(recvDataLast.data(), recvDataLast.size());
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "38mm��������Ϣ���մ���: " << e.what() << std::endl;
        EmitterService_DisConnected();
    }
}

// ����
bool EmitterService_Connection() {
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
void EmitterService_DisConnected() {
    if (!IsConned) return;
    closesocket(client);
    IsConned = false;
}

// �������״̬
bool EmitterService_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// ��������
void EmitterService_SendData(const char* data, int length) {
    if (!EmitterService_IsConnected()) return;
    try {
        send(client, data, length, 0);
    }
    catch (const std::exception& e) {
        std::cerr << "38mm��������Ϣ����ʧ��: " << e.what() << std::endl;
        EmitterService_DisConnected();
    }
}

// ����
void EmitterService_Launch(int index) {
    Msg msg;
    msg.SetMsgId(EMITTER_LAUNCH);
    std::vector<uint8_t> payload(16);
    payload[index] = static_cast<uint8_t>(0x01);
    msg.SetPayload(payload);
    EmitterService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// �����״̬��ѯ
void EmitterService_GetStatus() {
    Msg msg;
    msg.SetMsgId(EMITTER_STATUS);
    std::vector<uint8_t> payload(0);
    msg.SetPayload(payload);
    EmitterService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// �����״̬�ص�����
void EmitterService_RegisterCallback(EmitterStatusCallback callback)
{
    emitterStatusCallbacks.push_back(callback);
}