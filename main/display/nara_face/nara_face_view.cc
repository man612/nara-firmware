#include "nara_face_view.h"

#include <algorithm>
#include <cmath>

namespace {
int ClampInt(int value, int min_value, int max_value) {
    return std::max(min_value, std::min(max_value, value));
}

float EmotionAmount(const NaraFaceState& state, NaraEmotion emotion) {
    return state.emotion == emotion ? NaraClamp01(state.emotion_intensity) : 0.0f;
}
}  // namespace

NaraFaceView::NaraFaceView(lv_obj_t* parent, int width, int height)
    : width_(width), height_(height) {
    root_ = CreateShape(parent, 0x000000);
    lv_obj_set_size(root_, width_, height_);
    lv_obj_center(root_);
    lv_obj_clear_flag(root_, LV_OBJ_FLAG_SCROLLABLE);

    left_eye_ = CreateShape(root_, 0xffffff);
    right_eye_ = CreateShape(root_, 0xffffff);
    left_pupil_ = CreateShape(left_eye_, 0x000000);
    right_pupil_ = CreateShape(right_eye_, 0x000000);
    mouth_ = CreateShape(root_, 0xffffff);

    left_blush_ = CreateShape(root_, 0xd98b9a);
    right_blush_ = CreateShape(root_, 0xd98b9a);
    SetHidden(left_blush_, true);
    SetHidden(right_blush_, true);
}

NaraFaceView::~NaraFaceView() {
    if (root_ != nullptr) {
        lv_obj_del(root_);
        root_ = nullptr;
    }
}

