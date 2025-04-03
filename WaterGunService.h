#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
// 水枪
#ifdef WATERGUNDLL_EXPORTS
#define WATERGUNSERVICE_API __declspec(dllexport)
#else
#define WATERGUNSERVICE_API __declspec(dllimport)
#endif

// 回调函数类型定义
typedef void(__stdcall* WaterGunServiceStateCallback)(int state);

// C接口封装
extern "C" {
    WATERGUNSERVICE_API void WaterGunService_Cleanup();
    // IP设置
    WATERGUNSERVICE_API void WaterGunService_SetIp(const char* ip);

    // 连接与断开
    WATERGUNSERVICE_API bool WaterGunService_Connection();
    WATERGUNSERVICE_API void WaterGunService_DisConnected();
    WATERGUNSERVICE_API bool WaterGunService_IsConnected();

    // 操作水枪开关，0关，1开
    WATERGUNSERVICE_API void WaterGunService_Operate(int operateType);
    // 发送心跳包
    WATERGUNSERVICE_API void WaterGunService_Heartbeat();

    // 回调注册，0 已关闭，1 已打开
    WATERGUNSERVICE_API void WaterGunService_RegisterCallback(WaterGunServiceStateCallback callback);
}