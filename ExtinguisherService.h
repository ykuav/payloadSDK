#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
// ����
#ifdef EXTINGUISHERDLL_EXPORTS
#define EXTINGUISHERSERVICE_API __declspec(dllexport)
#else
#define EXTINGUISHERSERVICE_API __declspec(dllimport)
#endif

// �ص��������Ͷ���
typedef void(__stdcall* ExtinguisherStateCallback)(int state);

// C�ӿڷ�װ
extern "C" {
    EXTINGUISHERSERVICE_API void ExtinguisherService_Cleanup();
    // IP����
    EXTINGUISHERSERVICE_API void ExtinguisherService_SetIp(const char* ip);

    // ������Ͽ�
    EXTINGUISHERSERVICE_API bool ExtinguisherService_Connection();
    EXTINGUISHERSERVICE_API void ExtinguisherService_DisConnected();
    EXTINGUISHERSERVICE_API bool ExtinguisherService_IsConnected();

    // �������޿��أ�0�أ�1��
    EXTINGUISHERSERVICE_API void ExtinguisherService_Operate(int operateType);
    // ����������
    EXTINGUISHERSERVICE_API void ExtinguisherService_Heartbeat();

    // �ص�ע�ᣬ0 �ѹرգ�1 �Ѵ�
    EXTINGUISHERSERVICE_API void ExtinguisherService_RegisterCallback(ExtinguisherStateCallback callback);
}