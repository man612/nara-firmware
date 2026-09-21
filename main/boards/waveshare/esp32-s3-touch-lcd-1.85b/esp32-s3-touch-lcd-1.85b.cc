#include "wifi_board.h"
#include "display/nara_face/nara_face_display.h"
#include "codecs/box_audio_codec.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "power_save_timer.h"
#include "physical/gesture_classifier.h"
#include "vision/sscma_i2c.h"
#include "vision/vision_target_tracker.h"
#include "settings.h"
#include "assets.h"
#include "mcp_server.h"
#include "assets/lang_config.h"

#include <esp_log.h>
#include <algorithm>
#include <optional>
#include <memory>
#include <esp_timer.h>
#include <driver/i2c_master.h>
#include <driver/spi_master.h>
#include <esp_lcd_st77916.h>
#define TAG "waveshare_lcd_1_85b"

#define LCD_OPCODE_WRITE_CMD        (0x02ULL)
#define LCD_OPCODE_READ_CMD         (0x0BULL)
#define LCD_OPCODE_WRITE_COLOR      (0x32ULL)

static const st77916_lcd_init_cmd_t vendor_specific_init_version_1[] = {
    {0xF0, (uint8_t []){0x28}, 1, 0},
    {0xF2, (uint8_t []){0x28}, 1, 0},
    {0x7C, (uint8_t []){0xD1}, 1, 0},
    {0x83, (uint8_t []){0xE0}, 1, 0},
    {0x84, (uint8_t []){0x61}, 1, 0},
    {0xF2, (uint8_t []){0x82}, 1, 0},
    {0xF0, (uint8_t []){0x00}, 1, 0},
    {0xF0, (uint8_t []){0x01}, 1, 0},
    {0xF1, (uint8_t []){0x01}, 1, 0},
    {0xB0, (uint8_t []){0x49}, 1, 0},
    {0xB1, (uint8_t []){0x4A}, 1, 0},
    {0xB2, (uint8_t []){0x1F}, 1, 0},
    {0xB4, (uint8_t []){0x46}, 1, 0},
    {0xB5, (uint8_t []){0x34}, 1, 0},
    {0xB6, (uint8_t []){0xD5}, 1, 0},
    {0xB7, (uint8_t []){0x30}, 1, 0},
    {0xB8, (uint8_t []){0x04}, 1, 0},
    {0xBA, (uint8_t []){0x00}, 1, 0},
    {0xBB, (uint8_t []){0x08}, 1, 0},
    {0xBC, (uint8_t []){0x08}, 1, 0},
    {0xBD, (uint8_t []){0x00}, 1, 0},
    {0xC0, (uint8_t []){0x80}, 1, 0},
    {0xC1, (uint8_t []){0x10}, 1, 0},
    {0xC2, (uint8_t []){0x37}, 1, 0},
    {0xC3, (uint8_t []){0x80}, 1, 0},
    {0xC4, (uint8_t []){0x10}, 1, 0},
    {0xC5, (uint8_t []){0x37}, 1, 0},
    {0xC6, (uint8_t []){0xA9}, 1, 0},
    {0xC7, (uint8_t []){0x41}, 1, 0},
    {0xC8, (uint8_t []){0x01}, 1, 0},
    {0xC9, (uint8_t []){0xA9}, 1, 0},
    {0xCA, (uint8_t []){0x41}, 1, 0},
    {0xCB, (uint8_t []){0x01}, 1, 0},
    {0xD0, (uint8_t []){0x91}, 1, 0},
    {0xD1, (uint8_t []){0x68}, 1, 0},
    {0xD2, (uint8_t []){0x68}, 1, 0},
    {0xF5, (uint8_t []){0x00, 0xA5}, 2, 0},
    // {0xDD, (uint8_t []){0x35}, 1, 0},
    // {0xDE, (uint8_t []){0x35}, 1, 0},
    // {0xDD, (uint8_t []){0x3F}, 1, 0},
    // {0xDE, (uint8_t []){0x3F}, 1, 0},
    {0xF1, (uint8_t []){0x10}, 1, 0},
    {0xF0, (uint8_t []){0x00}, 1, 0},
    {0xF0, (uint8_t []){0x02}, 1, 0},
    {0xE0, (uint8_t []){0x70, 0x09, 0x12, 0x0C, 0x0B, 0x27, 0x38, 0x54, 0x4E, 0x19, 0x15, 0x15, 0x2C, 0x2F}, 14, 0},
    {0xE1, (uint8_t []){0x70, 0x08, 0x11, 0x0C, 0x0B, 0x27, 0x38, 0x43, 0x4C, 0x18, 0x14, 0x14, 0x2B, 0x2D}, 14, 0},
    // {0xE0, (uint8_t []){0xF0, 0x0E, 0x15, 0x0B, 0x0B, 0x07, 0x3C, 0x44, 0x51, 0x38, 0x15, 0x15, 0x32, 0x36}, 14, 0},
    // {0xE1, (uint8_t []){0xF0, 0x0D, 0x15, 0x0A, 0x0A, 0x26, 0x3B, 0x43, 0x50, 0x37, 0x14, 0x15, 0x31, 0x36}, 14, 0},
    {0xF0, (uint8_t []){0x10}, 1, 0},
    {0xF3, (uint8_t []){0x10}, 1, 0},
    {0xE0, (uint8_t []){0x08}, 1, 0},
    {0xE1, (uint8_t []){0x00}, 1, 0},
    {0xE2, (uint8_t []){0x0B}, 1, 0},
    {0xE3, (uint8_t []){0x00}, 1, 0},
    {0xE4, (uint8_t []){0xE0}, 1, 0},
    {0xE5, (uint8_t []){0x06}, 1, 0},
    {0xE6, (uint8_t []){0x21}, 1, 0},
    {0xE7, (uint8_t []){0x00}, 1, 0},
    {0xE8, (uint8_t []){0x05}, 1, 0},
    {0xE9, (uint8_t []){0x82}, 1, 0},
    {0xEA, (uint8_t []){0xDF}, 1, 0},
    {0xEB, (uint8_t []){0x89}, 1, 0},
    {0xEC, (uint8_t []){0x20}, 1, 0},
    {0xED, (uint8_t []){0x14}, 1, 0},
    {0xEE, (uint8_t []){0xFF}, 1, 0},
    {0xEF, (uint8_t []){0x00}, 1, 0},
    {0xF8, (uint8_t []){0xFF}, 1, 0},
    {0xF9, (uint8_t []){0x00}, 1, 0},
    {0xFA, (uint8_t []){0x00}, 1, 0},
    {0xFB, (uint8_t []){0x30}, 1, 0},
    {0xFC, (uint8_t []){0x00}, 1, 0},
    {0xFD, (uint8_t []){0x00}, 1, 0},
    {0xFE, (uint8_t []){0x00}, 1, 0},
    {0xFF, (uint8_t []){0x00}, 1, 0},
    {0x60, (uint8_t []){0x42}, 1, 0},
    {0x61, (uint8_t []){0xE0}, 1, 0},
    {0x62, (uint8_t []){0x40}, 1, 0},
    {0x63, (uint8_t []){0x40}, 1, 0},
    {0x64, (uint8_t []){0x02}, 1, 0},
    {0x65, (uint8_t []){0x00}, 1, 0},
    {0x66, (uint8_t []){0x40}, 1, 0},
    {0x67, (uint8_t []){0x03}, 1, 0},
    {0x68, (uint8_t []){0x00}, 1, 0},
    {0x69, (uint8_t []){0x00}, 1, 0},
    {0x6A, (uint8_t []){0x00}, 1, 0},
    {0x6B, (uint8_t []){0x00}, 1, 0},
    {0x70, (uint8_t []){0x42}, 1, 0},
    {0x71, (uint8_t []){0xE0}, 1, 0},
    {0x72, (uint8_t []){0x40}, 1, 0},
    {0x73, (uint8_t []){0x40}, 1, 0},
    {0x74, (uint8_t []){0x02}, 1, 0},
    {0x75, (uint8_t []){0x00}, 1, 0},
    {0x76, (uint8_t []){0x40}, 1, 0},
    {0x77, (uint8_t []){0x03}, 1, 0},
    {0x78, (uint8_t []){0x00}, 1, 0},
    {0x79, (uint8_t []){0x00}, 1, 0},
    {0x7A, (uint8_t []){0x00}, 1, 0},
    {0x7B, (uint8_t []){0x00}, 1, 0},
    // {0x80, (uint8_t []){0x38}, 1, 0},
    {0x80, (uint8_t []){0x38}, 1, 0},
    {0x81, (uint8_t []){0x00}, 1, 0},
    // {0x82, (uint8_t []){0x04}, 1, 0},
    {0x82, (uint8_t []){0x04}, 1, 0},
    {0x83, (uint8_t []){0x02}, 1, 0},
    // {0x84, (uint8_t []){0xDC}, 1, 0},
    {0x84, (uint8_t []){0xDC}, 1, 0},
    {0x85, (uint8_t []){0x00}, 1, 0},
    {0x86, (uint8_t []){0x00}, 1, 0},
    {0x87, (uint8_t []){0x00}, 1, 0},
    // {0x88, (uint8_t []){0x38}, 1, 0},
    {0x88, (uint8_t []){0x38}, 1, 0},
    {0x89, (uint8_t []){0x00}, 1, 0},
    // {0x8A, (uint8_t []){0x06}, 1, 0},
    {0x8A, (uint8_t []){0x06}, 1, 0},
    {0x8B, (uint8_t []){0x02}, 1, 0},
    // {0x8C, (uint8_t []){0xDE}, 1, 0},
    {0x8C, (uint8_t []){0xDE}, 1, 0},
    {0x8D, (uint8_t []){0x00}, 1, 0},
    {0x8E, (uint8_t []){0x00}, 1, 0},
    {0x8F, (uint8_t []){0x00}, 1, 0},
    // {0x90, (uint8_t []){0x38}, 1, 0},
    {0x90, (uint8_t []){0x38}, 1, 0},
    {0x91, (uint8_t []){0x00}, 1, 0},
    // {0x92, (uint8_t []){0x08}, 1, 0},
    {0x92, (uint8_t []){0x08}, 1, 0},
    {0x93, (uint8_t []){0x02}, 1, 0},
    // {0x94, (uint8_t []){0xE0}, 1, 0},
    {0x94, (uint8_t []){0xE0}, 1, 0},
    {0x95, (uint8_t []){0x00}, 1, 0},
    {0x96, (uint8_t []){0x00}, 1, 0},
    {0x97, (uint8_t []){0x00}, 1, 0},
    // {0x98, (uint8_t []){0x38}, 1, 0},
    {0x98, (uint8_t []){0x38}, 1, 0},
    {0x99, (uint8_t []){0x00}, 1, 0},
    // {0x9A, (uint8_t []){0x0A}, 1, 0},
    {0x9A, (uint8_t []){0x0A}, 1, 0},
    {0x9B, (uint8_t []){0x02}, 1, 0},
    // {0x9C, (uint8_t []){0xE2}, 1, 0},
    {0x9C, (uint8_t []){0xE2}, 1, 0},
    {0x9D, (uint8_t []){0x00}, 1, 0},
    {0x9E, (uint8_t []){0x00}, 1, 0},
    {0x9F, (uint8_t []){0x00}, 1, 0},
    // {0xA0, (uint8_t []){0x38}, 1, 0},
    {0xA0, (uint8_t []){0x38}, 1, 0},
    {0xA1, (uint8_t []){0x00}, 1, 0},
    // {0xA2, (uint8_t []){0x03}, 1, 0},
    {0xA2, (uint8_t []){0x03}, 1, 0},
    {0xA3, (uint8_t []){0x02}, 1, 0},
    // {0xA4, (uint8_t []){0xDB}, 1, 0},
    {0xA4, (uint8_t []){0xDB}, 1, 0},
    {0xA5, (uint8_t []){0x00}, 1, 0},
    {0xA6, (uint8_t []){0x00}, 1, 0},
    {0xA7, (uint8_t []){0x00}, 1, 0},
    // {0xA8, (uint8_t []){0x38}, 1, 0},
    {0xA8, (uint8_t []){0x38}, 1, 0},
    {0xA9, (uint8_t []){0x00}, 1, 0},
    // {0xAA, (uint8_t []){0x05}, 1, 0},
    {0xAA, (uint8_t []){0x05}, 1, 0},
    {0xAB, (uint8_t []){0x02}, 1, 0},
    // {0xAC, (uint8_t []){0xDD}, 1, 0},
    {0xAC, (uint8_t []){0xDD}, 1, 0},
    {0xAD, (uint8_t []){0x00}, 1, 0},
    {0xAE, (uint8_t []){0x00}, 1, 0},
    {0xAF, (uint8_t []){0x00}, 1, 0},
    // {0xB0, (uint8_t []){0x38}, 1, 0},
    {0xB0, (uint8_t []){0x38}, 1, 0},
    {0xB1, (uint8_t []){0x00}, 1, 0},
    // {0xB2, (uint8_t []){0x07}, 1, 0},
    {0xB2, (uint8_t []){0x07}, 1, 0},
    {0xB3, (uint8_t []){0x02}, 1, 0},
    // {0xB4, (uint8_t []){0xDF}, 1, 0},
    {0xB4, (uint8_t []){0xDF}, 1, 0},
    {0xB5, (uint8_t []){0x00}, 1, 0},
    {0xB6, (uint8_t []){0x00}, 1, 0},
    {0xB7, (uint8_t []){0x00}, 1, 0},
    // {0xB8, (uint8_t []){0x38}, 1, 0},
    {0xB8, (uint8_t []){0x38}, 1, 0},
    {0xB9, (uint8_t []){0x00}, 1, 0},
    // {0xBA, (uint8_t []){0x09}, 1, 0},
    {0xBA, (uint8_t []){0x09}, 1, 0},
    {0xBB, (uint8_t []){0x02}, 1, 0},
    // {0xBC, (uint8_t []){0xE1}, 1, 0},
    {0xBC, (uint8_t []){0xE1}, 1, 0},
    {0xBD, (uint8_t []){0x00}, 1, 0},
    {0xBE, (uint8_t []){0x00}, 1, 0},
    {0xBF, (uint8_t []){0x00}, 1, 0},
    // {0xC0, (uint8_t []){0x22}, 1, 0},
    {0xC0, (uint8_t []){0x22}, 1, 0},
    {0xC1, (uint8_t []){0xAA}, 1, 0},
    {0xC2, (uint8_t []){0x65}, 1, 0},
    {0xC3, (uint8_t []){0x74}, 1, 0},
    {0xC4, (uint8_t []){0x47}, 1, 0},
    {0xC5, (uint8_t []){0x56}, 1, 0},
    {0xC6, (uint8_t []){0x00}, 1, 0},
    {0xC7, (uint8_t []){0x88}, 1, 0},
    {0xC8, (uint8_t []){0x99}, 1, 0},
    {0xC9, (uint8_t []){0x33}, 1, 0},
    // {0xD0, (uint8_t []){0x11}, 1, 0},
    {0xD0, (uint8_t []){0x11}, 1, 0},
    {0xD1, (uint8_t []){0xAA}, 1, 0},
    {0xD2, (uint8_t []){0x65}, 1, 0},
    {0xD3, (uint8_t []){0x74}, 1, 0},
    {0xD4, (uint8_t []){0x47}, 1, 0},
    {0xD5, (uint8_t []){0x56}, 1, 0},
    {0xD6, (uint8_t []){0x00}, 1, 0},
    {0xD7, (uint8_t []){0x88}, 1, 0},
    {0xD8, (uint8_t []){0x99}, 1, 0},
    {0xD9, (uint8_t []){0x33}, 1, 0},
    {0xF3, (uint8_t []){0x01}, 1, 0},
    {0xF0, (uint8_t []){0x00}, 1, 0},
    // {0x3A, (uint8_t []){0x55}, 1, 0},
    {0x21, (uint8_t []){0x00}, 0, 0},
    {0x11, (uint8_t []){0x00}, 0, 120},
    {0x29, (uint8_t []){0x00}, 0, 0},
};
static const st77916_lcd_init_cmd_t vendor_specific_init_version_2[] = {
  {0xF0, (uint8_t []){0x28}, 1, 0},
  {0xF2, (uint8_t []){0x28}, 1, 0},
  {0x73, (uint8_t []){0xF0}, 1, 0},
  {0x7C, (uint8_t []){0xD1}, 1, 0},
  {0x83, (uint8_t []){0xE0}, 1, 0},
  {0x84, (uint8_t []){0x61}, 1, 0},
  {0xF2, (uint8_t []){0x82}, 1, 0},
  {0xF0, (uint8_t []){0x00}, 1, 0},
  {0xF0, (uint8_t []){0x01}, 1, 0},
  {0xF1, (uint8_t []){0x01}, 1, 0},
  {0xB0, (uint8_t []){0x56}, 1, 0},
  {0xB1, (uint8_t []){0x4D}, 1, 0},
  {0xB2, (uint8_t []){0x24}, 1, 0},
  {0xB4, (uint8_t []){0x87}, 1, 0},
  {0xB5, (uint8_t []){0x44}, 1, 0},
  {0xB6, (uint8_t []){0x8B}, 1, 0},
  {0xB7, (uint8_t []){0x40}, 1, 0},
  {0xB8, (uint8_t []){0x86}, 1, 0},
  {0xBA, (uint8_t []){0x00}, 1, 0},
  {0xBB, (uint8_t []){0x08}, 1, 0},
  {0xBC, (uint8_t []){0x08}, 1, 0},
  {0xBD, (uint8_t []){0x00}, 1, 0},
  {0xC0, (uint8_t []){0x80}, 1, 0},
  {0xC1, (uint8_t []){0x10}, 1, 0},
  {0xC2, (uint8_t []){0x37}, 1, 0},
  {0xC3, (uint8_t []){0x80}, 1, 0},
  {0xC4, (uint8_t []){0x10}, 1, 0},
  {0xC5, (uint8_t []){0x37}, 1, 0},
  {0xC6, (uint8_t []){0xA9}, 1, 0},
  {0xC7, (uint8_t []){0x41}, 1, 0},
  {0xC8, (uint8_t []){0x01}, 1, 0},
  {0xC9, (uint8_t []){0xA9}, 1, 0},
  {0xCA, (uint8_t []){0x41}, 1, 0},
  {0xCB, (uint8_t []){0x01}, 1, 0},
  {0xD0, (uint8_t []){0x91}, 1, 0},
  {0xD1, (uint8_t []){0x68}, 1, 0},
  {0xD2, (uint8_t []){0x68}, 1, 0},
  {0xF5, (uint8_t []){0x00, 0xA5}, 2, 0},
  {0xDD, (uint8_t []){0x4F}, 1, 0},
  {0xDE, (uint8_t []){0x4F}, 1, 0},
  {0xF1, (uint8_t []){0x10}, 1, 0},
  {0xF0, (uint8_t []){0x00}, 1, 0},
  {0xF0, (uint8_t []){0x02}, 1, 0},
  {0xE0, (uint8_t []){0xF0, 0x0A, 0x10, 0x09, 0x09, 0x36, 0x35, 0x33, 0x4A, 0x29, 0x15, 0x15, 0x2E, 0x34}, 14, 0},
  {0xE1, (uint8_t []){0xF0, 0x0A, 0x0F, 0x08, 0x08, 0x05, 0x34, 0x33, 0x4A, 0x39, 0x15, 0x15, 0x2D, 0x33}, 14, 0},
  {0xF0, (uint8_t []){0x10}, 1, 0},
  {0xF3, (uint8_t []){0x10}, 1, 0},
  {0xE0, (uint8_t []){0x07}, 1, 0},
  {0xE1, (uint8_t []){0x00}, 1, 0},
  {0xE2, (uint8_t []){0x00}, 1, 0},
  {0xE3, (uint8_t []){0x00}, 1, 0},
  {0xE4, (uint8_t []){0xE0}, 1, 0},
  {0xE5, (uint8_t []){0x06}, 1, 0},
  {0xE6, (uint8_t []){0x21}, 1, 0},
  {0xE7, (uint8_t []){0x01}, 1, 0},
  {0xE8, (uint8_t []){0x05}, 1, 0},
  {0xE9, (uint8_t []){0x02}, 1, 0},
  {0xEA, (uint8_t []){0xDA}, 1, 0},
  {0xEB, (uint8_t []){0x00}, 1, 0},
  {0xEC, (uint8_t []){0x00}, 1, 0},
  {0xED, (uint8_t []){0x0F}, 1, 0},
  {0xEE, (uint8_t []){0x00}, 1, 0},
  {0xEF, (uint8_t []){0x00}, 1, 0},
  {0xF8, (uint8_t []){0x00}, 1, 0},
  {0xF9, (uint8_t []){0x00}, 1, 0},
  {0xFA, (uint8_t []){0x00}, 1, 0},
  {0xFB, (uint8_t []){0x00}, 1, 0},
  {0xFC, (uint8_t []){0x00}, 1, 0},
  {0xFD, (uint8_t []){0x00}, 1, 0},
  {0xFE, (uint8_t []){0x00}, 1, 0},
  {0xFF, (uint8_t []){0x00}, 1, 0},
  {0x60, (uint8_t []){0x40}, 1, 0},
  {0x61, (uint8_t []){0x04}, 1, 0},
  {0x62, (uint8_t []){0x00}, 1, 0},
  {0x63, (uint8_t []){0x42}, 1, 0},
  {0x64, (uint8_t []){0xD9}, 1, 0},
  {0x65, (uint8_t []){0x00}, 1, 0},
  {0x66, (uint8_t []){0x00}, 1, 0},
  {0x67, (uint8_t []){0x00}, 1, 0},
  {0x68, (uint8_t []){0x00}, 1, 0},
  {0x69, (uint8_t []){0x00}, 1, 0},
  {0x6A, (uint8_t []){0x00}, 1, 0},
  {0x6B, (uint8_t []){0x00}, 1, 0},
  {0x70, (uint8_t []){0x40}, 1, 0},
  {0x71, (uint8_t []){0x03}, 1, 0},
  {0x72, (uint8_t []){0x00}, 1, 0},
  {0x73, (uint8_t []){0x42}, 1, 0},
  {0x74, (uint8_t []){0xD8}, 1, 0},
  {0x75, (uint8_t []){0x00}, 1, 0},
  {0x76, (uint8_t []){0x00}, 1, 0},
  {0x77, (uint8_t []){0x00}, 1, 0},
  {0x78, (uint8_t []){0x00}, 1, 0},
  {0x79, (uint8_t []){0x00}, 1, 0},
  {0x7A, (uint8_t []){0x00}, 1, 0},
  {0x7B, (uint8_t []){0x00}, 1, 0},
  {0x80, (uint8_t []){0x48}, 1, 0},
  {0x81, (uint8_t []){0x00}, 1, 0},
  {0x82, (uint8_t []){0x06}, 1, 0},
  {0x83, (uint8_t []){0x02}, 1, 0},
  {0x84, (uint8_t []){0xD6}, 1, 0},
  {0x85, (uint8_t []){0x04}, 1, 0},
  {0x86, (uint8_t []){0x00}, 1, 0},
  {0x87, (uint8_t []){0x00}, 1, 0},
  {0x88, (uint8_t []){0x48}, 1, 0},
  {0x89, (uint8_t []){0x00}, 1, 0},
  {0x8A, (uint8_t []){0x08}, 1, 0},
  {0x8B, (uint8_t []){0x02}, 1, 0},
  {0x8C, (uint8_t []){0xD8}, 1, 0},
  {0x8D, (uint8_t []){0x04}, 1, 0},
  {0x8E, (uint8_t []){0x00}, 1, 0},
  {0x8F, (uint8_t []){0x00}, 1, 0},
  {0x90, (uint8_t []){0x48}, 1, 0},
  {0x91, (uint8_t []){0x00}, 1, 0},
  {0x92, (uint8_t []){0x0A}, 1, 0},
  {0x93, (uint8_t []){0x02}, 1, 0},
  {0x94, (uint8_t []){0xDA}, 1, 0},
  {0x95, (uint8_t []){0x04}, 1, 0},
  {0x96, (uint8_t []){0x00}, 1, 0},
  {0x97, (uint8_t []){0x00}, 1, 0},
  {0x98, (uint8_t []){0x48}, 1, 0},
  {0x99, (uint8_t []){0x00}, 1, 0},
  {0x9A, (uint8_t []){0x0C}, 1, 0},
  {0x9B, (uint8_t []){0x02}, 1, 0},
  {0x9C, (uint8_t []){0xDC}, 1, 0},
  {0x9D, (uint8_t []){0x04}, 1, 0},
  {0x9E, (uint8_t []){0x00}, 1, 0},
  {0x9F, (uint8_t []){0x00}, 1, 0},
  {0xA0, (uint8_t []){0x48}, 1, 0},
  {0xA1, (uint8_t []){0x00}, 1, 0},
  {0xA2, (uint8_t []){0x05}, 1, 0},
  {0xA3, (uint8_t []){0x02}, 1, 0},
  {0xA4, (uint8_t []){0xD5}, 1, 0},
  {0xA5, (uint8_t []){0x04}, 1, 0},
  {0xA6, (uint8_t []){0x00}, 1, 0},
  {0xA7, (uint8_t []){0x00}, 1, 0},
  {0xA8, (uint8_t []){0x48}, 1, 0},
  {0xA9, (uint8_t []){0x00}, 1, 0},
  {0xAA, (uint8_t []){0x07}, 1, 0},
  {0xAB, (uint8_t []){0x02}, 1, 0},
  {0xAC, (uint8_t []){0xD7}, 1, 0},
  {0xAD, (uint8_t []){0x04}, 1, 0},
  {0xAE, (uint8_t []){0x00}, 1, 0},
  {0xAF, (uint8_t []){0x00}, 1, 0},
  {0xB0, (uint8_t []){0x48}, 1, 0},
  {0xB1, (uint8_t []){0x00}, 1, 0},
  {0xB2, (uint8_t []){0x09}, 1, 0},
  {0xB3, (uint8_t []){0x02}, 1, 0},
  {0xB4, (uint8_t []){0xD9}, 1, 0},
  {0xB5, (uint8_t []){0x04}, 1, 0},
  {0xB6, (uint8_t []){0x00}, 1, 0},
  {0xB7, (uint8_t []){0x00}, 1, 0},
  
  {0xB8, (uint8_t []){0x48}, 1, 0},
  {0xB9, (uint8_t []){0x00}, 1, 0},
  {0xBA, (uint8_t []){0x0B}, 1, 0},
  {0xBB, (uint8_t []){0x02}, 1, 0},
  {0xBC, (uint8_t []){0xDB}, 1, 0},
  {0xBD, (uint8_t []){0x04}, 1, 0},
  {0xBE, (uint8_t []){0x00}, 1, 0},
  {0xBF, (uint8_t []){0x00}, 1, 0},
  {0xC0, (uint8_t []){0x10}, 1, 0},
  {0xC1, (uint8_t []){0x47}, 1, 0},
  {0xC2, (uint8_t []){0x56}, 1, 0},
  {0xC3, (uint8_t []){0x65}, 1, 0},
  {0xC4, (uint8_t []){0x74}, 1, 0},
  {0xC5, (uint8_t []){0x88}, 1, 0},
  {0xC6, (uint8_t []){0x99}, 1, 0},
  {0xC7, (uint8_t []){0x01}, 1, 0},
  {0xC8, (uint8_t []){0xBB}, 1, 0},
  {0xC9, (uint8_t []){0xAA}, 1, 0},
  {0xD0, (uint8_t []){0x10}, 1, 0},
  {0xD1, (uint8_t []){0x47}, 1, 0},
  {0xD2, (uint8_t []){0x56}, 1, 0},
  {0xD3, (uint8_t []){0x65}, 1, 0},
  {0xD4, (uint8_t []){0x74}, 1, 0},
  {0xD5, (uint8_t []){0x88}, 1, 0},
  {0xD6, (uint8_t []){0x99}, 1, 0},
  {0xD7, (uint8_t []){0x01}, 1, 0},
  {0xD8, (uint8_t []){0xBB}, 1, 0},
  {0xD9, (uint8_t []){0xAA}, 1, 0},
  {0xF3, (uint8_t []){0x01}, 1, 0},
  {0xF0, (uint8_t []){0x00}, 1, 0},
  {0x21, (uint8_t []){0x00}, 1, 0},
  {0x11, (uint8_t []){0x00}, 1, 120},
  {0x29, (uint8_t []){0x00}, 1, 0},  
};

