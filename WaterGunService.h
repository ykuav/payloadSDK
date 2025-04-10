#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
// ˮǹ
#ifdef WATERGUNDLL_EXPORTS
#define WATERGUNSERVICE_API __declspec(dllexport)
#else
#define WATERGUNSERVICE_API __declspec(dllimport)
#endif

// �ص��������Ͷ���
typedef void(__stdcall* WaterGunServiceStateCallback)(int state);

// C�ӿڷ�װ
extern "C" {
    WATERGUNSERVICE_API void WaterGunService_Cleanup();
    // IP����
    WATERGUNSERVICE_API void WaterGunService_SetIp(const char* ip);

    // ������Ͽ�
    WATERGUNSERVICE_API bool WaterGunService_Connection();
    WATERGUNSERVICE_API void WaterGunService_DisConnected();
    WATERGUNSERVICE_API bool WaterGunService_IsConnected();

    // ����ˮǹ���أ�0�أ�1��
    WATERGUNSERVICE_API void WaterGunService_Operate(int operateType);
    // ����������
    WATERGUNSERVICE_API void WaterGunService_Heartbeat();

    // �ص�ע�ᣬ0 �ѹرգ�1 �Ѵ�
    WATERGUNSERVICE_API void WaterGunService_RegisterCallback(WaterGunServiceStateCallback callback);
}