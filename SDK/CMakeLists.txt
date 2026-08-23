add_library(gspp_sdk INTERFACE)
target_include_directories(
    gspp_sdk
    INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
