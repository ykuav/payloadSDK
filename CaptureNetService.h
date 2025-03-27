#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
// ��׽��
#ifdef CAPTURENETSERVICEDLL_EXPORTS
#define CAPTURENETSERVICE_API __declspec(dllexport)
#else
#define CAPTURENETSERVICE_API __declspec(dllimport)
#endif

// �ص��������Ͷ���
typedef void(__stdcall* TemperatureCallback)(int, int);

// C�ӿڷ�װ
extern "C" {
    CAPTURENETSERVICE_API void CaptureNetService_Cleanup();
    // IP����
    CAPTURENETSERVICE_API void CaptureNetService_SetIp(const char* ip);

    // ������Ͽ�
    CAPTURENETSERVICE_API bool CaptureNetService_Connection();
    CAPTURENETSERVICE_API void CaptureNetService_DisConnected();
    CAPTURENETSERVICE_API bool CaptureNetService_IsConnected();

    // ����
    CAPTURENETSERVICE_API void CaptureNetService_Launch();
}