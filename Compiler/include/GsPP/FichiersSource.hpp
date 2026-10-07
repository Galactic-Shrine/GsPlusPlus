#pragma once

#include "GsPP/ErreurCompilation.hpp"

#include <cstdint>
#include <filesystem>
#include <istream>
#include <limits>
#include <system_error>

namespace GsPP
{
    enum class OperationFichierSource
    {
        Identification,
        Statut,
        Ouverture,
        Lecture
    };

    /**
     * <résumé>Erreur hôte de fichier, distincte d'un diagnostic de langue et de ses coordonnées.</résumé>
     * @etc. Conserve opération, chemin physique et code système ; reste interceptable comme runtime_error.
     **/
    class ErreurFichierSource final : public std::runtime_error
    {
    public:
        ErreurFichierSource(OperationFichierSource operation, std::filesystem::path chemin, std::error_code code);
        [[nodiscard]] OperationFichierSource Operation() const noexcept { return _Operation; }
        [[nodiscard]] const std::filesystem::path& Chemin() const noexcept { return _Chemin; }
        [[nodiscard]] const std::error_code& CodeSysteme() const noexcept { return _Code; }
        [[nodiscard]] const std::string& Message(LangueDiagnostic langue) const noexcept;

    private:
        OperationFichierSource _Operation;
        std::filesystem::path _Chemin;
        std::error_code _Code;
        std::string _MessageAnglais;
        std::string _MessageFrancais;
    };

    /** <résumé>Identité canonique commune au bootstrap et aux catalogues ; casse ASCII normalisée sur Windows.</résumé> **/
    std::string IdentifierFichierSource(const std::filesystem::path& chemin);

    /**
     * <résumé>Contrôle le statut sans confondre une absence avec une erreur système.</résumé>
     * @etc. Absence, parent non répertoire et cible non régulière donnent faux ; les autres erreurs lèvent ErreurFichierSource.
     **/
    bool EstFichierSourceRegulier(const std::filesystem::path& chemin);

    /**
     * <résumé>Lit depuis la position du flux jusqu'à EOF, ou refuse sans rendre un préfixe incomplet.</résumé>
     * @etc. Flux emprunté, non fermé ; les échecs de lecture et ios_base::failure sont des erreurs hôtes.
     * @etc. EOF normal accepté, même avec exceptions de flux ; dépasser maximum lève length_error.
     **/
    std::string LireFluxSource(std::istream& flux, const std::filesystem::path& chemin,
        std::uint64_t maximum = std::numeric_limits<std::uint64_t>::max());

    /** <résumé>Ouvre en binaire puis utilise la lecture contrôlée partagée, avec limite facultative.</résumé> **/
    std::string LireFichierSource(const std::filesystem::path& chemin,
        std::uint64_t maximum = std::numeric_limits<std::uint64_t>::max());
}
