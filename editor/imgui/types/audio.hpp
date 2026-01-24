#pragma once

#include <ImReflect.hpp>

namespace tmt {

	class AudioEvent;
	class VolumeControl;

}

void tag_invoke(ImReflect::ImInput_t, const char* label, tmt::AudioEvent& value, ImSettings& settings, ImResponse& response);

void tag_invoke(ImReflect::ImInput_t, const char* label, tmt::VolumeControl& value, ImSettings& settings, ImResponse& response);