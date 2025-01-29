#include <Arduino.h>
#include <display.h>
#include <Indicator.h>
#include <myFonts.h>
#include <GwPrefs.h>
#include <display/lv_display_private.h>



// Constructor. Binds to the parent object.
Indicator::Indicator(lv_obj_t *parent, const char *name, uint32_t x,
                     uint32_t y, void (*cb)(lv_event_t * ev), void* userData) {
    container = lv_obj_create(parent);
    lv_obj_set_pos(container, x, y);
    lv_obj_set_width(container, IND_WIDTH - (2 * padding));
    lv_obj_set_height(container, IND_HEIGHT - (2 * padding));

    lv_style_init(&style);
    lv_style_set_border_width(&style, border);
    lv_style_set_pad_all(&style, 0);
    lv_style_set_pad_left(&style, 10);
    lv_obj_add_style(container, &style, 0);

    lv_obj_set_layout(container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    label = lv_label_create(container);
    lv_label_set_text(label, name);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);

    lv_style_init(&text_style);
    lv_style_set_text_font(&text_style, &RobotoCondensedVariableFont_wght24);
    lv_style_set_pad_all(&text_style, 0);
    lv_obj_add_style(label, &text_style, 0);

    text = lv_label_create(container);
    lv_style_init(&value_style);
    lv_style_set_text_font(&value_style, &RobotoCondensedVariableFont_wght32);
    lv_style_set_pad_all(&value_style, 0);
    lv_obj_add_style(text, &value_style, 0);
    lv_obj_set_style_text_align(text, LV_TEXT_ALIGN_CENTER, 0);

    lv_label_set_text(text, "---");

    // If a callback was supplied add it to the main widget and set th efocus colour
    if(cb) {
        lv_obj_add_event_cb(container, cb, LV_EVENT_CLICKED, userData);
        lv_obj_set_style_bg_color(container, lv_palette_main(LV_PALETTE_ORANGE), LV_PART_MAIN | LV_STATE_FOCUSED);
    }
}

// Change the text size
void Indicator::setFont(const lv_font_t *value) {
    lv_style_set_text_font(&value_style, value);
}

// set the value using a double and precision.
void Indicator::setValue(double value, const char *units, uint32_t prec) {
    String v(value, prec);
    v += units;
    setValue(v.c_str());
}

void Indicator::setValue(const char *value) { lv_label_set_text(text, value); }

