#include "player.hpp"
#include "engine/core/input/input.hpp"

void Player::start() {
    tmt::engine.input.lock_mouse(true);
    tmt::engine.input.set_mouse_relative_to_window(true);
}

void Player::update(const tmt::FrameData& time) {
    // Update code for the player component
}

void Player::end() {
    // Cleanup code for the player component
}