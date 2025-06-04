#include "LightService.h"
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
static std::string Ip = "192.168.144.24"; // ����ʹ��std::string����ȻC#��������ip��벿�ֻ�����
static int Port = 8519;
static bool IsConned = false;

static uint8_t OPEN_CLOSE_LIGHT = 0x01; // ���ص�
static uint8_t LUMINANCE_CHANGE = 0x02; // ����
static uint8_t SHARP_FLASH = 0x03;      // ����
static uint8_t FETCH_TEMPERATURE = 0x04;// ��ȡ�¶�
static uint8_t TRIPOD_HEAD = 0x06;      // ��̨����

static std::vector<TemperatureCallback> temperatureCallbacks;

// ������
void LightService_Cleanup() {
    if (client != INVALID_SOCKET) closesocket(client);
    CleanupWinsock();
}

// ����IP
void LightService_SetIp(const char* ip) {
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
        std::cerr << LightService_IsConnected() << std::endl;
        while (LightService_IsConnected())
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
                    for (TemperatureCallback temperatureCallback : temperatureCallbacks) {
                        if (recvDataLast[0] != 0x8D) {
                            continue;
                        }
                        if (recvDataLast[2] == FETCH_TEMPERATURE) {
                            temperatureCallback((int)(recvDataLast[3] & 0xFF) - 50, (int)(recvDataLast[4] & 0xFF) - 50);
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "̽�յ���Ϣ���մ���: " << e.what() << std::endl;
        LightService_DisConnected();
    }
}

// ����
bool LightService_Connection() {
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
void LightService_DisConnected() {
    if (!IsConned) return;
    closesocket(client);
    IsConned = false;
}

// �������״̬
bool LightService_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// ��������
void LightService_SendData(const char* data, int length) {
    if (!LightService_IsConnected()) return;

    int result = send(client, data, length, 0);
    if (result == SOCKET_ERROR) {
        // ��ȡ�����벢����
        int error = WSAGetLastError();
        std::cerr << "̽�յ���Ϣ����ʧ�ܣ�������: " << error << std::endl;
        LightService_DisConnected();
    }
}

// 1 ���ƣ�0 �ص�
void LightService_OpenLight(int open) {
    Msg msg;
    msg.SetMsgId(OPEN_CLOSE_LIGHT);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(open);
    msg.SetPayload(payload);
    LightService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());

    // ����ǹصƣ�ҲҪ�ѱ���ģʽ�ص�����ֹ���ű���ʱ�صƵ����´ο��ƻ����Ǳ���ģʽ
    if (open == 0) {
        Sleep(100);
        LightService_SharpFlash(0);
    }
}

// ���ȵ���
void LightService_LuminanceChange(int lum) {
    Msg msg;
    msg.SetMsgId(LUMINANCE_CHANGE);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(lum);
    msg.SetPayload(payload);
    LightService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 1 ��������0 �ر���
void LightService_SharpFlash(int open) {
    Msg msg;
    msg.SetMsgId(SHARP_FLASH);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(open);
    msg.SetPayload(payload);
    LightService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// ��ȡ�¶�
void LightService_FetchTemperature() {
    Msg msg;
    msg.SetMsgId(FETCH_TEMPERATURE);
    std::vector<uint8_t> payload(0);
    msg.SetPayload(payload);
    LightService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// ��̨���ƣ�pitch ������0 - 12000����roll �������ʱ�������ã���yaw ƫ����0 - 34000��
void LightService_GimbalControl(int16_t pitch, int16_t roll, int16_t yaw) {
    Msg msg;
    msg.SetMsgId(TRIPOD_HEAD);
    std::vector<uint8_t> payload(6); // ��������Ϊ6��vector
    // ʹ�� memcpy �����ݰ�С����д�� vector
    memcpy(payload.data(), &pitch, 2);      // ���� pitch �� [0, 1]
    memcpy(payload.data() + 2, &roll, 2);   // ���� roll �� [2, 3]
    memcpy(payload.data() + 4, &yaw, 2);    // ���� yaw �� [4, 5]
    msg.SetPayload(payload);
    LightService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// ע���¶Ȼ�ȡ�ص�����
void LightService_RegisterCallback(TemperatureCallback callback)
{
    temperatureCallbacks.push_back(callback);
}