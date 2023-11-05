// This file was generated by SquareLine Studio
// SquareLine Studio version: SquareLine Studio 1.3.0
// LVGL version: 8.3.6
// Project name: SquareLine_Project

#include "ui.h"
#include "ui_helpers.h"

///////////////////// VARIABLES ////////////////////

// SCREEN: ui_Screen1
void ui_Screen1_screen_init(void);
lv_obj_t * ui_Screen1;
lv_obj_t * ui_Image1;
lv_obj_t * ui_loadingBar;
lv_obj_t * ui_Label4;
lv_obj_t * ui_Label11;

// SCREEN: ui_Screen2
void ui_Screen2_screen_init(void);
lv_obj_t * ui_Screen2;
lv_obj_t * ui_PanelGear;
lv_obj_t * ui_LabeltextGear;
lv_obj_t * ui_LabelGear;
lv_obj_t * ui_PanelRPM;
lv_obj_t * ui_LabeltextRPM;
lv_obj_t * ui_LabelRPM;
lv_obj_t * ui_PanelSpeed;
lv_obj_t * ui_LabeltextSpeed;
lv_obj_t * ui_LabelSpeed;
lv_obj_t * ui_PanelGForce;
lv_obj_t * ui_LabeltextGForce;
lv_obj_t * ui_LabelGForce;
lv_obj_t * ui_LabelTPS;
lv_obj_t * ui_LabelBPS;
lv_obj_t * ui_BarRPM;
lv_obj_t * ui_LabelRev5K;
lv_obj_t * ui_LabelRev15;
lv_obj_t * ui_LabelRev10;
lv_obj_t * ui_BarTPS;
lv_obj_t * ui_BarBPS;
lv_obj_t * ui____initial_actions0;
const lv_img_dsc_t * ui_imgset_1360207280[1] = {&ui_img_227049602};

///////////////////// TEST LVGL SETTINGS ////////////////////
#if LV_COLOR_DEPTH != 16
    #error "LV_COLOR_DEPTH should be 16bit to match SquareLine Studio's settings"
#endif
#if LV_COLOR_16_SWAP !=0
    #error "LV_COLOR_16_SWAP should be 0 to match SquareLine Studio's settings"
#endif

///////////////////// ANIMATIONS ////////////////////

///////////////////// FUNCTIONS ////////////////////

///////////////////// SCREENS ////////////////////

void ui_init(void)
{
    lv_disp_t * dispp = lv_disp_get_default();
    lv_theme_t * theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                                               false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    ui_Screen1_screen_init();
    ui_Screen2_screen_init();
    ui____initial_actions0 = lv_obj_create(NULL);
    lv_disp_load_scr(ui_Screen1);
}
