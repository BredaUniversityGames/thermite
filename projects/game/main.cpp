#include "engine/entry_point.hpp"
#include "engine/core/logger.hpp"

class Game : public tmt::Application {
   public:
    Game(const tmt::ApplicationSpecs& specs) : Application(specs) {}

    void on_start() override;
    void on_update(const tmt::FrameData& time) override;
    void on_end() override;
};

tmt::Application* create_application(const tmt::CommandLineArgs& args) {
    // clang-format off
    tmt::ApplicationSpecs specs {
        .name = "Example Game",
        .command_args = args,
        .log_file = "example_game_logs.txt"
    };
    // clang-format on

    return new Game(specs);
}

void Game::on_start() {}

void Game::on_update(const tmt::FrameData& time) {}

void Game::on_end() {}