// Constructor. Binds to the parent object.
// Info bar has the screen title and the time
InfoBar::InfoBar(lv_obj_t *parent, uint32_t y) {
    static lv_style_t style;
    static lv_style_t value_style;

    container = lv_obj_create(parent);
    lv_obj_set_pos(container, 0, y);
    lv_obj_set_width(container, (BAR_WIDTH) - (2 * padding));
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

MenuBar::MenuBar(lv_obj_t *parent, uint32_t y) {
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

void InfoBar::setValue(const char *value) { lv_label_set_text(text, value); }

void InfoBar::setTime(const char *t) { lv_label_set_text(curTime, t); }

// Add a button to a menu bar. The callbackl will change the screen to the target
lv_obj_t * MenuBar::addButton(const char* label, Screens target) {
    lv_obj_t* button = lv_button_create(container);
    lv_obj_t* text = lv_label_create(button);

    lv_obj_set_height(button, BAR_MENU_HEIGHT);
    lv_label_set_text(text, label);
    lv_obj_set_flex_grow(button, 1);

    /*Init the style for the default state*/
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_radius(&style, 3);
    lv_style_set_bg_opa(&style, LV_OPA_100);
    lv_style_set_bg_color(&style, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_border_opa(&style, LV_OPA_40);
    lv_style_set_border_width(&style, 2);
    lv_style_set_border_color(&style, lv_palette_main(LV_PALETTE_GREY));
    lv_style_set_outline_opa(&style, LV_OPA_COVER);
    lv_style_set_outline_color(&style, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_text_color(&style, lv_color_white());
    lv_style_set_text_font(&style, &RobotoCondensedVariableFont_wght24);
    lv_obj_add_style(button, &style, 0);

    lv_obj_set_style_bg_color(button, lv_palette_main(LV_PALETTE_ORANGE), LV_PART_MAIN | LV_STATE_FOCUSED);

    lv_event_code_t code = LV_EVENT_PRESSED;
    //,             /**< The object has been pressed*/
    // LV_EVENT_PRESSING,            /**< The object is being pressed (called continuously while pressing)*/
    // LV_EVENT_PRESS_LOST,          /**< The object is still being pressed but slid cursor/finger off of the object */
    // LV_EVENT_SHORT_CLICKED,       /**< The object was pressed for a short period of time, then released it. Not called if scrolled.*/
    // LV_EVENT_LONG_PRESSED,        /**< Object has been pressed for at least `long_press_time`.  Not called if scrolled.*/
    // LV_EVENT_LONG_PRESSED_REPEAT, /**< Called after `long_press_time` in every `long_press_repeat_time` ms.  Not called if scrolled.*/
    // LV_EVENT_CLICKED,             /**< Called on release if not scrolled (regardless to long press)*/
    // LV_EVENT_RELEASED,
    lv_obj_add_event_cb(button, buttonHandler, code, (void*)target);

    buttons.push_back(button);

    return button;
}

// Add a button to a menu bar. The callback will change the screen to the target
// returns a pointer to the label object
lv_obj_t* MenuBar::addActionButton(const char* label, void (*ptr)(lv_event_t* e)) {
    lv_obj_t* b = lv_button_create(container);
    lv_obj_t* l = lv_label_create(b);
    lv_label_set_text(l, label);
    lv_obj_set_flex_grow(b, 1);
    /*Init the style for the default state*/
    static lv_style_t style;
    lv_style_init(&style);

    lv_style_set_radius(&style, 3);

    lv_style_set_bg_opa(&style, LV_OPA_100);
    lv_style_set_bg_color(&style, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_bg_grad_color(&style, lv_palette_darken(LV_PALETTE_BLUE, 2));
    lv_style_set_bg_grad_dir(&style, LV_GRAD_DIR_VER);

    lv_style_set_border_opa(&style, LV_OPA_40);
    lv_style_set_border_width(&style, 2);
    lv_style_set_border_color(&style, lv_palette_main(LV_PALETTE_GREY));

    lv_style_set_shadow_width(&style, 8);
    lv_style_set_shadow_color(&style, lv_palette_main(LV_PALETTE_GREY));
    lv_style_set_shadow_offset_y(&style, 8);

    lv_style_set_outline_opa(&style, LV_OPA_COVER);
    lv_style_set_outline_color(&style, lv_palette_main(LV_PALETTE_BLUE));

    lv_style_set_text_color(&style, lv_color_white());
    lv_style_set_pad_all(&style, 10);
    //    lv_obj_remove_style_all(b);
    lv_obj_add_style(b, &style, 0);
    lv_obj_add_event_cb(b, ptr, LV_EVENT_CLICKED, NULL);

    return l;
}

// Event callback for the StateButton
static void callback(lv_event_t * e) {
    StateButton * b = (StateButton * )lv_event_get_user_data(e);
    if(b->callBack) {
        b->callBack(0, NULL);
    }
    b->update();
}

StateButton::StateButton(lv_obj_t * parent, const char * label, uint32_t x, uint32_t y, const char * pref)  {
    prefname = pref;
    container = lv_obj_create(parent);
    lv_obj_set_pos(container, x, y);
    lv_obj_set_width(container, SB_WIDTH - (2 * padding));
    lv_obj_set_height(container, SB_HEIGHT - (2 * padding));

    lv_style_init(&style);
    lv_style_set_border_width(&style, border);
    lv_obj_add_style(container, &style, 0);

    lv_obj_set_layout(container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_outline_width(container, 4, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_color(container, lv_palette_main(LV_PALETTE_ORANGE), LV_PART_MAIN | LV_STATE_FOCUSED);

    value = lv_label_create(container);
    lv_style_init(&value_style);
    lv_style_set_text_font(&value_style, &RobotoCondensedVariableFont_wght32);
    lv_style_set_text_color(&value_style, lv_color_white());
    lv_obj_add_style(value, &value_style, 0);
    lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_CENTER, 0);

    lv_label_set_text(value, label);

    lv_style_set_bg_opa(&style, LV_OPA_100);
    lv_obj_add_style(container, &style, 0);
    lv_obj_add_event_cb(container, callback, LV_EVENT_PRESSED, this);
    update();
}

void StateButton::update() {
    val = GwGetInt(prefname);
    if(val) {
        lv_obj_set_style_bg_color(container, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
    } else {
        lv_obj_set_style_bg_color(container, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
    }
}

