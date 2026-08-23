#include "GsPP/VerificateurGsE.hpp"

#include "GsPP/FormatGsE.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <limits>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace GsPP
{
    namespace
    {
        bool AdditionValide(std::uint64_t a, std::uint64_t b, std::uint64_t limite)
        { return a <= limite && b <= limite - a; }
        std::uint16_t Lire16(const std::vector<std::uint8_t>& c, std::size_t p)
        { return static_cast<std::uint16_t>(c[p] | (c[p + 1] << 8)); }
        std::uint32_t Lire32(const std::vector<std::uint8_t>& c, std::size_t p)
        {
            std::uint32_t v = 0; for (int i = 0; i < 4; ++i) v |= static_cast<std::uint32_t>(c[p + i]) << (i * 8); return v;
        }
        std::uint64_t Lire64(const std::vector<std::uint8_t>& c, std::size_t p)
        {
            std::uint64_t v = 0; for (int i = 0; i < 8; ++i) v |= static_cast<std::uint64_t>(c[p + i]) << (i * 8); return v;
        }
        std::string LireNom(const std::vector<std::uint8_t>& c, std::size_t p, std::size_t taille)
        {
            const auto fin = std::find(c.begin() + static_cast<std::ptrdiff_t>(p),
                                      c.begin() + static_cast<std::ptrdiff_t>(p + taille), 0);
            return std::string(c.begin() + static_cast<std::ptrdiff_t>(p), fin);
        }
        bool PuissanceDeux(std::uint64_t v) { return v != 0 && (v & (v - 1)) == 0; }

        struct SegmentLu { std::uint32_t Drapeaux; std::uint64_t Fichier, TailleFichier, Va, TailleMemoire; };
        struct SectionLue
        {
            std::string Nom;
            std::uint32_t Type, Drapeaux;
            std::uint64_t Fichier, Taille, Va, TailleMemoire;
            std::uint32_t TailleEntree, Nombre;
        };
    }

    RapportVerificationGsE VerificateurGsE::Verifier(const std::vector<std::uint8_t>& contenu) const
    {
        RapportVerificationGsE rapport;
        auto erreur = [&](std::string message) { rapport.Erreurs.push_back(std::move(message)); };
        if (contenu.size() < 0x70)
        { erreur("fichier plus petit que l’en-tête GsE"); return rapport; }
        if (!(contenu[0] == 'G' && contenu[1] == 'S' && contenu[2] == 'E'
              && contenu[3] == ':' && contenu[4] == '0' && contenu[5] == 0
              && contenu[6] == 0 && contenu[7] == 0))
        { erreur("signature GSE:0 absente"); return rapport; }

        const auto majeure = Lire16(contenu, 8);
        const auto mineure = Lire16(contenu, 10);
        const auto tailleEntete = Lire16(contenu, 12);
        const auto architecture = Lire16(contenu, 14);
        const auto typeFichier = Lire16(contenu, 16);
        const auto versionAbi = Lire16(contenu, 20);
        const auto nombreSegments = Lire32(contenu, 24);
        const auto nombreSections = Lire32(contenu, 28);
        const auto pointEntree = Lire64(contenu, 32);
        const auto tailleImage = Lire64(contenu, 48);
        const auto tableSegments = Lire64(contenu, 56);
        const auto tableSections = Lire64(contenu, 64);
        const auto metaOffset = Lire64(contenu, 72);
        const auto metaTaille = Lire64(contenu, 80);

        if (majeure != VersionMajeureGsE || mineure != VersionMineureGsE)
            erreur("version GsE non prise en charge");
        if (versionAbi != VersionAbiGsE) erreur("ABI GsE non prise en charge");
        if (Lire16(contenu, 22) != 0 || Lire64(contenu, 40) != 0
            || Lire64(contenu, 88) != 0 || Lire64(contenu, 96) != 0
            || Lire64(contenu, 104) != 0)
            erreur("champ réservé GsE non nul");
        if (tailleEntete < 0x70 || tailleEntete > contenu.size()) erreur("taille d’en-tête invalide");
        if (architecture != 0x8664) erreur("architecture GsE non prise en charge");
        if (typeFichier != 1) erreur("le fichier GsE n’est pas un exécutable");
        if (tailleImage == 0) erreur("taille d’image nulle");
        if ((tailleImage & 0xFFF) != 0) erreur("taille d’image non alignée sur une page");
        if (pointEntree >= tailleImage) erreur("point d’entrée hors de l’image");
        if (nombreSegments == 0 || nombreSegments > 128) erreur("nombre de segments invalide");
        if (nombreSections == 0 || nombreSections > 1024) erreur("nombre de sections invalide");
        if (!AdditionValide(tableSegments, static_cast<std::uint64_t>(nombreSegments) * 64, contenu.size()))
            erreur("table des segments hors fichier");
        if (!AdditionValide(tableSections, static_cast<std::uint64_t>(nombreSections) * 64, contenu.size()))
            erreur("table des sections hors fichier");
        if (tableSegments < tailleEntete || tableSections < tailleEntete)
            erreur("table recouvrant l’en-tête GsE");
        if (!rapport.Erreurs.empty()) return rapport;

        const auto finTableSegments = tableSegments + static_cast<std::uint64_t>(nombreSegments) * 64;
        const auto finTableSections = tableSections + static_cast<std::uint64_t>(nombreSections) * 64;
        if (tableSegments < finTableSections && tableSections < finTableSegments)
            erreur("chevauchement des tables de segments et de sections");
        const auto finTables = std::max(finTableSegments, finTableSections);

        std::vector<SegmentLu> segments;
        bool entreeExecutable = false;
        for (std::uint32_t i = 0; i < nombreSegments; ++i)
        {
            const auto p = static_cast<std::size_t>(tableSegments + i * 64ULL);
            SegmentLu segment{Lire32(contenu, p + 4), Lire64(contenu, p + 8), Lire64(contenu, p + 16),
                              Lire64(contenu, p + 24), Lire64(contenu, p + 32)};
            const auto alignement = Lire64(contenu, p + 40);
            const bool plageVirtuelleValide = AdditionValide(
                segment.Va, segment.TailleMemoire, std::numeric_limits<std::uint64_t>::max());
            if (segment.TailleMemoire == 0) erreur("segment de taille mémoire nulle");
            if (!plageVirtuelleValide) erreur("dépassement de la plage virtuelle d’un segment");
            else if (segment.Va + segment.TailleMemoire > tailleImage)
                erreur("segment hors de la taille d’image");
            if (segment.TailleFichier > segment.TailleMemoire) erreur("segment : taille fichier supérieure à la mémoire");
            if (segment.TailleFichier > 0 && !AdditionValide(segment.Fichier, segment.TailleFichier, contenu.size()))
                erreur("segment hors fichier");
            if (segment.TailleFichier > 0 && segment.Fichier < finTables)
                erreur("segment recouvrant les tables GsE");
            if (!PuissanceDeux(alignement) || alignement < 0x1000)
                erreur("alignement de segment invalide");
            else if ((segment.Va & (alignement - 1)) != 0) erreur("adresse de segment mal alignée");
            if ((segment.Drapeaux & ~7U) != 0 || (segment.Drapeaux & 1) == 0)
                erreur("drapeaux de segment invalides");
            if ((segment.Drapeaux & 2) && (segment.Drapeaux & 4)) erreur("segment simultanément inscriptible et exécutable");
            if (plageVirtuelleValide && (segment.Drapeaux & 4) && pointEntree >= segment.Va
                && pointEntree < segment.Va + segment.TailleMemoire) entreeExecutable = true;
            for (const auto& precedent : segments)
            {
                if (plageVirtuelleValide
                    && AdditionValide(precedent.Va, precedent.TailleMemoire, std::numeric_limits<std::uint64_t>::max())
                    && segment.Va < precedent.Va + precedent.TailleMemoire
                    && precedent.Va < segment.Va + segment.TailleMemoire)
                    erreur("chevauchement de segments virtuels");
            }
            segments.push_back(segment);
        }
        if (!entreeExecutable) erreur("point d’entrée hors segment exécutable");

        std::vector<SectionLue> sections;
        std::uint32_t nombreImports = 0;
        std::uint32_t nombreExports = 0;
        std::unordered_set<std::string> nomsSections;
        std::array<std::uint32_t, 9> comptesTypes{};
        bool sectionMetadonneesTrouvee = false;
        for (std::uint32_t i = 0; i < nombreSections; ++i)
        {
            const auto p = static_cast<std::size_t>(tableSections + i * 64ULL);
            SectionLue section{LireNom(contenu, p, 16), Lire32(contenu, p + 16),
                               Lire32(contenu, p + 20), Lire64(contenu, p + 24),
                               Lire64(contenu, p + 32), Lire64(contenu, p + 40),
                               Lire64(contenu, p + 48), Lire32(contenu, p + 56),
                               Lire32(contenu, p + 60)};
            if (section.Nom.empty()) erreur("section sans nom");
            else if (!nomsSections.insert(section.Nom).second) erreur("nom de section dupliqué : " + section.Nom);
            if (section.Type == 0 || section.Type >= comptesTypes.size())
                erreur("type de section inconnu : " + section.Nom);
            else if (++comptesTypes[section.Type] > 1)
                erreur("type de section dupliqué : " + section.Nom);
            if (section.Taille > 0 && !AdditionValide(section.Fichier, section.Taille, contenu.size()))
                erreur("section hors fichier : " + section.Nom);
            if (section.Taille > 0 && section.Fichier < finTables)
                erreur("section recouvrant les tables : " + section.Nom);
            if (section.TailleEntree > 0
                && static_cast<std::uint64_t>(section.TailleEntree) * section.Nombre > section.Taille)
                erreur("table tronquée dans " + section.Nom);
            if (section.Type >= 1 && section.Type <= 3)
            {
                if (!AdditionValide(section.Va, section.TailleMemoire, tailleImage))
                    erreur("section chargée hors de l’image : " + section.Nom);
                bool couverte = false;
                for (const auto& segment : segments)
                    if (section.Va >= segment.Va
                        && AdditionValide(section.Va, section.TailleMemoire,
                                          segment.Va + segment.TailleMemoire)
                        && section.Va + section.TailleMemoire <= segment.Va + segment.TailleMemoire)
                        couverte = true;
                if (!couverte) erreur("section non couverte par un segment : " + section.Nom);
            }
            if (section.Type == TypeSectionImportsGsE)
            {
                nombreImports = section.Nombre;
                if (section.TailleEntree != TailleEntreeImportGsE)
                    erreur("taille d’entrée import incorrecte");
            }
            if (section.Type == TypeSectionExportsGsE)
            {
                nombreExports = section.Nombre;
                if (section.TailleEntree != TailleEntreeExportGsE)
                    erreur("taille d’entrée export incorrecte");
            }
            if (section.Type == TypeSectionChainesGsE)
            {
                if (section.TailleEntree != 0 || section.Nombre != 0)
                    erreur("la table de chaînes ne doit pas annoncer d’entrées fixes");
            }
            if (section.Type == TypeSectionMetadonneesGsE)
            {
                sectionMetadonneesTrouvee = true;
                if (section.Fichier != metaOffset || section.Taille != metaTaille)
                    erreur("section .meta incohérente avec l’en-tête");
            }
            if (section.Type >= TypeSectionImportsGsE
                && section.Type <= TypeSectionRelocalisationsGsE
                && static_cast<std::uint64_t>(section.TailleEntree) * section.Nombre != section.Taille)
                erreur("taille de table incohérente dans " + section.Nom);
            sections.push_back(std::move(section));
        }

        if (comptesTypes[TypeSectionTexteGsE] != 1) erreur("section de texte absente");

        const auto sectionChaines = std::find_if(
            sections.begin(), sections.end(),
            [](const SectionLue& section) { return section.Type == TypeSectionChainesGsE; });
        std::unordered_map<std::uint32_t, std::string> chaines;
        if (sectionChaines == sections.end())
        {
            if (nombreImports != 0 || nombreExports != 0)
                erreur("table de chaînes absente pour les noms de symboles");
        }
        else if (AdditionValide(sectionChaines->Fichier, sectionChaines->Taille, contenu.size()))
        {
            std::uint64_t positionRelative = 0;
            while (positionRelative < sectionChaines->Taille)
            {
                const auto positionAbsolue = sectionChaines->Fichier + positionRelative;
                const auto finTable = sectionChaines->Fichier + sectionChaines->Taille;
                auto fin = positionAbsolue;
                while (fin < finTable && contenu[static_cast<std::size_t>(fin)] != 0) ++fin;
                if (fin == finTable)
                {
                    erreur("chaîne de symbole GsE sans terminaison nulle");
                    break;
                }
                const auto longueur = fin - positionAbsolue;
                if (longueur == 0)
                    erreur("chaîne de symbole GsE vide");
                else if (longueur > TailleNomSymboleGsEMaximale)
                    erreur("nom de symbole GsE supérieur à 1024 octets UTF-8");
                else if (!Utf8GsEValide(
                    contenu.data() + static_cast<std::size_t>(positionAbsolue),
                    static_cast<std::size_t>(longueur)))
                    erreur("nom de symbole GsE en UTF-8 invalide");
                else if (positionRelative > std::numeric_limits<std::uint32_t>::max())
                    erreur("position de chaîne GsE non représentable");
                else
                    chaines.emplace(
                        static_cast<std::uint32_t>(positionRelative),
                        std::string(
                            contenu.begin() + static_cast<std::ptrdiff_t>(positionAbsolue),
                            contenu.begin() + static_cast<std::ptrdiff_t>(fin)));
                positionRelative += longueur + 1;
            }
        }

        auto lireNomSymbole = [&](std::size_t position, const char* nature)
            -> std::optional<std::string>
        {
            const auto decalage = Lire32(contenu, position);
            const auto longueur = Lire16(contenu, position + 4);
            const auto trouve = chaines.find(decalage);
            if (trouve == chaines.end())
            {
                erreur(std::string(nature) + " référençant une chaîne inexistante");
                return std::nullopt;
            }
            if (longueur != trouve->second.size())
            {
                erreur(std::string(nature) + " avec une longueur de nom incohérente");
                return std::nullopt;
            }
            return trouve->second;
        };

        std::unordered_set<std::string> nomsImports;
        std::unordered_set<std::string> nomsExports;
        for (const auto& section : sections)
        {
            if (section.Type == TypeSectionImportsGsE
                && section.TailleEntree == TailleEntreeImportGsE
                && section.Taille == static_cast<std::uint64_t>(section.Nombre)
                    * TailleEntreeImportGsE
                && AdditionValide(section.Fichier, section.Taille, contenu.size()))
            {
                for (std::uint32_t n = 0; n < section.Nombre; ++n)
                {
                    const auto q = static_cast<std::size_t>(
                        section.Fichier + static_cast<std::uint64_t>(n) * TailleEntreeImportGsE);
                    const auto nom = lireNomSymbole(q, "import");
                    if (nom && !nomsImports.insert(*nom).second)
                        erreur("import dupliqué : " + *nom);
                    if (Lire16(contenu, q + 6) != 1)
                        erreur("type d’import GsE non pris en charge");
                    if (Lire16(contenu, q + 8) != VersionAbiGsE)
                        erreur("ABI d’import GsE non prise en charge");
                    if ((Lire32(contenu, q + 12) & ~1U) != 0)
                        erreur("drapeaux d’import GsE inconnus");
                    if (Lire16(contenu, q + 10) != 0
                        || Lire64(contenu, q + 16) != 0
                        || Lire64(contenu, q + 24) != 0)
                        erreur("champs réservés d’un import non nuls");
                }
            }
            if (section.Type == TypeSectionExportsGsE
                && section.TailleEntree == TailleEntreeExportGsE
                && section.Taille == static_cast<std::uint64_t>(section.Nombre)
                    * TailleEntreeExportGsE
                && AdditionValide(section.Fichier, section.Taille, contenu.size()))
            {
                for (std::uint32_t n = 0; n < section.Nombre; ++n)
                {
                    const auto q = static_cast<std::size_t>(
                        section.Fichier + static_cast<std::uint64_t>(n) * TailleEntreeExportGsE);
                    const auto nom = lireNomSymbole(q, "export");
                    if (nom && !nomsExports.insert(*nom).second)
                        erreur("export dupliqué : " + *nom);
                    const auto sectionSymbole = Lire16(contenu, q + 20);
                    const auto typeSymbole = Lire16(contenu, q + 22);
                    if (sectionSymbole > 2)
                        erreur("section d’export GsE inconnue");
                    if (typeSymbole != 1 && typeSymbole != 2)
                        erreur("type d’export GsE inconnu");
                    if ((Lire32(contenu, q + 24) & ~1U) != 0)
                        erreur("drapeaux d’export GsE inconnus");
                    if (Lire16(contenu, q + 6) != 0 || Lire32(contenu, q + 28) != 0)
                        erreur("champs réservés d’un export non nuls");
                    const auto rva = Lire64(contenu, q + 8);
                    bool dansSegment = false;
                    for (const auto& segment : segments)
                        if (AdditionValide(segment.Va, segment.TailleMemoire,
                                          std::numeric_limits<std::uint64_t>::max())
                            && rva >= segment.Va && rva < segment.Va + segment.TailleMemoire)
                            dansSegment = true;
                    if (!dansSegment) erreur("export hors segment chargé");
                }
            }
        }

        for (const auto& section : sections)
        {
            if (section.Type != TypeSectionRelocalisationsGsE) continue;
            if (section.TailleEntree != TailleEntreeRelocalisationGsE)
                erreur("taille d’entrée de relocalisation incorrecte");
            if (section.TailleEntree != TailleEntreeRelocalisationGsE
                || section.Taille != static_cast<std::uint64_t>(section.Nombre)
                    * TailleEntreeRelocalisationGsE
                || !AdditionValide(section.Fichier, section.Taille, contenu.size()))
                continue;
            for (std::uint32_t n = 0; n < section.Nombre; ++n)
            {
                const auto p = static_cast<std::size_t>(
                    section.Fichier
                    + static_cast<std::uint64_t>(n) * TailleEntreeRelocalisationGsE);
                const auto rva = Lire64(contenu, p);
                const auto indexImport = Lire32(contenu, p + 8);
                const auto type = Lire16(contenu, p + 12);
                const auto sectionSource = Lire16(contenu, p + 14);
                const auto ajout = Lire64(contenu, p + 16);
                const bool relocalisationBase = type == TypeRelocalisationBase64GsE;
                if (relocalisationBase)
                {
                    if (indexImport != IndiceImportRelocalisationBaseGsE)
                        erreur("indice d’import invalide pour une relocalisation BASE64");
                    bool cibleDansSegment = false;
                    for (const auto& segment : segments)
                        if (AdditionValide(segment.Va, segment.TailleMemoire,
                                          std::numeric_limits<std::uint64_t>::max())
                            && ajout >= segment.Va
                            && ajout < segment.Va + segment.TailleMemoire)
                            cibleDansSegment = true;
                    if (!cibleDansSegment)
                        erreur("cible BASE64 hors segment chargé");
                }
                else
                {
                    if (indexImport >= nombreImports)
                        erreur("relocalisation vers un import inexistant");
                    if (type != TypeRelocalisationImportRelatif32GsE
                        && type != TypeRelocalisationImportAdresse64GsE)
                        erreur("type de relocalisation inconnu");
                }
                if (sectionSource > 1) erreur("section source de relocalisation inconnue");
                bool dansSegment = false;
                const auto largeur = static_cast<std::uint64_t>(
                    type == TypeRelocalisationImportRelatif32GsE ? 4 : 8);
                for (const auto& segment : segments)
                    if (AdditionValide(rva, largeur, std::numeric_limits<std::uint64_t>::max())
                        && AdditionValide(segment.Va, segment.TailleMemoire, std::numeric_limits<std::uint64_t>::max())
                        && rva >= segment.Va && rva + largeur <= segment.Va + segment.TailleMemoire)
                        dansSegment = true;
                if (!dansSegment) erreur("relocalisation hors segment chargé");
            }
        }

        if (metaTaille == 0 || !AdditionValide(metaOffset, metaTaille, contenu.size()))
            erreur("métadonnées absentes ou hors fichier");
        else
        {
            const std::string meta(contenu.begin() + static_cast<std::ptrdiff_t>(metaOffset),
                                   contenu.begin() + static_cast<std::ptrdiff_t>(metaOffset + metaTaille));
            if (meta.find("Format = \"GsEMetadata:1\";") == std::string::npos)
                erreur("signature des métadonnées absente");
        }
        if (!sectionMetadonneesTrouvee) erreur("section .meta absente");

        std::ostringstream info;
        info << "GsE " << majeure << '.' << mineure << ", " << nombreSegments
             << " segment(s), " << nombreSections << " section(s), " << nombreImports
             << " import(s), " << nombreExports << " export(s)";
        rapport.Informations.push_back(info.str());
        rapport.Valide = rapport.Erreurs.empty();
        return rapport;
    }

    RapportVerificationGsE VerificateurGsE::Verifier(const std::filesystem::path& chemin) const
    {
        std::ifstream flux(chemin, std::ios::binary);
        if (!flux) return {false, {"impossible d’ouvrir le fichier"}, {}};
        std::vector<std::uint8_t> contenu(
            (std::istreambuf_iterator<char>(flux)), std::istreambuf_iterator<char>());
        return Verifier(contenu);
    }
}
