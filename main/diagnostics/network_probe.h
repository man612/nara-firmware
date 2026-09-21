#pragma once

#include <string>

struct NaraNetworkDiagnosticResult {
    bool ok = false;
    int rtt_ms = 0;
    float download_mbps = 0.0f;
    float upload_mbps = 0.0f;
    std::string error;
};

class NaraNetworkProbe {
public:
    NaraNetworkDiagnosticResult RunQuick() const;
    static std::string Describe(const NaraNetworkDiagnosticResult& result);

private:
    static std::string GatewayHttpBase();
};
