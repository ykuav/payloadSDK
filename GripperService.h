#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
// ��еצ
#ifdef GRIPPERSERVICEDLL_EXPORTS
#define GRIPPERSERVICE_API __declspec(dllexport)
#else
#define GRIPPERSERVICE_API __declspec(dllimport)
#endif

// C�ӿڷ�װ
extern "C" {
    GRIPPERSERVICE_API void GripperService_Cleanup();
    // IP����
    GRIPPERSERVICE_API void GripperService_SetIp(const char* ip);

    // ������Ͽ�
    GRIPPERSERVICE_API bool GripperService_Connection();
    GRIPPERSERVICE_API void GripperService_DisConnected();
    GRIPPERSERVICE_API bool GripperService_IsConnected();

    // ץȡ
    GRIPPERSERVICE_API void GripperService_Grab();
    // �ɿ�
    GRIPPERSERVICE_API void GripperService_Release();
}