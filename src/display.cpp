#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <display.h>
#include <drivers/display/tft_espi/lv_tft_espi.h>
#include <myFonts.h>

#define TFT_ROTATION LV_DISPLAY_ROTATION_90
    /*Set to your screen resolution and rotation*/
#define TFT_HOR_RES   (TFT_WIDTH) //TFT_HEIGHT
#define TFT_VER_RES   (TFT_HEIGHT) // TFT_WIDTH

/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))
//uint32_t draw_buf[DRAW_BUF_SIZE / 4];

static lv_obj_t *screen[SCR_MAX];
static InfoBar* bars[SCR_MAX];
// define the text areas
static lv_obj_t* textAreas[SCR_MAX];

static lv_obj_t * createMain();

static const uint32_t border = 1, padding = 0;

#if LV_USE_LOG != 0
void my_print( lv_log_level_t level, const char * buf )
{
    LV_UNUSED(level);
    Serial.println(buf);
    Serial.flush();
}
#endif

/* LVGL calls it when a rendered image needs to copied to the display*/
void my_disp_flush( lv_display_t *disp, const lv_area_t *area, uint8_t * px_map)
{
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
void my_touchpad_read( lv_indev_t * indev, lv_indev_data_t * data )
{
    /*For example  ("my_..." functions needs to be implemented by you)
    int32_t x, y;
    bool touched = my_get_touch( &x, &y );

    if(!touched) {
        data->state = LV_INDEV_STATE_RELEASED;
    } else {
        data->state = LV_INDEV_STATE_PRESSED;

        data->point.x = x;
        data->point.y = y;
    }
     */
}

/*use Arduinos millis() as tick source*/
static uint32_t my_tick(void)
{
    return millis();
}

// Constructor. Binds to the parent object.
Indicator::Indicator(lv_obj_t* parent, const char* name, uint32_t x, uint32_t y) {
    container = lv_obj_create(parent);
    lv_obj_set_pos(container, x, y);
    lv_obj_set_width(container, IND_WIDTH - (2 * padding));
    lv_obj_set_height(container, IND_HEIGHT - (2 * padding));

    lv_style_init(&style);
    lv_style_set_border_width(&style, border);
    lv_obj_add_style(container, &style, 0);

    lv_obj_set_layout(container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    label = lv_label_create(container);
    lv_label_set_text(label, name);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);

    lv_style_init(&text_style);
    lv_style_set_text_font(&text_style, &RobotoCondensedVariableFont_wght8);
    lv_obj_add_style(label, &text_style, 0);

    text = lv_label_create(container);
    lv_style_init(&value_style);
    lv_style_set_text_font(&value_style, &RobotoCondensedVariableFont_wght32);
    lv_obj_add_style(text, &value_style, 0);
    lv_obj_set_style_text_align(text, LV_TEXT_ALIGN_CENTER, 0);

    lv_label_set_text(text, "---");
}

// Change the text size
void Indicator::setFont(const lv_font_t* value) {
    lv_style_set_text_font(&value_style, value);
}

// set the value using a double and precision.
void Indicator::setValue(double value, const char* units, uint32_t prec) {
    String v(value, prec);
    v += units;
    setValue(v.c_str());
}

void Indicator::setValue(const char* value) {
    lv_label_set_text(text, value);
}

// Constructor. Binds to the parent object.
// Info bar has the screen title and the time
InfoBar::InfoBar(lv_obj_t* parent, uint32_t y) {
    static lv_style_t style;
    static lv_style_t value_style;

    container = lv_obj_create(parent);
    lv_obj_set_pos(container, 0, y);
    lv_obj_set_width(container, (BAR_WIDTH)-(2 * padding));
    lv_obj_set_height(container, (BAR_HEIGHT)-2 * padding);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    lv_style_init(&style);
    lv_style_set_border_width(&style, border);

    lv_style_set_radius(&style, 3);

    lv_style_set_bg_opa(&style, LV_OPA_100);
    lv_style_set_bg_color(&style, lv_palette_main(LV_PALETTE_BLUE));

    lv_obj_add_style(container, &style, 0);

    //    lv_obj_set_layout(container, LV_LAYOUT_FLEX);
    //    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);

    // Title text
    text = lv_label_create(container);
    lv_style_init(&value_style);
    lv_style_set_bg_opa(&value_style, LV_OPA_100);
    lv_style_set_bg_color(&value_style, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_text_color(&value_style, lv_color_white());
    lv_style_set_pad_all(&value_style, 10);
    lv_style_set_text_font(&value_style, &RobotoCondensedVariableFont_wght24);
    lv_obj_add_style(text, &value_style, 0);
    lv_obj_set_align(text, LV_ALIGN_LEFT_MID);

    curTime = lv_label_create(container);
    lv_obj_add_style(curTime, &value_style, 0);
    lv_label_set_text(curTime, "00:00:00");
    lv_obj_set_align(curTime, LV_ALIGN_RIGHT_MID);
}

MenuBar::MenuBar(lv_obj_t* parent, uint32_t y) {
    // Constructor. Binds to the parent object.
    static lv_style_t style;

    container = lv_obj_create(parent);
    lv_obj_set_pos(container, 0, y);
    lv_obj_set_width(container, (BAR_WIDTH)-2 * padding);
    lv_obj_set_height(container, BAR_MENU_HEIGHT);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    lv_style_init(&style);
    lv_style_set_border_width(&style, border);
    lv_style_set_pad_all(&style, 0);
    lv_obj_add_style(container, &style, 0);

    lv_obj_set_layout(container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);
}

uint32_t * draw_buf;

void displayInit() {
    Serial.printf("Display Init\n");
    Serial.printf("DRAWBUFSIZE %ld\n", DRAW_BUF_SIZE);

    Serial.printf("W %d H %d BARH %d BARW %d BODH %d\n",  DISP_WIDTH, DISP_HEIGHT, BAR_HEIGHT, BAR_WIDTH, BODY_HEIGHT);
    String LVGL_Arduino = "Hello Arduino! ";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

    Serial.begin( 115200 );
    Serial.println( LVGL_Arduino );

    draw_buf = (uint32_t *)malloc(DRAW_BUF_SIZE);
    if(!draw_buf) {
        Serial.printf("Failed to allocate %d bytes for draw_buf\n", DRAW_BUF_SIZE);
        return;
    }
    lv_init();

    /*Set a tick source so that LVGL will know how much time elapsed. */
    lv_tick_set_cb(my_tick);

    /* register print function for debugging */
#if LV_USE_LOG != 0
    lv_log_register_print_cb( my_print );
#endif

    lv_display_t * disp;
    /*TFT_eSPI can be enabled lv_conf.h to initialize the display in a simple way*/
    disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf, DRAW_BUF_SIZE);
    lv_display_set_rotation(disp, TFT_ROTATION);

    /*Initialize the (dummy) input device driver*/
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); /*Touchpad should have POINTER type*/
    lv_indev_set_read_cb(indev, my_touchpad_read);

    lv_obj_t *label = lv_label_create( lv_screen_active() );
    lv_label_set_text( label, "Hello Arduino, I'm LVGL!" );
    lv_obj_align( label, LV_ALIGN_CENTER, 0, 0 );


    screen[SCR_MAIN] = createMain();

    lv_scr_load(screen[SCR_MAIN]);
    handleDisplay();
    Serial.println( "Setup done" );
}

