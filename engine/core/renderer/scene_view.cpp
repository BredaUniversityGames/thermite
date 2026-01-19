#include "scene_view.hpp"

#include <graphite/vram_bank.hh>
#include <graphite/render_graph.hh>

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/renderer/voxel_object.hpp"
#include "engine/core/polyline.hpp"

#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/components/sphere_light.hpp"

namespace tmt {

void SceneView::init() {
    /* Get the VRAM bank */
    VRAMBank& bank = engine.renderer.vram_bank();

    /* Create GPU resources */
    const BufferUsage usage = BufferUsage::Storage | BufferUsage::TransferDst;
    bvh_nodes = bank.create_buffer("BVH Nodes Buffer", usage, MAX_VOXEL_OBJECTS * 2u + 1u, sizeof(AilaLaineNode)).expect("failed to create bvh nodes buffer.");
    object_indices = bank.create_buffer("Object Indices Buffer", usage, MAX_VOXEL_OBJECTS, sizeof(uint32_t)).expect("failed to create object indices buffer.");
    object_data = bank.create_buffer("Object Data Buffer", usage, MAX_VOXEL_OBJECTS, sizeof(GpuVoxelObject)).expect("failed to create object data buffer.");
}

void SceneView::update(RenderGraph& render_graph, const RenderView& render_view) {
    /* Update voxel objects */
    update_voxel_objects(render_graph);

    /* Update lights */
    update_lights(render_graph, render_view);
}

void SceneView::deinit() {
    /* Get the VRAM bank */
    VRAMBank& bank = engine.renderer.vram_bank();

    /* Destroy GPU resources */
    bank.destroy(bvh_nodes);
    bank.destroy(object_indices);
    bank.destroy(object_data);
}

void SceneView::update_voxel_objects(RenderGraph& render_graph) {
    /* Capture all voxel renderers in the scene */
    const entt::basic_group group = engine.ecs.get_registry().group<const VoxelRenderer>(entt::get<Transform>);

    /* Allocate space for all voxel objects */
    std::vector<VoxelObject> cpu_objects {};
    std::vector<GpuVoxelObject> gpu_objects {};
    cpu_objects.reserve(std::min(group.size(), (size_t)MAX_VOXEL_OBJECTS));
    gpu_objects.reserve(std::min(group.size(), (size_t)MAX_VOXEL_OBJECTS));
    entities.clear();
    entities.reserve(std::min(group.size(), (size_t)MAX_VOXEL_OBJECTS));

    /* Iterate over all voxel renderers */
    for (auto&& [entity, renderer, transform] : group.each()) {
        /* Respect the object limit */
        if (cpu_objects.size() >= (size_t)MAX_VOXEL_OBJECTS) break;

        /* Don't render objects with a zero scale or null resource */
        if (glm::any(glm::equal(transform.get_world_scale(), glm::vec3(0.0f))) || renderer.resource == nullptr) continue;

        /* Create new CPU and GPU object */
        VoxelObject& cpu_object = cpu_objects.emplace_back();
        GpuVoxelObject& gpu_object = gpu_objects.emplace_back();

        /* Shared data */
        cpu_object.local_to_world = gpu_object.local_to_world = transform.get_world_matrix();
        cpu_object.world_to_local = gpu_object.world_to_local = glm::inverse(cpu_object.local_to_world);
        cpu_object.size = gpu_object.size = renderer.resource->size;
        cpu_object.rcp_tree_width = gpu_object.rcp_tree_width = 1.0f / powf(4.0f, (float)renderer.resource->blas->depth);

        /* CPU-only data */
        cpu_object.volume = renderer.resource.resource.get();

        /* GPU-only data */
        gpu_object.tree_depth = renderer.resource->blas->depth;
        gpu_object.blas_handle = renderer.resource->blas_nodes.get_index();
        gpu_object.voxels_handle = renderer.resource->blas_voxels.get_index();
        gpu_object.palette_handle = renderer.resource->blas_palette.get_index();

        /* Save the entity id */
        entities.push_back(entity);
    }

    /* Build a BVH over the scene */
    bvh.build(cpu_objects.data(), (uint32_t)cpu_objects.size());

    /* Upload the BVH buffers */
    render_graph.upload_buffer(bvh_nodes, bvh.gpu_nodes, 0u, bvh.node_count * sizeof(AilaLaineNode));
    render_graph.upload_buffer(object_indices, bvh.indices, 0u, bvh.prim_count * sizeof(uint32_t));
    render_graph.upload_buffer(object_data, gpu_objects.data(), 0u, bvh.prim_count * sizeof(GpuVoxelObject));
}

void SceneView::update_lights(RenderGraph& render_graph, const RenderView& render_view) {
    /* Capture all sphere lights in the scene */
    const entt::basic_group group = engine.ecs.get_registry().group<const SphereLight>(entt::get<Transform>);
    
    /* Iterate over all sphere lights */
    for (auto&& [entity, light, transform] : group.each()) {
        engine.polyline.use_line_width(1.5f);

        engine.polyline.use_color(1.0f, 0.62352943f, 0.17254902f);
        /* Center */
        const float distance = glm::distance(glm::vec3(render_view.gpu_view.origin), transform.get_world_position());
        engine.polyline.draw_circle(transform.get_world_position(), distance * 0.01f, 4u);

        engine.polyline.use_color(0.4f, 0.4f, 0.4f);
        /* Source radius */
        engine.polyline.draw_circle(transform.get_world_position(), light.source_radius, 32u);
        /* Attenuation radius */
        engine.polyline.draw_circle(transform.get_world_position(), light.attenuation_radius, 32u);
    }

    (void)render_graph;
}

}  // namespace tmt
