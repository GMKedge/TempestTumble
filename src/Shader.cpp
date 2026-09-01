#include "Shader.hpp"
#include <glad/glad.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace tempest {
namespace {
std::string readFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) return {};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}
}

unsigned Shader::compile(unsigned type, const std::string& src, std::string& err) {
    unsigned s = glCreateShader(type);
    const char* p = src.c_str();
    glShaderSource(s, 1, &p, nullptr);
    glCompileShader(s);
    int ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        int n = 0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &n);
        std::vector<char> log(static_cast<size_t>(n));
        glGetShaderInfoLog(s, n, nullptr, log.data());
        err = log.data();
        glDeleteShader(s);
        return 0;
    }
    return s;
}

bool Shader::loadFromFiles(const std::string& vertPath, const std::string& fragPath) {
    auto vs = readFile(vertPath);
    auto fs = readFile(fragPath);
    if (vs.empty() || fs.empty()) {
        std::cerr << "Missing shader file: " << vertPath << " or " << fragPath << "\n";
        return false;
    }
    std::string err;
    unsigned v = compile(GL_VERTEX_SHADER, vs, err);
    if (!v) { std::cerr << "Vertex shader: " << err << "\n"; return false; }
    unsigned f = compile(GL_FRAGMENT_SHADER, fs, err);
    if (!f) { std::cerr << "Fragment shader: " << err << "\n"; glDeleteShader(v); return false; }
    id_ = glCreateProgram();
    glAttachShader(id_, v);
    glAttachShader(id_, f);
    glLinkProgram(id_);
    glDeleteShader(v);
    glDeleteShader(f);
    int ok = 0;
    glGetProgramiv(id_, GL_LINK_STATUS, &ok);
    if (!ok) {
        int n = 0;
        glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &n);
        std::vector<char> log(static_cast<size_t>(n));
        glGetProgramInfoLog(id_, n, nullptr, log.data());
        std::cerr << "Link: " << log.data() << "\n";
        glDeleteProgram(id_);
        id_ = 0;
        return false;
    }
    return true;
}

void Shader::destroy() {
    if (id_) { glDeleteProgram(id_); id_ = 0; }
}
void Shader::bind() const { glUseProgram(id_); }
void Shader::setMat4(const char* name, const glm::mat4& m) const {
    glUniformMatrix4fv(glGetUniformLocation(id_, name), 1, GL_FALSE, &m[0][0]);
}
void Shader::setVec4(const char* name, const glm::vec4& v) const {
    glUniform4f(glGetUniformLocation(id_, name), v.x, v.y, v.z, v.w);
}
void Shader::setInt(const char* name, int v) const {
    glUniform1i(glGetUniformLocation(id_, name), v);
}

} // namespace tempest
