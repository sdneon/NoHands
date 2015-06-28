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
 * > Random surprise picture pops up once every hour (at a random minute).
 *
 * Modified by @neon from simple-analog-master example.
 **/
#include "nohands.h"

#include "pebble.h"

//Debug flags:
//#define DEBUG_MODE 1
//#define DEBUG_COLOURS 1
//#define DEBUG_HOUR_HINT 1

//Specify this flag to invert the colour scheme:
#define INVERT_COLOURS

//Interval (in ms) to show surprise pic
#define PIC_SHOW_INTERVAL 1000

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

//List of Primary & Secondary colour pairs used to paint the watchface:
#define MAX_COLOURS 17
static uint8_t COLOURS[MAX_COLOURS][2] = {
    {GColorBulgarianRoseARGB8, GColorRoseValeARGB8},
    {GColorImperialPurpleARGB8, GColorJazzberryJamARGB8},
    {GColorPurpleARGB8, GColorShockingPinkARGB8},
    {GColorFashionMagentaARGB8, GColorRichBrilliantLavenderARGB8},
    {GColorMagentaARGB8, GColorShockingPinkARGB8},
    {GColorLibertyARGB8, GColorBabyBlueEyesARGB8},
    {GColorOxfordBlueARGB8, GColorIndigoARGB8},
    {GColorBlueARGB8, GColorPictonBlueARGB8},
    {GColorDukeBlueARGB8, GColorVividCeruleanARGB8},
    {GColorCobaltBlueARGB8, GColorCyanARGB8},
    {GColorDarkGreenARGB8, GColorKellyGreenARGB8},
    {GColorArmyGreenARGB8, GColorBrassARGB8},
    {GColorJaegerGreenARGB8, GColorMintGreenARGB8}, //pastel
    {GColorIslamicGreenARGB8, GColorInchwormARGB8},
    {GColorMidnightGreenARGB8, GColorMediumSpringGreenARGB8},
    {GColorOrangeARGB8, GColorChromeYellowARGB8},
    {GColorSunsetOrangeARGB8, GColorMelonARGB8} //pinkish
};

//List of surprise pics:
#define MAX_PICS 6
static const int PIC_ID[MAX_PICS] = {
    RESOURCE_ID_IMAGE_1,
    RESOURCE_ID_IMAGE_2,
    RESOURCE_ID_IMAGE_3,
    RESOURCE_ID_IMAGE_4,
    RESOURCE_ID_IMAGE_5,
    RESOURCE_ID_IMAGE_6
};

/**
 * The (empty) quadrants in which to place the date & surprise pic displays respectively.
 * Indexed by: [min hand's quadrant][hr hand's quadrant]
 */
static const int8_t DATE_QUAD_INDEX[4][4] = {
    { 2, 2, 1, 1 },
    { 2, 2, 0, 2 },
    { 1, 0, 1, 1 },
    { 1, 2, 1, 1 }
};
static const int8_t SURPRISE_QUAD_INDEX[4][4] = {
    { 3, 3, 3, 2 },
    { 3, 3, 3, 0 },
    { 3, 3, 0, 0 },
    { 2, 0, 0, 0 }
};
/**
 * Whether the quadrant is in the primary colour.
 * Diagonals (computation needed as both min & hr are in this same area):
 *   true if angleM <= angleH, false o.w.
 */
static const bool DATE_QUAD_USE_PRI[4][4] = {
    {  true,  true, false, false },
    { false,  true,  true, false },
    {  true, false,  true,  true },
    {  true,  true, false,  true }
};
static const bool SURPRISE_QUAD_USE_PRI[4][4] = {
    {  true,  true,  true, false },
    { false,  true,  true,  true },
    { false, false,  true,  true },
    {  true, false, false,  true }
};

/**
 * Frame position offsets for hour text layers to paint text with outline.
 * 1st 4 layers together constitute the outline.
 * Last layer is the main text body.
 **/
static const int OFFSET_OUTLINE[5][2] = {
    { -1, -1 },
    { 1, -1 },
    { 1, 1 },
    { -1, 1 },
    { 0, 0 }
};

static Window *window;
static Layer *s_simple_bg_layer, *s_date_layer, *s_spoke_layer;
static TextLayer *s_day_label, *s_hour_label[5];
static BitmapLayer *m_spbmLayer;
static GBitmap *m_spbmPics[MAX_PICS] = {0};
static AppTimer *m_sptimer1;

