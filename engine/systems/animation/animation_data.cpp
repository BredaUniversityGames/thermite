#include "animation_data.hpp"

using namespace tmt;

glm::quat direction_to_quaternion(const glm::vec3& direction, const glm::vec3& up = glm::vec3(0, 1, 0)) {
    glm::vec3 fwd = glm::normalize(direction);
    glm::vec3 right = glm::normalize(glm::cross(up, fwd));
    glm::vec3 newUp = glm::cross(fwd, right);
    glm::mat3 rotationMatrix(right, newUp, fwd);
    return glm::quat_cast(rotationMatrix);
}

// helper functions from chatGPT
std::vector<float> get_float_data_from_accessor(const tinygltf::Accessor& accessor, const tinygltf::Model& model) {
    const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[buffer_view.buffer];
    const float* data = reinterpret_cast<const float*>(&buffer.data[accessor.byteOffset + buffer_view.byteOffset]);
    return std::vector<float>(data, data + accessor.count);
}
std::vector<glm::vec3> get_vec3_data_from_accessor(const tinygltf::Accessor& accessor, const tinygltf::Model& model) {
    const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[buffer_view.buffer];
    const glm::vec3* data = reinterpret_cast<const glm::vec3*>(&buffer.data[accessor.byteOffset + buffer_view.byteOffset]);
    return std::vector<glm::vec3>(data, data + accessor.count);
}
std::vector<glm::quat> get_quat_data_from_accessor(const tinygltf::Accessor& accessor, const tinygltf::Model& model) {
    const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[buffer_view.buffer];
    const glm::quat* data = reinterpret_cast<const glm::quat*>(&buffer.data[accessor.byteOffset + buffer_view.byteOffset]);
    std::vector<glm::quat> quats(accessor.count);
    for (int i = 0; i < accessor.count; ++i) {
        quats[i].x = data[i].w;
        quats[i].y = data[i].x;
        quats[i].z = data[i].y;
        quats[i].w = data[i].z;
    }
    return quats;
}
// --------------------------------

void RigData::get_bone_entity(const tinygltf::Node& node, int& index) {
    for (int i = 0; i < bones.size(); i++) {
        if (bones[i].name == node.name) {
            index = i;
            return;
        }
    }
}

void RigData::extract_bone_keyframes(const tinygltf::Model& model) {
    for (const auto& animation : model.animations) {
        // add animation to every bone involved in animation exactly once
        std::vector<int> added;
        for (int i = 0; i < animation.channels.size(); i++) {
            bool skip = false;
            for (int j = 0; j < added.size(); j++) {
                if (animation.channels[i].target_node == added[j]) skip = true;
            }
            if (skip) continue;
            added.push_back(animation.channels[i].target_node);
            int index = 0;
            get_bone_entity(model.nodes[animation.channels[i].target_node], index);
            auto& bone_comp = bones[index];
            bone_comp.animations[animation.name] = Animation();
            bone_comp.animations[animation.name].name = animation.name;
        }
        for (const auto& channel : animation.channels) {
            const auto& target_node = model.nodes[channel.target_node];
            int index = 0;
            get_bone_entity(target_node, index);
            const auto& sampler = animation.samplers[channel.sampler];

            // Get input keyframe times
            const tinygltf::Accessor& input_accessor = model.accessors[sampler.input];
            const std::vector<float> keyframe_times = get_float_data_from_accessor(input_accessor, model);

            const tinygltf::Accessor& output_accessor = model.accessors[sampler.output];
            if (channel.target_path == "translation") {
                std::vector<glm::vec3> translations = get_vec3_data_from_accessor(output_accessor, model);
                for (size_t i = 0; i < keyframe_times.size(); ++i) {
                    bones[index].animations[animation.name].keyframes_pos.push_back({keyframe_times[i], (translations[i])});
                }
            } else if (channel.target_path == "scale") {
                std::vector<glm::vec3> scales = get_vec3_data_from_accessor(output_accessor, model);
                for (size_t i = 0; i < keyframe_times.size(); ++i) {
                    bones[index].animations[animation.name].keyframes_scale.push_back({keyframe_times[i], scales[i]});
                }
            } else if (channel.target_path == "rotation") {
                std::vector<glm::quat> rotations = get_quat_data_from_accessor(output_accessor, model);
                for (size_t i = 0; i < keyframe_times.size(); ++i) {
                    bones[index].animations[animation.name].keyframes_rot.push_back({keyframe_times[i], rotations[i]});
                }
            }
        }
    }
}

