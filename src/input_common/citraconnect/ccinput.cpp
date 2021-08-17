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
#include "core/core.h"
#include "ccinputadapter.h"
#include <tuple>
#include <memory>

namespace InputCommon {

class CCButton final : public Input::ButtonDevice {
public:
    explicit CCButton(std::shared_ptr<CCInputAdapter> cc_server, int but) {
        cc = cc_server;
        button = but;
    }

    ~CCButton() override = default;

    bool GetStatus() const override {
        return cc->PollButton(button);
    }

private:
    std::shared_ptr<CCInputAdapter> cc;
    int button;
};

CCButtonFactory::CCButtonFactory(std::shared_ptr<CCInputAdapter> inputAdapter) {
    adapter = inputAdapter;
}

std::unique_ptr<Input::ButtonDevice> CCButtonFactory::Create(const Common::ParamPackage& params) {
    const int button = (size_t)params.Get("button", Settings::NativeButton::NumButtons); // Default to invalid button always returning false
    
    return std::make_unique<CCButton>(adapter, button);
}

Common::ParamPackage CCButtonFactory::GetButtonMapping(
    Settings::NativeButton::Values button) {
    Common::ParamPackage params({{"engine", "ccremote"}});
    params.Set("button", static_cast<u16>(static_cast<int>(button)));
    return params;
}

class CCAnalog final : public Input::AnalogDevice {
public:
    explicit CCAnalog(std::shared_ptr<CCInputAdapter> cc_server, int analog) {
        cc = cc_server;
        stick = analog;
    }

    ~CCAnalog() override = default;

    std::tuple<float, float> GetStatus() const override {
        uint8_t ux, uy;
        std::tie(ux, uy) = cc->PollAnalog(stick);

        float x = coorToFloat(ux);
        float y = coorToFloat(uy)*-1;

        float r = (x * x) + (y * y);
        if (r > 1.0f) {
            r = std::sqrtf(r);
            x = x / r;
            y = y / r;
        }

        return {x, y};
    }

private:
    std::shared_ptr<CCInputAdapter> cc;
    int stick;

    float coorToFloat(uint8_t& c) const {
        return (c / 127.0f) - 1;
    }
};

CCAnalogFactory::CCAnalogFactory(std::shared_ptr<CCInputAdapter> inputAdapter) {
    adapter = inputAdapter;
}

std::unique_ptr<Input::AnalogDevice> CCAnalogFactory::Create(const Common::ParamPackage& params) {
    int stick = (size_t)params.Get("analog", Settings::NativeAnalog::NumAnalogs);
    return std::make_unique<CCAnalog>(adapter, stick);
}

Common::ParamPackage CCAnalogFactory::GetAnalogMapping(Settings::NativeAnalog::Values stick) {
    Common::ParamPackage params({{"engine", "ccremote"}});
    params.Set("analog", static_cast<int>(stick));
    return params;
}

class CCTouchDevice final : public Input::TouchDevice {
public:
    explicit CCTouchDevice(std::shared_ptr<CCInputAdapter> cc_server) {
        cc = cc_server;
    }

    ~CCTouchDevice() override = default;

    std::tuple<float, float, bool> GetStatus() const override {
        return cc->PollTouch();
    }

private:
    std::shared_ptr<CCInputAdapter> cc;
};

CCTouchFactory::CCTouchFactory(std::shared_ptr<CCInputAdapter> inputAdapter) {
    adapter = inputAdapter;
}

std::unique_ptr<Input::TouchDevice> CCTouchFactory::Create(const Common::ParamPackage& params) {
    return std::make_unique<CCTouchDevice>(adapter);
}

} // namespace InputCommon