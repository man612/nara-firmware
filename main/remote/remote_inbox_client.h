#pragma once

#include <string>

enum class NaraRemoteInboxKind {
    None,
    Notify,
    Voice,
};

struct NaraRemoteInboxItem {
    NaraRemoteInboxKind kind = NaraRemoteInboxKind::None;
    std::string id;
    std::string mode;
    std::string text;
    std::string emotion;
    std::string sound;
};

class NaraRemoteInboxClient {
public:
    bool Poll(NaraRemoteInboxItem& item) const;
    bool Ack(const std::string& id) const;

private:
    static std::string GatewayHttpBase();
};
