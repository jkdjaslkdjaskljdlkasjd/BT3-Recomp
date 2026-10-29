# Standalone build of the grabprobe diagnostic mod. It only needs the runtime's public mod header.
#   cmake -S mods-src/grabprobe -B build/grabprobe -DCMAKE_BUILD_TYPE=Release
#   cmake --build build/grabprobe --config Release
# then copy grabprobe.dll (Windows) / grabprobe.so (Linux) into the game's mods/ folder.
cmake_minimum_required(VERSION 3.16)
project(grabprobe C)
add_library(grabprobe SHARED grabprobe.c)
target_include_directories(grabprobe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../ps2xRuntime/include)
set_target_properties(grabprobe PROPERTIES PREFIX "" C_VISIBILITY_PRESET hidden)
