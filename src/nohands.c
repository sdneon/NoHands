/**
 * "No Hands" - ChromaTick-like watch face.
 * Analogue watch with coloured sectors and hour hint.
 * Info displayed:
 * > hour hint colour: AM= white, PM= black. (maybe draw a background circle if can centre digit in it nicely)
 * > day of week, date (less year) display.
 * > middle spoke indicator for bluetooth connectivity & battery level.
 *   > bluetooth indicated by colour of centre: white= connected, light gray= disconnected.
 *   > battery level:
 *     > red= charging, black= draining.
 *     > spoke rim thickness of 1 to 5 for 5 battery levels of <20% to 100%.
 *
 * Modified by @neon from simple-analog-master example.
 **/
#include "nohands.h"

#include "pebble.h"

//#define DEBUG_MODE 1
//#define DEBUG_COLOURS 1
//#define DEBUG_HOUR_HINT 1

#define MAX_COLOURS 16
//#define MAX_COLOURS 5
static const int32_t ANGLE_HOUR_HINT = TRIG_MAX_ANGLE * 5 / 360; //overlaps hour line
//static const int32_t ANGLE_HOUR_HINT = TRIG_MAX_ANGLE * 15 / 360; //avoids overlapping hour line
static const int32_t ANGLE_DEG_45 = TRIG_MAX_ANGLE * 45 / 360;
static const int32_t ANGLE_DEG_90 = TRIG_MAX_ANGLE / 4;
static const int32_t ANGLE_DEG_135 = TRIG_MAX_ANGLE * 135 / 360;
static const int32_t ANGLE_DEG_180 = TRIG_MAX_ANGLE / 2;
static const int32_t ANGLE_DEG_225 = TRIG_MAX_ANGLE * 225 / 360;
static const int32_t ANGLE_DEG_270 = TRIG_MAX_ANGLE * 3 / 4;
static const int32_t ANGLE_DEG_315 = TRIG_MAX_ANGLE * 315 / 360;
static const int32_t ANGLE_DEG_360 = TRIG_MAX_ANGLE;
static uint8_t COLOURS[MAX_COLOURS][2] = {
    {GColorBulgarianRoseARGB8, GColorRoseValeARGB8},
    {GColorImperialPurpleARGB8, GColorJazzberryJamARGB8},
    {GColorPurpleARGB8, GColorPurpureusARGB8},
    {GColorFashionMagentaARGB8, GColorRichBrilliantLavenderARGB8},
    {GColorMagentaARGB8, GColorShockingPinkARGB8},
    {GColorVividVioletARGB8, GColorLavenderIndigoARGB8}, //too similar
    {GColorElectricUltramarineARGB8, GColorVeryLightBlueARGB8},
    {GColorLibertyARGB8, GColorBabyBlueEyesARGB8},
    {GColorOxfordBlueARGB8, GColorIndigoARGB8},
    {GColorBlueARGB8, GColorPictonBlueARGB8},
    {GColorDukeBlueARGB8, GColorVividCeruleanARGB8}, //both similarly dark
    {GColorCobaltBlueARGB8, GColorCyanARGB8},
    {GColorDarkGreenARGB8, GColorKellyGreenARGB8},
    {GColorArmyGreenARGB8, GColorBrassARGB8},
    //{GColorGreenARGB8, GColorMintGreenARGB8}, //way too light
    //{GColorIslamicGreenARGB8, GColorInchwormARGB8}, //too light
    {GColorOrangeARGB8, GColorRajahARGB8},
    {GColorSunsetOrangeARGB8, GColorMelonARGB8} //pinkish
};

static Window *window;
static Layer *s_simple_bg_layer, *s_date_layer, *s_spoke_layer;
static TextLayer *s_day_label, *s_hour_label;

static char s_day_buffer[15], s_hour_buffer[4];

int hr = 0, min = 0;
int m_nColourIndex = 0;
int dateQuadrant = 0, //which quadrant to put date in: 0: NE, 1, SE, 2: SW, 3: NW
    battQuadrant = 1; //which quadrant to put battery indicator in (always try to put it above date)

//
//Bluetooth stuff
//
bool m_bBtConnected = false;
static void bt_handler(bool connected) {
    m_bBtConnected = connected;
    if (s_spoke_layer) layer_mark_dirty(s_spoke_layer);
}

