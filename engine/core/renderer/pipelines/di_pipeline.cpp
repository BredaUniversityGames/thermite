#include "di_pipeline.hpp"

#include <graphite/render_graph.hh>
#include <graphite/nodes/compute_node.hh>

#include "engine/engine.hpp"
#include "engine/core/renderer/render_view.hpp"
#include "engine/core/renderer/scene_view.hpp"
#include "engine/core/renderer/voxel_object.hpp"
#include "engine/core/renderer/renderer.hpp"

namespace tmt {
    
/* clang-format off */

void DiPipeline::enqueue(RenderGraph& render_graph, RenderView& render_view, SceneView& scene_view) {
    /* Get Render Image */
    const BindHandle render_image = render_view.get_render_image();

    /* Enqueue the direct illumination compute pass */
    const glm::uvec2 render_res = render_view.gpu_view.resolution;
    render_graph.add_compute_pass("di pass", "di.cs")
        /* Render view */
        .read(render_view.render_view_buffer)
        /* Object buffers */
        .read(scene_view.bvh_nodes)
        .read(scene_view.object_indices)
        .read(scene_view.object_data)
        /* Visibility buffer */
        .read(render_view.vbuffer.image)
        /* TODO: Use Illuminance buffer */
        .write(render_image)
        .group_size(16, 8)
        .work_size(render_res.x, render_res.y);
}

/* clang-format on */

}  // namespace tmt