static char s_day_buffer[15], s_hour_buffer[4];

int hr = 0, min = 0;
int m_nColourIndex = 0;
int dateQuadrant = 0, //which quadrant to put date in: 0: NE, 1, SE, 2: SW, 3: NW
    surpQuadrant = 1; //which quadrant to put battery indicator in (always try to put it above date)
bool surpQuadrantUseApc = false,
    dateQuadrantUseApc = false;
int lastSurpriseHr = -1, nextSurpriseMin = -1;

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

void hidePic(void *a_pData)
{
    layer_set_frame((Layer*)m_spbmLayer, GRect(0,0,0,0));
}

void moveLayer(Layer *layer, Layer *refLayer, int quad)
{
    GRect bounds = layer_get_bounds(window_get_root_layer(window));
    int winHalfWidth = bounds.size.w / 2,
        winHalfHeight = bounds.size.h / 2;
    GRect frame = layer_get_frame(refLayer);
    int offsetX = (winHalfWidth - frame.size.w) / 2,
        offsetY = (winHalfHeight - frame.size.h) / 2;
    switch (quad)
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
    layer_set_frame(layer, frame);
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
#ifdef DEBUG_MODE
    int sec = t->tm_sec;
    min = sec;
    if (min == 59) --lastSurpriseHr; //force re-selection of surprise min after every round of min hand
#endif
    int32_t angleM = TRIG_MAX_ANGLE * min / 60;
    //int32_t angleH = TRIG_MAX_ANGLE * (hr % 12) / 12; //without minutes contribution
    int32_t angleH = (TRIG_MAX_ANGLE * (((hr % 12) * 6) + (min / 10))) / (12 * 6); //with minutes contribution
    int32_t angleMid;

#ifdef DEBUG_COLOURS
    m_nColourIndex = t->tm_sec * MAX_COLOURS / 60;
#else
    m_nColourIndex = (t->tm_yday * 24 + hr) % MAX_COLOURS;
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

    int mInQ = angleM / ANGLE_DEG_90,
        hInQ = angleH / ANGLE_DEG_90;
    dateQuadrant = DATE_QUAD_INDEX[mInQ][hInQ];
    dateQuadrantUseApc = (mInQ != hInQ)?
        DATE_QUAD_USE_PRI[mInQ][hInQ]: (angleM < angleH);
    surpQuadrant = SURPRISE_QUAD_INDEX[mInQ][hInQ];
    surpQuadrantUseApc = (mInQ != hInQ)?
        SURPRISE_QUAD_USE_PRI[mInQ][hInQ]: (angleM < angleH);

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

    //bIsSmallSectorLight = ! bIsSmallSectorLight; //try inverting colour scheme to more light (i.e. bigger light sector)
#ifdef INVERT_COLOURS
    GColor clrDark = (GColor8){.argb=COLOURS[m_nColourIndex][1]};
    GColor clrLight = (GColor8){.argb=COLOURS[m_nColourIndex][0]};
#else
    GColor clrDark = (GColor8){.argb=COLOURS[m_nColourIndex][0]};
    GColor clrLight = (GColor8){.argb=COLOURS[m_nColourIndex][1]};
#endif
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
    int i;
    for (i = 0; i < 5; ++i)
    {
        text_layer_set_text(s_hour_label[i], s_hour_buffer);
        GRect frame = layer_get_frame((Layer*)s_hour_label[i]);
        frame.origin.x = OFFSET_OUTLINE[i][0] + center.x - (frame.size.w / 2)
            + radius * sin_lookup(angle) / TRIG_MAX_RATIO;
        frame.origin.y = OFFSET_OUTLINE[i][1] + center.y - (frame.size.h / 2) - 5 //offset 5px up
            - radius * cos_lookup(angle) / TRIG_MAX_RATIO;
        layer_set_frame((Layer*)s_hour_label[i], frame);
        if (i < 4)
        {
            text_layer_set_text_color(s_hour_label[i], (hr >= 12)? GColorBlack: GColorWhite);
        }
        else
        {
            text_layer_set_text_color(s_hour_label[i], (hr >= 12)? GColorWhite: GColorBlack);
        }
    }

    bool bToShowPicFor1stTime = false;
    if (lastSurpriseHr < 0)
    {
        bToShowPicFor1stTime = true; //show pic when watchface is first loaded
    }
    if (bToShowPicFor1stTime || (hr != lastSurpriseHr))
    {
        lastSurpriseHr = hr;
        //randomly pick a min to show surprise pic:
        if (!bToShowPicFor1stTime)
        {
            nextSurpriseMin = rand() % 60;
        }
        else //showing pic for 1st time, so randomly pick a min after current min
        {
            nextSurpriseMin = (rand() % (59 - min)) + min;
        }
    }
    if (bToShowPicFor1stTime || (min == nextSurpriseMin))
    {
        bitmap_layer_set_bitmap(m_spbmLayer, m_spbmPics[rand() % MAX_PICS]);
        //move surprise pic to appropriate quadrant:
        moveLayer((Layer*)m_spbmLayer, layer, surpQuadrant);
        m_sptimer1 = app_timer_register(PIC_SHOW_INTERVAL, (AppTimerCallback) hidePic, NULL);
    }
}

