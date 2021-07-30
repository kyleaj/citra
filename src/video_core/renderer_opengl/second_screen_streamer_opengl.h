// Copyright 2020 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "../CitraConnect/Server/cc-server.h"
#include "core/frontend/framebuffer_layout.h"
#include "core/frontend/emu_window.h"
#include "core/frontend/emu_window.h"
#include "core/frontend/scope_acquire_context.h"
#include "video_core/renderer_opengl/gl_resource_manager.h"

#include <thread>
#include <atomic>

namespace CitraConnect {

/**
* Wrapper around CCServer class to forward frames over the network.
**/
class SecondScreenStream {

private:
    CCServer* server;
    std::unique_ptr<Frontend::GraphicsContext> context;
    std::atomic_bool stop_requested{false};
    std::thread present_thread;

    // PBOs used to dump frames faster
    std::array<OpenGL::OGLBuffer, 2> pbos;
    GLuint current_pbo = 1;
    GLuint next_pbo = 0;

public:
    std::unique_ptr<Frontend::TextureMailbox> mailbox;

    explicit SecondScreenStream(CCServer& cc_server, Frontend::EmuWindow& emu_window)
        : context(emu_window.CreateSharedContext()) {
        server = &cc_server;
    }

    ~SecondScreenStream() = default;

    bool IsStreaming() {
        return server->isClientConnected();
    }

    void StartForwarding() {
        if (present_thread.joinable())
            present_thread.join();

        present_thread = std::thread(&SecondScreenStream::PresentLoop, this);
    }

    void StopForwarding() {
        stop_requested.store(true, std::memory_order_relaxed);
    }

    Layout::FramebufferLayout GetLayout() {
        return Layout::SingleFrameLayout(server->getRequestedWidth(), server->getRequestedHeight(),
                                         true, true);
    }

    void PresentLoop() {
        Frontend::ScopeAcquireContext scope{*context};
        InitializeOpenGLObjects();

        const auto& layout = GetLayout();
        while (!stop_requested.exchange(false)) {
            auto frame = mailbox->TryGetPresentFrame(200);
            if (!frame) {
                continue;
            }

            if (frame->color_reloaded) {
                LOG_DEBUG(Render_OpenGL, "Reloading present frame");
                mailbox->ReloadPresentFrame(frame, layout.width, layout.height);
            }
            glWaitSync(frame->render_fence, 0, GL_TIMEOUT_IGNORED);

            glBindFramebuffer(GL_READ_FRAMEBUFFER, frame->present.handle);
            glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos[current_pbo].handle);
            glReadPixels(0, 0, layout.width, layout.height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
                         0);

            // Insert fence for the main thread to block on
            frame->present_fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
            glFlush();

            // Bind the previous PBO and read the pixels
            glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos[next_pbo].handle);
            GLubyte* pixels =
                static_cast<GLubyte*>(glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY));
            VideoDumper::VideoFrame frame_data{layout.width, layout.height, pixels};
            server->sendFrame(frame_data.data.data(), frame_data.width, frame_data.height,
                              frame_data.stride);
            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

            current_pbo = (current_pbo + 1) % 2;
            next_pbo = (current_pbo + 1) % 2;
        }
        stop_requested = false;
        CleanupOpenGLObjects();
    }

    void InitializeOpenGLObjects() {
        const auto& layout = GetLayout();
        for (auto& buffer : pbos) {
            buffer.Create();
            glBindBuffer(GL_PIXEL_PACK_BUFFER, buffer.handle);
            glBufferData(GL_PIXEL_PACK_BUFFER, layout.width * layout.height * 4, nullptr,
                         GL_STREAM_READ);
            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        }
    }

    void CleanupOpenGLObjects() {
        for (auto& buffer : pbos) {
            buffer.Release();
        }
    }

};

}