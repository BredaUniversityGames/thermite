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

void DiPipeline::enqueue(RenderGraph& render_graph, RenderView& render_view, SceneView&) {
    /* Get Render Image */
    const BindHandle render_image = render_view.get_render_image();
    const glm::uvec2 render_res = render_view.gpu_view.resolution;
    
    /* Macrofacet reset pass */
    render_graph.add_compute_pass("macrofacet reset pass", "macrofacet_reset.cs")
        .write(render_view.macrofacet_hashset) /* Macrofacet hash set buffer */
        .write(render_view.macrofacet_shading_commands) /* Shading commands buffer */
        .group_size(128)
        .work_size(render_res.x * render_res.y);

    /* Macrofacet dispatch pass */
    render_graph.add_compute_pass("macrofacet dispatch pass", "macrofacet_dispatch.cs")
        .read(render_view.render_view_buffer) /* Render view buffer */
        .read(render_view.vbuffer.image) /* Visibility buffer */
        .write(render_view.macrofacet_hashset) /* Macrofacet hash set buffer */
        .write(render_view.macrofacet_shading_commands) /* Shading commands buffer */
        .group_size(16, 8)
        .work_size(render_res.x, render_res.y);
        
    /* DEBUGGING */
    render_graph.add_compute_pass("[debug] occupancy pass", "debug/occupancy.cs")
        .read(render_view.render_view_buffer) /* Render view buffer */
        .read(render_view.vbuffer.image) /* Visibility buffer */
        .read(render_view.macrofacet_hashset) /* Macrofacet hash set buffer */
        .write(render_image) /* Render target */
        .group_size(16, 8)
        .work_size(render_res.x, render_res.y);
        
    // render_graph.add_compute_pass("di pass", "di.cs")
    //     /* Object buffers */
    //     .read(scene_view.bvh_nodes)
    //     .read(scene_view.object_indices)
    //     .read(scene_view.object_data)
    //     /* Macrofacets */
    //     .write(render_view.macrofacets)
    //     /* Cache buffers */
    //     .read(render_view.cache_info)
    //     .write(render_view.cache)
    //     .group_size(128)
    //     .indirect_size(render_view.macrofacets);
}

/* clang-format on */

}  // namespace tmt
