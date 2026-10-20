#include "GsPP/SemantiqueAssemblee.hpp"
#include "GsPP/ErreurCompilation.hpp"
#include "GsPP/Lexeur.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <map>
#include <new>
#include <stdexcept>
#include <utility>

namespace GsPP
{
    namespace
    {
        [[noreturn]] void ContratSemantiqueIncoherent(const char* contexte = "entrée ou sortie")
        {
            throw std::runtime_error(std::string("contrat de sémantique assemblée incohérent : ") + contexte);
        }

        std::uint32_t GenreSymbole(std::uint32_t genre)
        {
            if (genre == 4 || genre == 5 || genre == 6 || genre == 8) return 1;
            if (genre == 1 || (genre >= 12 && genre <= 15)) return 2;
            if (genre == 3) return 3;
            if (genre == 10) return 4;
            if (genre == 7) return 5;
            if (genre == 11) return 6;
            if (genre == 9) return 7;
            if (genre == 2) return 8;
            if (genre == 19) return 9;
            return 0;
        }

        struct MessageSemantique
        {
            std::uint32_t Code;
            const char* Francais;
            const char* Anglais;
        };

        constexpr std::array<MessageSemantique, 47> MessagesSemantiques{{
            {16, "paramètre déclaré plusieurs fois", "parameter declared more than once"},
            {20, "le programme ne contient aucune fonction", "the program contains no function"},
            {39, "une classe possédant une valeur de champ par défaut doit déclarer un constructeur", "a class with a default field value must declare a constructor"},
            {42, "trop d’éléments dans l’initialiseur de tableau", "too many elements in array initializer"},
            {43, "trop d’éléments dans l’initialiseur de structure", "too many elements in struct initializer"},
            {44, "un initialiseur scalaire agrégé accepte au maximum une valeur", "a scalar aggregate initializer accepts at most one value"},
            {47, "'&' exige une valeur adressable", "'&' requires an addressable value"},
            {48, "les pointeurs vers tableaux complets ne sont pas encore pris en charge", "pointers to complete arrays are not supported yet"},
            {49, "'*' exige un pointeur", "'*' requires a pointer"},
            {50, "l’indexation exige un pointeur ou un tableau", "indexing requires a pointer or array"},
            {51, "l’indice doit être entier", "index must be an integer"},
            {52, "l’indexation de vide* est interdite", "indexing void* is forbidden"},
            {54, "nombre d’arguments incorrect", "incorrect argument count"},
            {60, "'!' exige une valeur scalaire", "'!' requires a scalar value"},
            {61, "opérateur unaire réservé aux entiers", "unary operator requires an integer"},
            {63, "opérandes de types différents", "operands have different types"},
            {64, "un décalage binaire exige deux entiers", "a bit shift requires two integers"},
            {65, "calcul arithmétique réservé aux entiers", "arithmetic requires integers"},
            {66, "comparaison de ce type non prise en charge", "comparison of this type is unsupported"},
            {67, "seules == et != sont permises sur les pointeurs", "only == and != are allowed for pointers"},
            {68, "seules == et != sont permises sur les booléens", "only == and != are allowed for bool"},
            {70, "cible d’affectation non modifiable", "assignment target is not modifiable"},
            {71, "affectation d’une valeur constante interdite", "cannot assign to a const value"},
            {72, "copie de tableau non prise en charge", "array assignment is unsupported"},
            {74, "valeur de retour attendue", "return value expected"},
            {76, "les objets classes globaux ne sont pas pris en charge : utilisez une durée de vie locale ou un stockage initialisé explicitement", "global class objects are unsupported: use local lifetime or explicitly initialized storage"},
            {77, "les références globales ne sont pas prises en charge", "global references are unsupported"},
            {78, "une variable globale ne peut pas être 'vide'", "a global variable cannot be 'void'"},
            {79, "une variable globale importée ne peut pas être exportée", "an imported global variable cannot be exported"},
            {80, "une globale constante doit être initialisée", "a const global must be initialized"},
            {83, "initialisation globale de pointeur de données non prise en charge", "global data-pointer initialization is unsupported"},
            {85, "la valeur d’énumération doit être une constante entière", "enum value must be an integer constant"},
            {86, "la valeur d’énumération doit être constante", "enum value must be constant"},
            {87, "valeur d’énumération hors de la plage entier32", "enum value is outside int32 range"},
            {88, "débordement de la valeur d’énumération suivante", "next enum value would overflow"},
            {89, "division constante par zéro", "constant division by zero"},
            {94, "type cible de conversion non scalaire", "cast target is not scalar"},
            {95, "source de conversion non scalaire", "cast source is not scalar"},
            {96, "une conversion entre pointeur et entier est interdite", "casts between pointers and integers are forbidden"},
            {97, "conversion entre signatures de fonction incompatibles", "cast between incompatible function signatures"},
            {101, "une référence de tableau n’est pas prise en charge", "array references are unsupported"},
            {113, "Gs++ prend en charge uniquement l’héritage public", "Gs++ supports public inheritance only"},
            {115, "une classe ne peut pas hériter d’elle-même", "a class cannot inherit from itself"},
            {122, "la syntaxe de construction exige un type classe", "constructor syntax requires a class type"},
            {123, "une variable ne peut pas être 'vide'", "a variable cannot be 'void'"},
            {124, "une référence doit être initialisée", "a reference must be initialized"},
            {125, "une variable constante doit être initialisée", "a const variable must be initialized"}
        }};

        const MessageSemantique* MessageSemantiqueTraduisible(std::uint32_t code) noexcept
        {
            const auto trouve = std::find_if(MessagesSemantiques.begin(), MessagesSemantiques.end(),
                [&](const auto& message) { return message.Code == code; });
            return trouve == MessagesSemantiques.end() ? nullptr : &*trouve;
        }

        constexpr std::array<MessageSemantique, 8> MessagesContextuels{{
            {46, "un tableau exige un initialiseur agrégé", "an array requires an aggregate initializer"},
            {46, "un tableau global exige un initialiseur agrégé", "a global array requires an aggregate initializer"},
            {62, "un opérateur logique exige deux valeurs scalaires", "a logical operator requires two scalar values"},
            {62, "condition non scalaire", "condition is not scalar"},
            {23, "'.' exige une structure", "'.' requires a struct"},
            {23, "'->' exige un pointeur vers structure", "'->' requires a pointer to struct"},
            {23, "'.' exige une classe", "'.' requires a class"},
            {23, "'->' exige un pointeur vers classe", "'->' requires a pointer to class"}
        }};

        constexpr std::uint64_t HacherTexteDiagnostic(std::string_view texte) noexcept
        {
            std::uint64_t hachage = 14'695'981'039'346'656'037ULL;
            for (const auto c : texte) hachage = (hachage ^ static_cast<unsigned char>(c)) * 1'099'511'628'211ULL;
            return hachage;
        }

        bool OrigineDiagnosticCorrespond(const SemantiqueAssemblee& resultat, std::size_t index) noexcept
        {
            if (!resultat.PositionErreur || index >= resultat.Declarations.Origines.size()) return false;
            const auto& n = resultat.Declarations.Noeuds[index];
            const auto& o = resultat.Declarations.Origines[index];
            const auto& p = *resultat.PositionErreur;
            return n.Ligne == resultat.Resultat.LigneErreur && n.Colonne == resultat.Resultat.ColonneErreur
                && o.IndexUnite == resultat.IndexUniteErreur && o.Position.Fichier == p.Fichier
                && o.Position.Ligne == p.Ligne && o.Position.Colonne == p.Colonne
                && o.Position.Unite == p.Unite && o.Position.Ordre == p.Ordre;
        }

        bool EstAncetreDiagnostic(const std::vector<NoeudDeclarationHote>& noeuds,
            std::size_t ancetre, std::size_t enfant) noexcept
        {
            while (enfant > ancetre && noeuds[enfant].Genre && noeuds[enfant].Parent < enfant)
                enfant = noeuds[enfant].Parent;
            return enfant == ancetre;
        }

        /** <résumé>Choisit un message seulement si le contexte possédé est complet et non ambigu.</résumé> **/
        const MessageSemantique* MessageSemantiqueTraduisible(const SemantiqueAssemblee& resultat) noexcept
        {
            const auto code = resultat.Resultat.Erreur;
            if (const auto* invariant = MessageSemantiqueTraduisible(code)) return invariant;
            if ((code != 23 && code != 46 && code != 62) || !resultat.PositionErreur
                || resultat.Declarations.Origines.size() != resultat.Declarations.Noeuds.size()) return nullptr;
            const auto& noeuds = resultat.Declarations.Noeuds;
            if (code == 23)
            {
                if (resultat.Resultat.Detail || resultat.IndexUniteErreur >= resultat.Declarations.Unites.size()) return nullptr;
                std::size_t membre = 0;
                for (std::size_t i = 1; i < noeuds.size(); ++i)
                    if (OrigineDiagnosticCorrespond(resultat, i) && noeuds[i].Genre == 29)
                    {
                        if (membre || noeuds[i].Parent >= i || (noeuds[i].Drapeaux & ~32768U)) return nullptr;
                        membre = i;
                    }
                if (!membre || membre + 1 >= noeuds.size() || noeuds[membre + 1].Parent != membre
                    || noeuds[membre + 1].Genre < 22 || noeuds[membre + 1].Genre > 32) return nullptr;
                for (std::size_t i = 1; i < noeuds.size(); ++i)
                    if (OrigineDiagnosticCorrespond(resultat, i) && i != membre
                        && (!noeuds[i].Genre || (!EstAncetreDiagnostic(noeuds, i, membre)
                            && !EstAncetreDiagnostic(noeuds, membre, i)))) return nullptr;
                const auto& n = noeuds[membre];
                const bool cibleAppel = membre == n.Parent + 1 && noeuds[n.Parent].Genre == 28;
                return &MessagesContextuels[4 + ((n.Drapeaux & 32768U) ? 1 : 0) + (cibleAppel ? 2 : 0)];
            }
            const MessageSemantique* message = nullptr;
            std::size_t indexContexte = 0;
            for (std::size_t i = 1; i < noeuds.size(); ++i)
            {
                if (!OrigineDiagnosticCorrespond(resultat, i)) continue;
                const auto& n = noeuds[i];
                const MessageSemantique* candidat = nullptr;
                if (code == 46 && resultat.Resultat.Detail)
                {
                    if (n.Genre == 19) candidat = &MessagesContextuels[0];
                    if (n.Genre == 3) candidat = &MessagesContextuels[1];
                }
                if (code == 62 && !resultat.Resultat.Detail && n.Genre == 26
                    && (n.HachageNom == HacherTexteDiagnostic("&&") || n.HachageNom == HacherTexteDiagnostic("||")))
                    candidat = &MessagesContextuels[2];
                if (code == 62 && resultat.Resultat.Detail == 1 && n.Parent < i && i == n.Parent + 1
                    && n.Genre >= 22 && n.Genre <= 32
                    && (noeuds[n.Parent].Genre == 20 || noeuds[n.Parent].Genre == 21))
                    candidat = &MessagesContextuels[3];
                if (!candidat)
                {
                    if (code == 46 && message && n.Genre == 41 && i == indexContexte + 1
                        && n.Parent == indexContexte && !n.Drapeaux && n.TailleNom
                        && n.HachageType == noeuds[indexContexte].HachageType
                        && n.HachageEspace == noeuds[indexContexte].HachageEspace
                        && n.DebutNom <= noeuds[indexContexte].DebutNom
                        && n.TailleNom <= noeuds[indexContexte].DebutNom - n.DebutNom) continue;
                    // Un appel et sa cible peuvent partager la position de la condition.
                    // Seuls les descendants expressions du contexte déjà identifié sont admis.
                    auto descendant = i;
                    while (descendant > indexContexte && noeuds[descendant].Genre >= 22
                        && noeuds[descendant].Genre <= 32 && noeuds[descendant].Parent < descendant)
                        descendant = noeuds[descendant].Parent;
                    if (code != 62 || !message || descendant != indexContexte) return nullptr;
                    continue;
                }
                if (message) return nullptr;
                message = candidat;
                indexContexte = i;
            }
            return message;
        }

