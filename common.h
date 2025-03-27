#pragma once

#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <WinSock2.h>
#include <vector>

void InitializeWinsock();
void CleanupWinsock();
SOCKET connection(std::string Ip, int Port);

class Msg {
private:
    uint8_t header = 0x8D;
    uint8_t len;
    uint8_t msgId;
    std::vector<uint8_t> payload;
    uint8_t checksum;
public:
    std::vector<uint8_t> GetMsg();
    void SetMsgId(uint8_t msgId);
    void SetPayload(const std::vector<uint8_t>& payload);
    const std::vector<uint8_t>& GetPayload();
    int length();
};

class CrcUtil {
public:
    static uint8_t Crc8Calculate(const uint8_t* data, size_t length);

private:
    static const uint8_t CRC8Table[256];
};

void printHex(const std::vector<uint8_t>& data);
#endif