#pragma once

#include <cstdint>
#include <ctime>

#include <driver/i2c_master.h>

class NaraPcf85063Clock {
public:
    explicit NaraPcf85063Clock(i2c_master_bus_handle_t bus);
    ~NaraPcf85063Clock();

    NaraPcf85063Clock(const NaraPcf85063Clock&) = delete;
    NaraPcf85063Clock& operator=(const NaraPcf85063Clock&) = delete;

    bool Probe();
    bool ReadEpoch(std::time_t& epoch) const;
    bool WriteEpoch(std::time_t epoch) const;

private:
    i2c_master_dev_handle_t device_ = nullptr;

    bool Read(uint8_t reg, uint8_t* data, size_t length) const;
    bool Write(const uint8_t* data, size_t length) const;

    static uint8_t DecToBcd(int value);
    static int BcdToDec(uint8_t value);
};
