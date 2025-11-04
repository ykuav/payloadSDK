#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
// 品灵探照灯
#ifdef PLLIGHTSERVICEDLL_EXPORTS
#define PLLIGHTSERVICE_API __declspec(dllexport)
#else
#define PLLIGHTSERVICE_API __declspec(dllimport)
#endif

// 回调函数类型定义
typedef void(__stdcall* Callback)(uint8_t* data, int length);

// C接口封装
extern "C" {
    PLLIGHTSERVICE_API void PLLightService_Cleanup();
    // IP设置
    PLLIGHTSERVICE_API void PLLightService_SetIp(const char* ip);

    // 连接与断开
    PLLIGHTSERVICE_API bool PLLightService_Connection();
    PLLIGHTSERVICE_API void PLLightService_DisConnected();
    PLLIGHTSERVICE_API bool PLLightService_IsConnected();

    // 开灯、关灯，open 1 开 0 关
    PLLIGHTSERVICE_API void PLLightService_OpenLight(bool open);
    // 亮度调整，lum 亮度值 0-100
    PLLIGHTSERVICE_API void PLLightService_LuminanceChange(int lum);
    // 打开闪烁模式（没有关闭命令，关闭LDE再重启LDE的时候，会变回常量模式）
    PLLIGHTSERVICE_API void PLLightService_startFlashing();
    /**
     * 闪烁频率设置
     * frequency 闪烁频率，2、5、10、15Hz
     */
    PLLIGHTSERVICE_API void PLLightService_setFlashingFrequency(int frequency);
    // 云台回中
    PLLIGHTSERVICE_API void PLLightService_PTZToCenter();
    // 云台控制，手动速度模式
    PLLIGHTSERVICE_API void PLLightService_PTZCtrlBySpeed(int16_t picth, int16_t roll, int16_t yaw);
    // 云台控制，绝对角度控制，回中位置为0点
    PLLIGHTSERVICE_API void PLLightService_PTZCtrlByAngle(int16_t picth, int16_t roll, int16_t yaw);

    // 回调注册
    PLLIGHTSERVICE_API void PLLightService_RegisterCallback(Callback callback);
}