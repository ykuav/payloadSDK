#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
// 38mm发射器
#ifdef EMITTERSERVICEEDLL_EXPORTS
#define EMITTERSERVICE_API __declspec(dllexport)
#else
#define EMITTERSERVICE_API __declspec(dllimport)
#endif

// 回调函数类型定义
typedef void(__stdcall* EmitterStatusCallback)(uint8_t* data, int length);

// C接口封装
extern "C" {
    EMITTERSERVICE_API void EmitterService_Cleanup();
    // IP设置
    EMITTERSERVICE_API void EmitterService_SetIp(const char* ip);

    // 连接与断开
    EMITTERSERVICE_API bool EmitterService_Connection();
    EMITTERSERVICE_API void EmitterService_DisConnected();
    EMITTERSERVICE_API bool EmitterService_IsConnected();

    // 发射，index是发射口序号，0-5
    EMITTERSERVICE_API void EmitterService_Launch(int index);
    // 发射口状态获取
    EMITTERSERVICE_API void EmitterService_GetStatus();

    // 回调注册，状态值说明：0x00 空仓，0x01 在仓（可发射），0x02 发射中，0x03 卡住了
    EMITTERSERVICE_API void EmitterService_RegisterCallback(EmitterStatusCallback callback);
}