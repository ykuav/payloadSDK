#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
// 38mm������
#ifdef EMITTERSERVICEEDLL_EXPORTS
#define EMITTERSERVICE_API __declspec(dllexport)
#else
#define EMITTERSERVICE_API __declspec(dllimport)
#endif

// �ص��������Ͷ���
typedef void(__stdcall* EmitterStatusCallback)(uint8_t* data, int length);

// C�ӿڷ�װ
extern "C" {
    EMITTERSERVICE_API void EmitterService_Cleanup();
    // IP����
    EMITTERSERVICE_API void EmitterService_SetIp(const char* ip);

    // ������Ͽ�
    EMITTERSERVICE_API bool EmitterService_Connection();
    EMITTERSERVICE_API void EmitterService_DisConnected();
    EMITTERSERVICE_API bool EmitterService_IsConnected();

    // ���䣬index�Ƿ������ţ�0-5
    EMITTERSERVICE_API void EmitterService_Launch(int index);
    // �����״̬��ȡ
    EMITTERSERVICE_API void EmitterService_GetStatus();

    // �ص�ע�ᣬ״ֵ̬˵����0x00 �ղ֣�0x01 �ڲ֣��ɷ��䣩��0x02 �����У�0x03 ��ס��
    EMITTERSERVICE_API void EmitterService_RegisterCallback(EmitterStatusCallback callback);
}