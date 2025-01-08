#pragma once

#include <TFT_eSPI.h>
#include <lvgl.h>

typedef enum {
    SCR_INVALID = -1,   // Invaluid screen
    SCR_MAIN = 0,       // Main window with controls and summary
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

// StateButton indexes
typedef enum {
    SBIDX_CAN = 0,
    SBIDX_YD
}
StateButtonIndex;
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


#define SB_HEIGHT       (IND_HEIGHT)
#define SB_WIDTH        (DISP_WIDTH / 4)

#define COL1            (0)
#define COL2            (SB_WIDTH)
#define COL3            (SB_WIDTH * 2)
#define COL4            (SB_HEIGHT * 3)


#define border 1
#define padding 0

extern TFT_eSPI * tft;

void displayInit();
void handleDisplay();
void displayText(const char * txt);
void buttonHandler(lv_event_t* e);
