#pragma once

#include <TFT_eSPI.h>
#include <lvgl.h>

typedef enum {
    SCR_MAIN,           // Main window with controls and summary
    SCR_MSG,            // message controls
    SCR_CAN,            // Can related info
    SCR_SYSINFO,        // system and network info
    SCR_MAX,
} Screens;

// screen menu buttons
typedef enum {
    BTN_GPS,
    BTN_SKY,
    BTN_INFO1,
    BTN_MAX
}BTN;

// define the diemsnions of the actual tft display
#undef TFT_WIDTH
#undef TFT_HEIGHT
#define TFT_WIDTH 320
#define TFT_HEIGHT 480

// define the dimensions of the rotated display for positioning elements
#define DISP_WIDTH  TFT_HEIGHT
#define DISP_HEIGHT TFT_WIDTH

// Define the positions of elements on the screen.
// The elements are laid out in a grid with a header and footer
#define BAR_HEIGHT      (DISP_HEIGHT / 8)
#define BAR_MENU_HEIGHT (DISP_HEIGHT / 6) 
#define BAR_WIDTH       (DISP_WIDTH)
#define BAR_ROW_TOP     (0)
#define BAR_ROW_BOTTOM  (DISP_HEIGHT - BAR_MENU_HEIGHT)
#define BODY_HEIGHT     (DISP_HEIGHT - BAR_HEIGHT - BAR_MENU_HEIGHT)
#define IND_HEIGHT      (BODY_HEIGHT / 3)
#define IND_WIDTH       (DISP_WIDTH / 2)
#define ROW1            (BAR_HEIGHT)
#define ROW2            (ROW1 + IND_HEIGHT)
#define ROW3            (ROW2 + IND_HEIGHT)
#define COL1            (0)
#define COL2            (DISP_WIDTH / 2)

// This class implements a rectangle container which has a main display for
// eg voltage, a smaller header. It is designed to work with the lvgl library
// on an ESP32 or similar.
// It has a fixed size.
class Indicator {
   public:
    // Constructor:
    Indicator(lv_obj_t *parent, const char *label, uint32_t x, uint32_t y);
    void setValue(const char *value);

    // Set the value of a meter using a double and set the precision
    void setValue(double value, const char* units, uint32_t prec);
    
    //Change the main indicator font
    void setFont(const lv_font_t *value);

    // private:
    lv_obj_t *container;
    lv_obj_t *label;
    lv_obj_t *text;

    lv_style_t text_style;
    lv_style_t value_style;
    lv_style_t style;

    // The interval used for moving averages
    static const int interval = 4;  
};

// Class to implement a full width info text area. 
class InfoBar {
    public:
    InfoBar(lv_obj_t * parent, uint32_t y);
    void setValue(const char * value);
    void setTime(const char * t);

    bool isActive;

    lv_obj_t * container;
    lv_obj_t * text;
    lv_obj_t * curTime;
};


class MenuBar {
    public:
    MenuBar(lv_obj_t * parent, uint32_t y);

    void addButton(const char * label, Screens target);
    lv_obj_t *  addActionButton(const char * label, void (*ptr)(lv_event_t *));
    lv_obj_t * container;
};

typedef struct Buttons {
    BTN btn;
    Screens target;
    const char * label;
}Buttons;

#define border 1
#define padding 0

extern TFT_eSPI * tft;

void displayInit();
void handleDisplay();
void displayText(const char * txt);
void buttonHandler(lv_event_t* e);
