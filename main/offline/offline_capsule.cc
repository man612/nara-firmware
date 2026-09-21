#include "offline_capsule.h"

#include <algorithm>
#include <cctype>
#include <unordered_set>

#include <cJSON.h>

namespace {
constexpr size_t kMaxFacts = 64;
constexpr size_t kMaxText = 640;
constexpr size_t kMaxAliases = 16;

bool CopyJsonString(
    const cJSON* object,
    const char* key,
    std::string& output,
    size_t max_length,
    bool required = true) {
    const cJSON* value = cJSON_GetObjectItemCaseSensitive(object, key);
    if (!cJSON_IsString(value) || value->valuestring == nullptr) {
        return !required;
    }
    output.assign(value->valuestring);
    return !output.empty() && output.size() <= max_length;
}
}  // namespace

void NaraOfflineCapsule::Clear() {
    loaded_ = false;
    revision_.clear();
    recipient_person_id_.clear();
    subject_person_id_.clear();
    facts_.clear();
}

bool NaraOfflineCapsule::LoadFromJson(const char* data, size_t size) {
    Clear();
    if (data == nullptr || size == 0 || size > 128 * 1024) {
        return false;
    }

    cJSON* root = cJSON_ParseWithLength(data, size);
    if (root == nullptr) {
        return false;
    }

    bool ok = false;
    do {
        const cJSON* version = cJSON_GetObjectItemCaseSensitive(root, "version");
        const cJSON* facts = cJSON_GetObjectItemCaseSensitive(root, "facts");
        if (!cJSON_IsNumber(version) || version->valueint != 1 ||
            !cJSON_IsArray(facts)) {
            break;
        }

        if (!CopyJsonString(root, "revision", revision_, 64) ||
            revision_.size() != 64 ||
            !CopyJsonString(
                root, "recipientPersonId", recipient_person_id_, 128) ||
            !CopyJsonString(
                root, "subjectPersonId", subject_person_id_, 128)) {
            break;
        }

        const int count = cJSON_GetArraySize(facts);
        if (count < 0 || static_cast<size_t>(count) > kMaxFacts) {
            break;
        }

        facts_.reserve(static_cast<size_t>(count));
        const cJSON* item = nullptr;
        cJSON_ArrayForEach(item, facts) {
            if (!cJSON_IsObject(item)) {
                facts_.clear();
                break;
            }

            NaraOfflineCapsuleFact fact;
            if (!CopyJsonString(item, "id", fact.id, 128) ||
                !CopyJsonString(item, "kind", fact.kind, 64) ||
                !CopyJsonString(item, "text", fact.text, kMaxText)) {
                facts_.clear();
                break;
            }

            const cJSON* aliases =
                cJSON_GetObjectItemCaseSensitive(item, "aliases");
            if (!cJSON_IsArray(aliases) ||
                cJSON_GetArraySize(aliases) > static_cast<int>(kMaxAliases)) {
                facts_.clear();
                break;
            }

            const cJSON* alias = nullptr;
            cJSON_ArrayForEach(alias, aliases) {
                if (!cJSON_IsString(alias) ||
                    alias->valuestring == nullptr ||
                    alias->valuestring[0] == '\0') {
                    facts_.clear();
                    break;
                }
                std::string value(alias->valuestring);
                if (value.size() > 64) {
                    facts_.clear();
                    break;
                }
                fact.aliases.push_back(std::move(value));
            }
            if (fact.aliases.size() !=
                static_cast<size_t>(cJSON_GetArraySize(aliases))) {
                facts_.clear();
                break;
            }

            facts_.push_back(std::move(fact));
        }

        if (facts_.size() != static_cast<size_t>(count)) {
            break;
        }

        ok = true;
    } while (false);

    cJSON_Delete(root);
    loaded_ = ok;
    if (!ok) {
        Clear();
    }
    return ok;
}

std::string NaraOfflineCapsule::Lower(std::string_view value) {
    std::string output;
    output.reserve(value.size());
    for (unsigned char ch : value) {
        output.push_back(static_cast<char>(std::tolower(ch)));
    }
    return output;
}

std::vector<std::string> NaraOfflineCapsule::Tokenize(
    std::string_view value) {
    std::vector<std::string> tokens;
    std::string current;
    for (unsigned char ch : value) {
        if (std::isalnum(ch)) {
            current.push_back(static_cast<char>(std::tolower(ch)));
        } else if (!current.empty()) {
            tokens.push_back(std::move(current));
            current.clear();
        }
    }
    if (!current.empty()) {
        tokens.push_back(std::move(current));
    }
    return tokens;
}

std::vector<const NaraOfflineCapsuleFact*> NaraOfflineCapsule::Search(
    std::string_view query,
    size_t limit) const {
    std::vector<const NaraOfflineCapsuleFact*> result;
    if (!loaded_ || facts_.empty() || limit == 0) {
        return result;
    }

    limit = std::min<size_t>(10, limit);
    const std::string normalized = Lower(query);
    const auto tokens = Tokenize(query);

    struct ScoredFact {
        const NaraOfflineCapsuleFact* fact = nullptr;
        int score = 0;
    };
    std::vector<ScoredFact> scored;
    scored.reserve(facts_.size());

    for (const auto& fact : facts_) {
        std::string haystack = Lower(fact.kind);
        haystack.push_back(' ');
        haystack += Lower(fact.text);
        for (const auto& alias : fact.aliases) {
            haystack.push_back(' ');
            haystack += Lower(alias);
        }

        int score = 0;
        if (!normalized.empty() &&
            haystack.find(normalized) != std::string::npos) {
            score += 4;
        }
        for (const auto& token : tokens) {
            if (haystack.find(token) != std::string::npos) {
                ++score;
            }
        }
        if (tokens.empty()) {
            score = 1;
        }
        if (score > 0) {
            scored.push_back({.fact = &fact, .score = score});
        }
    }

    std::sort(
        scored.begin(), scored.end(),
        [](const ScoredFact& left, const ScoredFact& right) {
            if (left.score != right.score) {
                return left.score > right.score;
            }
            return left.fact->id < right.fact->id;
        });

    for (size_t i = 0; i < scored.size() && i < limit; ++i) {
        result.push_back(scored[i].fact);
    }
    return result;
}

const NaraOfflineCapsuleFact* NaraOfflineCapsule::At(size_t index) const {
    if (!loaded_ || index >= facts_.size()) {
        return nullptr;
    }
    return &facts_[index];
}
