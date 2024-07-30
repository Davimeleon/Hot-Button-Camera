#--------------------------------------------------------------------------------------------------------------------------#
#Project configuration
#--------------------------------------------------------------------------------------------------------------------------#
project(seekcamera-daq DESCRIPTION "Seek Thermal Camera Recording with DAQ Device")

#--------------------------------------------------------------------------------------------------------------------------#
#Executable configuration
#--------------------------------------------------------------------------------------------------------------------------#
add_executable(${PROJECT_NAME}
	src/seekcamera-daq.cpp
)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(MYLIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/cbw64.lib)

add_library(cbw64 STATIC IMPORTED)
set_target_properties(cbw64 PROPERTIES IMPORTED_LOCATION ${MYLIB_PATH})


if(SDL2_FOUND)
	include_directories(${SDL2_EXEC_PREFIX}/include)
endif(SDL2_FOUND)

target_link_libraries(${PROJECT_NAME}
	seekcamera
	SDL2
	cbw64
)

if(SDL2_FOUND)
	target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})
endif()

if(UNIX)
	target_link_libraries(${PROJECT_NAME} m)
endif()

#--------------------------------------------------------------------------------------------------------------------------#
#Runtime dependencies
#--------------------------------------------------------------------------------------------------------------------------#
if(MSVC)
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		$<TARGET_FILE_DIR:seekcamera>/seekcamera.dll
		$<TARGET_FILE_DIR:${PROJECT_NAME}>/seekcamera.dll
	)
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		$<TARGET_FILE_DIR:SDL2>/SDL2.dll
		$<TARGET_FILE_DIR:${PROJECT_NAME}>/SDL2.dll
	)
endif()

#--------------------------------------------------------------------------------------------------------------------------#
#Install
#--------------------------------------------------------------------------------------------------------------------------#
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
