#include "PLMsg.h"
#include <iostream>
#include <iomanip>

// 品灵校验码计算
uint8_t PL_Msg::viewlinkProtocolChecksum(const std::vector<uint8_t>& data)
{
    if (data.size() < 4) return 0; // 安全检查
    uint8_t checksum_val = data[3]; // len字段
    size_t calc_len = static_cast<size_t>(data[3]);
    for (size_t i = 0; i < calc_len - 2 && i + 4 < data.size(); ++i) {
        checksum_val ^= data[4 + i];
    }
    return checksum_val;
}

// 更新长度和校验和
void PL_Msg::updateLengthAndChecksum()
{
    len = static_cast<uint8_t>(payload.size() + 3); // 包含len和checksum自身
    std::vector<uint8_t> tempData = header;
    tempData.push_back(len);
    tempData.push_back(msgId);
    tempData.insert(tempData.end(), payload.begin(), payload.end());
    checksum = viewlinkProtocolChecksum(tempData);
}

std::vector<uint8_t> PL_Msg::GetMsg() {
    updateLengthAndChecksum();
    std::vector<uint8_t> result = header;
    result.push_back(len);
    result.push_back(msgId);
    result.insert(result.end(), payload.begin(), payload.end());
    result.push_back(checksum);
    return result;
}

// 设置msgId
void PL_Msg::SetMsgId(uint8_t msgId) {
    this->msgId = msgId;
}

// 设置Payload
void PL_Msg::SetPayload(const std::vector<uint8_t>& payload) {
    this->payload = payload;
}

// 获取Payload
const std::vector<uint8_t>& PL_Msg::GetPayload() {
    return this->payload;
}

// 获取 msg长度
int PL_Msg::length() {
    return 3 + 1 + 1 + payload.size() + 1;
}