class WaveshareEsp32s3TouchLcd1_85B : public WifiBoard {
private:
    i2c_master_bus_handle_t i2c_bus_ = nullptr;
    i2c_master_dev_handle_t qmi8658_ = nullptr;
    i2c_master_dev_handle_t bq27220_ = nullptr;
    Button boot_button_;
    Display* display_ = nullptr;
    PowerSaveTimer* power_save_timer_ = nullptr;
    NaraGestureClassifier gesture_classifier_;
    std::unique_ptr<SscmaI2cVisionSensor> vision_sensor_;
    std::unique_ptr<NaraVisionTargetTracker> vision_tracker_;
    bool battery_saver_active_ = false;
    bool battery_critical_ = false;
    uint32_t last_battery_check_ms_ = 0;


    bool AddI2cDevice(uint8_t address, i2c_master_dev_handle_t* handle) {
        i2c_device_config_t config = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = address,
            .scl_speed_hz = 400000,
            .scl_wait_us = 0,
            .flags = {},
        };
        esp_err_t err = i2c_master_bus_add_device(i2c_bus_, &config, handle);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "I2C device 0x%02x unavailable: %s", address, esp_err_to_name(err));
            *handle = nullptr;
            return false;
        }
        return true;
    }

    bool WriteRegister(i2c_master_dev_handle_t device, uint8_t reg, uint8_t value) {
        if (device == nullptr) return false;
        const uint8_t data[] = {reg, value};
        return i2c_master_transmit(device, data, sizeof(data), 100) == ESP_OK;
    }

    bool ReadRegisters(i2c_master_dev_handle_t device, uint8_t reg, uint8_t* data, size_t length) {
        if (device == nullptr) return false;
        return i2c_master_transmit_receive(device, &reg, 1, data, length, 100) == ESP_OK;
    }

    bool ReadWord(i2c_master_dev_handle_t device, uint8_t reg, uint16_t& value) {
        uint8_t data[2] = {};
        if (!ReadRegisters(device, reg, data, sizeof(data))) return false;
        value = static_cast<uint16_t>(data[0]) |
                (static_cast<uint16_t>(data[1]) << 8);
        return true;
    }

    void InitializePhysicalSensors() {
        if (!AddI2cDevice(0x6B, &qmi8658_)) {
            return;
        }

        uint8_t who_am_i = 0;
        if (!ReadRegisters(qmi8658_, 0x00, &who_am_i, 1) || who_am_i != 0x05) {
            ESP_LOGW(TAG, "QMI8658 not detected (WHO_AM_I=0x%02x)", who_am_i);
            i2c_master_bus_rm_device(qmi8658_);
            qmi8658_ = nullptr;
            return;
        }

        // Nara-owned conservative profile: address auto-increment, +/-8g accel
        // at 125Hz, +/-512dps gyro at ~117Hz, both sensors enabled.
        // Gesture thresholds remain provisional until physical enclosure/HIL calibration.
        WriteRegister(qmi8658_, 0x60, 0xB0);
        vTaskDelay(pdMS_TO_TICKS(20));
        const bool configured =
            WriteRegister(qmi8658_, 0x02, 0x40) &&
            WriteRegister(qmi8658_, 0x03, 0x26) &&
            WriteRegister(qmi8658_, 0x04, 0x56) &&
            WriteRegister(qmi8658_, 0x08, 0x03);
        if (!configured) {
            ESP_LOGW(TAG, "Failed to configure QMI8658");
            i2c_master_bus_rm_device(qmi8658_);
            qmi8658_ = nullptr;
            return;
        }
        ESP_LOGI(TAG, "QMI8658 motion reflex input enabled");
    }

    void InitializeBatteryGauge() {
        if (AddI2cDevice(0x55, &bq27220_)) {
            uint16_t soc = 0;
            if (ReadWord(bq27220_, 0x2C, soc) && soc <= 100) {
                ESP_LOGI(TAG, "BQ27220 battery gauge enabled, SOC=%u%%", soc);
                return;
            }
            ESP_LOGW(TAG, "BQ27220 did not return a valid SOC yet");
        }
    }

    bool ReadMotionSample(NaraMotionSample& sample) {
        uint8_t raw[12] = {};
        if (!ReadRegisters(qmi8658_, 0x35, raw, sizeof(raw))) return false;

        auto s16 = [&raw](size_t offset) -> int16_t {
            return static_cast<int16_t>(
                static_cast<uint16_t>(raw[offset]) |
                (static_cast<uint16_t>(raw[offset + 1]) << 8));
        };

        constexpr float kAccelCountsPerG = 4096.0f;  // +/-8g
        constexpr float kGyroCountsPerDps = 64.0f;  // +/-512dps
        sample.ax_g = static_cast<float>(s16(0)) / kAccelCountsPerG;
        sample.ay_g = static_cast<float>(s16(2)) / kAccelCountsPerG;
        sample.az_g = static_cast<float>(s16(4)) / kAccelCountsPerG;
        sample.gx_dps = static_cast<float>(s16(6)) / kGyroCountsPerDps;
        sample.gy_dps = static_cast<float>(s16(8)) / kGyroCountsPerDps;
        sample.gz_dps = static_cast<float>(s16(10)) / kGyroCountsPerDps;
        sample.timestamp_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
        return true;
    }

    const char* GestureKey(NaraMotionGesture gesture) const {
        switch (gesture) {
            case NaraMotionGesture::Flip:
                return "flip";
            case NaraMotionGesture::Shake:
                return "shake";
            case NaraMotionGesture::Spin:
                return "spin";
            case NaraMotionGesture::None:
            default:
                return "";
        }
    }

    std::string DefaultGestureEmotion(NaraMotionGesture gesture) const {
        return gesture == NaraMotionGesture::Shake ? "annoyed" : "surprised";
    }

    std::string DefaultGestureSound(NaraMotionGesture gesture) const {
        return gesture == NaraMotionGesture::Shake
                   ? "builtin:exclamation"
                   : "builtin:popup";
    }

    bool IsValidEmotion(const std::string& emotion) const {
        return emotion == "neutral" || emotion == "happy" || emotion == "shy" ||
               emotion == "sad" || emotion == "annoyed" || emotion == "surprised";
    }

    bool IsValidSoundSpec(const std::string& sound) const {
        if (sound == "none" || sound == "builtin:popup" ||
            sound == "builtin:exclamation") {
            return true;
        }
        if (sound.rfind("asset:", 0) != 0) {
            return false;
        }
        const std::string asset = sound.substr(6);
        if (asset.empty() || asset.size() > 96 || asset.find("..") != std::string::npos ||
            asset.front() == '/' || asset.front() == '\\') {
            return false;
        }
        void* data = nullptr;
        size_t size = 0;
        return Assets::GetInstance().GetAssetData(asset, data, size) &&
               data != nullptr && size > 0;
    }

    void PlayReactionSound(const std::string& sound) {
        auto& app = Application::GetInstance();
        if (sound == "none") {
            return;
        }
        if (sound == "builtin:exclamation") {
            app.PlaySound(Lang::Sounds::OGG_EXCLAMATION);
            return;
        }
        if (sound == "builtin:popup") {
            app.PlaySound(Lang::Sounds::OGG_POPUP);
            return;
        }
        if (sound.rfind("asset:", 0) == 0) {
            if (!app.PlayAssetSound(sound.substr(6))) {
                ESP_LOGW(TAG, "Reaction asset failed at playback: %s", sound.c_str());
            }
        }
    }

    void RunGestureReaction(NaraMotionGesture gesture, bool require_idle = true) {
        if (gesture == NaraMotionGesture::None) return;
        auto& app = Application::GetInstance();
        if (require_idle && app.GetDeviceState() != kDeviceStateIdle) {
            return;
        }

        const std::string key = GestureKey(gesture);
        Settings reflex("reflex", false);
        if (!reflex.GetBool((key + "_on").c_str(), true)) {
            return;
        }

        const std::string emotion = reflex.GetString(
            (key + "_emote").c_str(), DefaultGestureEmotion(gesture));
        const std::string sound = reflex.GetString(
            (key + "_sound").c_str(), DefaultGestureSound(gesture));

        if (IsValidEmotion(emotion)) {
            GetDisplay()->SetEmotion(emotion.c_str());
        }
        PlayReactionSound(sound);
    }

    void HandleMotionGesture(NaraMotionGesture gesture) {
        if (gesture == NaraMotionGesture::None) return;
        Application::GetInstance().Schedule([this, gesture]() {
            RunGestureReaction(gesture, true);
        });
    }

    std::optional<NaraMotionGesture> ParseGesture(const std::string& value) const {
        if (value == "flip") return NaraMotionGesture::Flip;
        if (value == "shake") return NaraMotionGesture::Shake;
        if (value == "spin") return NaraMotionGesture::Spin;
        return std::nullopt;
    }

    void InitializeReactionTools() {
        McpServer::GetInstance().AddTool(
            "self.reflex.configure",
            "Configure a local physical reflex. Use only when the user explicitly asks to change "
            "what Nara does when flipped, shaken, or spun. sound supports none, "
            "builtin:popup, builtin:exclamation, or asset:<ogg asset name>.",
            PropertyList({
                Property("gesture", kPropertyTypeString).SetMaxLength(8),
                Property("enabled", kPropertyTypeBoolean, true),
                Property("emotion", kPropertyTypeString, std::string("")).SetMaxLength(16),
                Property("sound", kPropertyTypeString, std::string("")).SetMaxLength(110),
                Property("preview", kPropertyTypeBoolean, false),
            }),
            [this](const PropertyList& properties) -> ToolResult {
                const auto gesture_name = properties["gesture"].value<std::string>();
                const auto gesture = ParseGesture(gesture_name);
                if (!gesture) {
                    return std::unexpected("gesture must be flip, shake, or spin");
                }

                const auto emotion = properties["emotion"].value<std::string>();
                const auto sound = properties["sound"].value<std::string>();
                if (!emotion.empty() && !IsValidEmotion(emotion)) {
                    return std::unexpected(
                        "emotion must be neutral, happy, shy, sad, annoyed, or surprised");
                }
                if (!sound.empty() && !IsValidSoundSpec(sound)) {
                    return std::unexpected(
                        "sound must be none, builtin:popup, builtin:exclamation, "
                        "or an existing asset:<name>");
                }

                const std::string key = GestureKey(*gesture);
                Settings reflex("reflex", true);
                reflex.SetBool((key + "_on").c_str(),
                               properties["enabled"].value<bool>());
                if (!emotion.empty()) {
                    reflex.SetString((key + "_emote").c_str(), emotion);
                }
                if (!sound.empty()) {
                    reflex.SetString((key + "_sound").c_str(), sound);
                }

                if (properties["preview"].value<bool>()) {
                    Application::GetInstance().Schedule([this, gesture = *gesture]() {
                        RunGestureReaction(gesture, false);
                    });
                }
                return true;
            });
    }

    void ApplyBatteryPolicy(int level, bool charging) {
        Settings settings("power", false);
        if (!settings.GetBool("auto_battery_saver", true)) return;

        const int eco_percent = settings.GetInt("eco_percent", 20);
        const int critical_percent = settings.GetInt("critical_percent", 10);
        const int recover_percent = settings.GetInt("recover_percent", 25);

        if (charging || level >= recover_percent) {
            if (battery_saver_active_) {
                battery_saver_active_ = false;
                battery_critical_ = false;
                ESP_LOGI(TAG, "Battery saver released at %d%%", level);
                Application::GetInstance().Schedule([this]() {
                    GetBacklight()->RestoreBrightness();
                    GetDisplay()->SetPowerSaveMode(false);
                });
            }
            return;
        }

        if (level <= critical_percent) {
            if (!battery_critical_) {
                battery_critical_ = true;
                battery_saver_active_ = true;
                ESP_LOGW(TAG, "Critical battery: %d%%", level);
                WifiBoard::SetPowerSaveLevel(PowerSaveLevel::LOW_POWER);
                Application::GetInstance().Schedule([this]() {
                    GetBacklight()->SetBrightness(8);
                    GetDisplay()->SetEmotion("sad");
                });
            }
            return;
        }

        if (level <= eco_percent && !battery_saver_active_) {
            battery_saver_active_ = true;
            ESP_LOGI(TAG, "Automatic battery saver enabled at %d%%", level);
            WifiBoard::SetPowerSaveLevel(PowerSaveLevel::LOW_POWER);
            Application::GetInstance().Schedule([this]() {
                if (GetBacklight()->brightness() > 25) {
                    GetBacklight()->SetBrightness(25);
                }
            });
        }
    }

    void PollBatteryPolicy(uint32_t now_ms) {
        if (bq27220_ == nullptr ||
            (last_battery_check_ms_ != 0 && now_ms - last_battery_check_ms_ < 30000)) {
            return;
        }
        last_battery_check_ms_ = now_ms;

        int level = 0;
        bool charging = false;
        bool discharging = false;
        if (GetBatteryLevel(level, charging, discharging)) {
            ApplyBatteryPolicy(level, charging);
        }
    }

    void InitializeOptionalVision() {
        Settings vision("vision", false);
        if (!vision.GetBool("enabled", true)) {
            ESP_LOGI(TAG, "External local vision disabled by settings");
            return;
        }

        auto sensor = std::make_unique<SscmaI2cVisionSensor>(i2c_bus_);
        if (!sensor->Probe()) {
            return;
        }

        NaraVisionTrackerConfig config;
        config.frame_width =
            static_cast<float>(std::max<int32_t>(1, vision.GetInt("frame_w", 240)));
        config.frame_height =
            static_cast<float>(std::max<int32_t>(1, vision.GetInt("frame_h", 240)));
        config.min_score =
            static_cast<float>(std::max<int32_t>(
            0, std::min<int32_t>(100, vision.GetInt("min_score", 60))));
        config.smoothing = 0.30f;
        config.deadband = 0.04f;
        config.hold_ms = 900;

        vision_tracker_ = std::make_unique<NaraVisionTargetTracker>(config);
        vision_sensor_ = std::move(sensor);

        xTaskCreate(
            [](void* arg) {
                auto* self = static_cast<WaveshareEsp32s3TouchLcd1_85B*>(arg);
                std::vector<NaraVisionBox> boxes;
                bool gaze_active = false;

                while (self->vision_sensor_ != nullptr &&
                       self->vision_tracker_ != nullptr) {
                    const uint32_t now_ms =
                        static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);

                    if (self->vision_sensor_->Invoke(boxes, 1200)) {
                        const NaraVisionBox* best = nullptr;
                        for (const auto& box : boxes) {
                            if (best == nullptr || box.score > best->score) {
                                best = &box;
                            }
                        }

                        if (best != nullptr) {
                            const auto gaze =
                                self->vision_tracker_->Update(*best, now_ms);
                            if (gaze) {
                                gaze_active = true;
                                Application::GetInstance().Schedule(
                                    [display = self->GetDisplay(),
                                     x = gaze->x,
                                     y = gaze->y]() {
                                        display->SetGazeTarget(x, y);
                                    });
                            }
                        } else if (!self->vision_tracker_->Tick(now_ms)) {
                            if (gaze_active) {
                                gaze_active = false;
                                Application::GetInstance().Schedule(
                                    [display = self->GetDisplay()]() {
                                        display->ClearGazeTarget();
                                    });
                            }
                        }
                    } else if (!self->vision_tracker_->Tick(now_ms)) {
                        if (gaze_active) {
                            gaze_active = false;
                            Application::GetInstance().Schedule(
                                [display = self->GetDisplay()]() {
                                    display->ClearGazeTarget();
                                });
                        }
                    }

                    // The external module performs inference. Nara only follows
                    // compact detection coordinates, so no camera frames or AI
                    // tokens are spent on idle eye tracking.
                    vTaskDelay(pdMS_TO_TICKS(80));
                }
                vTaskDelete(nullptr);
            },
            "nara_vision", 6144, this, 2, nullptr);

        ESP_LOGI(TAG,
                 "Local SSCMA vision gaze enabled (%dx%d, min score %d)",
                 static_cast<int>(config.frame_width),
                 static_cast<int>(config.frame_height),
                 static_cast<int>(config.min_score));
    }

    void StartPhysicalReflexTask() {
        xTaskCreate(
            [](void* arg) {
                auto* self = static_cast<WaveshareEsp32s3TouchLcd1_85B*>(arg);
                while (true) {
                    const uint32_t now_ms =
                        static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
                    if (self->qmi8658_ != nullptr) {
                        NaraMotionSample sample;
                        if (self->ReadMotionSample(sample)) {
                            self->HandleMotionGesture(self->gesture_classifier_.Update(sample));
                        }
                    }
                    self->PollBatteryPolicy(now_ms);
                    vTaskDelay(pdMS_TO_TICKS(20));
                }
            },
            "nara_reflex", 4096, this, 2, nullptr);
    }

    static void st77916_reset(void)
    {
        gpio_config_t ioconf = {
            .pin_bit_mask = 1ULL << QSPI_PIN_NUM_LCD_RST,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&ioconf);

        gpio_set_level(QSPI_PIN_NUM_LCD_RST,0);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(QSPI_PIN_NUM_LCD_RST,1);
        vTaskDelay(pdMS_TO_TICKS(10));

    }

    void InitializePowerSaveTimer() {
        power_save_timer_ = new PowerSaveTimer(-1, 60, 300);
        power_save_timer_->OnEnterSleepMode([this]() {
            GetDisplay()->SetPowerSaveMode(true);
            GetBacklight()->SetBrightness(20); });
        power_save_timer_->OnExitSleepMode([this]() {
            GetDisplay()->SetPowerSaveMode(false);
            GetBacklight()->RestoreBrightness(); });
        // power_save_timer_->OnShutdownRequest([this](){ 
        //     pmic_->PowerOff(); });
        power_save_timer_->SetEnabled(true);
    }

    void InitializeCodecI2c() {
        ESP_LOGI(TAG, "Initialize I2C0: SDA=%d, SCL=%d",
                 AUDIO_CODEC_I2C_SDA_PIN, AUDIO_CODEC_I2C_SCL_PIN);
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = AUDIO_CODEC_I2C_SDA_PIN,
            .scl_io_num = AUDIO_CODEC_I2C_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &i2c_bus_));
    }


    void InitializeSpi() {
        ESP_LOGI(TAG, "Initialize QSPI bus");

        const spi_bus_config_t bus_config = TAIJIPI_ST77916_PANEL_BUS_QSPI_CONFIG(QSPI_PIN_NUM_LCD_PCLK,
                                                                        QSPI_PIN_NUM_LCD_DATA0,
                                                                        QSPI_PIN_NUM_LCD_DATA1,
                                                                        QSPI_PIN_NUM_LCD_DATA2,
                                                                        QSPI_PIN_NUM_LCD_DATA3,
                                                                        QSPI_LCD_H_RES * 80 * sizeof(uint16_t));
        ESP_ERROR_CHECK(spi_bus_initialize(QSPI_LCD_HOST, &bus_config, SPI_DMA_CH_AUTO));
    }

    void Initializest77916Display() {
        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_lcd_panel_handle_t panel = nullptr;

        ESP_LOGI(TAG, "Install panel IO");

        esp_lcd_panel_io_spi_config_t io_config = {
            .cs_gpio_num = QSPI_PIN_NUM_LCD_CS,               
            .dc_gpio_num = GPIO_NUM_NC,
            .spi_mode = 0,                     
            .pclk_hz = 3 * 1000 * 1000,      
            .trans_queue_depth = 10,            
            .on_color_trans_done = NULL,                            
            .user_ctx = NULL,                   
            .lcd_cmd_bits = 32,                 
            .lcd_param_bits = 8,                
            .flags = {                          
            .dc_low_on_data = 0,            
            .octal_mode = 0,                
            .quad_mode = 1,                 
            .sio_mode = 0,                  
            .lsb_first = 0,                 
            .cs_high_active = 0,            
            },                                  
        };
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)QSPI_LCD_HOST, &io_config, &panel_io));

        ESP_LOGI(TAG, "Install ST77916 panel driver");
        
        st77916_vendor_config_t vendor_config = {
            .flags = {
                .use_qspi_interface = 1,
            },
        };
        
        printf("-------------------------------------- Version selection -------------------------------------- \r\n");
        esp_err_t ret;
        int lcd_cmd = 0x04;
        uint8_t register_data[4] = {};
        size_t param_size = sizeof(register_data);
        lcd_cmd &= 0xff;
        lcd_cmd <<= 8;
        lcd_cmd |= LCD_OPCODE_READ_CMD << 24;  // Use the read opcode instead of write
        ret = esp_lcd_panel_io_rx_param(panel_io, lcd_cmd, register_data, param_size); 
        if (ret == ESP_OK) {
            printf("Register 0x04 data: %02x %02x %02x %02x\n", register_data[0], register_data[1], register_data[2], register_data[3]);
        } else {
            printf("Failed to read register 0x04, error code: %d\n", ret);
        } 
        ESP_ERROR_CHECK(esp_lcd_panel_io_del(panel_io));
        panel_io = nullptr;
        io_config.pclk_hz = 80 * 1000 * 1000;
        if (esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)QSPI_LCD_HOST, &io_config, &panel_io) != ESP_OK) {
            printf("Failed to set LCD communication parameters -- SPI\r\n");
            return ;
        }
        printf("LCD communication parameters are set successfully -- SPI\r\n");
        
        // Check register values and configure accordingly
        if (register_data[0] == 0x00 && register_data[1] == 0x7F && register_data[2] == 0x7F && register_data[3] == 0x7F) {
            vendor_config.init_cmds = vendor_specific_init_version_1;
            vendor_config.init_cmds_size = sizeof(vendor_specific_init_version_1) / sizeof(st77916_lcd_init_cmd_t);
            printf("Vendor-specific initialization for case 1.\n");
        }
        else if (register_data[0] == 0x00 && register_data[1] == 0x02 && register_data[2] == 0x7F && register_data[3] == 0x7F) {
            vendor_config.init_cmds = vendor_specific_init_version_2;
            vendor_config.init_cmds_size = sizeof(vendor_specific_init_version_2) / sizeof(st77916_lcd_init_cmd_t);
            printf("Vendor-specific initialization for case 2.\n");
        }
        printf("------------------------------------- End of version selection------------------------------------- \r\n");
 
        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
        panel_config.bits_per_pixel = QSPI_LCD_BIT_PER_PIXEL;
        panel_config.reset_gpio_num = QSPI_PIN_NUM_LCD_RST;
        panel_config.vendor_config = &vendor_config;
        ESP_ERROR_CHECK(esp_lcd_new_panel_st77916(panel_io, &panel_config, &panel));

        esp_lcd_panel_reset(panel);
        esp_lcd_panel_init(panel);
        esp_lcd_panel_disp_on_off(panel, true);
        esp_lcd_panel_swap_xy(panel, DISPLAY_SWAP_XY);
        esp_lcd_panel_mirror(panel, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);

        display_ = new NaraFaceDisplay(panel_io, panel,
                                      DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y,
                                      DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
    }

    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting) {
                EnterWifiConfigMode();
                return;
            }
            app.ToggleChatState();
        });

        // A configured device no longer falls into provisioning just because
        // known Wi-Fi is temporarily absent. Keep a deliberate physical escape
        // hatch: hold BOOT for ~3 seconds to reopen Wi-Fi setup/recovery.
        boot_button_.OnLongPress([this]() {
            auto state = Application::GetInstance().GetDeviceState();
            if (state == kDeviceStateIdle || state == kDeviceStateStarting) {
                EnterWifiConfigMode();
            }
        });
