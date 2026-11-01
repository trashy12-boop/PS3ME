// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <cstdio>
#include <cstring>
#include <string>

namespace ps3me {
enum MenuLanguage { PT_BR = 0, ENGLISH = 1 };
inline MenuLanguage& menuLanguage() {
    static MenuLanguage current = PT_BR;
    return current;
}
inline const char* tr(const char* portuguese, const char* english) {
    return menuLanguage() == ENGLISH ? english : portuguese;
}
inline const char* languageName() {
    return menuLanguage() == ENGLISH ? "English" : "Português (Brasil)";
}
inline const char* languageFile() {
    return "/dev_hdd0/game/PS3ME0001/USRDIR/config/language.cfg";
}
inline bool parseLanguage(const std::string& path, MenuLanguage& result) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    char magic[32] = {}, code[16] = {}, extra[2] = {};
    bool valid = std::fscanf(f, "%31s %15s %1s", magic, code, extra) == 2
              && std::strcmp(magic, "PS3ME_LANGUAGE_1") == 0;
    std::fclose(f);
    if (!valid) return false;
    if (std::strcmp(code, "pt-BR") == 0) result = PT_BR;
    else if (std::strcmp(code, "en") == 0) result = ENGLISH;
    else return false;
    return true;
}
inline void loadMenuLanguage() {
    MenuLanguage selected = PT_BR;
    const std::string path(languageFile());
    if (parseLanguage(path, selected) || parseLanguage(path + ".bak", selected))
        menuLanguage() = selected;
}
inline bool saveMenuLanguage() {
    const std::string path(languageFile());
    const std::string temp = path + ".tmp", backup = path + ".bak";
    FILE* f = std::fopen(temp.c_str(), "wb");
    if (!f) return false;
    const char* code = menuLanguage() == ENGLISH ? "en" : "pt-BR";
    bool ok = std::fprintf(f, "PS3ME_LANGUAGE_1 %s\n", code) > 0;
    if (std::fflush(f) != 0) ok = false;
    if (std::fclose(f) != 0) ok = false;
    if (!ok) { std::remove(temp.c_str()); return false; }
    if (std::rename(temp.c_str(), path.c_str()) == 0) return true;
    // Existing installations can reject replacing a file directly (CellFs).
    FILE* old = std::fopen(path.c_str(), "rb");
    if (!old) { std::remove(temp.c_str()); return false; }
    std::fclose(old);
    // Keep the valid prior settings if we cannot safely rotate the backup.
    MenuLanguage previous;
    if (!parseLanguage(path, previous)) { std::remove(temp.c_str()); return false; }
    std::remove(backup.c_str());
    if (std::rename(path.c_str(), backup.c_str()) != 0) {
        std::remove(temp.c_str()); return false;
    }
    if (std::rename(temp.c_str(), path.c_str()) == 0) return true;
    std::rename(backup.c_str(), path.c_str());
    std::remove(temp.c_str());
    return false;
}
} // namespace ps3me