lv_obj_t* NaraFaceView::CreateShape(lv_obj_t* parent, uint32_t color) {
    lv_obj_t* object = lv_obj_create(parent);
    lv_obj_remove_style_all(object);
    lv_obj_set_style_bg_color(object, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(object, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(object, 0, 0);
    lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE);
    return object;
}

void NaraFaceView::SetShape(
    lv_obj_t* object, int x, int y, int width, int height, int radius) {
    lv_obj_set_pos(object, x, y);
    lv_obj_set_size(object, width, height);
    lv_obj_set_style_radius(object, radius, 0);
}

void NaraFaceView::SetHidden(lv_obj_t* object, bool hidden) {
    if (hidden) {
        lv_obj_add_flag(object, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(object, LV_OBJ_FLAG_HIDDEN);
    }
}

void NaraFaceView::Render(const NaraFaceState& state) {
    const float happy = EmotionAmount(state, NaraEmotion::Happy);
    const float shy = EmotionAmount(state, NaraEmotion::Shy);
    const float sad = EmotionAmount(state, NaraEmotion::Sad);
    const float annoyed = EmotionAmount(state, NaraEmotion::Annoyed);
    const float surprised = EmotionAmount(state, NaraEmotion::Surprised);

    const int breath_offset = static_cast<int>(std::round((state.breath - 0.5f) * 4.0f));

    float eye_width_scale = 1.0f - sad * 0.08f + surprised * 0.08f;
    float eye_height_scale =
        1.0f - happy * 0.42f - shy * 0.22f - annoyed * 0.34f + surprised * 0.08f;

    if (state.interaction == NaraInteractionState::Listening) {
        eye_width_scale += 0.03f;
        eye_height_scale += 0.03f;
    }

    const int base_eye_width = 84;
    const int base_eye_height = 94;
    const int left_eye_height = ClampInt(
        static_cast<int>(std::round(base_eye_height * eye_height_scale * state.left_eye_open)),
        6,
        112);
    const int right_eye_height = ClampInt(
        static_cast<int>(std::round(base_eye_height * eye_height_scale * state.right_eye_open)),
        6,
        112);
    const int eye_width = ClampInt(
        static_cast<int>(std::round(base_eye_width * eye_width_scale)),
        52,
        104);

    const int left_center_x = 112;
    const int right_center_x = 248;
    const int eye_center_y = 145 + breath_offset;

    SetShape(
        left_eye_,
        left_center_x - eye_width / 2,
        eye_center_y - left_eye_height / 2,
        eye_width,
        left_eye_height,
        std::min(eye_width, left_eye_height) / 2);
    SetShape(
        right_eye_,
        right_center_x - eye_width / 2,
        eye_center_y - right_eye_height / 2,
        eye_width,
        right_eye_height,
        std::min(eye_width, right_eye_height) / 2);

    const bool pupils_visible = left_eye_height > 20 && right_eye_height > 20;
    SetHidden(left_pupil_, !pupils_visible);
    SetHidden(right_pupil_, !pupils_visible);

    if (pupils_visible) {
        const int pupil_width = surprised > 0.1f ? 25 : 22;
        const int pupil_height = surprised > 0.1f ? 32 : 30;

        float gaze_x = NaraClampSigned(state.gaze_x);
        float gaze_y = NaraClampSigned(state.gaze_y);
        if (shy > 0.0f && std::abs(gaze_x) < 0.15f) {
            gaze_x = -0.38f * shy;
        }

        const int left_range_x = std::max(0, (eye_width - pupil_width) / 2 - 8);
        const int right_range_x = left_range_x;
        const int left_range_y = std::max(0, (left_eye_height - pupil_height) / 2 - 6);
        const int right_range_y = std::max(0, (right_eye_height - pupil_height) / 2 - 6);

        const int left_pupil_x =
            (eye_width - pupil_width) / 2 + static_cast<int>(std::round(gaze_x * left_range_x));
        const int right_pupil_x =
            (eye_width - pupil_width) / 2 + static_cast<int>(std::round(gaze_x * right_range_x));
        const int left_pupil_y =
            (left_eye_height - pupil_height) / 2 + static_cast<int>(std::round(gaze_y * left_range_y));
        const int right_pupil_y =
            (right_eye_height - pupil_height) / 2 + static_cast<int>(std::round(gaze_y * right_range_y));

        SetShape(
            left_pupil_,
            left_pupil_x,
            left_pupil_y,
            pupil_width,
            pupil_height,
            std::min(pupil_width, pupil_height) / 2);
        SetShape(
            right_pupil_,
            right_pupil_x,
            right_pupil_y,
            pupil_width,
            pupil_height,
            std::min(pupil_width, pupil_height) / 2);
    }

    const bool show_blush = shy > 0.05f;
    SetHidden(left_blush_, !show_blush);
    SetHidden(right_blush_, !show_blush);
    if (show_blush) {
        const int blush_width = 34 + static_cast<int>(std::round(shy * 12.0f));
        SetShape(left_blush_, 56, 207 + breath_offset, blush_width, 8, 4);
        SetShape(right_blush_, width_ - 56 - blush_width, 207 + breath_offset, blush_width, 8, 4);
    }

    int mouth_width = 46;
    int mouth_height = 7;
    int mouth_y = 238 + breath_offset;

    if (happy > 0.0f) {
        mouth_width += static_cast<int>(std::round(30.0f * happy));
        mouth_height += static_cast<int>(std::round(5.0f * happy));
    }
    if (sad > 0.0f) {
        mouth_width -= static_cast<int>(std::round(10.0f * sad));
        mouth_y += static_cast<int>(std::round(7.0f * sad));
    }
    if (annoyed > 0.0f) {
        mouth_width -= static_cast<int>(std::round(8.0f * annoyed));
        mouth_height = 6;
    }
    if (shy > 0.0f) {
        mouth_width -= static_cast<int>(std::round(6.0f * shy));
    }

    if (state.interaction == NaraInteractionState::Speaking) {
        mouth_width = std::max(mouth_width, 48);
        mouth_height =
            8 + static_cast<int>(std::round(NaraClamp01(state.mouth_open) * 34.0f));
    } else if (surprised > 0.0f) {
        const int size = 18 + static_cast<int>(std::round(surprised * 18.0f));
        mouth_width = size;
        mouth_height = size;
    }

    mouth_width = ClampInt(mouth_width, 24, 90);
    mouth_height = ClampInt(mouth_height, 5, 48);
    SetShape(
        mouth_,
        width_ / 2 - mouth_width / 2,
        mouth_y - mouth_height / 2,
        mouth_width,
        mouth_height,
        std::min(mouth_width, mouth_height) / 2);
}
