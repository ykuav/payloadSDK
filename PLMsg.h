#pragma once

#ifndef PLMSG_H
#define PLMSG_H

#include <string>
#include <WinSock2.h>
#include <vector>

class PL_Msg {
    private:
        std::vector<uint8_t> header{ 0x55, 0xAA, 0xDC }; // ¹Ì¶¨Í·
        uint8_t len = 0;
        uint8_t msgId = 0x00;
        std::vector<uint8_t> payload;
        uint8_t checksum = 0x00;
        uint8_t viewlinkProtocolChecksum(const std::vector<uint8_t>& data);
    public:
        void updateLengthAndChecksum();
        std::vector<uint8_t> GetMsg();
        void SetMsgId(uint8_t msgId);
        void SetPayload(const std::vector<uint8_t>& payload);
        const std::vector<uint8_t>& GetPayload();
        int length();
    };
#endif