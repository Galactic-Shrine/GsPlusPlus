add_library(gspp_sdk INTERFACE)
target_include_directories(
    gspp_sdk
    INTERFACE
        "${GS_REPERTOIRE_ENTETES_GENERES}"
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
