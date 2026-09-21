#include "nara_face_display.h"
#include "../lvgl_display/lvgl_theme.h"

#include <cstring>
#include <algorithm>
#include <esp_random.h>
#include <qrcode.h>

namespace {
constexpr uint32_t kFaceFramePeriodMs = 33;
NaraFaceDisplay* g_qr_target = nullptr;

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
    HideQrCode();
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

void NaraFaceDisplay::SetGazeTarget(float x, float y) {
    std::lock_guard<std::mutex> lock(face_mutex_);
    controller_.SetGaze(x, y);
}

void NaraFaceDisplay::ClearGazeTarget() {
    std::lock_guard<std::mutex> lock(face_mutex_);
    controller_.ClearManualGaze();
}

bool NaraFaceDisplay::ShowQrCode(
    const std::string& payload,
    const std::string& caption) {
    if (payload.empty()) return false;

    std::lock_guard<std::mutex> qr_lock(qr_mutex_);
    if (g_qr_target != nullptr) {
        return false;
    }

    g_qr_target = this;
    esp_qrcode_config_t config = ESP_QRCODE_CONFIG_DEFAULT();
    config.max_qrcode_version = 15;
    config.qrcode_ecc_level = ESP_QRCODE_ECC_MED;
    config.display_func = [](esp_qrcode_handle_t handle) {
        if (g_qr_target != nullptr) {
            g_qr_target->RenderQrCode(
                const_cast<uint8_t*>(handle));
        }
    };

    const esp_err_t result =
        esp_qrcode_generate(&config, payload.c_str());
    g_qr_target = nullptr;
    if (result != ESP_OK) {
        HideQrCode();
        return false;
    }

    if (!caption.empty()) {
        DisplayLockGuard lock(this);
        if (lock && qr_caption_ != nullptr) {
            lv_label_set_text(qr_caption_, caption.c_str());
        }
    }
    return true;
}

void NaraFaceDisplay::RenderQrCode(void* raw_handle) {
    auto handle =
        static_cast<esp_qrcode_handle_t>(raw_handle);
    const int modules = esp_qrcode_get_size(handle);
    if (modules <= 0) return;

    constexpr int quiet = 4;
    const int available = std::max(
        64, std::min(width_ - 24, height_ - 72));
    const int scale = std::max(
        1, available / (modules + quiet * 2));
    const int side = (modules + quiet * 2) * scale;

    DisplayLockGuard lock(this);
    if (!lock || emoji_box_ == nullptr) return;

    if (face_view_ != nullptr) {
        face_view_->SetVisible(false);
    }
    if (qr_overlay_ != nullptr) {
        lv_obj_delete(qr_overlay_);
        qr_overlay_ = nullptr;
        qr_canvas_ = nullptr;
        qr_caption_ = nullptr;
        qr_buffer_.clear();
    }

    qr_overlay_ = lv_obj_create(emoji_box_);
    lv_obj_remove_style_all(qr_overlay_);
    lv_obj_set_size(qr_overlay_, width_, height_);
    lv_obj_set_style_bg_color(
        qr_overlay_, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(qr_overlay_, LV_OPA_COVER, 0);
    lv_obj_align(qr_overlay_, LV_ALIGN_CENTER, 0, 0);

    qr_canvas_ = lv_canvas_create(qr_overlay_);
    qr_buffer_.assign(
        static_cast<size_t>(side) *
            static_cast<size_t>(side),
        0xffff);
    lv_canvas_set_buffer(
        qr_canvas_,
        qr_buffer_.data(),
        side,
        side,
        LV_COLOR_FORMAT_RGB565);
    lv_canvas_fill_bg(
        qr_canvas_, lv_color_white(), LV_OPA_COVER);
    lv_obj_align(qr_canvas_, LV_ALIGN_CENTER, 0, -18);

    for (int y = 0; y < modules; ++y) {
        for (int x = 0; x < modules; ++x) {
            if (!esp_qrcode_get_module(handle, x, y)) {
                continue;
            }
            const int px0 = (x + quiet) * scale;
            const int py0 = (y + quiet) * scale;
            for (int py = 0; py < scale; ++py) {
                for (int px = 0; px < scale; ++px) {
                    lv_canvas_set_px(
                        qr_canvas_,
                        px0 + px,
                        py0 + py,
                        lv_color_black(),
                        LV_OPA_COVER);
                }
            }
        }
    }

    qr_caption_ = lv_label_create(qr_overlay_);
    lv_label_set_long_mode(
        qr_caption_, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(qr_caption_, width_ - 28);
    lv_obj_set_style_text_align(
        qr_caption_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(
        qr_caption_, lv_color_black(), 0);
    lv_obj_align(
        qr_caption_, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_label_set_text(qr_caption_, "Scan to connect Nara");
}

void NaraFaceDisplay::HideQrCode() {
    DisplayLockGuard lock(this);
    if (!lock) return;
    if (qr_overlay_ != nullptr) {
        lv_obj_delete(qr_overlay_);
        qr_overlay_ = nullptr;
        qr_canvas_ = nullptr;
        qr_caption_ = nullptr;
        qr_buffer_.clear();
    }
    if (face_view_ != nullptr) {
        face_view_->SetVisible(true);
    }
}

void NaraFaceDisplay::SetPowerSaveMode(bool on) {
    SetChatMessage("system", "");
    SetInteraction(on ? "sleeping" : "idle");
    if (!on) {
        SetEmotion("neutral");
    }
}
