#pragma once

#include <cstddef>
#include <string>

namespace GsPP
{
    enum class GenreJeton
    {
        Fin,
        Identifiant,
        NombreEntier,
        ChaineCaracteres,

        Espace,
        Structure,
        Union,
        Enumeration,
        Alias,
        Externe,
        Publique,
        Privee,
        Constante,
        Volatile,
        Entier8,
        Entier16,
        Entier32,
        Entier64,
        Naturel8,
        Naturel16,
        Naturel32,
        Naturel64,
        Booleen,
        Octet,
        Caractere,
        Vide,
        PointeurFonction,
        Convertir,
        Retourner,
        Si,
        Sinon,
        TantQue,
        Vrai,
        Faux,

        ParentheseOuvrante,
        ParentheseFermante,
        AccoladeOuvrante,
        AccoladeFermante,
        CrochetOuvrant,
        CrochetFermant,
        PointVirgule,
        Virgule,
        Point,
        Fleche,
        DeuxPointsDouble,

        Egal,
        EgalEgal,
        Different,
        Inferieur,
        InferieurEgal,
        Superieur,
        SuperieurEgal,
        PointExclamation,
        EtLogique,
        OuLogique,
        Esperluette,
        BarreVerticale,
        Circonflexe,
        Tilde,
        DecalageGauche,
        DecalageDroite,
        Plus,
        Moins,
        Etoile,
        BarreOblique,
        Pourcentage,

        // Les jetons ajoutés en 0.18 restent en fin d’énumération afin de
        // préserver les identifiants numériques du classificateur 0.17.
        Classe,
        Protegee,
        Virtuel,
        Constructeur,
        Destructeur,
        Operateur,
        Soi,
        DeuxPoints,

        // Ajouté en 0.19, toujours en fin d’énumération pour conserver les
        // identifiants numériques utilisés par l’auto-hébergement.
        Remplacer,

        // Ajouté en 0.20 sans renuméroter les jetons antérieurs.
        Parent
    };

    struct Jeton
    {
        GenreJeton Genre;
        std::string Texte;
        std::size_t Ligne;
        std::size_t Colonne;
    };

    [[nodiscard]] const char* NomGenreJeton(GenreJeton genre) noexcept;
}
