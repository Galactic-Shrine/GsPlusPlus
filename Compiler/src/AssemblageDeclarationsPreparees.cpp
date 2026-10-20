#include "GsPP/AssemblageDeclarationsPreparees.hpp"

#include "GsPP/ErreurCompilation.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

namespace GsPP
{
    namespace
    {
        [[noreturn]] void ContratAssemblageIncoherent()
        {
            throw std::runtime_error("sorties incompatibles avec le contrat d'assemblage préparé");
        }

        struct InstantaneUnite
        {
            std::string Source, Nom;
            std::vector<OrigineJetonDeclarationsHote> Origines, OriginesAvant;
            std::vector<std::pair<std::uint32_t, std::uint32_t>> Positions;
            std::uint32_t EstInterface = 0;
        };

        bool NomConflitValide(const std::string& nom)
        {
            if (nom.empty()) return false;
            // UTF-8 strict, sans contrôles ni espaces ; les noms synthétiques ne sont pas des identifiants source.
            for (std::size_t i = 0; i < nom.size();)
            {
                const auto premier = static_cast<unsigned char>(nom[i++]);
                if (premier < 0x80)
                {
                    if (premier <= 0x20 || premier == 0x7F) return false;
                    continue;
                }
                const auto suite = premier >= 0xC2 && premier <= 0xDF ? 1U
                    : premier >= 0xE0 && premier <= 0xEF ? 2U : premier >= 0xF0 && premier <= 0xF4 ? 3U : 0U;
                if (!suite || suite > nom.size() - i) return false;
                const auto second = static_cast<unsigned char>(nom[i]);
                if ((premier == 0xE0 && second < 0xA0) || (premier == 0xED && second >= 0xA0)
                    || (premier == 0xF0 && second < 0x90) || (premier == 0xF4 && second >= 0x90)) return false;
                for (unsigned j = 0; j < suite; ++j)
                    if ((static_cast<unsigned char>(nom[i++]) & 0xC0) != 0x80) return false;
            }
            return true;
        }
    }

    std::string_view DeclarationsAssemblees::Nom(std::size_t index) const
    {
        const auto& noeud = Noeuds.at(index);
        if (noeud.DebutNom > Source.size() || noeud.TailleNom > Source.size() - noeud.DebutNom)
            ContratAssemblageIncoherent();
        return std::string_view(Source).substr(static_cast<std::size_t>(noeud.DebutNom),
            static_cast<std::size_t>(noeud.TailleNom));
    }

    void DeclarationsAssemblees::ExigerValide() const
    {
        if (!Resultat.Erreur && !PositionErreur && NomConflit.empty()) return;
        if (!PositionErreur || !PositionErreur->Ligne || !PositionErreur->Colonne || PositionErreur->Fichier.empty()
            || Resultat.Reserve || Resultat.NombreNoeuds || !Noeuds.empty() || !Source.empty()
            || !Origines.empty() || !Unites.empty())
            ContratAssemblageIncoherent();
        if (Resultat.Erreur == 4)
        {
            if (!NomConflit.empty()) ContratAssemblageIncoherent();
            DeclarationsPreparees syntaxe;
            syntaxe.Resultat.Erreur = Resultat.Detail; syntaxe.Resultat.Detail = Resultat.DetailLexical;
            syntaxe.PositionErreur = PositionErreur;
            syntaxe.ExigerValide();
            ContratAssemblageIncoherent();
        }
        if (Resultat.Erreur < 6 || Resultat.Erreur > 10 || Resultat.Detail || Resultat.DetailLexical
            || Resultat.IndexUniteErreur >= Resultat.NombreOrigines || !NomConflitValide(NomConflit))
            ContratAssemblageIncoherent();
        constexpr const char* francais[]{
            "déclarations incompatibles pour la fonction ", "fonction définie plusieurs fois : ",
            "déclarations incompatibles pour la globale ", "variable globale définie plusieurs fois : ",
            "déclarations incompatibles pour l’alias "};
        constexpr const char* anglais[]{
            "incompatible declarations for function ", "function defined more than once: ",
            "incompatible declarations for global ", "global variable defined more than once: ",
            "incompatible declarations for alias "};
        const auto index = Resultat.Erreur - 6;
        throw ErreurCompilation(francais[index] + NomConflit, anglais[index] + NomConflit,
            PositionErreur->Ligne, PositionErreur->Colonne, PositionErreur->Fichier);
    }

