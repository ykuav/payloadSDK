#include "common.h"
#include <string>
#include <iostream>
#include <ws2tcpip.h>
#include <vector>
#include <iomanip>
#pragma comment(lib, "ws2_32.lib")

// 初始化 Winsock
void InitializeWinsock() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
}

// 清理 Winsock
void CleanupWinsock() {
    WSACleanup();
}

// 连接
SOCKET connection(std::string Ip, int Port) {
    SOCKET client = INVALID_SOCKET;
    // 初始化 Winsock（只需调用一次）
    static bool isWsaInitialized = false;
    if (!isWsaInitialized) {
        InitializeWinsock();
        isWsaInitialized = true;
    }

    if (Ip.empty()) {
        std::cerr << "未设置IP" << std::endl;
        return INVALID_SOCKET;
    }
    // 设置服务器地址
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(Port);
    if (inet_pton(AF_INET, Ip.c_str(), &serverAddr.sin_addr) != 1) {
        std::cerr << "无效的IP地址: " << Ip << std::endl;
        return INVALID_SOCKET;
    }

    // 创建套接字
    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client == INVALID_SOCKET) {
        std::cerr << "套接字创建失败，错误码: " << WSAGetLastError() << std::endl;
        return INVALID_SOCKET;
    }

    // 连接服务器
    if (connect(client, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "连接失败，错误码: " << WSAGetLastError() << std::endl;
        closesocket(client);
        client = INVALID_SOCKET;
        return INVALID_SOCKET;
    }
    std::cout << "连接成功！IP：" << Ip << "，端口：" << Port << std::endl;
    return client;
}

std::vector<uint8_t> Msg::GetMsg() {
    len = static_cast<uint8_t>(payload.size());
    std::vector<uint8_t> checksumData(1 + 1 + payload.size());
    checksumData[0] = len;
    checksumData[1] = msgId;
    std::memcpy(checksumData.data() + 2, payload.data(), payload.size());

    checksum = CrcUtil::Crc8Calculate(checksumData.data(), checksumData.size());
    std::vector<uint8_t> data(1 + 1 + 1 + payload.size() + 1);
    data[0] = header;
    data[1] = len;
    data[2] = msgId;
    std::memcpy(data.data() + 3, payload.data(), payload.size());
    data[data.size() - 1] = checksum;

    printHex(data);
    return data;
}

// 设置msgId
void Msg::SetMsgId(uint8_t msgId) {
    this->msgId = msgId;
}

// 设置Payload
void Msg::SetPayload(const std::vector<uint8_t>& payload) {
    this->payload = payload;
}

// 获取Payload
const std::vector<uint8_t>& Msg::GetPayload() {
    return this->payload;
}

// 获取 msg长度
int Msg::length() {
    return 3 + this->payload.size() + 1;
}

uint8_t CrcUtil::Crc8Calculate(const uint8_t* data, size_t length) {
    uint8_t crc = 0;
    for (size_t i = 0; i < length; ++i) {
        crc = CRC8Table[crc ^ data[i]];
    }
    return crc;
}

// CRC8 查表（十六进制形式）
const uint8_t CrcUtil::CRC8Table[256] = {
    0x00, 0x5E, 0xBC, 0xE2, 0x61, 0x3F, 0xDD, 0x83, 0xC2, 0x9C, 0x7E, 0x20, 0xA3, 0xFD, 0x1F, 0x41,
    0x9D, 0xC3, 0x21, 0x7F, 0xFC, 0xA2, 0x40, 0x1E, 0x5F, 0x01, 0xE3, 0xBD, 0x3E, 0x60, 0x82, 0xDC,
    0x23, 0x7D, 0x9F, 0xC1, 0x42, 0x1C, 0xFE, 0xA0, 0xE1, 0xBF, 0x5D, 0x03, 0x80, 0xDE, 0x3C, 0x62,
    0xBE, 0xE0, 0x02, 0x5C, 0xDF, 0x81, 0x63, 0x3D, 0x7C, 0x22, 0xC0, 0x9E, 0x1D, 0x43, 0xA1, 0xFF,
    0x46, 0x18, 0xFA, 0xA4, 0x27, 0x79, 0x9B, 0xC5, 0x84, 0xDA, 0x38, 0x66, 0xE5, 0xBB, 0x59, 0x07,
    0xDB, 0x85, 0x67, 0x39, 0xBA, 0xE4, 0x06, 0x58, 0x19, 0x47, 0xA5, 0xFB, 0x78, 0x26, 0xC4, 0x9A,
    0x65, 0x3B, 0xD9, 0x87, 0x04, 0x5A, 0xB8, 0xE6, 0xA7, 0xF9, 0x1B, 0x45, 0xC6, 0x98, 0x7A, 0x24,
    0xF8, 0xA6, 0x44, 0x1A, 0x99, 0xC7, 0x25, 0x7B, 0x3A, 0x64, 0x86, 0xD8, 0x5B, 0x05, 0xE7, 0xB9,
    0x8C, 0xD2, 0x30, 0x6E, 0xED, 0xB3, 0x51, 0x0F, 0x4E, 0x10, 0xF2, 0xAC, 0x2F, 0x71, 0x93, 0xCD,
    0x11, 0x4F, 0xAD, 0xF3, 0x70, 0x2E, 0xCC, 0x92, 0xD3, 0x8D, 0x6F, 0x31, 0xB2, 0xEC, 0x0E, 0x50,
    0xAF, 0xF1, 0x13, 0x4D, 0xCE, 0x90, 0x72, 0x2C, 0x6D, 0x33, 0xD1, 0x8F, 0x0C, 0x52, 0xB0, 0xEE,
    0x32, 0x6C, 0x8E, 0xD0, 0x53, 0x0D, 0xEF, 0xB1, 0xF0, 0xAE, 0x4C, 0x12, 0x91, 0xCF, 0x2D, 0x73,
    0xCA, 0x94, 0x76, 0x28, 0xAB, 0xF5, 0x17, 0x49, 0x08, 0x56, 0xB4, 0xEA, 0x69, 0x37, 0xD5, 0x8B,
    0x57, 0x09, 0xEB, 0xB5, 0x36, 0x68, 0x8A, 0xD4, 0x95, 0xCB, 0x29, 0x77, 0xF4, 0xAA, 0x48, 0x16,
    0xE9, 0xB7, 0x55, 0x0B, 0x88, 0xD6, 0x34, 0x6A, 0x2B, 0x75, 0x97, 0xC9, 0x4A, 0x14, 0xF6, 0xA8,
    0x74, 0x2A, 0xC8, 0x96, 0x15, 0x4B, 0xA9, 0xF7, 0xB6, 0xE8, 0x0A, 0x54, 0xD7, 0x89, 0x6B, 0x35
};

void printHex(const std::vector<uint8_t>& data) {
    for (uint8_t b : data) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(b) << " ";
    }
    std::cout << std::dec << std::endl; // 恢复十进制格式
}

// Helper function to convert wide string to UTF-8
std::string WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// Helper function to convert UTF-8 to wide string
std::wstring Utf8ToWide(const std::string& str) {
    if (str.empty()) return std::wstring();

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}