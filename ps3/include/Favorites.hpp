// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <string>
#include <vector>

namespace ps3me {
class Favorites {
public:
    explicit Favorites(const std::string& file) : file_(file) {}
    bool load();
    bool contains(const std::string& path) const;
    bool toggle(const std::string& path, bool& favorite);
    const std::vector<std::string>& paths() const { return paths_; }
    static bool validGamePath(const std::string& path);
private:
    bool save() const;
    std::string file_;
    std::vector<std::string> paths_;
};
}
