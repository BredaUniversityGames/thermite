#include "engine/entry_point.hpp"

class Game : public tmt::Application {
   public:
    Game(const tmt::ApplicationSpecs& specs) : Application(specs) {}

    // Inherited via Application
    void on_start() override {}
    void on_update(const tmt::FrameData& time) override {}
    void on_end() override {}
};

tmt::Application* create_application(const tmt::CommandLineArgs& args) {
    tmt::ApplicationSpecs specs {
        .name = "Example Game", 
        .command_args = args, 
        .log_file = "example_game_logs.txt"
    };

    return new Game(specs);
}