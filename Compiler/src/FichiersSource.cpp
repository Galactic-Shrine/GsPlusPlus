#include "GsPP/FichiersSource.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <fstream>
#include <utility>

namespace GsPP
{
    namespace
    {
        std::string MessageErreur(OperationFichierSource operation, const std::filesystem::path& chemin,
            LangueDiagnostic langue)
        {
            const bool anglais = langue == LangueDiagnostic::Anglais;
            const char* message = anglais ? "source file error: " : "erreur de fichier source : ";
            switch (operation)
            {
            case OperationFichierSource::Identification:
                message = anglais ? "cannot identify source file: " : "impossible d'identifier le fichier source : "; break;
            case OperationFichierSource::Statut:
                message = anglais ? "cannot inspect source file: " : "impossible de vérifier le fichier source : "; break;
            case OperationFichierSource::Ouverture:
                message = anglais ? "cannot open source file: " : "impossible d'ouvrir le fichier source : "; break;
            case OperationFichierSource::Lecture:
                message = anglais ? "cannot read source file: " : "lecture du fichier source échouée : "; break;
            }
            const auto utf8 = chemin.generic_u8string();
            return std::string(message) + std::string(utf8.begin(), utf8.end());
        }

        std::error_code CodeLecture(int valeur)
        {
            // iostream ne garantit pas errno ; zéro signifie une erreur d'E/S sans détail système disponible.
            return valeur ? std::error_code(valeur, std::generic_category()) : std::make_error_code(std::errc::io_error);
        }
    }

    ErreurFichierSource::ErreurFichierSource(OperationFichierSource operation, std::filesystem::path chemin, std::error_code code)
        : std::runtime_error(MessageErreur(operation, chemin, LangueDiagnostic::Francais)),
          _Operation(operation), _Chemin(std::move(chemin)), _Code(code),
          _MessageAnglais(MessageErreur(operation, _Chemin, LangueDiagnostic::Anglais)),
          _MessageFrancais(what())
    {
    }

    const std::string& ErreurFichierSource::Message(LangueDiagnostic langue) const noexcept
    {
        return langue == LangueDiagnostic::Francais ? _MessageFrancais : _MessageAnglais;
    }

    std::string IdentifierFichierSource(const std::filesystem::path& chemin)
    {
        std::error_code code;
        const auto canonique = std::filesystem::weakly_canonical(chemin, code);
        if (code) throw ErreurFichierSource(OperationFichierSource::Identification, chemin, code);
        const auto utf8 = canonique.generic_u8string();
        std::string cle(utf8.begin(), utf8.end());
#if defined(_WIN32)
        std::transform(cle.begin(), cle.end(), cle.begin(), [](unsigned char valeur)
        { return valeur >= 'A' && valeur <= 'Z' ? static_cast<char>(valeur + ('a' - 'A')) : static_cast<char>(valeur); });
#endif
        return cle;
    }

    bool EstFichierSourceRegulier(const std::filesystem::path& chemin)
    {
        std::error_code code;
        const auto statut = std::filesystem::status(chemin, code);
        if (code && code != std::errc::no_such_file_or_directory && code != std::errc::not_a_directory)
            throw ErreurFichierSource(OperationFichierSource::Statut, chemin, code);
        return std::filesystem::is_regular_file(statut);
    }

    std::string LireFluxSource(std::istream& flux, const std::filesystem::path& chemin, std::uint64_t maximum)
    {
        std::string source;
        std::array<char, 8192> bloc{};
        while (flux)
        {
            errno = 0;
            try { flux.read(bloc.data(), static_cast<std::streamsize>(bloc.size())); }
            catch (const std::ios_base::failure&)
            {
                if (!flux.eof() || flux.bad())
                    throw ErreurFichierSource(OperationFichierSource::Lecture, chemin, CodeLecture(errno));
                // Une lecture courte à EOF reste normale, même si failbit fait lever le flux.
            }
            if (flux.bad() || (flux.fail() && !flux.eof()))
                throw ErreurFichierSource(OperationFichierSource::Lecture, chemin, CodeLecture(errno));
            const auto taille = static_cast<std::uint64_t>(flux.gcount());
            if (taille > maximum - source.size())
                throw std::length_error("limite d'octets de source dépassée : " + chemin.string());
            source.append(bloc.data(), static_cast<std::size_t>(taille));
        }
        if (!flux.eof() || flux.bad())
            throw ErreurFichierSource(OperationFichierSource::Lecture, chemin, CodeLecture(0));
        return source;
    }

    std::string LireFichierSource(const std::filesystem::path& chemin, std::uint64_t maximum)
    {
        errno = 0;
        std::ifstream flux(chemin, std::ios::binary);
        if (!flux) throw ErreurFichierSource(OperationFichierSource::Ouverture, chemin, CodeLecture(errno));
        return LireFluxSource(flux, chemin, maximum);
    }
}
