#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
// ������
#ifdef SLOWDESCENTDEVICESERVICEDLL_EXPORTS
#define SLOWDESCENTDEVICESERVICE_API __declspec(dllexport)
#else
#define SLOWDESCENTDEVICESERVICE_API __declspec(dllimport)
#endif

// �ص��������Ͷ���
typedef void(__stdcall* SlowDescentDeviceCallback)(uint8_t* data, int length);

// C�ӿڷ�װ
extern "C" {
    SLOWDESCENTDEVICESERVICE_API void SlowDescentDeviceService_Cleanup();
    // IP����
    SLOWDESCENTDEVICESERVICE_API void SlowDescentDeviceService_SetIp(const char* ip);

    // ������Ͽ�
    SLOWDESCENTDEVICESERVICE_API bool SlowDescentDeviceService_Connection();
    SLOWDESCENTDEVICESERVICE_API void SlowDescentDeviceService_DisConnected();
    SLOWDESCENTDEVICESERVICE_API bool SlowDescentDeviceService_IsConnected();

    // ���û������Ƿ���ã�true���ã�false������
    SLOWDESCENTDEVICESERVICE_API void SlowDescentDeviceService_Enable(BOOL flag);
    // �������������ƣ�0 �������״̬��1 �����ƶ���2 �����۶�
    SLOWDESCENTDEVICESERVICE_API void SlowDescentDeviceService_EmergencyControl(int command);
    /* ��������������
    * mode: 0�������ȣ�1�����ٶ�
    * speedOrLength: �ٶȻ򳤶�
    * */
    SLOWDESCENTDEVICESERVICE_API void SlowDescentDeviceService_actionControl(int mode, int speedOrLength);
    // �ص�ע��
    SLOWDESCENTDEVICESERVICE_API void SlowDescentDeviceService_RegisterCallback(SlowDescentDeviceCallback callback);
}