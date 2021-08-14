// Copyright 2021 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include "core/frontend/input.h"
#include "core/settings.h"
#include "cc-server.h"
#include "input_common/main.h"

namespace InputCommon {

/**
 * A button device factory representing a remote gamepad connected with Citra Connect.
 */
class CCButtonFactory final : public Input::Factory<Input::ButtonDevice> {
public:
    explicit CCButtonFactory(CCServer* cc_server);

    std::unique_ptr<Input::ButtonDevice> Create(const Common::ParamPackage& params) override;

    Common::ParamPackage GetButtonMapping(Settings::NativeButton::Values button);

private:
    CCServer* cc;
};

/// An analog device factory that creates analog devices repreenting a remote gamepad connected with Citra Connect.
class CCAnalogFactory final : public Input::Factory<Input::AnalogDevice> {
public:
    explicit CCAnalogFactory(CCServer* cc_server);

    std::unique_ptr<Input::AnalogDevice> Create(const Common::ParamPackage& params) override;

    Common::ParamPackage GetAnalogMapping(Settings::NativeAnalog::Values button);

private:
    CCServer* cc;
};

} // namespace InputCommon