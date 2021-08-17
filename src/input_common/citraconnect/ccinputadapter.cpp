#include "ccinputadapter.h"
#include "cc-server.h"
#include "core/core.h"
#include "core/settings.h"

#include <chrono>

const int MIN_POLL_INTERVAL_MS = 8; // At least 8ms must pass before polling a new state (~120 fps)
const uint8_t CENTER = 127;

CCInputAdapter::CCInputAdapter() {
    server = nullptr;
    currState = ControllerState();
    lastPoll = 0;
}

bool CCInputAdapter::Preamble() {
    if (server == nullptr) {
        server = Core::System::GetInstance().CitraConnectManager();
        if (server == nullptr) {
            return false;
        }
    }

    if (!server->isClientConnected()) {
        return false;
    }

    long currTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();

    if (currTime - lastPoll >= MIN_POLL_INTERVAL_MS) {
        server->getKeyState(currState);
        lastPoll = currTime;
    }

    return true;
}

bool CCInputAdapter::PollButton(const int button) {
    if (!Preamble()) {
        return false;
    }

    switch (button) {
    case Settings::NativeButton::A:
        return currState.a;
    case Settings::NativeButton::B:
        return currState.b;
    case Settings::NativeButton::X:
        return currState.x;
    case Settings::NativeButton::Y:
        return currState.y;
    case Settings::NativeButton::Up:
        return currState.d_up;
    case Settings::NativeButton::Down:
        return currState.d_down;
    case Settings::NativeButton::Left:
        return currState.d_left;
    case Settings::NativeButton::Right:
        return currState.d_right;
    case Settings::NativeButton::L:
        return currState.l;
    case Settings::NativeButton::R:
        return currState.r;
    case Settings::NativeButton::Start:
        return currState.plus;
    case Settings::NativeButton::Select:
        return currState.minus;
    case Settings::NativeButton::ZL:
        return currState.zl;
    case Settings::NativeButton::ZR:
        return currState.zr;
    default:
        return false;
    }

    UNREACHABLE();
    return false;
}

std::tuple<uint8_t, uint8_t> CCInputAdapter::PollAnalog(int analog) {
    if (!Preamble()) {
        return {CENTER, CENTER};
    }

    switch (analog) { 
        case Settings::NativeAnalog::CirclePad:
            return {currState.l_stick_x, currState.l_stick_y};
        case Settings::NativeAnalog::CStick:
            return {currState.r_stick_x, currState.r_stick_y};
        default:
            return {CENTER, CENTER};
    }

    UNREACHABLE();
    return {CENTER, CENTER};
}

std::tuple<float, float, bool> CCInputAdapter::PollTouch() {
    if (!Preamble()) {
        return {false, 0.0f, 0.0f};
    }

    // TODO: Calculate cutoffs based on requested resolution
    double border = (255*(1-(640.0F / 854.0F)))/2.0;

    bool touching = currState.touch_down;
    float touchY = currState.touch_y/255.0;
    double touchX = currState.touch_x;
    if (touchX < border || touchX > (255.0 - border)) {
        touching = false;
        touchX = 0;
    } else {
        touchX = (touchX - border) / (255.0 - (2*border));
    }

    return {(float)touchX, touchY, touching};
}