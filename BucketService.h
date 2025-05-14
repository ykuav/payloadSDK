#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
// ��Ͱ
#ifdef BUCKETDLL_EXPORTS
#define BUCKETSERVICE_API __declspec(dllexport)
#else
#define BUCKETSERVICE_API __declspec(dllimport)
#endif

// �ص��������Ͷ���
typedef void(__stdcall* BucketServiceStateCallback)(int state);

// C�ӿڷ�װ
extern "C" {
    BUCKETSERVICE_API void BucketService_Cleanup();
    // IP����
    BUCKETSERVICE_API void BucketService_SetIp(const char* ip);

    // ������Ͽ�
    BUCKETSERVICE_API bool BucketService_Connection();
    BUCKETSERVICE_API void BucketService_DisConnected();
    BUCKETSERVICE_API bool BucketService_IsConnected();

    // ������Ͱ���أ�0ͣ��1����������2�أ�����
    BUCKETSERVICE_API void BucketService_BarrelControl(int operateType);
    // �����ҹ����أ�0�أ�1��
    BUCKETSERVICE_API void BucketService_HookControl(int controlType);

    // ����������
    BUCKETSERVICE_API void BucketService_Heartbeat();

    // �ص�ע�ᣬ0 �ѹرգ�1 �Ѵ�
    BUCKETSERVICE_API void BucketService_RegisterCallback(BucketServiceStateCallback callback);
}