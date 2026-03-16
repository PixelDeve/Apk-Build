#pragma once
#include <glm/glm.hpp>
#include <unordered_map>
#include <android/input.h>

// ── Mirrors the JS touch control layout ──────────────────────────
// Left joystick zone: movement
// Right swipe zone:   camera look
// Action buttons:     jump, break, place

struct TouchPointer {
    int   id       = -1;
    float startX   = 0, startY = 0;
    float currentX = 0, currentY = 0;
    bool  active   = false;
};

struct InputState {
    // Movement (from joystick, normalised -1..1)
    float moveX = 0, moveZ = 0;

    // Camera delta (from right-swipe)
    float lookDeltaX = 0, lookDeltaY = 0;

    // Action buttons
    bool jumpPressed    = false;
    bool flyUp          = false;
    bool flyDown        = false;
    bool breakHeld      = false;    // left-click held
    bool placeJustTapped= false;    // right-click tap

    // Hotbar scroll (pinch / button tap)
    int  hotbarDelta    = 0;        // +1 / -1

    void clearFrameDeltas() {
        lookDeltaX   = 0;
        lookDeltaY   = 0;
        jumpPressed  = false;
        placeJustTapped = false;
        hotbarDelta  = 0;
    }
};

// ── Screen layout constants (fraction of screen) ─────────────────
static constexpr float JOY_ZONE_X  = 0.0f;   // left 30% = joystick
static constexpr float JOY_ZONE_W  = 0.30f;
static constexpr float JOY_RADIUS  = 0.12f;  // in screen-height units
static constexpr float LOOK_ZONE_X = 0.30f;  // right 70% = look

class TouchInput {
public:
    void onTouchEvent(AInputEvent* event, int screenW, int screenH);
    void update(float dt, InputState& out);

    // Button hit areas (set by UI layout)
    // These are set from the game loop after layout is known
    struct ButtonRect { float x,y,w,h; bool active=false; };
    ButtonRect jumpBtn, flyUpBtn, flyDownBtn, breakBtn, placeBtn;

private:
    static constexpr int MAX_POINTERS = 10;
    TouchPointer pointers_[MAX_POINTERS];

    int  joyPointerId_  = -1;
    int  lookPointerId_ = -1;

    float joyBaseX_ = 0, joyBaseY_ = 0;

    float lookSensitivity_ = 0.004f;

    // Raw look accumulator between frames
    float rawLookDX_ = 0, rawLookDY_ = 0;
    bool  breakHeld_ = false;
    bool  placeTap_  = false;
    bool  jumpTap_   = false;
    bool  flyUpHeld_ = false, flyDownHeld_ = false;

    TouchPointer* findPointer(int id);
    TouchPointer* freePointer();
    void releasePointer(int id);
};
