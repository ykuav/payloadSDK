#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
// ̽�յ�
#ifdef LIGHTSERVICEDLL_EXPORTS
#define LIGHTSERVICE_API __declspec(dllexport)
#else
#define LIGHTSERVICE_API __declspec(dllimport)
#endif

// �ص��������Ͷ���
typedef void(__stdcall* TemperatureCallback)(int, int);

// C�ӿڷ�װ
extern "C" {
    LIGHTSERVICE_API void LightService_Cleanup();
    // IP����
    LIGHTSERVICE_API void LightService_SetIp(const char* ip);

    // ������Ͽ�
    LIGHTSERVICE_API bool LightService_Connection();
    LIGHTSERVICE_API void LightService_DisConnected();
    LIGHTSERVICE_API bool LightService_IsConnected();

    // ���ơ��صƣ�open 1 �� 0 ��
    LIGHTSERVICE_API void LightService_OpenLight(int open);
    // ���ȵ���
    LIGHTSERVICE_API void LightService_LuminanceChange(int lum);
    // ���������أ�open 1 �� 0 ��
    LIGHTSERVICE_API void LightService_SharpFlash(int open);
    // �¶Ȼ�ȡ
    LIGHTSERVICE_API void LightService_FetchTemperature();
    // ��̨����
    LIGHTSERVICE_API void LightService_GimbalControl(int16_t picth, int16_t roll, int16_t yaw);

    // �ص�ע��
    LIGHTSERVICE_API void LightService_RegisterCallback(TemperatureCallback callback);
}