add_executable(gspp_tests Unitaires/Tests.cpp)
target_link_libraries(gspp_tests PRIVATE gspp_compiler)

add_executable(
    gspp_autohebergement_tests
    AutoHebergement/AutoHebergement.cpp
)
target_link_libraries(gspp_autohebergement_tests PRIVATE gspp_compiler)

if(MSVC)
    target_compile_options(gspp_tests PRIVATE /W4 /permissive- /utf-8)
    target_compile_options(
        gspp_autohebergement_tests PRIVATE /W4 /permissive- /utf-8)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
       AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
    target_compile_options(
        gspp_autohebergement_tests PRIVATE -maccumulate-outgoing-args)
endif()

set(GS_TEST_HEBERGE_REPERTOIRE
    "${GS_REPERTOIRE_ARTEFACTS}/GsPlusPlus/Tests/AutoHebergement")
set(GS_TEST_HEBERGE_OBJET
    "${GS_TEST_HEBERGE_REPERTOIRE}/TestHebergee.GsObj")
set(GS_TEST_HEBERGE_GSE
    "${GS_TEST_HEBERGE_REPERTOIRE}/TestHebergee.GsE")

add_custom_command(
    OUTPUT "${GS_TEST_HEBERGE_OBJET}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory
        "${GS_TEST_HEBERGE_REPERTOIRE}"
    COMMAND $<TARGET_FILE:gsppc>
        "${GS_HEBERGEE_INTERFACE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/AutoHebergement/BibliothequeHebergee.GsPP"
        --format gsobj -o "${GS_TEST_HEBERGE_OBJET}"
    DEPENDS
        gsppc
        "${GS_HEBERGEE_INTERFACE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/AutoHebergement/BibliothequeHebergee.GsPP"
    VERBATIM
)

add_custom_command(
    OUTPUT "${GS_TEST_HEBERGE_GSE}"
    COMMAND $<TARGET_FILE:gsppc>
        "${GS_TEST_HEBERGE_OBJET}" "${GS_HEBERGEE_GSA}"
        --format gse
        --point-entree TesterBibliothequeHebergee
        --nom "Test bibliothèque hébergée"
        --version-application 0.27.0-alpha.8
        -o "${GS_TEST_HEBERGE_GSE}"
    DEPENDS
        gsppc
        "${GS_TEST_HEBERGE_OBJET}"
        bibliotheque_hebergee
    VERBATIM
)

add_custom_target(
    gspp_images_tests
    DEPENDS
        "${GS_FRONTEND_GSE}"
        "${GS_TEST_HEBERGE_GSE}"
)
add_dependencies(gspp_images_tests autohebergement_images bibliotheque_hebergee)
add_dependencies(gspp_autohebergement_tests gspp_images_tests)

add_test(NAME gspp_tests COMMAND gspp_tests)
add_test(
    NAME gspp_autohebergement_tests
    COMMAND gspp_autohebergement_tests
        "${GS_FRONTEND_GSE}"
        "${GS_TEST_HEBERGE_GSE}"
)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(
    NAME gspp_style_sources
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/Conformite/verifier_style_sources.py"
        --source-root "${GS_RACINE_GSPP}"
)
set_tests_properties(
    gspp_style_sources PROPERTIES
    LABELS "GsPlusPlus;Conformite;Style"
)

set(GS_RAPPORT_CONFORMITE
    "${GS_REPERTOIRE_TESTS}/GsPlusPlus/Conformite/rapport.json")
add_test(
    NAME gspp_conformite
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/Conformite/executer_conformite.py"
        --source-root "${GS_RACINE_GSPP}"
        --build-root "${CMAKE_BINARY_DIR}"
        --compiler $<TARGET_FILE:gsppc>
        --verifier $<TARGET_FILE:gseverifier>
        --loader $<TARGET_FILE:gsechargeur>
        --hosted-library "${GS_HEBERGEE_GSA}"
        --system-library "${GS_SYSTEME_GSA}"
        --hosted-test "${GS_TEST_HEBERGE_GSE}"
        --report "${GS_RAPPORT_CONFORMITE}"
)
set_tests_properties(
    gspp_conformite PROPERTIES
    LABELS "GsPlusPlus;Conformite"
)

if(UNIX AND NOT MSVC)
    find_program(GS_BASH bash REQUIRED)
    add_test(
        NAME gspp_integration
        COMMAND "${GS_BASH}"
            "${CMAKE_CURRENT_SOURCE_DIR}/Integration/IntegrationGsPlusPlus.sh"
            "${GS_RACINE_GSPP}"
            "${CMAKE_BINARY_DIR}"
            $<TARGET_FILE:gsppc>
            $<TARGET_FILE:gseverifier>
            $<TARGET_FILE:gsechargeur>
            "${GS_SYSTEME_GSA}"
            "${GS_HEBERGEE_GSA}"
            "${GS_FRONTEND_GSE}"
            "${GS_TEST_HEBERGE_GSE}"
    )
else()
    message(STATUS
        "Test d'intégration Gs++ GNU ignoré : utiliser WSL/Linux pour ce test.")
endif()

add_custom_target(tests_gspp_preparation)
add_dependencies(
    tests_gspp_preparation
    gspp_tests
    gspp_autohebergement_tests
    gseverifier
    gsechargeur
    bibliotheque_systeme
    bibliotheque_hebergee
    gspp_images_tests
)
