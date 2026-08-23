#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>

namespace GsPP
{
    enum class LangueDiagnostic
    {
        Francais,
        Anglais
    };

    class ErreurCompilation final : public std::runtime_error
    {
    public:
        ErreurCompilation(
            std::string messageFrancais,
            std::string messageAnglais,
            std::size_t ligne,
            std::size_t colonne,
            std::string fichier = {})
            : std::runtime_error(messageFrancais),
              _MessageFrancais(std::move(messageFrancais)),
              _MessageAnglais(std::move(messageAnglais)),
              _Ligne(ligne),
              _Colonne(colonne),
              _Fichier(std::move(fichier))
        {
        }

        [[nodiscard]] const std::string& Message(LangueDiagnostic langue) const noexcept
        {
            return langue == LangueDiagnostic::Francais
                ? _MessageFrancais
                : _MessageAnglais;
        }

        [[nodiscard]] std::size_t Ligne() const noexcept { return _Ligne; }
        [[nodiscard]] std::size_t Colonne() const noexcept { return _Colonne; }
        [[nodiscard]] const std::string& Fichier() const noexcept { return _Fichier; }

    private:
        std::string _MessageFrancais;
        std::string _MessageAnglais;
        std::size_t _Ligne;
        std::size_t _Colonne;
        std::string _Fichier;
    };
}
