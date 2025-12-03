#include "camera_system.hpp"

#include "engine.hpp"
#include "core/input.hpp"
#include "core/logger.hpp"
#include "core/components/camera.hpp"
#include "core/components/transform.hpp"

namespace tmt {
void CameraSystem::on_start() { 
    // Setup Input Actions
    engine.input.add_action(M_SPRINT, Key::LEFT_SHIFT);
    engine.input.add_action(M_FORWARD, Key::W);
    engine.input.add_action(M_BACKWARD, Key::S);
    engine.input.add_action(M_RIGHT, Key::D);
    engine.input.add_action(M_LEFT, Key::A);
    engine.input.add_action(M_UP, Key::E);
    engine.input.add_action(M_DOWN, Key::Q);
}

void CameraSystem::on_update(const FrameData& time) {
	//bool enableMouseLook = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

    auto& input = engine.input;
    Entity camera_entity = Camera::get_active_camera();
    if (camera_entity == entt::null) Log::error(Log::Scope::ENGINE, "Camera Entity is NULL. Are there any active cameras in the scene?");
    Camera& camera = engine.ecs.get_component<Camera>(camera_entity);
    Transform& transform = engine.ecs.get_component<Transform>(camera_entity);

	// Movement (WASD + QE)
    float speed = 4.0f;
    if (input.is_action_pressed(M_SPRINT)) speed *= 3.0f;

    glm::vec3 moveDir = {0.0f, 0.0f, 0.0f};
    if (input.is_action_pressed(M_FORWARD)) moveDir += transform.get_forward();
    if (input.is_action_pressed(M_BACKWARD)) moveDir -= transform.get_forward();
    if (input.is_action_pressed(M_LEFT)) moveDir -= transform.get_right();
    if (input.is_action_pressed(M_RIGHT)) moveDir += transform.get_right();
    if (input.is_action_pressed(M_UP)) moveDir += transform.get_up();
    if (input.is_action_pressed(M_DOWN)) moveDir -= transform.get_up();

    glm::vec3 pos = transform.get_world_position();
    if (glm::length(moveDir) > 0.0f) pos += glm::normalize(moveDir) * speed * time.delta_time;

    transform.set_world_position(pos);
}

void CameraSystem::on_end() {}
}  // namespace tmt
