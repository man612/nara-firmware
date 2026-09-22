#ifndef NARA_BINARY_FRAME_PARSER_H
#define NARA_BINARY_FRAME_PARSER_H

#include <cstddef>
#include <cstdint>

struct NaraBinaryAudioFrameView {
    uint32_t timestamp = 0;
    const uint8_t* payload = nullptr;
    size_t payload_size = 0;
};

inline uint16_t NaraReadBe16(const uint8_t* data) {
    return static_cast<uint16_t>(
        (static_cast<uint16_t>(data[0]) << 8) |
        static_cast<uint16_t>(data[1]));
}

inline uint32_t NaraReadBe32(const uint8_t* data) {
    return
        (static_cast<uint32_t>(data[0]) << 24) |
        (static_cast<uint32_t>(data[1]) << 16) |
        (static_cast<uint32_t>(data[2]) << 8) |
        static_cast<uint32_t>(data[3]);
}

inline bool NaraParseBinaryAudioV2(
    const uint8_t* data,
    size_t length,
    NaraBinaryAudioFrameView& frame) {
    constexpr size_t kHeaderSize = 16;
    constexpr uint16_t kProtocolVersion = 2;
    constexpr uint16_t kAudioType = 0;

    frame = {};
    if (data == nullptr || length < kHeaderSize) {
        return false;
    }

    const uint16_t version = NaraReadBe16(data);
    const uint16_t type = NaraReadBe16(data + 2);
    const uint32_t payload_size = NaraReadBe32(data + 12);
    if (
        version != kProtocolVersion ||
        type != kAudioType ||
        payload_size == 0 ||
        static_cast<size_t>(payload_size) != length - kHeaderSize) {
        return false;
    }

    frame.timestamp = NaraReadBe32(data + 8);
    frame.payload = data + kHeaderSize;
    frame.payload_size = payload_size;
    return true;
}

inline bool NaraParseBinaryAudioV3(
    const uint8_t* data,
    size_t length,
    NaraBinaryAudioFrameView& frame) {
    constexpr size_t kHeaderSize = 4;
    constexpr uint8_t kAudioType = 0;

    frame = {};
    if (data == nullptr || length < kHeaderSize) {
        return false;
    }

    const uint8_t type = data[0];
    const uint16_t payload_size = NaraReadBe16(data + 2);
    if (
        type != kAudioType ||
        payload_size == 0 ||
        static_cast<size_t>(payload_size) != length - kHeaderSize) {
        return false;
    }

    frame.payload = data + kHeaderSize;
    frame.payload_size = payload_size;
    return true;
}

#endif  // NARA_BINARY_FRAME_PARSER_H
