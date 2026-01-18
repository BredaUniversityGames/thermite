#pragma once
#include "engine/tools/component_registry.hpp"

/* All components */
#include "engine/core/components/transform.hpp"
#include "engine/core/components/name.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/component_collection.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/systems/animation/components/constraints.hpp"
#include "engine/systems/animation/components/bone_hierarchy_renderer.hpp"
#include "engine/systems/ai/navigation/nav_mesh.hpp"
#include "engine/core/components/audio_listener.hpp"
#include "engine/core/components/audio_emitter.hpp"

namespace tmt {

// clang-format off
using SerializeComponents = ComponentRegistry<
	/* Order in which components are serialized */
	Name, 
	Transform, 
	VoxelRenderer, 
	Camera,
	VoxelBody,
	ComponentCollection,
	NavMesh,
	AnimConstraints::DampedTransformConstraint,
	BoneHierarchyRenderer,
	AudioListener,
	AudioEmitter
>;
// clang-format on

// clang-format off
using InspectComponents = ComponentRegistry<
	/* Order in which components are displayed in the editor */
	Name, 
	Transform,
	VoxelRenderer, 
	Camera,
	VoxelBody,
	NavMesh,
	AnimConstraints::DampedTransformConstraint,
	BoneHierarchyRenderer,
	AudioListener,
	AudioEmitter
>;
// clang-format on

}  // namespace tmt