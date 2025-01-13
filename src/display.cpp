#include <Arduino.h>
#include <TFT_eSPI.h>
#include <display.h>
#include <Indicator.h>
#include <drivers/display/tft_espi/lv_tft_espi.h>
#include <lvgl.h>
#include <myFonts.h>
#include <display/lv_display_private.h>
#include <touch_calibrate.h>
#include <StringStream.h>
#include <SysInfo.h>
#include <rotary_encoder.h>
#include <GwPrefs.h>
#include <n2ksender.h>
#include <n2kdatadecode.h>

// Forward decls
lv_obj_t *createMain();
lv_obj_t *createCanScreen();
lv_obj_t *createMsgScreen();
lv_obj_t *createSysInfoScreen();
void refreshMainScreen();
void refreshMsgScreen();
void refreshCanScreen();
void refreshSysinfo();
void loadScreen(Screens s);

/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is
 * in bytes*/
#define DRAW_BUF_SIZE (TFT_WIDTH * TFT_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))

// The display object
TFT_eSPI * tft;

extern std::map<int, int> N2kMsgMap;
extern std::map<long, const char *> pgnToStr;

static lv_obj_t *screen[SCR_MAX];
static InfoBar *bars[SCR_MAX];
static MenuBar *menus[SCR_MAX];
static StateButton *ind[SCR_MAX][12];

// define the text areas
static lv_obj_t *textAreas[SCR_MAX];

// Secondary areas for some screens
static lv_obj_t *secondArea[SCR_MAX];

lv_obj_t * cb[MAXN2KMESSAGES];
static Screens curscreen = SCR_INVALID;

// Callbacks to refresh each screen
// NULL means no callback
static void(* refreshCb [SCR_MAX])() = {refreshMainScreen, refreshMsgScreen, refreshCanScreen, refreshSysinfo};

// Groups for navigation
static lv_group_t *group[SCR_MAX];

static lv_obj_t * firstFocus[SCR_MAX];

// Indev for the rotary encoder
lv_indev_t * rotaryIndev;



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

