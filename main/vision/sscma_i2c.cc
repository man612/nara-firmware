#include "sscma_i2c.h"

#include <algorithm>
#include <cstring>

#include <cJSON.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace {
constexpr char TAG[] = "SscmaVision";
constexpr size_t kMaxTransportChunk = 250;
constexpr size_t kMaxResponseBytes = 8 * 1024;
constexpr uint8_t kTransportFeature = 0x10;
constexpr uint8_t kRead = 0x01;
constexpr uint8_t kWrite = 0x02;
constexpr uint8_t kAvailable = 0x03;
}  // namespace

SscmaI2cVisionSensor::SscmaI2cVisionSensor(i2c_master_bus_handle_t bus) {
    if (bus == nullptr) return;

    const i2c_device_config_t config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = kDefaultAddress,
        .scl_speed_hz = 400000,
        .scl_wait_us = 0,
        .flags = {},
    };
    if (i2c_master_bus_add_device(bus, &config, &device_) != ESP_OK) {
        device_ = nullptr;
    }
}

SscmaI2cVisionSensor::~SscmaI2cVisionSensor() {
    if (device_ != nullptr) {
        i2c_master_bus_rm_device(device_);
        device_ = nullptr;
    }
}

bool SscmaI2cVisionSensor::Probe() {
    if (device_ == nullptr) return false;
    const int available = Available();
    if (available < 0) {
        ESP_LOGI(TAG, "No SSCMA I2C sensor detected at 0x%02x", kDefaultAddress);
        return false;
    }
    ESP_LOGI(TAG, "SSCMA vision sensor detected at 0x%02x", kDefaultAddress);
    return true;
}

bool SscmaI2cVisionSensor::WriteCommand(const std::string& command) {
    if (device_ == nullptr || command.empty() || command.size() > kMaxTransportChunk) {
        return false;
    }

    std::vector<uint8_t> packet(4 + command.size());
    packet[0] = kTransportFeature;
    packet[1] = kWrite;
    packet[2] = static_cast<uint8_t>((command.size() >> 8) & 0xff);
    packet[3] = static_cast<uint8_t>(command.size() & 0xff);
    std::memcpy(packet.data() + 4, command.data(), command.size());

    return i2c_master_transmit(device_, packet.data(), packet.size(), 100) == ESP_OK;
}

int SscmaI2cVisionSensor::Available() {
    if (device_ == nullptr) return -1;

    const uint8_t command[] = {
        kTransportFeature, kAvailable, 0x00, 0x00, 0x00, 0x00
    };
    uint8_t response[2] = {};
    if (i2c_master_transmit_receive(
            device_, command, sizeof(command), response, sizeof(response), 100) != ESP_OK) {
        return -1;
    }
    return (static_cast<int>(response[0]) << 8) | response[1];
}

bool SscmaI2cVisionSensor::ReadBytes(uint8_t* output, size_t length) {
    if (device_ == nullptr || output == nullptr || length == 0 ||
        length > kMaxTransportChunk) {
        return false;
    }

    const uint8_t command[] = {
        kTransportFeature,
        kRead,
        static_cast<uint8_t>((length >> 8) & 0xff),
        static_cast<uint8_t>(length & 0xff),
        0x00,
        0x00
    };
    return i2c_master_transmit_receive(
               device_, command, sizeof(command), output, length, 100) == ESP_OK;
}

bool SscmaI2cVisionSensor::ParseEvent(
    const std::string& json,
    std::vector<NaraVisionBox>& boxes) {
    cJSON* root = cJSON_ParseWithLength(json.data(), json.size());
    if (root == nullptr) return false;

    bool matched = false;
    const cJSON* type = cJSON_GetObjectItemCaseSensitive(root, "type");
    const cJSON* name = cJSON_GetObjectItemCaseSensitive(root, "name");
    const cJSON* data = cJSON_GetObjectItemCaseSensitive(root, "data");

    if (cJSON_IsNumber(type) && type->valueint == 1 &&
        cJSON_IsString(name) && std::strcmp(name->valuestring, "INVOKE") == 0 &&
        cJSON_IsObject(data)) {
        matched = true;
        boxes.clear();

        const cJSON* raw_boxes = cJSON_GetObjectItemCaseSensitive(data, "boxes");
        if (cJSON_IsArray(raw_boxes)) {
            const cJSON* raw_box = nullptr;
            cJSON_ArrayForEach(raw_box, raw_boxes) {
                if (!cJSON_IsArray(raw_box) || cJSON_GetArraySize(raw_box) < 6) continue;

                const cJSON* x = cJSON_GetArrayItem(raw_box, 0);
                const cJSON* y = cJSON_GetArrayItem(raw_box, 1);
                const cJSON* w = cJSON_GetArrayItem(raw_box, 2);
                const cJSON* h = cJSON_GetArrayItem(raw_box, 3);
                const cJSON* score = cJSON_GetArrayItem(raw_box, 4);
                const cJSON* target = cJSON_GetArrayItem(raw_box, 5);
                if (!cJSON_IsNumber(x) || !cJSON_IsNumber(y) ||
                    !cJSON_IsNumber(w) || !cJSON_IsNumber(h) ||
                    !cJSON_IsNumber(score) || !cJSON_IsNumber(target)) {
                    continue;
                }

                boxes.push_back({
                    .x = static_cast<float>(x->valuedouble),
                    .y = static_cast<float>(y->valuedouble),
                    .w = static_cast<float>(w->valuedouble),
                    .h = static_cast<float>(h->valuedouble),
                    .score = static_cast<float>(score->valuedouble),
                    .target = target->valueint,
                });
            }
        }
    }

    cJSON_Delete(root);
    return matched;
}

bool SscmaI2cVisionSensor::ReadResponse(
    std::vector<NaraVisionBox>& boxes,
    uint32_t timeout_ms) {
    const int64_t deadline =
        esp_timer_get_time() + static_cast<int64_t>(timeout_ms) * 1000;

    while (esp_timer_get_time() < deadline) {
        int available = Available();
        if (available < 0) return false;
        if (available == 0) {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }

        while (available > 0) {
            const size_t chunk =
                std::min<size_t>(static_cast<size_t>(available), kMaxTransportChunk);
            uint8_t buffer[kMaxTransportChunk];
            if (!ReadBytes(buffer, chunk)) return false;

            receive_buffer_.append(
                reinterpret_cast<const char*>(buffer), chunk);
            if (receive_buffer_.size() > kMaxResponseBytes) {
                receive_buffer_.erase(
                    0, receive_buffer_.size() - kMaxResponseBytes);
            }
            available -= static_cast<int>(chunk);
        }

        while (true) {
            const size_t start = receive_buffer_.find("\r{");
            if (start == std::string::npos) break;
            const size_t end = receive_buffer_.find("}\n", start + 2);
            if (end == std::string::npos) break;

            const size_t json_start = start + 1;
            const size_t json_length = end - json_start + 1;
            const std::string json =
                receive_buffer_.substr(json_start, json_length);
            receive_buffer_.erase(0, end + 2);

            if (ParseEvent(json, boxes)) {
                return true;
            }
        }
    }

    return false;
}

bool SscmaI2cVisionSensor::Invoke(
    std::vector<NaraVisionBox>& boxes,
    uint32_t timeout_ms) {
    boxes.clear();
    if (!WriteCommand("AT+INVOKE=1,0,1\r\n")) {
        return false;
    }
    return ReadResponse(boxes, timeout_ms);
}
