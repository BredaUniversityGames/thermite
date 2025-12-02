#pragma once

#include <cstdint> /* uint32_t, int16_t, etc */
#include <cmath>   /* fminf, fmaxf, powf, etc */
#include <utility> /* std::swap */

#include <glm/glm.hpp> /* vector math */

/* Large 32-bit floating point constant. */
constexpr float BIG_F32 = 1e30f;

/* Number of voxels per unit in world-space. */
constexpr uint32_t VOXELS_PER_UNIT = 10u;
/* Number of world-space units per voxel. */
constexpr float UNITS_PER_VOXEL = 1.0f / VOXELS_PER_UNIT;
