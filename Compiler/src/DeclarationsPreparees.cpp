#include "GsPP/DeclarationsPreparees.hpp"
#include "GsPP/ErreurCompilation.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>

namespace GsPP
{
    namespace
    {
        [[noreturn]] void ContratIncoherent()
        {
            throw std::runtime_error("sorties de l'analyseur Gs++ incompatibles avec le contrat d'AST préparé");
        }

        struct PositionJetonPrepare
        {
            std::uint32_t Ligne, Colonne;
        };

        struct MessageDeclaration
        {
            std::uint32_t Code;
            const char* Francais;
            const char* Anglais;
        };

        constexpr std::array<MessageDeclaration, 26> MessagesGeneriques{{
            {5, "identifiant attendu", "expected identifier"},
            {6, "type attendu", "expected type"},
            {7, "'(' attendue", "expected '('"},
            {8, "')' attendue", "expected ')'"},
            {9, "'{' attendue", "expected '{'"},
            {10, "'}' attendue", "expected '}'"},
            {11, "';' attendu", "expected ';'"},
            {12, "déclaration non prise en charge", "unsupported declaration"},
            {13, "taille de tableau invalide", "invalid array size"},
            {14, "expression attendue", "expected expression"},
            {15, "une variable globale externe ne peut pas avoir d’initialiseur", "an external global variable cannot have an initializer"},
            {16, "membre de classe non pris en charge", "unsupported class member"},
            {17, "']' attendu", "expected ']'"},
            {18, "'=' attendu dans l’alias", "expected '=' in alias"},
            {19, "':' attendu après la visibilité", "expected ':' after visibility"},
            {20, "un initialiseur de champ par défaut est réservé aux classes", "a default field initializer is only allowed in classes"},
            {21, "opérateur surchargeable attendu", "expected overloadable operator"},
            {22, "modificateur de membre répété", "duplicate member modifier"},
            {23, "un constructeur ne peut pas être virtuel ou remplacer une méthode", "a constructor cannot be virtual or override a method"},
            {24, "un destructeur ne reçoit aucun paramètre explicite", "a destructor takes no explicit parameter"},
            {25, "une liste d’initialisation est réservée aux constructeurs", "an initializer list is only allowed on constructors"},
            {26, "initialiseur de constructeur invalide", "invalid constructor initializer"},
            {27, "'<' attendu après convertir", "expected '<' after cast"},
            {28, "'>' attendu après le type", "expected '>' after cast type"},
            {29, "nombre entier invalide ou trop grand", "invalid or oversized integer"},
            {30, "'espace' attendu après 'utilisant'", "expected 'namespace' after 'using'"}
        }};

