// Copyright 2021 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include "core/frontend/input.h"
#include "core/settings.h"
#include "input_common/main.h"
#include "ccinputadapter.h"

namespace InputCommon {

/**
 * A button device factory representing a remote gamepad connected with Citra Connect.
 */
class CCButtonFactory final : public Input::Factory<Input::ButtonDevice> {
private:
    std::shared_ptr<CCInputAdapter> adapter;

public:
    explicit CCButtonFactory(std::shared_ptr<CCInputAdapter> inputAdapter);

    std::unique_ptr<Input::ButtonDevice> Create(const Common::ParamPackage& params) override;

    Common::ParamPackage GetButtonMapping(Settings::NativeButton::Values button);
};

/// An analog device factory that creates analog devices repreenting a remote gamepad connected with Citra Connect.
class CCAnalogFactory final : public Input::Factory<Input::AnalogDevice> {
private:
    std::shared_ptr<CCInputAdapter> adapter;

public:
    explicit CCAnalogFactory(std::shared_ptr<CCInputAdapter> inputAdapter);

    std::unique_ptr<Input::AnalogDevice> Create(const Common::ParamPackage& params) override;

    Common::ParamPackage GetAnalogMapping(Settings::NativeAnalog::Values button);
};

/**
 * A touch device factory representing a remote gamepad connected with Citra Connect.
 */
class CCTouchFactory final : public Input::Factory<Input::TouchDevice> {
private:
    std::shared_ptr<CCInputAdapter> adapter;

public:
    explicit CCTouchFactory(std::shared_ptr<CCInputAdapter> inputAdapter);

    std::unique_ptr<Input::TouchDevice> Create(const Common::ParamPackage& params) override;
};


} // namespace InputCommon