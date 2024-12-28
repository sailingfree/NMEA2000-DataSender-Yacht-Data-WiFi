#include <Arduino.h>
#include <display.h>
#include <myFonts.h>

// Constructor. Binds to the parent object.
Indicator::Indicator(lv_obj_t *parent, const char *name, uint32_t x,
                     uint32_t y) {
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