//
// Battery stuff
//
BatteryChargeState m_sBattState = {
    0,      //charge_percent
    false,  //is_charging
    false   //is_plugged
};
static void battery_handler(BatteryChargeState new_state) {
    m_sBattState = new_state;
    if (s_spoke_layer) layer_mark_dirty(s_spoke_layer);
}

//1. Base layer contains coloured analogue watch face
static void bg_update_proc(Layer *layer, GContext *ctx)
{
    GRect bounds = layer_get_bounds(layer);
    int widthHalf = bounds.size.w / 2,
        heightHalf = bounds.size.h / 2;
    int32_t width = bounds.size.w * 3;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    hr = t->tm_hour;
    min = t->tm_min;
    //hr = 7; min = 20; //DEBUG
#ifndef DEBUG_MODE
    int32_t angleM = TRIG_MAX_ANGLE * min / 60;
#else
    int sec = t->tm_sec;
    int32_t angleM = TRIG_MAX_ANGLE * sec / 60; //DEBUG
#endif
    //int32_t angleH = TRIG_MAX_ANGLE * (hr % 12) / 12; //without minutes contribution
    int32_t angleH = (TRIG_MAX_ANGLE * (((hr % 12) * 6) + (min / 10))) / (12 * 6); //with minutes contribution
    int32_t angleMid;

#ifdef DEBUG_COLOURS
    m_nColourIndex = t->tm_sec * MAX_COLOURS / 60;
#else
    m_nColourIndex = hr * MAX_COLOURS / 24;
#endif
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "h:m %d:%d => angles: %d, %d", hr, min, (int) (angleM * 360 / TRIG_MAX_ANGLE), (int) (angleH * 360 / TRIG_MAX_ANGLE));
    bool bIsSmallSectorLight = true;

    if (angleM >= angleH)
    {
        if ((angleM - angleH) <= ANGLE_DEG_180)
        {
            angleMid = (angleM + angleH) / 2;
        }
        else
        {
            bIsSmallSectorLight = false;
            angleMid = (TRIG_MAX_ANGLE - angleM + angleH) / 2 + angleM;
        }
    }
    else //angleH > angleM
    {
        if ((angleH - angleM) <= ANGLE_DEG_180)
        {
            angleMid = (angleM + angleH) / 2;
            bIsSmallSectorLight = false;
        }
        else
        {
            angleMid = (TRIG_MAX_ANGLE - angleH + angleM) / 2 + angleH;
        }
    }

    //Find 1st empty quadrant for text
    dateQuadrant = 0;
    if (angleM <= ANGLE_DEG_90)
    {
        if (angleH <= ANGLE_DEG_180)
        {
            //can use any of quadrants (2), (3), so use these:
            battQuadrant = 3;
            dateQuadrant = 2;
        }
        else if (angleH <= ANGLE_DEG_270)
        {
            battQuadrant = 3;
            dateQuadrant = 1;
        }
        else
        {
            battQuadrant = 2;
            dateQuadrant = 1;
        }
    }
    else if (angleM <= ANGLE_DEG_180)
    {
        if (angleH <= ANGLE_DEG_180)
        {
            battQuadrant = 3;
            dateQuadrant = 2;
        }
        else if (angleH <= ANGLE_DEG_270)
        {
            battQuadrant = 3;
            dateQuadrant = 0;
        }
        else
        {
            battQuadrant = 0;
            dateQuadrant = 2;
        }
    }
    else if (angleM <= ANGLE_DEG_270)
    {
        if (angleH <= ANGLE_DEG_90)
        {
            battQuadrant = 3;
            dateQuadrant = 1;
        }
        else if (angleH <= ANGLE_DEG_180)
        {
            battQuadrant = 3;
            dateQuadrant = 0;
        }
        else
        {
            battQuadrant = 0;
            dateQuadrant = 1;
        }
    }
    else
    {
        if (angleH <= ANGLE_DEG_90)
        {
            battQuadrant = 2;
            dateQuadrant = 1;
        }
        else if (angleH <= ANGLE_DEG_180)
        {
            battQuadrant = 0;
            dateQuadrant = 2;
        }
        else
        {
            battQuadrant = 0;
            dateQuadrant = 1;
        }
    }

    //create rectangle (for simplicity instead of sector; triangle is insufficient as at wide angles, it won't cover the corner!)
    GPathInfo sectorInfo = {
        .num_points = 4,
        .points = (GPoint []) {
            {0, 0},
            {width * sin_lookup(angleH) / TRIG_MAX_RATIO, -width * cos_lookup(angleH) / TRIG_MAX_RATIO},
            {width * sin_lookup(angleMid) / TRIG_MAX_RATIO, -width * cos_lookup(angleMid) / TRIG_MAX_RATIO},
            {width * sin_lookup(angleM) / TRIG_MAX_RATIO, -width * cos_lookup(angleM) / TRIG_MAX_RATIO}
        } };
    GPath *sector = gpath_create(&sectorInfo);
    GPoint center = grect_center_point(&bounds);
    gpath_move_to(sector, center);

    bIsSmallSectorLight = ! bIsSmallSectorLight; //try inverting colour scheme to more light (i.e. bigger light sector)
    GColor clrDark = (GColor8){.argb=COLOURS[m_nColourIndex][0]};
    GColor clrLight = (GColor8){.argb=COLOURS[m_nColourIndex][1]};
    //draw background (rest, i.e. big sector)
    graphics_context_set_fill_color(ctx, !bIsSmallSectorLight? clrLight: clrDark);
    graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);

    //draw small sector
    GColor c = bIsSmallSectorLight? clrLight: clrDark;
    graphics_context_set_fill_color(ctx, c);
    graphics_context_set_stroke_color(ctx, c);
    gpath_draw_filled(ctx, sector);
    gpath_draw_outline(ctx, sector);
    gpath_destroy(sector);

    //write 'hour' hint
    strftime(s_hour_buffer, sizeof(s_hour_buffer), "%l", t);
    //strftime(s_hour_buffer, sizeof(s_hour_buffer), "12", t); //DEBUG: Try widest hour displayed: 12
    text_layer_set_text(s_hour_label, s_hour_buffer);
    GRect frame = layer_get_frame((Layer*)s_hour_label);
    /**
     * In FONT_KEY_BITHAM_30_BLACK, digit '6' is ~18x21 px.
     * Thus, need vertical offset (of 5 px) as top of font character is 9px down.
     **/
