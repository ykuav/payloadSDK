#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
// 吊桶
#ifdef WATERBRANCHDLL_EXPORTS
#define WATERBRANCHSERVICE_API __declspec(dllexport)
#else
#define WATERBRANCHSERVICE_API __declspec(dllimport)
#endif

// 回调函数类型定义
typedef void(__stdcall* WaterBranchServiceStateCallback)(uint8_t* data, int length);

// C接口封装
extern "C" {
    WATERBRANCHSERVICE_API void WaterBranchService_Cleanup();
    // IP设置
    WATERBRANCHSERVICE_API void WaterBranchService_SetIp(const char* ip);

    // 连接与断开
    WATERBRANCHSERVICE_API bool WaterBranchService_Connection();
    WATERBRANCHSERVICE_API void WaterBranchService_DisConnected();
    WATERBRANCHSERVICE_API bool WaterBranchService_IsConnected();

    // 释放水带，0关，1开
    WATERBRANCHSERVICE_API void WaterBranchService_HoseRelease(int operateType);
    // 水带脱困，0关，1开
    WATERBRANCHSERVICE_API void WaterBranchService_HoseDetachment(int operateType);

    // 发送心跳包
    WATERBRANCHSERVICE_API void WaterBranchService_Heartbeat();

    // 回调注册，0 已关闭，1 已打开
    WATERBRANCHSERVICE_API void WaterBranchService_RegisterCallback(WaterBranchServiceStateCallback callback);
}