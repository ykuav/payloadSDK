#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
#include <vector>
// 多合一
#ifdef ALLINONESERVICEDLL_EXPORTS
#define ALLINONESERVICE_API __declspec(dllexport)
#else
#define ALLINONESERVICE_API __declspec(dllimport)
#endif

// 灯和抛投回调函数类型定义
typedef void(__stdcall* MainCallback)(uint8_t* data, int length);
// 俯仰回调函数类型定义
typedef void(__stdcall* PtzCallback)(uint8_t* data, int length);

extern "C" {
    // 清理
    ALLINONESERVICE_API void AllInOneService_Cleanup();

    // IP设置
    ALLINONESERVICE_API void AllInOneService_SetIp(const char* ip);

    // 喊话器连接与断开
    ALLINONESERVICE_API bool AllInOneService_Megaphone_Connection();
    ALLINONESERVICE_API void AllInOneService_Megaphone_DisConnected();
    ALLINONESERVICE_API bool AllInOneService_Megaphone_IsConnected();

    // 实时喊话
    ALLINONESERVICE_API void AllInOneService_RealTimeShout(uint8_t* data, int length);
    // 设置音量
    ALLINONESERVICE_API void AllInOneService_SetVolumn(int volumn);
    // 文字转语音播放
    ALLINONESERVICE_API void AllInOneService_PlayTTS(int voiceType, int loop, const char* text);
    // 停止文字转语音播放
    ALLINONESERVICE_API void AllInOneService_StopTTS();
    // 上传音频文件
    ALLINONESERVICE_API bool AllInOneService_UploadFile(const wchar_t* filePath);
    // 获取音频文件
    ALLINONESERVICE_API const char* AllInOneService_GetFileList();
    // 删除音频文件
    ALLINONESERVICE_API bool AllInOneService_DelFile(const wchar_t* audioName);
    // 播放音频文件
    ALLINONESERVICE_API void AllInOneService_PlayAudio(const char* audioName, int loop);
    // 停止播放音频文件
    ALLINONESERVICE_API void AllInOneService_StopPlayAudio();
    // 播放警报
    ALLINONESERVICE_API void AllInOneService_PlayAlarm();
    // 停止播放警报
    ALLINONESERVICE_API void AllInOneService_StopPlayAlarm();

    // 灯和抛投连接与断开
    ALLINONESERVICE_API bool AllInOneService_Main_Connection();
    ALLINONESERVICE_API void AllInOneService_Main_DisConnected();
    ALLINONESERVICE_API bool AllInOneService_Main_IsConnected();

    // 安全开关，open 1 开 0 关
    ALLINONESERVICE_API void AllInOneService_SafetySwitch(int open);
    // 开灯、关灯，open 1 开 0 关
    ALLINONESERVICE_API void AllInOneService_OpenLight(int open);
    // 亮度调整
    ALLINONESERVICE_API void AllInOneService_LuminanceChange(int lum);
    // 爆闪开、关，open 1 开 0 关
    ALLINONESERVICE_API void AllInOneService_SharpFlash(int open);
    // 红蓝控制，model 0 关，1-16都是开，是16种红蓝模式
    ALLINONESERVICE_API void AllInOneService_RedBlueLedControl(int model);

    // 抛投开关，index:0全部，1是1号，2是2号，isOpen:0关，1开
    ALLINONESERVICE_API void AllInOneService_ThrowerSwitch(int index, int isOpen);
    // 灭火弹充电放电，index:1是1号，2是2号，isAllow:0放电并禁止引爆，1充电并允许引爆
    ALLINONESERVICE_API void AllInOneService_AllowDetonate(int index, int isAllow);
    // 设置引爆高度
    ALLINONESERVICE_API void AllInOneService_SetDetonateHeight(int height);

    // 回调注册，用于接收灯和抛投状态
    ALLINONESERVICE_API void AllInOneService_RegisterMainCallback(MainCallback callback);

    // 俯仰连接与断开
    ALLINONESERVICE_API bool AllInOneService_Ptz_Connection();
    ALLINONESERVICE_API void AllInOneService_Ptz_DisConnected();
    ALLINONESERVICE_API bool AllInOneService_Ptz_IsConnected();
    // 俯仰控制,0-900，代表0-90°，其中90°时俯仰朝下
    ALLINONESERVICE_API void AllInOneService_PitchControl(int pitch);

    // 回调注册，用于接收俯仰状态
    ALLINONESERVICE_API void AllInOneService_RegisterPtzCallback(PtzCallback callback);
}