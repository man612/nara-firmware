#include "binary_frame_parser.h"

#include <cassert>
#include <cstdint>
#include <vector>

int main() {
    {
        const std::vector<uint8_t> frame = {
            0x00, 0x02,  // version
            0x00, 0x00,  // audio type
            0x00, 0x00, 0x00, 0x00,  // reserved
            0x01, 0x02, 0x03, 0x04,  // timestamp
            0x00, 0x00, 0x00, 0x03,  // payload size
            0xaa, 0xbb, 0xcc
        };
        NaraBinaryAudioFrameView parsed;
        assert(NaraParseBinaryAudioV2(frame.data(), frame.size(), parsed));
        assert(parsed.timestamp == 0x01020304U);
        assert(parsed.payload_size == 3);
        assert(parsed.payload[0] == 0xaa);
        assert(parsed.payload[2] == 0xcc);
    }

    {
        std::vector<uint8_t> malformed = {
            0x00, 0x02, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x01,
            0x00, 0x00, 0x00, 0x04,
            0xaa, 0xbb, 0xcc
        };
        NaraBinaryAudioFrameView parsed;
        assert(!NaraParseBinaryAudioV2(malformed.data(), 15, parsed));
        assert(!NaraParseBinaryAudioV2(malformed.data(), malformed.size(), parsed));

        malformed[2] = 0x00;
        malformed[3] = 0x01;
        malformed[15] = 0x03;
        assert(!NaraParseBinaryAudioV2(malformed.data(), malformed.size(), parsed));
    }

    {
        const std::vector<uint8_t> frame = {
            0x00, 0x00, 0x00, 0x02, 0x11, 0x22
        };
        NaraBinaryAudioFrameView parsed;
        assert(NaraParseBinaryAudioV3(frame.data(), frame.size(), parsed));
        assert(parsed.timestamp == 0);
        assert(parsed.payload_size == 2);
        assert(parsed.payload[0] == 0x11);
        assert(parsed.payload[1] == 0x22);

        assert(!NaraParseBinaryAudioV3(frame.data(), 3, parsed));

        auto malformed = frame;
        malformed[2] = 0x00;
        malformed[3] = 0x03;
        assert(!NaraParseBinaryAudioV3(
            malformed.data(), malformed.size(), parsed));

        malformed = frame;
        malformed[0] = 0x01;
        assert(!NaraParseBinaryAudioV3(
            malformed.data(), malformed.size(), parsed));
    }

    return 0;
}
