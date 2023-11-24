#include <lvgl.h>
#include <TFT_eSPI.h>
#include <TFT_Touch.h>

/*Change to your screen resolution*/
static const uint16_t screenWidth  = 320;//480;
static const uint16_t screenHeight = 240;//320;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[ screenWidth * screenHeight/4 ];

TFT_eSPI tft = TFT_eSPI(screenWidth, screenHeight); /* TFT instance */

// These are the pins used to interface between the 2046 touch controller and Arduino Pro
#define DOUT 39  /* Data out pin (T_DO) of touch screen */
#define DIN  32  /* Data in pin (T_DIN) of touch screen */
#define DCS  33  /* Chip select pin (T_CS) of touch screen */
#define DCLK 25  /* Clock pin (T_CLK) of touch screen */

// Rotation 0: Portrait, 1: Landscape, 2: Portrait flipped, 3: Landscape flipped
#define DISP_ROTATION 1
#define TOUCH_PRINT 1 // Serial.println on touch? Set to 0 to prevent it

/* Create an instance of the touch screen library */
TFT_Touch touch = TFT_Touch(DCS, DCLK, DIN, DOUT);

/* Used widgets */
static lv_obj_t * label;


/* Display flushing */
void my_disp_flush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p )
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );

    tft.startWrite();
    tft.setAddrWindow( area->x1, area->y1, w, h );
    tft.pushColors( ( uint16_t * )&color_p->full, w * h, true );
    tft.endWrite();

    lv_disp_flush_ready( disp );
}

/* Read the touchpad and provide x and y values for lv event data */
void on_touch( lv_indev_drv_t * indev_driver, lv_indev_data_t * data )
{
    uint16_t touchX, touchY;

    bool touched = touch.Pressed();//tft.getTouch( &touchX, &touchY, 600 );

    if( !touched )
    {
        data->state = LV_INDEV_STATE_REL;
    }
    else
    {   
      #if DISP_ROTATION == 0 //Portrait
        touchX = screenHeight - touch.Y();
        touchY = touch.X();
      #elif DISP_ROTATION == 2 //Portrait flipped
        touchX = touch.Y();
        touchY = screenWidth - touch.X();
      #elif DISP_ROTATION == 3 //Landscape flipped
        touchX = screenWidth - touch.X();
        touchY = screenHeight - touch.Y();
      #else //DISP_ROTATION == 1 (Landscape, default)
        touchX = touch.X();
        touchY = touch.Y();
      #endif
        
        data->state = LV_INDEV_STATE_PR;

        /*Set the coordinates*/
        data->point.x = touchX;
        data->point.y = touchY;

      #if TOUCH_PRINT
        Serial.print( "Data x " );
        Serial.println( touchX );

        Serial.print( "Data y " );
        Serial.println( touchY );
      #endif
    }
}


static void slider_event_cb(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);

    /*Refresh the text*/
    lv_label_set_text_fmt(label, "%"LV_PRId32, lv_slider_get_value(slider));
    lv_obj_align_to(label, slider, LV_ALIGN_OUT_TOP_MID, 0, -15);    /*Align top of the slider*/
}

/**
 * Create a slider and write its value on a label.
 */
void lv_example_slider(void)
{
    /*Create a slider in the center of the display*/
    lv_obj_t * slider = lv_slider_create(lv_scr_act());
    lv_obj_set_width(slider, 200);                          /*Set the width*/
    lv_obj_center(slider);                                  /*Align to the center of the parent (screen)*/
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);     /*Assign an event function*/

    /*Create a label above the slider*/
    label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "0");
    lv_obj_align_to(label, slider, LV_ALIGN_OUT_TOP_MID, 0, -15);    /*Align top of the slider*/
}

void setup()
{
    Serial.begin( 115200 ); /* prepare for possible serial debug */

    String LVGL_Arduino = "Hello Arduino! ";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

    Serial.println( LVGL_Arduino );
    Serial.println( "I am LVGL_Arduino" );

    lv_init();

    tft.begin();          /* TFT init */
    tft.setRotation(DISP_ROTATION);//( 3 ); /* Landscape orientation, flipped */

    /*Set the touchscreen calibration data,
     the actual data for your display can be acquired using
     the Generic -> Touch_calibrate example from the TFT_eSPI library*/
    //uint16_t calData[] = { 456, 3608, 469, 272, 3625, 3582, 3518, 263,  };
//    tft.setTouchCalibrate(calData);//   
//    uint16_t calData[5] = { 275, 3620, 264, 3532, 1 };
//    tft.setTouch( calData );
    touch.setCal(526, 3443, 750, 3377, 320, 240, 1);

    lv_disp_draw_buf_init( &draw_buf, buf, NULL, screenWidth * 10 );

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init( &disp_drv );
    /* Change screen resolution according to screen rotation */
    disp_drv.hor_res = (DISP_ROTATION % 2 == 1) ? screenWidth : screenHeight;
    disp_drv.ver_res = (DISP_ROTATION % 2 == 1) ? screenHeight : screenWidth;
    disp_drv.flush_cb = my_disp_flush; //刷新
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register( &disp_drv );

    /*Initialize the (dummy) input device driver*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init( &indev_drv );
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = on_touch;
    lv_indev_drv_register( &indev_drv );

    // ---------  LVGL content --------
    lv_example_slider();

    /* Create simple label */

    lv_obj_t *versionLabel = lv_label_create( lv_scr_act() );
    lv_label_set_text( versionLabel, LVGL_Arduino.c_str() );
    lv_obj_align( versionLabel, LV_ALIGN_BOTTOM_MID, 0, 0 );
    Serial.println( "Setup done" );
}

void loop()
{
    lv_timer_handler(); /* let the GUI do its work */
    delay( 5 );
}
//http://lvgl.100ask.org/8.2/index.html
//https://www.cnblogs.com/frozencandles/archive/2022/06/14/16375392.html
