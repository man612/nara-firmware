#pragma once

#include <functional>
#include <string>

#include <network_provisioning/manager.h>

class NaraSecureBleProvisioner {
public:
    struct Callbacks {
        std::function<void(const std::string& qr_payload)> on_qr;
        std::function<void()> on_success;
        std::function<void(const std::string& error)> on_error;
    };

    explicit NaraSecureBleProvisioner(Callbacks callbacks);
    ~NaraSecureBleProvisioner();

    bool Start();
    void Cancel();
    bool active() const { return active_; }

private:
    static void EventCallback(
        void* user_data,
        network_prov_cb_event_t event,
        void* event_data);
    void HandleEvent(
        network_prov_cb_event_t event,
        void* event_data);
    bool PrepareCredentials();
    void CleanupCredentials();
    void Finish(bool success, const std::string& error = {});

    Callbacks callbacks_;
    bool active_ = false;
    bool success_ = false;
    std::string service_name_;
    std::string username_;
    std::string password_;
    std::string pending_ssid_;
    std::string pending_password_;
    char* salt_ = nullptr;
    int salt_len_ = 0;
    char* verifier_ = nullptr;
    int verifier_len_ = 0;
};