// Read the rotary encoder callback
// returns tru of there was an event
void my_rotary_read(lv_indev_t *indev, lv_indev_data_t *data) {
    RotaryEV ev = rotary_loop();
    data->enc_diff = 0;
    if (ev != ROT_NONE) {
        switch (ev) {
        case ROT_UP:
            data->enc_diff = 1;
            break;
        case ROT_DOWN:
            data->enc_diff = -1;
            break;
        case ROT_CLICK:
            data->state = LV_INDEV_STATE_PRESSED;
            break;
        default:
            break;
        }
        data->continue_reading = false;
        Serial.printf("Rotary callback %d\n", ev);
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
    Serial.printf("DRAWBUFSIZE %d\n", DRAW_BUF_SIZE);

    Serial.printf("W %d H %d BARH %d BARW %d BODH %d\n", TFT_WIDTH,
                  TFT_HEIGHT, BAR_HEIGHT, BAR_WIDTH, BODY_HEIGHT);
    String LVGL_Arduino = "Hello Arduino! ";
    LVGL_Arduino += String('V') + lv_version_major() + "." +
                    lv_version_minor() + "." + lv_version_patch();

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
    lv_display_t * disp;

#if LV_USE_TFT_ESPI
    /*TFT_eSPI can be enabled lv_conf.h to initialize the display in a simple way*/
    disp = lv_tft_espi_create(DISP_WIDTH, DISP_HEIGHT, draw_buf, sizeof(draw_buf));
    lv_display_set_rotation(disp, TFT_ROTATION);

#else
    disp = lv_display_create(DISP_WIDTH, DISP_HEIGHT);
    tft = new TFT_eSPI(TFT_WIDTH, TFT_HEIGHT);
    tft->begin();          /* TFT init */
    tft->setRotation(TFT_ROTATION);

    // Read the calibration data from SPIFFS if it exists
    bool status = touch_calibrate_apply(tft);
    if(status) {
        Serial.printf("Touch calibration successful");
    } else {
        Serial.printf("Touch calibration unsuccessful");
        Serial.printf("Using hard coded values from previous run\n");
        // Use this calibration code in setup():
        // touchCalibration_rotate = parameters[4] & 0x01;
        // touchCalibration_invert_x = parameters[4] & 0x02;
        // touchCalibration_invert_y = parameters[4] & 0x04;
        uint16_t calData[5] = { 258, 3581, 215, 3658, 0x02 };
        tft->setTouch(calData);
    }

 
#endif


    lv_display_set_flush_cb(disp, flush_cb);
    lv_display_set_buffers(disp, (void *)draw_buf, NULL, DRAW_BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);  
//    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);

    /*Initialize the (dummy) input device driver for the touchscreen*/
    lv_indev_t * indev;
    indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); /*Touchpad should have POINTER type*/
    lv_indev_set_read_cb(indev, my_touchpad_read);
 
    // Register the rotary controller driver
    rotaryIndev = lv_indev_create();
    lv_indev_set_type(rotaryIndev, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(rotaryIndev, my_rotary_read);

    screen[SCR_MAIN] = createMain();
    screen[SCR_MSG]  = createMsgScreen();
    screen[SCR_CAN] = createCanScreen();
    screen[SCR_SYSINFO] = createSysInfoScreen();

    loadScreen(SCR_MAIN);
    handleDisplay();
    Serial.println("Setup done");
}

// called regularly to update the screen
void handleDisplay() {
    static const uint32_t tick_delay = 10;
    static time_t last = 0;
    time_t now = millis();

    lv_task_handler(); /* let the GUI do its work */

    // Refresh the currently displayed screen every 1000msecs
    if(now > last + 1000 && curscreen >= 0 && refreshCb[curscreen]) {
        refreshCb[curscreen]();
        last = now;
    }
    lv_tick_inc(tick_delay);
}



static void setupCommonstyles(lv_obj_t *obj) {
    lv_obj_set_style_pad_gap(obj, padding, 0);

    lv_obj_set_height(obj, DISP_HEIGHT);
    lv_obj_set_width(obj, DISP_WIDTH);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

// Group focus callback
void focus_cb(lv_group_t * g) {
    Serial.printf("Group focus callback\n");
}


static void setupHeader(Screens scr, lv_obj_t *screen, const char *title) {
    // Info bar at the top
    InfoBar *bar = new InfoBar(screen, BAR_ROW_TOP);
    bars[scr] = bar;
    bar->setValue(title);
}

static MenuBar * setupMenu(lv_obj_t *screen, Screens scr) {
    MenuBar *menuBar = new MenuBar(screen, BAR_ROW_BOTTOM);
    menuBar->addButton("Home", SCR_MAIN);
    menuBar->addButton("Msg", SCR_MSG);
    menuBar->addButton("Can", SCR_CAN);
    menuBar->addButton("Info", SCR_SYSINFO);
    menuBar->addButton("Sys", SCR_SYSINFO);

    int nb = menuBar->buttons.size();
    // Save the first object to focus on when screen shown
    firstFocus[scr] = menuBar->buttons[0];
    for(int i = 0; i < nb; i++) {
        lv_group_add_obj(group[scr], menuBar->buttons[i]);
        Serial.printf("Adding button to the group\n");
    }

    return menuBar;
}

lv_obj_t *createMain() {
    lv_obj_t *screen = lv_obj_create(NULL);
    setupCommonstyles(screen);
    setupHeader(SCR_MAIN, screen, "NMEA2000 Tester");

    group[SCR_MAIN] = lv_group_create();
    lv_group_set_focus_cb(group[SCR_MAIN], focus_cb);

    // Main body for the chart and sky viwe
    static lv_style_t style;
    lv_style_init(&style);

    lv_style_set_bg_opa(&style, LV_OPA_100);
    lv_style_set_bg_color(&style, lv_color_white());
    lv_style_set_pad_all(&style, 0);

    lv_obj_t *body = lv_obj_create(screen);
    lv_obj_set_pos(body, 0, BAR_HEIGHT);
    lv_obj_set_width(body, DISP_WIDTH);
    lv_obj_set_height(body, BODY_HEIGHT);
    lv_obj_remove_flag(body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(body, &style, 0);

    // the main buttons showing the state of the settings
    ind[SCR_MAIN][SBIDX_CAN] = new StateButton(body, "CAN", COL1, ROW1, CANENBALE);
    ind[SCR_MAIN][SBIDX_YD] = new StateButton(body, "YD", COL1, ROW2, YDENABLE);

    ind[SCR_MAIN][0]->callBack = ToggleSending;
    ind[SCR_MAIN][1]->callBack = ToggleYD;

    // add the buttons to the group for rotary select
    lv_group_add_obj(group[SCR_MAIN], ind[SCR_MAIN][0]->getContainer());
    lv_group_add_obj(group[SCR_MAIN], ind[SCR_MAIN][1]->getContainer());

    menus[SCR_MAIN] = setupMenu(screen, SCR_MAIN);

    return screen;
}

void refreshMainScreen() {
    if(ind[SCR_MAIN][SBIDX_CAN]) {
        ind[SCR_MAIN][SBIDX_CAN]->update();
    }
    if(ind[SCR_MAIN][SBIDX_YD]) {
        ind[SCR_MAIN][SBIDX_YD]->update();
    }
}

lv_obj_t* createSysInfoScreen() {
    lv_obj_t* screen = lv_obj_create(NULL);
    setupCommonstyles(screen);
    setupHeader(SCR_SYSINFO, screen, "System Info");

    group[SCR_SYSINFO] = lv_group_create();
    lv_group_set_focus_cb(group[SCR_SYSINFO], focus_cb);

    // Create a text area to display the info text
    textAreas[SCR_SYSINFO] = lv_textarea_create(screen);
    lv_obj_set_size(textAreas[SCR_SYSINFO], DISP_WIDTH, BODY_HEIGHT);
    lv_obj_align(textAreas[SCR_SYSINFO], LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(textAreas[SCR_SYSINFO], &UbuntuMonoB16, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_update_flag(textAreas[SCR_SYSINFO], LV_OBJ_FLAG_SCROLLABLE, true);
    menus[SCR_SYSINFO] = setupMenu(screen, SCR_SYSINFO);
    return screen;
}

// Callback for the message list checkboxes
// The user data is the index into the N2kSendMessages array
// a state change will update the map in nvram
static void msg_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target_obj(e);
    size_t idx = (size_t)lv_event_get_user_data(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        LV_UNUSED(obj);
        const char * txt = lv_checkbox_get_text(obj);
        const char * state = lv_obj_get_state(obj) & LV_STATE_CHECKED ? "Checked" : "Unchecked";
        LV_UNUSED(txt);
        LV_UNUSED(state);
//        Serial.printf("Changed %s: %s", txt, state);
        N2kSendMessages[idx].Enable(lv_obj_get_state(obj) & LV_STATE_CHECKED);
        saveMap();
    }
}


// Message screen.
// Provaides a list of checkboxes for the PGNS to be sent.
lv_obj_t* createMsgScreen() {
    lv_obj_t* screen = lv_obj_create(NULL);
    setupCommonstyles(screen);
    setupHeader(SCR_MSG, screen, "NMEA200 Config");

    group[SCR_MSG] = lv_group_create();
    lv_group_set_focus_cb(group[SCR_MSG], focus_cb);

    // Main body for the lists of messages
    static lv_style_t style;
    lv_style_init(&style);
    lv_obj_t *body = lv_obj_create(screen);
    lv_obj_set_pos(body, 0, BAR_HEIGHT);
    lv_obj_set_width(body, DISP_WIDTH);
    lv_obj_set_height(body, BODY_HEIGHT);
    lv_obj_remove_flag(body, LV_OBJ_FLAG_SCROLL_CHAIN_VER);
    lv_obj_add_style(body, &style, 0);

    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);

    // Get the list of supported N2K messages
    size_t cnt = nN2kSendMessages;
    for (size_t i = 0; i < cnt; i++) {
        if (i < MAXN2KMESSAGES) {
            // Dont overflow our static storage
            // Yes yes I know should use dynamic storage eg vector
            cb[i] = lv_checkbox_create(body);
            lv_obj_remove_flag(cb[i], LV_OBJ_FLAG_SCROLLABLE);
            lv_checkbox_set_text(cb[i], N2kSendMessages[i].Description);
            lv_obj_add_state(cb[i], N2kSendMessages[i].Scheduler.IsEnabled());
            lv_obj_add_event_cb(cb[i], msg_event_handler, LV_EVENT_ALL, (void *)i);

            // Add to the group for this screen so it can be scrolled by the encoder
            lv_group_add_obj(group[SCR_MSG], cb[i]);
        }
    }

    // start with the first entry at the top of the screen
    lv_obj_scroll_to(body, 0, 0, LV_ANIM_OFF);
    lv_obj_update_layout(body);
    menus[SCR_MSG] = setupMenu(screen, SCR_MSG);
    return screen;
}

// Update the checkboxes in case the state has been changed by the CLI or API (future)
void refreshMsgScreen() {
    size_t cnt = nN2kSendMessages;
    for (size_t i = 0; i < cnt; i++) {
        lv_obj_set_state(cb[i], LV_STATE_CHECKED, N2kSendMessages[i].Scheduler.IsEnabled());
        lv_obj_set_style_bg_color(cb[i], lv_palette_main(LV_PALETTE_ORANGE), LV_PART_INDICATOR | LV_STATE_FOCUSED);
    }
}


lv_obj_t* createCanScreen() {
    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_t * textArea;
    setupCommonstyles(screen);
    setupHeader(SCR_CAN, screen, "NMEA2000 CAN");

    group[SCR_CAN] = lv_group_create();
    lv_group_set_focus_cb(group[SCR_CAN], focus_cb);

    // Create a text area to display the info text
    // This takes up the right hand portion of the screen
    textArea = textAreas[SCR_CAN] = lv_textarea_create(screen);
    lv_obj_set_size(textArea, DISP_WIDTH / 2, BODY_HEIGHT);
    lv_obj_set_pos(textArea, DISP_WIDTH / 2, BAR_HEIGHT);
 //   lv_obj_align(textArea, LV_ALIGN_CENTER, DISP_WIDTH / 2, 0);
    lv_obj_set_style_text_font(textArea, &UbuntuMonoB16, LV_PART_MAIN | LV_STATE_DEFAULT);

    // The list of received messages and the selector to view them on the left
    lv_obj_t * body = secondArea[SCR_CAN] = lv_obj_create(screen);
    lv_obj_set_size(body, DISP_WIDTH / 2, BODY_HEIGHT);
    lv_obj_set_pos(body, 0, BAR_HEIGHT);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(body, padding, 0);
    lv_obj_set_style_margin_all(body, 0, 0);

    refreshCanScreen();
 
    lv_obj_update_flag(textArea, LV_OBJ_FLAG_SCROLLABLE, true);
    menus[SCR_CAN] = setupMenu(screen, SCR_CAN);
    return screen;
}

extern std::map<int, tN2kMsg> N2kLastMessage;
void canButtonHandler(lv_event_t *e) {
    std::map<int, tN2kMsg>::iterator iter;
    long int pgn = (long int)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        Serial.printf("PGN %ld\n", pgn);
        iter = N2kLastMessage.find(pgn);
        if (iter == N2kLastMessage.end()) {
            Serial.printf("No messages with PGN %ld found\n", pgn);
        } else {
            StringStream output;
            Serial.printf("Message with PGN %ld found\n", pgn);
            HandleNMEA2000Msg(iter->second, &output);
            updateTextArea(SCR_CAN, output.data);
            Serial.printf("%s\n", output.data.c_str());
        }
    }
}

// Update the list of messages seen
// Only do an update if the number of messages has changed since last time
// The messages are shown with the PGN and if clicked will show the decode
// of the last saved message in the right hand side of the screen
void refreshCanScreen() {
    static size_t lastCount = 0;
    size_t thisCount = N2kMsgMap.size();
    Screens scr = SCR_CAN;
    lv_obj_t *area = secondArea[scr];

    // Clear the existing list if list changed
    if (area && thisCount > lastCount) {
        lv_obj_clean(area);        
        std::map<int, int>::iterator it = N2kMsgMap.begin();
        std::map<long, const char *>::iterator it2;
        
        while (it != N2kMsgMap.end()) {
            it2 = pgnToStr.begin();
            lv_obj_t *pgnBox = lv_obj_create(area);
            lv_obj_t *button = lv_button_create(pgnBox);
            lv_obj_t *label = lv_label_create(button);
            lv_obj_t *name = lv_label_create(pgnBox);
            const char * nameStr = "";
            long pgn = it->first;

            lv_obj_set_style_bg_color(button, lv_palette_main(LV_PALETTE_ORANGE), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_obj_set_flex_flow(pgnBox, LV_FLEX_FLOW_ROW);
            lv_obj_clear_flag(pgnBox, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_pad_all(pgnBox, padding, 0);
            lv_obj_set_style_margin_all(pgnBox, 0, 0);
            lv_obj_set_width(pgnBox, lv_obj_get_width(area));
            lv_obj_set_height(pgnBox, BAR_HEIGHT);

            // hand rolled find() equivalent
            bool found = false;
            while (!found && it2 != pgnToStr.end()) {
                if(it2->first == pgn) {
                    nameStr = it2->second;
                    found = true;
                }
                it2++;
            }

            Serial.printf("Adding pgn %ld %s\n", pgn, nameStr);
            lv_label_set_text(label, String(pgn).c_str());
            lv_label_set_text(name, nameStr);
            lv_obj_center(label);
            lv_obj_add_event_cb(button, canButtonHandler, LV_EVENT_ALL, (void *)pgn);
            lv_obj_update_layout(area);

            // Add the button to this screens group
            lv_group_add_obj(group[SCR_CAN], button);
            it++;
        }
    }

    lastCount = thisCount;
}

void displayText(const char * txt) {
    Serial.printf("Displaying '%s'\n", txt);
}

// Refresh the info in the sysinfo page
void refreshSysinfo() {
    StringStream s;
    s.clear();
    getSysInfo(s);
    getNetInfo(s);
    lv_textarea_set_text(textAreas[SCR_SYSINFO], s.data.c_str());
}

// Load a given screen if defined
// Also sets the group to the screen and focusses on the first member of the
// group if those exist
void loadScreen(Screens s) {
    if (screen[s]) {
        lv_scr_load(screen[s]);
        curscreen = s;
        if (group[s]) {
            lv_indev_set_group(rotaryIndev, group[s]);
            if (firstFocus[s]) {
                lv_group_focus_obj(firstFocus[s]);
            }
        }
    }
}

void buttonHandler(lv_event_t* e) {
    void* target = lv_event_get_user_data(e);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
    Screens s = reinterpret_cast<Screens&>(target);
#pragma GCC diagnostic pop
    lv_event_code_t code = lv_event_get_code(e);
//    Serial.printf("Button handler scr %d code %d\n", s, code);
    if (code == LV_EVENT_PRESSED) {
        if (s >= 0 && s < SCR_MAX && screen[s]) {
            loadScreen(s);
        }
    }
}

void updateTextArea(Screens scr, String & val) {
    if(scr >= SCR_MAIN && scr < SCR_MAX) {
        lv_textarea_set_text(textAreas[scr], val.c_str());
        lv_obj_scroll_to(textAreas[scr], 0, 0, LV_ANIM_OFF);
    }
}