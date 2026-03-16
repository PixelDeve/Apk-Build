#include "TouchInput.h"
#include <cmath>

TouchPointer* TouchInput::findPointer(int id) {
    for (auto& p : pointers_) if (p.id == id && p.active) return &p;
    return nullptr;
}
TouchPointer* TouchInput::freePointer() {
    for (auto& p : pointers_) if (!p.active) return &p;
    return nullptr;
}
void TouchInput::releasePointer(int id) {
    for (auto& p : pointers_) if (p.id == id) { p.active = false; p.id = -1; }
}

void TouchInput::onTouchEvent(AInputEvent* event, int sw, int sh) {
    int action = AMotionEvent_getAction(event);
    int actionCode = action & AMOTION_EVENT_ACTION_MASK;
    int ptrIdx = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                  >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

    auto processPointer = [&](int idx, bool isDown, bool isMove) {
        int pid = AMotionEvent_getPointerId(event, idx);
        float rx = AMotionEvent_getX(event, idx) / sw;  // 0..1
        float ry = AMotionEvent_getY(event, idx) / sh;

        if (isDown) {
            auto* p = freePointer();
            if (!p) return;
            p->id = pid; p->active = true;
            p->startX = rx; p->startY = ry;
            p->currentX = rx; p->currentY = ry;

            // Determine zone
            if (rx < JOY_ZONE_X + JOY_ZONE_W && joyPointerId_ < 0) {
                joyPointerId_ = pid;
                joyBaseX_ = rx; joyBaseY_ = ry;
            } else if (rx >= LOOK_ZONE_X && lookPointerId_ < 0) {
                lookPointerId_ = pid;
                // Check action buttons
                auto inBtn = [](const ButtonRect& b, float x, float y){
                    return b.active && x>=b.x && x<=b.x+b.w && y>=b.y && y<=b.y+b.h;
                };
                if (inBtn(jumpBtn, rx, ry))     jumpTap_    = true;
                if (inBtn(flyUpBtn, rx, ry))    flyUpHeld_  = true;
                if (inBtn(flyDownBtn, rx, ry))  flyDownHeld_= true;
                if (inBtn(breakBtn, rx, ry))    breakHeld_  = true;
                if (inBtn(placeBtn, rx, ry))    placeTap_   = true;
            }
        } else if (isMove) {
            auto* p = findPointer(pid);
            if (!p) return;
            if (pid == lookPointerId_) {
                rawLookDX_ += (rx - p->currentX) * sw;
                rawLookDY_ += (ry - p->currentY) * sh;
            }
            p->currentX = rx; p->currentY = ry;
        } else {
            // Up
            if (pid == joyPointerId_) joyPointerId_ = -1;
            if (pid == lookPointerId_) {
                lookPointerId_ = -1;
                flyUpHeld_ = flyDownHeld_ = breakHeld_ = false;
            }
            releasePointer(pid);
        }
    };

    if (actionCode == AMOTION_EVENT_ACTION_DOWN || actionCode == AMOTION_EVENT_ACTION_POINTER_DOWN)
        processPointer(ptrIdx, true, false);
    else if (actionCode == AMOTION_EVENT_ACTION_MOVE) {
        int cnt = AMotionEvent_getPointerCount(event);
        for (int i = 0; i < cnt; i++) processPointer(i, false, true);
    } else if (actionCode == AMOTION_EVENT_ACTION_UP || actionCode == AMOTION_EVENT_ACTION_POINTER_UP
            || actionCode == AMOTION_EVENT_ACTION_CANCEL)
        processPointer(ptrIdx, false, false);
}

void TouchInput::update(float dt, InputState& out) {
    out.clearFrameDeltas();

    // ── Joystick ─────────────────────────────────────────────────
    if (joyPointerId_ >= 0) {
        auto* p = findPointer(joyPointerId_);
        if (p) {
            float dx = p->currentX - joyBaseX_;
            float dy = p->currentY - joyBaseY_;
            float len = sqrtf(dx*dx + dy*dy);
            if (len > JOY_RADIUS) { dx *= JOY_RADIUS/len; dy *= JOY_RADIUS/len; }
            out.moveX = dx / JOY_RADIUS;
            out.moveZ = dy / JOY_RADIUS;
        }
    } else {
        out.moveX = 0; out.moveZ = 0;
    }

    // ── Camera look ───────────────────────────────────────────────
    out.lookDeltaX = rawLookDX_ * lookSensitivity_;
    out.lookDeltaY = rawLookDY_ * lookSensitivity_;
    rawLookDX_ = 0; rawLookDY_ = 0;

    out.jumpPressed     = jumpTap_;    jumpTap_   = false;
    out.placeJustTapped = placeTap_;   placeTap_  = false;
    out.breakHeld       = breakHeld_;
    out.flyUp           = flyUpHeld_;
    out.flyDown         = flyDownHeld_;
}
