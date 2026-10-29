// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "hal/IGameStorage.hpp"
#include <cstdio>
#include <string>
#include <vector>

namespace ps3me {
class Storage : public ps2::hal::IGameStorage {
public:
    explicit Storage(const std::vector<std::string>& roots);
    ~Storage();
    int list();
    const char* nameAt(int index);
    int openAt(int index);
    int read(void* dst, int length);
    int readAt(unsigned int offset, void* dst, int length);
    void close();
private:
    struct Entry { std::string name, path; };
    std::vector<std::string> roots_;
    std::vector<Entry> entries_;
    FILE* file_;
};

// Stores the existing RMS blob verbatim; no pointer or endian conversion here.
// Keys are provided by the suite manager and must be stable across boots.
class SaveStore {
public:
    explicit SaveStore(const std::string& root) : root_(root) {}
    bool write(const std::string& key, const void* data, unsigned int length);
    int read(const std::string& key, void* data, unsigned int capacity) const;
    static bool validKey(const std::string& key);
private:
    std::string root_;
};
}
