#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

struct NaraOfflineCapsuleFact {
    std::string id;
    std::string kind;
    std::string text;
    std::vector<std::string> aliases;
};

class NaraOfflineCapsule {
public:
    bool LoadFromJson(const char* data, size_t size);
    void Clear();

    bool loaded() const { return loaded_; }
    size_t size() const { return facts_.size(); }
    const std::string& revision() const { return revision_; }
    const std::string& recipient_person_id() const { return recipient_person_id_; }
    const std::string& subject_person_id() const { return subject_person_id_; }

    std::vector<const NaraOfflineCapsuleFact*> Search(
        std::string_view query,
        size_t limit = 5) const;

    const NaraOfflineCapsuleFact* At(size_t index) const;

private:
    static std::vector<std::string> Tokenize(std::string_view value);
    static std::string Lower(std::string_view value);

    bool loaded_ = false;
    std::string revision_;
    std::string recipient_person_id_;
    std::string subject_person_id_;
    std::vector<NaraOfflineCapsuleFact> facts_;
};
