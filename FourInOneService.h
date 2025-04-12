#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
#include <vector>
// �ĺ�һ
#ifdef FOURINONESERVICEDLL_EXPORTS
#define FOURINONESERVICE_API __declspec(dllexport)
#else
#define FOURINONESERVICE_API __declspec(dllimport)
#endif

// �ص��������Ͷ���
typedef void(__stdcall* RadioCallback)(uint8_t* data, int length);

extern "C" {
    // ��ʼ������
    FOURINONESERVICE_API void FourInOneService_Cleanup();

    // IP����
    FOURINONESERVICE_API void FourInOneService_SetIp(const char* ip);

    // ������Ͽ�
    FOURINONESERVICE_API bool FourInOneService_Connection();
    FOURINONESERVICE_API void FourInOneService_DisConnected();
    FOURINONESERVICE_API bool FourInOneService_IsConnected();

    // ʵʱ����
    FOURINONESERVICE_API void FourInOneService_RealTimeShout(uint8_t* data, int length);
    // ��������
    FOURINONESERVICE_API void FourInOneService_SetVolumn(int volumn);
    // ����ת��������
    FOURINONESERVICE_API void FourInOneService_PlayTTS(int voiceType, int loop, const char* text);
    // ֹͣ����ת��������
    FOURINONESERVICE_API void FourInOneService_StopTTS();
    // �ϴ���Ƶ�ļ�
    FOURINONESERVICE_API bool FourInOneService_UploadFile(const wchar_t* filePath);
    // ��ȡ��Ƶ�ļ�
    FOURINONESERVICE_API const char* FourInOneService_GetFileList();
    // ɾ����Ƶ�ļ�
    FOURINONESERVICE_API bool FourInOneService_DelFile(const wchar_t* audioName);
    // ������Ƶ�ļ�
    FOURINONESERVICE_API void FourInOneService_PlayAudio(const char* audioName, int loop);
    // ֹͣ������Ƶ�ļ�
    FOURINONESERVICE_API void FourInOneService_StopPlayAudio();
    // ���ž���
    FOURINONESERVICE_API void FourInOneService_PlayAlarm();
    // ֹͣ���ž���
    FOURINONESERVICE_API void FourInOneService_StopPlayAlarm();
    // ��������
    FOURINONESERVICE_API void FourInOneService_PitchControl(unsigned int pitch);

    // ��ʼ����
    FOURINONESERVICE_API void FourInOneService_StartRadio();
    // ��������
    FOURINONESERVICE_API void FourInOneService_StopRadio();

    // ���ơ��صƣ�open 1 �� 0 ��
    FOURINONESERVICE_API void FourInOneService_OpenLight(int open);
    // ���ȵ���
    FOURINONESERVICE_API void FourInOneService_LuminanceChange(int lum);
    // ���������أ�open 1 �� 0 ��
    FOURINONESERVICE_API void FourInOneService_SharpFlash(int open);
    // ��������, 0-100
    FOURINONESERVICE_API void FourInOneService_ControlServo(int pitch);


    // �������ƣ�model 0 �أ�1-16���ǿ�����16�ֺ���ģʽ
    FOURINONESERVICE_API void FourInOneService_RedBlueLedControl(int model);

    // �ص�ע�ᣬ���ڽ�����������
    FOURINONESERVICE_API void FourInOneService_RegisterCallback(RadioCallback callback);
}