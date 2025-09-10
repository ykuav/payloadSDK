#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>

#ifdef MEGAPHONESERVICE_EXPORTS
#define MEGAPHONESERVICE_API __declspec(dllexport)
#else
#define MEGAPHONESERVICE_API __declspec(dllimport)
#endif

// 其他回调函数类型定义
typedef void(__stdcall* Callback)(uint8_t* data, int length);

extern "C" {
    MEGAPHONESERVICE_API void MegaphoneService_Cleanup();

    // IP设置
    MEGAPHONESERVICE_API void MegaphoneService_SetIp(const char* ip);

    // 连接与断开
    MEGAPHONESERVICE_API bool MegaphoneService_Connection();
    MEGAPHONESERVICE_API void MegaphoneService_DisConnected();
    MEGAPHONESERVICE_API bool MegaphoneService_IsConnected();

    // 实时喊话
    MEGAPHONESERVICE_API void MegaphoneService_RealTimeShout(uint8_t* data, int length);
    // 设置音量
    MEGAPHONESERVICE_API void MegaphoneService_SetVolumn(int volumn);
    // 文字转语音播放
    MEGAPHONESERVICE_API void MegaphoneService_PlayTTS(int voiceType, int loop, const char* text);
    // 停止文字转语音播放
    MEGAPHONESERVICE_API void MegaphoneService_StopTTS();
    // 上传音频文件
    MEGAPHONESERVICE_API bool MegaphoneService_UploadFile(const wchar_t* filePath);
    // 获取音频文件
    MEGAPHONESERVICE_API const char* MegaphoneService_GetFileList();
    // 删除音频文件
    MEGAPHONESERVICE_API bool MegaphoneService_DelFile(const wchar_t* audioName);
    // 播放音频文件
    MEGAPHONESERVICE_API void MegaphoneService_PlayAudio(const char* audioName, int loop);
    // 停止播放音频文件
    MEGAPHONESERVICE_API void MegaphoneService_StopPlayAudio();
    // 播放警报
    MEGAPHONESERVICE_API void MegaphoneService_PlayAlarm();
    // 停止播放警报
    MEGAPHONESERVICE_API void MegaphoneService_StopPlayAlarm();
    // 俯仰控制
    MEGAPHONESERVICE_API void MegaphoneService_PitchControl(unsigned int pitch);

    // 回调注册，
    MEGAPHONESERVICE_API void MegaphoneService_RegisterCallback(Callback callback);
}