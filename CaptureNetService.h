#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
// 捕捉网
#ifdef CAPTURENETSERVICEDLL_EXPORTS
#define CAPTURENETSERVICE_API __declspec(dllexport)
#else
#define CAPTURENETSERVICE_API __declspec(dllimport)
#endif

// 回调函数类型定义
typedef void(__stdcall* TemperatureCallback)(int, int);

// C接口封装
extern "C" {
    CAPTURENETSERVICE_API void CaptureNetService_Cleanup();
    // IP设置
    CAPTURENETSERVICE_API void CaptureNetService_SetIp(const char* ip);

    // 连接与断开
    CAPTURENETSERVICE_API bool CaptureNetService_Connection();
    CAPTURENETSERVICE_API void CaptureNetService_DisConnected();
    CAPTURENETSERVICE_API bool CaptureNetService_IsConnected();

    // 发射
    CAPTURENETSERVICE_API void CaptureNetService_Launch();
}