#ifdef DEBUG_HOUR_HINT
    int32_t angle = angleM + ANGLE_HOUR_HINT;
#else
    int32_t angle = angleH + ANGLE_HOUR_HINT;
#endif
    int radius = heightHalf - 18; //less font size & margin
    if (((angle >= ANGLE_DEG_45) && (angle <= ANGLE_DEG_135))
        || ((angle >= ANGLE_DEG_225) && (angle <= ANGLE_DEG_315)))
    {
        radius = widthHalf - 18;
    }
    frame.origin.x = center.x - (frame.size.w / 2)
        + radius * sin_lookup(angle) / TRIG_MAX_RATIO;
    frame.origin.y = center.y - (frame.size.h / 2) - 5 //offset 5px up
        - radius * cos_lookup(angle) / TRIG_MAX_RATIO;
    layer_set_frame((Layer*)s_hour_label, frame);
    text_layer_set_text_color(s_hour_label, (hr >= 12)? GColorWhite: GColorBlack);
}

//2. 2nd layer contains date & hour text
static void date_update_proc(Layer *layer, GContext *ctx)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    strftime(s_day_buffer, sizeof(s_day_buffer), "%a\n%d %b", t); //print e.g. Wed\n16 Jun
    text_layer_set_text(s_day_label, s_day_buffer);

    GRect bounds = layer_get_bounds(window_get_root_layer(window));
    int winHalfWidth = bounds.size.w / 2,
        winHalfHeight = bounds.size.h / 2;
    GRect frame = layer_get_frame((Layer*)s_day_label);
    int offsetX = (winHalfWidth - frame.size.w) / 2,
        offsetY = (winHalfHeight - frame.size.h) / 2;
    switch (dateQuadrant)
    {
        case 0: //NE
            frame.origin.x = winHalfWidth + offsetX;
            frame.origin.y = offsetY;
            break;
        case 1: //SE
            frame.origin.x = winHalfWidth + offsetX;
            frame.origin.y = winHalfHeight + offsetY;
            break;
        case 2: //SW
            frame.origin.x = offsetX;
            frame.origin.y = winHalfHeight + offsetY;
            break;
        default: //NW
            frame.origin.x = offsetX;
            frame.origin.y = offsetY;
            break;
    }
    layer_set_frame((Layer*)s_day_label, frame);
}

