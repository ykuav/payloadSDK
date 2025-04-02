#include "SlowDescentDeviceService.h"
#include "common.h"
#include <string>
#include <iostream>
#include <thread>

static int parseIndex = 0;
static std::vector<uint8_t> recvData(128);
static std::vector<uint8_t> recvDataLast(128);

// ȫ�ֱ���
static SOCKET client = INVALID_SOCKET;
static std::string Ip = "192.168.144.29"; // ����ʹ��std::string����ȻC#��������ip��벿�ֻ�����
static int Port = 8519;
static bool IsConned = false;

/* ������ʹ�ܿ���
* ����1���ֽ�
* byte0: 0: Disable��������1: Enable������
* */
static uint8_t DESCENT_CONTROL = 0x12;

/* ��������������
* ����1���ֽ�
* byte0: 0: �������״̬��1: ����ͣ����2: �����۶�
* */
static uint8_t DESCENT_URGENT_CONTROL = 0x13;

/* ��������������
* ����3���ֽ�
* byte0: 0: ���ȿ���ģʽ(0 ~ 300����)��1: �ٶȿ���ģʽ(-20 ~ +20��/����)��ʵ�ʴ�0~40��0��-20��/���ӣ�40��20��/���ӣ�
* byte1: Ŀ�곤�Ȼ��ٶȸ�8λ
* byte2: Ŀ�곤�Ȼ��ٶȵ�8λ
* */
static uint8_t DESCENT_ACTION_CONTROL = 0x14;

/* ������״̬����
* ���跢�ͣ�����8���ֽ�
* byte0: 0����������Disable��1: ��������Enable
* byte1: 0: ���ȿ���ģʽ��1: �ٶȿ���ģʽ
* byte2: ��ǰ�ٶ�m/min
* byte3: ���ͷų��ȸ�8λ
* byte4: ���ͷų��ȵ�8λ
* byte5: 0: ������δ����λ��1: �������ѵ�����2: �������ѵ���
* byte6: ���� kg/LSB
* byte7:
*   0: �ѽ������״̬
*   1: ����ɲ��
*   2: �����۶�
*   3: ����ɲ��+�۶�
* */
static uint8_t DESCENT_STATE_GET = 0x15;

static std::vector<SlowDescentDeviceCallback> slowDescentDeviceCallbacks;

// ������
void SlowDescentDeviceService_Cleanup() {
    if (client != INVALID_SOCKET) closesocket(client);
    CleanupWinsock();
}

// ����IP
void SlowDescentDeviceService_SetIp(const char* ip) {
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
        std::cerr << SlowDescentDeviceService_IsConnected() << std::endl;
        while (SlowDescentDeviceService_IsConnected())
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
                    for (SlowDescentDeviceCallback slowDescentDeviceCallback : slowDescentDeviceCallbacks) {
                        if (recvDataLast[0] != 0x8D) {
                            continue;
                        }
                        if (recvDataLast[2] == DESCENT_STATE_GET) {
                            slowDescentDeviceCallback(recvDataLast.data(), recvDataLast.size());
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "��������Ϣ���մ���: " << e.what() << std::endl;
        SlowDescentDeviceService_DisConnected();
    }
}

// ����
bool SlowDescentDeviceService_Connection() {
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
void SlowDescentDeviceService_DisConnected() {
    if (!IsConned) return;
    closesocket(client);
    IsConned = false;
}

// �������״̬
bool SlowDescentDeviceService_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// ��������
void SlowDescentDeviceService_SendData(const char* data, int length) {
    if (!SlowDescentDeviceService_IsConnected()) return;
    try {
        send(client, data, length, 0);
    }
    catch (const std::exception& e) {
        std::cerr << "��������Ϣ����ʧ��: " << e.what() << std::endl;
        SlowDescentDeviceService_DisConnected();
    }
}

// ���û������Ƿ���ã�true���ã�false������
void SlowDescentDeviceService_Enable(BOOL flag) {
    Msg msg;
    msg.SetMsgId(DESCENT_CONTROL);
    std::vector<uint8_t> payload(1);
    // Enable������
    if (flag) {
        payload[0] = static_cast<uint8_t>(0x01);
    }
    // Disable������
    else {
        payload[0] = static_cast<uint8_t>(0x00);
    }
    msg.SetPayload(payload);
    SlowDescentDeviceService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// ��������������
void SlowDescentDeviceService_EmergencyControl(int command) {
    Msg msg;
    msg.SetMsgId(DESCENT_URGENT_CONTROL);
    std::vector<uint8_t> payload(1);
    switch (command) {
        // �������״̬���ڷ����˽���ͣ��������۶������ͻȻ��ȡ��ͣ�����۶ϵ�ʱ��ʹ�ã�
        case 0:
            payload[0] = static_cast<uint8_t>(0x00);
            break;
        // ����ͣ���������ƶ�
        case 1:
            payload[0] = static_cast<uint8_t>(0x01);
            break;
        // �����۶�
        case 2:
            payload[0] = static_cast<uint8_t>(0x02);
            break;
    }
    msg.SetPayload(payload);
    SlowDescentDeviceService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

/* ��������������
* mode: 0�������ȣ�1�����ٶ�
* speedOrLength: �ٶȻ򳤶�
* */
void SlowDescentDeviceService_actionControl(int mode, int speedOrLength) {
    Msg msg;
    msg.SetMsgId(DESCENT_ACTION_CONTROL);
    std::vector<uint8_t> payload(3);
    payload[0] = static_cast<uint8_t>(mode);
    // �ٶȴ�ֵ��0~40
    // ���ȴ�ֵ��0~300
    payload[1] = static_cast<uint8_t>(speedOrLength / 256);
    payload[2] = static_cast<uint8_t>(speedOrLength % 256);
    msg.SetPayload(payload);
    SlowDescentDeviceService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// ע����Ͷ״̬�ص�����
void SlowDescentDeviceService_RegisterCallback(SlowDescentDeviceCallback callback)
{
    slowDescentDeviceCallbacks.push_back(callback);
}