// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <cstdio>
#include <cstring>
#include <string>

namespace ps3me {
// Portuguese (Brazil) is the default; existing controls.cfg is left untouched.
enum MenuLanguage { MENU_PT_BR = 0, MENU_EN = 1 };
inline MenuLanguage& menuLanguage() {
    static MenuLanguage language = MENU_PT_BR;
    return language;
}
inline const char* tr(const char* pt, const char* en) {
    return menuLanguage() == MENU_EN ? en : pt;
}
inline const char* languageName() {
    return menuLanguage() == MENU_EN ? "English" : "Português (Brasil)";
}
inline const char* languageConfigPath() {
    return "/dev_hdd0/game/PS3ME0001/USRDIR/config/language.cfg";
}
inline bool readLanguage(const std::string& path, MenuLanguage& language) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    char magic[32] = {}, code[12] = {}, extra[2] = {};
    bool ok = std::fscanf(f, "%31s %11s %1s", magic, code, extra) == 2 &&
              std::strcmp(magic, "PS3ME_LANGUAGE_1") == 0;
    std::fclose(f);
    if (!ok) return false;
    if (!std::strcmp(code, "en")) language = MENU_EN;
    else if (!std::strcmp(code, "pt-BR")) language = MENU_PT_BR;
    else return false;
    return true;
}
inline void loadMenuLanguage() {
    MenuLanguage lang = MENU_PT_BR;
    const std::string path(languageConfigPath());
    if (readLanguage(path, lang) || readLanguage(path + ".bak", lang))
        menuLanguage() = lang;
}
inline bool saveMenuLanguage() {
    const std::string path(languageConfigPath()), tmp = path + ".tmp", backup = path + ".bak";
    FILE* f = std::fopen(tmp.c_str(), "wb");
    if (!f) return false;
    const char* code = menuLanguage() == MENU_EN ? "en" : "pt-BR";
    bool ok = std::fprintf(f, "PS3ME_LANGUAGE_1 %s\n", code) > 0;
    if (std::fflush(f)) ok = false;
    if (std::fclose(f)) ok = false;
    if (!ok) { std::remove(tmp.c_str()); return false; }
    if (std::rename(tmp.c_str(), path.c_str()) == 0) return true;
    FILE* old = std::fopen(path.c_str(), "rb");
    if (!old) { std::remove(tmp.c_str()); return false; }
    std::fclose(old);
    // PS3 filesystems may reject rename over an existing file.
    std::remove(backup.c_str());
    if (std::rename(path.c_str(), backup.c_str()) != 0) {
        std::remove(tmp.c_str()); return false;
    }
    if (std::rename(tmp.c_str(), path.c_str()) == 0) return true;
    std::rename(backup.c_str(), path.c_str());
    std::remove(tmp.c_str()); return false;
}
} // namespace ps3me
