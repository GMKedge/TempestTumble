#pragma once
#include "Shader.hpp"
#include "Texture.hpp"
#include "Camera.hpp"
#include "Types.hpp"
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace tempest {

struct SpriteVertex {
    float x, y, u, v, r, g, b, a;
};

class Renderer {
public:
    bool init(const std::string& shaderDir);
    void destroy();
    void begin(const Camera& cam, const glm::vec4& clearColor);
    void drawSprite(const Texture& tex, const Rect& dest, const glm::vec4& uv,
                    const glm::vec4& tint = {1, 1, 1, 1}, bool flipX = false);
    void drawQuad(const Rect& dest, const glm::vec4& color);
    void drawText(const Texture& atlas, const std::string& text, glm::vec2 pos,
                  float scale = 1.f, const glm::vec4& tint = {1, 1, 1, 1});
    void end();

private:
    void flush();
    Shader sprite_;
    unsigned vao_ = 0, vbo_ = 0;
    std::vector<SpriteVertex> verts_;
    const Texture* bound_ = nullptr;
    glm::mat4 proj_{1.0f};
    glm::mat4 view_{1.0f};
};

} // namespace tempest
