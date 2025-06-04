#include "WaterBranchService.h"
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
static std::string Ip = "192.168.144.35"; // ����ʹ��std::string����ȻC#��������ip��벿�ֻ�����
static int Port = 8519;
static bool IsConned = false;

static uint8_t WATERBRANCH_STATE_RECEIVE = 0x25; // ����������
static uint8_t WATERBRANCH_STATE_SEND = 0x26; // ����������
static uint8_t WATERBRANCH_HOSE_RELEASE = 0x2B; // �ͷ�ˮ��
static uint8_t WATERBRANCH_HOSE_DETACHMENT = 0x2C; // ˮ������

static std::vector<WaterBranchServiceStateCallback> bucketStateCallbacks;

// ������
void WaterBranchService_Cleanup() {
    if (client != INVALID_SOCKET) closesocket(client);
    CleanupWinsock();
}

// ����IP
void WaterBranchService_SetIp(const char* ip) {
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
        std::cerr << WaterBranchService_IsConnected() << std::endl;
        while (WaterBranchService_IsConnected())
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
                    for (WaterBranchServiceStateCallback WaterBranchStateCallback : bucketStateCallbacks) {
                        if (recvDataLast[0] != 0x8D) {
                            continue;
                        }
                        if (recvDataLast[2] == WATERBRANCH_STATE_RECEIVE) {
                            WaterBranchStateCallback(recvDataLast.data(), recvDataLast.size());
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "����ˮǹ��Ϣ���մ���: " << e.what() << std::endl;
        WaterBranchService_DisConnected();
    }
}

// ����
bool WaterBranchService_Connection() {
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
void WaterBranchService_DisConnected() {
    if (!IsConned) return;
    closesocket(client);
    IsConned = false;
}

// �������״̬
bool WaterBranchService_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// ��������
void WaterBranchService_SendData(const char* data, int length) {
    if (!WaterBranchService_IsConnected()) return;

    int result = send(client, data, length, 0);
    if (result == SOCKET_ERROR) {
        // ��ȡ�����벢����
        int error = WSAGetLastError();
        std::cerr << "����ˮǹ��Ϣ����ʧ�ܣ�������: " << error << std::endl;
        WaterBranchService_DisConnected();
    }
}

// �ͷ�ˮ����0�أ�1��
void WaterBranchService_HoseRelease(int operateType) {
    Msg msg;
    msg.SetMsgId(WATERBRANCH_HOSE_RELEASE);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(operateType);
    msg.SetPayload(payload);
    WaterBranchService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// ˮ��������0�أ�1��
void WaterBranchService_HoseDetachment(int operateType) {
    Msg msg;
    msg.SetMsgId(WATERBRANCH_HOSE_DETACHMENT);
    std::vector<uint8_t> payload(4);
    payload[0] = static_cast<uint8_t>(operateType);
    msg.SetPayload(payload);
    WaterBranchService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// ����������
void WaterBranchService_Heartbeat() {
    Msg msg;
    msg.SetMsgId(WATERBRANCH_STATE_SEND);
    std::vector<uint8_t> payload(4);
    msg.SetPayload(payload);
    WaterBranchService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// ע������ˮǹ״̬�ص�����
void WaterBranchService_RegisterCallback(WaterBranchServiceStateCallback callback)
{
    bucketStateCallbacks.push_back(callback);
}