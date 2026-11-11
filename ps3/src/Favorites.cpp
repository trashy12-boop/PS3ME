// SPDX-License-Identifier: GPL-2.0-only
#include "Favorites.hpp"
#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdio>
#include <cstring>

namespace ps3me {
namespace {
const size_t maxFavorites = 512;
const size_t maxPathLength = 1024;
const char header[] = "PS3ME_FAVORITES 1\n";

bool underRoot(const std::string& path, const char* root) {
    const size_t length = std::strlen(root);
    return path.size() > length && path.compare(0, length, root) == 0 && path[length] == '/';
}

bool allowedLocation(const std::string& path) {
    return underRoot(path, "/dev_hdd0") ||
           underRoot(path, "/dev_usb000") || underRoot(path, "/dev_usb001");
}

bool missing(const std::string& path) {
    errno = 0;
    FILE* file = std::fopen(path.c_str(), "rb");
    if (file) { std::fclose(file); return false; }
    return errno == ENOENT;
}

bool parseFile(const std::string& path, std::vector<std::string>& result) {
    FILE* file = std::fopen(path.c_str(), "rb");
    if (!file) return false;
    char line[maxPathLength + 2];
    bool ok = std::fgets(line, sizeof(line), file) && std::strcmp(line, header) == 0;
    std::vector<std::string> parsed;
    while (ok && std::fgets(line, sizeof(line), file)) {
        size_t length = std::strlen(line);
        if (length == 0 || line[length - 1] != '\n' || length > maxPathLength + 1) {
            ok = false;
            break;
        }
        line[--length] = '\0';
        std::string value(line, length);
        if (!Favorites::validGamePath(value) ||
            std::find(parsed.begin(), parsed.end(), value) != parsed.end() ||
            parsed.size() >= maxFavorites) {
            ok = false;
            break;
        }
        parsed.push_back(value);
    }
    if (std::ferror(file)) ok = false;
    std::fclose(file);
    if (ok) result.swap(parsed);
    return ok;
}

bool replaceFile(const std::string& temporary, const std::string& destination) {
    if (std::rename(temporary.c_str(), destination.c_str()) == 0) return true;
    FILE* existing = std::fopen(destination.c_str(), "rb");
    if (!existing) {
        std::remove(temporary.c_str());
        return false;
    }
    std::fclose(existing);
    const std::string backup = destination + ".bak";
    if (std::remove(backup.c_str()) != 0 && errno != ENOENT) {
        std::remove(temporary.c_str());
        return false;
    }
    if (std::rename(destination.c_str(), backup.c_str()) != 0) {
        std::remove(temporary.c_str());
        return false;
    }
    if (std::rename(temporary.c_str(), destination.c_str()) == 0) {
        std::remove(backup.c_str());
        return true;
    }
    std::rename(backup.c_str(), destination.c_str());
    std::remove(temporary.c_str());
    return false;
}
}

bool Favorites::validGamePath(const std::string& path) {
    if (path.empty() || path.size() > maxPathLength || !allowedLocation(path) ||
        path.find_first_of("\r\n\t") != std::string::npos || path.find("//") != std::string::npos)
        return false;
    size_t start = 1;
    while (start < path.size()) {
        size_t end = path.find('/', start);
        if (end == std::string::npos) end = path.size();
        const std::string component = path.substr(start, end - start);
        if (component.empty() || component == "." || component == "..") return false;
        start = end + 1;
    }
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) return false;
    std::string extension = path.substr(dot);
    for (size_t i = 0; i < extension.size(); ++i)
        extension[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(extension[i])));
    return extension == ".jar" || extension == ".jad";
}

bool Favorites::load() {
    paths_.clear();
    if (parseFile(file_, paths_)) return true;
    if (parseFile(file_ + ".bak", paths_)) return true;
    return missing(file_) && missing(file_ + ".bak");
}

bool Favorites::contains(const std::string& path) const {
    return std::find(paths_.begin(), paths_.end(), path) != paths_.end();
}

bool Favorites::save() const {
    if (file_.empty() || paths_.size() > maxFavorites) return false;
    const std::string temporary = file_ + ".tmp";
    FILE* file = std::fopen(temporary.c_str(), "wb");
    if (!file) return false;
    bool ok = std::fwrite(header, 1, sizeof(header) - 1, file) == sizeof(header) - 1;
    for (size_t i = 0; ok && i < paths_.size(); ++i) {
        if (!validGamePath(paths_[i]) || paths_[i].find('\n') != std::string::npos ||
            std::fprintf(file, "%s\n", paths_[i].c_str()) < 0) ok = false;
    }
    if (std::fflush(file) != 0) ok = false;
    if (std::fclose(file) != 0) ok = false;
    if (!ok) {
        std::remove(temporary.c_str());
        return false;
    }
    return replaceFile(temporary, file_);
}

bool Favorites::toggle(const std::string& path, bool& favorite) {
    if (!validGamePath(path)) return false;
    const std::vector<std::string> previous = paths_;
    std::vector<std::string>::iterator found = std::find(paths_.begin(), paths_.end(), path);
    if (found == paths_.end()) {
        if (paths_.size() >= maxFavorites) return false;
        paths_.push_back(path);
        std::sort(paths_.begin(), paths_.end());
        favorite = true;
    } else {
        paths_.erase(found);
        favorite = false;
    }
    if (save()) return true;
    paths_ = previous;
    return false;
}
}
