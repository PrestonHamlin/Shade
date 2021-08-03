# The DirectX Shader Compiler (DXC) is used for compiling HLSL using the recent
#   shader models. It is an open-source project by Microsoft, based on a fork of
#   the LLVM source. As such, it is a sizable repo. For simplicity, we will just
#   grab an archive of release binaries off of GitHub.

# DXC origin and local file paths
set(DXC_ORIGIN_FILE https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.6.2106/dxc_2021_07_01.zip)
set(DXC_ORIGIN_FILE_MD5 7f9d593e65cec70326c3773eb2b05fa1)
set(DXC_PATH ${PROJECT_SOURCE_DIR}/extern/dxc)
set(DXC_LOCAL_FILE ${DXC_PATH}/dxc.zip)

# ensure that we have a local copy of the latest GitHub release binary archive
set(DXC_UPDATE_CHECK TRUE)  # TODO: convert this to an option
if(${DXC_UPDATE_CHECK})
    set(DXC_SHOULD_UPDATE FALSE)
    message("\nDXC update check enabled")
    if(EXISTS ${DXC_LOCAL_FILE})
        message("Local DXC archive located. Checking version...")
        file(MD5 ${DXC_LOCAL_FILE} DXC_LOCAL_FILE_MD5)
        message("Expected MD5: ${DXC_ORIGIN_FILE_MD5}")
        message("Local MD5:    ${DXC_LOCAL_FILE_MD5}")
        if(NOT ${DXC_ORIGIN_FILE_MD5} STREQUAL ${DXC_LOCAL_FILE_MD5})
            message("Archive MD5 hash does not match expected value!")
            set(DXC_SHOULD_UPDATE TRUE)
        else()
            message("Local DXC archive is up-to-date!")
        endif()
    else()
        message("Local DXC archive not found. Fetching from GitHub...")
        set(DXC_SHOULD_UPDATE TRUE)
    endif()

    # if archive missing or outdated, fetch a new one
    if(DXC_SHOULD_UPDATE)
        message("Fetching ${DXC_ORIGIN_FILE}")
        file(DOWNLOAD ${DXC_ORIGIN_FILE} ${DXC_LOCAL_FILE}
             EXPECTED_MD5 ${DXC_ORIGIN_FILE_MD5}
             TIMEOUT 30
        )
        # extract contents
        message("Extracting ${DXC_LOCAL_FILE}")
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${DXC_LOCAL_FILE}
                        WORKING_DIRECTORY ${DXC_PATH}
        )
    endif()
    message("")
endif()

set(DXC_INCLUDE_DIR ${DXC_PATH}/inc)
set(DXC_BINARY_DIR ${DXC_PATH}/bin/x64)
set(DXC_LIBRARY_DIR ${DXC_PATH}/lib/x64)

include_directories(${DXC_INCLUDE_DIR})
link_directories(${DXC_LIBRARY_DIR})