    template<typename Assembleur>
    static DeclarationsAssemblees AssemblerDeclarationsPrepareesInterne(
        std::span<const ResultatPreparationFichier* const> preparations,
        Assembleur assembler, const LimitesAssemblageDeclarations& limites)
    {
        if (preparations.empty()) throw std::invalid_argument("assemblage préparé sans unités");
        if (!limites.MaximumOctets || limites.MaximumOctets > 999'999'999
            || !limites.MaximumJetons || limites.MaximumJetons > 100'000'000
            || !limites.MaximumNoeuds || limites.MaximumNoeuds > 100'000'000
            || !limites.MaximumUnites || limites.MaximumUnites > 1'000'000
            || !limites.MaximumFichiers || limites.MaximumFichiers > 1'000'000)
            throw std::invalid_argument("limites d'assemblage préparé invalides");
        if (preparations.size() > limites.MaximumUnites) throw std::length_error("limite d'unités dépassée");
        std::vector<InstantaneUnite> instantanes;
        instantanes.reserve(preparations.size());
        std::vector<std::string> fichiers;
        std::vector<OrigineUniteDeclarationsHote> plages;
        std::string texteAttendu;
        std::uint64_t totalOctets = 0, totalJetons = 0;
        std::uint32_t premiereLigne = 1;
        // Valide toutes les entrées avant le premier appel natif, sans analyse syntaxique C++.
        for (const auto* preparation : preparations)
        {
            if (!preparation) throw std::invalid_argument("instantané d'assemblage nul");
            const auto& texte = preparation->Preparation.Source;
            const auto& origines = preparation->Preparation.Origines;
            const auto& catalogue = preparation->Catalogue;
            if (texte.size() >= limites.MaximumOctets - totalOctets
                || origines.size() > limites.MaximumJetons - totalJetons
                || catalogue.Fichiers().size() > limites.MaximumFichiers - fichiers.size())
                throw std::length_error("limite cumulée d'assemblage préparé dépassée");
            (void)ConvertirPreparationEnJetons(*preparation);
            totalOctets += texte.size() + 1; totalJetons += origines.size();
            InstantaneUnite unite;
            unite.Source = texte;
            unite.Nom = catalogue.NomsFichiers()[catalogue.IndexRacine()];
            unite.EstInterface = catalogue.Fichiers()[catalogue.IndexRacine()].EstInterface;
            unite.Origines = origines;
            const auto baseFichiers = fichiers.size();
            fichiers.insert(fichiers.end(), catalogue.NomsFichiers().begin(), catalogue.NomsFichiers().end());
            for (auto& origine : unite.Origines) origine.IndexFichier += baseFichiers;
            unite.OriginesAvant = unite.Origines;
            const auto bom = texte.starts_with("\xEF\xBB\xBF") ? 3U : 0U;
            const auto nombreLignes = 1U + static_cast<std::uint32_t>(std::count(texte.begin(), texte.end(), '\n'));
            plages.push_back({texteAttendu.size(), texte.size() - bom, premiereLigne, nombreLignes, bom, 0});
            texteAttendu.append(texte, bom, std::string::npos); texteAttendu.push_back('\n');
            premiereLigne += nombreLignes;
            std::size_t octet = bom;
            std::uint32_t ligne = 1, colonne = 1;
            unite.Positions.reserve(origines.size());
            for (const auto& origine : origines)
            {
                while (octet < origine.DebutOctets)
                {
                    if (texte[octet++] == '\n') { ++ligne; colonne = 1; }
                    else ++colonne;
                }
                unite.Positions.emplace_back(ligne, colonne);
            }
            instantanes.push_back(std::move(unite));
        }
        const auto borneNoeuds = totalJetons - preparations.size() + 1;
        auto indexOrigine = [&](std::size_t unite, std::uint32_t ligne, std::uint32_t colonne)
        {
            if (unite >= instantanes.size()) ContratAssemblageIncoherent();
            const auto& positions = instantanes[unite].Positions;
            const auto cherche = std::pair{ligne, colonne};
            const auto trouve = std::lower_bound(positions.begin(), positions.end(), cherche);
            if (trouve == positions.end() || *trouve != cherche) ContratAssemblageIncoherent();
            return static_cast<std::size_t>(trouve - positions.begin());
        };
        auto positionOriginale = [&](std::size_t unite, std::size_t index)
        {
            const auto& origine = instantanes[unite].OriginesAvant[index];
            return PositionSource{fichiers[origine.IndexFichier], origine.Ligne, origine.Colonne,
                instantanes[unite].Nom, index};
        };
        std::vector<UniteDeclarationsPrepareeHote> unites;
        std::vector<TableOriginesUniteDeclarationsHote> tables;
        for (const auto& unite : instantanes)
        {
            unites.push_back({unite.Source.data(), unite.Source.size(), unite.EstInterface, 0});
            tables.push_back({unite.Origines.data(), unite.Origines.size()});
        }
        const auto unitesAvant = unites; const auto tablesAvant = tables;
        RequeteAssemblageDeclarationsHote assemblage{unites.data(), unites.size(), nullptr, 0, nullptr, 0, nullptr, 0, {}};
        auto appeler = [&]()
        {
            const auto avant = assemblage;
            RequeteAssemblageAvecOriginesHote requete{&assemblage, tables.data(), fichiers.size(), fichiers.size(), 0, 0};
            const auto code = assembler(&requete);
            if (std::memcmp(&assemblage, &avant, offsetof(RequeteAssemblageDeclarationsHote, Resultat)) != 0
                || requete.Assemblage != &assemblage || requete.Tables != tables.data()
                || requete.NombreFichiers != fichiers.size()
                || std::memcmp(unites.data(), unitesAvant.data(), unites.size() * sizeof(unites[0])) != 0
                || std::memcmp(tables.data(), tablesAvant.data(), tables.size() * sizeof(tables[0])) != 0
                || code != assemblage.Resultat.Erreur || code > 10 || assemblage.Resultat.Reserve)
                ContratAssemblageIncoherent();
            for (std::size_t i = 0; i < instantanes.size(); ++i)
                if (instantanes[i].Source != preparations[i]->Preparation.Source
                    || std::memcmp(instantanes[i].Origines.data(), instantanes[i].OriginesAvant.data(),
                        instantanes[i].Origines.size() * sizeof(OrigineJetonDeclarationsHote)) != 0)
                    ContratAssemblageIncoherent();
            if (code == 3) throw std::bad_alloc();
            if (code == 2 || code == 5 || assemblage.Resultat.NombreNoeuds > borneNoeuds)
                ContratAssemblageIncoherent();
            return requete;
        };
        auto requete = appeler();
        const auto mesure = assemblage.Resultat;
        if (mesure.Erreur == 4 || mesure.Erreur >= 6)
        {
            const auto unite = static_cast<std::size_t>(mesure.IndexUniteErreur);
            const auto index = indexOrigine(unite, mesure.LigneErreur, mesure.ColonneErreur);
            const auto& origine = instantanes[unite].OriginesAvant[index];
            if (requete.IndexFichierErreur != origine.IndexFichier || requete.LigneLocaleErreur != origine.Ligne
                || requete.ColonneLocaleErreur != origine.Colonne)
                ContratAssemblageIncoherent();
            const auto position = positionOriginale(unite, index);
            if (mesure.Erreur == 4)
            {
                // Réutilise la validation code/contexte bilingue de l'AST mono-unité.
                DeclarationsPreparees syntaxe;
                syntaxe.Resultat.Erreur = mesure.Detail; syntaxe.Resultat.Detail = mesure.DetailLexical;
                syntaxe.PositionErreur = position;
                try { syntaxe.ExigerValide(); }
                catch (const ErreurCompilation&) { }
            }
            else if (mesure.Detail || mesure.DetailLexical || mesure.NombreNoeuds
                || mesure.NombreOctetsSource != texteAttendu.size() || mesure.NombreOrigines != unites.size())
                ContratAssemblageIncoherent();
            DeclarationsAssemblees refus;
            refus.Resultat = mesure; refus.PositionErreur = position;
            return refus;
        }
        auto verifierSucces = [&](std::uint32_t code)
        {
            const auto& r = assemblage.Resultat;
            if (r.Erreur != code || !r.NombreNoeuds || r.LigneErreur || r.ColonneErreur || r.Detail || r.DetailLexical
                || r.IndexUniteErreur != unites.size() || r.NombreOctetsSource != texteAttendu.size()
                || r.NombreOrigines != unites.size() || requete.IndexFichierErreur != fichiers.size()
                || requete.LigneLocaleErreur || requete.ColonneLocaleErreur)
                ContratAssemblageIncoherent();
        };
        verifierSucces(1);
        if (mesure.NombreNoeuds > limites.MaximumNoeuds
            || mesure.NombreNoeuds >= std::numeric_limits<std::size_t>::max() / sizeof(NoeudDeclarationHote))
            throw std::length_error("limite de nœuds assemblés dépassée");
        NoeudDeclarationHote gardeNoeud;
        OrigineUniteDeclarationsHote gardeUnite;
        std::memset(&gardeNoeud, 0xA5, sizeof(gardeNoeud)); std::memset(&gardeUnite, 0xA5, sizeof(gardeUnite));
        std::vector<char> texte(texteAttendu.size() + 1, 'Z');
        std::vector<NoeudDeclarationHote> noeuds(static_cast<std::size_t>(mesure.NombreNoeuds) + 1, gardeNoeud);
        std::vector<OrigineUniteDeclarationsHote> sortiesUnites(unites.size() + 1, gardeUnite);
        assemblage.SourceAssemblee = texte.data(); assemblage.CapaciteSource = texteAttendu.size();
        assemblage.Noeuds = noeuds.data(); assemblage.CapaciteNoeuds = mesure.NombreNoeuds;
        assemblage.Origines = sortiesUnites.data(); assemblage.CapaciteOrigines = unites.size();
        requete = appeler(); verifierSucces(0);
        // L'arène brute de publication peut être plus grande que celle de mesure : ce n'est pas une capacité de sortie.
        if (assemblage.Resultat.NombreNoeuds != mesure.NombreNoeuds || texte.back() != 'Z'
            || std::memcmp(&noeuds.back(), &gardeNoeud, sizeof(gardeNoeud)) != 0
            || std::memcmp(&sortiesUnites.back(), &gardeUnite, sizeof(gardeUnite)) != 0
            || std::memcmp(texte.data(), texteAttendu.data(), texteAttendu.size()) != 0
            || std::memcmp(sortiesUnites.data(), plages.data(), plages.size() * sizeof(plages[0])) != 0)
            ContratAssemblageIncoherent();
        noeuds.pop_back(); sortiesUnites.pop_back();
        DeclarationsAssemblees resultat;
        resultat.Origines.reserve(noeuds.size());
        for (std::size_t i = 0; i < noeuds.size(); ++i)
        {
            const auto& noeud = noeuds[i];
            if (noeud.Genre > 43 || (noeud.Genre != 41 && noeud.Genre != 42 && noeud.Genre != 43 && (noeud.Drapeaux & ~0x1FFFFU))
                || noeud.DebutNom > texteAttendu.size() || noeud.TailleNom > texteAttendu.size() - noeud.DebutNom)
                ContratAssemblageIncoherent();
            if (!i)
            {
                if (noeud.Genre || noeud.Ligne != 1 || noeud.Colonne != 1 || noeud.Parent || noeud.Drapeaux
                    || noeud.DebutNom || noeud.TailleNom || noeud.HachageNom || noeud.HachageType)
                    ContratAssemblageIncoherent();
                resultat.Origines.push_back({{}, unites.size(), false});
                continue;
            }
            if (!noeud.Genre || noeud.Parent >= i || !noeud.Colonne) ContratAssemblageIncoherent();
            if (noeuds[noeud.Parent].Genre >= 40 && noeuds[noeud.Parent].Genre <= 43) ContratAssemblageIncoherent();
            if (noeud.Genre == 43)
            {
                const auto& parent = noeuds[noeud.Parent];
                if (!noeud.Parent || parent.Genre != 31 || parent.Drapeaux || parent.DebutNom || parent.TailleNom || parent.HachageNom
                    || !noeud.TailleNom || noeud.Drapeaux < 2 || noeud.Drapeaux > texteAttendu.size()
                    || noeud.HachageType != parent.HachageType || noeud.HachageEspace != parent.HachageEspace
                    || noeud.Ligne < parent.Ligne || (noeud.Ligne == parent.Ligne && noeud.Colonne <= parent.Colonne)
                    || (noeud.Parent + 1 != i && (noeuds[i - 1].Genre != 43 || noeuds[i - 1].Parent != noeud.Parent
                        || noeuds[i - 1].Drapeaux >= noeud.Drapeaux
                        || noeuds[i - 1].DebutNom + noeuds[i - 1].TailleNom > noeud.DebutNom))) ContratAssemblageIncoherent();
            }
            if (noeud.Genre == 41 || noeud.Genre == 42)
            {
                const auto& parent = noeuds[noeud.Parent]; const auto genre = parent.Genre;
                if (!noeud.Parent || (noeud.Genre == 41 && noeud.Parent + 1 != i) || (genre != 1 && genre != 2 && genre != 3
                    && genre != 7 && genre != 12 && genre != 15 && genre != 19) || !noeud.TailleNom
                    || noeud.HachageType != parent.HachageType || noeud.HachageEspace != parent.HachageEspace
                    || noeud.Drapeaux > texteAttendu.size() || noeud.DebutNom + noeud.TailleNom > parent.DebutNom) ContratAssemblageIncoherent();
                if (noeud.Genre == 42 && (noeud.Drapeaux < 2 || (noeud.Parent + 1 != i
                    && (noeuds[i - 1].Genre != 42 || noeuds[i - 1].Parent != noeud.Parent
                        || noeuds[i - 1].Drapeaux >= noeud.Drapeaux
                        || noeuds[i - 1].DebutNom + noeuds[i - 1].TailleNom > noeud.DebutNom)))) ContratAssemblageIncoherent();
            }
            if (noeud.Genre == 40 && (!noeud.Parent || noeud.Parent + 1 != i
                || noeuds[noeud.Parent].Genre != 6 || noeuds[noeud.Parent].Parent
                || !(noeuds[noeud.Parent].Drapeaux & 32) || noeud.Drapeaux || noeud.HachageType || !noeud.TailleNom
                || noeud.HachageNom != noeuds[noeud.Parent].HachageType
                || noeud.HachageEspace != noeuds[noeud.Parent].HachageEspace)) ContratAssemblageIncoherent();
            if ((noeud.Genre == 37 || noeud.Genre == 38) && (!noeud.Parent || noeud.Parent + 1 != i
                || noeuds[noeud.Parent].Genre != (noeud.Genre == 37 ? 11U : 10U)
                || noeud.Drapeaux || noeud.HachageType || !noeud.TailleNom
                || noeud.HachageNom != noeuds[noeud.Parent].HachageType
                || noeud.HachageEspace != noeuds[noeud.Parent].HachageEspace)) ContratAssemblageIncoherent();
            if (noeud.Genre == 39 && (!noeud.Parent || noeuds[noeud.Parent].Genre != 10
                || noeuds[noeud.Parent].Parent || noeud.Drapeaux || noeud.HachageType || !noeud.TailleNom
                || noeuds[i - 1].Parent != noeud.Parent
                || (noeuds[i - 1].Genre != 38 && noeuds[i - 1].Genre != 39))) ContratAssemblageIncoherent();
            const auto trouve = std::upper_bound(plages.begin(), plages.end(), noeud.Ligne,
                [](auto ligne, const auto& plage) { return ligne < plage.PremiereLigne; });
            if (trouve == plages.begin()) ContratAssemblageIncoherent();
            const auto unite = static_cast<std::size_t>(trouve - plages.begin() - 1);
            const auto& plage = plages[unite];
            const auto ligne = noeud.Ligne - plage.PremiereLigne + 1;
            if (ligne > plage.NombreLignes || (noeud.TailleNom && (noeud.DebutNom < plage.DebutOctets
                || noeud.DebutNom - plage.DebutOctets > plage.TailleOctets
                || noeud.TailleNom > plage.TailleOctets - (noeud.DebutNom - plage.DebutOctets))))
                ContratAssemblageIncoherent();
            const auto index = indexOrigine(unite, ligne, noeud.Colonne);
            resultat.Origines.push_back({positionOriginale(unite, index), unite,
                instantanes[unite].EstInterface != 0 || instantanes[unite].OriginesAvant[index].EstInterface != 0});
        }
        resultat.Resultat = assemblage.Resultat;
        resultat.Source = std::move(texteAttendu); resultat.Noeuds = std::move(noeuds);
        resultat.Unites = std::move(sortiesUnites);
        return resultat;
    }

