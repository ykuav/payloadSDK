#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
// 清洗水枪
#ifdef WATERGUNDLL_EXPORTS
#define WATERGUNSERVICE_API __declspec(dllexport)
#else
#define WATERGUNSERVICE_API __declspec(dllimport)
#endif

// 回调函数类型定义
typedef void(__stdcall* WaterGunServiceStateCallback)(int state, int locationStatus);

// C接口封装
extern "C" {
    WATERGUNSERVICE_API void WaterGunService_Cleanup();
    // IP设置
    WATERGUNSERVICE_API void WaterGunService_SetIp(const char* ip);

    // 连接与断开
    WATERGUNSERVICE_API bool WaterGunService_Connection();
    WATERGUNSERVICE_API void WaterGunService_DisConnected();
    WATERGUNSERVICE_API bool WaterGunService_IsConnected();

    // 水枪手动模式时，向左转
    WATERGUNSERVICE_API void WaterGunService_ToLeft();
    // 水枪手动模式时，向右转
    WATERGUNSERVICE_API void WaterGunService_ToRight();
    /* 操作水枪模式切换
    * 状态为0时，不可用
    * 状态为1时，发送该命令会变成自动模式，即水枪自动左右转，状态会变为2
    * 状态为2时，发送该命令，水枪停止左右转动，状态变为3，水枪慢慢回归中点，然后自动变为手动模式，状态变为4
    * 状态为4时，发送该命令，状态变为5，与状态1一致
    * 注：状态值从回调函数里面获取
    * */
    WATERGUNSERVICE_API void WaterGunService_ModeSwitch();
    // 发送心跳包
    WATERGUNSERVICE_API void WaterGunService_Heartbeat();

    // 回调注册，0 已关闭，1 已打开
    WATERGUNSERVICE_API void WaterGunService_RegisterCallback(WaterGunServiceStateCallback callback);
}