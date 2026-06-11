#include "glm/fwd.hpp"
#include <vector>
#include "engine/core/entity.hpp"
#include <glm/gtx/quaternion.hpp>

namespace tmt {
namespace AnimConstraints {
namespace FABRIKIK {


struct FABRIKSolverInput
{
	std::vector<tmt::Entity> chain_entities;
	std::vector<glm::vec3> poles;
	std::vector<float> angle_strengths;
	glm::vec3 effector;
	glm::quat rig_world_rotation;
	glm::vec3 local_aim;
	glm::vec3 local_up;
};

struct FABRIKSolverOutput
{
	std::vector<glm::quat> chain_rotations;
};

FABRIKSolverOutput solve_fabrik(const FABRIKSolverInput& input);
bool derive_pole_dir(const glm::vec3& a, const glm::vec3& joint, const glm::vec3& b, glm::vec3& out_dir);
}
}
}