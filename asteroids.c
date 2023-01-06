#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>

//fps counter by p4nic4ttack
#define FRAME_TIME 66.666666
double delta = 1;
uint32_t lastFrameTime = 0;
void fps() {
  while (furi_get_tick() - lastFrameTime < FRAME_TIME);
  delta = (double)(furi_get_tick() - lastFrameTime) / (double)FRAME_TIME;
  lastFrameTime = furi_get_tick();
}
double getActualFps() {
  return 1000 / ((double)FRAME_TIME * (double)delta);
}

float pi = 3.1415926535;
float shipScale = 0.125;
bool thrusting = false;

typedef enum {
    EventTypeTick,
    EventTypeKey,
} EventType;

typedef struct {
    EventType type;
    InputEvent input;
} PluginEvent;

struct fVec2{
    float X;
    float Y;
};

struct bullet{
    int index;
    bool active;
    struct fVec2 position;
    struct fVec2 heading;
};

struct asteroid{
    struct fVec2 position;
    struct fVec2 heading;
    float angle;
    float rVelocity;
    float velocity;
    float points[5];
    float hitRadius;
};

struct asteroidSet{
    int stage; //0: inactive, 1: whole, 2: split
    struct asteroid primary; //used during stage 1 & 2
    struct asteroid secondary; //used during stage 2 only
};

struct ship{
    struct bullet bullets[16]; //only 16 bullets can exist on screen at once. you shouldnt need more than that.
    struct fVec2 position;
    struct fVec2 heading;
    struct fVec2 velocity; //velocity is independant to heading
    float angle;
    float rotationalVelocity;
};

struct ship gShip;
struct asteroidSet asteroids[8];

struct fVec2 getPoint(struct fVec2 start, float length, float angle){
    struct fVec2 nPos = { start.X + ((float)cos(angle) * length), start.Y + ((float)sin(angle) * length) };
    return nPos;
}

void fireBullet(){
    for(int i = 0; i < 16; i++){
        if(gShip.bullets[i].active == false){
            gShip.bullets[i].active = true;
            gShip.bullets[i].position = getPoint(gShip.position, 4, gShip.angle + 4.712f);
            gShip.bullets[i].heading.X = gShip.heading.X;
            gShip.bullets[i].heading.Y = gShip.heading.Y;
            return;
        }
    }
}

void resetShip(){
    gShip.position.X = 128.f / 2.f;
    gShip.position.Y = 64.f / 2.f;
    gShip.heading.X = 0.f;
    gShip.heading.Y = 0.f;
    gShip.velocity.X = 0.f;
    gShip.velocity.Y = 0.f;
    gShip.angle = 0.f;
    gShip.rotationalVelocity = 0.f;
}

float r2dp(float var)
{
	float value = (int)(var * 100 + .5);
	return (float)value / 100;
}

void recalculate(){
    if(gShip.rotationalVelocity != 0.f){
        gShip.angle += gShip.rotationalVelocity;
        if (r2dp(gShip.rotationalVelocity) < 0.f) { gShip.rotationalVelocity += 0.05f; }
        else if (r2dp(gShip.rotationalVelocity) > 0.f) { gShip.rotationalVelocity -= 0.05f; }
        else { gShip.rotationalVelocity = 0.f; }

        if (gShip.angle > (pi * 2.0)) { gShip.angle = 0; }
        else if (gShip.angle < 0) { gShip.angle = (pi * 2); }
        gShip.heading.X = cos(gShip.angle + (pi * 1.5));
        gShip.heading.Y = sin(gShip.angle + (pi * 1.5));
    }

    struct fVec2 nextPos;
    nextPos.X = gShip.position.X + gShip.velocity.X;
    nextPos.Y = gShip.position.Y + gShip.velocity.Y;

    gShip.position.X = nextPos.X;
    gShip.position.Y = nextPos.Y;

    gShip.velocity.X -= (gShip.velocity.X / 25.f);
    gShip.velocity.Y -= (gShip.velocity.Y / 25.f);

    if (gShip.position.X > 128) { gShip.position.X -= 128; }
    else if (gShip.position.X < 0) { gShip.position.X += 128; }

    if (gShip.position.Y > 64) { gShip.position.Y -= 64; }
    else if (gShip.position.Y < 0) { gShip.position.Y += 64; }

    for(int i = 0; i < 16; i++){
        if(gShip.bullets[i].active){
            gShip.bullets[i].position.X += gShip.bullets[i].heading.X * 5.f;
            gShip.bullets[i].position.Y += gShip.bullets[i].heading.Y * 5.f;

            if(gShip.bullets[i].position.X > 128 || gShip.bullets[i].position.X < 0){
                gShip.bullets[i].active = false;
            }
            else if(gShip.bullets[i].position.Y > 128 || gShip.bullets[i].position.Y < 0){
                gShip.bullets[i].active = false;
            }
        }
    }
}

