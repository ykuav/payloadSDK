#include "BucketService.h"
#include "common.h"
#include <string>
#include <iostream>
#include <thread>

static int parseIndex = 0;
static std::vector<uint8_t> recvData(128);
static std::vector<uint8_t> recvDataLast(128);

// ȫ�ֱ���
static SOCKET client = INVALID_SOCKET;
static std::string Ip = "192.168.144.34"; // ����ʹ��std::string����ȻC#��������ip��벿�ֻ�����
static int Port = 8519;
static bool IsConned = false;

static uint8_t BUCKET_STATE_RECEIVE = 0x25; // ����������
static uint8_t BUCKET_STATE_SEND = 0x26; // ����������
static uint8_t BUCKET_BARREL_CONTROL = 0x36; // ˮͰ����
static uint8_t BUCKET_HOOK_CONTROL = 0x38; // �ҹ�����

static std::vector<BucketServiceStateCallback> bucketStateCallbacks;

// ������
void BucketService_Cleanup() {
    if (client != INVALID_SOCKET) closesocket(client);
    CleanupWinsock();
}

// ����IP
void BucketService_SetIp(const char* ip) {
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
        std::cerr << BucketService_IsConnected() << std::endl;
        while (BucketService_IsConnected())
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
                    for (BucketServiceStateCallback BucketStateCallback : bucketStateCallbacks) {
                        if (recvDataLast[0] != 0x8D) {
                            continue;
                        }
                        if (recvDataLast[2] == BUCKET_STATE_RECEIVE) {
                            BucketStateCallback(recvDataLast.data(), recvDataLast.size());
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "��Ͱ��Ϣ���մ���: " << e.what() << std::endl;
        BucketService_DisConnected();
    }
}

// ����
bool BucketService_Connection() {
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
void BucketService_DisConnected() {
    if (!IsConned) return;
    closesocket(client);
    IsConned = false;
}

// �������״̬
bool BucketService_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// ��������
void BucketService_SendData(const char* data, int length) {
    if (!BucketService_IsConnected()) return;
    try {
        send(client, data, length, 0);
    }
    catch (const std::exception& e) {
        std::cerr << "��Ͱ��Ϣ����ʧ��: " << e.what() << std::endl;
        BucketService_DisConnected();
    }
}

// ������Ͱ���أ�0ͣ��1����������2�أ�����
void BucketService_BarrelControl(int controlType) {
    Msg msg;
    msg.SetMsgId(BUCKET_BARREL_CONTROL);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(controlType);
    msg.SetPayload(payload);
    BucketService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// �����ҹ����أ�0�أ�1��
void BucketService_HookControl(int controlType) {
    Msg msg;
    msg.SetMsgId(BUCKET_HOOK_CONTROL);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(controlType);
    msg.SetPayload(payload);
    BucketService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// ����������
void BucketService_Heartbeat() {
    Msg msg;
    msg.SetMsgId(BUCKET_STATE_SEND);
    std::vector<uint8_t> payload(4);
    msg.SetPayload(payload);
    BucketService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// ע���Ͱ״̬�ص�����
void BucketService_RegisterCallback(BucketServiceStateCallback callback)
{
    bucketStateCallbacks.push_back(callback);
}