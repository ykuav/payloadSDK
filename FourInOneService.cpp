#include "FourInOneService.h"
#include "common.h"
#include <iostream>
#include <thread>

// ȫ�ֱ���
static SOCKET client = INVALID_SOCKET;
static SOCKET servoControlClient = INVALID_SOCKET;
static std::string Ip = "192.168.144.27"; // ����ʹ��std::string����ȻC#��������ip��벿�ֻ�����
static int Port = 8519;
static bool IsConned = false;

static uint8_t OPEN_CLOSE_LIGHT = 0x01; // ���ص�
static uint8_t LUMINANCE_CHANGE = 0x02; // ����
static uint8_t SHARP_FLASH = 0x03;      // ����
static uint8_t FETCH_TEMPERATURE = 0x04;// ��ȡ�¶�
static uint8_t RED_BLUE_FLASHES = 0x07; // ����
static uint8_t SEROV_CONTROL = 0x09;	// ������������

static std::vector<RadioCallback> radioCallbacks; // �����ص�����

// ������
void FourInOneService_Cleanup() {
    if (client != INVALID_SOCKET) closesocket(client);
    if (servoControlClient != INVALID_SOCKET) closesocket(servoControlClient);
    CleanupWinsock();
}

// ����IP
void FourInOneService_SetIp(const char* ip) {
    Ip = std::string(ip);
}

