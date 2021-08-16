// Copyright 2020 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "cc-server.h"
#include "core/core.h"
#include "core/frontend/framebuffer_layout.h"
#include "core/frontend/emu_window.h"
#include "core/frontend/scope_acquire_context.h"
#include "core/hle/service/hid/hid.h"
#include "video_core/renderer_opengl/gl_resource_manager.h"
#include "video_core/renderer_opengl/renderer_opengl.h"
#include "video_core/renderer_opengl/second_screen_streamer_opengl.h"

#include <thread>
#include <atomic>
#include <memory>
#include <core/hle/service/ir/ir_user.h>
#include <core/hle/service/ir/ir_rst.h>

namespace CitraConnect {

    SecondScreenStream::SecondScreenStream(std::shared_ptr<CCServer> cc_server,
                                       Frontend::EmuWindow& emu_window)
        : context(emu_window.CreateSharedContext()) {
        server = cc_server;
    }

    bool SecondScreenStream::IsStreaming() {
        return server->isClientConnected();
    }

    void SecondScreenStream::StartForwarding() {
        if (present_thread.joinable())
            present_thread.join();

        present_thread = std::thread(&SecondScreenStream::PresentLoop, this);
    }

    void SecondScreenStream::StopForwarding() {
        stop_requested.store(true, std::memory_order_relaxed);
    }

    Layout::FramebufferLayout SecondScreenStream::GetLayout() {
        return Layout::SingleFrameLayout(server->getRequestedWidth(), server->getRequestedHeight(),
                                         true, true);
    }

    void SecondScreenStream::PresentLoop() {
        LOG_INFO(Render_OpenGL, "In present loop, waiting for connection...");
        while (!IsStreaming()) {
            mailbox->TryGetPresentFrame(200);
        }

        LOG_INFO(Render_OpenGL, "Connected!");

        // Force input pollers to use CitraConnect
        auto& system = Core::System::GetInstance();
        auto hid = Service::HID::GetModule(system);
        if (hid) {
            hid->ReloadInputDevices();
            LOG_INFO(Render_OpenGL, "Requested input device reload!");
        } else {
            LOG_WARNING(Render_OpenGL, "Failed to reload input devices after remote connection!");
        }

        auto sm = system.ServiceManager();
        auto ir_user = sm.GetService<Service::IR::IR_USER>("ir:USER");
        if (ir_user)
            ir_user->ReloadInputDevices();
        auto ir_rst = sm.GetService<Service::IR::IR_RST>("ir:rst");
        if (ir_rst)
            ir_rst->ReloadInputDevices();

        Frontend::ScopeAcquireContext scope{*context};
        InitializeOpenGLObjects();

        const auto& layout = GetLayout();
        while (!stop_requested.exchange(false)) {
            Frontend::Frame* frame = mailbox->TryGetPresentFrame(200);
            if (!frame) {
                continue;
            }

            if (frame->color_reloaded) {
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
            if (pixels == NULL) {
                LOG_WARNING(Render_OpenGL, "Pixels are null");
            } else {
                VideoDumper::VideoFrame frame_data{layout.width, layout.height, pixels};
                if (!server->sendFrame(frame_data.data.data(), frame_data.width, frame_data.height,
                                       frame_data.stride)) {
                    LOG_WARNING(Render_OpenGL, "Send frame failed!");
                }
            }
            
            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

            current_pbo = (current_pbo + 1) % 2;
            next_pbo = (current_pbo + 1) % 2;
        }

        // If not still connected, reset input settings.

        stop_requested = false;
        CleanupOpenGLObjects();
    }

    void SecondScreenStream::InitializeOpenGLObjects() {
        const auto& layout = GetLayout();
        for (auto& buffer : pbos) {
            buffer.Create();
            glBindBuffer(GL_PIXEL_PACK_BUFFER, buffer.handle);
            glBufferData(GL_PIXEL_PACK_BUFFER, layout.width * layout.height * 4, nullptr,
                         GL_STREAM_READ);
            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        }
    }

    void SecondScreenStream::CleanupOpenGLObjects() {
        for (auto& buffer : pbos) {
            buffer.Release();
        }
    }

}