    DeclarationsAssemblees AssemblerDeclarationsPreparees(
        std::span<const ResultatPreparationFichier* const> preparations,
        AssembleurDeclarationsAvecOriginesHote assembler, const LimitesAssemblageDeclarations& limites)
    {
        if (!assembler) throw std::invalid_argument("export d'assemblage préparé nul");
        return AssemblerDeclarationsPrepareesInterne(preparations, assembler, limites);
    }

    DeclarationsAssemblees AssemblerDeclarationsNormaliseesAvecDiagnostic(
        std::span<const ResultatPreparationFichier* const> preparations,
        AssembleurDeclarationsAvecDiagnosticHote assembler, const LimitesAssemblageDeclarations& limites)
    {
        if (!assembler || !limites.MaximumOctetsNom || limites.MaximumOctetsNom > 999'999'999)
            throw std::invalid_argument("export ou limite du diagnostic d'assemblage invalide");
        std::vector<char> nom;
        std::uint64_t tailleNom = 0, tailleAttendue = 0;
        auto appeler = [&](RequeteAssemblageAvecOriginesHote* assemblage)
        {
            RequeteDiagnosticAssemblageHote requete{assemblage, nom.empty() ? nullptr : nom.data(), tailleAttendue, 0, 0};
            const auto avant = requete;
            const auto code = assembler(&requete);
            if (requete.Assemblage != avant.Assemblage || requete.Nom != avant.Nom
                || requete.CapaciteNom != avant.CapaciteNom || requete.Reserve
                || (!nom.empty() && nom.back() != 'Z'))
                ContratAssemblageIncoherent();
            tailleNom = requete.NombreOctetsNom;
            const bool conflit = code >= 6 && code <= 10;
            if ((conflit && !tailleNom) || (!conflit && tailleNom)
                || (!nom.empty() && (!conflit || tailleNom != tailleAttendue)))
                ContratAssemblageIncoherent();
            if (tailleNom > limites.MaximumOctetsNom) throw std::length_error("limite du nom de conflit dépassée");
            return code;
        };
        auto resultat = AssemblerDeclarationsPrepareesInterne(preparations, appeler, limites);
        if (resultat.Resultat.Erreur < 6 || resultat.Resultat.Erreur > 10) return resultat;
        tailleAttendue = tailleNom;
        nom.assign(static_cast<std::size_t>(tailleNom) + 1, 'Z');
        // Les entrées ont été validées et les contrôles de lecture seule achevés avant le second appel.
        const auto seconde = AssemblerDeclarationsPrepareesInterne(preparations, appeler, limites);
        const auto& a = resultat.Resultat; const auto& b = seconde.Resultat;
        if (a.Erreur != b.Erreur || a.LigneErreur != b.LigneErreur || a.ColonneErreur != b.ColonneErreur
            || a.Detail != b.Detail || a.IndexUniteErreur != b.IndexUniteErreur
            || a.NombreOctetsSource != b.NombreOctetsSource || a.NombreOrigines != b.NombreOrigines
            || a.DetailLexical != b.DetailLexical || !seconde.PositionErreur
            || resultat.PositionErreur->Fichier != seconde.PositionErreur->Fichier
            || resultat.PositionErreur->Ligne != seconde.PositionErreur->Ligne
            || resultat.PositionErreur->Colonne != seconde.PositionErreur->Colonne)
            ContratAssemblageIncoherent();
        resultat.NomConflit.assign(nom.data(), static_cast<std::size_t>(tailleNom));
        if (!NomConflitValide(resultat.NomConflit)) ContratAssemblageIncoherent();
        return resultat;
    }
}
