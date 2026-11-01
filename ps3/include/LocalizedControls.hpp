// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "Controls.hpp"
#include "MenuLanguage.hpp"
namespace ps3me {
inline const char* localizedButton(int b) {
    if (menuLanguage() == ENGLISH) return buttonName(b);
    switch (b) {
        case UP: return "Cima"; case DOWN: return "Baixo";
        case LEFT: return "Esquerda"; case RIGHT: return "Direita";
        default: return buttonName(b);
    }
}
inline const char* localizedAction(int a) {
    if (menuLanguage() == ENGLISH) return actionName(a);
    switch (a) {
        case NONE: return "Desativado"; case KEY_UP: return "Cima";
        case KEY_DOWN: return "Baixo"; case KEY_LEFT: return "Esquerda";
        case KEY_RIGHT: return "Direita"; case FIRE: return "Ação / Centro";
        case SOFT1: return "Tecla esquerda"; case SOFT2: return "Tecla direita";
        case TOUCH: return "Toque / segurar";
        default: return actionName(a);
    }
}
} // namespace ps3me
