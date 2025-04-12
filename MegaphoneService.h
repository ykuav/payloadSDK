#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
#include <winhttp.h>

#ifdef MEGAPHONESERVICE_EXPORTS
#define MEGAPHONESERVICE_API __declspec(dllexport)
#else
#define MEGAPHONESERVICE_API __declspec(dllimport)
#endif

extern "C" {
    MEGAPHONESERVICE_API void MegaphoneService_Cleanup();

    // IP����
    MEGAPHONESERVICE_API void MegaphoneService_SetIp(const char* ip);

    // ������Ͽ�
    MEGAPHONESERVICE_API bool MegaphoneService_Connection();
    MEGAPHONESERVICE_API void MegaphoneService_DisConnected();
    MEGAPHONESERVICE_API bool MegaphoneService_IsConnected();

    // ʵʱ����
    MEGAPHONESERVICE_API void MegaphoneService_RealTimeShout(uint8_t* data, int length);
    // ��������
    MEGAPHONESERVICE_API void MegaphoneService_SetVolumn(int volumn);
    // ����ת��������
    MEGAPHONESERVICE_API void MegaphoneService_PlayTTS(int voiceType, int loop, const char* text);
    // ֹͣ����ת��������
    MEGAPHONESERVICE_API void MegaphoneService_StopTTS();
    // �ϴ���Ƶ�ļ�
    MEGAPHONESERVICE_API bool MegaphoneService_UploadFile(const wchar_t* filePath);
    // ��ȡ��Ƶ�ļ�
    MEGAPHONESERVICE_API const char* MegaphoneService_GetFileList();
    // ɾ����Ƶ�ļ�
    MEGAPHONESERVICE_API bool MegaphoneService_DelFile(const wchar_t* audioName);
    // ������Ƶ�ļ�
    MEGAPHONESERVICE_API void MegaphoneService_PlayAudio(const char* audioName, int loop);
    // ֹͣ������Ƶ�ļ�
    MEGAPHONESERVICE_API void MegaphoneService_StopPlayAudio();
    // ���ž���
    MEGAPHONESERVICE_API void MegaphoneService_PlayAlarm();
    // ֹͣ���ž���
    MEGAPHONESERVICE_API void MegaphoneService_StopPlayAlarm();
    // ��������
    MEGAPHONESERVICE_API void MegaphoneService_PitchControl(unsigned int pitch);
}