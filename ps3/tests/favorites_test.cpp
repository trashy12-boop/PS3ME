// SPDX-License-Identifier: GPL-2.0-only
#include "Favorites.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <unistd.h>

int main() {
    char directory[] = "/tmp/ps3me-favorites-XXXXXX";
    assert(::mkdtemp(directory));
    const std::string file = std::string(directory) + "/favorites.cfg";
    const std::string game = "/dev_hdd0/game/PS3ME0001/USRDIR/games/Example.jar";
    const std::string usb = "/dev_usb000/Example.jad";
    assert(ps3me::Favorites::validGamePath(game));
    assert(ps3me::Favorites::validGamePath(usb));
    assert(!ps3me::Favorites::validGamePath("/dev_hdd0/game/PS3ME0001/USRDIR/games/../outside.jar"));
    assert(!ps3me::Favorites::validGamePath("/dev_usb002/outside.jar"));

    ps3me::Favorites favorites(file);
    assert(favorites.load());
    bool selected = false;
    assert(favorites.toggle(game, selected) && selected && favorites.contains(game));
    assert(favorites.toggle(usb, selected) && selected && favorites.contains(usb));
    ps3me::Favorites restored(file);
    assert(restored.load());
    assert(restored.paths().size() == 2);
    assert(restored.toggle(game, selected) && !selected && !restored.contains(game));
    ps3me::Favorites afterRemoval(file);
    assert(afterRemoval.load());
    assert(afterRemoval.paths().size() == 1 && afterRemoval.contains(usb));

    {
        std::ofstream backup((file + ".bak").c_str(), std::ios::binary);
        backup << "PS3ME_FAVORITES 1\n" << game << "\n";
    }
    {
        std::ofstream broken(file.c_str(), std::ios::binary | std::ios::trunc);
        broken << "broken data\n";
    }
    ps3me::Favorites recovered(file);
    assert(recovered.load() && recovered.paths().size() == 1 && recovered.contains(game));

    std::remove(file.c_str());
    std::remove((file + ".bak").c_str());
    std::remove((file + ".tmp").c_str());
    ::rmdir(directory);
    return 0;
}
