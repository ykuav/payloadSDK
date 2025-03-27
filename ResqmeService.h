#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
// 破窗器
#ifdef RESQMESERVICEDLL_EXPORTS
#define RESQMESERVICE_API __declspec(dllexport)
#else
#define RESQMESERVICE_API __declspec(dllimport)
#endif

// 回调函数类型定义
typedef void(__stdcall* ResqmeStateCallback)(uint8_t* data, int length);

// C接口封装
extern "C" {
    RESQMESERVICE_API void ResqmeService_Cleanup();
    // IP设置
    RESQMESERVICE_API void ResqmeService_SetIp(const char* ip);

    // 连接与断开
    RESQMESERVICE_API bool ResqmeService_Connection();
    RESQMESERVICE_API void ResqmeService_DisConnected();
    RESQMESERVICE_API bool ResqmeService_IsConnected();

    // 发射，index: 1：1号口，2：2号口，3：全部
    RESQMESERVICE_API void ResqmeService_Launch(int index);
    // 安全开关，state=true时打开
    RESQMESERVICE_API void ResqmeService_SafetySwitch(BOOL state);

    // 回调注册
    // 安全开关打开后，才会能正确获取发射口状态，0x00 在仓（可发射），0x01 空仓，0x02 卡住了
    RESQMESERVICE_API void ResqmeService_RegisterCallback(ResqmeStateCallback callback);
}