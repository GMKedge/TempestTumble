#include "Renderer.hpp"
#include "Atlas.hpp"
#include <glad/glad.h>
#include <cctype>
#include <cmath>

namespace tempest {

bool Renderer::init(const std::string& shaderDir) {
    if (!sprite_.loadFromFiles(shaderDir + "/sprite.vert", shaderDir + "/sprite.frag"))
        return false;
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(SpriteVertex) * kMaxBatchVerts, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), reinterpret_cast<void*>(8));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), reinterpret_cast<void*>(16));
    glBindVertexArray(0);
    verts_.reserve(1024);
    return true;
}

void Renderer::destroy() {
    sprite_.destroy();
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    vbo_ = vao_ = 0;
}

void Renderer::begin(const Camera& cam, const glm::vec4& clearColor) {
    glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    proj_ = cam.projectionMatrix();
    view_ = cam.viewMatrix();
    verts_.clear();
    bound_ = nullptr;
}

void Renderer::flush() {
    if (verts_.empty() || !bound_) return;
    sprite_.bind();
    sprite_.setMat4("uProjection", proj_);
    sprite_.setMat4("uView", view_);
    sprite_.setInt("uTex", 0);
    bound_->bind(0);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    static_cast<GLsizeiptr>(verts_.size() * sizeof(SpriteVertex)), verts_.data());
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(verts_.size()));
    verts_.clear();
}

void Renderer::drawSprite(const Texture& tex, const Rect& dest, const glm::vec4& uv,
                          const glm::vec4& tint, bool flipX) {
    if (bound_ != &tex) {
        flush();
        bound_ = &tex;
    }
    if (verts_.size() + 6 > static_cast<size_t>(kMaxBatchVerts)) flush();

    float u0 = uv.x, v0 = uv.y, u1 = uv.x + uv.z, v1 = uv.y + uv.w;
    if (flipX) std::swap(u0, u1);
    // Dest top (y0) samples atlas v0 (PNG top). No extra V invert.
    const float x0 = std::floor(dest.x + 0.5f);
    const float y0 = std::floor(dest.y + 0.5f);
    const float x1 = x0 + std::floor(dest.w + 0.5f);
    const float y1 = y0 + std::floor(dest.h + 0.5f);
    const float r = tint.r, g = tint.g, b = tint.b, a = tint.a;
    SpriteVertex q[6] = {
        {x0, y0, u0, v0, r, g, b, a},
        {x1, y0, u1, v0, r, g, b, a},
        {x1, y1, u1, v1, r, g, b, a},
        {x0, y0, u0, v0, r, g, b, a},
        {x1, y1, u1, v1, r, g, b, a},
        {x0, y1, u0, v1, r, g, b, a},
    };
    verts_.insert(verts_.end(), q, q + 6);
}

void Renderer::drawQuad(const Rect& dest, const glm::vec4& color) {
    if (!bound_) return;
    drawSprite(*bound_, dest, spriteUV("whitepx"), color);
}

static const char* fontSprite(char c) {
    unsigned char u = static_cast<unsigned char>(std::toupper(static_cast<unsigned char>(c)));
    switch (u) {
    case '0': return "f_0"; case '1': return "f_1"; case '2': return "f_2";
    case '3': return "f_3"; case '4': return "f_4"; case '5': return "f_5";
    case '6': return "f_6"; case '7': return "f_7"; case '8': return "f_8";
    case '9': return "f_9";
    case 'A': return "f_A"; case 'B': return "f_B"; case 'C': return "f_C";
    case 'D': return "f_D"; case 'E': return "f_E"; case 'F': return "f_F";
    case 'G': return "f_G"; case 'H': return "f_H"; case 'I': return "f_I";
    case 'J': return "f_J"; case 'K': return "f_K"; case 'L': return "f_L";
    case 'M': return "f_M"; case 'N': return "f_N"; case 'O': return "f_O";
    case 'P': return "f_P"; case 'Q': return "f_Q"; case 'R': return "f_R";
    case 'S': return "f_S"; case 'T': return "f_T"; case 'U': return "f_U";
    case 'V': return "f_V"; case 'W': return "f_W"; case 'X': return "f_X";
    case 'Y': return "f_Y"; case 'Z': return "f_Z";
    case '+': return "f_plus"; case '/': return "f_slashc"; case ':': return "f_colon";
    case '!': return "f_bang"; case '?': return "f_q"; case '-': return "f_dash";
    case '.': return "f_dot"; case ' ': return "f_sp";
    default: return "f_q";
    }
}

void Renderer::drawText(const Texture& atlas, const std::string& text, glm::vec2 pos,
                        float scale, const glm::vec4& tint) {
    float x = pos.x;
    for (char c : text) {
        drawSprite(atlas, Rect{x, pos.y, kTileSize * scale, kTileSize * scale},
                   spriteUV(fontSprite(c)), tint);
        x += 8.f * scale;
    }
}

void Renderer::end() { flush(); }

} // namespace tempest
