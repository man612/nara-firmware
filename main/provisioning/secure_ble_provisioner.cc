#include "secure_ble_provisioner.h"

#include "ssid_manager.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>

#include <crypto/srp6a/esp_srp.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_random.h>
#include <network_provisioning/scheme_ble.h>

static const char* TAG = "NaraSecureProv";

namespace {

std::string RandomProof(size_t length) {
    static constexpr char kAlphabet[] =
        "23456789ABCDEFGHJKLMNPQRSTUVWXYZ";
    if (length == 0 || length > 32) {
        return {};
    }

    uint8_t random[32] = {};
    esp_fill_random(random, length);

    std::string value(length, '0');
    for (size_t i = 0; i < length; ++i) {
        value[i] =
            kAlphabet[random[i] % (sizeof(kAlphabet) - 1)];
    }
    return value;
}

std::string ServiceName() {
    uint8_t mac[6] = {};
    if (esp_read_mac(mac, ESP_MAC_WIFI_STA) != ESP_OK) {
        return "Nara-Setup";
    }

    char value[16] = {};
    snprintf(
        value,
        sizeof(value),
        "Nara-%02X%02X%02X",
        mac[3],
        mac[4],
        mac[5]);
    return value;
}

std::string JsonEscape(const std::string& input) {
    std::string out;
    out.reserve(input.size() + 8);
    for (char ch : input) {
        if (ch == '\\' || ch == '"') {
            out.push_back('\\');
        }
        out.push_back(ch);
    }
    return out;
}

void SecureClear(std::string& value) {
    if (!value.empty()) {
        std::fill(value.begin(), value.end(), '\0');
        value.clear();
    }
}

}  // namespace

NaraSecureBleProvisioner::NaraSecureBleProvisioner(
    Callbacks callbacks)
    : callbacks_(std::move(callbacks)) {}

NaraSecureBleProvisioner::~NaraSecureBleProvisioner() {
    if (active_) {
        callbacks_ = {};
        network_prov_mgr_deinit();
        active_ = false;
    }
    CleanupCredentials();
}

bool NaraSecureBleProvisioner::PrepareCredentials() {
    CleanupCredentials();

    service_name_ = ServiceName();
    username_ = "nara";
    password_ = RandomProof(16);
    if (password_.empty()) {
        ESP_LOGE(TAG, "Failed to generate provisioning proof");
        return false;
    }

    constexpr int kSaltLength = 16;
    const esp_err_t err = esp_srp_gen_salt_verifier(
        username_.data(),
        static_cast<int>(username_.size()),
        password_.data(),
        static_cast<int>(password_.size()),
        &salt_,
        kSaltLength,
        &verifier_,
        &verifier_len_);
    if (err != ESP_OK ||
        salt_ == nullptr ||
        verifier_ == nullptr ||
        verifier_len_ <= 0) {
        ESP_LOGE(
            TAG,
            "Security 2 verifier generation failed: %s",
            esp_err_to_name(err));
        CleanupCredentials();
        return false;
    }

    salt_len_ = kSaltLength;
    return true;
}

void NaraSecureBleProvisioner::CleanupCredentials() {
    if (salt_ != nullptr) {
        free(salt_);
        salt_ = nullptr;
    }
    if (verifier_ != nullptr) {
        free(verifier_);
        verifier_ = nullptr;
    }
    salt_len_ = 0;
    verifier_len_ = 0;
    SecureClear(password_);
    SecureClear(pending_password_);
}

