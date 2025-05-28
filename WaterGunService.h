#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
// ��ϴˮǹ
#ifdef WATERGUNDLL_EXPORTS
#define WATERGUNSERVICE_API __declspec(dllexport)
#else
#define WATERGUNSERVICE_API __declspec(dllimport)
#endif

// �ص��������Ͷ���
typedef void(__stdcall* WaterGunServiceStateCallback)(int state, int locationStatus);

// C�ӿڷ�װ
extern "C" {
    WATERGUNSERVICE_API void WaterGunService_Cleanup();
    // IP����
    WATERGUNSERVICE_API void WaterGunService_SetIp(const char* ip);

    // ������Ͽ�
    WATERGUNSERVICE_API bool WaterGunService_Connection();
    WATERGUNSERVICE_API void WaterGunService_DisConnected();
    WATERGUNSERVICE_API bool WaterGunService_IsConnected();

    // ˮǹ�ֶ�ģʽʱ������ת
    WATERGUNSERVICE_API void WaterGunService_ToLeft();
    // ˮǹ�ֶ�ģʽʱ������ת
    WATERGUNSERVICE_API void WaterGunService_ToRight();
    /* ����ˮǹģʽ�л�
    * ״̬Ϊ0ʱ��������
    * ״̬Ϊ1ʱ�����͸���������Զ�ģʽ����ˮǹ�Զ�����ת��״̬���Ϊ2
    * ״̬Ϊ2ʱ�����͸����ˮǹֹͣ����ת����״̬��Ϊ3��ˮǹ�����ع��е㣬Ȼ���Զ���Ϊ�ֶ�ģʽ��״̬��Ϊ4
    * ״̬Ϊ4ʱ�����͸����״̬��Ϊ5����״̬1һ��
    * ע��״ֵ̬�ӻص����������ȡ
    * */
    WATERGUNSERVICE_API void WaterGunService_ModeSwitch();
    // ����������
    WATERGUNSERVICE_API void WaterGunService_Heartbeat();

    // �ص�ע�ᣬ0 �ѹرգ�1 �Ѵ�
    WATERGUNSERVICE_API void WaterGunService_RegisterCallback(WaterGunServiceStateCallback callback);
}