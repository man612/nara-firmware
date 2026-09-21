#include "network_probe.h"

#include "board.h"
#include "settings.h"
#include "system_info.h"
#include "sdkconfig.h"

#include <esp_timer.h>

#include <algorithm>
#include <cstdio>
#include <memory>
#include <string>

namespace {
constexpr size_t kDownloadBytes = 64 * 1024;
constexpr size_t kUploadBytes = 16 * 1024;
constexpr size_t kReadChunk = 4096;

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
}
}  // namespace

std::string NaraNetworkProbe::GatewayHttpBase() {
    Settings diagnostics("diagnostics", false);
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

NaraNetworkDiagnosticResult NaraNetworkProbe::RunQuick() const {
    NaraNetworkDiagnosticResult result;
    const std::string base = GatewayHttpBase();
    if (base.empty()) {
        result.error = "gateway URL is not configured";
        return result;
    }

    auto network = Board::GetInstance().GetNetwork();
    if (network == nullptr) {
        result.error = "network interface is unavailable";
        return result;
    }

    {
        auto http = network->CreateHttp(0);
        ConfigureHttp(*http);
        const int64_t start = esp_timer_get_time();
        if (auto opened = http->Open(
                "GET", base + "/api/diagnostics/ping");
            !opened) {
            result.error = opened.error().ToString();
            return result;
        }
        const auto status = http->GetStatusCode();
        if (!status || *status != 200) {
            result.error = "diagnostic ping failed";
            http->Close();
            return result;
        }
        (void)http->ReadAll();
        result.rtt_ms = static_cast<int>(
            (esp_timer_get_time() - start) / 1000);
        http->Close();
    }

    {
        auto http = network->CreateHttp(0);
        ConfigureHttp(*http);
        const std::string url =
            base + "/api/diagnostics/download?bytes=" +
            std::to_string(kDownloadBytes);
        const int64_t start = esp_timer_get_time();
        if (auto opened = http->Open("GET", url); !opened) {
            result.error = opened.error().ToString();
            return result;
        }
        const auto status = http->GetStatusCode();
        if (!status || *status != 200) {
            result.error = "diagnostic download failed";
            http->Close();
            return result;
        }

        char buffer[kReadChunk];
        size_t received = 0;
        while (true) {
            auto read = http->Read(buffer, sizeof(buffer));
            if (!read) {
                result.error = read.error().ToString();
                http->Close();
                return result;
            }
            if (*read <= 0) break;
            received += static_cast<size_t>(*read);
            if (received > kDownloadBytes) break;
        }
        const int64_t elapsed_us =
            std::max<int64_t>(1, esp_timer_get_time() - start);
        http->Close();
        result.download_mbps =
            static_cast<float>(received * 8.0 / elapsed_us);
    }

    {
        auto http = network->CreateHttp(0);
        ConfigureHttp(*http);
        http->SetHeader("Content-Type", "application/octet-stream");
        std::string payload(kUploadBytes, 'N');
        http->SetContent(std::move(payload));

        const int64_t start = esp_timer_get_time();
        if (auto opened = http->Open(
                "POST", base + "/api/diagnostics/upload");
            !opened) {
            result.error = opened.error().ToString();
            return result;
        }
        const auto status = http->GetStatusCode();
        if (!status || *status != 200) {
            result.error = "diagnostic upload failed";
            http->Close();
            return result;
        }
        (void)http->ReadAll();
        const int64_t elapsed_us =
            std::max<int64_t>(1, esp_timer_get_time() - start);
        http->Close();
        result.upload_mbps =
            static_cast<float>(kUploadBytes * 8.0 / elapsed_us);
    }

    result.ok = true;
    return result;
}

std::string NaraNetworkProbe::Describe(
    const NaraNetworkDiagnosticResult& result) {
    if (!result.ok) {
        return "Network test failed: " + result.error;
    }

    const char* quality =
        result.rtt_ms < 40
            ? "very responsive"
            : result.rtt_ms < 90
                ? "good for realtime voice"
                : result.rtt_ms < 180
                    ? "usable but latency may be noticeable"
                    : "high latency; voice may feel delayed";

    char text[256] = {};
    std::snprintf(
        text, sizeof(text),
        "ping=%d ms (%s); download=%.1f Mbps (~%.1f MB/s); "
        "upload=%.1f Mbps (~%.1f MB/s)",
        result.rtt_ms,
        quality,
        result.download_mbps,
        result.download_mbps / 8.0f,
        result.upload_mbps,
        result.upload_mbps / 8.0f);
    return text;
}