        /** <résumé>Identifie une référence unique au nom possédé, sans interpréter son hachage.</résumé> **/
        std::size_t ReferenceDiagnosticPossede(const SemantiqueAssemblee& resultat) noexcept
        {
            const auto code = resultat.Resultat.Erreur;
            if ((code != 18 && code != 19 && code != 24 && code != 121)
                || ((code == 18 || code == 24 || code == 121) && resultat.Resultat.Detail)
                || (code == 19 && (resultat.Resultat.Detail < 2 || resultat.Resultat.Detail > resultat.Resultat.NombreSymboles))
                || !resultat.PositionErreur
                || resultat.IndexUniteErreur >= resultat.Declarations.Unites.size()
                || resultat.Declarations.Noeuds.size() != resultat.Declarations.Origines.size()) return 0;
            const auto& noeuds = resultat.Declarations.Noeuds;
            const auto genreReference = code == 24 ? 29U : 24U;
            std::size_t reference = 0;
            for (std::size_t i = 1; i < noeuds.size(); ++i)
                if (OrigineDiagnosticCorrespond(resultat, i) && noeuds[i].Genre == genreReference)
                {
                    if (reference || noeuds[i].Parent >= i || (noeuds[i].Drapeaux & 65536U)
                        || (code == 24 && (noeuds[i].Drapeaux & ~32768U))
                        || (code == 121 && (!noeuds[i].Parent || noeuds[i].Drapeaux || noeuds[i].HachageType))) return 0;
                    reference = i;
                }
            if (!reference) return 0;
            const auto& n = noeuds[reference];
            if (code == 121 && (resultat.Resultat.NombreSymboles != static_cast<std::uint64_t>(std::count_if(
                    noeuds.begin(), noeuds.end(), [](const auto& candidat) { return GenreSymbole(candidat.Genre) != 0; }))
                || std::any_of(noeuds.begin() + 1, noeuds.end(), [&](const auto& enfant) { return enfant.Parent == reference; }))) return 0;
            if (code == 19 && reference == n.Parent + 1 && noeuds[n.Parent].Genre == 28) return 0;
            const auto& unite = resultat.Declarations.Unites[resultat.IndexUniteErreur];
            if (!n.TailleNom || n.DebutNom > resultat.Declarations.Source.size()
                || n.TailleNom > resultat.Declarations.Source.size() - n.DebutNom
                || n.DebutNom < unite.DebutOctets || n.DebutNom - unite.DebutOctets > unite.TailleOctets
                || n.TailleNom > unite.TailleOctets - (n.DebutNom - unite.DebutOctets)) return 0;
            for (std::size_t i = 1; i < noeuds.size(); ++i)
                if (OrigineDiagnosticCorrespond(resultat, i) && i != reference
                    && (!noeuds[i].Genre || (!EstAncetreDiagnostic(noeuds, i, reference)
                        && !EstAncetreDiagnostic(noeuds, reference, i)))) return 0;
            return reference;
        }

        bool NomDiagnosticDansUnite(const SemantiqueAssemblee& resultat, std::size_t index) noexcept
        {
            const auto& d = resultat.Declarations;
            if (index >= d.Noeuds.size() || index >= d.Origines.size()
                || d.Origines[index].IndexUnite >= d.Unites.size()) return false;
            const auto& n = d.Noeuds[index]; const auto& u = d.Unites[d.Origines[index].IndexUnite];
            return n.TailleNom && n.DebutNom <= d.Source.size() && n.TailleNom <= d.Source.size() - n.DebutNom
                && n.DebutNom >= u.DebutOctets && n.DebutNom - u.DebutOctets <= u.TailleOctets
                && n.TailleNom <= u.TailleOctets - (n.DebutNom - u.DebutOctets);
        }

        /**
         * <résumé>Relie le code 120 au nom écrit de l'utilisation, sans rechercher de namespace.</résumé>
         * @etc. Detail reste nul ; la passe d'import précède les résolutions sémantiques.
         * @Retourner(std::size_t) Nœud nominal 36 unique à l'origine possédée, ou zéro si incohérent.
         **/
        std::size_t EspaceUtiliseDiagnostic(const SemantiqueAssemblee& resultat) noexcept
        {
            if (resultat.Resultat.Erreur != 120 || resultat.Resultat.Detail || !resultat.PositionErreur
                || resultat.Resultat.NombreResolutions || resultat.IndexUniteErreur >= resultat.Declarations.Unites.size()
                || resultat.Declarations.Noeuds.size() != resultat.Declarations.Origines.size()) return 0;
            const auto& d = resultat.Declarations;
            if (resultat.Resultat.NombreSymboles != static_cast<std::uint64_t>(std::count_if(
                d.Noeuds.begin(), d.Noeuds.end(), [](const auto& n) { return GenreSymbole(n.Genre) != 0; }))) return 0;
            std::size_t reference = 0;
            for (std::size_t i = 1; i < d.Noeuds.size(); ++i)
                if (OrigineDiagnosticCorrespond(resultat, i))
                {
                    const auto& n = d.Noeuds[i];
                    if (reference || n.Genre != 36 || n.Parent || n.Drapeaux || n.HachageType
                        || !NomDiagnosticDansUnite(resultat, i)) return 0;
                    const auto& u = d.Unites[resultat.IndexUniteErreur];
                    if (!n.Colonne || n.Ligne < u.PremiereLigne || n.Ligne - u.PremiereLigne >= u.NombreLignes) return 0;
                    reference = i;
                }
            if (!reference || std::any_of(d.Noeuds.begin() + 1, d.Noeuds.end(),
                [&](const auto& n) { return n.Parent == reference; })) return 0;
            return reference;
        }

        struct AccesDiagnosticPossede
        {
            std::size_t Reference = 0, Cible = 0;
        };

        /**
         * <résumé>Relie le nom de type choisi par la conversion à son contexte possédé, sans le résoudre.</résumé>
         * @etc. Les feuilles 43 précèdent l'unique opérande ; les ancêtres à la même position restent autorisés.
         **/
        AccesDiagnosticPossede NomTypeConversionAmbiguDiagnostic(const SemantiqueAssemblee& resultat) noexcept
        {
            const auto& d = resultat.Declarations; const auto cible = static_cast<std::size_t>(resultat.Resultat.Detail);
            if (resultat.Resultat.Erreur != 121 || !cible || !resultat.PositionErreur
                || resultat.IndexUniteErreur >= d.Unites.size() || d.Noeuds.size() != d.Origines.size()
                || cible >= d.Noeuds.size()) return {};
            const auto& nominal = d.Noeuds[cible]; const auto reference = static_cast<std::size_t>(nominal.Parent);
            if (nominal.Genre != 43 || !reference || reference >= cible) return {};
            const auto& conversion = d.Noeuds[reference];
            if (conversion.Genre != 31 || conversion.Parent >= reference || !conversion.Parent
                || conversion.Drapeaux || conversion.DebutNom || conversion.TailleNom || conversion.HachageNom
                || !OrigineDiagnosticCorrespond(resultat, reference)
                || d.Origines[reference].IndexUnite != resultat.IndexUniteErreur) return {};
            const auto genreParent = d.Noeuds[conversion.Parent].Genre;
            if ((genreParent != 3 && genreParent != 7 && (genreParent < 16 || genreParent > 35))
                || (genreParent >= 22 && genreParent <= 24)) return {};
            const auto& unite = d.Unites[resultat.IndexUniteErreur];
            const auto& origine = d.Origines[reference].Position;
            if (!conversion.Colonne || conversion.Ligne < unite.PremiereLigne
                || conversion.Ligne - unite.PremiereLigne >= unite.NombreLignes
                || resultat.Resultat.NombreSymboles != static_cast<std::uint64_t>(std::count_if(
                    d.Noeuds.begin(), d.Noeuds.end(), [](const auto& n) { return GenreSymbole(n.Genre) != 0; }))) return {};
            auto valeur = reference + 1;
            while (valeur < d.Noeuds.size() && d.Noeuds[valeur].Genre == 43 && d.Noeuds[valeur].Parent == reference) ++valeur;
            if (cible >= valeur || valeur >= d.Noeuds.size() || d.Noeuds[valeur].Parent != reference
                || d.Noeuds[valeur].Genre < 22 || d.Noeuds[valeur].Genre > 32
                || d.Origines[valeur].IndexUnite != resultat.IndexUniteErreur) return {};
            for (std::size_t i = 1; i < d.Noeuds.size(); ++i)
            {
                const auto& n = d.Noeuds[i];
                if (OrigineDiagnosticCorrespond(resultat, i) && i != reference
                    && !EstAncetreDiagnostic(d.Noeuds, i, reference)) return {};
                if (n.Parent >= reference + 1 && n.Parent < valeur) return {};
                if (n.Parent != reference) continue;
                if (i == valeur) continue;
                if (n.Genre != 43 || i >= valeur || !NomDiagnosticDansUnite(resultat, i)
                    || d.Origines[i].IndexUnite != resultat.IndexUniteErreur
                    || n.HachageType != conversion.HachageType || n.HachageEspace != conversion.HachageEspace
                    || n.Drapeaux < 2 || n.Drapeaux > d.Source.size() || n.Ligne < conversion.Ligne
                    || !n.Colonne || n.Ligne < unite.PremiereLigne || n.Ligne - unite.PremiereLigne >= unite.NombreLignes) return {};
                const auto& origineNom = d.Origines[i].Position;
                if (origineNom.Fichier.empty() || !origineNom.Ligne || !origineNom.Colonne || origineNom.Unite != origine.Unite
                    || origineNom.Ordre < origine.Ordre || origineNom.Ordre - origine.Ordre != n.Drapeaux
                    || origineNom.Ordre >= d.Origines[valeur].Position.Ordre
                    || (i > reference + 1 && (d.Noeuds[i - 1].Drapeaux >= n.Drapeaux
                        || d.Noeuds[i - 1].DebutNom + d.Noeuds[i - 1].TailleNom > n.DebutNom))) return {};
            }
            return {reference, cible};
        }

        /**
         * <résumé>Relie le code 121 au nom direct ou imbriqué du type, sans le résoudre.</résumé>
         * @etc. Detail indexe la feuille 41/42 ; Drapeaux conserve le décalage lexical depuis la déclaration.
         **/
        AccesDiagnosticPossede NomTypeAmbiguDiagnostic(const SemantiqueAssemblee& resultat) noexcept
        {
            const auto& d = resultat.Declarations; const auto cible = static_cast<std::size_t>(resultat.Resultat.Detail);
            if (resultat.Resultat.Erreur != 121 || !cible || !resultat.PositionErreur
                || resultat.IndexUniteErreur >= d.Unites.size() || d.Noeuds.size() != d.Origines.size()
                || cible >= d.Noeuds.size()) return {};
            const auto& nominal = d.Noeuds[cible];
            if ((nominal.Genre != 41 && nominal.Genre != 42) || !nominal.Parent
                || (nominal.Genre == 41 && nominal.Parent + 1 != cible)
                || !NomDiagnosticDansUnite(resultat, cible) || d.Origines[cible].IndexUnite != resultat.IndexUniteErreur) return {};
            const auto reference = static_cast<std::size_t>(nominal.Parent); const auto& declaration = d.Noeuds[reference];
            const auto genre = declaration.Genre;
            if (genre != 1 && genre != 2 && genre != 3 && genre != 7 && genre != 12 && genre != 15 && genre != 19) return {};
            if (declaration.Parent >= reference) return {};
            const auto genreParent = d.Noeuds[declaration.Parent].Genre;
            if (((genre == 1 || genre == 3) && declaration.Parent)
                || (genre == 2 && genreParent != 1 && (genreParent < 12 || genreParent > 15))
                || (genre == 7 && (genreParent < 4 || genreParent > 6))
                || (genre == 12 && genreParent != 6)
                || (genre == 15 && declaration.Parent && genreParent != 6)
                || (genre == 19 && genreParent != 16 && genreParent != 20 && genreParent != 21)
                || (genre != 3 && genre != 19 && resultat.Resultat.NombreResolutions)) return {};
            const auto& origine = d.Origines[cible].Position; const auto& origineDeclaration = d.Origines[reference].Position;
            const auto& unite = d.Unites[resultat.IndexUniteErreur];
            if (!OrigineDiagnosticCorrespond(resultat, reference) || !NomDiagnosticDansUnite(resultat, reference)
                || nominal.HachageType != declaration.HachageType || nominal.HachageEspace != declaration.HachageEspace
                || nominal.DebutNom + nominal.TailleNom > declaration.DebutNom || nominal.Drapeaux > d.Source.size()
                || nominal.Ligne < declaration.Ligne || !nominal.Colonne || nominal.Ligne < unite.PremiereLigne
                || nominal.Ligne - unite.PremiereLigne >= unite.NombreLignes
                || origine.Fichier.empty() || !origine.Ligne || !origine.Colonne || origine.Unite != origineDeclaration.Unite
                || origine.Ordre < origineDeclaration.Ordre || origine.Ordre - origineDeclaration.Ordre != nominal.Drapeaux
                || (!nominal.Drapeaux && !OrigineDiagnosticCorrespond(resultat, cible))
                || resultat.Resultat.NombreSymboles != static_cast<std::uint64_t>(std::count_if(
                    d.Noeuds.begin(), d.Noeuds.end(), [](const auto& n) { return GenreSymbole(n.Genre) != 0; }))) return {};
            for (std::size_t i = 1; i < d.Noeuds.size(); ++i)
            {
                if (d.Noeuds[i].Parent == cible || (d.Noeuds[i].Genre == 41 && d.Noeuds[i].Parent == reference && i != cible)
                    || (OrigineDiagnosticCorrespond(resultat, i) && i != reference && i != cible)) return {};
                const auto& n = d.Noeuds[i];
                if (n.Genre == 42 && n.Parent == reference)
                {
                    const auto& origineNom = d.Origines[i].Position;
                    if (nominal.Genre != 42 || n.Drapeaux < 2 || !NomDiagnosticDansUnite(resultat, i)
                        || d.Origines[i].IndexUnite != resultat.IndexUniteErreur
                        || n.HachageType != declaration.HachageType || n.HachageEspace != declaration.HachageEspace
                        || n.DebutNom + n.TailleNom > declaration.DebutNom || n.Drapeaux > d.Source.size()
                        || origineNom.Unite != origineDeclaration.Unite || origineNom.Ordre < origineDeclaration.Ordre
                        || origineNom.Ordre - origineDeclaration.Ordre != n.Drapeaux
                        || (i != reference + 1 && (d.Noeuds[i - 1].Genre != 42 || d.Noeuds[i - 1].Parent != reference
                            || d.Noeuds[i - 1].Drapeaux >= n.Drapeaux
                            || d.Noeuds[i - 1].DebutNom + d.Noeuds[i - 1].TailleNom > n.DebutNom))) return {};
                    for (const auto& enfant : d.Noeuds) if (enfant.Parent == i) return {};
                }
            }
            if (nominal.Genre == 42)
                for (std::size_t i = reference + 1; i <= cible; ++i)
                    if (d.Noeuds[i].Genre != 42 || d.Noeuds[i].Parent != reference) return {};
            return {reference, cible};
        }

