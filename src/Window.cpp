#include "Window.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>

namespace tempest {

bool Window::create(int width, int height, const std::string& title, bool vsync) {
    if (!glfwInit()) {
        std::cerr << "glfwInit failed\n";
        return false;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
    handle_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!handle_) {
        std::cerr << "glfwCreateWindow failed (need OpenGL 3.3)\n";
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(handle_);
    glfwSwapInterval(vsync ? 1 : 0);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "gladLoadGLLoader failed\n";
        return false;
    }
    width_ = width;
    height_ = height;
    glfwGetFramebufferSize(handle_, &fbW_, &fbH_);
    glViewport(0, 0, fbW_, fbH_);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    std::cout << "OpenGL " << glGetString(GL_VERSION) << "\n";
    return true;
}

void Window::destroy() {
    if (handle_) {
        glfwDestroyWindow(handle_);
        handle_ = nullptr;
    }
    glfwTerminate();
}

bool Window::shouldClose() const { return handle_ && glfwWindowShouldClose(handle_); }
void Window::poll() {
    glfwPollEvents();
    if (handle_) glfwGetFramebufferSize(handle_, &fbW_, &fbH_);
}
void Window::swap() { if (handle_) glfwSwapBuffers(handle_); }
void Window::requestClose() { if (handle_) glfwSetWindowShouldClose(handle_, 1); }

void Window::beginLetterboxed(int logicalW, int logicalH) {
    if (!handle_ || logicalW <= 0 || logicalH <= 0) return;
    glfwGetFramebufferSize(handle_, &fbW_, &fbH_);
    glViewport(0, 0, fbW_, fbH_);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    int sx = fbW_ / logicalW;
    int sy = fbH_ / logicalH;
    int s = std::min(sx, sy);
    if (s < 1) s = 1;
    int dw = logicalW * s;
    int dh = logicalH * s;
    int ox = (fbW_ - dw) / 2;
    int oy = (fbH_ - dh) / 2;
    glViewport(ox, oy, dw, dh);
}

} // namespace tempest
