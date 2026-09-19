#define SDL_MAIN_HANDLED

#include "nara_face_controller.h"
#include "nara_face_view.h"

#include <lvgl.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>

namespace {
using Clock = std::chrono::steady_clock;

uint32_t MillisSince(const Clock::time_point& start) {
    return static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now() - start)
            .count());
}

void ApplyDemoPhase(NaraFaceController& face, int phase) {
    face.ClearManualGaze();

    switch (phase) {
        case 0:
            face.SetInteraction(NaraInteractionState::Idle);
            face.SetEmotion(NaraEmotion::Neutral);
            break;
        case 1:
            face.SetInteraction(NaraInteractionState::Listening);
            face.SetEmotion(NaraEmotion::Happy, 0.45f);
            break;
        case 2:
            face.SetInteraction(NaraInteractionState::Thinking);
            face.SetEmotion(NaraEmotion::Neutral);
            break;
        case 3:
            face.SetInteraction(NaraInteractionState::Speaking);
            face.SetEmotion(NaraEmotion::Happy, 0.8f);
            break;
        case 4:
            face.SetInteraction(NaraInteractionState::Speaking);
            face.SetEmotion(NaraEmotion::Shy, 0.85f);
            break;
        case 5:
            face.SetInteraction(NaraInteractionState::Listening);
            face.SetEmotion(NaraEmotion::Surprised, 0.75f);
            break;
        case 6:
            face.SetInteraction(NaraInteractionState::Idle);
            face.SetEmotion(NaraEmotion::Annoyed, 0.7f);
            break;
        default:
            face.SetInteraction(NaraInteractionState::Sleeping);
            face.SetEmotion(NaraEmotion::Neutral);
            break;
    }
}
}  // namespace

int main() {
    lv_init();

    lv_display_t* display = lv_sdl_window_create(360, 360);
    if (display == nullptr) {
        std::fprintf(stderr, "Failed to create LVGL SDL window\n");
        return 1;
    }

    lv_sdl_mouse_create();

    lv_obj_t* screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    NaraFaceController controller;
    NaraFaceView view(screen, 360, 360);

    const auto started = Clock::now();
    int previous_phase = -1;

    while (true) {
        const uint32_t now_ms = MillisSince(started);
        const int phase = static_cast<int>((now_ms / 3500u) % 8u);

        if (phase != previous_phase) {
            ApplyDemoPhase(controller, phase);
            previous_phase = phase;
        }

        if (controller.state().interaction == NaraInteractionState::Speaking) {
            const float wave =
                0.5f + 0.5f * std::sin(static_cast<float>(now_ms) * 0.018f);
            const float syllable =
                std::sin(static_cast<float>(now_ms) * 0.043f) > -0.15f
                    ? wave
                    : 0.05f;
            controller.SetSpeechLevel(syllable);
        } else {
            controller.SetSpeechLevel(0.0f);
        }

        controller.Tick(now_ms);
        view.Render(controller.state());

        lv_timer_handler();
        lv_delay_ms(16);
    }
}
