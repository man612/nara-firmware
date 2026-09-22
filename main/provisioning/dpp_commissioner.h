#pragma once

#include <functional>
#include <mutex>
#include <string>

#include <esp_event.h>

class NaraDppCommissioner {
public:
    struct Callbacks {
        std::function<void(const std::string&)> on_uri;
        std::function<void()> on_success;
        std::function<void(const std::string&)> on_error;
    };

    explicit NaraDppCommissioner(Callbacks callbacks);
    ~NaraDppCommissioner();

    bool Start(
        const std::string& channels = "6",
        const std::string& device_info = "Nara");
    void Cancel();
    bool active() const;

private:
    mutable std::mutex mutex_;
    Callbacks callbacks_;
    esp_event_handler_instance_t wifi_handler_ = nullptr;
    bool active_ = false;
    bool finishing_ = false;
    int retry_count_ = 0;

    static void WifiEventThunk(
        void* arg,
        esp_event_base_t event_base,
        int32_t event_id,
        void* event_data);
    void HandleWifiEvent(int32_t event_id, void* event_data);
    void FinishAsync(bool success, std::string error = "");
    void Cleanup();
};
