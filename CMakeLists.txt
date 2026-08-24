cmake_minimum_required(VERSION 3.20)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(
        GsPlusPlus
        VERSION 0.27.0
        LANGUAGES CXX
    )

    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)

    set(GS_REPERTOIRE_BINAIRES "${CMAKE_BINARY_DIR}/Bin")
    set(GS_REPERTOIRE_ARTEFACTS "${CMAKE_BINARY_DIR}/Artefacts")
    set(GS_REPERTOIRE_TESTS "${CMAKE_BINARY_DIR}/Tests")

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${GS_REPERTOIRE_BINAIRES}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/BibliothequesNatives")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/BibliothequesNatives")

    foreach(GS_CONFIGURATION Debug Release RelWithDebInfo MinSizeRel)
        string(TOUPPER "${GS_CONFIGURATION}" GS_CONFIGURATION_MAJUSCULE)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${GS_CONFIGURATION_MAJUSCULE}
            "${GS_REPERTOIRE_BINAIRES}")
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${GS_CONFIGURATION_MAJUSCULE}
            "${CMAKE_BINARY_DIR}/BibliothequesNatives")
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${GS_CONFIGURATION_MAJUSCULE}
            "${CMAKE_BINARY_DIR}/BibliothequesNatives")
    endforeach()

    include(CTest)
endif()

set(GS_RACINE_GSPP "${CMAKE_CURRENT_SOURCE_DIR}")
include(GNUInstallDirs)

# Ces valeurs de repli permettent aussi l'intégration comme sous-projet d'un
# autre consommateur qui ne définit pas les répertoires du monorepo historique.
if(NOT DEFINED GS_REPERTOIRE_BINAIRES)
    set(GS_REPERTOIRE_BINAIRES "${CMAKE_BINARY_DIR}/Bin")
endif()
if(NOT DEFINED GS_REPERTOIRE_ARTEFACTS)
    set(GS_REPERTOIRE_ARTEFACTS "${CMAKE_BINARY_DIR}/Artefacts")
endif()
if(NOT DEFINED GS_REPERTOIRE_TESTS)
    set(GS_REPERTOIRE_TESTS "${CMAKE_BINARY_DIR}/Tests")
endif()

add_subdirectory(SDK)
add_subdirectory(Compiler)
add_subdirectory(Bibliotheques)
add_subdirectory(AutoHebergement)

if(BUILD_TESTING)
    add_subdirectory(Tests)
endif()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    add_custom_target(compilateurs)
    add_dependencies(compilateurs gsppc gseverifier gsechargeur gseload)

    add_custom_target(bibliotheques_gspp)
    add_dependencies(
        bibliotheques_gspp
        bibliotheque_systeme
        bibliotheque_hebergee
    )

    if(BUILD_TESTING)
        add_custom_target(preparer_tests)
        add_dependencies(preparer_tests tests_gspp_preparation)
    endif()

    add_custom_target(espace_travail)
    add_dependencies(
        espace_travail
        compilateurs
        bibliotheques_gspp
        autohebergement_images
    )
    if(BUILD_TESTING)
        add_dependencies(espace_travail preparer_tests)
    endif()

    install(
        TARGETS gsppc gseverifier gsechargeur gseload
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
    install(
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/SDK/include/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    )
    install(
        FILES "${GS_SYSTEME_GSA}" "${GS_HEBERGEE_GSA}"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/GsPlusPlus"
    )
    install(
        FILES
            "${GS_CLASSIFICATEUR_GSE}"
            "${GS_LEXEUR_GSE}"
            "${GS_ANALYSEUR_DECLARATIONS_GSE}"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/GsPlusPlus/AutoHebergement"
    )
    install(
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Bibliotheques/"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/GsPlusPlus/Bibliotheques"
        FILES_MATCHING
        PATTERN "*.Gs++"
        PATTERN "*.GsPP"
        PATTERN "*.GsPlusPlus"
        PATTERN "*.HGs++"
        PATTERN "*.HGsPP"
        PATTERN "*.HeaderGsPlusPlus"
        PATTERN "*.GsPj"
        PATTERN "*.GsProject"
    )
    install(
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Exemples/"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/GsPlusPlus/Exemples"
        FILES_MATCHING
        PATTERN "*.Gs++"
        PATTERN "*.GsPP"
        PATTERN "*.GsPlusPlus"
        PATTERN "*.HGs++"
        PATTERN "*.HGsPP"
        PATTERN "*.HeaderGsPlusPlus"
        PATTERN "*.GsPj"
        PATTERN "*.GsProject"
        PATTERN "*.GsPs"
    )
    install(
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Documentation/"
        DESTINATION "${CMAKE_INSTALL_DOCDIR}/Documentation"
        FILES_MATCHING PATTERN "*.md"
    )
    install(
        FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
            "${CMAKE_CURRENT_SOURCE_DIR}/README.en.md"
            "${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG.md"
            "${CMAKE_CURRENT_SOURCE_DIR}/RELEASE_NOTES_0.27.0-alpha.6.md"
            "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
            "${CMAKE_CURRENT_SOURCE_DIR}/VERSION"
        DESTINATION "${CMAKE_INSTALL_DOCDIR}"
    )
    install(
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Gs++.png"
        DESTINATION "${CMAKE_INSTALL_DOCDIR}/Assets"
    )

    set(CPACK_PACKAGE_NAME "GsPlusPlus")
    set(CPACK_PACKAGE_VENDOR "Galactic-Shrine")
    set(CPACK_PACKAGE_VERSION "0.27.0-alpha.6")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
        "Compilateur, SDK et bibliothèques autonomes Gs++")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    set(CPACK_PACKAGE_FILE_NAME
        "GsPlusPlus-0.27.0-alpha.6-${CMAKE_SYSTEM_NAME}-x86_64")
    set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
    if(WIN32)
        set(CPACK_GENERATOR "ZIP")
    else()
        set(CPACK_GENERATOR "TGZ")
    endif()
    include(CPack)
endif()
