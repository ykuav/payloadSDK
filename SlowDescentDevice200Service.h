#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
// 200kg缓降器
#ifdef SLOWDESCENTDEVICE200SERVICEDLL_EXPORTS
#define SLOWDESCENTDEVICE200SERVICE_API __declspec(dllexport)
#else
#define SLOWDESCENTDEVICE200SERVICE_API __declspec(dllimport)
#endif

// 回调函数类型定义
typedef void(__stdcall* SlowDescentDevice200Callback)(uint8_t* data, int length);

// C接口封装
extern "C" {
    SLOWDESCENTDEVICE200SERVICE_API void SlowDescentDevice200Service_Cleanup();
    // IP设置
    SLOWDESCENTDEVICE200SERVICE_API void SlowDescentDevice200Service_SetIp(const char* ip);

    // 连接与断开
    SLOWDESCENTDEVICE200SERVICE_API bool SlowDescentDevice200Service_Connection();
    SLOWDESCENTDEVICE200SERVICE_API void SlowDescentDevice200Service_DisConnected();
    SLOWDESCENTDEVICE200SERVICE_API bool SlowDescentDevice200Service_IsConnected();

    // 设置缓降器是否可用，true可用，false不可用（安全开关）
    SLOWDESCENTDEVICE200SERVICE_API void SlowDescentDevice200Service_Enable(BOOL flag);
    // 缓降器紧急控制，0: 复位，1：急停，2：熔断
    SLOWDESCENTDEVICE200SERVICE_API void SlowDescentDevice200Service_EmergencyControl(int command);
    // 红蓝警示灯开关控制，false: 关，true: 开
    SLOWDESCENTDEVICE200SERVICE_API void SlowDescentDevice200Service_WarningLightControl(BOOL flag);
    // 按速度控制
    SLOWDESCENTDEVICE200SERVICE_API void SlowDescentDevice200Service_ControlBySpeed(int speed);
    // 按长度控制
    SLOWDESCENTDEVICE200SERVICE_API void SlowDescentDevice200Service_ControlByLength(int length);
    // 挂钩开关控制，false: 关，true: 开
    SLOWDESCENTDEVICE200SERVICE_API void SlowDescentDevice200Service_HookControl(BOOL flag);
    // 重量清零
    SLOWDESCENTDEVICE200SERVICE_API void SlowDescentDevice200Service_ResetWeight();

    // 回调注册
    SLOWDESCENTDEVICE200SERVICE_API void SlowDescentDevice200Service_RegisterCallback(SlowDescentDevice200Callback callback);
}