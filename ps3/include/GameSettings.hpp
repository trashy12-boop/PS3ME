// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "Controls.hpp"
#include <string>
namespace ps3me {
class GameSettings {
 ControlConfig global_;std::string base_;
public:
 void begin(const std::string& game,const std::string& root,ControlConfig&,int&,int&);
 void end(ControlConfig& c){c=global_;base_.clear();}
 bool save(const ControlConfig&,int,int)const;
 bool useGlobal(ControlConfig&,int&,int&)const;
 const std::string& base()const{return base_;}
private:int defaultW_,defaultH_;
};
GameSettings& gameSettings();bool overlayActive();int showInGameMenu(bool automated=false);
}
