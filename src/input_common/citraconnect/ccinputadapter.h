#pragma once

#include "cc-server.h"

class CCInputAdapter {
private:
    std::shared_ptr<CCServer> server;
    ControllerState currState;
    long lastPoll;

    bool Preamble();

public:
    CCInputAdapter();

    bool PollButton(int button);

    std::tuple<uint8_t, uint8_t> PollAnalog(int analog);
};