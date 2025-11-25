#pragma once
#include "event.hpp"
#include "core/time_step.hpp"

namespace tmt {

class OnUpdate : public EventListenerBase<OnUpdate, FrameData> {
   public:
    OnUpdate() : EventListenerBase() {}

    virtual void on_update(const FrameData& time) = 0;

    void on_event(FrameData&) final override { /* Empty */ };
    void on_event(const FrameData& time) final override { on_update(time); }
};

class OnFixedUpdate : public EventListenerBase<OnFixedUpdate, FrameData> {
   public:
    OnFixedUpdate() : EventListenerBase() {}

    virtual void on_fixed_update(const FrameData& time) = 0;

    void on_event(FrameData&) final override { /* Empty */ };
    void on_event(const FrameData& time) final override {
        on_fixed_update(time);
    }
};

}  // namespace tmt