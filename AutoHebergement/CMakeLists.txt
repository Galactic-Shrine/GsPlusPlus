set(GS_AUTO_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/ClassificateurMotsCles")
set(GS_AUTO_REPERTOIRE "${GS_REPERTOIRE_ARTEFACTS}/GsPlusPlus/AutoHebergement")
set(GS_CLASSIFICATEUR_OBJET
    "${GS_AUTO_REPERTOIRE}/ClassificateurMotsCles.GsObj")
set(GS_CLASSIFICATEUR_GSE
    "${GS_AUTO_REPERTOIRE}/ClassificateurMotsCles.GsE")
set(GS_LEXEUR_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/Lexeur")
set(GS_LEXEUR_OBJET "${GS_AUTO_REPERTOIRE}/Lexeur.GsObj")
set(GS_LEXEUR_GSE "${GS_AUTO_REPERTOIRE}/Lexeur.GsE")
set(GS_ANALYSEUR_DECLARATIONS_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/AnalyseurDeclarations")
set(GS_ANALYSEUR_DECLARATIONS_OBJET
    "${GS_AUTO_REPERTOIRE}/AnalyseurDeclarations.GsObj")
set(GS_ANALYSEUR_DECLARATIONS_GSE
    "${GS_AUTO_REPERTOIRE}/AnalyseurDeclarations.GsE")

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
    OUTPUT "${GS_CLASSIFICATEUR_GSE}"
    COMMAND $<TARGET_FILE:gsppc>
        "${GS_CLASSIFICATEUR_OBJET}" "${GS_HEBERGEE_GSA}"
        --format gse
        --point-entree Gs::Autohebergement::ClassifierMotCle
        --nom "Classificateur Gs++"
        --version-application 0.27.0-alpha.6
        -o "${GS_CLASSIFICATEUR_GSE}"
    DEPENDS
        gsppc
        "${GS_CLASSIFICATEUR_OBJET}"
        bibliotheque_hebergee
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
    OUTPUT "${GS_LEXEUR_GSE}"
    COMMAND $<TARGET_FILE:gsppc>
        "${GS_LEXEUR_OBJET}" "${GS_HEBERGEE_GSA}"
        --format gse
        --point-entree Gs::Autohebergement::AnalyserSource
        --nom "Lexeur auto-hébergé Gs++"
        --version-application 0.27.0-alpha.6
        -o "${GS_LEXEUR_GSE}"
    DEPENDS
        gsppc
        "${GS_LEXEUR_OBJET}"
        bibliotheque_hebergee
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
    OUTPUT "${GS_ANALYSEUR_DECLARATIONS_GSE}"
    COMMAND $<TARGET_FILE:gsppc>
        "${GS_ANALYSEUR_DECLARATIONS_OBJET}" "${GS_HEBERGEE_GSA}"
        --format gse
        --point-entree Gs::Autohebergement::AnalyserDeclarationsSource
        --nom "Analyseur de déclarations auto-hébergé Gs++"
        --version-application 0.27.0-alpha.6
        -o "${GS_ANALYSEUR_DECLARATIONS_GSE}"
    DEPENDS
        gsppc
        "${GS_ANALYSEUR_DECLARATIONS_OBJET}"
        bibliotheque_hebergee
    VERBATIM
)

add_custom_target(
    autohebergement_images
    DEPENDS
        "${GS_CLASSIFICATEUR_GSE}"
        "${GS_LEXEUR_GSE}"
        "${GS_ANALYSEUR_DECLARATIONS_GSE}"
)
add_dependencies(autohebergement_images bibliotheque_hebergee)
set(GS_CLASSIFICATEUR_GSE "${GS_CLASSIFICATEUR_GSE}" CACHE INTERNAL "")
set(GS_LEXEUR_GSE "${GS_LEXEUR_GSE}" CACHE INTERNAL "")
set(GS_ANALYSEUR_DECLARATIONS_GSE
    "${GS_ANALYSEUR_DECLARATIONS_GSE}" CACHE INTERNAL "")
