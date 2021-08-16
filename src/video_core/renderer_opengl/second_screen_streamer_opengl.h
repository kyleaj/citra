// Copyright 2020 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "cc-server.h"
#include "core/frontend/framebuffer_layout.h"
#include "core/frontend/emu_window.h"
#include "video_core/renderer_opengl/renderer_opengl.h"

#include <thread>
#include <atomic>
#include <memory>

namespace CitraConnect {

/**
* Wrapper around CCServer class to forward frames over the network.
**/
class SecondScreenStream {

private:
    std::shared_ptr<CCServer> server;
    std::unique_ptr<Frontend::GraphicsContext> context;
    std::atomic_bool stop_requested{false};
    std::thread present_thread;

    // PBOs used to dump frames faster
    std::array<OpenGL::OGLBuffer, 2> pbos;
    GLuint current_pbo = 1;
    GLuint next_pbo = 0;

public:
    std::unique_ptr<Frontend::TextureMailbox> mailbox;

    explicit SecondScreenStream(std::shared_ptr<CCServer> cc_server,
                                Frontend::EmuWindow& emu_window);

    ~SecondScreenStream() = default;

    bool IsStreaming();

    void StartForwarding();

    void StopForwarding();

    Layout::FramebufferLayout GetLayout();

    void PresentLoop();

    void InitializeOpenGLObjects();

    void CleanupOpenGLObjects();

};

}