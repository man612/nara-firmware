#include "offline_capsule.h"

#include <cassert>
#include <string>

int main() {
    const std::string json = R"JSON({
      "version": 1,
      "recipientPersonId": "person:recipient",
      "subjectPersonId": "person:creator",
      "generatedAt": "2026-09-21T12:00:00.000Z",
      "revision": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
      "facts": [
        {
          "id": "fact-1",
          "kind": "hobby",
          "text": "Likes astronomy and small electronics projects",
          "aliases": ["space", "electronics", "maker"]
        },
        {
          "id": "fact-2",
          "kind": "message",
          "text": "Remember to eat before leaving",
          "aliases": ["note", "reminder"]
        }
      ]
    })JSON";

    NaraOfflineCapsule capsule;
    assert(capsule.LoadFromJson(json.data(), json.size()));
    assert(capsule.loaded());
    assert(capsule.size() == 2);
    assert(capsule.recipient_person_id() == "person:recipient");

    const auto search = capsule.Search("electronics");
    assert(search.size() == 1);
    assert(search[0]->id == "fact-1");

    const auto note = capsule.Search("remember eat");
    assert(note.size() == 1);
    assert(note[0]->id == "fact-2");

    assert(capsule.At(0) != nullptr);
    assert(capsule.At(2) == nullptr);

    const std::string bad = R"JSON({
      "version": 1,
      "recipientPersonId": "person:r",
      "subjectPersonId": "person:s",
      "revision": "short",
      "facts": []
    })JSON";
    assert(!capsule.LoadFromJson(bad.data(), bad.size()));

    return 0;
}
