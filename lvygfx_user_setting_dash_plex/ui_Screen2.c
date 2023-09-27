// This file was generated by SquareLine Studio
// SquareLine Studio version: SquareLine Studio 1.3.0
// LVGL version: 8.3.6
// Project name: SquareLine_Project

#include "ui.h"

void ui_Screen2_screen_init(void)
{
    ui_Screen2 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen2, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_Screen2, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_PanelGear = lv_obj_create(ui_Screen2);
    lv_obj_set_width(ui_PanelGear, 120);
    lv_obj_set_height(ui_PanelGear, 170);
    lv_obj_set_x(ui_PanelGear, 0);
    lv_obj_set_y(ui_PanelGear, 30);
    lv_obj_set_align(ui_PanelGear, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_PanelGear, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_PanelGear, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_PanelGear, lv_color_hex(0x0B0B0B), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_PanelGear, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_PanelGear, lv_color_hex(0x7F7F7F), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_PanelGear, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LabeltextGear = lv_label_create(ui_PanelGear);
    lv_obj_set_width(ui_LabeltextGear, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_LabeltextGear, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_LabeltextGear, 0);
    lv_obj_set_y(ui_LabeltextGear, 75);
    lv_obj_set_align(ui_LabeltextGear, LV_ALIGN_CENTER);
    lv_label_set_text(ui_LabeltextGear, "GEAR");
    lv_obj_set_style_text_color(ui_LabeltextGear, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_LabeltextGear, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LabelGear = lv_label_create(ui_PanelGear);
    lv_obj_set_width(ui_LabelGear, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_LabelGear, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_LabelGear, 2);
    lv_obj_set_y(ui_LabelGear, -25);
    lv_obj_set_align(ui_LabelGear, LV_ALIGN_CENTER);
    lv_label_set_text(ui_LabelGear, "2");
    lv_obj_set_style_text_color(ui_LabelGear, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_LabelGear, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_LabelGear, &ui_font_OD180, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_PanelRPM = lv_obj_create(ui_Screen2);
    lv_obj_set_width(ui_PanelRPM, 90);
    lv_obj_set_height(ui_PanelRPM, 70);
    lv_obj_set_x(ui_PanelRPM, -110);
    lv_obj_set_y(ui_PanelRPM, -10);
    lv_obj_set_align(ui_PanelRPM, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_PanelRPM, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_PanelRPM, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_PanelRPM, lv_color_hex(0x131313), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_PanelRPM, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_PanelRPM, lv_color_hex(0x6E6E6E), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_PanelRPM, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LabeltextRPM = lv_label_create(ui_PanelRPM);
    lv_obj_set_width(ui_LabeltextRPM, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_LabeltextRPM, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_LabeltextRPM, 0);
    lv_obj_set_y(ui_LabeltextRPM, 20);
    lv_obj_set_align(ui_LabeltextRPM, LV_ALIGN_CENTER);
    lv_label_set_text(ui_LabeltextRPM, "RPM");
    lv_obj_set_style_text_color(ui_LabeltextRPM, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_LabeltextRPM, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LabelRPM = lv_label_create(ui_PanelRPM);
    lv_obj_set_width(ui_LabelRPM, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_LabelRPM, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_LabelRPM, 0);
    lv_obj_set_y(ui_LabelRPM, -10);
    lv_obj_set_align(ui_LabelRPM, LV_ALIGN_CENTER);
    lv_label_set_text(ui_LabelRPM, "12000");
    lv_obj_set_style_text_color(ui_LabelRPM, lv_color_hex(0xD37D06), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_LabelRPM, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_LabelRPM, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_PanelSpeed = lv_obj_create(ui_Screen2);
    lv_obj_set_width(ui_PanelSpeed, 90);
    lv_obj_set_height(ui_PanelSpeed, 70);
    lv_obj_set_x(ui_PanelSpeed, -109);
    lv_obj_set_y(ui_PanelSpeed, 80);
    lv_obj_set_align(ui_PanelSpeed, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_PanelSpeed, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_PanelSpeed, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_PanelSpeed, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_PanelSpeed, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_PanelSpeed, lv_color_hex(0x898989), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_PanelSpeed, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LabeltextSpeed = lv_label_create(ui_PanelSpeed);
    lv_obj_set_width(ui_LabeltextSpeed, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_LabeltextSpeed, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_LabeltextSpeed, 0);
    lv_obj_set_y(ui_LabeltextSpeed, 20);
    lv_obj_set_align(ui_LabeltextSpeed, LV_ALIGN_CENTER);
    lv_label_set_text(ui_LabeltextSpeed, "SPEED");
    lv_obj_set_style_text_color(ui_LabeltextSpeed, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_LabeltextSpeed, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_LabeltextSpeed, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LabelSpeed = lv_label_create(ui_PanelSpeed);
    lv_obj_set_width(ui_LabelSpeed, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_LabelSpeed, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_LabelSpeed, 0);
    lv_obj_set_y(ui_LabelSpeed, -10);
    lv_obj_set_align(ui_LabelSpeed, LV_ALIGN_CENTER);
    lv_label_set_text(ui_LabelSpeed, "50");
    lv_obj_set_style_text_color(ui_LabelSpeed, lv_color_hex(0xD37D06), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_LabelSpeed, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_LabelSpeed, &lv_font_montserrat_40, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_PanelGForce = lv_obj_create(ui_Screen2);
    lv_obj_set_width(ui_PanelGForce, 90);
    lv_obj_set_height(ui_PanelGForce, 60);
    lv_obj_set_x(ui_PanelGForce, 111);
    lv_obj_set_y(ui_PanelGForce, 85);
    lv_obj_set_align(ui_PanelGForce, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_PanelGForce, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_PanelGForce, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_PanelGForce, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_PanelGForce, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_PanelGForce, lv_color_hex(0x8B8B8B), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_PanelGForce, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LabeltextGForce = lv_label_create(ui_PanelGForce);
    lv_obj_set_width(ui_LabeltextGForce, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_LabeltextGForce, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_LabeltextGForce, 0);
    lv_obj_set_y(ui_LabeltextGForce, 15);
    lv_obj_set_align(ui_LabeltextGForce, LV_ALIGN_CENTER);
    lv_label_set_text(ui_LabeltextGForce, "G-FORCE");
    lv_obj_set_style_text_color(ui_LabeltextGForce, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_LabeltextGForce, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_LabeltextGForce, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LabelGForce = lv_label_create(ui_PanelGForce);
    lv_obj_set_width(ui_LabelGForce, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_LabelGForce, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_LabelGForce, 0);
    lv_obj_set_y(ui_LabelGForce, -10);
    lv_obj_set_align(ui_LabelGForce, LV_ALIGN_CENTER);
    lv_label_set_text(ui_LabelGForce, "1.0");
    lv_obj_set_style_text_color(ui_LabelGForce, lv_color_hex(0xD37D06), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_LabelGForce, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_LabelGForce, &lv_font_montserrat_32, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LabelTPS = lv_label_create(ui_Screen2);
    lv_obj_set_width(ui_LabelTPS, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_LabelTPS, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_LabelTPS, 82);
    lv_obj_set_y(ui_LabelTPS, 42);
    lv_obj_set_align(ui_LabelTPS, LV_ALIGN_CENTER);
    lv_label_set_text(ui_LabelTPS, "TPS");
    lv_obj_set_style_text_color(ui_LabelTPS, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_LabelTPS, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_LabelTPS, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LabelBPS = lv_label_create(ui_Screen2);
    lv_obj_set_width(ui_LabelBPS, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_LabelBPS, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_LabelBPS, 133);
    lv_obj_set_y(ui_LabelBPS, 41);
    lv_obj_set_align(ui_LabelBPS, LV_ALIGN_CENTER);
    lv_label_set_text(ui_LabelBPS, "BPS");
    lv_obj_set_style_text_color(ui_LabelBPS, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_LabelBPS, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_LabelBPS, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_BarRPM = lv_bar_create(ui_Screen2);
    lv_bar_set_value(ui_BarRPM, 5, LV_ANIM_OFF);
    lv_obj_set_width(ui_BarRPM, 310);
    lv_obj_set_height(ui_BarRPM, 40);
    lv_obj_set_x(ui_BarRPM, 0);
    lv_obj_set_y(ui_BarRPM, -91);
    lv_obj_set_align(ui_BarRPM, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ui_BarRPM, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_BarRPM, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_BarRPM, 60, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_BarRPM, lv_color_hex(0xC3C3C3), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_BarRPM, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_BarRPM, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_BarRPM, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_BarRPM, lv_color_hex(0xD37D06), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_BarRPM, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    ui_LabelRev5K = lv_label_create(ui_BarRPM);
    lv_obj_set_width(ui_LabelRev5K, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_LabelRev5K, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_LabelRev5K, -69);
    lv_obj_set_y(ui_LabelRev5K, 0);
    lv_obj_set_align(ui_LabelRev5K, LV_ALIGN_CENTER);
    lv_label_set_text(ui_LabelRev5K, "5K |");
    lv_obj_set_style_text_color(ui_LabelRev5K, lv_color_hex(0xF1F1F1), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_LabelRev5K, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_LabelRev5K, &ui_font_OD26, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LabelRev15 = lv_label_create(ui_BarRPM);
    lv_obj_set_width(ui_LabelRev15, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_LabelRev15, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_LabelRev15, 131);
    lv_obj_set_y(ui_LabelRev15, 0);
    lv_obj_set_align(ui_LabelRev15, LV_ALIGN_CENTER);
    lv_label_set_text(ui_LabelRev15, "15K |");
    lv_obj_set_style_text_color(ui_LabelRev15, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_LabelRev15, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_LabelRev15, &ui_font_OD26, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LabelRev10 = lv_label_create(ui_BarRPM);
    lv_obj_set_width(ui_LabelRev10, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_LabelRev10, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_LabelRev10, 31);
    lv_obj_set_y(ui_LabelRev10, 0);
    lv_obj_set_align(ui_LabelRev10, LV_ALIGN_CENTER);
    lv_label_set_text(ui_LabelRev10, "10K |");
    lv_obj_set_style_text_color(ui_LabelRev10, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_LabelRev10, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_LabelRev10, &ui_font_OD26, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_BarTPS = lv_bar_create(ui_Screen2);
    lv_bar_set_value(ui_BarTPS, 25, LV_ANIM_OFF);
    lv_obj_set_width(ui_BarTPS, 35);
    lv_obj_set_height(ui_BarTPS, 80);
    lv_obj_set_x(ui_BarTPS, 84);
    lv_obj_set_y(ui_BarTPS, -13);
    lv_obj_set_align(ui_BarTPS, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ui_BarTPS, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_BarTPS, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_BarTPS, 60, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_BarTPS, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_BarTPS, lv_color_hex(0x00FF00), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_BarTPS, 200, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    ui_BarBPS = lv_bar_create(ui_Screen2);
    lv_bar_set_value(ui_BarBPS, 25, LV_ANIM_OFF);
    lv_obj_set_width(ui_BarBPS, 35);
    lv_obj_set_height(ui_BarBPS, 80);
    lv_obj_set_x(ui_BarBPS, 130);
    lv_obj_set_y(ui_BarBPS, -13);
    lv_obj_set_align(ui_BarBPS, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ui_BarBPS, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_BarBPS, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_BarBPS, 60, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_BarBPS, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_BarBPS, lv_color_hex(0xFF0000), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_BarBPS, 200, LV_PART_INDICATOR | LV_STATE_DEFAULT);

}