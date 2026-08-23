set(GSPP_COMPILER_SOURCES
    src/Lexeur.cpp
    src/Compilation.cpp
    src/AnalyseurSyntaxique.cpp
    src/AnalyseurSemantique.cpp
    src/GenerateurX64.cpp
    src/EcrivainCoff.cpp
    src/ObjetGsO.cpp
    src/BibliothequeGsA.cpp
    src/EditeurLiens.cpp
    src/ConstructeurProjet.cpp
    src/EcrivainGsE.cpp
    src/VerificateurGsE.cpp
    src/ChargeurGsE.cpp
)

add_library(gspp_compiler ${GSPP_COMPILER_SOURCES})
target_include_directories(
    gspp_compiler
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries(gspp_compiler PUBLIC gspp_sdk)

function(gspp_appliquer_avertissements cible)
    if(MSVC)
        target_compile_options(${cible} PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(${cible} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

gspp_appliquer_avertissements(gspp_compiler)

add_executable(gsppc src/Principal.cpp)
target_link_libraries(gsppc PRIVATE gspp_compiler)
gspp_appliquer_avertissements(gsppc)

add_executable(gseverifier src/VerifierGsEPrincipal.cpp)
target_link_libraries(gseverifier PRIVATE gspp_compiler)
gspp_appliquer_avertissements(gseverifier)

add_executable(gsechargeur src/ChargeurGsEPrincipal.cpp)
target_link_libraries(gsechargeur PRIVATE gspp_compiler)
gspp_appliquer_avertissements(gsechargeur)

add_executable(gseload src/ChargeurGsEPrincipal.cpp)
target_link_libraries(gseload PRIVATE gspp_compiler)
gspp_appliquer_avertissements(gseload)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
   AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
    target_compile_options(gsechargeur PRIVATE -maccumulate-outgoing-args)
    target_compile_options(gseload PRIVATE -maccumulate-outgoing-args)
endif()
