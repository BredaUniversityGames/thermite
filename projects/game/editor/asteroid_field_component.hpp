#include <ImReflect.hpp>
#include "projects/game/components/asteroid_field_component.hpp"

inline void tag_invoke(ImReflect::ImInput_t, const char* name, AsteroidFieldComponent& value, ImSettings& settings, ImResponse& response) {
	ImReflect::Detail::imgui_input_visit_field(name, value, settings, response);

	auto& type_resp = response.get<AsteroidFieldComponent>();

	if(type_resp.is_changed())
	{
		value.clear_children();
		value.spawn_field();
	}
}