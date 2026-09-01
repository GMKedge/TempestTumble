#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace tempest {

void Camera::setViewportPixels(float viewW, float viewH) {
    viewW_ = viewW;
    viewH_ = viewH;
}
void Camera::setWorldBounds(float worldW, float worldH) {
    worldW_ = worldW;
    worldH_ = worldH;
}
void Camera::snapTo(const glm::vec2& target) {
    pos_ = target - glm::vec2(viewW_, viewH_) * 0.5f;
    pos_.x = std::clamp(pos_.x, 0.f, std::max(0.f, worldW_ - viewW_));
    pos_.y = std::clamp(pos_.y, 0.f, std::max(0.f, worldH_ - viewH_));
}
void Camera::follow(const glm::vec2& target, float dt) {
    glm::vec2 want = target - glm::vec2(viewW_, viewH_) * 0.5f;
    want.x = std::clamp(want.x, 0.f, std::max(0.f, worldW_ - viewW_));
    want.y = std::clamp(want.y, 0.f, std::max(0.f, worldH_ - viewH_));
    pos_ += (want - pos_) * std::min(1.f, dt * 8.f);
}
glm::mat4 Camera::viewMatrix() const { return glm::mat4(1.0f); }
glm::mat4 Camera::projectionMatrix() const {
    return glm::ortho(pos_.x, pos_.x + viewW_, pos_.y + viewH_, pos_.y, -1.f, 1.f);
}

} // namespace tempest