        /**
         * <résumé>Relie le code 121 d'héritage à sa base nominale, sans résoudre un type côté hôte.</résumé>
         * @etc. Detail indexe la feuille 40 ; le diagnostic reste à la position de la classe.
         * @Retourner(AccesDiagnosticPossede) Classe et base cohérentes, ou indices nuls.
         **/
        AccesDiagnosticPossede BaseClasseAmbigueDiagnostic(const SemantiqueAssemblee& resultat) noexcept
        {
            const auto& d = resultat.Declarations;
            const auto cible = static_cast<std::size_t>(resultat.Resultat.Detail);
            if (resultat.Resultat.Erreur != 121 || !cible || !resultat.PositionErreur
                || resultat.Resultat.NombreResolutions || resultat.IndexUniteErreur >= d.Unites.size()
                || d.Noeuds.size() != d.Origines.size() || cible >= d.Noeuds.size()) return {};
            const auto& base = d.Noeuds[cible];
            if (base.Genre != 40 || !base.Parent || base.Parent + 1 != cible || base.Drapeaux || base.HachageType
                || !NomDiagnosticDansUnite(resultat, cible) || d.Origines[cible].IndexUnite != resultat.IndexUniteErreur) return {};
            const auto reference = static_cast<std::size_t>(base.Parent);
            const auto& classe = d.Noeuds[reference];
            const auto& origineClasse = d.Origines[reference].Position;
            const auto& origineBase = d.Origines[cible].Position;
            const auto& unite = d.Unites[resultat.IndexUniteErreur];
            if (classe.Genre != 6 || classe.Parent || classe.Drapeaux != (4U | 32U | 1U)
                || !OrigineDiagnosticCorrespond(resultat, reference) || !NomDiagnosticDansUnite(resultat, reference)
                || base.HachageNom != classe.HachageType || base.HachageEspace != classe.HachageEspace
                || base.DebutNom < classe.DebutNom + classe.TailleNom || base.Ligne < classe.Ligne || !base.Colonne
                || base.Ligne < unite.PremiereLigne || base.Ligne - unite.PremiereLigne >= unite.NombreLignes
                || origineBase.Fichier.empty() || !origineBase.Ligne || !origineBase.Colonne
                || origineBase.Unite != origineClasse.Unite || origineBase.Ordre <= origineClasse.Ordre
                || origineBase.Ordre - origineClasse.Ordre < 2 || origineBase.Ordre - origineClasse.Ordre > 3
                || resultat.Resultat.NombreSymboles != static_cast<std::uint64_t>(std::count_if(
                    d.Noeuds.begin(), d.Noeuds.end(), [](const auto& n) { return GenreSymbole(n.Genre) != 0; }))) return {};
            for (std::size_t i = 1; i < d.Noeuds.size(); ++i)
                if (d.Noeuds[i].Parent == cible
                    || (d.Noeuds[i].Genre == 40 && d.Noeuds[i].Parent == reference && i != cible)
                    || (OrigineDiagnosticCorrespond(resultat, i) && i != reference)) return {};
            return {reference, cible};
        }

        /**
         * <résumé>Relie un nom répété ou en conflit à la déclaration déjà désignée par le natif.</résumé>
         * @etc. Detail est ici un index AST, pas un ordinal de symbole ; aucun nom ni type n'est résolu à nouveau.
         * @etc. Les portées sont contrôlées structurellement, sans rechercher une autre déclaration candidate.
         **/
        AccesDiagnosticPossede ConflitDeclarationDiagnostic(const SemantiqueAssemblee& resultat) noexcept
        {
            const auto code = resultat.Resultat.Erreur;
            if ((code != 12 && code != 13 && code != 14 && code != 15 && code != 17) || !resultat.PositionErreur
                || resultat.IndexUniteErreur >= resultat.Declarations.Unites.size()
                || resultat.Declarations.Noeuds.size() != resultat.Declarations.Origines.size()) return {};
            const auto& noeuds = resultat.Declarations.Noeuds;
            const std::size_t cible = resultat.Resultat.Detail;
            if (!cible || cible >= noeuds.size() || !NomDiagnosticDansUnite(resultat, cible)
                || !noeuds[cible].Parent || noeuds[cible].Parent >= cible
                || resultat.Declarations.Origines[cible].IndexUnite != resultat.IndexUniteErreur
                || resultat.Resultat.NombreSymboles != static_cast<std::uint64_t>(std::count_if(
                    noeuds.begin(), noeuds.end(), [](const auto& n) { return GenreSymbole(n.Genre) != 0; }))) return {};
            const auto genre = code == 12 ? 7U : (code == 13 || code == 14) ? 11U : code == 15 ? 9U : 19U;
            const auto fonctionAncetre = [&](std::size_t index)
            {
                while (index && index < noeuds.size())
                {
                    const auto& n = noeuds[index];
                    if (n.Genre == 1 || (n.Genre >= 12 && n.Genre <= 15)) return index;
                    if (n.Parent >= index) break;
                    index = static_cast<std::size_t>(n.Parent);
                }
                return std::size_t{0};
            };
            std::size_t reference = 0;
            for (std::size_t i = 1; i < noeuds.size(); ++i)
                if (noeuds[i].Genre == genre && OrigineDiagnosticCorrespond(resultat, i))
                {
                    const auto& n = noeuds[i]; const auto& precedent = noeuds[cible];
                    if (reference || (code != 14 && cible >= i) || !n.Parent || n.Parent >= i
                        || !NomDiagnosticDansUnite(resultat, i) || precedent.HachageNom != n.HachageNom
                        || (code == 14 ? precedent.DebutNom == n.DebutNom : precedent.DebutNom >= n.DebutNom)) return {};
                    if (code == 17)
                    {
                        const auto fonction = fonctionAncetre(i);
                        if ((precedent.Genre != 2 && precedent.Genre != 19) || !fonction
                            || fonctionAncetre(cible) != fonction) return {};
                        const auto portee = static_cast<std::size_t>(precedent.Parent);
                        const auto ancetre = precedent.Genre == 19 && (noeuds[portee].Genre == 20 || noeuds[portee].Genre == 21)
                            ? cible : portee;
                        if (!EstAncetreDiagnostic(noeuds, ancetre, i)) return {};
                    }
                    else
                    {
                        const auto parent = noeuds[n.Parent].Genre;
                        if (precedent.Genre != (code == 14 ? 7U : genre) || precedent.Parent != n.Parent
                            || (code == 15 ? parent != 8 : parent < 4 || parent > 6)) return {};
                    }
                    reference = i;
                }
            if (!reference) return {};
            for (std::size_t i = 1; i < noeuds.size(); ++i)
                if (OrigineDiagnosticCorrespond(resultat, i) && i != reference
                    && (!noeuds[i].Genre || (!EstAncetreDiagnostic(noeuds, i, reference)
                        && !EstAncetreDiagnostic(noeuds, reference, i)))) return {};
            return {reference, cible};
        }

        /** <résumé>Retrouve l'indice AST désigné par Detail, sans conserver de préfixe sémantique.</résumé> **/
        std::size_t NoeudSymboleDiagnostic(const SemantiqueAssemblee& resultat) noexcept
        {
            if (resultat.Resultat.Detail >= resultat.Resultat.NombreSymboles) return 0;
            std::size_t cible = 0;
            std::uint64_t ordinal = 0;
            const auto& noeuds = resultat.Declarations.Noeuds;
            for (std::size_t i = 1; i < noeuds.size(); ++i)
                if (GenreSymbole(noeuds[i].Genre))
                {
                    if (ordinal == resultat.Resultat.Detail) cible = i;
                    ++ordinal;
                }
            return ordinal == resultat.Resultat.NombreSymboles ? cible : 0;
        }

        /**
         * <résumé>Identifie l'alias revisité dans un cycle et l'entrée désignée par Detail.</résumé>
         * @etc. Detail est un rang de symbole, pas un index AST ; l'entrée et l'alias signalé peuvent différer.
         * @etc. Vérifie noms, unité et agrégat, sans chercher les cibles ni refaire la détection du cycle.
         **/
        AccesDiagnosticPossede CycleAliasChampDiagnostic(const SemantiqueAssemblee& resultat) noexcept
        {
            if (resultat.Resultat.Erreur != 107 || !resultat.PositionErreur
                || resultat.IndexUniteErreur >= resultat.Declarations.Unites.size()
                || resultat.Declarations.Noeuds.size() != resultat.Declarations.Origines.size()) return {};
            const auto& noeuds = resultat.Declarations.Noeuds;
            const auto entree = NoeudSymboleDiagnostic(resultat);
            if (!entree || noeuds[entree].Genre != 11 || !noeuds[entree].Parent
                || noeuds[entree].Parent >= entree || !NomDiagnosticDansUnite(resultat, entree)
                || resultat.Declarations.Origines[entree].IndexUnite != resultat.IndexUniteErreur) return {};
            const auto parent = noeuds[entree].Parent;
            if (noeuds[parent].Genre < 4 || noeuds[parent].Genre > 6
                || resultat.Declarations.Origines[parent].IndexUnite != resultat.IndexUniteErreur) return {};
            std::size_t reference = 0;
            for (std::size_t i = 1; i < noeuds.size(); ++i)
                if (noeuds[i].Genre == 11 && OrigineDiagnosticCorrespond(resultat, i))
                {
                    if (reference || noeuds[i].Parent != parent || noeuds[i].Parent >= i
                        || !NomDiagnosticDansUnite(resultat, i)) return {};
                    reference = i;
                }
            if (!reference) return {};
            for (std::size_t i = 1; i < noeuds.size(); ++i)
                if (OrigineDiagnosticCorrespond(resultat, i) && i != reference
                    && (!noeuds[i].Genre || (!EstAncetreDiagnostic(noeuds, i, reference)
                        && !EstAncetreDiagnostic(noeuds, reference, i)))) return {};
            return {reference, entree};
        }

        /**
         * <résumé>Vérifie les feuilles 39 facultatives, sans déduire un namespace de son hachage.</résumé>
         * @Paramètre(const SemantiqueAssemblee&: resultat) AST et origines possédés.
         * @Paramètre(std::size_t: alias) Alias racine dont les segments précèdent lexicalement le nom.
         * @Retourner(bool) Vrai pour un contexte canonique ou un ancien AST sans segments.
         **/
        bool ContexteEspaceAliasDiagnostic(const SemantiqueAssemblee& resultat, std::size_t alias) noexcept
        {
            const auto& d = resultat.Declarations;
            std::size_t precedent = 0;
            for (std::size_t i = 1; i < d.Noeuds.size(); ++i)
            {
                const auto& n = d.Noeuds[i];
                if (n.Genre == 39 && (!n.Parent || n.Parent >= i || d.Noeuds[n.Parent].Genre != 10
                    || d.Noeuds[n.Parent].Parent || n.Drapeaux || n.HachageType || !n.TailleNom
                    || d.Noeuds[i - 1].Parent != n.Parent
                    || (d.Noeuds[i - 1].Genre != 38 && d.Noeuds[i - 1].Genre != 39))) return false;
                if (n.Parent == alias + 1 && alias + 1 < d.Noeuds.size()
                    && d.Noeuds[alias + 1].Genre == 38 && d.Noeuds[alias + 1].Parent == alias) return false;
                if (n.Parent == alias && n.Genre != 38)
                {
                    if (n.Genre != 39 || i != (precedent ? precedent + 1 : alias + 2)
                        || d.Noeuds[alias + 1].Genre != 38 || d.Noeuds[alias + 1].Parent != alias
                        || n.Drapeaux || n.HachageType || !NomDiagnosticDansUnite(resultat, i)
                        || d.Origines[i].IndexUnite != d.Origines[alias].IndexUnite) return false;
                    const auto& o = d.Origines[i].Position; const auto& a = d.Origines[alias].Position;
                    const auto& u = d.Unites[d.Origines[i].IndexUnite];
                    if (o.Fichier.empty() || !o.Ligne || !o.Colonne || o.Unite != a.Unite || o.Ordre >= a.Ordre
                        || !n.Colonne || n.Ligne < u.PremiereLigne || n.Ligne - u.PremiereLigne >= u.NombreLignes
                        || n.DebutNom + n.TailleNom > d.Noeuds[alias].DebutNom
                        || (precedent && (n.DebutNom < d.Noeuds[precedent].DebutNom + d.Noeuds[precedent].TailleNom
                            || o.Ordre <= d.Origines[precedent].Position.Ordre))) return false;
                    precedent = i;
                }
                if (n.Parent < d.Noeuds.size() && d.Noeuds[n.Parent].Genre == 39
                    && d.Noeuds[n.Parent].Parent == alias) return false;
            }
            return !precedent || (d.Noeuds[alias].HachageEspace != HacherTexteDiagnostic({})
                && d.Noeuds[precedent].HachageEspace == d.Noeuds[alias].HachageEspace);
        }

