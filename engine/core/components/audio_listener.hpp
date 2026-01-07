#pragma once

#include "engine/core/reflection.hpp"

namespace tmt {

class AudioListener {
   public:
    [[nodiscard]] int get_listener_index() const { return listener_index; }

    void set_weight(float weight) const;
    [[nodiscard]] float get_weight() const;

   private:
    BEFRIEND_VISITABLE()
    friend class Audio;

    int listener_index {-1};
};

}  // namespace tmt

TMT_COMPONENT(tmt::AudioListener, "Audio Listener", (listener_index));