        // Indices 1..43 du contrat DetailDiagnosticDeclarations ; 0 conserve la catégorie historique.
        constexpr std::array<MessageDeclaration, 43> MessagesContextuels{{
            {5, "nom attendu après '::'", "expected name after '::'"},
            {5, "nom de fonction attendu", "expected function name"},
            {6, "'<' attendu après pointeur_fonction", "expected '<' after function_pointer"},
            {7, "'(' attendue dans la signature de fonction", "expected '(' in function signature"},
            {8, "')' attendue dans la signature de fonction", "expected ')' in function signature"},
            {6, "'>' attendu après la signature de fonction", "expected '>' after function signature"},
            {13, "taille entière attendue dans le tableau", "integer array size expected"},
            {5, "nom de structure attendu", "expected structure name"},
            {19, "':' attendu après la visibilité", "expected ':' after visibility"},
            {5, "nom d’alias de champ attendu", "expected field alias name"},
            {5, "champ cible attendu", "expected target field"},
            {5, "nom de membre attendu", "expected member name"},
            {5, "nom de champ attendu", "expected field name"},
            {11, "';' attendu après la structure", "expected ';' after struct"},
            {22, "modificateur virtuel répété", "duplicate virtual modifier"},
            {22, "modificateur remplacer répété", "duplicate override modifier"},
            {5, "nom d’énumération attendu", "expected enum name"},
            {5, "nom d’énumérateur attendu", "expected enumerator name"},
            {11, "';' attendu après l’énumération", "expected ';' after enum"},
            {12, "une fonction ne peut pas retourner un tableau", "a function cannot return an array"},
            {5, "nom de paramètre attendu", "expected parameter name"},
            {26, "soi doit être l’unique initialiseur d’un constructeur délégué", "this must be the only initializer of a delegating constructor"},
            {26, "un constructeur délégué ne peut pas initialiser directement parent ou un champ", "a delegating constructor cannot directly initialize super or a field"},
            {26, "parent doit être le premier initialiseur", "super must be the first initializer"},
            {5, "nom de champ attendu dans la liste d’initialisation", "expected field name in initializer list"},
            {11, "';' attendu après la déclaration externe", "expected ';' after extern declaration"},
            {7, "'(' attendue après soi", "expected '(' after this"},
            {8, "')' attendue après l’initialiseur soi", "expected ')' after this initializer"},
            {7, "'(' attendue après parent", "expected '(' after super"},
            {8, "')' attendue après l’initialiseur parent", "expected ')' after super initializer"},
            {7, "'(' attendue après le nom du champ", "expected '(' after field name"},
            {8, "')' attendue après l’initialiseur de champ", "expected ')' after field initializer"},
            {15, "une déclaration externe ne porte pas de liste d’initialisation", "an external declaration cannot have an initializer list"},
            {7, "'(' attendue après 'si'", "expected '(' after 'if'"},
            {7, "'(' attendue après 'tantque'", "expected '(' after 'while'"},
            {5, "nom de variable attendu", "expected variable name"},
            {10, "'}' attendue après l’initialiseur agrégé", "expected '}' after aggregate initializer"},
            {18, "'=' attendu dans l’alias", "expected '=' in alias"},
            {11, "';' attendu après l’alias", "expected ';' after alias"},
            {11, "';' attendu après l'utilisation d'espace", "expected ';' after using namespace"},
            {6, "fin de fichier attendue", "expected end of file"},
            {10, "type attendu", "expected type"},
            {5, "nom de membre attendu", "expected member name"}
        }};

        const MessageDeclaration& MessageRefus(const ResultatAnalyseDeclarationsHote& resultat)
        {
            if (resultat.Erreur < 5 || resultat.Erreur > 30 || resultat.Detail > MessagesContextuels.size())
                ContratIncoherent();
            const auto& message = resultat.Detail
                ? MessagesContextuels[resultat.Detail - 1] : MessagesGeneriques[resultat.Erreur - 5];
            if (message.Code != resultat.Erreur) ContratIncoherent();
            return message;
        }
    }

    std::string_view DeclarationsPreparees::Nom(std::size_t index) const
    {
        const auto& noeud = Noeuds.at(index);
        if (noeud.DebutNom > Source.size() || noeud.TailleNom > Source.size() - noeud.DebutNom)
            ContratIncoherent();
        return std::string_view(Source).substr(static_cast<std::size_t>(noeud.DebutNom),
            static_cast<std::size_t>(noeud.TailleNom));
    }

    void DeclarationsPreparees::ExigerValide() const
    {
        if (!Resultat.Erreur && !PositionErreur) return;
        const auto& message = MessageRefus(Resultat);
        if (!PositionErreur || !PositionErreur->Ligne || !PositionErreur->Colonne
            || PositionErreur->Fichier.empty() || Resultat.Reserve)
            ContratIncoherent();
        throw ErreurCompilation(message.Francais, message.Anglais, PositionErreur->Ligne,
            PositionErreur->Colonne, PositionErreur->Fichier);
    }

