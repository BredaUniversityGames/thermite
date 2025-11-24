#pragma once

#include <string>

namespace tmt {

#if THERMITE_EDITOR
#pragma message(" THERMITE_EDITOR=1 ")
#else
#pragma message(" THERMITE_EDITOR=0 ")
#endif

#if THERMITE_DEBUG
#pragma message("THERMITE_DEBUG=1")
#else
#pragma message("THERMITE_DEBUG=0")
#endif

class Engine {
   public:
    Engine();
    ~Engine();

    /* No copies allowed */
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;


    void init();
    void run();
    void end();
};

/* Singleton */
extern Engine engine;

}  // namespace tmt
