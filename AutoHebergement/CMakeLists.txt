set(GS_AUTO_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/ClassificateurMotsCles")
set(GS_AUTO_REPERTOIRE "${GS_REPERTOIRE_ARTEFACTS}/GsPlusPlus/AutoHebergement")
set(GS_CLASSIFICATEUR_OBJET
    "${GS_AUTO_REPERTOIRE}/ClassificateurMotsCles.GsObj")
set(GS_LEXEUR_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/Lexeur")
set(GS_LEXEUR_OBJET "${GS_AUTO_REPERTOIRE}/Lexeur.GsObj")
set(GS_ANALYSEUR_DECLARATIONS_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/AnalyseurDeclarations")
set(GS_ANALYSEUR_DECLARATIONS_OBJET
    "${GS_AUTO_REPERTOIRE}/AnalyseurDeclarations.GsObj")
set(GS_ANALYSEUR_SEMANTIQUE_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/AnalyseurSemantique")
set(GS_ANALYSEUR_SEMANTIQUE_OBJET
    "${GS_AUTO_REPERTOIRE}/AnalyseurSemantique.GsObj")
set(GS_FRONTEND_GSE "${GS_AUTO_REPERTOIRE}/Frontend.GsE")

add_custom_command(
    OUTPUT "${GS_CLASSIFICATEUR_OBJET}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${GS_AUTO_REPERTOIRE}"
    COMMAND $<TARGET_FILE:gsppc>
        "${GS_HEBERGEE_INTERFACE}"
        "${GS_AUTO_SOURCE}/ClassificateurMotsCles.HGsPP"
        "${GS_AUTO_SOURCE}/ClassificateurMotsCles.GsPP"
        --format gsobj -o "${GS_CLASSIFICATEUR_OBJET}"
    DEPENDS
        gsppc
        "${GS_HEBERGEE_INTERFACE}"
        "${GS_AUTO_SOURCE}/ClassificateurMotsCles.HGsPP"
        "${GS_AUTO_SOURCE}/ClassificateurMotsCles.GsPP"
    VERBATIM
)

add_custom_command(
    OUTPUT "${GS_LEXEUR_OBJET}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${GS_AUTO_REPERTOIRE}"
    COMMAND $<TARGET_FILE:gsppc>
        "${GS_HEBERGEE_INTERFACE}"
        "${GS_AUTO_SOURCE}/ClassificateurMotsCles.HGsPP"
        "${GS_LEXEUR_SOURCE}/Lexeur.HGsPP"
        "${GS_AUTO_SOURCE}/ClassificateurMotsCles.GsPP"
        "${GS_LEXEUR_SOURCE}/Lexeur.GsPP"
        --format gsobj -o "${GS_LEXEUR_OBJET}"
    DEPENDS
        gsppc
        "${GS_HEBERGEE_INTERFACE}"
        "${GS_AUTO_SOURCE}/ClassificateurMotsCles.HGsPP"
        "${GS_LEXEUR_SOURCE}/Lexeur.HGsPP"
        "${GS_AUTO_SOURCE}/ClassificateurMotsCles.GsPP"
        "${GS_LEXEUR_SOURCE}/Lexeur.GsPP"
    VERBATIM
)

add_custom_command(
    OUTPUT "${GS_ANALYSEUR_DECLARATIONS_OBJET}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${GS_AUTO_REPERTOIRE}"
    COMMAND $<TARGET_FILE:gsppc>
        "${GS_HEBERGEE_INTERFACE}"
        "${GS_AUTO_SOURCE}/ClassificateurMotsCles.HGsPP"
        "${GS_LEXEUR_SOURCE}/Lexeur.HGsPP"
        "${GS_ANALYSEUR_DECLARATIONS_SOURCE}/AnalyseurDeclarations.HGsPP"
        "${GS_AUTO_SOURCE}/ClassificateurMotsCles.GsPP"
        "${GS_LEXEUR_SOURCE}/Lexeur.GsPP"
        "${GS_ANALYSEUR_DECLARATIONS_SOURCE}/AnalyseurDeclarations.GsPP"
        --format gsobj -o "${GS_ANALYSEUR_DECLARATIONS_OBJET}"
    DEPENDS
        gsppc
        "${GS_HEBERGEE_INTERFACE}"
        "${GS_AUTO_SOURCE}/ClassificateurMotsCles.HGsPP"
        "${GS_LEXEUR_SOURCE}/Lexeur.HGsPP"
        "${GS_ANALYSEUR_DECLARATIONS_SOURCE}/AnalyseurDeclarations.HGsPP"
        "${GS_AUTO_SOURCE}/ClassificateurMotsCles.GsPP"
        "${GS_LEXEUR_SOURCE}/Lexeur.GsPP"
        "${GS_ANALYSEUR_DECLARATIONS_SOURCE}/AnalyseurDeclarations.GsPP"
    VERBATIM
)

add_custom_command(
    OUTPUT "${GS_ANALYSEUR_SEMANTIQUE_OBJET}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${GS_AUTO_REPERTOIRE}"
    COMMAND $<TARGET_FILE:gsppc>
        "${GS_HEBERGEE_INTERFACE}"
        "${GS_LEXEUR_SOURCE}/Lexeur.HGsPP"
        "${GS_ANALYSEUR_DECLARATIONS_SOURCE}/AnalyseurDeclarations.HGsPP"
        "${GS_ANALYSEUR_SEMANTIQUE_SOURCE}/AnalyseurSemantique.HGsPP"
        "${GS_ANALYSEUR_SEMANTIQUE_SOURCE}/AnalyseurSemantique.GsPP"
        --format gsobj -o "${GS_ANALYSEUR_SEMANTIQUE_OBJET}"
    DEPENDS
        gsppc
        "${GS_HEBERGEE_INTERFACE}"
        "${GS_LEXEUR_SOURCE}/Lexeur.HGsPP"
        "${GS_ANALYSEUR_DECLARATIONS_SOURCE}/AnalyseurDeclarations.HGsPP"
        "${GS_ANALYSEUR_SEMANTIQUE_SOURCE}/AnalyseurSemantique.HGsPP"
        "${GS_ANALYSEUR_SEMANTIQUE_SOURCE}/AnalyseurSemantique.GsPP"
    VERBATIM
)

add_custom_target(
    autohebergement_objets
    DEPENDS
        "${GS_CLASSIFICATEUR_OBJET}"
        "${GS_LEXEUR_OBJET}"
        "${GS_ANALYSEUR_DECLARATIONS_OBJET}"
        "${GS_ANALYSEUR_SEMANTIQUE_OBJET}"
)

add_custom_command(
    OUTPUT "${GS_FRONTEND_GSE}"
    COMMAND $<TARGET_FILE:gsppc>
        "${GS_ANALYSEUR_DECLARATIONS_OBJET}"
        "${GS_ANALYSEUR_SEMANTIQUE_OBJET}"
        "${GS_HEBERGEE_GSA}"
        --format gse
        --point-entree
            GalacticShrine::GsPP::Autohebergement::AnalyserDeclarationsSource
        --nom "Frontend auto-hébergé Gs++"
        --version-application "${GS_VERSION_PRODUIT}"
        -o "${GS_FRONTEND_GSE}"
    DEPENDS
        gsppc
        autohebergement_objets
        "${GS_ANALYSEUR_DECLARATIONS_OBJET}"
        "${GS_ANALYSEUR_SEMANTIQUE_OBJET}"
        bibliotheque_hebergee
    VERBATIM
)

add_custom_target(autohebergement_frontend DEPENDS "${GS_FRONTEND_GSE}")
add_dependencies(
    autohebergement_frontend
    autohebergement_objets
    bibliotheque_hebergee
)

add_custom_target(
    autohebergement_images
    DEPENDS "${GS_FRONTEND_GSE}"
)
add_dependencies(autohebergement_images autohebergement_frontend)
set(GS_FRONTEND_GSE "${GS_FRONTEND_GSE}" CACHE INTERNAL "")
