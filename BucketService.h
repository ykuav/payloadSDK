#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
// 吊桶
#ifdef BUCKETDLL_EXPORTS
#define BUCKETSERVICE_API __declspec(dllexport)
#else
#define BUCKETSERVICE_API __declspec(dllimport)
#endif

// 回调函数类型定义
typedef void(__stdcall* BucketServiceStateCallback)(int state);

// C接口封装
extern "C" {
    BUCKETSERVICE_API void BucketService_Cleanup();
    // IP设置
    BUCKETSERVICE_API void BucketService_SetIp(const char* ip);

    // 连接与断开
    BUCKETSERVICE_API bool BucketService_Connection();
    BUCKETSERVICE_API void BucketService_DisConnected();
    BUCKETSERVICE_API bool BucketService_IsConnected();

    // 操作吊桶开关，0停，1开（升），2关（降）
    BUCKETSERVICE_API void BucketService_BarrelControl(int operateType);
    // 操作挂钩开关，0关，1开
    BUCKETSERVICE_API void BucketService_HookControl(int controlType);

    // 发送心跳包
    BUCKETSERVICE_API void BucketService_Heartbeat();

    // 回调注册，0 已关闭，1 已打开
    BUCKETSERVICE_API void BucketService_RegisterCallback(BucketServiceStateCallback callback);
}