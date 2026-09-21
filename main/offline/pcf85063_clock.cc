#include "pcf85063_clock.h"

#include <esp_log.h>

#include <array>

namespace {
constexpr uint8_t kAddress = 0x51;
constexpr uint8_t kSecondsRegister = 0x04;
constexpr int kYearOffset = 1970;
constexpr const char* kTag = "nara_rtc";

bool ValidDateTime(
    int year, int month, int day, int hour, int minute, int second) {
    return year >= 2020 && year <= 2069 &&
           month >= 1 && month <= 12 &&
           day >= 1 && day <= 31 &&
           hour >= 0 && hour <= 23 &&
           minute >= 0 && minute <= 59 &&
           second >= 0 && second <= 59;
}
}  // namespace

NaraPcf85063Clock::NaraPcf85063Clock(i2c_master_bus_handle_t bus) {
    if (bus == nullptr) {
        return;
    }
    i2c_device_config_t config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = kAddress,
        .scl_speed_hz = 400000,
        .scl_wait_us = 0,
        .flags = {},
    };
    if (i2c_master_bus_add_device(bus, &config, &device_) != ESP_OK) {
        device_ = nullptr;
    }
}

NaraPcf85063Clock::~NaraPcf85063Clock() {
    if (device_ != nullptr) {
        i2c_master_bus_rm_device(device_);
    }
}

bool NaraPcf85063Clock::Read(
    uint8_t reg, uint8_t* data, size_t length) const {
    return device_ != nullptr && data != nullptr && length > 0 &&
           i2c_master_transmit_receive(
               device_, &reg, 1, data, length, 100) == ESP_OK;
}

bool NaraPcf85063Clock::Write(
    const uint8_t* data, size_t length) const {
    return device_ != nullptr && data != nullptr && length > 0 &&
           i2c_master_transmit(device_, data, length, 100) == ESP_OK;
}

uint8_t NaraPcf85063Clock::DecToBcd(int value) {
    return static_cast<uint8_t>(((value / 10) << 4) | (value % 10));
}

int NaraPcf85063Clock::BcdToDec(uint8_t value) {
    return ((value >> 4) * 10) + (value & 0x0f);
}

bool NaraPcf85063Clock::Probe() {
    std::array<uint8_t, 7> raw{};
    if (!Read(kSecondsRegister, raw.data(), raw.size())) {
        ESP_LOGW(kTag, "PCF85063 unavailable at I2C 0x51");
        return false;
    }

    const int second = BcdToDec(raw[0] & 0x7f);
    const int minute = BcdToDec(raw[1] & 0x7f);
    const int hour = BcdToDec(raw[2] & 0x3f);
    const int day = BcdToDec(raw[3] & 0x3f);
    const int month = BcdToDec(raw[5] & 0x1f);
    const int year = BcdToDec(raw[6]) + kYearOffset;

    if (!ValidDateTime(year, month, day, hour, minute, second)) {
        ESP_LOGI(kTag, "PCF85063 present but time is not initialized");
    } else {
        ESP_LOGI(kTag, "PCF85063 RTC enabled");
    }
    return true;
}

bool NaraPcf85063Clock::ReadEpoch(std::time_t& epoch) const {
    std::array<uint8_t, 7> raw{};
    if (!Read(kSecondsRegister, raw.data(), raw.size())) {
        return false;
    }

    std::tm value{};
    const int second = BcdToDec(raw[0] & 0x7f);
    const int minute = BcdToDec(raw[1] & 0x7f);
    const int hour = BcdToDec(raw[2] & 0x3f);
    const int day = BcdToDec(raw[3] & 0x3f);
    const int month = BcdToDec(raw[5] & 0x1f);
    const int year = BcdToDec(raw[6]) + kYearOffset;

    if (!ValidDateTime(year, month, day, hour, minute, second)) {
        return false;
    }

    value.tm_sec = second;
    value.tm_min = minute;
    value.tm_hour = hour;
    value.tm_mday = day;
    value.tm_mon = month - 1;
    value.tm_year = year - 1900;
    value.tm_isdst = 0;

    epoch = timegm(&value);
    return epoch >= 1577836800;  // 2020-01-01 UTC.
}

bool NaraPcf85063Clock::WriteEpoch(std::time_t epoch) const {
    if (device_ == nullptr || epoch < 1577836800) {
        return false;
    }

    std::tm value{};
    if (gmtime_r(&epoch, &value) == nullptr) {
        return false;
    }

    const int year = value.tm_year + 1900;
    const int encoded_year = year - kYearOffset;
    if (encoded_year < 0 || encoded_year > 99) {
        return false;
    }

    const std::array<uint8_t, 8> data = {
        kSecondsRegister,
        DecToBcd(value.tm_sec),
        DecToBcd(value.tm_min),
        DecToBcd(value.tm_hour),
        DecToBcd(value.tm_mday),
        DecToBcd(value.tm_wday),
        DecToBcd(value.tm_mon + 1),
        DecToBcd(encoded_year),
    };
    return Write(data.data(), data.size());
}
