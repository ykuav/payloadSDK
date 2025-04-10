#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
#include <vector>
// 四合一
#ifdef FOURINONESERVICEDLL_EXPORTS
#define FOURINONESERVICE_API __declspec(dllexport)
#else
#define FOURINONESERVICE_API __declspec(dllimport)
#endif

// 回调函数类型定义
typedef void(__stdcall* RadioCallback)(uint8_t* data, int length);

extern "C" {
    // 初始化函数
    FOURINONESERVICE_API void FourInOneService_Cleanup();

    // IP设置
    FOURINONESERVICE_API void FourInOneService_SetIp(const char* ip);

    // 连接与断开
    FOURINONESERVICE_API bool FourInOneService_Connection();
    FOURINONESERVICE_API void FourInOneService_DisConnected();
    FOURINONESERVICE_API bool FourInOneService_IsConnected();

    // 实时喊话
    FOURINONESERVICE_API void FourInOneService_RealTimeShout(uint8_t* data, int length);
    // 设置音量
    FOURINONESERVICE_API void FourInOneService_SetVolumn(int volumn);
    // 文字转语音播放
    FOURINONESERVICE_API void FourInOneService_PlayTTS(int voiceType, int loop, const char* text);
    // 停止文字转语音播放
    FOURINONESERVICE_API void FourInOneService_StopTTS();
    // 上传音频文件
    FOURINONESERVICE_API bool FourInOneService_UploadFile(const wchar_t* filePath);
    // 获取音频文件
    FOURINONESERVICE_API const char* FourInOneService_GetFileList();
    // 删除音频文件
    FOURINONESERVICE_API bool FourInOneService_DelFile(const wchar_t* audioName);
    // 播放音频文件
    FOURINONESERVICE_API void FourInOneService_PlayAudio(const char* audioName, int loop);
    // 停止播放音频文件
    FOURINONESERVICE_API void FourInOneService_StopPlayAudio();
    // 播放警报
    FOURINONESERVICE_API void FourInOneService_PlayAlarm();
    // 停止播放警报
    FOURINONESERVICE_API void FourInOneService_StopPlayAlarm();
    // 俯仰控制
    FOURINONESERVICE_API void FourInOneService_PitchControl(unsigned int pitch);

    // 开始收音
    FOURINONESERVICE_API void FourInOneService_StartRadio();
    // 结束收音
    FOURINONESERVICE_API void FourInOneService_StopRadio();

    // 开灯、关灯，open 1 开 0 关
    FOURINONESERVICE_API void FourInOneService_OpenLight(int open);
    // 亮度调整
    FOURINONESERVICE_API void FourInOneService_LuminanceChange(int lum);
    // 爆闪开、关，open 1 开 0 关
    FOURINONESERVICE_API void FourInOneService_SharpFlash(int open);
    // 俯仰控制, 0-100
    FOURINONESERVICE_API void FourInOneService_ControlServo(int pitch);


    // 红蓝控制，model 0 关，1-16都是开，是16种红蓝模式
    FOURINONESERVICE_API void FourInOneService_RedBlueLedControl(int model);

    // 回调注册，用于接收收音内容
    FOURINONESERVICE_API void FourInOneService_RegisterCallback(RadioCallback callback);
}