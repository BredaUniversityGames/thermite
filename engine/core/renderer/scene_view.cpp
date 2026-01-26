#include "scene_view.hpp"

#include <graphite/vram_bank.hh>
#include <graphite/render_graph.hh>

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/renderer/voxel_object.hpp"

#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/components/light.hpp"

namespace tmt {

struct GpuLight {
    /* Origin point of the light source. */
    glm::vec3 origin {};
    /* Radius of the light source, defines the softness of its shadows. */
    float source_radius = 0.0f;
    /* Exitent luminance of the light source. (in candela) */
    glm::vec3 luminance {};
    /* Radius at which the emitted light influence reaches zero. */
    float attenuation_radius = 0.0f;
};

void SceneView::init() {
    /* Get the VRAM bank */
    VRAMBank& bank = engine.renderer.vram_bank();

    /* Create GPU resources */
    const BufferUsage s = BufferUsage::Storage | BufferUsage::TransferDst;
    const BufferUsage c = BufferUsage::Constant | BufferUsage::TransferDst;
    bvh_nodes = bank.create_buffer("BVH Nodes Buffer", s, MAX_VOXEL_OBJECTS * 2u + 1u, sizeof(AilaLaineNode)).expect("failed to create bvh nodes buffer.");
    object_indices = bank.create_buffer("Object Indices Buffer", s, MAX_VOXEL_OBJECTS, sizeof(uint32_t)).expect("failed to create object indices buffer.");
    object_data = bank.create_buffer("Object Data Buffer", s, MAX_VOXEL_OBJECTS, sizeof(GpuVoxelObject)).expect("failed to create object data buffer.");
    lights_data = bank.create_buffer("Lights Data Buffer", s, MAX_LIGHTS, sizeof(GpuLight)).expect("failed to create lights data buffer.");
    scene_view = bank.create_buffer("Scene View Buffer", c, sizeof(GpuSceneView)).expect("failed to create scene view buffer.");
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
    bank.destroy(lights_data);
    bank.destroy(scene_view);
}

/* Create a vector containing type `T`, with space reserved for `count` instances. */
template <typename T>
inline std::vector<T> reserved(const size_t count) {
    std::vector<T> v {};
    v.reserve(count);
    return std::move(v);
}

void SceneView::update_voxel_objects(RenderGraph& render_graph) {
    /* Capture all voxel renderers in the scene */
    const entt::basic_group group = engine.ecs.get_registry().group<const VoxelRenderer>(entt::get<Transform>);

    /* Allocate space for all voxel objects */
    const size_t count = std::min(group.size(), (size_t)MAX_VOXEL_OBJECTS);
    std::vector cpu_objects = reserved<VoxelObject>(count);
    std::vector gpu_objects = reserved<GpuVoxelObject>(count);
    entities = reserved<Entity>(count);

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
    /* Capture all lights in the scene */
    const entt::basic_group group = engine.ecs.get_registry().group<const Light>(entt::get<Transform>);
    
    /* Allocate space for all lights */
    const size_t count = std::min(group.size(), (size_t)MAX_LIGHTS);
    std::vector gpu_lights = reserved<GpuLight>(count);
    
    /* Iterate over all lights */
    for (auto&& [entity, light, transform] : group.each()) {
        /* TEMP: Skip all lights which are not sphere lights. */
        if (light.type != LightType::SPHERE_LIGHT) continue;

        GpuLight& gpu_light = gpu_lights.emplace_back();
        const SphereLight sphere_light = std::get<SphereLight>(light.light);

        gpu_light.origin = transform.get_world_position();
        gpu_light.source_radius = sphere_light.source_radius;
        gpu_light.luminance = light.color * sphere_light.luminous_flux / (glm::pi<float>() * 4.0f);
        gpu_light.attenuation_radius = sphere_light.attenuation_radius;
    }

    GpuSceneView gpu_view {};
    gpu_view.light_count = (uint32_t)gpu_lights.size();

    /* Upload the light buffers */
    render_graph.upload_buffer(lights_data, gpu_lights.data(), 0u, gpu_lights.size() * sizeof(GpuLight));
    render_graph.upload_buffer(scene_view, &gpu_view, 0u, sizeof(GpuSceneView));
}

}  // namespace tmt
