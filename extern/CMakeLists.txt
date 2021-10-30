# CMake file for configuring library submodules

# configure and add assimp library
option(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
option(ASSIMP_BUILD_TESTS OFF)
option(ASSIMP_INSTALL OFF)
option(ASSIMP_INSTALL_PDB OFF)
add_subdirectory(assimp)
set_target_properties(assimp PROPERTIES FOLDER "Libraries")
set_target_properties(zlibstatic PROPERTIES FOLDER "Libraries")
set_target_properties(UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTIES FOLDER "Libraries")

# FMT is the precursor to std::format in C++20
add_subdirectory(fmt)
set_target_properties(fmt PROPERTIES FOLDER "Libraries")

