#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <driver/i2c_master.h>

#include "vision_target_tracker.h"

class SscmaI2cVisionSensor {
public:
    static constexpr uint8_t kDefaultAddress = 0x62;

    explicit SscmaI2cVisionSensor(i2c_master_bus_handle_t bus);
    ~SscmaI2cVisionSensor();

    bool Probe();
    bool Invoke(std::vector<NaraVisionBox>& boxes, uint32_t timeout_ms = 1200);

private:
    i2c_master_dev_handle_t device_ = nullptr;
    std::string receive_buffer_;

    bool WriteCommand(const std::string& command);
    int Available();
    bool ReadBytes(uint8_t* output, size_t length);
    bool ReadResponse(std::vector<NaraVisionBox>& boxes, uint32_t timeout_ms);
    bool ParseEvent(const std::string& json, std::vector<NaraVisionBox>& boxes);
};