static void draw_callback(Canvas* canvas, void* ctx) {
    UNUSED(ctx);
    canvas_clear(canvas);
    canvas_draw_frame(canvas, 0, 0, 128, 64);

	char buf[4];
	itoa(getActualFps(), buf, 10);
    canvas_draw_str(canvas, 5, 10, buf);

    struct fVec2 shipPosFront = getPoint(gShip.position, 3.75f, gShip.angle + 4.712f);
    struct fVec2 shipPosLeft = getPoint(gShip.position, 3.75f, gShip.angle + 2.199f);
    struct fVec2 shipPosRight = getPoint(gShip.position, 3.75f, gShip.angle + 0.942f);

    canvas_draw_line(canvas, shipPosFront.X, shipPosFront.Y, shipPosLeft.X, shipPosLeft.Y);
    canvas_draw_line(canvas, shipPosLeft.X, shipPosLeft.Y, gShip.position.X, gShip.position.Y);
    canvas_draw_line(canvas, gShip.position.X, gShip.position.Y, shipPosRight.X, shipPosRight.Y);
    canvas_draw_line(canvas, shipPosRight.X, shipPosRight.Y, shipPosFront.X, shipPosFront.Y);

    if(thrusting){
        struct fVec2 shipPosBack = getPoint(gShip.position, 3.75f, gShip.angle + 1.570f);
        canvas_draw_circle(canvas, shipPosBack.X, shipPosBack.Y, 1);
    }

    for(int i = 0; i < 16; i++){
        if(gShip.bullets[i].active){
            canvas_draw_circle(canvas, gShip.bullets[i].position.X, gShip.bullets[i].position.Y, 1);
        }
    }
}

static void input_callback(InputEvent* input_event, void* ctx) {
    furi_assert(ctx);
    FuriMessageQueue* event_queue = ctx;
    furi_message_queue_put(event_queue, input_event, FuriWaitForever);
}

static void timer_callback(FuriMessageQueue* event_queue) {
    UNUSED(event_queue);
    fps();
    recalculate();
}

int32_t asteroids_app(void* p) { 
    UNUSED(p);

    InputEvent event;
    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));

    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, draw_callback, NULL);
    view_port_input_callback_set(view_port, input_callback, event_queue);

    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);
    void* speaker = (void*)furi_hal_speaker_acquire(1000);

    FuriTimer* timer = furi_timer_alloc(timer_callback, FuriTimerTypePeriodic, event_queue);
    furi_timer_start(timer, furi_kernel_get_tick_frequency()/ 12);

    resetShip();

    while(true) {
        furi_check(furi_message_queue_get(event_queue, &event, FuriWaitForever) == FuriStatusOk);

        if(event.type == InputTypePress && event.key == InputKeyOk) {
            fireBullet(); //pew pew pew
        }
        if(event.type == InputTypePress && event.key == InputKeyBack) {
            break; //exit game
        }

        if(event.key == InputKeyUp) {
            if(event.type == InputTypePress) {
                thrusting = true;
            }
            if(event.type == InputTypeRelease) {
                thrusting = false;
            }
            
            //increase heading vector
            gShip.velocity.X += gShip.heading.X * 0.5f;
            gShip.velocity.Y += gShip.heading.Y * 0.5f;
        }
        if(event.key == InputKeyRight) {
            //angle right
            if (gShip.rotationalVelocity < 0.50f) {
                gShip.rotationalVelocity += 0.15f;
            }
        }
        if(event.key == InputKeyLeft) {
            //angle left
            if (gShip.rotationalVelocity > -0.50f) {
                gShip.rotationalVelocity -= 0.15f;
            }
        }
    }

    furi_timer_free(timer);
    furi_hal_speaker_release(speaker);
    furi_message_queue_free(event_queue);
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_record_close(RECORD_GUI);

    return 0;
}