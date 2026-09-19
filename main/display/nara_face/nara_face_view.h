#pragma once

#include "nara_face_state.h"

#include <lvgl.h>

class NaraFaceView {
public:
    NaraFaceView(lv_obj_t* parent, int width, int height);
    ~NaraFaceView();

    NaraFaceView(const NaraFaceView&) = delete;
    NaraFaceView& operator=(const NaraFaceView&) = delete;

    void Render(const NaraFaceState& state);

private:
    int width_;
    int height_;

    lv_obj_t* root_ = nullptr;
    lv_obj_t* left_eye_ = nullptr;
    lv_obj_t* right_eye_ = nullptr;
    lv_obj_t* left_pupil_ = nullptr;
    lv_obj_t* right_pupil_ = nullptr;
    lv_obj_t* mouth_ = nullptr;
    lv_obj_t* left_blush_ = nullptr;
    lv_obj_t* right_blush_ = nullptr;

    lv_obj_t* CreateShape(lv_obj_t* parent, uint32_t color);
    void SetShape(lv_obj_t* object, int x, int y, int width, int height, int radius);
    void SetHidden(lv_obj_t* object, bool hidden);
};
