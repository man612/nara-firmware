#include "asset_authenticity.h"

#include <algorithm>
#include <array>
#include <cctype>

#include <psa/crypto.h>
#include <sdkconfig.h>

namespace {

bool DecodeHex(
    const std::string& value,
    uint8_t* output,
    size_t output_size) {
    if (value.size() != output_size * 2) {
        return false;
    }

    auto nibble = [](char ch) -> int {
        if (ch >= '0' && ch <= '9') return ch - '0';
        if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
        if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
        return -1;
    };

    for (size_t i = 0; i < output_size; ++i) {
        const int high = nibble(value[i * 2]);
        const int low = nibble(value[i * 2 + 1]);
        if (high < 0 || low < 0) {
            return false;
        }
        output[i] =
            static_cast<uint8_t>((high << 4) | low);
    }
    return true;
}

}  // namespace

bool AssetPublisherKeyConfigured() {
    const std::string key =
        CONFIG_NARA_ASSET_PUBLISHER_PUBLIC_KEY_HEX;
    return key.size() == 130 &&
           key.rfind("04", 0) == 0;
}

bool ParseAssetPackAuthenticity(
    const std::string& sha256_hex,
    const std::string& signature_hex,
    AssetPackAuthenticity& output,
    std::string& error) {
    if (!DecodeHex(
            sha256_hex,
            output.expected_sha256.data(),
            output.expected_sha256.size())) {
        error =
            "asset pack sha256 must be exactly 64 hexadecimal characters";
        return false;
    }

    if (!DecodeHex(
            signature_hex,
            output.signature.data(),
            output.signature.size())) {
        error =
            "asset pack signature must be exactly 128 hexadecimal characters";
        return false;
    }

    return true;
}

bool VerifyAssetPublisherSignature(
    const std::array<uint8_t, 32>& digest,
    const AssetPackAuthenticity& authenticity,
    std::string& error) {
    if (digest != authenticity.expected_sha256) {
        error = "asset pack SHA-256 does not match signed metadata";
        return false;
    }

    const std::string public_key_hex =
        CONFIG_NARA_ASSET_PUBLISHER_PUBLIC_KEY_HEX;
    std::array<uint8_t, 65> public_key{};
    if (!DecodeHex(
            public_key_hex,
            public_key.data(),
            public_key.size()) ||
        public_key[0] != 0x04) {
        error =
            "asset publisher public key is not configured";
        return false;
    }

    if (psa_crypto_init() != PSA_SUCCESS) {
        error = "PSA Crypto initialization failed";
        return false;
    }

    psa_key_attributes_t attributes =
        PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_usage_flags(
        &attributes,
        PSA_KEY_USAGE_VERIFY_HASH);
    psa_set_key_algorithm(
        &attributes,
        PSA_ALG_ECDSA(PSA_ALG_SHA_256));
    psa_set_key_type(
        &attributes,
        PSA_KEY_TYPE_ECC_PUBLIC_KEY(
            PSA_ECC_FAMILY_SECP_R1));
    psa_set_key_bits(&attributes, 256);

    psa_key_id_t key_id = 0;
    const psa_status_t import_status =
        psa_import_key(
            &attributes,
            public_key.data(),
            public_key.size(),
            &key_id);
    psa_reset_key_attributes(&attributes);
    if (import_status != PSA_SUCCESS) {
        error = "failed to import asset publisher public key";
        return false;
    }

    const psa_status_t verify_status =
        psa_verify_hash(
            key_id,
            PSA_ALG_ECDSA(PSA_ALG_SHA_256),
            digest.data(),
            digest.size(),
            authenticity.signature.data(),
            authenticity.signature.size());
    psa_destroy_key(key_id);

    if (verify_status != PSA_SUCCESS) {
        error = "asset publisher signature verification failed";
        return false;
    }

    return true;
}
