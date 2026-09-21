#include "remote_inbox_client.h"

#include "board.h"
#include "settings.h"
#include "system_info.h"
#include "sdkconfig.h"

#include <cJSON.h>
#include <esp_log.h>

#include <memory>
#include <string>

namespace {
constexpr const char* TAG = "RemoteInbox";

std::string GatewayToken() {
    Settings settings("websocket", false);
    std::string token = settings.GetString("token");
#ifdef CONFIG_NARA_GATEWAY_TOKEN
    if (token.empty()) token = CONFIG_NARA_GATEWAY_TOKEN;
#endif
    return token;
}

void ConfigureHttp(Http& http) {
    std::string token = GatewayToken();
    if (!token.empty()) {
        if (token.find(' ') == std::string::npos) {
            token = "Bearer " + token;
        }
        http.SetHeader("Authorization", token);
    }
    http.SetHeader("Device-Id", SystemInfo::GetMacAddress().c_str());
    http.SetHeader("Client-Id", Board::GetInstance().GetUuid());
    http.SetHeader("Cache-Control", "no-store");
    http.SetHeader("Content-Type", "application/json");
}

bool ParsePollBody(
    const std::string& body,
    NaraRemoteInboxItem& item) {
    item = {};
    cJSON* root = cJSON_ParseWithLength(body.data(), body.size());
    if (root == nullptr) {
        ESP_LOGW(TAG, "Invalid inbox JSON");
        return false;
    }

    auto cleanup = std::unique_ptr<cJSON, decltype(&cJSON_Delete)>(
        root, cJSON_Delete);
    auto ok = cJSON_GetObjectItem(root, "ok");
    if (!cJSON_IsBool(ok) || !cJSON_IsTrue(ok)) {
        return false;
    }

    auto raw = cJSON_GetObjectItem(root, "item");
    if (raw == nullptr || cJSON_IsNull(raw)) {
        return true;
    }
    if (!cJSON_IsObject(raw)) {
        return false;
    }

    auto id = cJSON_GetObjectItem(raw, "id");
    auto kind = cJSON_GetObjectItem(raw, "kind");
    if (!cJSON_IsString(id) || !cJSON_IsString(kind) ||
        id->valuestring[0] == '\0') {
        return false;
    }
    item.id = id->valuestring;

    if (strcmp(kind->valuestring, "voice") == 0) {
        auto mode = cJSON_GetObjectItem(raw, "mode");
        if (!cJSON_IsString(mode) ||
            (strcmp(mode->valuestring, "ask") != 0 &&
             strcmp(mode->valuestring, "say") != 0)) {
            return false;
        }
        item.kind = NaraRemoteInboxKind::Voice;
        item.mode = mode->valuestring;
        return true;
    }

    if (strcmp(kind->valuestring, "notify") == 0) {
        auto text = cJSON_GetObjectItem(raw, "text");
        if (!cJSON_IsString(text) || text->valuestring[0] == '\0') {
            return false;
        }
        item.kind = NaraRemoteInboxKind::Notify;
        item.text = text->valuestring;

        auto emotion = cJSON_GetObjectItem(raw, "emotion");
        if (cJSON_IsString(emotion)) {
            item.emotion = emotion->valuestring;
        }
        auto sound = cJSON_GetObjectItem(raw, "sound");
        if (cJSON_IsString(sound)) {
            item.sound = sound->valuestring;
        }
        return true;
    }

    return false;
}
}  // namespace

std::string NaraRemoteInboxClient::GatewayHttpBase() {
    Settings diagnostics("remote_inbox", false);
    std::string url = diagnostics.GetString("base_url");
    if (!url.empty()) {
        while (!url.empty() && url.back() == '/') url.pop_back();
        return url;
    }

    Settings websocket("websocket", false);
    url = websocket.GetString("url");
#ifdef CONFIG_NARA_GATEWAY_URL
    if (url.empty()) url = CONFIG_NARA_GATEWAY_URL;
#endif
    if (url.rfind("wss://", 0) == 0) {
        url.replace(0, 6, "https://");
    } else if (url.rfind("ws://", 0) == 0) {
        url.replace(0, 5, "http://");
    } else if (
        url.rfind("https://", 0) != 0 &&
        url.rfind("http://", 0) != 0) {
        return {};
    }

    const size_t scheme = url.find("://");
    const size_t path = url.find('/', scheme + 3);
    if (path != std::string::npos) {
        url.resize(path);
    }
    while (!url.empty() && url.back() == '/') url.pop_back();
    return url;
}

bool NaraRemoteInboxClient::Poll(
    NaraRemoteInboxItem& item) const {
    item = {};
    const std::string base = GatewayHttpBase();
    if (base.empty()) {
        return false;
    }

    auto network = Board::GetInstance().GetNetwork();
    if (network == nullptr) {
        return false;
    }

    auto http = network->CreateHttp(0);
    ConfigureHttp(*http);
    if (auto opened = http->Open(
            "GET", base + "/api/device-inbox/poll");
        !opened) {
        ESP_LOGD(
            TAG, "Inbox poll failed: %s",
            opened.error().ToString().c_str());
        return false;
    }

    const auto status = http->GetStatusCode();
    if (!status || *status != 200) {
        ESP_LOGW(
            TAG, "Inbox poll HTTP status %d",
            status ? *status : -1);
        http->Close();
        return false;
    }

    const std::string body = http->ReadAll();
    http->Close();
    return ParsePollBody(body, item);
}

bool NaraRemoteInboxClient::Ack(
    const std::string& id) const {
    if (id.empty()) return false;
    const std::string base = GatewayHttpBase();
    if (base.empty()) return false;

    auto network = Board::GetInstance().GetNetwork();
    if (network == nullptr) return false;

    cJSON* root = cJSON_CreateObject();
    if (root == nullptr) return false;
    cJSON_AddStringToObject(root, "id", id.c_str());
    char* raw = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (raw == nullptr) return false;
    std::string body(raw);
    cJSON_free(raw);

    auto http = network->CreateHttp(0);
    ConfigureHttp(*http);
    http->SetContent(std::move(body));
    if (auto opened = http->Open(
            "POST", base + "/api/device-inbox/ack");
        !opened) {
        return false;
    }

    const auto status = http->GetStatusCode();
    (void)http->ReadAll();
    http->Close();
    return status && *status == 200;
}
