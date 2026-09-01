#pragma once
#include <string>
#include <glm/glm.hpp>

namespace tempest {

class Shader {
public:
    bool loadFromFiles(const std::string& vertPath, const std::string& fragPath);
    void destroy();
    void bind() const;
    void setMat4(const char* name, const glm::mat4& m) const;
    void setVec4(const char* name, const glm::vec4& v) const;
    void setInt(const char* name, int v) const;
    unsigned id() const { return id_; }

private:
    unsigned id_ = 0;
    static unsigned compile(unsigned type, const std::string& src, std::string& err);
};

} // namespace tempest