void handleDisplay()
{
    lv_timer_handler();
    return;
    static const uint32_t tick_delay = 100;
    lv_task_handler(); /* let the GUI do its work */
    lv_tick_inc(tick_delay);
}

void InfoBar::setValue(const char* value) {
    lv_label_set_text(text, value);
}

void InfoBar::setTime(const char* t) {
    lv_label_set_text(curTime, t);
}

static void setupCommonstyles(lv_obj_t* obj) {
    lv_obj_set_style_pad_gap(obj, padding, 0);

    lv_obj_set_height(obj, BODY_HEIGHT);
    lv_obj_set_width(obj, DISP_WIDTH);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

static void setupHeader(Screens scr, lv_obj_t* screen, const char* title) {
    // Info bar at the tope
    InfoBar* bar = new InfoBar(screen, BAR_ROW_TOP);
    bars[scr] = bar;
    bar->setValue(title);
}

static void setupMenu(lv_obj_t* screen) {
    MenuBar* menuBar = new MenuBar(screen, BAR_ROW_BOTTOM);
}

static lv_obj_t * createMain() {
    lv_obj_t * screen = lv_obj_create(NULL);
    setupCommonstyles(screen);
    setupHeader(SCR_MAIN, screen, "NMEA2000 Tester");

    // Main body for the chart and sky viwe
    static lv_style_t style;
    lv_style_init(&style);

//    lv_style_set_bg_opa(&style, LV_OPA_100);
//    lv_style_set_bg_color(&style, lv_palette_main(LV_PALETTE_GREEN));
//    lv_style_set_pad_all(&style, 0);
//    lv_obj_t* body = lv_obj_create(screen);
//    lv_obj_set_pos(body, 0, BAR_HEIGHT);
//    lv_obj_set_width(body, DISP_WIDTH);
//    lv_obj_set_height(body, BODY_HEIGHT);
//    lv_obj_remove_flag(body, LV_OBJ_FLAG_SCROLLABLE);
//    lv_obj_add_style(body, &style, 0);

    setupMenu(screen);
    
    return screen;
}