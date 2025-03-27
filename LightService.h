#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
// 探照灯
#ifdef LIGHTSERVICEDLL_EXPORTS
#define LIGHTSERVICE_API __declspec(dllexport)
#else
#define LIGHTSERVICE_API __declspec(dllimport)
#endif

// 回调函数类型定义
typedef void(__stdcall* TemperatureCallback)(int, int);

// C接口封装
extern "C" {
    LIGHTSERVICE_API void LightService_Cleanup();
    // IP设置
    LIGHTSERVICE_API void LightService_SetIp(const char* ip);

    // 连接与断开
    LIGHTSERVICE_API bool LightService_Connection();
    LIGHTSERVICE_API void LightService_DisConnected();
    LIGHTSERVICE_API bool LightService_IsConnected();

    // 开灯、关灯，open 1 开 0 关
    LIGHTSERVICE_API void LightService_OpenLight(int open);
    // 亮度调整
    LIGHTSERVICE_API void LightService_LuminanceChange(int lum);
    // 爆闪开、关，open 1 开 0 关
    LIGHTSERVICE_API void LightService_SharpFlash(int open);
    // 温度获取
    LIGHTSERVICE_API void LightService_FetchTemperature();
    // 云台控制
    LIGHTSERVICE_API void LightService_GimbalControl(int16_t picth, int16_t roll, int16_t yaw);

    // 回调注册
    LIGHTSERVICE_API void LightService_RegisterCallback(TemperatureCallback callback);
}