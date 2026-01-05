#include "editor/core/window.hpp"

namespace tmt {
class MotionMathPreview : public IWindow {
   public:
    MotionMathPreview() = default;
    ~MotionMathPreview() override = default;

    void display() override;

    [[nodiscard]] constexpr std::string get_title() const override { return "Motion Math Preview"; }

    void on_editor_start() override {}
    void on_editor_update(const FrameData&) override {}
    void on_editor_end() override {}

   private:
    float frequency = 1.f;
    float damping = 1.f;
    float initial_response = 0.f;
};
}  // namespace tmt