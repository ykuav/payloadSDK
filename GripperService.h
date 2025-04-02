#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
// 机械爪
#ifdef GRIPPERSERVICEDLL_EXPORTS
#define GRIPPERSERVICE_API __declspec(dllexport)
#else
#define GRIPPERSERVICE_API __declspec(dllimport)
#endif

// C接口封装
extern "C" {
    GRIPPERSERVICE_API void GripperService_Cleanup();
    // IP设置
    GRIPPERSERVICE_API void GripperService_SetIp(const char* ip);

    // 连接与断开
    GRIPPERSERVICE_API bool GripperService_Connection();
    GRIPPERSERVICE_API void GripperService_DisConnected();
    GRIPPERSERVICE_API bool GripperService_IsConnected();

    // 抓取
    GRIPPERSERVICE_API void GripperService_Grab();
    // 松开
    GRIPPERSERVICE_API void GripperService_Release();
}