//3. 3rd layer contains the bluetooth & battery indicator
static void spoke_update_proc(Layer *layer, GContext *ctx)
{
    GRect bounds = layer_get_bounds(window_get_root_layer(window));
    GPoint center = grect_center_point(&bounds);
    //m_sBattState.charge_percent = 10; //DEBUG
    int px = 1 + m_sBattState.charge_percent / 20;

    // dot in the middle of watch face
    GColor c = m_sBattState.is_charging? GColorRed: GColorBlack;
    graphics_context_set_stroke_color(ctx, c);
    graphics_context_set_fill_color(ctx, c);
    graphics_fill_circle(ctx, center, 3 + px);
    c = m_bBtConnected? GColorWhite: GColorLightGray;
    graphics_context_set_stroke_color(ctx, c);
    graphics_context_set_fill_color(ctx, c);
    graphics_fill_circle(ctx, center, 3);
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(window));
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    //for GOTHIC_18_BOLD:
    //int textWidth = bounds.size.w / 3 - 4; //slightly less (for margin) than half screen width
    //for GOTHIC_24_BOLD:
    int textWidth = bounds.size.w / 2 - 4; //slightly less (for margin) than half screen width

    //1. Base layer contains coloured analogue watch face
    s_simple_bg_layer = layer_create(bounds);
    layer_set_update_proc(s_simple_bg_layer, bg_update_proc);
    layer_add_child(window_layer, s_simple_bg_layer);

    //2. 2nd layer contains date & hour text
    s_date_layer = layer_create(bounds);
    layer_set_update_proc(s_date_layer, date_update_proc);
    layer_add_child(window_layer, s_date_layer);

    //s_day_label = text_layer_create(GRect(46, 114, textWidth, 40));
    s_day_label = text_layer_create(GRect(46, 114, textWidth, 60));
    text_layer_set_text(s_day_label, s_day_buffer);
    text_layer_set_background_color(s_day_label, GColorClear);
    text_layer_set_text_color(s_day_label, GColorWhite);
    text_layer_set_font(s_day_label, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text_alignment(s_day_label, GTextAlignmentCenter);
    layer_add_child(s_date_layer, text_layer_get_layer(s_day_label));

    s_hour_label = text_layer_create(GRect(0, 0, 40, 32));
    text_layer_set_text(s_hour_label, s_hour_buffer);
    text_layer_set_background_color(s_hour_label, GColorClear);
    //text_layer_set_background_color(s_hour_label, GColorOrange); //DEBUG
    text_layer_set_text_color(s_hour_label, GColorWhite);
    text_layer_set_font(s_hour_label, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK)); //FONT_KEY_GOTHIC_28_BOLD));
    text_layer_set_text_alignment(s_hour_label, GTextAlignmentCenter);
    layer_add_child(s_date_layer, text_layer_get_layer(s_hour_label));

    //3. 3rd layer contains the bluetooth & battery indicator
    s_spoke_layer = layer_create(bounds);
    layer_set_update_proc(s_spoke_layer, spoke_update_proc);
    layer_add_child(window_layer, s_spoke_layer);
}

static void window_unload(Window *window) {
    layer_destroy(s_simple_bg_layer);
    layer_destroy(s_date_layer);
    layer_destroy(s_spoke_layer);

    text_layer_destroy(s_day_label);
    text_layer_destroy(s_hour_label);
}

//
//Window setup sutff
//
static void init() {
    window = window_create();
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
    window_stack_push(window, true);

    s_day_buffer[0] = '\0';
    s_hour_buffer[0] = '\0';

#ifdef DEBUG_MODE
    tick_timer_service_subscribe(SECOND_UNIT, &handle_minute_tick);
#else
    tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick);
#endif

    // Subscribe to Bluetooth updates
    bluetooth_connection_service_subscribe(bt_handler);
    // Show current connection state
    bt_handler(bluetooth_connection_service_peek());

    // Subscribe to the Battery State Service
    battery_state_service_subscribe(battery_handler);
    // Get the current battery level
    battery_handler(battery_state_service_peek());
}

static void deinit() {
    tick_timer_service_unsubscribe();
    window_destroy(window);
}

int main() {
    init();
    app_event_loop();
    deinit();
}

