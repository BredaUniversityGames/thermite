#pragma once

#include <graphite/resources/handle.hh>

#include "core/renderer/render_view.hpp"

#include "engine/shared/bvh2.hpp"
#include "engine/shared/svt64.hpp"

class GPUAdapter;
class RenderGraph;

namespace tmt {

struct VoxelObject;

/* Maximum number of voxel objects we support in the scene at once. */
constexpr uint32_t MAX_VOXEL_OBJECTS = 10000u;
/* We have a hard limit due to technical limitations. */
static_assert(MAX_VOXEL_OBJECTS < (1u << 14));

class GeometryPipeline {
   private:
    /* GPU resources */
    Buffer bvh_nodes {};
    Buffer object_indices {};
    Buffer object_data {};

    /* Acceleration structures */
    Bvh2<VoxelObject> bvh {};

   public:
    GeometryPipeline() {}
    ~GeometryPipeline() {}

    GeometryPipeline(const GeometryPipeline&) = delete;
    GeometryPipeline& operator=(const GeometryPipeline&) = delete;

    void init(GPUAdapter& gpu);
    void enqueue(RenderGraph& render_graph, RenderView render_view);
    void deinit(GPUAdapter& gpu);
};

}  // namespace tmt
