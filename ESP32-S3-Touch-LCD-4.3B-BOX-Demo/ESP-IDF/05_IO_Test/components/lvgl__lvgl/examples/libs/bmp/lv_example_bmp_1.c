#include "../../lv_examples.h"

#if LV_USE_BMP && LV_BUILD_EXAMPLES

/**
 * Open a BMP file from a file
 */
void lv_example_bmp_1(void)
{

    LV_IMG_DECLARE(img_wink_bmp);
    lv_obj_t * img;

    img = lv_img_create(lv_scr_act());
    lv_img_set_src(img, &img_wink_bmp);
    // lv_obj_align(img, LV_ALIGN_LEFT_MID, 20, 0);


//     lv_obj_t * img = lv_img_create(lv_scr_act());
//     /* Assuming a File system is attached to letter 'A'
//      * E.g. set LV_USE_FS_STDIO 'A' in lv_conf.h */
// #if LV_COLOR_DEPTH == 32
//     lv_img_set_src(img, "A:lvgl__lvgl/examples/libs/bmp/example_32bit.bmp");
// #elif LV_COLOR_DEPTH == 16
//     lv_img_set_src(img, "A:lvgl__lvgl/examples/libs/bmp/example_16bit.bmp");
// #endif
    lv_obj_center(img);

}

#endif