        bool PossedeEspaceAliasDiagnostic(const SemantiqueAssemblee& resultat, std::size_t alias) noexcept
        {
            const auto& noeuds = resultat.Declarations.Noeuds;
            return noeuds[alias].HachageEspace == HacherTexteDiagnostic({})
                || (alias + 2 < noeuds.size() && noeuds[alias + 2].Parent == alias && noeuds[alias + 2].Genre == 39);
        }

        /**
         * <résumé>Contrôle l'entrée et l'alias racine revisité, même dans deux unités ou espaces différents.</résumé>
         * @etc. Detail est un rang de symbole ; aucune cible ni chaîne d'alias n'est résolue dans l'hôte.
         * @etc. Les anciens AST sans préfixe nominal restent numériques pour un alias signalé dans un namespace.
         **/
        AccesDiagnosticPossede CycleAliasRacineDiagnostic(const SemantiqueAssemblee& resultat) noexcept
        {
            if (resultat.Resultat.Erreur != 109 || !resultat.PositionErreur
                || resultat.IndexUniteErreur >= resultat.Declarations.Unites.size()
                || resultat.Declarations.Noeuds.size() != resultat.Declarations.Origines.size()) return {};
            const auto& d = resultat.Declarations;
            const auto entree = NoeudSymboleDiagnostic(resultat);
            const auto aliasValide = [&](std::size_t i)
            {
                if (!i || i >= d.Noeuds.size() || d.Noeuds[i].Genre != 10 || d.Noeuds[i].Parent
                    || d.Noeuds[i].Drapeaux || !NomDiagnosticDansUnite(resultat, i)) return false;
                const auto& n = d.Noeuds[i]; const auto& o = d.Origines[i]; const auto& u = d.Unites[o.IndexUnite];
                return !o.Position.Fichier.empty() && o.Position.Ligne && o.Position.Colonne
                    && n.Colonne && n.Ligne >= u.PremiereLigne && n.Ligne - u.PremiereLigne < u.NombreLignes;
            };
            if (!aliasValide(entree)) return {};
            std::size_t reference = 0;
            for (std::size_t i = 1; i < d.Noeuds.size(); ++i)
                if (OrigineDiagnosticCorrespond(resultat, i))
                {
                    if (reference || !aliasValide(i)) return {};
                    reference = i;
                }
            if (!reference) return {};
            for (std::size_t i = 1; i < d.Noeuds.size(); ++i)
                if (d.Noeuds[i].Parent == reference || d.Noeuds[i].Parent == entree)
                {
                    const auto& n = d.Noeuds[i]; const auto& alias = d.Noeuds[n.Parent];
                    if (n.Genre == 39) continue;
                    if (i != n.Parent + 1 || n.Genre != 38 || n.Drapeaux || n.HachageType
                        || n.HachageNom != alias.HachageType || n.HachageEspace != alias.HachageEspace
                        || !NomDiagnosticDansUnite(resultat, i)
                        || d.Origines[i].IndexUnite != d.Origines[n.Parent].IndexUnite) return {};
                }
            if (!ContexteEspaceAliasDiagnostic(resultat, reference) || !ContexteEspaceAliasDiagnostic(resultat, entree)) return {};
            return {reference, entree};
        }

        /**
         * <résumé>Relie les codes 110/112/121 à la cible qualifiée possédée de l'alias signalé.</résumé>
         * @etc. Detail indexe le nœud AST 38 pour 110 ; il conserve le nombre de surcharges pour 112.
         * @etc. Aucun namespace, type ou candidat n'est recherché ; le nom écrit n'est pas un nom résolu.
         **/
        AccesDiagnosticPossede CibleAliasRacineDiagnostic(const SemantiqueAssemblee& resultat) noexcept
        {
            const auto code = resultat.Resultat.Erreur;
            if ((code != 110 && code != 112 && code != 121)
                || (code == 121 ? resultat.Resultat.Detail != 0 : !resultat.Resultat.Detail) || !resultat.PositionErreur
                || resultat.IndexUniteErreur >= resultat.Declarations.Unites.size()
                || resultat.Declarations.Noeuds.size() != resultat.Declarations.Origines.size()) return {};
            const auto& d = resultat.Declarations;
            std::size_t cible = resultat.Resultat.Detail;
            if (code == 112)
            {
                const auto fonctions = static_cast<std::uint64_t>(std::count_if(d.Noeuds.begin(), d.Noeuds.end(),
                    [](const auto& n) { return GenreSymbole(n.Genre) == 2; }));
                if (resultat.Resultat.Detail < 2 || resultat.Resultat.Detail > fonctions) return {};
            }
            if (code == 112 || code == 121)
            {
                std::size_t alias = 0;
                for (std::size_t i = 1; i < d.Noeuds.size(); ++i)
                    if (OrigineDiagnosticCorrespond(resultat, i))
                    {
                        if (alias || d.Noeuds[i].Genre != 10) return {};
                        alias = i;
                    }
                if (!alias) return {};
                cible = alias + 1;
            }
            if (cible >= d.Noeuds.size() || d.Noeuds[cible].Genre != 38 || !d.Noeuds[cible].Parent
                || d.Noeuds[cible].Parent >= cible || d.Noeuds[cible].Drapeaux || d.Noeuds[cible].HachageType
                || !NomDiagnosticDansUnite(resultat, cible)
                || d.Origines[cible].IndexUnite != resultat.IndexUniteErreur) return {};
            const auto reference = static_cast<std::size_t>(d.Noeuds[cible].Parent);
            const auto& alias = d.Noeuds[reference]; const auto& nominal = d.Noeuds[cible];
            if (alias.Genre != 10 || alias.Parent || alias.Drapeaux || cible != reference + 1
                || !OrigineDiagnosticCorrespond(resultat, reference) || !NomDiagnosticDansUnite(resultat, reference)
                || nominal.HachageNom != alias.HachageType || nominal.HachageEspace != alias.HachageEspace
                || nominal.DebutNom < alias.DebutNom + alias.TailleNom) return {};
            const auto& o = d.Origines[cible].Position; const auto& origineAlias = d.Origines[reference].Position;
            const auto& unite = d.Unites[resultat.IndexUniteErreur];
            if (o.Fichier.empty() || !o.Ligne || !o.Colonne || o.Unite != origineAlias.Unite
                || o.Ordre <= origineAlias.Ordre || nominal.Ligne < alias.Ligne || !nominal.Colonne
                || nominal.Ligne < unite.PremiereLigne || nominal.Ligne - unite.PremiereLigne >= unite.NombreLignes
                || resultat.Resultat.NombreSymboles != static_cast<std::uint64_t>(std::count_if(
                    d.Noeuds.begin(), d.Noeuds.end(), [](const auto& n) { return GenreSymbole(n.Genre) != 0; }))) return {};
            for (std::size_t i = 1; i < d.Noeuds.size(); ++i)
                if ((d.Noeuds[i].Parent == reference && i != cible && d.Noeuds[i].Genre != 39) || d.Noeuds[i].Parent == cible
                    || (OrigineDiagnosticCorrespond(resultat, i) && i != reference)) return {};
            if (!ContexteEspaceAliasDiagnostic(resultat, reference)) return {};
            return {reference, cible};
        }

        /**
         * <résumé>Relie le code 108 à la cible nominale possédée, sans chercher un champ candidat.</résumé>
         * @etc. Detail indexe le nœud AST 37, enfant unique de l'alias à l'origine du refus.
         * @etc. Le nom cible et celui de l'alias sont contrôlés séparément ; Detail 0 reste numérique.
         **/
        AccesDiagnosticPossede CibleAliasChampDiagnostic(const SemantiqueAssemblee& resultat) noexcept
        {
            if (resultat.Resultat.Erreur != 108 || !resultat.Resultat.Detail || !resultat.PositionErreur
                || resultat.IndexUniteErreur >= resultat.Declarations.Unites.size()
                || resultat.Declarations.Noeuds.size() != resultat.Declarations.Origines.size()) return {};
            const auto& noeuds = resultat.Declarations.Noeuds;
            const std::size_t cible = resultat.Resultat.Detail;
            if (cible >= noeuds.size() || noeuds[cible].Genre != 37 || !noeuds[cible].Parent
                || noeuds[cible].Parent >= cible || noeuds[cible].Drapeaux || noeuds[cible].HachageType
                || !NomDiagnosticDansUnite(resultat, cible)
                || resultat.Declarations.Origines[cible].IndexUnite != resultat.IndexUniteErreur) return {};
            const auto reference = static_cast<std::size_t>(noeuds[cible].Parent);
            const auto& alias = noeuds[reference];
            if (alias.Genre != 11 || cible != reference + 1 || !alias.Parent || alias.Parent >= reference
                || noeuds[alias.Parent].Genre < 4 || noeuds[alias.Parent].Genre > 6
                || !OrigineDiagnosticCorrespond(resultat, reference) || !NomDiagnosticDansUnite(resultat, reference)
                || resultat.Declarations.Origines[alias.Parent].IndexUnite != resultat.IndexUniteErreur
                || noeuds[cible].HachageNom != alias.HachageType || noeuds[cible].HachageEspace != alias.HachageEspace
                || noeuds[cible].DebutNom < alias.DebutNom + alias.TailleNom) return {};
            const auto& origineCible = resultat.Declarations.Origines[cible].Position;
            const auto& origineAlias = resultat.Declarations.Origines[reference].Position;
            const auto& unite = resultat.Declarations.Unites[resultat.IndexUniteErreur];
            if (origineCible.Fichier.empty() || !origineCible.Ligne || !origineCible.Colonne
                || origineCible.Unite != origineAlias.Unite || origineAlias.Ordre > UINT64_MAX - 3
                || origineCible.Ordre != origineAlias.Ordre + 3
                || noeuds[cible].Ligne < alias.Ligne || !noeuds[cible].Colonne
                || noeuds[cible].Ligne < unite.PremiereLigne
                || noeuds[cible].Ligne - unite.PremiereLigne >= unite.NombreLignes) return {};
            if (resultat.Resultat.NombreSymboles != static_cast<std::uint64_t>(
                std::count_if(noeuds.begin(), noeuds.end(), [](const auto& n) { return GenreSymbole(n.Genre) != 0; }))) return {};
            for (std::size_t i = 1; i < noeuds.size(); ++i)
                if ((noeuds[i].Parent == reference && i != cible) || noeuds[i].Parent == cible
                    || (OrigineDiagnosticCorrespond(resultat, i) && i != reference
                        && (!noeuds[i].Genre || (!EstAncetreDiagnostic(noeuds, i, reference)
                            && !EstAncetreDiagnostic(noeuds, reference, i))))) return {};
            return {reference, cible};
        }

        /**
         * <résumé>Relie Detail du code 25 à la déclaration possédée, dans l'ordre ABI des symboles.</résumé>
         * @etc. Ce parcours reproduit uniquement l'indexation des genres, jamais la résolution d'un nom ou type.
         * @etc. Les préfixes natifs restent jetés ; champs, méthodes et opérateurs ne sont pas confondus.
         **/
        AccesDiagnosticPossede AccesInterditDiagnostic(const SemantiqueAssemblee& resultat) noexcept
        {
            if (resultat.Resultat.Erreur != 25 || !resultat.PositionErreur
                || resultat.IndexUniteErreur >= resultat.Declarations.Unites.size()
                || resultat.Declarations.Noeuds.size() != resultat.Declarations.Origines.size()
                || resultat.Resultat.Detail >= resultat.Resultat.NombreSymboles) return {};
            const auto& noeuds = resultat.Declarations.Noeuds;
            const auto cible = NoeudSymboleDiagnostic(resultat);
            if (!cible || !NomDiagnosticDansUnite(resultat, cible)
                || noeuds[cible].Parent >= cible) return {};
            const auto genreCible = noeuds[cible].Genre;
            if (genreCible != 7 && genreCible != 12 && genreCible != 15) return {};
            const auto proprietaire = noeuds[noeuds[cible].Parent].Genre;
            if (genreCible == 7 ? proprietaire < 4 || proprietaire > 6 : proprietaire != 6) return {};
            std::size_t reference = 0;
            for (std::size_t i = 1; i < noeuds.size(); ++i)
            {
                if (!OrigineDiagnosticCorrespond(resultat, i)) continue;
                const auto& n = noeuds[i];
                const bool membre = n.Genre == 29;
                const bool operateur = n.Genre == 25 || n.Genre == 26;
                const bool appel = n.Parent < i && i == n.Parent + 1 && noeuds[n.Parent].Genre == 28;
                const bool candidat = genreCible == 7 ? membre
                    : genreCible == 12 ? appel && (membre || n.Genre == 24) : operateur;
                if (!candidat) continue;
                if (reference || !n.Parent || n.Parent >= i || (membre && (n.Drapeaux & ~32768U))
                    || (!membre && n.Drapeaux) || !NomDiagnosticDansUnite(resultat, i)
                    || ((genreCible == 15 || (genreCible == 12 && membre)) && n.HachageNom != noeuds[cible].HachageNom)) return {};
                reference = i;
            }
            if (!reference) return {};
            for (std::size_t i = 1; i < noeuds.size(); ++i)
                if (OrigineDiagnosticCorrespond(resultat, i) && i != reference
                    && (!noeuds[i].Genre || (!EstAncetreDiagnostic(noeuds, i, reference)
                        && !EstAncetreDiagnostic(noeuds, reference, i)))) return {};
            return {reference, cible};
        }

