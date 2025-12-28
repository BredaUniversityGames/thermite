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
    render_graph.add_compute_pass("macro-facets pass", "macrofacets.cs")
        .read(render_view.render_view_buffer)
        /* Visibility buffer */
        .read(render_view.vbuffer.image)
        /* Cache buffers */
        .read(render_view.cache_info)
        .write(render_view.cache)
        /* Macrofacets */
        .write(render_view.macrofacets)
        .group_size(16, 8)
        .work_size(render_res.x, render_res.y);
        
    /* DEBUGGING */
    render_graph.add_compute_pass("[debug] occupancy pass", "debug/occupancy.cs")
        /* Visibility buffer */
        .read(render_view.vbuffer.image)
        /* Cache buffers */
        .read(render_view.cache_info)
        .write(render_view.cache)
        /* Render target */
        .write(render_image)
        .group_size(16, 8)
        .work_size(render_res.x, render_res.y);
        
    render_graph.add_compute_pass("di pass", "di.cs")
        /* Object buffers */
        .read(scene_view.bvh_nodes)
        .read(scene_view.object_indices)
        .read(scene_view.object_data)
        /* Macrofacets */
        .write(render_view.macrofacets)
        /* Cache buffers */
        .read(render_view.cache_info)
        .write(render_view.cache)
        .group_size(128)
        .indirect_size(render_view.macrofacets);
}

/* clang-format on */

}  // namespace tmt
