#pragma once

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/name.hpp"

#include "tiny_gltf.h"
#include <unordered_map>

#include "engine/core/resource.hpp"
//#include "kudzu/systems/fileio.hpp"

namespace tmt {
struct KeyframePos {
    float time = -1.0f;

    glm::vec3 position = glm::vec3(NAN);
};

struct KeyframeRot {
    float time = -1.0f;

    glm::quat rotation = glm::quat(NAN, 0, 0, 0);
};

struct KeyframeScale {
    float time = -1.0f;

    glm::vec3 scale = glm::vec3(NAN);
};

struct Animation {
    std::string name;
    std::vector<KeyframePos> keyframes_pos;
    std::vector<KeyframeRot> keyframes_rot;
    std::vector<KeyframeScale> keyframes_scale;
};

struct Bone {
    std::string name;
    float length = 0.0f;
    int index = -1;
    std::vector<int> children;
    Transform default_trans;
    glm::vec3 mesh_offset;
    glm::quat mesh_rot;
    glm::vec3 world_pos;
    std::unordered_map<std::string, Animation> animations;
};

struct RigData : public Resource {
   public:
    RigData(const IO::FileLocation& directory);
    
    // Inherited via Resource
    bool load() override;
    void unload() override;
    bool reload() override;

    std::vector<Bone> bones;
    std::string name;

   private:
    void get_bone_entity(const tinygltf::Node& node, int& entity);
    void extract_bone_keyframes(const tinygltf::Model& model);
    void init_bone(const glm::vec3& o, const glm::quat& q, const tinygltf::Node& n, const tinygltf::Model& model);
};
}  // namespace tmt