    DeclarationsPreparees AnalyserDeclarationsPreparees(
        const ResultatPreparationFichier& preparation, AnalyseurDeclarationsAvecOriginesHote analyser,
        bool estInterface, const LimitesDeclarationsPreparees& limites)
    {
        if (!analyser) throw std::invalid_argument("analyseur de déclarations avec origines absent");
        if (!limites.MaximumOctets || limites.MaximumOctets > 1'000'000'000
            || !limites.MaximumJetons || limites.MaximumJetons > 100'000'000
            || !limites.MaximumNoeuds || limites.MaximumNoeuds > 100'000'000)
            throw std::invalid_argument("limites de l'AST préparé invalides");
        if (preparation.Preparation.Source.size() > limites.MaximumOctets
            || preparation.Preparation.Origines.size() > limites.MaximumJetons)
            throw std::length_error("limite d'entrée de l'AST préparé dépassée");
        // Vérifie les lexèmes, EOF, modes et catalogue, sans analyse syntaxique C++.
        (void)ConvertirPreparationEnJetons(preparation);
        std::string source = preparation.Preparation.Source;
        auto origines = preparation.Preparation.Origines;
        const auto& catalogue = preparation.Catalogue;
        const auto nombreFichiers = catalogue.Fichiers().size();
        const auto racine = catalogue.IndexRacine();
        const auto& nomUnite = catalogue.NomsFichiers()[racine];
        const auto modeInterface = estInterface || catalogue.Fichiers()[racine].EstInterface != 0;
        if (nombreFichiers > 1'000'000) throw std::length_error("catalogue de l'AST préparé trop grand");

        // Un seul parcours du texte puis recherches binaires ; pas de relecture par nœud.
        std::vector<PositionJetonPrepare> positions;
        positions.reserve(origines.size());
        std::size_t octet = source.starts_with("\xEF\xBB\xBF") ? 3 : 0;
        std::uint32_t ligne = 1, colonne = 1;
        for (const auto& origine : origines)
        {
            while (octet < origine.DebutOctets)
            {
                if (source[octet++] == '\n') { ++ligne; colonne = 1; }
                else ++colonne;
            }
            positions.push_back({ligne, colonne});
        }
        auto indexOrigine = [&](std::uint32_t l, std::uint32_t c) -> std::size_t
        {
            const auto trouve = std::lower_bound(positions.begin(), positions.end(), std::pair{l, c},
                [](const PositionJetonPrepare& p, const auto& valeur)
                { return std::pair{p.Ligne, p.Colonne} < valeur; });
            if (trouve == positions.end() || trouve->Ligne != l || trouve->Colonne != c) ContratIncoherent();
            return static_cast<std::size_t>(trouve - positions.begin());
        };
        auto positionOriginale = [&](std::size_t index)
        {
            const auto& origine = origines[index];
            return PositionSource{catalogue.NomsFichiers()[origine.IndexFichier], origine.Ligne,
                origine.Colonne, nomUnite, index};
        };

        RequeteAnalyseDeclarationsHote analyse{source.data(), source.size(), nullptr, 0, {}};
        auto appeler = [&]()
        {
            const auto* noeuds = analyse.Noeuds;
            const auto capacite = analyse.Capacite;
            RequeteDeclarationsAvecOriginesHote requete{&analyse, origines.data(), origines.size(),
                nombreFichiers, nombreFichiers, 0, 0, modeInterface ? 1U : 0U, 0};
            const auto code = analyser(&requete);
            if (analyse.Source != source.data() || analyse.Taille != source.size()
                || analyse.Noeuds != noeuds || analyse.Capacite != capacite || requete.Analyse != &analyse
                || requete.Origines != origines.data() || requete.NombreOrigines != origines.size()
                || requete.NombreFichiers != nombreFichiers || requete.EstInterface != (modeInterface ? 1U : 0U)
                || requete.Reserve || source != preparation.Preparation.Source
                || std::memcmp(origines.data(), preparation.Preparation.Origines.data(),
                    origines.size() * sizeof(OrigineJetonDeclarationsHote)) != 0
                || code != analyse.Resultat.Erreur || code > 30 || analyse.Resultat.Reserve)
                ContratIncoherent();
            if (code == 4) throw std::bad_alloc();
            if (code == 2 || code == 3) ContratIncoherent();
            if (analyse.Resultat.NombreNoeuds > origines.size()
                || analyse.Resultat.CapaciteRequise != analyse.Resultat.NombreNoeuds)
                ContratIncoherent();
            return requete;
        };
        auto requete = appeler();
        if (analyse.Resultat.Erreur > 4)
        {
            (void)MessageRefus(analyse.Resultat);
            const auto index = indexOrigine(analyse.Resultat.LigneErreur, analyse.Resultat.ColonneErreur);
            const auto& origine = origines[index];
            if (requete.IndexFichierErreur != origine.IndexFichier
                || requete.LigneLocaleErreur != origine.Ligne || requete.ColonneLocaleErreur != origine.Colonne)
                ContratIncoherent();
            DeclarationsPreparees refus;
            refus.Resultat = analyse.Resultat;
            refus.PositionErreur = positionOriginale(index);
            return refus;
        }
        if (analyse.Resultat.Erreur != 1 || !analyse.Resultat.NombreNoeuds
            || analyse.Resultat.LigneErreur || analyse.Resultat.ColonneErreur || analyse.Resultat.Detail
            || requete.IndexFichierErreur != nombreFichiers || requete.LigneLocaleErreur || requete.ColonneLocaleErreur)
            ContratIncoherent();
        const auto mesure = analyse.Resultat;
        if (mesure.NombreNoeuds > limites.MaximumNoeuds
            || mesure.NombreNoeuds >= std::numeric_limits<std::size_t>::max() / sizeof(NoeudDeclarationHote))
            throw std::length_error("limite de nœuds de l'AST préparé dépassée");
        const auto nombre = static_cast<std::size_t>(mesure.NombreNoeuds);
        NoeudDeclarationHote garde;
        std::memset(&garde, 0xA5, sizeof(garde));
        std::vector<NoeudDeclarationHote> noeuds(nombre + 1, garde);
        analyse.Noeuds = noeuds.data(); analyse.Capacite = nombre;
        requete = appeler();
        if (analyse.Resultat.Erreur || analyse.Resultat.NombreNoeuds != mesure.NombreNoeuds
            || analyse.Resultat.NombreOctetsArene != mesure.NombreOctetsArene
            || analyse.Resultat.LigneErreur || analyse.Resultat.ColonneErreur || analyse.Resultat.Detail
            || requete.IndexFichierErreur != nombreFichiers || requete.LigneLocaleErreur || requete.ColonneLocaleErreur
            || std::memcmp(&noeuds.back(), &garde, sizeof(garde)) != 0)
            ContratIncoherent();
        noeuds.pop_back();
        DeclarationsPreparees resultat;
        resultat.Origines.reserve(nombre);
        for (std::size_t i = 0; i < nombre; ++i)
        {
            const auto& noeud = noeuds[i];
            if (noeud.Genre > 36 || (noeud.Drapeaux & ~0x1FFFFU)
                || noeud.DebutNom > source.size() || noeud.TailleNom > source.size() - noeud.DebutNom)
                ContratIncoherent();
            if (!i)
            {
                if (noeud.Genre || noeud.Ligne != 1 || noeud.Colonne != 1 || noeud.Parent
                    || noeud.Drapeaux || noeud.DebutNom || noeud.TailleNom || noeud.HachageNom || noeud.HachageType)
                    ContratIncoherent();
                resultat.Origines.push_back({PositionSource{nomUnite, 1, 1, nomUnite, 0}, modeInterface});
                continue;
            }
            if (!noeud.Genre || noeud.Parent >= i || !noeud.Ligne || !noeud.Colonne) ContratIncoherent();
            const auto index = indexOrigine(noeud.Ligne, noeud.Colonne);
            resultat.Origines.push_back({positionOriginale(index), modeInterface || origines[index].EstInterface != 0});
        }
        resultat.Resultat = analyse.Resultat;
        resultat.Source = std::move(source);
        resultat.Noeuds = std::move(noeuds);
        return resultat;
    }
}
