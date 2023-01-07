#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>

#define WND_WIDTH 128
#define WND_HEIGHT 64
#define SOFFSET 10.f

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

float randFloat(float min, float max) {
    return min + (((float) rand()) / (float) RAND_MAX) * (max - min);
}

float pi = 3.1415926535;
float shipScale = 0.125;
bool thrusting = false;

typedef struct{
    float X;
    float Y;
}fVec2;

typedef struct{
    fVec2 position;
    float angle;
}startPos;

startPos spawnPositions[] = {
	{{-SOFFSET             ,  -SOFFSET            }, 0.25f * 3.1415926535}, //TL
	{{WND_WIDTH / 2.f      ,  -SOFFSET            }, 0.50f * 3.1415926535}, //TM
	{{WND_WIDTH + 10.f     ,  -SOFFSET            }, 0.75f * 3.1415926535}, //TR
	{{-SOFFSET             ,  WND_HEIGHT / 2.f    }, 0.00f * 3.1415926535}, //ML
	{{WND_WIDTH + SOFFSET  ,  WND_HEIGHT / 2.f    }, 1.00f * 3.1415926535}, //MR
	{{-SOFFSET             ,  WND_HEIGHT + SOFFSET}, 1.75f * 3.1415926535}, //BL
	{{WND_WIDTH / 2.f      ,  WND_HEIGHT + SOFFSET}, 0.50f * 3.1415926535}, //BM
	{{WND_WIDTH + SOFFSET  ,  WND_HEIGHT + SOFFSET}, 1.25f * 3.1415926535}, //BR
};

typedef struct{
    int index;
    bool active;
    fVec2 position;
    fVec2 heading;
}bullet;

typedef struct{
    fVec2 position;
    fVec2 heading;
    fVec2 points[5]; //X: length, Y: angle
    float angle;
    float rVelocity;
    float velocity;
    float hitRadius;
}asteroid;

typedef struct{
    int stage; //0: inactive, 1: whole, 2: split
    asteroid primary; //used during stage 1 & 2
    asteroid secondary; //used during stage 2 only
}asteroidSet;

typedef struct{
    bullet bullets[16]; //only 16 bullets can exist on screen at once. you shouldnt need more than that.
    fVec2 position;
    fVec2 heading;
    fVec2 velocity; //velocity is independant to heading
    float angle;
    float rotationalVelocity;
}ship;

ship gShip;
asteroidSet asteroids[8];

