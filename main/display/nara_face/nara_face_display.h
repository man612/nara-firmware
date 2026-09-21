#pragma once

#include "display/lcd_display.h"
#include "nara_face_controller.h"
#include "nara_face_view.h"

#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <qrcode.h>

class NaraFaceDisplay : public SpiLcdDisplay {
public:
    NaraFaceDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel, int width,
                    int height, int offset_x, int offset_y, bool mirror_x, bool mirror_y,
                    bool swap_xy);
    ~NaraFaceDisplay() override;

    void SetupUI() override;
    void SetEmotion(const char* emotion) override;
    void SetInteraction(const char* interaction) override;
    void SetSpeechLevel(float level) override;
    void SetGazeTarget(float x, float y) override;
    void ClearGazeTarget() override;
    void SetPowerSaveMode(bool on) override;

    bool ShowQrCode(
        const std::string& payload,
        const std::string& caption = "");
    void HideQrCode();

private:
    std::mutex face_mutex_;
    NaraFaceController controller_;
    std::unique_ptr<NaraFaceView> face_view_;
    lv_timer_t* face_timer_ = nullptr;
    lv_obj_t* qr_overlay_ = nullptr;
    lv_obj_t* qr_canvas_ = nullptr;
    lv_obj_t* qr_caption_ = nullptr;
    std::vector<uint16_t> qr_buffer_;
    std::mutex qr_mutex_;

    void RenderQrCode(esp_qrcode_handle_t handle);
    void RenderFace(uint32_t now_ms);
    void ShowFace();
    void ShowLegacyEmotion(const char* emotion);
};