void RigData::init_bone(const glm::vec3& o, const glm::quat& q, const tinygltf::Node& n, const tinygltf::Model& model) {
    float length = 0.0f;
    if (!n.translation.empty()) length = (float)n.translation[1];
    glm::quat rot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    if (!n.rotation.empty())
        rot = glm::quat((float)n.rotation[3], (float)n.rotation[0], (float)n.rotation[1], (float)n.rotation[2]);
    glm::vec3 to = q * (rot * (glm::vec3(0, 1, 0) * length));

    Bone bone;
    bone.name = n.name;
    for (int i = 0; i < model.nodes.size(); i++) {
        if (model.nodes[i].name == n.name) {
            bone.index = i;
            break;
        }
    }

    if (!n.children.empty())
        bone.length = (float)model.nodes[n.children[0]].translation[1];
    else
        bone.length = 1.0f;

    glm::vec3 tmp {};
    if (!n.translation.empty()) tmp = glm::vec3((float)n.translation[0], (float)n.translation[1], (float)n.translation[2]);

    // might not need to be world pos/rot, check this if things break
    bone.default_trans.set_world_position(tmp);
    bone.default_trans.set_world_rotation(rot);

    bone.world_pos = o;

    for (int i = 0; i < n.children.size(); i++) {
        bone.children.push_back(n.children[i]); 
        init_bone(o + tmp, q * rot,
            model.nodes[n.children[i]], model
        );
    }
    bones.push_back(bone);
}

RigData::RigData(const IO::FileLocation& _directory) : Resource(_directory) {
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err;
    std::string warn;
    std::string base_dir;


    // Load the GLTF file (.glb)
    auto rig = IO::read_file(_directory);
    bool ret = loader.LoadBinaryFromMemory(&model, &err, &warn, (unsigned char*)rig.data(), (uint32_t)rig.size(), base_dir);

    if (!ret) {
        printf("Failed to load the GLTF model.");
    }

    name = model.nodes[model.scenes[model.defaultScene].nodes[0]].name;

    init_bone(
        glm::vec3(0, 0, 0), glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
        model.nodes[model.nodes[model.scenes[model.defaultScene].nodes[0]].children[0]], model
    );

    for (const auto& node : model.nodes) {
        if (node.mesh == -1) continue;
        glm::vec3 mesh_world_pos = {0, 0, 0};
        glm::quat mesh_world_rot = {0, 0, 0, 0};

        if (!node.translation.empty()) {
            glm::vec3 mesh_translation =
                glm::vec3((float)node.translation[0], (float)node.translation[1], (float)node.translation[2]);
            mesh_world_pos = mesh_translation;
        }

        if (!node.rotation.empty()) {
            glm::quat mesh_rotation =
                glm::quat((float)node.rotation[3], (float)node.rotation[0], (float)node.rotation[1], (float)node.rotation[2]);
            mesh_world_rot = mesh_rotation;
        }

        auto index = model.meshes[node.mesh].primitives[0].attributes["POSITION"];
        glm::vec3 min = {
            model.accessors[index].minValues[0], model.accessors[index].minValues[1], model.accessors[index].minValues[2]
        };
        glm::vec3 max = {
            model.accessors[index].maxValues[0], model.accessors[index].maxValues[1], model.accessors[index].maxValues[2]
        };
        
        glm::vec3 extent = max - min;
        glm::vec3 center_offset = extent * 0.5f;
        glm::vec3 center = min + center_offset;

        for (auto& bone : bones) {
            if (bone.name == node.name) {
                glm::vec3 bone_translation = bone.world_pos;
                bone.mesh_offset = glm::vec3(center.x, -center.z, center.y);
                       
                bone.mesh_rot = mesh_world_rot;
                break;
            }
        }
    }

    for (int i = 0; i < bones.size(); i++) {
        if (bones[i].children.empty()) continue;
        for (int j = 0; j < bones[i].children.size(); j++) {
            for (int k = 0; k < bones.size(); k++) {
                if (bones[i].children[j] == bones[k].index) {
                    bones[i].children[j] = k;
                }
            }
        }
    }

    extract_bone_keyframes(model);
}

bool tmt::RigData::load() { return true; }

void tmt::RigData::unload() {}

bool tmt::RigData::reload() {
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err;
    std::string warn;
    std::string base_dir;

    // Load the GLTF file (.glb)
    auto rig = IO::read_file(file_location);
    bool ret = loader.LoadBinaryFromMemory(&model, &err, &warn, (unsigned char*)rig.data(), (uint32_t)rig.size(), base_dir);

    if (!ret) {
        printf("Failed to load the GLTF model.");
    }

    name = model.nodes[model.scenes[model.defaultScene].nodes[0]].name;

    bones.clear();

    init_bone(
        glm::vec3(0, 0, 0), glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
        model.nodes[model.nodes[model.scenes[model.defaultScene].nodes[0]].children[0]], model
    );

    for (int i = 0; i < bones.size(); i++) {
        if (bones[i].children.empty()) continue;
        for (int j = 0; j < bones[i].children.size(); j++) {
            for (int k = 0; k < bones.size(); k++) {
                if (bones[i].children[j] == bones[k].index) {
                    bones[i].children[j] = k;
                }
            }
        }
    }

    extract_bone_keyframes(model);
    return true;
}