// ���ݽ���
static void dataReceive() {
    try {
        while (FourInOneService_IsConnected())
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
            std::vector<uint8_t> tmp;
            // ������Ϣ�ԡ�[40]����ͷ
            uint8_t radioTarget[4] = { '[', '4', '0', ']' };
            for (int i = 0; i < bytesReceived; ++i) {
                if (tmp.empty()) {
                    if (recvBuffer[i] == '[') {
                        tmp.push_back(recvBuffer[i]);
                    }
                }
                else {
                    // ������'['��tmp��Ϊ��ʱ��������������������һ���ˣ��ȴ����ص������������tmp
                    if (recvBuffer[i] == '[') {
                        if (std::equal(std::begin(tmp), std::begin(tmp) + 4, std::begin(radioTarget))) {
                            for (RadioCallback radioCallback : radioCallbacks) {
                                radioCallback(tmp.data(), tmp.size());
                            }
                        }
                        tmp.clear();
                    }
                    tmp.push_back(recvBuffer[i]);
                }
            }
            if (!tmp.empty() && std::equal(std::begin(tmp), std::begin(tmp) + 4, std::begin(radioTarget))) {
                for (RadioCallback radioCallback : radioCallbacks) {
                    std::cerr << "�ĺ�һ���յ���Ϣ��" << std::endl;
                    printHex(tmp);
                    radioCallback(tmp.data(), tmp.size());
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "�ĺ�һ��Ϣ���մ���: " << e.what() << std::endl;
        FourInOneService_DisConnected();
    }
}

// ����
bool FourInOneService_Connection() {
    if (IsConned) return true;

    client = connection(Ip, Port);
    if (client == INVALID_SOCKET) {
        IsConned = false;
        return !(client == INVALID_SOCKET);
    }

    IsConned = true;
    FourInOneService_SetVolumn(50);
    std::thread t(dataReceive);
    t.detach();
    return true;
}

// �Ͽ�����
void FourInOneService_DisConnected() {
    if (!IsConned) return;
    closesocket(client);
    IsConned = false;
}

// �������״̬
bool FourInOneService_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// ��������
void FourInOneService_SendData(const char* data, int length) {
    if (!FourInOneService_IsConnected()) return;
    try {
        send(client, data, length, 0);
    }
    catch (const std::exception& e) {
        std::cerr << "�ĺ�һ��Ϣ����ʧ��: " << e.what() << std::endl;
        FourInOneService_DisConnected();
    }
}

// ʵʱ����
void FourInOneService_RealTimeShout(uint8_t* data, int length) {
    std::string realTimeShout = "[42]";
    std::string combinedData = realTimeShout + reinterpret_cast<const char*>(data);
    FourInOneService_SendData(combinedData.c_str(), combinedData.length());
}

// ��������
void FourInOneService_SetVolumn(int volumn) {
    std::string setVolume = "[14]";
    std::string combinedData = setVolume + std::to_string(volumn);
    FourInOneService_SendData(combinedData.c_str(), combinedData.length());
}

// ����TTS
void FourInOneService_PlayTTS(int voiceType, int loop, const char* text) {
    std::string ttsPlay = (loop == 0) ? "[31]" : "[32]";
    std::string combinedData = ttsPlay + std::to_string(voiceType) + std::string(text);
    FourInOneService_SendData(combinedData.c_str(), combinedData.length());
}

// ֹͣTTS
void FourInOneService_StopTTS() {
    std::string stopTTS = "[17]";
    FourInOneService_SendData(stopTTS.c_str(), stopTTS.length());
}

// ������Ƶ
void FourInOneService_PlayAudio(const char* audioName, int loop) {
    std::string audioPlay = "[12]" + std::to_string(loop) + std::string(audioName);
    FourInOneService_SendData(audioPlay.c_str(), audioPlay.length());
}

// ֹͣ������Ƶ
void FourInOneService_StopPlayAudio() {
    std::string stopAudioPlay = "[13]";
    FourInOneService_SendData(stopAudioPlay.c_str(), stopAudioPlay.length());
}

// ���ž���
void FourInOneService_PlayAlarm() {
    FourInOneService_StopPlayAlarm();
    std::string alarmPlay = "[18]";
    FourInOneService_SendData(alarmPlay.c_str(), alarmPlay.length());
}

// ֹͣ���ž���
void FourInOneService_StopPlayAlarm() {
    std::string stopAlarmPlay = "[19]";
    FourInOneService_SendData(stopAlarmPlay.c_str(), stopAlarmPlay.length());
}

// ��������
void FourInOneService_PitchControl(unsigned int pitch) {
    if (servoControlClient == INVALID_SOCKET) {
        // ���Ӻ������������ƵĶ��
        servoControlClient = connection(Ip, 12345);
        if (servoControlClient == INVALID_SOCKET) {
            return;
        }
    }

    unsigned int dc = pitch + 80u;
    unsigned int data = (dc < 80u) ? 80u : (dc > 220u) ? 220u : dc;

    unsigned char sendData[2] = { 0x8d, (unsigned char)data };
    try {
        send(servoControlClient, (const char*)sendData, 2, 0);
    }
    catch (const std::exception& ex) {
        std::cerr << "��������������ʧ��: " << ex.what() << std::endl;
    }
}

// ��ʼ����
void FourInOneService_StartRadio() {
    std::string startRadio = "[40]";
    FourInOneService_SendData(startRadio.c_str(), startRadio.length());
}

// ��������
void FourInOneService_StopRadio() {
    std::string stopRadio = "[41]";
    FourInOneService_SendData(stopRadio.c_str(), stopRadio.length());
}

// 1 ���ƣ�0 �ص�
void FourInOneService_OpenLight(int open) {
    Msg msg;
    msg.SetMsgId(OPEN_CLOSE_LIGHT);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(open);
    msg.SetPayload(payload);
    FourInOneService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
    // ����ǹصƣ�ҲҪ�ѱ���ģʽ�ص�����ֹ���ű���ʱ�صƵ����´ο��ƻ����Ǳ���ģʽ
    if (open == 0) {
        Sleep(100);
        FourInOneService_SharpFlash(0);
    }
}

// ���ȵ���
void FourInOneService_LuminanceChange(int lum) {
    Msg msg;
    msg.SetMsgId(LUMINANCE_CHANGE);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(lum);
    msg.SetPayload(payload);
    FourInOneService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 1 ��������0 �ر���
void FourInOneService_SharpFlash(int open) {
    Msg msg;
    msg.SetMsgId(SHARP_FLASH);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(open);
    msg.SetPayload(payload);
    FourInOneService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// ��������, 0-100��ʵ�ʴ����ʱ����100 - 200
void FourInOneService_ControlServo(int pitch) {
    Msg msg;
    msg.SetMsgId(SEROV_CONTROL);
    std::vector<uint8_t> payload(2);
    payload[0] = 0xFF;
    payload[1] = static_cast<uint8_t>(pitch + 100);
    msg.SetPayload(payload);
    FourInOneService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// �������ƣ�model 0 �أ�1-16���ǿ�����16�ֺ���ģʽ
void FourInOneService_RedBlueLedControl(int model) {
    Msg msg;
    msg.SetMsgId(RED_BLUE_FLASHES);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(model);
    msg.SetPayload(payload);
    FourInOneService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// ע�������ص�����
void FourInOneService_RegisterCallback(RadioCallback callback)
{
    radioCallbacks.push_back(callback);
}