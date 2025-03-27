#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
// ��Ͷ
#ifdef THROWERSERVICEDLL_EXPORTS
#define THROWERSERVICE_API __declspec(dllexport)
#else
#define THROWERSERVICE_API __declspec(dllimport)
#endif

// �ص��������Ͷ���
typedef void(__stdcall* ThrowerStateCallback)(uint8_t* data, int length);

// C�ӿڷ�װ
extern "C" {
    THROWERSERVICE_API void ThrowerService_Cleanup();
    // IP����
    THROWERSERVICE_API void ThrowerService_SetIp(const char* ip);

    // ������Ͽ�
    THROWERSERVICE_API bool ThrowerService_Connection();
    THROWERSERVICE_API void ThrowerService_DisConnected();
    THROWERSERVICE_API bool ThrowerService_IsConnected();

    // �򿪶����index�Ƕ����ţ�0-5
    THROWERSERVICE_API void ThrowerService_Open(int index);
    // �����λ���رգ���index�Ƕ����ţ�0-5
    THROWERSERVICE_API void ThrowerService_Close(int index);
    // ���м��������
    THROWERSERVICE_API void ThrowerService_OpenCenter();
    // �ر��м��������
    THROWERSERVICE_API void ThrowerService_CloseCenter();
    // ������������
    THROWERSERVICE_API void ThrowerService_OpenLeft();
    // �ر�����������
    THROWERSERVICE_API void ThrowerService_CloseLeft();
    // ���Ҳ��������
    THROWERSERVICE_API void ThrowerService_OpenRight();
    // �ر��Ҳ��������
    THROWERSERVICE_API void ThrowerService_CloseRight();
    // ��ȫ�����
    THROWERSERVICE_API void ThrowerService_OpenAll();
    // �ر�ȫ�����
    THROWERSERVICE_API void ThrowerService_CloseAll();
    // ���ŵ磬type=true�ǳ�磬type=false�Ƿŵ�
    THROWERSERVICE_API void ThrowerService_Charging(BOOL type);
    // �Ƿ�����������type=true������������type=false�ǽ�ֹ����
    THROWERSERVICE_API void ThrowerService_AllowDetonation(BOOL type);
    // ���������߶ȣ�������䵽ָ���߶Ⱥ���Զ�����
    THROWERSERVICE_API void ThrowerService_SetDetonateHeight(int height);
    // ����������Ͷ���ӳɹ����趨ʱ���͡���Ͷ���15s����δ�յ������������Զ�����
    THROWERSERVICE_API void ThrowerService_Heartbeat();
    // ���°��ӳ���
    THROWERSERVICE_API void ThrowerService_Update();

    // �ص�ע��
    THROWERSERVICE_API void ThrowerService_RegisterCallback(ThrowerStateCallback callback);
}