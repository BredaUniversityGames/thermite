function(find_and_add_targets)

	set(PROJECTS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/projects/")

	file(GLOB PROJECT_DIRS RELATIVE "${PROJECTS_ROOT}" "${PROJECTS_ROOT}/*")

	foreach(project ${PROJECT_DIRS})
		set(project_dir "${PROJECTS_ROOT}/${project}")
		if (IS_DIRECTORY "${project_dir}" AND EXISTS "${project_dir}/main.cpp")
			# use folder name as target name
			set(target_name "${project}")
	
			message(STATUS "Auto-adding project target: ${target_name}")
		
		# get all code
		file(GLOB_RECURSE PROJECT_SOURCES
				${project_dir}/*.cpp
				${project_dir}/*.hpp
		)
		
		# define executable
			add_executable(${target_name}
				${PROJECT_SOURCES}
			)
		
		# link with ThermiteEngine
			target_link_libraries(${target_name} PRIVATE ThermiteEngine)
		
		# add some build state macros for code
			if (THERMITE_EDITOR_BUILD)
				target_compile_definitions(${target_name} PRIVATE THERMITE_EDITOR=1)
			endif()
	
			if (THERMITE_DEBUG_BUILD)
				target_compile_definitions(${target_name} PRIVATE THERMITE_DEBUG=1)
			endif()
		endif()
	endforeach()

	
endfunction()