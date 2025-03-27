#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
// 缓降器
#ifdef SLOWDESCENTDEVICESERVICEDLL_EXPORTS
#define SLOWDESCENTDEVICESERVICE_API __declspec(dllexport)
#else
#define SLOWDESCENTDEVICESERVICE_API __declspec(dllimport)
#endif

// 回调函数类型定义
typedef void(__stdcall* SlowDescentDeviceCallback)(uint8_t* data, int length);

// C接口封装
extern "C" {
    SLOWDESCENTDEVICESERVICE_API void SlowDescentDeviceService_Cleanup();
    // IP设置
    SLOWDESCENTDEVICESERVICE_API void SlowDescentDeviceService_SetIp(const char* ip);

    // 连接与断开
    SLOWDESCENTDEVICESERVICE_API bool SlowDescentDeviceService_Connection();
    SLOWDESCENTDEVICESERVICE_API void SlowDescentDeviceService_DisConnected();
    SLOWDESCENTDEVICESERVICE_API bool SlowDescentDeviceService_IsConnected();

    // 设置缓降器是否可用，true可用，false不可用
    SLOWDESCENTDEVICESERVICE_API void SlowDescentDeviceService_Enable(BOOL flag);
    // 缓降器紧急控制，0 解除紧急状态，1 紧急制动，2 紧急熔断
    SLOWDESCENTDEVICESERVICE_API void SlowDescentDeviceService_EmergencyControl(int command);
    /* 缓降器动作控制
    * mode: 0：按长度，1：按速度
    * speedOrLength: 速度或长度
    * */
    SLOWDESCENTDEVICESERVICE_API void SlowDescentDeviceService_actionControl(int mode, int speedOrLength);
    // 回调注册
    SLOWDESCENTDEVICESERVICE_API void SlowDescentDeviceService_RegisterCallback(SlowDescentDeviceCallback callback);
}