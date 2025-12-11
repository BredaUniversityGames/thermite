#include "navigation_system.hpp"

std::string tmt::NavigationSystem::get_name() { return std::string(); }

void tmt::NavigationSystem::on_start() { 
	nav_mesh.generate_mesh({tmt::IO::Location::PROJECT, "dragon128.vengi"}); }

void tmt::NavigationSystem::on_update(const FrameData& time) { nav_mesh.inspect(); }

void tmt::NavigationSystem::on_end() {}