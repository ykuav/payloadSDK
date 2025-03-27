#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
// �ƴ���
#ifdef RESQMESERVICEDLL_EXPORTS
#define RESQMESERVICE_API __declspec(dllexport)
#else
#define RESQMESERVICE_API __declspec(dllimport)
#endif

// �ص��������Ͷ���
typedef void(__stdcall* ResqmeStateCallback)(uint8_t* data, int length);

// C�ӿڷ�װ
extern "C" {
    RESQMESERVICE_API void ResqmeService_Cleanup();
    // IP����
    RESQMESERVICE_API void ResqmeService_SetIp(const char* ip);

    // ������Ͽ�
    RESQMESERVICE_API bool ResqmeService_Connection();
    RESQMESERVICE_API void ResqmeService_DisConnected();
    RESQMESERVICE_API bool ResqmeService_IsConnected();

    // ���䣬index: 1��1�ſڣ�2��2�ſڣ�3��ȫ��
    RESQMESERVICE_API void ResqmeService_Launch(int index);
    // ��ȫ���أ�state=trueʱ��
    RESQMESERVICE_API void ResqmeService_SafetySwitch(BOOL state);

    // �ص�ע��
    // ��ȫ���ش򿪺󣬲Ż�����ȷ��ȡ�����״̬��0x00 �ڲ֣��ɷ��䣩��0x01 �ղ֣�0x02 ��ס��
    RESQMESERVICE_API void ResqmeService_RegisterCallback(ResqmeStateCallback callback);
}