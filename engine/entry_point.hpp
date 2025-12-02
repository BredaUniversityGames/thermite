#pragma once
#include "engine/core/application.hpp"
#include "engine/engine.hpp"

extern tmt::Application* create_application(const tmt::CommandLineArgs& args);

int main(int argc, char** argv) {
    tmt::CommandLineArgs args {argc, argv};
    tmt::Application* app = create_application(args);
    const tmt::ApplicationSpecs& specs = app->specs;
    tmt::engine.init(specs, app);

    tmt::engine.run();

    tmt::engine.end();
}