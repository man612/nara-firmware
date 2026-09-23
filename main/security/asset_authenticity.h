#pragma once

#include <array>
#include <cstdint>
#include <string>

struct AssetPackAuthenticity {
    std::array<uint8_t, 32> expected_sha256{};
    std::array<uint8_t, 64> signature{};
};

bool AssetPublisherKeyConfigured();
bool ParseAssetPackAuthenticity(
    const std::string& sha256_hex,
    const std::string& signature_hex,
    AssetPackAuthenticity& output,
    std::string& error);
bool VerifyAssetPublisherSignature(
    const std::array<uint8_t, 32>& digest,
    const AssetPackAuthenticity& authenticity,
    std::string& error);
