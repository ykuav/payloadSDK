#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
// 运输箱
#ifdef CARGOBOXDLL_EXPORTS
#define CARGOBOXSERVICE_API __declspec(dllexport)
#else
#define CARGOBOXSERVICE_API __declspec(dllimport)
#endif

// 回调函数类型定义
typedef void(__stdcall* CargoBoxServiceStateCallback)(uint8_t* data, int length);

// C接口封装
extern "C" {
    CARGOBOXSERVICE_API void CargoBoxService_Cleanup();
    // IP设置
    CARGOBOXSERVICE_API void CargoBoxService_SetIp(const char* ip);

    // 连接与断开
    CARGOBOXSERVICE_API bool CargoBoxService_Connection();
    CARGOBOXSERVICE_API void CargoBoxService_DisConnected();
    CARGOBOXSERVICE_API bool CargoBoxService_IsConnected();

    // 压缩机使能
    CARGOBOXSERVICE_API void CargoBoxService_SetEnable(bool isEnable);
    // 提高功率
    CARGOBOXSERVICE_API void CargoBoxService_IncreasePower();
    // 降低功率
    CARGOBOXSERVICE_API void CargoBoxService_ReducePower();
    // 回调注册
    CARGOBOXSERVICE_API void CargoBoxService_RegisterCallback(CargoBoxServiceStateCallback callback);
}