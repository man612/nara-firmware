#include "dpp_commissioner.h"

#include <esp_dpp.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <ssid_manager.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <cstring>
#include <utility>

namespace {
constexpr const char* TAG = "NaraDPP";
constexpr int kMaxAuthRetries = 4;

std::string WifiField(
    const uint8_t* value,
    size_t max_length) {
    size_t length = 0;
    while (length < max_length && value[length] != 0) {
        ++length;
    }
    return std::string(
        reinterpret_cast<const char*>(value),
        length);
}
}  // namespace

NaraDppCommissioner::NaraDppCommissioner(
    Callbacks callbacks)
    : callbacks_(std::move(callbacks)) {}

NaraDppCommissioner::~NaraDppCommissioner() {
    Cancel();
}

bool NaraDppCommissioner::active() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return active_;
}

bool NaraDppCommissioner::Start(
    const std::string& channels,
    const std::string& device_info) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (active_ || finishing_) {
        return false;
    }
    if (channels.empty()) {
        return false;
    }

    retry_count_ = 0;
    finishing_ = false;

    esp_err_t err = esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &NaraDppCommissioner::WifiEventThunk,
        this,
        &wifi_handler_);
    if (err != ESP_OK) {
        ESP_LOGE(
            TAG, "DPP event handler failed: %s",
            esp_err_to_name(err));
        wifi_handler_ = nullptr;
        return false;
    }

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        Cleanup();
        return false;
    }

    err = esp_supp_dpp_init();
    if (err != ESP_OK) {
        ESP_LOGE(
            TAG, "DPP init failed: %s",
            esp_err_to_name(err));
        Cleanup();
        return false;
    }

    err = esp_supp_dpp_bootstrap_gen(
        channels.c_str(),
        DPP_BOOTSTRAP_QR_CODE,
        nullptr,
        device_info.empty()
            ? nullptr
            : device_info.c_str());
    if (err != ESP_OK) {
        ESP_LOGE(
            TAG, "DPP bootstrap failed: %s",
            esp_err_to_name(err));
        esp_supp_dpp_deinit();
        Cleanup();
        return false;
    }

    active_ = true;
    err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(
            TAG, "DPP Wi-Fi start failed: %s",
            esp_err_to_name(err));
        active_ = false;
        esp_supp_dpp_deinit();
        Cleanup();
        return false;
    }

    ESP_LOGI(
        TAG, "DPP commissioning started on channel(s) %s",
        channels.c_str());
    return true;
}

void NaraDppCommissioner::Cancel() {
    bool should_cleanup = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!active_ && !finishing_) {
            return;
        }
        active_ = false;
        finishing_ = false;
        should_cleanup = true;
    }

    if (should_cleanup) {
        esp_supp_dpp_stop_listen();
        esp_supp_dpp_deinit();
        esp_wifi_stop();
        Cleanup();
    }
}

void NaraDppCommissioner::WifiEventThunk(
    void* arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void* event_data) {
    if (event_base != WIFI_EVENT || arg == nullptr) {
        return;
    }
    static_cast<NaraDppCommissioner*>(arg)
        ->HandleWifiEvent(event_id, event_data);
}