fVec2 getPoint(fVec2 start, float length, float angle){
    fVec2 nPos = { start.X + ((float)cos(angle) * length), start.Y + ((float)sin(angle) * length) };
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

void buildAsteroid(asteroid* asteroid, bool small, fVec2 position, float angle){
    asteroid->position.X = position.X;
    asteroid->position.Y = position.Y;
    asteroid->angle = angle;
    asteroid->rVelocity = randFloat(0.1f, 0.3f);
    asteroid->velocity = randFloat(0.5f, 1.f);
    asteroid->hitRadius = 0.f;

    float dX = cos(asteroid->angle);
    float dY = sin(asteroid->angle);

    float v2cLen = sqrt(dX * dX + dY * dY);
    asteroid->heading.X = (dX / v2cLen) * asteroid->velocity;
    asteroid->heading.Y = (dY / v2cLen) * asteroid->velocity;

    for(int i = 0; i < 5; i++){
        if(small){ asteroid->points[i].X = randFloat(2.f, 4.f); }
        else{ asteroid->points[i].X = randFloat(5.f, 7.f); }
        if(asteroid->points[i].X > asteroid->hitRadius){ asteroid->hitRadius = asteroid->points[i].X; }
        asteroid->points[i].Y = 1.25663f * (float)i;
    }
}

void spawnNextAsteroid(){
    for(int i = 0; i < 5; i++){
        if(asteroids[i].stage == 0){
            int sp = (rand() % 8);
            float offset = randFloat(-(0.25 * pi), (0.25 * pi));
            buildAsteroid(&asteroids[i].primary, (rand() % 2 == 1), spawnPositions[sp].position, spawnPositions[sp].angle + offset);
            asteroids[i].stage = 1;
            break;
        }
    }
}

void resetShip(){
    gShip.position.X = WND_WIDTH / 2;
    gShip.position.Y = WND_HEIGHT / 2;
    gShip.velocity.X = 0.f;
    gShip.velocity.Y = 0.f;
    gShip.angle = 0.f;
    gShip.rotationalVelocity = 0.f;
    gShip.heading.X = cos(gShip.angle + (pi * 1.5));
    gShip.heading.Y = sin(gShip.angle + (pi * 1.5));
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

    fVec2 nextPos;
    nextPos.X = gShip.position.X + gShip.velocity.X;
    nextPos.Y = gShip.position.Y + gShip.velocity.Y;

    gShip.position.X = nextPos.X;
    gShip.position.Y = nextPos.Y;

    gShip.velocity.X -= (gShip.velocity.X / 25.f);
    gShip.velocity.Y -= (gShip.velocity.Y / 25.f);

    if (gShip.position.X > WND_WIDTH) { gShip.position.X -= WND_WIDTH; }
    else if (gShip.position.X < 0) { gShip.position.X += WND_WIDTH; }

    if (gShip.position.Y > WND_HEIGHT) { gShip.position.Y -= WND_HEIGHT; }
    else if (gShip.position.Y < 0) { gShip.position.Y += WND_HEIGHT; }

    //recalculate bullet positions and check if any hit an asteroid
    for(int i = 0; i < 16; i++){
        if(gShip.bullets[i].active){
            gShip.bullets[i].position.X += gShip.bullets[i].heading.X * 5.f;
            gShip.bullets[i].position.Y += gShip.bullets[i].heading.Y * 5.f;

            if(gShip.bullets[i].position.X > WND_WIDTH || gShip.bullets[i].position.X < 0){
                gShip.bullets[i].active = false;
            }
            else if(gShip.bullets[i].position.Y > WND_WIDTH || gShip.bullets[i].position.Y < 0){
                gShip.bullets[i].active = false;
            }

            for(int i = 0; i < 8; i++){
                if(asteroids[i].stage == 0){ continue; }
                float dist = sqrt(pow(gShip.bullets[i].position.X - asteroids[i].primary.position.X, 2) + pow(gShip.bullets[i].position.Y - asteroids[i].primary.position.Y, 2));
                if(dist <= asteroids[i].primary.hitRadius){
                    asteroids[i].stage = 0;
                    gShip.bullets[i].active = false;
                }
            }
        }
    }

    //recalculate asteroid positions
    for(int i = 0; i < 8; i++){
        if(asteroids[i].stage == 0){ continue; }
        asteroids[i].primary.angle += asteroids[i].primary.rVelocity;
        asteroids[i].primary.position.X += asteroids[i].primary.heading.X;
        asteroids[i].primary.position.Y += asteroids[i].primary.heading.Y;

        if(asteroids[i].primary.position.X < -SOFFSET || asteroids[i].primary.position.X > WND_WIDTH + SOFFSET){
            asteroids[i].stage = 0;
        }
        else if(asteroids[i].primary.position.Y < -SOFFSET || asteroids[i].primary.position.Y > WND_HEIGHT + SOFFSET){
            asteroids[i].stage = 0;
        }
    }
}

uint32_t fCounter = 0;
static void draw_callback(Canvas* canvas, void* ctx) {
    if(fCounter++ % 15 == 0){
        spawnNextAsteroid();
    }

    UNUSED(ctx);
    canvas_clear(canvas);
    canvas_draw_frame(canvas, 0, 0, WND_WIDTH, WND_HEIGHT);

	char buf[4];
	itoa(getActualFps(), buf, 10);
    canvas_draw_str(canvas, 5, 10, buf);

    //draw ship
    fVec2 shipPosFront = getPoint(gShip.position, 4.f, gShip.angle + 4.712f);
    fVec2 shipPosLeft = getPoint(gShip.position, 4.f, gShip.angle + 2.199f);
    fVec2 shipPosRight = getPoint(gShip.position, 4.f, gShip.angle + 0.942f);

    canvas_draw_line(canvas, shipPosFront.X, shipPosFront.Y, shipPosLeft.X, shipPosLeft.Y);
    canvas_draw_line(canvas, shipPosLeft.X, shipPosLeft.Y, shipPosRight.X, shipPosRight.Y);
    canvas_draw_line(canvas, shipPosRight.X, shipPosRight.Y, shipPosFront.X, shipPosFront.Y);

    if(thrusting){
        fVec2 shipPosBack = getPoint(gShip.position, 3.75f, gShip.angle + 1.570f);
        canvas_draw_circle(canvas, shipPosBack.X, shipPosBack.Y, 1);
    }

    //draw asteroids
    for(int i = 0; i < 8; i++){
        if(asteroids[i].stage == 0){ continue; }

        fVec2 primaryPoints[5];
        for(int ps = 0; ps < 5; ps++){
            primaryPoints[ps] = getPoint(asteroids[i].primary.position, asteroids[i].primary.points[ps].X, asteroids[i].primary.points[ps].Y + asteroids[i].primary.angle);
        }
        for(int d = 0; d < 5; d++){
            if(d == 4){ canvas_draw_line(canvas, primaryPoints[0].X, primaryPoints[0].Y, primaryPoints[4].X, primaryPoints[4].Y); }
            else{ canvas_draw_line(canvas, primaryPoints[d].X, primaryPoints[d].Y, primaryPoints[d+1].X, primaryPoints[d+1].Y); }
        }
    }

    //draw bullets
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
            if (gShip.rotationalVelocity < 0.40f) {
                gShip.rotationalVelocity += 0.15f;
            }
        }
        if(event.key == InputKeyLeft) {
            //angle left
            if (gShip.rotationalVelocity > -0.40f) {
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