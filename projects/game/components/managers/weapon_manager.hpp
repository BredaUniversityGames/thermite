#pragma once
#include "engine/systems/gameplay/game_component.hpp"

#include "projects/game/data_headers/events.hpp"
namespace game {

enum class WeaponType { RIFLE, GRAVITY, MINING };
class WeaponManager : public tmt::GameComponent<WeaponManager> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "WeaponManager"; }

    WeaponType get_active_weapon() const { return current_weapon; }

    void switch_to(WeaponType weapon_slot);
    void set_new_weapon(game::WeaponType weapon_slot);
    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;
    void subscribe_weapon(WeaponType slot);
    void unsubscribe_weapon(WeaponType slot);
    void draw_debug_lines() const override;

    struct WeaponProcAnimData
    {
        tmt::Entity root = entt::null;
        float start_offset = 0.f;
        float anti_overlap_check_length = 0.5f;
        float anti_overlap_scan_extent = 0.1f;
        float length_to_shoulder = 0.1f;
        glm::vec3 shoulder_eulers{};
    };
    struct WeaponEntry
    {
        WeaponProcAnimData proc_anim_data;
        tmt::Entity entity = entt::null;
    };

    std::unordered_map<WeaponType, WeaponEntry> weapons;
    WeaponType starting_weapon { WeaponType::RIFLE };

    tmt::Entity shooting_entity;

    float overheat_time = .8f;
    float switching_time = .5f;

   private:
    void complete_switch();
    float overheat_remaining_time = -0.1f;
    float switching_remaining_time = -0.1f;
    bool switching = false;
    /// <summary>
    /// Handles the weapon overheat event when a weapon is fired.
    /// </summary>
    /// <param name="event">The weapon fired event containing information about the fired weapon.</param>
    void on_overheat(const WeaponFiredEvent& event);
    void check_trigger_shoot_event();
    void transition_to_other_weapons();
    WeaponType current_weapon;
    // weapon that is being switched to by the player
    WeaponType pending_weapon;

    // Weapon procanim values
    glm::vec3 target_pos;
    struct RotTrans
    {
        glm::vec3 translation;
        glm::quat rotation;
    };
    std::unordered_map<WeaponType, RotTrans> rest_poses;
    RotTrans animate_antioverlap();
};

}  // namespace game
TMT_OBJECT(game::WeaponManager::WeaponProcAnimData, (root, start_offset, anti_overlap_check_length, anti_overlap_scan_extent, length_to_shoulder, shoulder_eulers));
TMT_OBJECT(game::WeaponManager::WeaponEntry, (proc_anim_data, entity));
TMT_OBJECT(game::WeaponManager, (weapons, starting_weapon, shooting_entity, overheat_time, switching_time));
