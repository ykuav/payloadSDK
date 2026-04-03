#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
#include <vector>
// 四合一2代
#ifdef FOURINONE2SERVICEDLL_EXPORTS
#define FOURINONE2SERVICE_API __declspec(dllexport)
#else
#define FOURINONE2SERVICE_API __declspec(dllimport)
#endif

// 灯回调函数类型定义
typedef void(__stdcall* LightCallback)(uint8_t* data, int length);
// 收音回调函数类型定义
typedef void(__stdcall* RadioCallback)(uint8_t* data, int length);

extern "C" {
    // 清理
    FOURINONE2SERVICE_API void FourInOne2Service_Cleanup();

    // IP设置
    FOURINONE2SERVICE_API void FourInOne2Service_SetIp(const char* ip);

    // 喊话器连接与断开
    FOURINONE2SERVICE_API bool FourInOne2Service_Megaphone_Connection();
    FOURINONE2SERVICE_API void FourInOne2Service_Megaphone_DisConnected();
    FOURINONE2SERVICE_API bool FourInOne2Service_Megaphone_IsConnected();

    // 实时喊话
    FOURINONE2SERVICE_API void FourInOne2Service_RealTimeShout(uint8_t* data, int length);
    // 设置音量
    FOURINONE2SERVICE_API void FourInOne2Service_SetVolumn(int volumn);
    // 文字转语音播放
    FOURINONE2SERVICE_API void FourInOne2Service_PlayTTS(int voiceType, int loop, const char* text);
    // 停止文字转语音播放
    FOURINONE2SERVICE_API void FourInOne2Service_StopTTS();
    // 上传音频文件
    FOURINONE2SERVICE_API bool FourInOne2Service_UploadFile(const wchar_t* filePath);
    // 获取音频文件
    FOURINONE2SERVICE_API const char* FourInOne2Service_GetFileList();
    // 删除音频文件
    FOURINONE2SERVICE_API bool FourInOne2Service_DelFile(const wchar_t* audioName);
    // 播放音频文件
    FOURINONE2SERVICE_API void FourInOne2Service_PlayAudio(const char* audioName, int loop);
    // 停止播放音频文件
    FOURINONE2SERVICE_API void FourInOne2Service_StopPlayAudio();
    // 播放警报
    FOURINONE2SERVICE_API void FourInOne2Service_PlayAlarm();
    // 停止播放警报
    FOURINONE2SERVICE_API void FourInOne2Service_StopPlayAlarm();

    // 开始收音
    FOURINONE2SERVICE_API void FourInOne2Service_StartRadio();
    // 结束收音
    FOURINONE2SERVICE_API void FourInOne2Service_StopRadio();

    // 灯连接与断开
    FOURINONE2SERVICE_API bool FourInOne2Service_Light_Connection();
    FOURINONE2SERVICE_API void FourInOne2Service_Light_DisConnected();
    FOURINONE2SERVICE_API bool FourInOne2Service_Light_IsConnected();

    // 开灯、关灯，open 1 开 0 关
    FOURINONE2SERVICE_API void FourInOne2Service_OpenLight(int open);
    // 亮度调整
    FOURINONE2SERVICE_API void FourInOne2Service_LuminanceChange(int lum);
    // 爆闪开、关，open 1 开 0 关
    FOURINONE2SERVICE_API void FourInOne2Service_SharpFlash(int open);
    // 红蓝控制，model 0 关，1-16都是开，是16种红蓝模式
    FOURINONE2SERVICE_API void FourInOne2Service_RedBlueLedControl(int model);

    // 回调注册，用于接收灯状态
    FOURINONE2SERVICE_API void FourInOne2Service_RegisterLightCallback(LightCallback callback);

    // 回调注册，用于接收收音数据
    FOURINONE2SERVICE_API void FourInOne2Service_RegisterRadioCallback(RadioCallback callback);
}