#if CONFIG_USE_DEVICE_AEC
        boot_button_.OnDoubleClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateIdle) {
                app.SetAecMode(app.GetAecMode() == kAecOff ? kAecOnDeviceSide : kAecOff);
            }
        });
#endif
    }


public:
    WaveshareEsp32s3TouchLcd1_85B() : boot_button_(BOOT_BUTTON_GPIO, false, 3000) {
        InitializePowerSaveTimer();
        InitializeCodecI2c();
        InitializePhysicalSensors();
        InitializeBatteryGauge();
        st77916_reset();
        InitializeSpi();
        Initializest77916Display();
        InitializeButtons();
        InitializeReactionTools();
        GetBacklight()->RestoreBrightness();
        InitializeOptionalVision();
        StartPhysicalReflexTask();
    }

    virtual AudioCodec* GetAudioCodec() override {
        static BoxAudioCodec audio_codec(
            i2c_bus_, 
            AUDIO_INPUT_SAMPLE_RATE, 
            AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_MCLK, 
            AUDIO_I2S_GPIO_BCLK, 
            AUDIO_I2S_GPIO_WS, 
            AUDIO_I2S_GPIO_DOUT, 
            AUDIO_I2S_GPIO_DIN,
            AUDIO_CODEC_PA_PIN, 
            AUDIO_CODEC_ES8311_ADDR, 
            AUDIO_CODEC_ES7210_ADDR, 
            AUDIO_INPUT_REFERENCE);
        return &audio_codec;
    }

    virtual Display* GetDisplay() override {
        return display_;
    }

    virtual Backlight* GetBacklight() override {
        static PwmBacklight backlight(DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT);
        return &backlight;
    }

    virtual bool GetBatteryLevel(int& level, bool& charging, bool& discharging) override {
        uint16_t soc = 0;
        uint16_t voltage_mv = 0;
        uint16_t raw_current = 0;
        if (bq27220_ == nullptr ||
            !ReadWord(bq27220_, 0x2C, soc) ||
            !ReadWord(bq27220_, 0x08, voltage_mv) ||
            !ReadWord(bq27220_, 0x0C, raw_current) ||
            soc > 100 || voltage_mv < 2500 || voltage_mv > 4600) {
            return false;
        }

        const int16_t current_ma = static_cast<int16_t>(raw_current);
        level = static_cast<int>(soc);
        charging = current_ma < -5;
        discharging = current_ma > 5;
        return true;
    }

    virtual void SetPowerSaveLevel(PowerSaveLevel level) override {
        if (level != PowerSaveLevel::LOW_POWER) {
            power_save_timer_->WakeUp();
        }
        WifiBoard::SetPowerSaveLevel(level);
    }
};

DECLARE_BOARD(WaveshareEsp32s3TouchLcd1_85B);
