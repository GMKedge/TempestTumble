#pragma once
#include <string>
struct GLFWwindow;

namespace tempest {

class Window {
public:
    bool create(int width, int height, const std::string& title, bool vsync);
    void destroy();
    bool shouldClose() const;
    void poll();
    void swap();
    void requestClose();
    void beginLetterboxed(int logicalW, int logicalH);
    GLFWwindow* handle() const { return handle_; }
    int width() const { return width_; }
    int height() const { return height_; }
    int framebufferWidth() const { return fbW_; }
    int framebufferHeight() const { return fbH_; }

private:
    GLFWwindow* handle_ = nullptr;
    int width_ = 0, height_ = 0, fbW_ = 0, fbH_ = 0;
};

} // namespace tempest
