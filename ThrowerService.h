#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
// 抛投
#ifdef THROWERSERVICEDLL_EXPORTS
#define THROWERSERVICE_API __declspec(dllexport)
#else
#define THROWERSERVICE_API __declspec(dllimport)
#endif

// 回调函数类型定义
typedef void(__stdcall* ThrowerStateCallback)(uint8_t* data, int length);

// C接口封装
extern "C" {
    THROWERSERVICE_API void ThrowerService_Cleanup();
    // IP设置
    THROWERSERVICE_API void ThrowerService_SetIp(const char* ip);

    // 连接与断开
    THROWERSERVICE_API bool ThrowerService_Connection();
    THROWERSERVICE_API void ThrowerService_DisConnected();
    THROWERSERVICE_API bool ThrowerService_IsConnected();

    // 打开舵机，index是舵机序号，0-5
    THROWERSERVICE_API void ThrowerService_Open(int index);
    // 舵机复位（关闭），index是舵机序号，0-5
    THROWERSERVICE_API void ThrowerService_Close(int index);
    // 打开中间两个舵机
    THROWERSERVICE_API void ThrowerService_OpenCenter();
    // 关闭中间两个舵机
    THROWERSERVICE_API void ThrowerService_CloseCenter();
    // 打开左侧两个舵机
    THROWERSERVICE_API void ThrowerService_OpenLeft();
    // 关闭左侧两个舵机
    THROWERSERVICE_API void ThrowerService_CloseLeft();
    // 打开右侧两个舵机
    THROWERSERVICE_API void ThrowerService_OpenRight();
    // 关闭右侧两个舵机
    THROWERSERVICE_API void ThrowerService_CloseRight();
    // 打开全部舵机
    THROWERSERVICE_API void ThrowerService_OpenAll();
    // 关闭全部舵机
    THROWERSERVICE_API void ThrowerService_CloseAll();
    // 充电放电，type=true是充电，type=false是放电
    THROWERSERVICE_API void ThrowerService_Charging(BOOL type);
    // 是否允许引爆，type=true是允许引爆，type=false是禁止引爆
    THROWERSERVICE_API void ThrowerService_AllowDetonation(BOOL type);
    // 设置引爆高度，灭火弹下落到指定高度后会自动引爆
    THROWERSERVICE_API void ThrowerService_SetDetonateHeight(int height);
    // 心跳包，抛投连接成功后需定时发送。抛投如果15s左右未收到心跳包，会自动重启
    THROWERSERVICE_API void ThrowerService_Heartbeat();
    // 更新板子程序
    THROWERSERVICE_API void ThrowerService_Update();

    // 回调注册
    THROWERSERVICE_API void ThrowerService_RegisterCallback(ThrowerStateCallback callback);
}