        /**
         * <résumé>Identifie l'initialiseur et son champ canonique lorsqu'il existe.</résumé>
         * @etc. Code 33 : Detail 0 (absent) ou 1 (hérité), nom écrit ; aucune recherche de champ/base hôte.
         * @etc. Codes 34/35/36 et 41 explicite : Detail désigne la déclaration, même pour un alias de champ.
         * @etc. Le constructeur et le champ canonique doivent appartenir à la même classe.
         **/
        AccesDiagnosticPossede InitialiseurChampDiagnostic(const SemantiqueAssemblee& resultat) noexcept
        {
            const auto code = resultat.Resultat.Erreur;
            if ((code != 33 && code != 34 && code != 35 && code != 36 && code != 41) || !resultat.PositionErreur
                || resultat.IndexUniteErreur >= resultat.Declarations.Unites.size()
                || resultat.Declarations.Noeuds.size() != resultat.Declarations.Origines.size()) return {};
            const auto& noeuds = resultat.Declarations.Noeuds;
            const auto cible = code == 33 ? 0 : NoeudSymboleDiagnostic(resultat);
            if (code == 33)
            {
                if (resultat.Resultat.Detail > 1 || resultat.Resultat.NombreSymboles != static_cast<std::uint64_t>(
                    std::count_if(noeuds.begin(), noeuds.end(), [](const auto& n) { return GenreSymbole(n.Genre) != 0; }))) return {};
            }
            else if (!cible || noeuds[cible].Genre != 7 || !NomDiagnosticDansUnite(resultat, cible)
                || !noeuds[cible].Parent || noeuds[cible].Parent >= cible
                || noeuds[noeuds[cible].Parent].Genre != 6) return {};
            std::size_t reference = 0;
            for (std::size_t i = 1; i < noeuds.size(); ++i)
            {
                const auto& n = noeuds[i];
                if (n.Genre != 35 || !OrigineDiagnosticCorrespond(resultat, i)) continue;
                if (reference || n.Drapeaux || !n.Parent || n.Parent >= i || !NomDiagnosticDansUnite(resultat, i)) return {};
                const auto& constructeur = noeuds[n.Parent];
                if (constructeur.Genre != 13 || !constructeur.Parent || constructeur.Parent >= n.Parent
                    || noeuds[constructeur.Parent].Genre != 6) return {};
                if (code == 33)
                {
                    const auto drapeauxClasse = noeuds[constructeur.Parent].Drapeaux;
                    if ((drapeauxClasse & 32U) && !(drapeauxClasse & 1U)) return {};
                    if (resultat.Resultat.Detail == 1 && !(noeuds[constructeur.Parent].Drapeaux & 32U)) return {};
                }
                else if (constructeur.Parent != noeuds[cible].Parent
                    || resultat.Declarations.Origines[cible].IndexUnite != resultat.IndexUniteErreur) return {};
                if (code == 36 || code == 41)
                {
                    std::size_t nombreArguments = 0, argument = 0;
                    for (std::size_t j = i + 1; j < noeuds.size(); ++j)
                        if (noeuds[j].Parent == i)
                        {
                            if (noeuds[j].Genre < 22 || noeuds[j].Genre > 32) return {};
                            ++nombreArguments; argument = j;
                        }
                    if (code == 36 ? nombreArguments == 1
                        : nombreArguments != 1 || noeuds[argument].Genre == 32) return {};
                }
                reference = i;
            }
            if (!reference) return {};
            for (std::size_t i = 1; i < noeuds.size(); ++i)
                if (OrigineDiagnosticCorrespond(resultat, i) && i != reference
                    && (!noeuds[i].Genre || (!EstAncetreDiagnostic(noeuds, i, reference)
                        && !EstAncetreDiagnostic(noeuds, reference, i)))) return {};
            return {reference, cible};
        }

        /**
         * <résumé>Reconnaît le contexte par défaut du code 41, sans en fabriquer le message de types.</résumé>
         * @etc. Le champ et sa première expression non agrégée sont possédés ; les types restent opaques.
         **/
        bool ChampTableauParDefautDiagnostic(const SemantiqueAssemblee& resultat) noexcept
        {
            if (resultat.Resultat.Erreur != 41 || !resultat.PositionErreur
                || resultat.IndexUniteErreur >= resultat.Declarations.Unites.size()
                || resultat.Declarations.Noeuds.size() != resultat.Declarations.Origines.size()) return false;
            const auto& noeuds = resultat.Declarations.Noeuds;
            const auto champ = NoeudSymboleDiagnostic(resultat);
            auto expression = champ + 1;
            while (champ && expression < noeuds.size() && (noeuds[expression].Genre == 41 || noeuds[expression].Genre == 42))
            {
                const auto& nominal = noeuds[expression];
                if (nominal.Parent != champ || !NomDiagnosticDansUnite(resultat, expression)
                    || nominal.HachageType != noeuds[champ].HachageType
                    || nominal.HachageEspace != noeuds[champ].HachageEspace) return false;
                ++expression;
            }
            if (!champ || noeuds[champ].Genre != 7 || !noeuds[champ].Parent || noeuds[champ].Parent >= champ
                || noeuds[noeuds[champ].Parent].Genre != 6 || !(noeuds[champ].Drapeaux & 4U)
                || !NomDiagnosticDansUnite(resultat, champ)
                || resultat.Declarations.Origines[champ].IndexUnite != resultat.IndexUniteErreur
                || expression >= noeuds.size() || noeuds[expression].Parent != champ
                || noeuds[expression].Genre < 22 || noeuds[expression].Genre >= 32
                || !OrigineDiagnosticCorrespond(resultat, expression)) return false;
            for (std::size_t i = 1; i < noeuds.size(); ++i)
                if (OrigineDiagnosticCorrespond(resultat, i) && i != expression
                    && !((noeuds[i].Genre == 41 || noeuds[i].Genre == 42) && noeuds[i].Parent == champ)
                    && (!noeuds[i].Genre || (!EstAncetreDiagnostic(noeuds, i, expression)
                        && !EstAncetreDiagnostic(noeuds, expression, i)))) return false;
            return true;
        }

        /**
         * <résumé>Relie un refus de valeur par défaut ou de constante à son champ déclaré.</résumé>
         * @etc. Detail indexe le champ canonique ; son origine doit être celle du refus natif.
         * @etc. Aucun type opaque ni plan de constructeur n'est réinterprété dans l'hôte.
         **/
        std::size_t ChampDeclareDiagnostic(const SemantiqueAssemblee& resultat) noexcept
        {
            const auto code = resultat.Resultat.Erreur;
            if ((code != 38 && code != 40) || !resultat.PositionErreur
                || resultat.IndexUniteErreur >= resultat.Declarations.Unites.size()
                || resultat.Declarations.Noeuds.size() != resultat.Declarations.Origines.size()) return 0;
            const auto& noeuds = resultat.Declarations.Noeuds;
            const auto cible = NoeudSymboleDiagnostic(resultat);
            if (!cible || noeuds[cible].Genre != 7 || !noeuds[cible].Parent || noeuds[cible].Parent >= cible
                || noeuds[noeuds[cible].Parent].Genre != 6
                || noeuds[noeuds[cible].Parent].Parent >= noeuds[cible].Parent
                || resultat.Declarations.Origines[noeuds[cible].Parent].IndexUnite != resultat.IndexUniteErreur
                || !NomDiagnosticDansUnite(resultat, cible)
                || !OrigineDiagnosticCorrespond(resultat, cible)
                || ((noeuds[cible].Drapeaux & 4U) != 0) != (code == 38)) return 0;
            for (std::size_t i = 1; i < noeuds.size(); ++i)
                if (OrigineDiagnosticCorrespond(resultat, i) && i != cible
                    && (!noeuds[i].Genre || (!EstAncetreDiagnostic(noeuds, i, cible)
                        && !EstAncetreDiagnostic(noeuds, cible, i)))) return 0;
            return cible;
        }

        /**
         * <résumé>Lit uniquement les lexèmes du nom déjà possédé, jamais un fichier ou un Programme.</résumé>
         * @etc. Les qualifications ignorent les séparations ; this devient soi seulement pour le code 18.
         * @etc. Les membres et déclarations de méthode exigent un identifiant simple, jamais une qualification ni un mot-clé.
         * @etc. Un texte ou hachage incohérent est une erreur hôte, pas un nouveau diagnostic source.
         **/
        std::string NomReferenceDiagnostic(const SemantiqueAssemblee& resultat, std::size_t reference)
        {
            const auto& n = resultat.Declarations.Noeuds[reference];
            std::vector<Jeton> jetons;
            try { jetons = Lexeur(resultat.Declarations.Nom(reference)).Analyser(); }
            catch (const ErreurCompilation&) { ContratSemantiqueIncoherent("lexèmes du nom possédé"); }
            std::string nom;
            if (n.Genre == 15 || n.Genre == 25 || n.Genre == 26)
            {
                // L'AST natif conserve le lexème du signe, sans le mot-clé opérateur/operator.
                if (jetons.size() != 2)
                    ContratSemantiqueIncoherent("nom opérateur possédé");
                constexpr std::array<std::string_view, 18> operateurs{
                    "+", "-", "*", "/", "%", "==", "!=", "<", "<=", ">", ">=", "&", "|", "^", "<<", ">>", "!", "~"};
                const auto& operateur = jetons.front().Texte;
                if (std::find(operateurs.begin(), operateurs.end(), operateur) == operateurs.end())
                    ContratSemantiqueIncoherent("opérateur possédé inconnu");
                nom = operateur;
            }
            else if (n.Genre == 2 || n.Genre == 6 || n.Genre == 7 || n.Genre == 9 || n.Genre == 11 || n.Genre == 19
                || n.Genre == 29 || n.Genre == 12 || n.Genre == 35 || n.Genre == 37 || n.Genre == 39)
            {
                if (jetons.size() != 2 || jetons.front().Genre != GenreJeton::Identifiant)
                    ContratSemantiqueIncoherent("nom membre possédé");
                nom = jetons.front().Texte;
            }
            else if (resultat.Resultat.Erreur == 18 && jetons.size() == 2 && jetons.front().Genre == GenreJeton::Soi) nom = "soi";
            else
            {
                if (jetons.size() < 2 || jetons.size() % 2 != 0) ContratSemantiqueIncoherent("nom possédé incomplet");
                for (std::size_t i = 0; i + 1 < jetons.size(); ++i)
                {
                    const auto genre = i % 2 ? GenreJeton::DeuxPointsDouble : GenreJeton::Identifiant;
                    if (jetons[i].Genre != genre) ContratSemantiqueIncoherent("lexèmes du nom possédé");
                    nom += jetons[i].Texte;
                }
            }
            if (HacherTexteDiagnostic(nom) != n.HachageNom) ContratSemantiqueIncoherent("hachage du nom possédé");
            return nom;
        }

        /** <résumé>Joint uniquement des noms possédés, en contrôlant chaque hachage de préfixe.</résumé> **/
        std::string NomCompletAliasDiagnostic(const SemantiqueAssemblee& resultat, std::size_t alias)
        {
            std::string prefixe;
            const auto& noeuds = resultat.Declarations.Noeuds;
            for (std::size_t i = alias + 2; i < noeuds.size() && noeuds[i].Parent == alias && noeuds[i].Genre == 39; ++i)
            {
                if (!prefixe.empty()) prefixe += "::";
                prefixe += NomReferenceDiagnostic(resultat, i);
                if (HacherTexteDiagnostic(prefixe) != noeuds[i].HachageEspace)
                    ContratSemantiqueIncoherent("préfixe de namespace possédé");
            }
            if (!prefixe.empty()) prefixe += "::";
            return prefixe + NomReferenceDiagnostic(resultat, alias);
        }

        bool RefusSemantique(std::uint32_t code)
        {
            // Les capacités 91..93 appartiennent à l'émission des globales, pas à cette passe.
            return (code >= 6 && code <= 90) || (code >= 94 && code <= 125);
        }
    }

    bool SemantiqueAssemblee::PeutTraduireDiagnostic() const noexcept
    {
        if (const auto cycle = CycleAliasRacineDiagnostic(*this); cycle.Reference
            && PossedeEspaceAliasDiagnostic(*this, cycle.Reference)) return true;
        return MessageSemantiqueTraduisible(*this) != nullptr || ReferenceDiagnosticPossede(*this) != 0
            || AccesInterditDiagnostic(*this).Reference != 0 || InitialiseurChampDiagnostic(*this).Reference != 0
            || ChampDeclareDiagnostic(*this) != 0 || ConflitDeclarationDiagnostic(*this).Reference != 0
            || CycleAliasChampDiagnostic(*this).Reference != 0 || CibleAliasChampDiagnostic(*this).Reference != 0
            || CibleAliasRacineDiagnostic(*this).Reference != 0 || EspaceUtiliseDiagnostic(*this) != 0
            || BaseClasseAmbigueDiagnostic(*this).Reference != 0 || NomTypeAmbiguDiagnostic(*this).Reference != 0
            || NomTypeConversionAmbiguDiagnostic(*this).Reference != 0;
    }

