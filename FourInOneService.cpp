#include "FourInOneService.h"
#include "common.h"
#include <iostream>
#include <thread>

// 全局变量
static SOCKET client = INVALID_SOCKET;
static SOCKET servoControlClient = INVALID_SOCKET;
static std::string Ip = "192.168.144.27"; // 必须使用std::string，不然C#传过来的ip后半部分会乱码
static int Port = 8519;
static bool IsConned = false;

static uint8_t OPEN_CLOSE_LIGHT = 0x01; // 开关灯
static uint8_t LUMINANCE_CHANGE = 0x02; // 亮度
static uint8_t SHARP_FLASH = 0x03;      // 爆闪
static uint8_t FETCH_TEMPERATURE = 0x04;// 获取温度
static uint8_t RED_BLUE_FLASHES = 0x07; // 红蓝
static uint8_t SEROV_CONTROL = 0x09;	// 照明俯仰控制

static std::vector<RadioCallback> radioCallbacks; // 收音回调函数

// 清理函数
void FourInOneService_Cleanup() {
    if (client != INVALID_SOCKET) closesocket(client);
    if (servoControlClient != INVALID_SOCKET) closesocket(servoControlClient);
    CleanupWinsock();
}

// 设置IP
void FourInOneService_SetIp(const char* ip) {
    Ip = std::string(ip);
}

// 数据接收
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
                throw std::runtime_error("接收失败，错误码: " + std::to_string(error));
            }

            if (bytesReceived == 0) {
                continue;
            }
            std::vector<uint8_t> tmp;
            // 收音信息以“[40]”开头
            uint8_t radioTarget[4] = { '[', '4', '0', ']' };
            for (int i = 0; i < bytesReceived; ++i) {
                if (tmp.empty()) {
                    if (recvBuffer[i] == '[') {
                        tmp.push_back(recvBuffer[i]);
                    }
                }
                else {
                    // 当遇到'['且tmp不为空时，可能是两个报文连到一块了，先触发回调函数，再清空tmp
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
                    std::cerr << "四合一接收到消息：" << std::endl;
                    printHex(tmp);
                    radioCallback(tmp.data(), tmp.size());
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "四合一消息接收错误: " << e.what() << std::endl;
        FourInOneService_DisConnected();
    }
}

// 连接
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

// 断开连接
void FourInOneService_DisConnected() {
    if (!IsConned) return;
    closesocket(client);
    IsConned = false;
}

// 检查连接状态
bool FourInOneService_IsConnected() {
    return IsConned && client != INVALID_SOCKET;
}

// 发送数据
void FourInOneService_SendData(const char* data, int length) {
    if (!FourInOneService_IsConnected()) return;
    try {
        send(client, data, length, 0);
    }
    catch (const std::exception& e) {
        std::cerr << "四合一消息发送失败: " << e.what() << std::endl;
        FourInOneService_DisConnected();
    }
}

// 实时喊话
void FourInOneService_RealTimeShout(uint8_t* data, int length) {
    std::string realTimeShout = "[42]";
    std::string combinedData = realTimeShout + reinterpret_cast<const char*>(data);
    FourInOneService_SendData(combinedData.c_str(), combinedData.length());
}

// 设置音量
void FourInOneService_SetVolumn(int volumn) {
    std::string setVolume = "[14]";
    std::string combinedData = setVolume + std::to_string(volumn);
    FourInOneService_SendData(combinedData.c_str(), combinedData.length());
}

// 播放TTS
void FourInOneService_PlayTTS(int voiceType, int loop, const char* text) {
    std::string ttsPlay = (loop == 0) ? "[31]" : "[32]";
    std::string combinedData = ttsPlay + std::to_string(voiceType) + std::string(text);
    FourInOneService_SendData(combinedData.c_str(), combinedData.length());
}

// 停止TTS
void FourInOneService_StopTTS() {
    std::string stopTTS = "[17]";
    FourInOneService_SendData(stopTTS.c_str(), stopTTS.length());
}

// 播放音频
void FourInOneService_PlayAudio(const char* audioName, int loop) {
    std::string audioPlay = "[12]" + std::to_string(loop) + std::string(audioName);
    FourInOneService_SendData(audioPlay.c_str(), audioPlay.length());
}

// 停止播放音频
void FourInOneService_StopPlayAudio() {
    std::string stopAudioPlay = "[13]";
    FourInOneService_SendData(stopAudioPlay.c_str(), stopAudioPlay.length());
}

// 播放警报
void FourInOneService_PlayAlarm() {
    FourInOneService_StopPlayAlarm();
    std::string alarmPlay = "[18]";
    FourInOneService_SendData(alarmPlay.c_str(), alarmPlay.length());
}

// 停止播放警报
void FourInOneService_StopPlayAlarm() {
    std::string stopAlarmPlay = "[19]";
    FourInOneService_SendData(stopAlarmPlay.c_str(), stopAlarmPlay.length());
}

// 俯仰控制
void FourInOneService_PitchControl(unsigned int pitch) {
    if (servoControlClient == INVALID_SOCKET) {
        // 连接喊话器俯仰控制的舵机
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
        std::cerr << "喊话器俯仰控制失败: " << ex.what() << std::endl;
    }
}

// 开始收音
void FourInOneService_StartRadio() {
    std::string startRadio = "[40]";
    FourInOneService_SendData(startRadio.c_str(), startRadio.length());
}

// 结束收音
void FourInOneService_StopRadio() {
    std::string stopRadio = "[41]";
    FourInOneService_SendData(stopRadio.c_str(), stopRadio.length());
}

// 1 开灯，0 关灯
void FourInOneService_OpenLight(int open) {
    Msg msg;
    msg.SetMsgId(OPEN_CLOSE_LIGHT);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(open);
    msg.SetPayload(payload);
    FourInOneService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
    // 如果是关灯，也要把爆闪模式关掉，防止开着爆闪时关灯导致下次开灯还会是爆闪模式
    if (open == 0) {
        Sleep(100);
        FourInOneService_SharpFlash(0);
    }
}

// 亮度调整
void FourInOneService_LuminanceChange(int lum) {
    Msg msg;
    msg.SetMsgId(LUMINANCE_CHANGE);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(lum);
    msg.SetPayload(payload);
    FourInOneService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 1 开爆闪，0 关爆闪
void FourInOneService_SharpFlash(int open) {
    Msg msg;
    msg.SetMsgId(SHARP_FLASH);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(open);
    msg.SetPayload(payload);
    FourInOneService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 俯仰控制, 0-100，实际传输的时候是100 - 200
void FourInOneService_ControlServo(int pitch) {
    Msg msg;
    msg.SetMsgId(SEROV_CONTROL);
    std::vector<uint8_t> payload(2);
    payload[0] = 0xFF;
    payload[1] = static_cast<uint8_t>(pitch + 100);
    msg.SetPayload(payload);
    FourInOneService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 红蓝控制，model 0 关，1-16都是开，是16种红蓝模式
void FourInOneService_RedBlueLedControl(int model) {
    Msg msg;
    msg.SetMsgId(RED_BLUE_FLASHES);
    std::vector<uint8_t> payload(1);
    payload[0] = static_cast<uint8_t>(model);
    msg.SetPayload(payload);
    FourInOneService_SendData(reinterpret_cast<const char*>(msg.GetMsg().data()), msg.length());
}

// 注册收音回调函数
void FourInOneService_RegisterCallback(RadioCallback callback)
{
    radioCallbacks.push_back(callback);
}