bool NaraSecureBleProvisioner::Start() {
    if (active_) {
        return true;
    }
    if (!PrepareCredentials()) {
        return false;
    }

    network_prov_mgr_config_t config = {
        .scheme = network_prov_scheme_ble,
        .scheme_event_handler =
            NETWORK_PROV_EVENT_HANDLER_NONE,
        .app_event_handler = {
            .event_cb = &NaraSecureBleProvisioner::EventCallback,
            .user_data = this,
        },
        .network_prov_wifi_conn_cfg = {
            .wifi_conn_attempts = 3,
        },
    };

    esp_err_t err = network_prov_mgr_init(config);
    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "network_prov_mgr_init failed: %s",
            esp_err_to_name(err));
        CleanupCredentials();
        return false;
    }

    network_prov_security2_params_t security = {
        .salt =
            reinterpret_cast<const uint8_t*>(salt_),
        .salt_len =
            static_cast<uint16_t>(salt_len_),
        .verifier =
            reinterpret_cast<const uint8_t*>(verifier_),
        .verifier_len =
            static_cast<uint16_t>(verifier_len_),
    };

    err = network_prov_mgr_start_provisioning(
        NETWORK_PROV_SECURITY_2,
        &security,
        service_name_.c_str(),
        nullptr);
    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "network_prov_mgr_start_provisioning failed: %s",
            esp_err_to_name(err));
        network_prov_mgr_deinit();
        CleanupCredentials();
        return false;
    }

    active_ = true;
    success_ = false;

    const std::string qr =
        "{\"ver\":\"v1\",\"name\":\"" +
        JsonEscape(service_name_) +
        "\",\"username\":\"" +
        JsonEscape(username_) +
        "\",\"pop\":\"" +
        JsonEscape(password_) +
        "\",\"transport\":\"ble\"}";

    if (callbacks_.on_qr) {
        callbacks_.on_qr(qr);
    }

    // Security 2 authenticates with the salt/verifier. The plaintext
    // proof only needs to survive long enough to be copied into the
    // locally displayed QR payload.
    SecureClear(password_);
    return true;
}

void NaraSecureBleProvisioner::Cancel() {
    if (active_) {
        network_prov_mgr_stop_provisioning();
    }
}

void NaraSecureBleProvisioner::EventCallback(
    void* user_data,
    network_prov_cb_event_t event,
    void* event_data) {
    auto* self =
        static_cast<NaraSecureBleProvisioner*>(user_data);
    if (self != nullptr) {
        self->HandleEvent(event, event_data);
    }
}

void NaraSecureBleProvisioner::HandleEvent(
    network_prov_cb_event_t event,
    void* event_data) {
    switch (event) {
        case NETWORK_PROV_WIFI_CRED_RECV: {
            const auto* credentials =
                static_cast<wifi_sta_config_t*>(event_data);
            if (credentials == nullptr) {
                break;
            }

            pending_ssid_.assign(
                reinterpret_cast<const char*>(
                    credentials->ssid),
                strnlen(
                    reinterpret_cast<const char*>(
                        credentials->ssid),
                    sizeof(credentials->ssid)));
            pending_password_.assign(
                reinterpret_cast<const char*>(
                    credentials->password),
                strnlen(
                    reinterpret_cast<const char*>(
                        credentials->password),
                    sizeof(credentials->password)));

            ESP_LOGI(
                TAG,
                "Received Wi-Fi credentials for SSID '%s'",
                pending_ssid_.c_str());
            break;
        }

        case NETWORK_PROV_WIFI_CRED_SUCCESS:
            success_ = true;
            if (!pending_ssid_.empty()) {
                SsidManager::GetInstance().AddSsid(
                    pending_ssid_,
                    pending_password_);
            }
            ESP_LOGI(
                TAG,
                "Security 2 Wi-Fi provisioning succeeded");
            break;

        case NETWORK_PROV_WIFI_CRED_FAIL:
            ESP_LOGW(
                TAG,
                "Provisioned Wi-Fi credentials did not connect");
            break;

        case NETWORK_PROV_END: {
            const bool success = success_;
            active_ = false;
            network_prov_mgr_deinit();
            Finish(
                success,
                success
                    ? std::string()
                    : "Secure BLE provisioning ended before Wi-Fi was confirmed");
            break;
        }

        default:
            break;
    }
}

void NaraSecureBleProvisioner::Finish(
    bool success,
    const std::string& error) {
    pending_ssid_.clear();
    CleanupCredentials();

    if (success) {
        if (callbacks_.on_success) {
            callbacks_.on_success();
        }
        return;
    }

    if (callbacks_.on_error) {
        callbacks_.on_error(error);
    }
}
