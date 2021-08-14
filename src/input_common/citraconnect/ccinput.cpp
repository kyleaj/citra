// Create factory for analog and digital buttons and touch screen here
// Register the factories
// On connection, set input engine to citraconnect
// Reload input devices

// Copyright 2021 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "ccinput.h"
#include <cstddef>

namespace InputCommon {

const size_t NO_MAP = -1;
constexpr std::array<size_t, Settings::NativeButton::NumButtons> cc_button_to_3ds_mapping{{
    offsetof(ControllerState, ControllerState::a), offsetof(ControllerState, ControllerState::b),
    offsetof(ControllerState, ControllerState::x), offsetof(ControllerState, ControllerState::y),
    offsetof(ControllerState, ControllerState::d_up),
    offsetof(ControllerState, ControllerState::d_down),
    offsetof(ControllerState, ControllerState::d_left),
    offsetof(ControllerState, ControllerState::d_right),
    offsetof(ControllerState, ControllerState::l), offsetof(ControllerState, ControllerState::r),
    offsetof(ControllerState, ControllerState::plus),
    offsetof(ControllerState, ControllerState::minus),
    NO_MAP, // Don't map Debug
    NO_MAP, // Don't map Debug
    offsetof(ControllerState, ControllerState::zl), offsetof(ControllerState, ControllerState::zr),
    NO_MAP, // Don't map Home
}};

constexpr std::array<size_t, Settings::NativeAnalog::NumAnalogs> cc_analog_to_3ds_mapping_x{{
    offsetof(ControllerState, ControllerState::l_stick_x),
    offsetof(ControllerState, ControllerState::r_stick_x)
}};

constexpr std::array<size_t, Settings::NativeAnalog::NumAnalogs> cc_analog_to_3ds_mapping_y{
    {offsetof(ControllerState, ControllerState::l_stick_y),
     offsetof(ControllerState, ControllerState::r_stick_y)}};

class CCButton final : public Input::ButtonDevice {
public:
    explicit CCButton(CCServer* cc_server, size_t button) {
        cc = cc_server;
        offset = button;
    }

    ~CCButton() override = default;

    bool GetStatus() const override {
        if (cc->isClientConnected() && offset != NO_MAP) {
            bool val;
            cc->getKeyStateFromOffset(offset, &val, sizeof(bool));
            return val;
        }
        return false;
    }

private:
    CCServer* cc;
    size_t offset;
};

CCButtonFactory::CCButtonFactory(CCServer* cc_server) {
    cc = cc_server;
}

std::unique_ptr<Input::ButtonDevice> CCButtonFactory::Create(const Common::ParamPackage& params) {
    const size_t button_offset = (size_t)params.Get("button", (int)NO_MAP);
    return std::make_unique<CCButton>(cc, button_offset);
}

Common::ParamPackage CCButtonFactory::GetButtonMapping(
    Settings::NativeButton::Values button) {
    Common::ParamPackage params({{"engine", "ccremote"}});
    auto mapped_button = cc_button_to_3ds_mapping[static_cast<int>(button)];
    params.Set("button", static_cast<u16>(mapped_button));
    return params;
}

class CCAnalog final : public Input::AnalogDevice {
public:
    explicit CCAnalog(CCServer* cc_server, size_t addressX, size_t addressY) {
        cc = cc_server;
        offsetX = addressX;
        offsetY = addressY;
    }

    ~CCAnalog() override = default;

    std::tuple<float, float> GetStatus() const override {
        if (cc->isClientConnected() && offsetX != NO_MAP && offsetY != NO_MAP) {
            uint8_t ux, uy;
            cc->getKeyStateFromOffset(offsetX, &ux, sizeof(uint8_t));
            cc->getKeyStateFromOffset(offsetY, &uy, sizeof(uint8_t));

            float x = coorToFloat(ux);
            float y = coorToFloat(uy);

            float r = (x * x) + (y * y);
            if (r > 1.0f) {
                r = std::sqrtf(r);
                x = x / r;
                y = r / r;
            }

            return {x, y};
        }
        return {0.0f, 0.0f};
    }

private:
    CCServer* cc;
    size_t offsetX;
    size_t offsetY;

    float coorToFloat(uint8_t& c) const {
        return (c / 127.0F) - 1.0F;
    }
};

CCAnalogFactory::CCAnalogFactory(CCServer* cc_server) {
    cc = cc_server;
}

std::unique_ptr<Input::AnalogDevice> CCAnalogFactory::Create(const Common::ParamPackage& params) {
    const size_t stick_offset_x = (size_t)params.Get("analog_x", (int)NO_MAP);
    const size_t stick_offset_y = (size_t)params.Get("analog_y", (int)NO_MAP);
    return std::make_unique<CCAnalog>(cc, stick_offset_x, stick_offset_y);
}

Common::ParamPackage CCAnalogFactory::GetAnalogMapping(Settings::NativeAnalog::Values stick) {
    Common::ParamPackage params({{"engine", "ccremote"}});

    auto mapped_stick_x = cc_analog_to_3ds_mapping_x[static_cast<int>(stick)];
    params.Set("analog_x", static_cast<u16>(mapped_stick_x));

    auto mapped_stick_y = cc_analog_to_3ds_mapping_y[static_cast<int>(stick)];
    params.Set("analog_y", static_cast<u16>(mapped_stick_x));

    return params;
}

} // namespace InputCommon