# FILEPATH: /C:/Users/seanf/Desktop/projects/game/src/CMakeLists.txt

# Add the executable target
add_executable(mapbuilder main.c)

# Set the Win32 subsystem for the executable
if(WIN32)
    set_target_properties(mapbuilder PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
endif()
