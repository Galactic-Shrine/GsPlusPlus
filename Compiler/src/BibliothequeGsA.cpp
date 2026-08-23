#include "GsPP/BibliothequeGsA.hpp"

#include "GsPP/FormatGsE.hpp"
#include "GsPP/ObjetGsO.hpp"

#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace GsPP
{
    namespace
    {
        constexpr std::size_t TailleNomMembreMaximale = 1024;

        void Ajouter16(std::vector<std::uint8_t>& sortie, std::uint16_t valeur)
        {
            sortie.push_back(static_cast<std::uint8_t>(valeur));
            sortie.push_back(static_cast<std::uint8_t>(valeur >> 8));
        }

        void Ajouter32(std::vector<std::uint8_t>& sortie, std::uint32_t valeur)
        {
            for (int index = 0; index < 4; ++index)
                sortie.push_back(static_cast<std::uint8_t>(valeur >> (index * 8)));
        }

        void Ajouter64(std::vector<std::uint8_t>& sortie, std::uint64_t valeur)
        {
            for (int index = 0; index < 8; ++index)
                sortie.push_back(static_cast<std::uint8_t>(valeur >> (index * 8)));
        }

        std::uint16_t Lire16(const std::vector<std::uint8_t>& contenu, std::size_t position)
        {
            if (position > contenu.size() || contenu.size() - position < 2)
                throw std::runtime_error("bibliothèque GsA tronquée");
            return static_cast<std::uint16_t>(contenu[position])
                | static_cast<std::uint16_t>(contenu[position + 1] << 8);
        }

        std::uint32_t Lire32(const std::vector<std::uint8_t>& contenu, std::size_t position)
        {
            if (position > contenu.size() || contenu.size() - position < 4)
                throw std::runtime_error("bibliothèque GsA tronquée");
            std::uint32_t valeur = 0;
            for (int index = 0; index < 4; ++index)
                valeur |= static_cast<std::uint32_t>(contenu[position + index]) << (index * 8);
            return valeur;
        }

        std::uint64_t Lire64(const std::vector<std::uint8_t>& contenu, std::size_t position)
        {
            if (position > contenu.size() || contenu.size() - position < 8)
                throw std::runtime_error("bibliothèque GsA tronquée");
            std::uint64_t valeur = 0;
            for (int index = 0; index < 8; ++index)
                valeur |= static_cast<std::uint64_t>(contenu[position + index]) << (index * 8);
            return valeur;
        }

        std::size_t Aligner16(std::size_t valeur)
        {
            if (valeur > std::numeric_limits<std::size_t>::max() - 15)
                throw std::overflow_error("bibliothèque GsA trop grande");
            return (valeur + 15) & ~std::size_t{15};
        }

        std::vector<std::uint8_t> LireFichier(const std::filesystem::path& chemin)
        {
            std::ifstream flux(chemin, std::ios::binary);
            if (!flux)
                throw std::runtime_error(
                    "impossible d’ouvrir la bibliothèque GsA : " + chemin.string());
            flux.seekg(0, std::ios::end);
            const auto taille = flux.tellg();
            if (taille < 0) throw std::runtime_error("taille de bibliothèque GsA invalide");
            flux.seekg(0, std::ios::beg);
            std::vector<std::uint8_t> contenu(static_cast<std::size_t>(taille));
            flux.read(
                reinterpret_cast<char*>(contenu.data()),
                static_cast<std::streamsize>(contenu.size()));
            if (!flux && !contenu.empty())
                throw std::runtime_error("échec de lecture de la bibliothèque GsA");
            return contenu;
        }

        void VerifierNom(const std::string& nom)
        {
            if (nom.empty() || nom.size() > TailleNomMembreMaximale)
                throw std::runtime_error("nom de membre GsA vide ou trop long");
            if (nom.find('\0') != std::string::npos
                || !Utf8GsEValide(
                    reinterpret_cast<const std::uint8_t*>(nom.data()), nom.size()))
                throw std::runtime_error("nom de membre GsA en UTF-8 invalide");
        }
    }

    std::vector<std::uint8_t> EcrivainGsA::Construire(
        const std::vector<MembreGsA>& membres) const
    {
        if (membres.size() > UINT32_MAX)
            throw std::overflow_error("trop de membres dans la bibliothèque GsA");
        std::unordered_set<std::string> noms;
        for (const auto& membre : membres)
        {
            VerifierNom(membre.Nom);
            if (!noms.insert(membre.Nom).second)
                throw std::runtime_error("membre GsA dupliqué : " + membre.Nom);
            (void)LecteurGsO().Lire(membre.Objet);
        }

        std::vector<std::uint8_t> sortie;
        sortie.insert(sortie.end(), {'G', 'S', 'A', ':', '0', 0, 0, 0});
        Ajouter16(sortie, VersionMajeureGsA);
        Ajouter16(sortie, VersionMineureGsA);
        Ajouter32(sortie, static_cast<std::uint32_t>(TailleEnteteGsA));
        Ajouter32(sortie, static_cast<std::uint32_t>(membres.size()));
        Ajouter16(sortie, VersionAbiGsA);
        Ajouter16(sortie, 0);
        Ajouter64(sortie, 0);
        if (sortie.size() != TailleEnteteGsA)
            throw std::logic_error("taille interne d’en-tête GsA incorrecte");

        for (const auto& membre : membres)
        {
            Ajouter32(sortie, static_cast<std::uint32_t>(membre.Nom.size()));
            Ajouter32(sortie, 0);
            Ajouter64(sortie, membre.Objet.size());
            sortie.insert(sortie.end(), membre.Nom.begin(), membre.Nom.end());
            sortie.push_back(0);
            sortie.resize(Aligner16(sortie.size()), 0);
            sortie.insert(sortie.end(), membre.Objet.begin(), membre.Objet.end());
            sortie.resize(Aligner16(sortie.size()), 0);
        }

        const auto taille = static_cast<std::uint64_t>(sortie.size());
        for (int index = 0; index < 8; ++index)
            sortie[24 + index] = static_cast<std::uint8_t>(taille >> (index * 8));
        return sortie;
    }

    void EcrivainGsA::Ecrire(
        const std::vector<MembreGsA>& membres,
        const std::filesystem::path& chemin) const
    {
        const auto contenu = Construire(membres);
        std::ofstream flux(chemin, std::ios::binary | std::ios::trunc);
        if (!flux)
            throw std::runtime_error("impossible d’ouvrir la sortie GsA : " + chemin.string());
        flux.write(
            reinterpret_cast<const char*>(contenu.data()),
            static_cast<std::streamsize>(contenu.size()));
        if (!flux) throw std::runtime_error("échec de l’écriture de la bibliothèque GsA");
    }

    std::vector<MembreGsA> LecteurGsA::Lire(
        const std::vector<std::uint8_t>& contenu) const
    {
        if (contenu.size() < TailleEnteteGsA
            || contenu[0] != 'G' || contenu[1] != 'S'
            || contenu[2] != 'A' || contenu[3] != ':'
            || contenu[4] != '0' || contenu[5] != 0
            || contenu[6] != 0 || contenu[7] != 0)
            throw std::runtime_error("signature de bibliothèque GsA invalide");
        if (Lire16(contenu, 8) != VersionMajeureGsA
            || Lire16(contenu, 10) != VersionMineureGsA)
            throw std::runtime_error("version de bibliothèque GsA incompatible");
        if (Lire32(contenu, 12) != TailleEnteteGsA)
            throw std::runtime_error("taille d’en-tête GsA invalide");
        const auto nombreMembres = Lire32(contenu, 16);
        if (nombreMembres > 1'000'000)
            throw std::runtime_error("nombre de membres GsA déraisonnable");
        if (Lire16(contenu, 20) != VersionAbiGsA)
            throw std::runtime_error("ABI de bibliothèque GsA incompatible");
        if (Lire16(contenu, 22) != 0)
            throw std::runtime_error("champ réservé GsA non nul");
        if (Lire64(contenu, 24) != contenu.size())
            throw std::runtime_error("taille totale GsA incohérente");

        std::vector<MembreGsA> membres;
        membres.reserve(nombreMembres);
        std::unordered_set<std::string> noms;
        std::size_t curseur = TailleEnteteGsA;
        for (std::uint32_t index = 0; index < nombreMembres; ++index)
        {
            if (curseur > contenu.size() || contenu.size() - curseur < 16)
                throw std::runtime_error("entrée de membre GsA tronquée");
            const auto tailleNom = Lire32(contenu, curseur);
            if (Lire32(contenu, curseur + 4) != 0)
                throw std::runtime_error("champ réservé de membre GsA non nul");
            const auto tailleObjet = Lire64(contenu, curseur + 8);
            curseur += 16;
            if (tailleNom == 0 || tailleNom > TailleNomMembreMaximale
                || curseur > contenu.size()
                || static_cast<std::uint64_t>(tailleNom) + 1 > contenu.size() - curseur)
                throw std::runtime_error("nom de membre GsA invalide");
            if (contenu[curseur + tailleNom] != 0)
                throw std::runtime_error("nom de membre GsA non terminé");
            MembreGsA membre;
            membre.Nom.assign(
                reinterpret_cast<const char*>(contenu.data() + curseur), tailleNom);
            VerifierNom(membre.Nom);
            if (!noms.insert(membre.Nom).second)
                throw std::runtime_error("membre GsA dupliqué : " + membre.Nom);
            curseur = Aligner16(curseur + tailleNom + 1);
            if (curseur > contenu.size()
                || tailleObjet > contenu.size() - curseur)
                throw std::runtime_error("contenu de membre GsA tronqué");
            membre.Objet.assign(
                contenu.begin() + static_cast<std::ptrdiff_t>(curseur),
                contenu.begin() + static_cast<std::ptrdiff_t>(curseur + tailleObjet));
            (void)LecteurGsO().Lire(membre.Objet);
            curseur = Aligner16(curseur + static_cast<std::size_t>(tailleObjet));
            membres.push_back(std::move(membre));
        }
        if (curseur != contenu.size())
            throw std::runtime_error("données superflues après la bibliothèque GsA");
        return membres;
    }

    std::vector<MembreGsA> LecteurGsA::Lire(const std::filesystem::path& chemin) const
    {
        return Lire(LireFichier(chemin));
    }
}