    void SemantiqueAssemblee::ExigerValide() const
    {
        const auto& declarations = Declarations;
        if (declarations.Resultat.Erreur || declarations.Resultat.Reserve
            || declarations.Resultat.LigneErreur || declarations.Resultat.ColonneErreur
            || declarations.Resultat.Detail || declarations.Resultat.DetailLexical
            || declarations.PositionErreur || !declarations.NomConflit.empty()
            || declarations.Resultat.IndexUniteErreur != declarations.Unites.size()
            || declarations.Noeuds.empty() || declarations.Unites.empty()
            || declarations.Origines.size() != declarations.Noeuds.size()
            || declarations.Resultat.NombreNoeuds != declarations.Noeuds.size()
            || declarations.Resultat.NombreOrigines != declarations.Unites.size()
            || declarations.Resultat.NombreOctetsSource != declarations.Source.size()
            || Resultat.NombreSymboles != Resultat.CapaciteSymbolesRequise
            || Resultat.NombreResolutions != Resultat.CapaciteResolutionsRequise
            || Resultat.NombreSymboles > declarations.Noeuds.size())
            ContratSemantiqueIncoherent("diagnostic possédé");
        if (!Resultat.Erreur)
        {
            if (PositionErreur || IndexUniteErreur != declarations.Unites.size()
                || Resultat.LigneErreur || Resultat.ColonneErreur || Resultat.Detail
                || Symboles.size() != Resultat.NombreSymboles || Resolutions.size() != Resultat.NombreResolutions)
                ContratSemantiqueIncoherent("succès possédé");
            return;
        }
        if (!RefusSemantique(Resultat.Erreur) || !Symboles.empty() || !Resolutions.empty())
            ContratSemantiqueIncoherent("refus possédé");
        if (Resultat.Erreur == 20)
        {
            if (PositionErreur || IndexUniteErreur != declarations.Unites.size()
                || Resultat.LigneErreur != 1 || Resultat.ColonneErreur != 1 || Resultat.Detail
                || std::any_of(declarations.Noeuds.begin(), declarations.Noeuds.end(),
                    [](const auto& noeud) { return GenreSymbole(noeud.Genre) == 2; }))
                ContratSemantiqueIncoherent("refus global possédé");
        }
        else
        {
            if (!PositionErreur || IndexUniteErreur >= declarations.Unites.size()
                || PositionErreur->Fichier.empty() || !PositionErreur->Ligne || !PositionErreur->Colonne)
                ContratSemantiqueIncoherent("position possédée absente");
            bool correspond = false;
            for (std::size_t i = 1; i < declarations.Noeuds.size(); ++i)
            {
                if (OrigineDiagnosticCorrespond(*this, i)) correspond = true;
            }
            if (!correspond) ContratSemantiqueIncoherent("origine possédée incohérente");
        }
        if (Resultat.Erreur == 120)
        {
            const auto reference = EspaceUtiliseDiagnostic(*this);
            if (!reference) ContratSemantiqueIncoherent("espace utilisé introuvable");
            const auto nom = NomReferenceDiagnostic(*this, reference);
            throw ErreurCompilation("espace utilisé introuvable : " + nom, "used namespace not found: " + nom,
                PositionErreur->Ligne, PositionErreur->Colonne, PositionErreur->Fichier);
        }
        if (const auto champ = ChampDeclareDiagnostic(*this))
        {
            const auto nom = NomReferenceDiagnostic(*this, champ);
            throw ErreurCompilation(Resultat.Erreur == 38
                    ? "un champ objet classe utilise un constructeur, pas un initialiseur par défaut avec '=' : " + nom
                    : "un champ constant doit être initialisé explicitement ou posséder une valeur par défaut : " + nom,
                Resultat.Erreur == 38 ? "a class object field uses a constructor, not a default '=' initializer: " + nom
                    : "a const field must be explicitly initialized or have a default value: " + nom,
                PositionErreur->Ligne, PositionErreur->Colonne, PositionErreur->Fichier);
        }
        if (const auto initialiseur = InitialiseurChampDiagnostic(*this); initialiseur.Reference)
        {
            const auto reference = NomReferenceDiagnostic(*this, initialiseur.Reference);
            if (Resultat.Erreur == 33)
                throw ErreurCompilation(Resultat.Detail == 1
                        ? "un constructeur initialise uniquement les champs déclarés directement dans sa classe : " + reference
                        : "champ introuvable dans la liste d’initialisation : " + reference,
                    Resultat.Detail == 1 ? "a constructor only initializes fields declared directly in its class: " + reference
                        : "unknown field in initializer list: " + reference,
                    PositionErreur->Ligne, PositionErreur->Colonne, PositionErreur->Fichier);
            const auto nom = NomReferenceDiagnostic(*this, initialiseur.Cible);
            if (Resultat.Erreur == 36 || Resultat.Erreur == 41)
                throw ErreurCompilation(Resultat.Erreur == 36
                        ? "un initialiseur de champ exige exactement une expression : " + nom
                        : "un champ tableau exige un initialiseur agrégé : " + nom,
                    Resultat.Erreur == 36 ? "a field initializer requires exactly one expression: " + nom
                        : "an array field requires an aggregate initializer: " + nom,
                    PositionErreur->Ligne, PositionErreur->Colonne, PositionErreur->Fichier);
            throw ErreurCompilation(Resultat.Erreur == 34 ? "champ initialisé plusieurs fois : " + nom
                    : "les champs doivent être initialisés dans leur ordre de déclaration : " + nom,
                Resultat.Erreur == 34 ? "field initialized more than once: " + nom
                    : "fields must be initialized in declaration order: " + nom,
                PositionErreur->Ligne, PositionErreur->Colonne, PositionErreur->Fichier);
        }
        if (const auto acces = AccesInterditDiagnostic(*this); acces.Reference)
        {
            const auto genre = declarations.Noeuds[acces.Cible].Genre;
            const auto reference = NomReferenceDiagnostic(*this, acces.Reference);
            const auto cible = NomReferenceDiagnostic(*this, acces.Cible);
            if (genre == 15)
            {
                if (cible != reference) ContratSemantiqueIncoherent("opérateur cible différent");
                throw ErreurCompilation("opérateur membre inaccessible", "member operator is inaccessible",
                    PositionErreur->Ligne, PositionErreur->Colonne, PositionErreur->Fichier);
            }
            const auto& nom = genre == 7 ? reference : cible;
            throw ErreurCompilation(genre == 7 ? "accès interdit au membre " + nom : "méthode inaccessible : " + nom,
                genre == 7 ? "member access is not allowed: " + nom : "method is inaccessible: " + nom,
                PositionErreur->Ligne, PositionErreur->Colonne, PositionErreur->Fichier);
        }
        if (const auto cible = CibleAliasChampDiagnostic(*this); cible.Reference)
        {
            const auto nom = NomReferenceDiagnostic(*this, cible.Cible);
            (void)NomReferenceDiagnostic(*this, cible.Reference);
            throw ErreurCompilation("champ cible d’alias introuvable : " + nom,
                "unknown field alias target: " + nom,
                PositionErreur->Ligne, PositionErreur->Colonne, PositionErreur->Fichier);
        }
        auto typeAmbigu = NomTypeAmbiguDiagnostic(*this);
        if (!typeAmbigu.Reference) typeAmbigu = NomTypeConversionAmbiguDiagnostic(*this);
        if (const auto type = typeAmbigu; type.Reference)
        {
            const auto nom = NomReferenceDiagnostic(*this, type.Cible);
            const auto& proprietaire = declarations.Noeuds[type.Reference];
            if (proprietaire.Genre != 31)
            {
                const auto declaration = NomReferenceDiagnostic(*this, type.Reference);
                if (declaration.find("::") != std::string::npos) ContratSemantiqueIncoherent("nom de déclaration typée possédé");
            }
            const auto& nominal = declarations.Noeuds[type.Cible];
            const auto& unite = declarations.Unites[IndexUniteErreur];
            auto debut = static_cast<std::size_t>(unite.DebutOctets);
            for (auto ligne = unite.PremiereLigne; ligne < proprietaire.Ligne; ++ligne)
            {
                const auto finLigne = declarations.Source.find('\n', debut);
                if (finLigne == std::string::npos || finLigne >= nominal.DebutNom)
                    ContratSemantiqueIncoherent("ligne du préfixe de type possédé");
                debut = finLigne + 1;
            }
            if (!proprietaire.Colonne || debut > nominal.DebutNom
                || proprietaire.Colonne - 1 > nominal.DebutNom - debut)
                ContratSemantiqueIncoherent("colonne du préfixe de type possédé");
            debut += proprietaire.Colonne - 1;
            std::vector<Jeton> prefixe;
            try { prefixe = Lexeur(declarations.Source.substr(debut, nominal.DebutNom - debut)).Analyser(); }
            catch (const ErreurCompilation&) { ContratSemantiqueIncoherent("préfixe de type possédé"); }
            if (prefixe.size() - 1 != nominal.Drapeaux)
                ContratSemantiqueIncoherent("ordre lexical du type possédé");
            const bool callback = std::any_of(prefixe.begin(), prefixe.end(), [](const auto& j)
            { return j.Genre == GenreJeton::PointeurFonction; });
            if (nominal.Genre == 43)
            {
                if (prefixe.size() < 3 || prefixe[0].Genre != GenreJeton::Convertir || prefixe[1].Genre != GenreJeton::Inferieur)
                    ContratSemantiqueIncoherent("préfixe de conversion possédé");
            }
            else if ((nominal.Genre == 42) != callback)
                ContratSemantiqueIncoherent("genre du nom de type possédé");
            throw ErreurCompilation("nom importé ambigu : " + nom, "ambiguous imported name: " + nom,
                PositionErreur->Ligne, PositionErreur->Colonne, PositionErreur->Fichier);
        }
        if (const auto base = BaseClasseAmbigueDiagnostic(*this); base.Reference)
        {
            const auto nom = NomReferenceDiagnostic(*this, base.Cible);
            (void)NomReferenceDiagnostic(*this, base.Reference);
            const auto& classe = declarations.Noeuds[base.Reference];
            const auto debut = classe.DebutNom + classe.TailleNom;
            std::vector<Jeton> separation;
            try { separation = Lexeur(declarations.Source.substr(debut,
                declarations.Noeuds[base.Cible].DebutNom - debut)).Analyser(); }
            catch (const ErreurCompilation&) { ContratSemantiqueIncoherent("séparation de la base possédée"); }
            if ((separation.size() != 2 && separation.size() != 3)
                || separation.front().Genre != GenreJeton::DeuxPoints
                || (separation.size() == 3 && separation[1].Genre != GenreJeton::Publique)
                || declarations.Origines[base.Cible].Position.Ordre - declarations.Origines[base.Reference].Position.Ordre != separation.size())
                ContratSemantiqueIncoherent("ordre lexical de la base possédée");
            throw ErreurCompilation("nom importé ambigu : " + nom, "ambiguous imported name: " + nom,
                PositionErreur->Ligne, PositionErreur->Colonne, PositionErreur->Fichier);
        }
        if (const auto cible = CibleAliasRacineDiagnostic(*this); cible.Reference)
        {
            const auto nom = NomReferenceDiagnostic(*this, cible.Cible);
            const auto nomAlias = NomReferenceDiagnostic(*this, cible.Reference);
            std::uint64_t decalage = 3;
            for (std::size_t p = 0; (p = nomAlias.find("::", p)) != std::string::npos; p += 2) decalage += 2;
            const auto& alias = declarations.Origines[cible.Reference].Position;
            if (alias.Ordre > UINT64_MAX - decalage || declarations.Origines[cible.Cible].Position.Ordre != alias.Ordre + decalage)
                ContratSemantiqueIncoherent("ordre de la cible qualifiée d'alias");
            if (Resultat.Erreur == 112)
                throw ErreurCompilation("un alias de fonction surchargée est ambigu : " + nom,
                    "an overloaded function alias is ambiguous: " + nom,
                    PositionErreur->Ligne, PositionErreur->Colonne, PositionErreur->Fichier);
            if (Resultat.Erreur == 121)
                throw ErreurCompilation("nom importé ambigu : " + nom, "ambiguous imported name: " + nom,
                    PositionErreur->Ligne, PositionErreur->Colonne, PositionErreur->Fichier);
            throw ErreurCompilation("cible d’alias introuvable : " + nom,
                "unknown alias target: " + nom, PositionErreur->Ligne, PositionErreur->Colonne, PositionErreur->Fichier);
        }
        if (Resultat.Erreur == 109)
        {
            const auto cycle = CycleAliasRacineDiagnostic(*this);
            if (!cycle.Reference) ContratSemantiqueIncoherent("cycle d'alias racine");
            const auto nom = NomCompletAliasDiagnostic(*this, cycle.Reference);
            (void)NomCompletAliasDiagnostic(*this, cycle.Cible);
            if (PossedeEspaceAliasDiagnostic(*this, cycle.Reference))
                throw ErreurCompilation("cycle d’alias détecté pour " + nom,
                    "alias cycle detected for " + nom,
                    PositionErreur->Ligne, PositionErreur->Colonne, PositionErreur->Fichier);
        }
        if (const auto cycle = CycleAliasChampDiagnostic(*this); cycle.Reference)
        {
            const auto nom = NomReferenceDiagnostic(*this, cycle.Reference);
            (void)NomReferenceDiagnostic(*this, cycle.Cible);
            throw ErreurCompilation("cycle d’alias de champ détecté pour " + nom,
                "field alias cycle detected for " + nom,
                PositionErreur->Ligne, PositionErreur->Colonne, PositionErreur->Fichier);
        }
        if (const auto declaration = ConflitDeclarationDiagnostic(*this); declaration.Reference)
        {
            const auto nom = NomReferenceDiagnostic(*this, declaration.Reference);
            if (NomReferenceDiagnostic(*this, declaration.Cible) != nom)
                ContratSemantiqueIncoherent("noms de déclarations en conflit possédés");
            const auto code = Resultat.Erreur;
            const auto francais = code == 12 ? "champ déclaré plusieurs fois : "
                : code == 13 ? "alias de champ déclaré plusieurs fois : "
                : code == 14 ? "alias de champ en conflit avec un champ : "
                : code == 15 ? "énumérateur déclaré plusieurs fois : " : "nom déjà déclaré : ";
            const auto anglais = code == 12 ? "field declared more than once: "
                : code == 13 ? "field alias declared more than once: "
                : code == 14 ? "field alias conflicts with a field: "
                : code == 15 ? "enumerator declared more than once: " : "name already declared: ";
            throw ErreurCompilation(francais + nom, anglais + nom,
                PositionErreur->Ligne, PositionErreur->Colonne, PositionErreur->Fichier);
        }
        if (const auto reference = ReferenceDiagnosticPossede(*this))
        {
            const auto nom = NomReferenceDiagnostic(*this, reference);
            if (Resultat.Erreur == 121)
                throw ErreurCompilation("nom importé ambigu : " + nom, "ambiguous imported name: " + nom,
                    PositionErreur->Ligne, PositionErreur->Colonne, PositionErreur->Fichier);
            if (Resultat.Erreur == 19)
                throw ErreurCompilation("l’adresse d’une fonction surchargée exige un contexte non ambigu : " + nom,
                    "taking an overloaded function address requires an unambiguous context: " + nom,
                    PositionErreur->Ligne, PositionErreur->Colonne, PositionErreur->Fichier);
            if (Resultat.Erreur == 24)
                throw ErreurCompilation("membre introuvable : " + nom, "unknown member: " + nom,
                    PositionErreur->Ligne, PositionErreur->Colonne, PositionErreur->Fichier);
            throw ErreurCompilation("variable ou fonction introuvable : " + nom,
                "unknown variable or function: " + nom, PositionErreur->Ligne, PositionErreur->Colonne, PositionErreur->Fichier);
        }
        const auto* message = MessageSemantiqueTraduisible(*this);
        if (!message) throw std::runtime_error("diagnostic sémantique non traduit : code=" + std::to_string(Resultat.Erreur));
        throw ErreurCompilation(message->Francais, message->Anglais,
            PositionErreur ? PositionErreur->Ligne : 1, PositionErreur ? PositionErreur->Colonne : 1,
            PositionErreur ? PositionErreur->Fichier : std::string{});
    }

