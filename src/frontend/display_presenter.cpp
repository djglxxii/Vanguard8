#include "frontend/display_presenter.hpp"

#include <SDL_opengl.h>

namespace vanguard8::frontend {

namespace {

void draw_textured_quad() {
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(0.0F, 0.0F);
    glVertex2f(-1.0F, 1.0F);
    glTexCoord2f(0.0F, 1.0F);
    glVertex2f(-1.0F, -1.0F);
    glTexCoord2f(1.0F, 0.0F);
    glVertex2f(1.0F, 1.0F);
    glTexCoord2f(1.0F, 1.0F);
    glVertex2f(1.0F, -1.0F);
    glEnd();
}

}  // namespace

OpenGlDisplayPresenter::~OpenGlDisplayPresenter() { shutdown(); }

auto OpenGlDisplayPresenter::initialize(std::string& error) -> bool {
    shutdown();

    glGenTextures(1, &texture_id_);
    if (texture_id_ == 0U) {
        error = "OpenGL texture creation failed.";
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB8,
        Display::frame_width,
        Display::frame_height,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        nullptr
    );
    initialized_ = true;
    return true;
}

auto OpenGlDisplayPresenter::present(
    const Display& display,
    const int drawable_width,
    const int drawable_height,
    std::string& error
) -> bool {
    if (!initialized_) {
        error = "OpenGL presenter not initialized.";
        return false;
    }
    if (display.uploaded_frame().empty()) {
        error = "Display presenter requires an uploaded frame.";
        return false;
    }

    glViewport(0, 0, drawable_width, drawable_height);
    glClearColor(0.07F, 0.07F, 0.09F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0,
        0,
        Display::frame_width,
        Display::frame_height,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        display.uploaded_frame().data()
    );

    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_TEXTURE_2D);
    draw_textured_quad();
    glDisable(GL_TEXTURE_2D);
    return true;
}

auto OpenGlDisplayPresenter::readback_rgb_frame(std::string& error) const -> std::vector<std::uint8_t> {
    if (!initialized_) {
        error = "OpenGL presenter not initialized.";
        return {};
    }

    std::vector<std::uint8_t> frame(static_cast<std::size_t>(Display::frame_width * Display::frame_height * 3), 0x00);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, frame.data());
    return frame;
}

void OpenGlDisplayPresenter::shutdown() {
    if (texture_id_ != 0U) {
        glDeleteTextures(1, &texture_id_);
        texture_id_ = 0;
    }
    initialized_ = false;
}

}  // namespace vanguard8::frontend
