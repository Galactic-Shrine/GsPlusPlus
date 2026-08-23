#pragma once

#include "GsPP/Jeton.hpp"

#include <string_view>
#include <vector>

namespace GsPP
{
    [[nodiscard]] GenreJeton ClassifierMotCle(
        std::string_view texte);

    class Lexeur final
    {
    public:
        explicit Lexeur(std::string_view source, std::string fichier = {});

        [[nodiscard]] std::vector<Jeton> Analyser();

    private:
        [[nodiscard]] bool EstFin() const noexcept;
        [[nodiscard]] char Courant() const noexcept;
        [[nodiscard]] char Suivant() const noexcept;
        char Avancer();
        void IgnorerSeparations();
        [[nodiscard]] Jeton LireNombre();
        [[nodiscard]] Jeton LireIdentifiant();
        [[nodiscard]] Jeton LireChaine();
        [[nodiscard]] Jeton LireSymbole();
        [[nodiscard]] bool CommenceIdentifiant(unsigned char octet) const noexcept;
        [[nodiscard]] bool ContinueIdentifiant(unsigned char octet) const noexcept;

        std::string_view _Source;
        std::size_t _Position = 0;
        std::size_t _Ligne = 1;
        std::size_t _Colonne = 1;
        std::string _Fichier;
    };
}
