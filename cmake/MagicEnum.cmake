# Magic Enum provides static reflection for enums

set(MAGICENUM_SOURCE_PATH ${PROJECT_SOURCE_DIR}/extern/magic_enum)
set(MAGICENUM_INCLUDE_DIR ${MAGICENUM_SOURCE_PATH}/include)

# add_subdirectory(magic_enum)
# set_target_properties(magic_enum PROPERTIES FOLDER "Libraries")
# include_directories(magic_enum/include)
include_directories(${MAGICENUM_INCLUDE_DIR})
