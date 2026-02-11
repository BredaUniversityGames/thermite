#pragma once

#include "editor.hpp"
#include "editor/core/systems/undo_redo/undo_redo_manager.hpp"

#include <engine/core/ecs.hpp>
#include <engine/core/renderer/material.hpp>

namespace tmt {

class VoxelEditDiff : public IUndoRedo {
   public:
    VoxelEditDiff(const UUID& node_uuid) : node_uuid { node_uuid } {}

    // Add a change to a voxel, if voxel is/was empty, use `nullptr`.
    void add_change(const glm::uvec3& coord, const MaterialIndex* before_material, const MaterialIndex* material_after = nullptr);
    void commit(const std::string& message);

    // Inherited via IUndoRedo
    void undo() override;
    void redo() override;
    void inspect() override {}

   private:
    struct VoxelCoord {
        uint32_t x : 10 { 0 };
        uint32_t y : 10 { 0 };
        uint32_t z : 10 { 0 };
        uint32_t is_empty : 2 { false };  // Bool is uint32_t type to avoid padding in the struct.
    };

    struct VoxelChange {
        std::vector<VoxelCoord> positions;
        std::vector<MaterialIndex> indices;  // Only stores the indices of the voxels that aren't empty.
    };

    UUID node_uuid { NULL_UUID };

    VoxelChange before_change;
    VoxelChange after_change;
};

}  // namespace tmt