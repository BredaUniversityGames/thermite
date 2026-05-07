#pragma once

#include <ImReflect.hpp>

#include "engine/engine.hpp"
#include "engine/systems/physics/components/voxel_connection.hpp"

void tag_invoke(ImReflect::ImInput_t, const char*, tmt::VoxelConnection& connection, ImSettings& settings, ImResponse& response);