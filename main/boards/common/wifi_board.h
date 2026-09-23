#ifndef WIFI_BOARD_H
#define WIFI_BOARD_H

#include "board.h"
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_timer.h>
#include <memory>
#include <string>

#if CONFIG_ESP_WIFI_DPP_SUPPORT
#include "provisioning/dpp_commissioner.h"
#endif
#if CONFIG_USE_SECURE_BLE_WIFI_PROVISIONING
#include "provisioning/secure_ble_provisioner.h"
#endif

class WifiBoard : public Board {
protected:
    esp_timer_handle_t connect_timer_ = nullptr;
    bool in_config_mode_ = false;
    NetworkEventCallback network_event_callback_ = nullptr;

#if CONFIG_ESP_WIFI_DPP_SUPPORT
    std::unique_ptr<NaraDppCommissioner> dpp_commissioner_;
    virtual void OnDppUriReady(const std::string& uri);
    virtual void OnDppConfigFinished(bool success);
    void StartDppConfigMode();
#endif
#if CONFIG_USE_SECURE_BLE_WIFI_PROVISIONING
    std::unique_ptr<NaraSecureBleProvisioner> secure_ble_provisioner_;
    virtual void OnSecureProvisioningQrReady(
        const std::string& payload);
    virtual void OnSecureProvisioningFinished(bool success);
    void StartSecureBleConfigMode();
#endif

    virtual std::string GetBoardJson() override;

    /**
     * Handle network event (called from WiFi manager callbacks)
     * @param event The network event type
     * @param data Additional data (e.g., SSID for Connecting/Connected events)
     */
    void OnNetworkEvent(NetworkEvent event, const std::string& data = "");

    /**
     * Start WiFi connection attempt
     */
    void TryWifiConnect();

    /**
     * Enter WiFi configuration mode
     */
    void StartWifiConfigMode();

    /**
     * WiFi connection timeout callback
     */
    static void OnWifiConnectTimeout(void* arg);

public:
    WifiBoard();
    virtual ~WifiBoard();
    
    virtual std::string GetBoardType() override;
    
    /**
     * Start network connection asynchronously
     * This function returns immediately. Network events are notified through the callback set by SetNetworkEventCallback().
     */
    virtual void StartNetwork() override;
    
    virtual NetworkInterface* GetNetwork() override;
    virtual void SetNetworkEventCallback(NetworkEventCallback callback) override;
    virtual const char* GetNetworkStateIcon() override;
    virtual void SetPowerSaveLevel(PowerSaveLevel level) override;
    virtual AudioCodec* GetAudioCodec() override { return nullptr; }
    virtual std::string GetDeviceStatusJson() override;
    
    /**
     * Enter WiFi configuration mode (thread-safe, can be called from any task)
     */
    void EnterWifiConfigMode();

#if CONFIG_ESP_WIFI_DPP_SUPPORT
    /**
     * Enter Wi-Fi Easy Connect (DPP) QR commissioning. This is preferred over
     * the inherited open/plain-HTTP configuration portal on capable phones.
     */
    void EnterDppConfigMode();
    bool IsDppConfigMode() const;
#endif
    
    /**
     * Check if in WiFi config mode
     */
    bool IsInWifiConfigMode() const;
};

#endif // WIFI_BOARD_H
