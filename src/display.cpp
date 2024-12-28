#include <Arduino.h>
#include <TFT_eSPI.h>
#include <display.h>
#include <drivers/display/tft_espi/lv_tft_espi.h>
#include <lvgl.h>
#include <myFonts.h>
#include <display/lv_display_private.h>
#include <touch_calibrate.h>

#define TFT_ROTATION LV_DISPLAY_ROTATION_90
/*Set to your screen resolution and rotation*/
#define TFT_HOR_RES (TFT_WIDTH)  // TFT_HEIGHT
#define TFT_VER_RES (TFT_HEIGHT) // TFT_WIDTH

/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is
 * in bytes*/
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))
// uint32_t draw_buf[DRAW_BUF_SIZE / 4];

TFT_eSPI * tft;

static lv_obj_t *screen[SCR_MAX];
static InfoBar *bars[SCR_MAX];
// define the text areas
static lv_obj_t *textAreas[SCR_MAX];

static lv_obj_t *createMain();



#if LV_USE_LOG != 0
void my_print(lv_log_level_t level, const char *buf) {
    LV_UNUSED(level);
    Serial.println(buf);
    Serial.flush();
}
#endif

/* LVGL calls it when a rendered image needs to copied to the display*/
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    /*Copy `px map` to the `area`*/

    /*For example ("my_..." functions needs to be implemented by you)
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);

    my_set_window(area->x1, area->y1, w, h);
    my_draw_bitmaps(px_map, w * h);
     */

    /*Call it to tell LVGL you are ready*/
    lv_display_flush_ready(disp);
}

/*Read the touchpad*/
void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
    uint16_t touchX = 0, touchY = 0;

    bool touched = tft->getTouch(&touchX, &touchY, 600);

    if (!touched) {
        data->state = LV_INDEV_STATE_REL;
    } else {
        data->state = LV_INDEV_STATE_PR;

        /*Set the coordinates*/
        data->point.x = touchX;
        data->point.y = touchY;
    }
}

/*use Arduinos millis() as tick source*/
static uint32_t my_tick(void) { return millis(); }


/**********************
 *   STATIC FUNCTIONS
 **********************/
uint32_t *draw_buf;
static void flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft->startWrite();
    tft->setAddrWindow(area->x1, area->y1, w, h);
    tft->pushColors((uint16_t *)px_map, w * h, true);
    tft->endWrite();

    lv_display_flush_ready(disp);
}

void displayInit() {
    Serial.printf("Display Init\n");
    Serial.printf("DRAWBUFSIZE %ld\n", DRAW_BUF_SIZE);

    Serial.printf("W %d H %d BARH %d BARW %d BODH %d\n", TFT_WIDTH,
                  TFT_HEIGHT, BAR_HEIGHT, BAR_WIDTH, BODY_HEIGHT);
    String LVGL_Arduino = "Hello Arduino! ";
    LVGL_Arduino += String('V') + lv_version_major() + "." +
                    lv_version_minor() + "." + lv_version_patch();

    Serial.begin(115200);
    Serial.println(LVGL_Arduino);

    draw_buf = (uint32_t *)malloc(DRAW_BUF_SIZE);
    if (!draw_buf) {
        Serial.printf("Failed to allocate %d bytes for draw_buf\n",
                      DRAW_BUF_SIZE);
        return;
    }
    lv_init();

    /*Set a tick source so that LVGL will know how much time elapsed. */
    lv_tick_set_cb(my_tick);

    /* register print function for debugging */
#if LV_USE_LOG != 0
    lv_log_register_print_cb(my_print);
#endif

    lv_display_t * disp = lv_display_create(TFT_WIDTH, TFT_HEIGHT);
    tft = new TFT_eSPI(TFT_WIDTH, TFT_HEIGHT);
    // Read the calibration dta from SPIFFS if it exists
    bool status = touch_calibrate(tft);
    if(status) {
        Serial.printf("Touch calibration successful");
    } else {
        Serial.printf("Touch calibration unsuccessful");
    }

    tft->begin();          /* TFT init */
    tft->setRotation(TFT_ROTATION);
    lv_display_set_flush_cb(disp, flush_cb);
    lv_display_set_buffers(disp, (void *)draw_buf, NULL, DRAW_BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);  

        /*Initialize the (dummy) input device driver*/
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); /*Touchpad should have POINTER type*/
    lv_indev_set_read_cb(indev, my_touchpad_read);
 
    screen[SCR_MAIN] = createMain();

    lv_scr_load(screen[SCR_MAIN]);
    handleDisplay();
    Serial.println("Setup done");
}

void handleDisplay() {
    static const uint32_t tick_delay = 10;
    lv_task_handler(); /* let the GUI do its work */
    lv_tick_inc(tick_delay);
}



static void setupCommonstyles(lv_obj_t *obj) {
    lv_obj_set_style_pad_gap(obj, padding, 0);

    lv_obj_set_height(obj, BODY_HEIGHT);
    lv_obj_set_width(obj, TFT_WIDTH);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

static void setupHeader(Screens scr, lv_obj_t *screen, const char *title) {
    // Info bar at the tope
    InfoBar *bar = new InfoBar(screen, BAR_ROW_TOP);
    bars[scr] = bar;
    bar->setValue(title);
}

static void setupMenu(lv_obj_t *screen) {
    MenuBar *menuBar = new MenuBar(screen, BAR_ROW_BOTTOM);
}

static void buttonHandler(lv_event_t *e) { Serial.printf("Pressed:\n"); }

static lv_obj_t *createMain() {
    lv_obj_t *screen = lv_obj_create(NULL);
    setupCommonstyles(screen);
    setupHeader(SCR_MAIN, screen, "NMEA2000 Tester");

    // Main body for the chart and sky viwe
    static lv_style_t style;
    lv_style_init(&style);

    lv_style_set_bg_opa(&style, LV_OPA_100);
    lv_style_set_bg_color(&style, lv_palette_main(LV_PALETTE_GREEN));
    lv_style_set_pad_all(&style, 0);
    lv_obj_t *body = lv_obj_create(screen);
    lv_obj_set_pos(body, 0, BAR_HEIGHT);
    lv_obj_set_width(body, TFT_WIDTH);
    lv_obj_set_height(body, BODY_HEIGHT);
    lv_obj_remove_flag(body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(body, &style, 0);

    lv_event_code_t code = LV_EVENT_PRESSED;
    lv_obj_add_event_cb(body, buttonHandler, code, (void *)NULL);
    setupMenu(screen);

    return screen;
}