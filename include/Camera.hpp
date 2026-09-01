#pragma once
#include <glm/glm.hpp>

namespace tempest {

class Camera {
public:
    void setViewportPixels(float viewW, float viewH);
    void setWorldBounds(float worldW, float worldH);
    void follow(const glm::vec2& target, float dt);
    void snapTo(const glm::vec2& target);
    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix() const;
    glm::vec2 position() const { return pos_; }
    float viewWidth() const { return viewW_; }
    float viewHeight() const { return viewH_; }

private:
    glm::vec2 pos_{0, 0};
    float viewW_ = 320.f, viewH_ = 180.f;
    float worldW_ = 1000.f, worldH_ = 320.f;
};

} // namespace tempest
