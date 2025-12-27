#include "ecs.hpp"
#include "engine/tools/serializer.hpp"
#include "engine/core/entity.hpp"

#include "engine/core/components/transform.hpp"

/* ECS */
JsonReflect::json tag_invoke(JsonReflect::serialize_t, const tmt::Ecs& ecs) { 

}

void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::Ecs& ecs) {

}

/* Single entity */
JsonReflect::json tag_invoke(JsonReflect::serialize_t, const tmt::Entity& entity, const tmt::Ecs& ecs) { 

}

void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::Entity& entity, tmt::Ecs& ecs) {

}

/* Set of entities */
JsonReflect::json tag_invoke(JsonReflect::serialize_t, const std::set<tmt::Entity>& entities_original, const tmt::Ecs& ecs) { 
	auto entities = entities_original;
    for (const auto entity : entities_original) {
        //auto children = ecs.get_component<tmt::Transform>().get
	}
	

	JsonReflect::json result;

}

void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, std::set<tmt::Entity>& entities, tmt::Ecs& ecs) {

}
