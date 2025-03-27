#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
// 灭火罐
#ifdef EXTINGUISHERDLL_EXPORTS
#define EXTINGUISHERSERVICE_API __declspec(dllexport)
#else
#define EXTINGUISHERSERVICE_API __declspec(dllimport)
#endif

// 回调函数类型定义
typedef void(__stdcall* ExtinguisherStateCallback)(int state);

// C接口封装
extern "C" {
    EXTINGUISHERSERVICE_API void ExtinguisherService_Cleanup();
    // IP设置
    EXTINGUISHERSERVICE_API void ExtinguisherService_SetIp(const char* ip);

    // 连接与断开
    EXTINGUISHERSERVICE_API bool ExtinguisherService_Connection();
    EXTINGUISHERSERVICE_API void ExtinguisherService_DisConnected();
    EXTINGUISHERSERVICE_API bool ExtinguisherService_IsConnected();

    // 操作灭火罐开关，0关，1开
    EXTINGUISHERSERVICE_API void ExtinguisherService_Operate(int operateType);
    // 发送心跳包
    EXTINGUISHERSERVICE_API void ExtinguisherService_Heartbeat();

    // 回调注册，0 已关闭，1 已打开
    EXTINGUISHERSERVICE_API void ExtinguisherService_RegisterCallback(ExtinguisherStateCallback callback);
}