    SemantiqueAssemblee AnalyserSemantiqueAssemblee(const DeclarationsAssemblees& declarations,
        AnalyseurSemantiqueUnitesHote analyser, const LimitesSemantiqueAssemblee& limites)
    {
        if (!analyser || !limites.MaximumOctets || limites.MaximumOctets > 999'999'999
            || !limites.MaximumNoeuds || limites.MaximumNoeuds > 100'000'000
            || !limites.MaximumUnites || limites.MaximumUnites > 1'000'000
            || !limites.MaximumSymboles || limites.MaximumSymboles > 100'000'000
            || !limites.MaximumResolutions || limites.MaximumResolutions > 100'000'000)
            throw std::invalid_argument("export ou limites de sémantique assemblée invalides");
        if (declarations.Source.size() > limites.MaximumOctets || declarations.Noeuds.size() > limites.MaximumNoeuds
            || declarations.Unites.size() > limites.MaximumUnites)
            throw std::length_error("limite d'entrée sémantique assemblée dépassée");
        const auto& r = declarations.Resultat;
        if (r.Erreur || r.Reserve || r.LigneErreur || r.ColonneErreur || r.Detail || r.DetailLexical
            || declarations.PositionErreur || !declarations.NomConflit.empty() || declarations.Noeuds.empty()
            || declarations.Unites.empty() || r.NombreNoeuds != declarations.Noeuds.size()
            || r.NombreOrigines != declarations.Unites.size() || r.NombreOctetsSource != declarations.Source.size()
            || r.IndexUniteErreur != declarations.Unites.size() || declarations.Origines.size() != declarations.Noeuds.size())
            ContratSemantiqueIncoherent();
        std::uint64_t prochainOctet = 0;
        std::uint32_t prochaineLigne = 1;
        for (const auto& unite : declarations.Unites)
        {
            if (unite.Reserve || (unite.OctetsBom != 0 && unite.OctetsBom != 3)
                || unite.DebutOctets != prochainOctet || unite.PremiereLigne != prochaineLigne
                || prochainOctet >= declarations.Source.size()
                || unite.TailleOctets >= declarations.Source.size() - prochainOctet
                || declarations.Source[prochainOctet + unite.TailleOctets] != '\n')
                ContratSemantiqueIncoherent();
            const auto texte = std::string_view(declarations.Source).substr(prochainOctet, unite.TailleOctets);
            const auto lignes = 1U + static_cast<std::uint32_t>(std::count(texte.begin(), texte.end(), '\n'));
            if (unite.NombreLignes != lignes) ContratSemantiqueIncoherent();
            prochainOctet += unite.TailleOctets + 1; prochaineLigne += lignes;
        }
        if (prochainOctet != declarations.Source.size()) ContratSemantiqueIncoherent();
        std::vector<std::size_t> noeudsSymboles;
        std::map<std::pair<std::uint32_t, std::uint32_t>, std::size_t> positions;
        for (std::size_t i = 0; i < declarations.Noeuds.size(); ++i)
        {
            const auto& n = declarations.Noeuds[i]; const auto& origine = declarations.Origines[i];
            if (n.Genre > 43 || (n.Genre != 41 && n.Genre != 42 && n.Genre != 43 && (n.Drapeaux & ~0x1FFFFU)) || n.DebutNom > declarations.Source.size()
                || n.TailleNom > declarations.Source.size() - n.DebutNom) ContratSemantiqueIncoherent();
            if (!i)
            {
                if (n.Genre || n.Parent || n.Ligne != 1 || n.Colonne != 1 || n.Drapeaux || n.DebutNom || n.TailleNom
                    || n.HachageNom || n.HachageType || origine.IndexUnite != declarations.Unites.size()
                    || !origine.Position.Fichier.empty() || origine.EstInterface) ContratSemantiqueIncoherent();
                continue;
            }
            if (!n.Genre || n.Parent >= i || !n.Colonne || origine.IndexUnite >= declarations.Unites.size()
                || origine.Position.Fichier.empty() || !origine.Position.Ligne || !origine.Position.Colonne)
                ContratSemantiqueIncoherent();
            if (declarations.Noeuds[n.Parent].Genre >= 40 && declarations.Noeuds[n.Parent].Genre <= 43) ContratSemantiqueIncoherent();
            if (n.Genre == 43)
            {
                const auto& parent = declarations.Noeuds[n.Parent];
                if (!n.Parent || parent.Genre != 31 || parent.Drapeaux || parent.DebutNom || parent.TailleNom || parent.HachageNom
                    || !n.TailleNom || n.Drapeaux < 2 || n.Drapeaux > declarations.Source.size()
                    || n.HachageType != parent.HachageType || n.HachageEspace != parent.HachageEspace
                    || n.Ligne < parent.Ligne || (n.Ligne == parent.Ligne && n.Colonne <= parent.Colonne)
                    || (n.Parent + 1 != i && (declarations.Noeuds[i - 1].Genre != 43 || declarations.Noeuds[i - 1].Parent != n.Parent
                        || declarations.Noeuds[i - 1].Drapeaux >= n.Drapeaux
                        || declarations.Noeuds[i - 1].DebutNom + declarations.Noeuds[i - 1].TailleNom > n.DebutNom))) ContratSemantiqueIncoherent();
            }
            if (n.Genre == 41 || n.Genre == 42)
            {
                const auto& parent = declarations.Noeuds[n.Parent]; const auto genre = parent.Genre;
                if (!n.Parent || (n.Genre == 41 && n.Parent + 1 != i) || (genre != 1 && genre != 2 && genre != 3
                    && genre != 7 && genre != 12 && genre != 15 && genre != 19) || !n.TailleNom
                    || n.HachageType != parent.HachageType || n.HachageEspace != parent.HachageEspace
                    || n.Drapeaux > declarations.Source.size() || n.DebutNom + n.TailleNom > parent.DebutNom) ContratSemantiqueIncoherent();
                if (n.Genre == 42 && (n.Drapeaux < 2 || (n.Parent + 1 != i
                    && (declarations.Noeuds[i - 1].Genre != 42 || declarations.Noeuds[i - 1].Parent != n.Parent
                        || declarations.Noeuds[i - 1].Drapeaux >= n.Drapeaux
                        || declarations.Noeuds[i - 1].DebutNom + declarations.Noeuds[i - 1].TailleNom > n.DebutNom)))) ContratSemantiqueIncoherent();
            }
            if (n.Genre == 40 && (!n.Parent || n.Parent + 1 != i
                || declarations.Noeuds[n.Parent].Genre != 6 || declarations.Noeuds[n.Parent].Parent
                || !(declarations.Noeuds[n.Parent].Drapeaux & 32) || n.Drapeaux || n.HachageType || !n.TailleNom
                || n.HachageNom != declarations.Noeuds[n.Parent].HachageType
                || n.HachageEspace != declarations.Noeuds[n.Parent].HachageEspace)) ContratSemantiqueIncoherent();
            if ((n.Genre == 37 || n.Genre == 38) && (!n.Parent || n.Parent + 1 != i
                || declarations.Noeuds[n.Parent].Genre != (n.Genre == 37 ? 11U : 10U)
                || n.Drapeaux || n.HachageType || !n.TailleNom
                || n.HachageNom != declarations.Noeuds[n.Parent].HachageType
                || n.HachageEspace != declarations.Noeuds[n.Parent].HachageEspace)) ContratSemantiqueIncoherent();
            if (n.Genre == 39 && (!n.Parent || declarations.Noeuds[n.Parent].Genre != 10
                || declarations.Noeuds[n.Parent].Parent || n.Drapeaux || n.HachageType || !n.TailleNom
                || declarations.Noeuds[i - 1].Parent != n.Parent
                || (declarations.Noeuds[i - 1].Genre != 38 && declarations.Noeuds[i - 1].Genre != 39)))
                ContratSemantiqueIncoherent();
            const auto& unite = declarations.Unites[origine.IndexUnite];
            if (n.Ligne < unite.PremiereLigne || n.Ligne - unite.PremiereLigne >= unite.NombreLignes
                || (n.TailleNom && (n.DebutNom < unite.DebutOctets
                    || n.DebutNom - unite.DebutOctets > unite.TailleOctets
                    || n.TailleNom > unite.TailleOctets - (n.DebutNom - unite.DebutOctets))))
                ContratSemantiqueIncoherent();
            const auto [trouve, insere] = positions.emplace(std::pair{n.Ligne, n.Colonne}, i);
            if (!insere)
            {
                const auto& precedent = declarations.Origines[trouve->second];
                if (precedent.IndexUnite != origine.IndexUnite || precedent.Position.Fichier != origine.Position.Fichier
                    || precedent.Position.Ligne != origine.Position.Ligne || precedent.Position.Colonne != origine.Position.Colonne)
                    ContratSemantiqueIncoherent();
            }
            if (GenreSymbole(n.Genre)) noeudsSymboles.push_back(i);
        }
        SemantiqueAssemblee resultat;
        resultat.Declarations = declarations;
        resultat.IndexUniteErreur = declarations.Unites.size();
        auto& copie = resultat.Declarations;
        RequeteAnalyseSemantiqueHote analyse{copie.Source.data(), copie.Source.size(), copie.Noeuds.data(), copie.Noeuds.size(), nullptr, 0, nullptr, 0, {}};
        auto appeler = [&]()
        {
            const auto avant = analyse;
            RequeteAnalyseSemantiqueUnitesHote requete{&analyse, copie.Unites.data(), copie.Unites.size(), copie.Unites.size(), 0, 0};
            const auto code = analyser(&requete);
            if (std::memcmp(&analyse, &avant, offsetof(RequeteAnalyseSemantiqueHote, Resultat)) != 0
                || requete.Analyse != &analyse || requete.Origines != copie.Unites.data() || requete.NombreOrigines != copie.Unites.size()
                || copie.Source != declarations.Source
                || std::memcmp(copie.Noeuds.data(), declarations.Noeuds.data(), copie.Noeuds.size() * sizeof(copie.Noeuds[0])) != 0
                || std::memcmp(copie.Unites.data(), declarations.Unites.data(), copie.Unites.size() * sizeof(copie.Unites[0])) != 0
                || code != analyse.Resultat.Erreur) ContratSemantiqueIncoherent("rappel ou entrée modifiés");
            if (code == 3) throw std::bad_alloc();
            const auto& sortie = analyse.Resultat;
            if (code == 62 && sortie.Detail > 1) ContratSemantiqueIncoherent("contexte de condition/logique");
            if (code == 33 && (sortie.Detail > 1 || sortie.NombreSymboles != noeudsSymboles.size()))
                ContratSemantiqueIncoherent("contexte du champ absent/hérité");
            if ((code != 0 && code != 4 && code != 5 && !RefusSemantique(code))
                || sortie.NombreSymboles != sortie.CapaciteSymbolesRequise || sortie.NombreSymboles > noeudsSymboles.size()
                || sortie.NombreResolutions != sortie.CapaciteResolutionsRequise) ContratSemantiqueIncoherent("code ou tailles sémantiques");
            if (code == 25)
            {
                if (sortie.NombreSymboles != noeudsSymboles.size() || sortie.Detail >= noeudsSymboles.size())
                    ContratSemantiqueIncoherent("index du membre inaccessible");
                const auto& cible = copie.Noeuds[noeudsSymboles[sortie.Detail]];
                const auto proprietaire = copie.Noeuds[cible.Parent].Genre;
                if ((cible.Genre != 7 && cible.Genre != 12 && cible.Genre != 15)
                    || (cible.Genre == 7 ? proprietaire < 4 || proprietaire > 6 : proprietaire != 6))
                    ContratSemantiqueIncoherent("déclaration du membre inaccessible");
            }
            if (code == 34 || code == 35 || code == 36 || code == 38 || code == 40 || code == 41)
            {
                if (sortie.NombreSymboles != noeudsSymboles.size() || sortie.Detail >= noeudsSymboles.size())
                    ContratSemantiqueIncoherent("index du champ initialisé");
                const auto& cible = copie.Noeuds[noeudsSymboles[sortie.Detail]];
                if (cible.Genre != 7 || !cible.Parent || copie.Noeuds[cible.Parent].Genre != 6)
                    ContratSemantiqueIncoherent("déclaration du champ initialisé");
            }
            return requete;
        };
        auto requete = appeler();
        const auto mesure = analyse.Resultat;
        if (RefusSemantique(mesure.Erreur))
        {
            // L'absence de fonction est un refus global du programme, à la racine,
            // comme dans le bootstrap : aucune origine de fichier n'est inventée.
            if (mesure.Erreur == 20)
            {
                if (mesure.LigneErreur != 1 || mesure.ColonneErreur != 1 || mesure.Detail
                    || requete.IndexUniteErreur != 0 || requete.LigneLocaleErreur != 1 || requete.ColonneLocaleErreur != 1
                    || std::any_of(noeudsSymboles.begin(), noeudsSymboles.end(),
                        [&](auto index) { return GenreSymbole(copie.Noeuds[index].Genre) == 2; }))
                    ContratSemantiqueIncoherent("refus global sans fonction");
                resultat.Resultat = mesure;
                return resultat;
            }
            const auto trouve = positions.find({mesure.LigneErreur, mesure.ColonneErreur});
            if (trouve == positions.end())
                throw std::runtime_error("contrat de sémantique assemblée incohérent : position du refus absente de l'AST, code="
                    + std::to_string(mesure.Erreur) + ", ligne=" + std::to_string(mesure.LigneErreur)
                    + ", colonne=" + std::to_string(mesure.ColonneErreur) + ", noeuds=" + std::to_string(copie.Noeuds.size()));
            const auto& origine = copie.Origines[trouve->second];
            const auto& unite = copie.Unites[origine.IndexUnite];
            if (requete.IndexUniteErreur != origine.IndexUnite
                || requete.LigneLocaleErreur != mesure.LigneErreur - unite.PremiereLigne + 1
                || requete.ColonneLocaleErreur != mesure.ColonneErreur) ContratSemantiqueIncoherent("origine du refus");
            resultat.Resultat = mesure; resultat.PositionErreur = origine.Position;
            resultat.IndexUniteErreur = origine.IndexUnite;
            if ((mesure.Erreur == 33 || mesure.Erreur == 34 || mesure.Erreur == 35 || mesure.Erreur == 36)
                && !InitialiseurChampDiagnostic(resultat).Reference)
                ContratSemantiqueIncoherent("contexte du champ initialisé");
            if (mesure.Erreur == 41 && !InitialiseurChampDiagnostic(resultat).Reference
                && !ChampTableauParDefautDiagnostic(resultat)) ContratSemantiqueIncoherent("contexte du champ tableau");
            if ((mesure.Erreur == 38 || mesure.Erreur == 40) && !ChampDeclareDiagnostic(resultat))
                ContratSemantiqueIncoherent("origine du champ déclaré");
            if ((mesure.Erreur == 12 || mesure.Erreur == 13 || mesure.Erreur == 14 || mesure.Erreur == 15 || mesure.Erreur == 17)
                && !ConflitDeclarationDiagnostic(resultat).Reference)
                ContratSemantiqueIncoherent("déclarations en conflit");
            if (mesure.Erreur == 107 && !CycleAliasChampDiagnostic(resultat).Reference)
                ContratSemantiqueIncoherent("cycle d'alias de champ");
            if (mesure.Erreur == 109 && !CycleAliasRacineDiagnostic(resultat).Reference)
                ContratSemantiqueIncoherent("cycle d'alias racine");
            if (mesure.Erreur == 120 && !EspaceUtiliseDiagnostic(resultat))
                ContratSemantiqueIncoherent("espace utilisé introuvable");
            if (mesure.Erreur == 121)
            {
                const bool baseNominale = std::any_of(copie.Noeuds.begin(), copie.Noeuds.end(), [&](const auto& n)
                { return n.Genre == 40 && n.Parent < copie.Noeuds.size() && OrigineDiagnosticCorrespond(resultat, n.Parent); });
                if (baseNominale && !BaseClasseAmbigueDiagnostic(resultat).Reference)
                    ContratSemantiqueIncoherent("base de classe ambiguë possédée");
                const bool typeNominal = std::any_of(copie.Noeuds.begin(), copie.Noeuds.end(), [&](const auto& n)
                { return (n.Genre == 41 || n.Genre == 42) && n.Parent < copie.Noeuds.size() && OrigineDiagnosticCorrespond(resultat, n.Parent); });
                const bool typeConversion = std::any_of(copie.Noeuds.begin(), copie.Noeuds.end(), [&](const auto& n)
                { return n.Genre == 43 && n.Parent < copie.Noeuds.size() && OrigineDiagnosticCorrespond(resultat, n.Parent); });
                if ((typeConversion && !NomTypeConversionAmbiguDiagnostic(resultat).Reference)
                    || (typeNominal && !NomTypeAmbiguDiagnostic(resultat).Reference)
                    || (mesure.Detail && !baseNominale && !typeNominal && !typeConversion))
                    ContratSemantiqueIncoherent("type déclaré ambigu possédé");
                const bool referenceNominale = std::any_of(copie.Noeuds.begin(), copie.Noeuds.end(), [&](const auto& n)
                { return n.Genre == 24 && OrigineDiagnosticCorrespond(resultat, static_cast<std::size_t>(&n - copie.Noeuds.data())); });
                const bool cibleNominale = std::any_of(copie.Noeuds.begin(), copie.Noeuds.end(), [&](const auto& n)
                { return n.Genre == 38 && n.Parent < copie.Noeuds.size() && OrigineDiagnosticCorrespond(resultat, n.Parent); });
                if ((referenceNominale && !ReferenceDiagnosticPossede(resultat))
                    || (cibleNominale && !CibleAliasRacineDiagnostic(resultat).Reference))
                    ContratSemantiqueIncoherent("nom importé ambigu possédé");
            }
            if (mesure.Erreur == 108)
            {
                const bool cibleNominale = mesure.Detail || std::any_of(copie.Noeuds.begin(), copie.Noeuds.end(), [&](const auto& n)
                { return n.Genre == 37 && n.Parent < copie.Noeuds.size() && OrigineDiagnosticCorrespond(resultat, n.Parent); });
                if (cibleNominale && !CibleAliasChampDiagnostic(resultat).Reference)
                    ContratSemantiqueIncoherent("cible nominale d'alias de champ");
            }
            if (mesure.Erreur == 110 || mesure.Erreur == 112)
            {
                const bool cibleNominale = (mesure.Erreur == 110 && mesure.Detail)
                    || std::any_of(copie.Noeuds.begin(), copie.Noeuds.end(), [&](const auto& n)
                { return n.Genre == 38 && n.Parent < copie.Noeuds.size() && OrigineDiagnosticCorrespond(resultat, n.Parent); });
                if (cibleNominale && !CibleAliasRacineDiagnostic(resultat).Reference)
                    ContratSemantiqueIncoherent("cible nominale d'alias racine");
            }
            return resultat;
        }
        if (mesure.NombreSymboles != noeudsSymboles.size()) ContratSemantiqueIncoherent("nombre de symboles mesurés");
        if (mesure.NombreSymboles > limites.MaximumSymboles || mesure.NombreResolutions > limites.MaximumResolutions)
            throw std::length_error("limite de sortie sémantique assemblée dépassée");
        const auto codeMesure = mesure.NombreSymboles ? 4U : mesure.NombreResolutions ? 5U : 0U;
        if (mesure.Erreur != codeMesure) ContratSemantiqueIncoherent("code de mesure");
        auto verifierSucces = [&]()
        {
            if (analyse.Resultat.Erreur || analyse.Resultat.LigneErreur || analyse.Resultat.ColonneErreur || analyse.Resultat.Detail
                || requete.IndexUniteErreur != copie.Unites.size() || requete.LigneLocaleErreur || requete.ColonneLocaleErreur
                || analyse.Resultat.NombreSymboles != mesure.NombreSymboles || analyse.Resultat.NombreResolutions != mesure.NombreResolutions)
                ContratSemantiqueIncoherent("publication ou succès vide");
        };
        if (!codeMesure) { verifierSucces(); resultat.Resultat = mesure; return resultat; }
        if (mesure.LigneErreur != 1 || mesure.ColonneErreur != 1 || requete.IndexUniteErreur != 0
            || requete.LigneLocaleErreur != 1 || requete.ColonneLocaleErreur != 1
            || mesure.Detail != (codeMesure == 4 ? mesure.NombreSymboles : mesure.NombreResolutions)) ContratSemantiqueIncoherent("diagnostic de capacité mesurée");
        SymboleSemantiqueHote gardeSymbole; ResolutionSemantiqueHote gardeResolution;
        std::memset(&gardeSymbole, 0xA5, sizeof(gardeSymbole)); std::memset(&gardeResolution, 0xA5, sizeof(gardeResolution));
        std::vector<SymboleSemantiqueHote> symboles(mesure.NombreSymboles + 1, gardeSymbole);
        std::vector<ResolutionSemantiqueHote> resolutions(mesure.NombreResolutions + 1, gardeResolution);
        analyse.Symboles = symboles.data(); analyse.CapaciteSymboles = mesure.NombreSymboles;
        analyse.Resolutions = resolutions.data(); analyse.CapaciteResolutions = mesure.NombreResolutions;
        requete = appeler(); verifierSucces();
        if (std::memcmp(&symboles.back(), &gardeSymbole, sizeof(gardeSymbole)) != 0
            || std::memcmp(&resolutions.back(), &gardeResolution, sizeof(gardeResolution)) != 0) ContratSemantiqueIncoherent("gardes");
        symboles.pop_back(); resolutions.pop_back();
        for (std::size_t i = 0; i < symboles.size(); ++i)
        {
            const auto& s = symboles[i]; const auto index = noeudsSymboles[i]; const auto& n = copie.Noeuds[index];
            if (s.IndexNoeud != index || s.IndexPortee != n.Parent || s.Genre != GenreSymbole(n.Genre)
                || s.HachageNom != n.HachageNom || s.HachageEspace != n.HachageEspace || s.Drapeaux != n.Drapeaux)
                ContratSemantiqueIncoherent("symbole");
        }
        for (const auto& resolution : resolutions)
        {
            if (!resolution.IndexNoeud || resolution.IndexNoeud >= copie.Noeuds.size() || resolution.IndexSymbole >= symboles.size()
                || (resolution.Drapeaux & ~0x3FFFFFU)) ContratSemantiqueIncoherent("indices ou drapeaux de résolution");
            const auto& cible = symboles[resolution.IndexSymbole];
            if (resolution.GenreCible != cible.Genre
                || (!(resolution.Drapeaux & 32768U) && resolution.HachageType != cible.HachageType)) ContratSemantiqueIncoherent("type ou genre de résolution");
        }
        resultat.Resultat = analyse.Resultat;
        resultat.Symboles = std::move(symboles); resultat.Resolutions = std::move(resolutions);
        return resultat;
    }
}
