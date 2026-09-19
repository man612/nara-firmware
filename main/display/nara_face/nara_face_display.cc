#include "nara_face_display.h"
#include "../lvgl_display/lvgl_theme.h"

#include <cstring>
#include <esp_random.h>

namespace {
constexpr uint32_t kFaceFramePeriodMs = 33;

bool ParseEmotion(const char* value, NaraEmotion& emotion) {
    if (value == nullptr) return false;
    if (std::strcmp(value, "neutral") == 0) {
        emotion = NaraEmotion::Neutral;
    } else if (std::strcmp(value, "happy") == 0) {
        emotion = NaraEmotion::Happy;
    } else if (std::strcmp(value, "shy") == 0) {
        emotion = NaraEmotion::Shy;
    } else if (std::strcmp(value, "sad") == 0) {
        emotion = NaraEmotion::Sad;
    } else if (std::strcmp(value, "annoyed") == 0) {
        emotion = NaraEmotion::Annoyed;
    } else if (std::strcmp(value, "surprised") == 0) {
        emotion = NaraEmotion::Surprised;
    } else {
        return false;
    }
    return true;
}

bool ParseInteraction(const char* value, NaraInteractionState& interaction) {
    if (value == nullptr) return false;
    if (std::strcmp(value, "idle") == 0) {
        interaction = NaraInteractionState::Idle;
    } else if (std::strcmp(value, "listening") == 0) {
        interaction = NaraInteractionState::Listening;
    } else if (std::strcmp(value, "thinking") == 0) {
        interaction = NaraInteractionState::Thinking;
    } else if (std::strcmp(value, "speaking") == 0) {
        interaction = NaraInteractionState::Speaking;
    } else if (std::strcmp(value, "sleeping") == 0) {
        interaction = NaraInteractionState::Sleeping;
    } else if (std::strcmp(value, "error") == 0) {
        interaction = NaraInteractionState::Error;
    } else {
        return false;
    }
    return true;
}
}  // namespace

NaraFaceDisplay::NaraFaceDisplay(
    esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel, int width, int height,
    int offset_x, int offset_y, bool mirror_x, bool mirror_y, bool swap_xy)
    : SpiLcdDisplay(panel_io, panel, width, height, offset_x, offset_y, mirror_x, mirror_y, swap_xy),
      controller_(esp_random()) {}

NaraFaceDisplay::~NaraFaceDisplay() {
    DisplayLockGuard lock(this);
    if (face_timer_ != nullptr) {
        lv_timer_delete(face_timer_);
        face_timer_ = nullptr;
    }
    face_view_.reset();
}

void NaraFaceDisplay::SetupUI() {
    LcdDisplay::SetupUI();

    // The Nara face is intentionally white-on-black. Apply the existing dark
    // LCD theme after the base widgets exist so status/network/battery text
    // stays readable above the full-screen face layer.
    if (auto* dark_theme = LvglThemeManager::GetInstance().GetTheme("dark")) {
        LcdDisplay::SetTheme(dark_theme);
    }

    DisplayLockGuard lock(this);
    if (!lock || emoji_box_ == nullptr) {
        return;
    }

    lv_obj_set_size(emoji_box_, width_, height_);
    lv_obj_align(emoji_box_, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(emoji_box_, LV_OBJ_FLAG_SCROLLABLE);

    if (emoji_label_ != nullptr) {
        lv_obj_add_flag(emoji_label_, LV_OBJ_FLAG_HIDDEN);
    }
    if (emoji_image_ != nullptr) {
        lv_obj_add_flag(emoji_image_, LV_OBJ_FLAG_HIDDEN);
    }

    controller_.Reset(lv_tick_get());
    face_view_ = std::make_unique<NaraFaceView>(emoji_box_, width_, height_);
    face_view_->Render(controller_.state());

    face_timer_ = lv_timer_create(
        [](lv_timer_t* timer) {
            auto* display = static_cast<NaraFaceDisplay*>(lv_timer_get_user_data(timer));
            display->RenderFace(lv_tick_get());
        },
        kFaceFramePeriodMs, this);
}

void NaraFaceDisplay::RenderFace(uint32_t now_ms) {
    if (face_view_ == nullptr) {
        return;
    }

    NaraFaceState state;
    {
        std::lock_guard<std::mutex> lock(face_mutex_);
        controller_.Tick(now_ms);
        state = controller_.state();
    }

    face_view_->Render(state);
}

void NaraFaceDisplay::ShowFace() {
    DisplayLockGuard lock(this);
    if (!lock || face_view_ == nullptr) {
        return;
    }

    face_view_->SetVisible(true);
    if (emoji_label_ != nullptr) {
        lv_obj_add_flag(emoji_label_, LV_OBJ_FLAG_HIDDEN);
    }
    if (emoji_image_ != nullptr) {
        lv_obj_add_flag(emoji_image_, LV_OBJ_FLAG_HIDDEN);
    }
}

void NaraFaceDisplay::ShowLegacyEmotion(const char* emotion) {
    {
        DisplayLockGuard lock(this);
        if (lock && face_view_ != nullptr) {
            face_view_->SetVisible(false);
        }
    }
    LcdDisplay::SetEmotion(emotion);
}

void NaraFaceDisplay::SetEmotion(const char* emotion) {
    NaraEmotion parsed;
    if (ParseEmotion(emotion, parsed)) {
        {
            std::lock_guard<std::mutex> lock(face_mutex_);
            controller_.SetEmotion(parsed);
        }
        ShowFace();
        return;
    }

    // Compatibility with legacy semantic values while the rest of the firmware
    // migrates to the explicit interaction channel.
    if (emotion != nullptr && std::strcmp(emotion, "thinking") == 0) {
        SetInteraction("thinking");
        return;
    }
    if (emotion != nullptr && std::strcmp(emotion, "sleepy") == 0) {
        SetInteraction("sleeping");
        return;
    }

    ShowLegacyEmotion(emotion);
}

void NaraFaceDisplay::SetInteraction(const char* interaction) {
    NaraInteractionState parsed;
    if (!ParseInteraction(interaction, parsed)) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(face_mutex_);
        controller_.SetInteraction(parsed);
        if (parsed != NaraInteractionState::Speaking) {
            controller_.SetSpeechLevel(0.0f);
        }
    }
    ShowFace();
}

void NaraFaceDisplay::SetSpeechLevel(float level) {
    std::lock_guard<std::mutex> lock(face_mutex_);
    controller_.SetSpeechLevel(level);
}

void NaraFaceDisplay::SetPowerSaveMode(bool on) {
    SetChatMessage("system", "");
    SetInteraction(on ? "sleeping" : "idle");
    if (!on) {
        SetEmotion("neutral");
    }
}
