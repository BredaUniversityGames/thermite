#pragma once
#include "engine/core/audio.hpp"

namespace tmt {

struct MenuSounds {
    tmt::AudioEvent button_hover;        //
    tmt::AudioEvent button_accept;       //
    tmt::AudioEvent insufficient_funds;  //
    tmt::AudioEvent loading;             //
    tmt::AudioEvent money_gained;        //
    tmt::AudioEvent open_menu;           //
    tmt::AudioEvent slider_move;         //
};

}  // namespace tmt

TMT_OBJECT(tmt::MenuSounds, (button_hover, button_accept, insufficient_funds, loading, money_gained, open_menu, slider_move));
