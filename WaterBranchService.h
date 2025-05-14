#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
// ��Ͱ
#ifdef WATERBRANCHDLL_EXPORTS
#define WATERBRANCHSERVICE_API __declspec(dllexport)
#else
#define WATERBRANCHSERVICE_API __declspec(dllimport)
#endif

// �ص��������Ͷ���
typedef void(__stdcall* WaterBranchServiceStateCallback)(uint8_t* data, int length);

// C�ӿڷ�װ
extern "C" {
    WATERBRANCHSERVICE_API void WaterBranchService_Cleanup();
    // IP����
    WATERBRANCHSERVICE_API void WaterBranchService_SetIp(const char* ip);

    // ������Ͽ�
    WATERBRANCHSERVICE_API bool WaterBranchService_Connection();
    WATERBRANCHSERVICE_API void WaterBranchService_DisConnected();
    WATERBRANCHSERVICE_API bool WaterBranchService_IsConnected();

    // �ͷ�ˮ����0�أ�1��
    WATERBRANCHSERVICE_API void WaterBranchService_HoseRelease(int operateType);
    // ˮ��������0�أ�1��
    WATERBRANCHSERVICE_API void WaterBranchService_HoseDetachment(int operateType);

    // ����������
    WATERBRANCHSERVICE_API void WaterBranchService_Heartbeat();

    // �ص�ע�ᣬ0 �ѹرգ�1 �Ѵ�
    WATERBRANCHSERVICE_API void WaterBranchService_RegisterCallback(WaterBranchServiceStateCallback callback);
}