void NaraDppCommissioner::HandleWifiEvent(
    int32_t event_id,
    void* event_data) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!active_ || finishing_) {
            return;
        }
    }

    switch (event_id) {
        case WIFI_EVENT_STA_START: {
            const esp_err_t err =
                esp_supp_dpp_start_listen();
            if (err != ESP_OK) {
                FinishAsync(
                    false,
                    "DPP listen failed: " +
                        std::string(esp_err_to_name(err)));
            }
            return;
        }

        case WIFI_EVENT_DPP_URI_READY: {
            auto* ready =
                static_cast<wifi_event_dpp_uri_ready_t*>(
                    event_data);
            if (ready != nullptr && ready->uri != nullptr) {
                const std::string uri(
                    reinterpret_cast<const char*>(ready->uri));
                ESP_LOGI(TAG, "DPP QR URI ready");
                if (callbacks_.on_uri) {
                    callbacks_.on_uri(uri);
                }
            }
            return;
        }

        case WIFI_EVENT_DPP_CFG_RECVD: {
            auto* received =
                static_cast<wifi_event_dpp_config_received_t*>(
                    event_data);
            if (received == nullptr) {
                FinishAsync(
                    false,
                    "DPP configuration event was empty");
                return;
            }

            const std::string ssid = WifiField(
                received->wifi_cfg.sta.ssid,
                sizeof(received->wifi_cfg.sta.ssid));
            const std::string password = WifiField(
                received->wifi_cfg.sta.password,
                sizeof(received->wifi_cfg.sta.password));
            if (ssid.empty()) {
                FinishAsync(
                    false,
                    "DPP returned an empty SSID");
                return;
            }

            // Persist through the same Nara saved-network store used by
            // normal scanning/reconnect. Do not log the password.
            SsidManager::GetInstance().AddSsid(
                ssid, password);
            ESP_LOGI(
                TAG, "DPP received Wi-Fi credentials for %s",
                ssid.c_str());
            FinishAsync(true);
            return;
        }

        case WIFI_EVENT_DPP_FAILED: {
            auto* failed =
                static_cast<wifi_event_dpp_failed_t*>(
                    event_data);
            const esp_err_t reason =
                failed != nullptr
                    ? failed->failure_reason
                    : ESP_ERR_DPP_FAILURE;

            bool retry = false;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (
                    active_ &&
                    !finishing_ &&
                    retry_count_ < kMaxAuthRetries) {
                    ++retry_count_;
                    retry = true;
                }
            }

            if (retry) {
                ESP_LOGW(
                    TAG,
                    "DPP authentication failed (%s), retry %d/%d",
                    esp_err_to_name(reason),
                    retry_count_,
                    kMaxAuthRetries);
                const esp_err_t err =
                    esp_supp_dpp_start_listen();
                if (err != ESP_OK) {
                    FinishAsync(
                        false,
                        "DPP retry failed: " +
                            std::string(
                                esp_err_to_name(err)));
                }
            } else {
                FinishAsync(
                    false,
                    "DPP authentication failed: " +
                        std::string(
                            esp_err_to_name(reason)));
            }
            return;
        }

        default:
            return;
    }
}

void NaraDppCommissioner::FinishAsync(
    bool success,
    std::string error) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!active_ || finishing_) {
            return;
        }
        finishing_ = true;
    }

    struct FinishContext {
        NaraDppCommissioner* self;
        bool success;
        std::string error;
    };

    auto* context = new (std::nothrow) FinishContext{
        this,
        success,
        std::move(error)
    };
    if (context == nullptr) {
        ESP_LOGE(TAG, "Could not allocate DPP finish task");
        return;
    }

    if (xTaskCreate(
            [](void* raw) {
                std::unique_ptr<FinishContext> context(
                    static_cast<FinishContext*>(raw));
                auto* self = context->self;

                esp_supp_dpp_stop_listen();
                esp_supp_dpp_deinit();
                esp_wifi_stop();
                self->Cleanup();

                {
                    std::lock_guard<std::mutex> lock(
                        self->mutex_);
                    self->active_ = false;
                    self->finishing_ = false;
                }

                if (context->success) {
                    if (self->callbacks_.on_success) {
                        self->callbacks_.on_success();
                    }
                } else if (self->callbacks_.on_error) {
                    self->callbacks_.on_error(
                        context->error);
                }
                vTaskDelete(nullptr);
            },
            "nara_dpp_end",
            4096,
            context,
            2,
            nullptr) != pdPASS) {
        delete context;
        std::lock_guard<std::mutex> lock(mutex_);
        finishing_ = false;
        ESP_LOGE(TAG, "Could not create DPP finish task");
    }
}

void NaraDppCommissioner::Cleanup() {
    if (wifi_handler_ != nullptr) {
        esp_event_handler_instance_unregister(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            wifi_handler_);
        wifi_handler_ = nullptr;
    }
}
