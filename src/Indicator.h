#include <lvgl.h>
#include <display.h>
#include <vector>

// This class implements a rectangle container which has a main display for
// eg voltage, a smaller header. It is designed to work with the lvgl library
// on an ESP32 or similar.
// It has a fixed size.
// An optional callback and callback arg can be supplied
class Indicator {
   public:
    // Constructor:
    Indicator(lv_obj_t *parent, const char *label, uint32_t x, uint32_t y, void (*cb)(lv_event_t *) = NULL, void* =NULL );

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

    void* cbArg;
    void (*callback)();

    // The interval used for moving averages
    static const int interval = 4;  
};

// Class for button
// and which uses a preference varibale for its state
class StateButton {
    public:
        int val;
        const char * prefname;
        int (*callBack)(int, char **);
        StateButton(lv_obj_t * parent, const char * label, uint32_t x, uint32_t y, const char * pref);
        void update();
        lv_obj_t * getContainer() {return container;}

        private:
           // private:
    lv_obj_t *container;
    lv_obj_t *value;
    lv_style_t value_style;
    lv_style_t style;
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

    lv_obj_t * addButton(const char * label, Screens target);
    lv_obj_t * addActionButton(const char * label, void (*ptr)(lv_event_t *));
    lv_obj_t * container;

    std::vector<lv_obj_t *>buttons;
};

typedef struct Buttons {
    BTN btn;
    Screens target;
    const char * label;
}Buttons;