//2. 2nd layer contains date & hour text
static void date_update_proc(Layer *layer, GContext *ctx)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    strftime(s_day_buffer, sizeof(s_day_buffer), "%a\n%d %b", t); //print e.g. Wed\n16 Jun
    text_layer_set_text(s_day_label, s_day_buffer);
    text_layer_set_text_color(s_day_label,
        dateQuadrantUseApc?
#ifdef INVERT_COLOURS
            GColorWhite: GColorBlack);
            //(GColor8){.argb=COLOURS[m_nColourIndex][1]}:
            //(GColor8){.argb=COLOURS[m_nColourIndex][0]});
#else
            GColorBlack: GColorWhite);
            //(GColor8){.argb=COLOURS[m_nColourIndex][0]}:
            //(GColor8){.argb=COLOURS[m_nColourIndex][1]});
#endif

    moveLayer((Layer*)s_day_label, (Layer*)s_day_label, dateQuadrant);
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
    c = m_bBtConnected? GColorWhite: GColorShockingPink;
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

    //1st 4 layers are for the text outline, last layer (topmost) is the text itself
    int i;
    for (i = 0; i < 5; ++i)
    {
        s_hour_label[i] = text_layer_create(GRect(OFFSET_OUTLINE[i][0], OFFSET_OUTLINE[i][1], 40, 32));
        text_layer_set_text(s_hour_label[i], s_hour_buffer);
        text_layer_set_background_color(s_hour_label[i], GColorClear);
        //text_layer_set_background_color(s_hour_label[i], GColorOrange); //DEBUG positioning of label
        text_layer_set_text_color(s_hour_label[i], GColorWhite);
        text_layer_set_font(s_hour_label[i], fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK)); //FONT_KEY_GOTHIC_28_BOLD));
        text_layer_set_text_alignment(s_hour_label[i], GTextAlignmentCenter);
        layer_add_child(s_date_layer, text_layer_get_layer(s_hour_label[i]));
    }

    //3. 3rd layer contains the bluetooth & battery indicator
    s_spoke_layer = layer_create(bounds);
    layer_set_update_proc(s_spoke_layer, spoke_update_proc);
    layer_add_child(window_layer, s_spoke_layer);

    //4. Surprise pic layer
    m_spbmLayer = bitmap_layer_create(bounds);
    for (i = 0; i < MAX_PICS; ++i)
    {
        m_spbmPics[i] = gbitmap_create_with_resource(PIC_ID[i]);
    }
    bitmap_layer_set_background_color(m_spbmLayer, GColorClear);
    bitmap_layer_set_compositing_mode(m_spbmLayer, GCompOpSet);
    bitmap_layer_set_bitmap(m_spbmLayer, m_spbmPics[0]);
    layer_add_child(window_layer, bitmap_layer_get_layer(m_spbmLayer));
}

static void window_unload(Window *window) {
    layer_destroy(s_simple_bg_layer);
    layer_destroy(s_date_layer);
    layer_destroy(s_spoke_layer);

    text_layer_destroy(s_day_label);
    int i;
    for (i = 0; i < 5; ++i)
    {
        text_layer_destroy(s_hour_label[i]);
    }
    for (i = 0; i < MAX_PICS; ++i)
    {
        if (m_spbmPics[i])
        {
            gbitmap_destroy(m_spbmPics[i]);
        }
    }
    bitmap_layer_destroy(m_spbmLayer);
    app_timer_cancel(m_sptimer1);
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
    srand(time(NULL));
    init();
    app_event_loop();
    deinit();
}
