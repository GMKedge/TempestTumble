#pragma once
#include <string>

namespace tempest {

class Texture {
public:
    bool loadFromFile(const std::string& path);
    void destroy();
    void bind(int unit = 0) const;
    int width() const { return w_; }
    int height() const { return h_; }
    unsigned id() const { return id_; }

private:
    unsigned id_ = 0;
    int w_ = 0, h_ = 0;
};

} // namespace tempest
