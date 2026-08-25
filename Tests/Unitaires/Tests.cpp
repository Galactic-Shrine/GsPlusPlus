#include "GsPP/AnalyseurSyntaxique.hpp"
#include "GsPP/AnalyseurSemantique.hpp"
#include "GsPP/BibliothequeGsA.hpp"
#include "GsPP/ChargeurGsE.hpp"
#include "GsPP/Compilation.hpp"
#include "GsPP/ContexteDemarrage.hpp"
#include "GsPP/EditeurLiens.hpp"
#include "GsPP/EcrivainCoff.hpp"
#include "GsPP/EcrivainGsE.hpp"
#include "GsPP/ErreurCompilation.hpp"
#include "GsPP/FormatGsE.hpp"
#include "GsPP/FormatGsO.hpp"
#include "GsPP/GenerateurX64.hpp"
#include "GsPP/Lexeur.hpp"
#include "GsPP/ObjetGsO.hpp"
#include "GsPP/VerificateurGsE.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace
{
    void Exiger(bool condition, const std::string& message)
    {
        if (!condition) throw std::runtime_error(message);
    }

    void TesterExtensionsGalacticShrine()
    {
        Exiger(GsPP::EstExtensionSource("Unite.Gs++")
                   && GsPP::EstExtensionSource("Unite.GsPP")
                   && GsPP::EstExtensionSource("Unite.GsPlusPlus"),
               "une extension source Gs++ actuelle n’est pas reconnue");
        Exiger(GsPP::EstExtensionInterface("Unite.HGs++")
                   && GsPP::EstExtensionInterface("Unite.HGsPP")
                   && GsPP::EstExtensionInterface("Unite.HeaderGsPlusPlus"),
               "une extension d’interface Gs++ actuelle n’est pas reconnue");
        Exiger(GsPP::EstExtensionGsSharp("Unite.Gs#")
                   && GsPP::EstExtensionGsSharp("Unite.GsS")
                   && GsPP::EstExtensionGsSharp("Unite.GsSharp"),
               "une extension réservée à Gs# n’est pas reconnue");
        Exiger(GsPP::EstExtensionObsolete("Unite.GsPH")
                   && GsPP::EstExtensionObsolete("Unite.GsO")
                   && GsPP::EstExtensionObsolete("Unite.GsPPH")
                   && GsPP::EstExtensionObsolete("Unite.GsPlusPlusHeader"),
               "une extension obsolète Galactic-Shrine n’est pas reconnue");
    }

    GsPP::CodeMachine Compiler(const std::string& source)
    {
        auto jetons = GsPP::Lexeur(source).Analyser();
        auto programme = GsPP::AnalyseurSyntaxique(std::move(jetons)).Analyser();
        GsPP::AnalyseurSemantique().Analyser(programme);
        return GsPP::GenerateurX64().Generer(programme);
    }

    GsPP::Programme Analyser(const std::string& source)
    {
        auto jetons = GsPP::Lexeur(source, "test.GsPP").Analyser();
        auto programme = GsPP::AnalyseurSyntaxique(std::move(jetons), "test.GsPP").Analyser();
        GsPP::AnalyseurSemantique().Analyser(programme);
        return programme;
    }

    void TesterAliasMotsCles()
    {
        const std::string francais =
            "publique entier32 Calculer() { retourner 42; }";
        const std::string anglais =
            "public int32 Calculer() { return 42; }";

        const auto codeFrancais = Compiler(francais);
        const auto codeAnglais = Compiler(anglais);

        Exiger(codeFrancais.Texte == codeAnglais.Texte,
               "les alias anglais ne produisent pas le même code");
        Exiger(codeFrancais.Symboles.size() == 1, "symbole français absent");
        Exiger(codeAnglais.Symboles.size() == 1, "symbole anglais absent");
        Exiger(codeFrancais.Symboles[0].Nom == codeAnglais.Symboles[0].Nom,
               "le symbole normalisé diffère");
    }

    void TesterAliasApplicatifs()
    {
        auto programme = Analyser(R"(
            alias Shrine::Test::Start = Sanctuaire::Test::Principal;
            alias Shrine::Test::Point = Sanctuaire::Test::Point;

            espace Sanctuaire::Test
            {
                alias PointAlias = Point;
                alias Counter = Compteur;
                alias Add = Ajouter;
                alias AddAgain = Add;

                structure Point
                {
                    entier32 X;
                    alias Horizontal = X;
                    alias HorizontalValue = Horizontal;
                };

                publique entier32 Compteur = 40;

                publique entier32 Ajouter(PointAlias* point)
                {
                    point->Horizontal = point->HorizontalValue + 1;
                    Counter = Counter + 1;
                    retourner point->X + Compteur;
                }

                publique entier32 Principal()
                {
                    PointAlias point;
                    point.Horizontal = 0;
                    retourner AddAgain(&point);
                }
            }

            publique entier32 LirePointAnglais(Shrine::Test::Point* point)
            {
                retourner point->HorizontalValue;
            }
        )");

        Exiger(programme.Aliases.size() == 6, "nombre d’alias applicatifs incorrect");
        Exiger(programme.Structures[0].AliasesChamps.size() == 2,
               "alias de champs absents");
        Exiger(programme.Structures[0].AliasesChamps[0].CibleCanonique == "X",
               "cible canonique d’alias de champ incorrecte");
        Exiger(programme.Structures[0].AliasesChamps[1].CibleCanonique == "X",
               "chaîne d’alias de champ non normalisée");

        const auto machine = GsPP::GenerateurX64().Generer(programme);
        auto symbole = [&](std::string_view nom) -> const GsPP::SymboleMachine&
        {
            const auto trouve = std::find_if(
                machine.Symboles.begin(), machine.Symboles.end(),
                [&](const GsPP::SymboleMachine& valeur) { return valeur.Nom == nom; });
            if (trouve == machine.Symboles.end())
                throw std::runtime_error("symbole d’alias absent : " + std::string(nom));
            return *trouve;
        };

        const auto& ajouter = symbole("Sanctuaire::Test::Ajouter");
        const auto& add = symbole("Sanctuaire::Test::Add");
        const auto& addAgain = symbole("Sanctuaire::Test::AddAgain");
        Exiger(ajouter.Decalage == add.Decalage && ajouter.Decalage == addAgain.Decalage,
               "les alias de fonction ne partagent pas la même adresse");
        Exiger(ajouter.Taille == add.Taille && ajouter.Taille == addAgain.Taille,
               "les alias de fonction ne partagent pas la même implémentation");

        const auto& compteur = symbole("Sanctuaire::Test::Compteur");
        const auto& counter = symbole("Sanctuaire::Test::Counter");
        Exiger(compteur.Section == counter.Section && compteur.Decalage == counter.Decalage,
               "l’alias de globale ne partage pas le même stockage");

        const auto& principal = symbole("Sanctuaire::Test::Principal");
        const auto& start = symbole("Shrine::Test::Start");
        Exiger(principal.Decalage == start.Decalage && principal.Taille == start.Taille,
               "l’alias entièrement qualifié ne vise pas la fonction canonique");

        for (const auto& relocalisation : machine.Relocalisations)
            Exiger(relocalisation.Symbole != "Sanctuaire::Test::Add"
                       && relocalisation.Symbole != "Sanctuaire::Test::AddAgain"
                       && relocalisation.Symbole != "Sanctuaire::Test::Counter",
                   "une relocalisation n’a pas été normalisée vers son symbole canonique");

        const auto gse = GsPP::EcrivainGsE().Construire(
            machine, "Shrine::Test::Start");
        const auto image = GsPP::ChargeurGsE().Charger(gse, 0x10000000);
        const auto exportAjouter = image.ChercherExport("Sanctuaire::Test::Ajouter");
        const auto exportAdd = image.ChercherExport("Sanctuaire::Test::Add");
        const auto exportStart = image.ChercherExport("Shrine::Test::Start");
        const auto exportCompteur = image.ChercherExport("Sanctuaire::Test::Compteur");
        const auto exportCounter = image.ChercherExport("Sanctuaire::Test::Counter");
        Exiger(exportAjouter && exportAdd && *exportAjouter == *exportAdd,
               "les exports de fonction aliasés n’ont pas la même adresse");
        Exiger(exportStart && *exportStart == image.AdressePointEntree,
               "un alias public ne peut pas servir de point d’entrée");
        Exiger(exportCompteur && exportCounter && *exportCompteur == *exportCounter,
               "les exports de globale aliasés n’ont pas la même adresse");

        const auto machineImport = Compiler(R"(
            externe entier32 Journaliser(entier32 valeur);
            alias Log = Journaliser;
            publique entier32 Principal() { retourner Log(42); }
        )");
        const auto imports = std::count_if(
            machineImport.Symboles.begin(), machineImport.Symboles.end(),
            [](const GsPP::SymboleMachine& valeur) { return !valeur.EstDefini; });
        Exiger(imports == 1, "un alias d’import a créé un second import obligatoire");
        Exiger(std::none_of(
                   machineImport.Symboles.begin(), machineImport.Symboles.end(),
                   [](const GsPP::SymboleMachine& valeur) { return valeur.Nom == "Log"; }),
               "un symbole machine d’alias d’import a été émis");
        Exiger(machineImport.Relocalisations.size() == 1
                   && machineImport.Relocalisations[0].Symbole == "Journaliser",
               "l’appel par alias d’import n’a pas été normalisé");
    }

    void TesterErreursAlias()
    {
        auto doitEchouer = [](const std::string& source, std::string_view fragment)
        {
            try
            {
                (void)Analyser(source);
            }
            catch (const std::exception& erreur)
            {
                Exiger(std::string(erreur.what()).find(fragment) != std::string::npos,
                       "diagnostic d’alias inattendu : " + std::string(erreur.what()));
                return;
            }
            throw std::runtime_error("une déclaration d’alias invalide a été acceptée");
        };

        doitEchouer(
            "publique entier32 Principal() { retourner 0; } alias A = B; alias B = A;",
            "cycle d’alias");
        doitEchouer(
            "publique entier32 Principal() { retourner 0; } alias Inconnu = Introuvable;",
            "cible d’alias introuvable");
        doitEchouer(
            "publique entier32 Principal() { retourner 0; } alias Principal = Principal;",
            "conflit avec une déclaration");
        doitEchouer(R"(
            structure Point
            {
                entier32 X;
                alias A = B;
                alias B = A;
            };
            publique entier32 Principal() { retourner 0; }
        )", "cycle d’alias de champ");
    }

    void TesterTypesSysteme()
    {
        auto programme = Analyser(R"(
            énumération Etat { Arrete = 3, Actif, Termine = 9, };

            structure Disposition
            {
                naturel8 A;
                entier64 B;
                naturel16 C;
                booléen D;
                octet E;
                caractère F;
                naturel32 Valeurs[3];
            };

            union Stockage
            {
                naturel8 Petit;
                naturel64 Grand;
                naturel16 Mots[5];
            };

            publique naturel8 G8 = 1;
            publique naturel16 G16 = 2;
            publique naturel32 G32 = 3;
            publique naturel64 G64 = 18_446_744_073_709_551_615;
            publique booléen GB = vrai;
            publique Etat GE = Etat::Actif;
            publique entier64 GSigneEtendu = convertir<entier64>(convertir<entier8>(-1));
            naturel16 TableauGlobal[3];

            publique entier32 Principal()
            {
                constante entier32 limite = 4;
                volatile naturel32 temoin = 4;
                Etat etat = convertir<Etat>(9);
                si (etat == Etat::Termine) { retourner limite; }
                retourner convertir<entier32>(temoin);
            }
        )");

        Exiger(programme.Enumerations.size() == 1,
               "énumération système absente");
        Exiger(programme.Enumerations[0].Valeurs[1].Valeur == 4
                   && programme.Enumerations[0].Valeurs[2].Valeur == 9,
               "valeurs d’énumération incorrectes");
        Exiger(programme.Structures.size() == 2,
               "structure et union système absentes");
        const auto& disposition = programme.Structures[0];
        Exiger(!disposition.EstUnion && disposition.Alignement == 8
                   && disposition.Taille == 40,
               "disposition de structure multi-types incorrecte");
        Exiger(disposition.Champs[0].Decalage == 0
                   && disposition.Champs[1].Decalage == 8
                   && disposition.Champs[2].Decalage == 16
                   && disposition.Champs[6].Decalage == 24,
               "décalages de champs multi-types incorrects");
        const auto& stockage = programme.Structures[1];
        Exiger(stockage.EstUnion && stockage.Alignement == 8
                   && stockage.Taille == 16,
               "disposition d’union incorrecte");
        Exiger(std::all_of(
                   stockage.Champs.begin(), stockage.Champs.end(),
                   [](const GsPP::ChampStructure& champ) { return champ.Decalage == 0; }),
               "les champs d’union ne partagent pas le décalage zéro");

        const auto machine = GsPP::GenerateurX64().Generer(programme);
        auto tailleSymbole = [&](std::string_view nom)
        {
            const auto trouve = std::find_if(
                machine.Symboles.begin(), machine.Symboles.end(),
                [&](const GsPP::SymboleMachine& symbole) { return symbole.Nom == nom; });
            if (trouve == machine.Symboles.end())
                throw std::runtime_error("symbole scalaire absent : " + std::string(nom));
            return trouve->Taille;
        };
        Exiger(tailleSymbole("G8") == 1 && tailleSymbole("G16") == 2
                   && tailleSymbole("G32") == 4 && tailleSymbole("G64") == 8
                   && tailleSymbole("GB") == 1 && tailleSymbole("GE") == 4
                   && tailleSymbole("TableauGlobal") == 6,
               "tailles machine des types système incorrectes");
        Exiger(machine.Donnees.size() == 32,
               "sérialisation des globales multi-tailles incorrecte");
        const auto globale64 = std::find_if(
            machine.Symboles.begin(), machine.Symboles.end(),
            [](const GsPP::SymboleMachine& symbole) { return symbole.Nom == "G64"; });
        Exiger(globale64 != machine.Symboles.end()
                   && std::all_of(
                       machine.Donnees.begin() + globale64->Decalage,
                       machine.Donnees.begin() + globale64->Decalage + 8,
                       [](std::uint8_t octet) { return octet == 0xFF; }),
               "le littéral naturel64 maximal n’est pas sérialisé correctement");
        const auto signeeEtendue = std::find_if(
            machine.Symboles.begin(), machine.Symboles.end(),
            [](const GsPP::SymboleMachine& symbole) { return symbole.Nom == "GSigneEtendu"; });
        Exiger(signeeEtendue != machine.Symboles.end()
                   && std::all_of(
                       machine.Donnees.begin() + signeeEtendue->Decalage,
                       machine.Donnees.begin() + signeeEtendue->Decalage + 8,
                       [](std::uint8_t octet) { return octet == 0xFF; }),
               "l’extension signée d’une conversion constante est incorrecte");

        const auto machineAnglaise = Compiler(R"(
            enum State { Idle = 1, Running, };
            union Value { uint64 Whole; uint16 Words[4]; };
            public int32 Main()
            {
                const uint8 small = 7;
                volatile int16 signedValue = -2;
                byte raw = cast<byte>(200);
                char character = -1;
                bool valid = true;
                State state = State::Running;
                const int32 immutable = 2;
                const int32* pointer = &immutable;
                pointer = &immutable;
                if (valid == false) { return 0; }
                return cast<int32>(small) + cast<int32>(signedValue)
                    + cast<int32>(raw) + cast<int32>(character)
                    + cast<int32>(state) + *pointer;
            }
        )");
        Exiger(!machineAnglaise.Texte.empty(),
               "les alias anglais des types système ne compilent pas");

        auto doitEchouer = [](const std::string& source, std::string_view fragment)
        {
            try { (void)Analyser(source); }
            catch (const std::exception& erreur)
            {
                Exiger(std::string(erreur.what()).find(fragment) != std::string::npos,
                       "diagnostic de type système inattendu : " + std::string(erreur.what()));
                return;
            }
            throw std::runtime_error("une construction de type système invalide a été acceptée");
        };
        doitEchouer(
            "publique entier32 Principal() { constante entier32 x = 1; x = 2; retourner x; }",
            "constante");
        doitEchouer(
            "publique entier32 Principal() { constante entier32 x = 1; constante entier32* p = &x; *p = 2; retourner x; }",
            "constante");
        doitEchouer(
            "publique entier32 Principal() { entier32 x = vrai; retourner x; }",
            "incompatible");
        doitEchouer(
            "publique entier32 Principal() { naturel8 x = 256; retourner convertir<entier32>(x); }",
            "plage");
        doitEchouer(
            "publique entier32 Principal() { naturel8 x = convertir<naturel8>(300); retourner 0; }",
            "conversion constante");
        doitEchouer(
            "publique entier32 Principal() { entier32 valeurs[0]; retourner 0; }",
            "taille de tableau");
    }

    void TesterContratDemarrage()
    {
        const GsPP::ContexteDemarrage contexte;
        Exiger(contexte.Version == 3, "version du contrat de démarrage incorrecte");
        Exiger(contexte.TailleStructure == 320,
               "taille annoncée du contrat de démarrage incorrecte");
        Exiger(sizeof(GsPP::PlageMemoirePhysique) == 16,
               "taille de PlageMemoirePhysique incorrecte");
        Exiger(sizeof(GsPP::ContexteDemarrage) == 320,
               "taille de ContexteDemarrage incorrecte");
        Exiger(offsetof(GsPP::ContexteDemarrage, TablePages) == 104,
               "décalage de TablePages incorrect");
        Exiger(offsetof(GsPP::ContexteDemarrage, PlagesMemoireLibres) == 112,
               "décalage des plages mémoire incorrect");
        Exiger(offsetof(GsPP::ContexteDemarrage, DerniereException) == 128,
               "décalage du diagnostic d’exception incorrect");
        Exiger(offsetof(GsPP::ContexteDemarrage, PileNoyau) == 136,
               "décalage de la pile noyau incorrect");
        Exiger(offsetof(GsPP::ContexteDemarrage, Tss) == 168,
               "décalage de la TSS incorrect");
        Exiger(offsetof(GsPP::ContexteDemarrage, NombreTicksHorloge) == 200,
               "décalage de l’horloge incorrect");
        Exiger(offsetof(GsPP::ContexteDemarrage, GsiHorloge) == 224,
               "décalage du GSI horloge incorrect");
        Exiger(offsetof(GsPP::ContexteDemarrage, EtapeDemarrage) == 232,
               "décalage de l’étape de démarrage incorrect");
        Exiger(offsetof(GsPP::ContexteDemarrage, AdresseInstructionException) == 240,
               "décalage de l’adresse d’instruction incorrect");
        Exiger(offsetof(GsPP::ContexteDemarrage, AdresseMemoireException) == 248,
               "décalage de l’adresse mémoire incorrect");
        Exiger(offsetof(GsPP::ContexteDemarrage, AdresseInstructionInterruption) == 256,
               "décalage du RIP d’interruption incorrect");
        Exiger(offsetof(GsPP::ContexteDemarrage, PointeurCadreInterruption) == 264,
               "décalage du cadre d’interruption incorrect");
        Exiger(offsetof(GsPP::ContexteDemarrage, SelecteurCodeInterruption) == 272,
               "décalage du sélecteur d’interruption incorrect");
        Exiger(offsetof(GsPP::ContexteDemarrage, DrapeauxInterruption) == 280,
               "décalage des drapeaux d’interruption incorrect");
        Exiger(offsetof(GsPP::ContexteDemarrage, TaillePageMemoire) == 288,
               "décalage de la taille de page incorrect");
        Exiger(offsetof(GsPP::ContexteDemarrage, NombrePagesTablesPagination) == 292,
               "décalage du nombre de pages de pagination incorrect");
        Exiger(offsetof(GsPP::ContexteDemarrage, NombrePagesCartographiees) == 296,
               "décalage de la couverture de pagination incorrect");
        Exiger(offsetof(GsPP::ContexteDemarrage, NombrePagesRecuperees) == 300,
               "décalage des pages récupérées incorrect");
        Exiger(offsetof(GsPP::ContexteDemarrage, LimitePagination) == 304,
               "décalage de la limite de pagination incorrect");
        Exiger(offsetof(GsPP::ContexteDemarrage, CapacitePlagesMemoire) == 312,
               "décalage de la capacité des plages incorrect");
    }

    void TesterPrecedence()
    {
        const auto machine = Compiler(
            "publique entier32 Principal() { retourner 2 + 3 * 4; }");
        Exiger(!machine.Texte.empty(), "aucun code généré");
        Exiger(machine.Texte.back() == 0xC3, "instruction ret absente");
    }

    void TesterChainesEtLogique()
    {
        const auto jetons = GsPP::Lexeur(
            R"("Gs\n++" && vrai || faux)").Analyser();
        Exiger(
            jetons.size() == 6
                && jetons[0].Genre == GsPP::GenreJeton::ChaineCaracteres
                && jetons[0].Texte == "Gs\n++"
                && jetons[1].Genre == GsPP::GenreJeton::EtLogique
                && jetons[3].Genre == GsPP::GenreJeton::OuLogique,
            "lexage des chaînes ou des opérateurs logiques incorrect");

        auto verifierErreurChaine = [](std::string_view source)
        {
            try
            {
                (void)GsPP::Lexeur(source, "chaine.GsPP").Analyser();
            }
            catch (const GsPP::ErreurCompilation&)
            {
                return;
            }
            throw std::runtime_error(
                "un littéral chaîne invalide a été accepté");
        };
        verifierErreurChaine("\"inachevee");
        verifierErreurChaine("\"mauvais\\q\"");

        const auto machine = Compiler(R"(
            publique booléen CourtCircuit =
                faux && (1 / 0 == 0);
            publique constante caractère* ObtenirTexte()
            {
                retourner "Gs\n++";
            }
            publique constante caractère* ObtenirTexteDeux()
            {
                retourner "Gs\n++";
            }
            publique constante caractère* RendreLectureSeule(caractère* texte)
            {
                constante caractère* vue =
                    convertir<constante caractère*>(texte);
                retourner vue;
            }
            publique booléen TesterLogique(
                booléen gauche, booléen droite)
            {
                retourner gauche && droite || !gauche;
            }
        )");
        const std::array<std::uint8_t, 6> texte{
            'G', 's', '\n', '+', '+', 0};
        Exiger(
            std::search(
                machine.Donnees.begin(),
                machine.Donnees.end(),
                texte.begin(),
                texte.end()) != machine.Donnees.end(),
            "octets du littéral chaîne absents de la section de données");
        const auto symboleChaine = std::find_if(
            machine.Symboles.begin(),
            machine.Symboles.end(),
            [](const GsPP::SymboleMachine& symbole)
            {
                return symbole.Nom.find("@GsChaine") == 0
                    && symbole.Section == GsPP::SectionMachine::Donnees
                    && !symbole.EstPublic;
            });
        Exiger(symboleChaine != machine.Symboles.end(),
               "symbole local du littéral chaîne absent");
        Exiger(
            std::count_if(
                machine.Symboles.begin(),
                machine.Symboles.end(),
                [](const GsPP::SymboleMachine& symbole)
                {
                    return symbole.Nom.find("@GsChaine") == 0;
                }) == 1,
            "deux littéraux identiques n’ont pas partagé leur stockage");
        Exiger(
            std::any_of(
                machine.Relocalisations.begin(),
                machine.Relocalisations.end(),
                [&](const GsPP::CodeMachine::Relocalisation& relocalisation)
                {
                    return relocalisation.Symbole == symboleChaine->Nom
                        && relocalisation.Section
                            == GsPP::SectionMachine::Texte
                        && relocalisation.Type
                            == GsPP::TypeRelocalisationMachine::Relatif32;
                }),
            "relocalisation du littéral chaîne absente");

        bool ecritureRefusee = false;
        try
        {
            (void)Analyser(R"(
                publique entier32 Principal()
                {
                    "lecture seule"[0] = 65;
                    retourner 0;
                }
            )");
        }
        catch (const GsPP::ErreurCompilation& erreur)
        {
            ecritureRefusee = std::string(erreur.what()).find(
                "constante") != std::string::npos;
        }
        Exiger(ecritureRefusee,
               "l’écriture dans un littéral chaîne a été acceptée");
    }

    void TesterBitsEtIntrinseques()
    {
        const auto programme = Analyser(R"(
            espace GalacticShrine::GsPP::Intrinseques
            {
                externe naturel32 ChargerAtomique32(volatile naturel32* cible);
                externe naturel64 ChargerAtomique64(volatile naturel64* cible);
                externe vide StockerAtomique32(
                    volatile naturel32* cible, naturel32 valeur);
                externe naturel32 EchangerAtomique32(
                    volatile naturel32* cible, naturel32 valeur);
                externe naturel32 AjouterAtomique32(
                    volatile naturel32* cible, naturel32 valeur);
                externe naturel32 ComparerEchanger32(
                    volatile naturel32* cible, naturel32 attendu, naturel32 desire);
                externe vide BarriereMemoire();
                externe vide PauseProcesseur();
            }

            publique naturel32 PrioriteBits = 1 | 2 ^ 3 & 6;
            publique naturel32 DecalageBits = convertir<naturel32>(3) << 5;
            publique entier32 DecalageSigne = -16 >> 2;

            publique naturel32 TesterAtomiques(volatile naturel32* cible)
            {
                naturel32 avant = GalacticShrine::GsPP::Intrinseques::ChargerAtomique32(cible);
                GalacticShrine::GsPP::Intrinseques::StockerAtomique32(cible, avant + 1);
                avant = GalacticShrine::GsPP::Intrinseques::EchangerAtomique32(cible, 2);
                avant = GalacticShrine::GsPP::Intrinseques::AjouterAtomique32(cible, 3);
                avant = GalacticShrine::GsPP::Intrinseques::ComparerEchanger32(cible, 5, 8);
                GalacticShrine::GsPP::Intrinseques::BarriereMemoire();
                GalacticShrine::GsPP::Intrinseques::PauseProcesseur();
                retourner (~avant & 255) | (avant ^ 7);
            }
        )");
        const auto machine = GsPP::GenerateurX64().Generer(programme);

        auto lire32 = [&](std::string_view nom)
        {
            const auto trouve = std::find_if(
                machine.Symboles.begin(), machine.Symboles.end(),
                [&](const GsPP::SymboleMachine& symbole)
                { return symbole.Nom == nom; });
            Exiger(trouve != machine.Symboles.end(),
                   "globale binaire absente : " + std::string(nom));
            std::uint32_t valeur = 0;
            for (int index = 0; index < 4; ++index)
                valeur |= static_cast<std::uint32_t>(
                    machine.Donnees[trouve->Decalage + index]) << (index * 8);
            return valeur;
        };
        Exiger(lire32("PrioriteBits") == 1,
               "priorité des opérateurs binaires incorrecte");
        Exiger(lire32("DecalageBits") == 96,
               "évaluation constante du décalage incorrecte");
        Exiger(lire32("DecalageSigne") == 0xFFFFFFFCU,
               "décalage arithmétique constant incorrect");

        const auto contient = [&](std::initializer_list<std::uint8_t> octets)
        {
            return std::search(
                       machine.Texte.begin(), machine.Texte.end(),
                       octets.begin(), octets.end()) != machine.Texte.end();
        };
        Exiger(contient({0xF0, 0x0F, 0xC1, 0x01}),
               "encodage lock xadd absent");
        Exiger(contient({0xF0, 0x44, 0x0F, 0xB1, 0x01}),
               "encodage lock cmpxchg absent");
        Exiger(contient({0x0F, 0xAE, 0xF0}), "encodage mfence absent");
        Exiger(contient({0xF3, 0x90}), "encodage pause absent");
        Exiger(std::none_of(
                   machine.Symboles.begin(), machine.Symboles.end(),
                   [](const GsPP::SymboleMachine& symbole)
                   {
                       return !symbole.EstDefini
                           && symbole.Nom.starts_with("GalacticShrine::GsPP::Intrinseques::");
                   }),
               "une intrinsèque intégrée subsiste comme import externe");
        Exiger(std::none_of(
                   machine.Relocalisations.begin(), machine.Relocalisations.end(),
                   [](const GsPP::CodeMachine::Relocalisation& relocalisation)
                   { return relocalisation.Symbole.starts_with("GalacticShrine::GsPP::Intrinseques::"); }),
               "une intrinsèque intégrée possède encore une relocalisation");

        const auto sansImportInutile = Compiler(R"(
            externe entier32 ServiceInutilise(entier32 valeur);
            publique entier32 Principal() { retourner 42; }
        )");
        Exiger(std::none_of(
                   sansImportInutile.Symboles.begin(), sansImportInutile.Symboles.end(),
                   [](const GsPP::SymboleMachine& symbole) { return !symbole.EstDefini; }),
               "un prototype d’interface inutilisé est devenu un import");

        const auto typeImbrique = Compiler(R"(
            publique entier32 Principal()
            {
                pointeur_fonction<pointeur_fonction<entier32()>()> rappel;
                retourner 0;
            }
        )");
        Exiger(!typeImbrique.Texte.empty(),
               "la fermeture >> d’un type imbriqué est confondue avec un décalage");

        bool prototypeRefuse = false;
        try
        {
            (void)Compiler(R"(
                espace GalacticShrine::GsPP::Intrinseques
                {
                    externe naturel64 ChargerAtomique32(
                        volatile naturel32* cible);
                }
                publique naturel64 Principal(volatile naturel32* cible)
                {
                    retourner GalacticShrine::GsPP::Intrinseques::ChargerAtomique32(cible);
                }
            )");
        }
        catch (const std::exception& erreur)
        {
            prototypeRefuse = std::string(erreur.what()).find(
                "prototype invalide") != std::string::npos;
        }
        Exiger(prototypeRefuse,
               "un prototype d’intrinsèque réservé invalide a été accepté");
    }

    void TesterVariablesControlesEtAppel()
    {
        const auto machine = Compiler(R"(
            espace Shrine::Test
            {
                publique entier32 Additionner(entier32 gauche, entier32 droite)
                {
                    retourner gauche + droite;
                }

                publique entier32 Principal()
                {
                    entier32 résultat = Additionner(20, 22);
                    si (résultat == 42)
                    {
                        résultat = résultat + 1;
                    }
                    sinon
                    {
                        résultat = 0;
                    }
                    tantque (résultat < 45)
                    {
                        résultat = résultat + 1;
                    }
                    retourner résultat;
                }
            }
        )");

        Exiger(machine.Symboles.size() == 2, "les deux fonctions sont attendues");
        Exiger(machine.Relocalisations.size() == 1, "une relocalisation d’appel est attendue");
        Exiger(machine.Relocalisations[0].Symbole == "Shrine::Test::Additionner",
               "la cible de l’appel local est incorrecte");
    }

    void TesterEspaceUnicode()
    {
        const auto machine = Compiler(
            "espace Shrine::Mémoire { publique entier32 Évaluer() { retourner -7; } }");
        Exiger(machine.Symboles.size() == 1, "fonction Unicode absente");
        Exiger(machine.Symboles[0].Nom == "Shrine::Mémoire::Évaluer",
               "nom Unicode incorrect");
    }

    void TesterObjetCoff()
    {
        const auto machine = Compiler(R"(
            publique entier32 Calculer(entier32 valeur) { retourner valeur + 1; }
            publique entier32 Principal() { retourner Calculer(41); }
        )");
        const auto objet = GsPP::EcrivainCoff().Construire(machine);

        Exiger(objet.size() > 60, "objet COFF trop petit");
        Exiger(objet[0] == 0x64 && objet[1] == 0x86,
               "signature machine AMD64 absente");
        Exiger(objet[2] == 1 && objet[3] == 0,
               "nombre de sections COFF incorrect");
        Exiger(objet[20] == '.' && objet[21] == 't',
               "section .text absente");
        Exiger(objet[52] == 1 && objet[53] == 0,
               "relocalisation COFF absente");
    }

    void TesterStructuresEtPointeurs()
    {
        auto programme = Analyser(R"(
            espace Shrine::Test
            {
                structure Point
                {
                    entier32 X;
                    entier32 Y;
                };

                publique entier32 Principal()
                {
                    Point point;
                    point.X = 20;
                    Point* pointeur = &point;
                    pointeur->Y = 22;
                    retourner pointeur->X + point.Y;
                }
            }
        )");

        Exiger(programme.Structures.size() == 1, "structure Point absente");
        Exiger(programme.Structures[0].Taille == 8, "taille de Point incorrecte");
        Exiger(programme.Structures[0].Alignement == 4, "alignement de Point incorrect");
        Exiger(programme.Structures[0].Champs[1].Decalage == 4, "décalage de Y incorrect");
        const auto machine = GsPP::GenerateurX64().Generer(programme);
        Exiger(!machine.Texte.empty(), "code structure/pointeur absent");
    }

    void TesterValeursStructures()
    {
        auto programme = Analyser(R"(
            structure Petit
            {
                entier32 X;
                entier32 Y;
            };

            structure Paquet
            {
                Petit Point;
                entier32 Valeurs[2];
                pointeur_fonction<entier32(entier32)> Operation;
            };

            publique entier32 Doubler(entier32 valeur)
            {
                retourner valeur * 2;
            }

            publique Paquet Global = {{1, 2}, {3}, &Doubler};

            publique Petit Construire(entier32 x, entier32 y)
            {
                retourner {x, y};
            }

            publique Petit Decaler(Petit valeur, entier32 delta)
            {
                valeur.X = valeur.X + delta;
                retourner valeur;
            }

            publique entier32 Principal()
            {
                Petit point = {20, 22};
                Petit copie = point;
                copie = Construire(copie.X, copie.Y);
                retourner Decaler(copie, 1).X;
            }
        )");

        Exiger(programme.Structures.size() == 2,
               "structures de valeurs absentes");
        Exiger(programme.Structures[0].Taille == 8,
               "taille de la petite structure incorrecte");
        Exiger(programme.Structures[1].Taille == 24,
               "taille de la structure agrégée incorrecte");
        Exiger(programme.VariablesGlobales.size() == 1
                   && programme.VariablesGlobales[0].DonneesInitiales.size() == 24,
               "initialiseur global agrégé absent");

        const auto& donnees = programme.VariablesGlobales[0].DonneesInitiales;
        Exiger(donnees[0] == 1 && donnees[4] == 2 && donnees[8] == 3
                   && donnees[12] == 0,
               "aplatissement ou mise à zéro de l’agrégat global incorrect");
        Exiger(programme.VariablesGlobales[0].RelocalisationsInitialiseur.size() == 1
                   && programme.VariablesGlobales[0]
                          .RelocalisationsInitialiseur[0].Decalage == 16
                   && programme.VariablesGlobales[0]
                          .RelocalisationsInitialiseur[0].Symbole == "Doubler",
               "relocalisation de callback dans un agrégat global incorrecte");

        const auto machine = GsPP::GenerateurX64().Generer(programme);
        Exiger(std::all_of(
                   machine.Symboles.begin(), machine.Symboles.end(),
                   [](const GsPP::SymboleMachine& symbole)
                   { return symbole.SignatureAbi.starts_with("GsAbi:x64-ms-v1:"); }),
               "une valeur structurée n’utilise pas l’ABI canonique x64-ms-v1");
        Exiger(std::any_of(
                   machine.Relocalisations.begin(), machine.Relocalisations.end(),
                   [](const GsPP::CodeMachine::Relocalisation& relocalisation)
                   {
                       return relocalisation.Section == GsPP::SectionMachine::Donnees
                           && relocalisation.Decalage == 16
                           && relocalisation.Symbole == "Doubler";
                   }),
               "relocalisation machine de l’agrégat global absente");

        const auto objet = GsPP::EcrivainGsO().Construire(machine);
        Exiger(objet.size() >= GsPP::TailleEnteteGsO
                   && objet[14] == GsPP::VersionAbiGsO && objet[15] == 0,
               "l’objet structuré n’annonce pas l’ABI GsObj v1");
        const auto relu = GsPP::LecteurGsO().Lire(objet);
        Exiger(relu.Donnees == machine.Donnees
                   && relu.Relocalisations.size() == machine.Relocalisations.size(),
               "une valeur structurée est altérée par le trajet GsObj");

        const auto francais = Compiler(R"(
            structure Point { entier32 X; entier32 Y; };
            publique Point Fabriquer(entier32 x, entier32 y)
            { retourner {x, y}; }
            publique entier32 Principal()
            { Point p = Fabriquer(20, 22); retourner p.X + p.Y; }
        )");
        const auto anglais = Compiler(R"(
            struct Point { int32 X; int32 Y; };
            public Point Fabriquer(int32 x, int32 y)
            { return {x, y}; }
            public int32 Principal()
            { Point p = Fabriquer(20, 22); return p.X + p.Y; }
        )");
        Exiger(francais.Texte == anglais.Texte,
               "les syntaxes bilingues des valeurs structurées divergent");

        auto doitEchouer = [](const std::string& source, std::string_view fragment)
        {
            try { (void)Analyser(source); }
            catch (const std::exception& erreur)
            {
                Exiger(std::string(erreur.what()).find(fragment) != std::string::npos,
                       "diagnostic de valeur structurée inattendu : "
                           + std::string(erreur.what()));
                return;
            }
            throw std::runtime_error(
                "une valeur structurée invalide a été acceptée");
        };
        doitEchouer(R"(
            structure Point { entier32 X; entier32 Y; };
            publique entier32 Principal()
            { Point p = {1, 2, 3}; retourner p.X; }
        )", "trop d’éléments");
        doitEchouer(R"(
            structure A { entier32 X; };
            structure B { entier32 X; };
            publique entier32 Principal()
            { A a = {1}; B b = a; retourner b.X; }
        )", "incompatible");
        doitEchouer(R"(
            structure Point { entier32 X; };
            publique Point Fabriquer(
                entier32 a, entier32 b, entier32 c, entier32 d)
            { retourner {a}; }
            publique entier32 Principal() { retourner 0; }
        )", "au maximum trois paramètres");
        doitEchouer(R"(
            union Valeur { entier32 A; entier32 B; };
            publique entier32 Principal()
            { Valeur valeur = {1, 2}; retourner valeur.A; }
        )", "trop d’éléments");
    }

    void TesterIndexationPointeurs()
    {
        const auto machine = Compiler(R"(
            publique entier32 LireEtModifier(entier32* valeurs)
            {
                valeurs[1] = valeurs[0] + 2;
                retourner valeurs[1];
            }
        )");

        Exiger(!machine.Texte.empty(), "code d’indexation de pointeur absent");
        constexpr std::array<std::uint8_t, 7> multiplication{
            0x48, 0x69, 0xC0, 0x04, 0x00, 0x00, 0x00};
        Exiger(std::search(machine.Texte.begin(), machine.Texte.end(),
                          multiplication.begin(), multiplication.end()) != machine.Texte.end(),
               "mise à l’échelle entier32 de l’index absente");
    }

    void TesterPointeursFonction()
    {
        const std::string francais = R"(
            publique entier32 Doubler(entier32 valeur) { retourner valeur * 2; }
            publique entier32 Principal()
            {
                pointeur_fonction<entier32(entier32)> operation = &Doubler;
                retourner operation(21);
            }
        )";
        const std::string anglais = R"(
            public int32 Doubler(int32 valeur) { return valeur * 2; }
            public int32 Principal()
            {
                function_pointer<int32(int32)> operation = &Doubler;
                return operation(21);
            }
        )";
        const auto machineFrancaise = Compiler(francais);
        const auto machineAnglaise = Compiler(anglais);
        Exiger(machineFrancaise.Texte == machineAnglaise.Texte,
               "les syntaxes bilingues de pointeur de fonction divergent");
        Exiger(machineFrancaise.Symboles.size() == machineAnglaise.Symboles.size(),
               "les symboles bilingues de pointeur de fonction divergent");
        for (std::size_t index = 0; index < machineFrancaise.Symboles.size(); ++index)
            Exiger(machineFrancaise.Symboles[index].SignatureAbi
                       == machineAnglaise.Symboles[index].SignatureAbi,
                   "les signatures ABI bilingues de pointeur de fonction divergent");

        constexpr std::array<std::uint8_t, 3> appelIndirect{0x41, 0xFF, 0xD3};
        Exiger(std::search(
                   machineFrancaise.Texte.begin(), machineFrancaise.Texte.end(),
                   appelIndirect.begin(), appelIndirect.end())
                   != machineFrancaise.Texte.end(),
               "instruction call r11 absente pour l’appel indirect");
        Exiger(std::any_of(
                   machineFrancaise.Symboles.begin(), machineFrancaise.Symboles.end(),
                   [](const GsPP::SymboleMachine& symbole)
                   {
                       return symbole.SignatureAbi.find("F(") != std::string::npos;
                   }),
               "signature ABI du pointeur de fonction absente");

        const auto globale = Compiler(R"(
            publique entier32 Doubler(entier32 valeur) { retourner valeur * 2; }
            publique pointeur_fonction<entier32(entier32)> Operation = &Doubler;
            publique entier32 Principal() { retourner Operation(21); }
        )");
        Exiger(std::any_of(
                   globale.Relocalisations.begin(), globale.Relocalisations.end(),
                   [](const GsPP::CodeMachine::Relocalisation& relocalisation)
                   {
                       return relocalisation.Section == GsPP::SectionMachine::Donnees
                           && relocalisation.Type
                               == GsPP::TypeRelocalisationMachine::Adresse64
                           && relocalisation.Symbole == "Doubler";
                   }),
               "relocalisation Adresse64 de la globale callback absente");

        const auto imageGsE = GsPP::EcrivainGsE().Construire(globale, "Principal");
        Exiger(GsPP::VerificateurGsE().Verifier(imageGsE).Valide,
               "l’image GsE avec callback globale est invalide");
        auto lire16 = [&](std::size_t position)
        {
            return static_cast<std::uint16_t>(imageGsE[position]
                | (static_cast<std::uint16_t>(imageGsE[position + 1]) << 8));
        };
        auto lire32 = [&](std::size_t position)
        {
            std::uint32_t valeur = 0;
            for (int index = 0; index < 4; ++index)
                valeur |= static_cast<std::uint32_t>(imageGsE[position + index])
                    << (index * 8);
            return valeur;
        };
        auto lire64 = [&](std::size_t position)
        {
            std::uint64_t valeur = 0;
            for (int index = 0; index < 8; ++index)
                valeur |= static_cast<std::uint64_t>(imageGsE[position + index])
                    << (index * 8);
            return valeur;
        };
        const auto nombreSections = lire32(28);
        const auto tableSections = lire64(64);
        std::size_t entreeBase64 = 0;
        for (std::uint32_t index = 0; index < nombreSections; ++index)
        {
            const auto section = static_cast<std::size_t>(tableSections + index * 64ULL);
            if (lire32(section + 16) == GsPP::TypeSectionRelocalisationsGsE)
                entreeBase64 = static_cast<std::size_t>(lire64(section + 24));
        }
        Exiger(entreeBase64 != 0,
               "table de relocalisations de la callback globale absente");
        Exiger(lire32(entreeBase64 + 8)
                   == GsPP::IndiceImportRelocalisationBaseGsE
                   && lire16(entreeBase64 + 12)
                       == GsPP::TypeRelocalisationBase64GsE,
               "relocalisation BASE64 de la callback globale incorrecte");

        constexpr std::uint64_t baseChargement = 0x20000000;
        const auto imageChargee = GsPP::ChargeurGsE().Charger(
            imageGsE, baseChargement);
        const auto adresseOperation = imageChargee.ChercherExport("Operation");
        const auto adresseDoubler = imageChargee.ChercherExport("Doubler");
        Exiger(adresseOperation.has_value() && adresseDoubler.has_value(),
               "exports de la callback globale absents");
        const auto rvaOperation = static_cast<std::size_t>(
            *adresseOperation - baseChargement);
        std::uint64_t callbackChargee = 0;
        for (int index = 0; index < 8; ++index)
            callbackChargee |= static_cast<std::uint64_t>(
                imageChargee.Memoire[rvaOperation + index]) << (index * 8);
        Exiger(callbackChargee == *adresseDoubler,
               "la relocalisation BASE64 ne contient pas l’adresse chargée");

        auto base64Corrompue = imageGsE;
        base64Corrompue[entreeBase64 + 8] = 0;
        Exiger(!GsPP::VerificateurGsE().Verifier(base64Corrompue).Valide,
               "un indice d’import BASE64 falsifié a été accepté");
        base64Corrompue = imageGsE;
        for (int index = 0; index < 8; ++index)
            base64Corrompue[entreeBase64 + 16 + index] = 0xFF;
        Exiger(!GsPP::VerificateurGsE().Verifier(base64Corrompue).Valide,
               "une cible BASE64 hors image a été acceptée");
        base64Corrompue = imageGsE;
        base64Corrompue[entreeBase64 + 12] = 0x7F;
        Exiger(!GsPP::VerificateurGsE().Verifier(base64Corrompue).Valide,
               "un type de relocalisation GsE inconnu a été accepté");

        const auto fournisseur = GsPP::LecteurGsO().Lire(
            GsPP::EcrivainGsO().Construire(Compiler(R"(
                publique entier32 Tripler(entier32 valeur)
                {
                    retourner valeur * 3;
                }
            )")));
        const auto utilisateur = GsPP::LecteurGsO().Lire(
            GsPP::EcrivainGsO().Construire(Compiler(R"(
                externe entier32 Tripler(entier32 valeur);
                publique pointeur_fonction<entier32(entier32)> OperationLiee = &Tripler;
                publique entier32 Principal() { retourner OperationLiee(14); }
            )")));
        const auto liee = GsPP::EditeurLiens().Lier({
            {"Fournisseur.GsObj", fournisseur},
            {"Utilisateur.GsObj", utilisateur}}, {}, "Principal");
        const auto imageLiee = GsPP::ChargeurGsE().Charger(
            GsPP::EcrivainGsE().Construire(liee, "Principal"),
            baseChargement);
        const auto operationLiee = imageLiee.ChercherExport("OperationLiee");
        const auto triplerLie = imageLiee.ChercherExport("Tripler");
        Exiger(operationLiee.has_value() && triplerLie.has_value(),
               "callback globale liée entre GsObj absente");
        std::uint64_t adresseLiee = 0;
        const auto rvaLie = static_cast<std::size_t>(
            *operationLiee - baseChargement);
        for (int index = 0; index < 8; ++index)
            adresseLiee |= static_cast<std::uint64_t>(
                imageLiee.Memoire[rvaLie + index]) << (index * 8);
        Exiger(adresseLiee == *triplerLie,
               "callback globale GsObj non relocalisée après la liaison");

        auto doitEchouer = [](const std::string& source, std::string_view fragment)
        {
            try { (void)Analyser(source); }
            catch (const std::exception& erreur)
            {
                Exiger(std::string(erreur.what()).find(fragment) != std::string::npos,
                       "diagnostic de pointeur de fonction inattendu : "
                           + std::string(erreur.what()));
                return;
            }
            throw std::runtime_error("un pointeur de fonction invalide a été accepté");
        };
        doitEchouer(R"(
            publique entier32 Principal()
            {
                entier32 valeur = 1;
                retourner valeur();
            }
        )", "cible d’appel");
        doitEchouer(R"(
            publique booléen Tester(booléen valeur) { retourner valeur; }
            publique entier32 Principal()
            {
                pointeur_fonction<entier32(entier32)> operation = &Tester;
                retourner 0;
            }
        )", "type d’initialisation");

        const auto definition = Compiler(R"(
            publique entier32 Appliquer(
                pointeur_fonction<entier32(entier32)> rappel,
                entier32 valeur)
            {
                retourner rappel(valeur);
            }
        )");
        const auto consommateur = Compiler(R"(
            externe entier32 Appliquer(
                pointeur_fonction<naturel32(entier32)> rappel,
                entier32 valeur);
            naturel32 Produire(entier32 valeur) { retourner convertir<naturel32>(valeur); }
            publique entier32 Principal() { retourner Appliquer(&Produire, 42); }
        )");
        const auto definitionRelue = GsPP::LecteurGsO().Lire(
            GsPP::EcrivainGsO().Construire(definition));
        const auto consommateurRelu = GsPP::LecteurGsO().Lire(
            GsPP::EcrivainGsO().Construire(consommateur));
        bool abiRefusee = false;
        try
        {
            (void)GsPP::EditeurLiens().Lier({
                {"Definition.GsObj", definitionRelue},
                {"Consommateur.GsObj", consommateurRelu}}, {}, "Principal");
        }
        catch (const std::runtime_error& erreur)
        {
            abiRefusee = std::string(erreur.what()).find("incompatibilité ABI")
                != std::string::npos;
        }
        Exiger(abiRefusee,
               "une signature de callback incompatible a été liée entre GsObj");
    }

    void TesterGsE()
    {
        const auto machine = Compiler(R"(
            publique entier32 Calculer(entier32 valeur) { retourner valeur + 1; }
            publique entier32 Principal() { retourner Calculer(41); }
        )");
        const auto gse = GsPP::EcrivainGsE().Construire(machine, "Principal");
        Exiger(gse.size() > 176, "GsE trop petit");
        Exiger(gse[0] == 'G' && gse[1] == 'S' && gse[2] == 'E'
                   && gse[3] == ':' && gse[4] == '0'
                   && gse[5] == 0 && gse[6] == 0 && gse[7] == 0,
               "signature GsE incorrecte");
        Exiger(gse[8] == GsPP::VersionMajeureGsE
                   && gse[10] == GsPP::VersionMineureGsE,
               "version GsE incorrecte");
        Exiger(gse[12] == 0x70 && gse[13] == 0,
               "taille d’en-tête GsE incorrecte");
        Exiger(gse[20] == GsPP::VersionAbiGsE && gse[21] == 0,
               "ABI d’en-tête GsE incorrecte");
        Exiger(gse[112] == 1 && gse[116] == 5, "segment exécutable GsE incorrect");

        auto lire64 = [&](std::size_t position)
        {
            std::uint64_t valeur = 0;
            for (int index = 0; index < 8; ++index)
                valeur |= static_cast<std::uint64_t>(gse[position + index]) << (index * 8);
            return valeur;
        };
        const auto tableSections = lire64(64);
        const auto fichierTexte = lire64(static_cast<std::size_t>(tableSections + 24));
        const auto& relocalisation = machine.Relocalisations[0];
        const std::size_t position = static_cast<std::size_t>(fichierTexte + relocalisation.Decalage);
        const auto valeur = static_cast<std::int32_t>(
            static_cast<std::uint32_t>(gse[position])
            | (static_cast<std::uint32_t>(gse[position + 1]) << 8)
            | (static_cast<std::uint32_t>(gse[position + 2]) << 16)
            | (static_cast<std::uint32_t>(gse[position + 3]) << 24));
        const auto attendu = static_cast<std::int32_t>(
            machine.Symboles[0].Decalage - (relocalisation.Decalage + 4));
        Exiger(valeur == attendu, "relocalisation interne GsE incorrecte");

        const auto rapport = GsPP::VerificateurGsE().Verifier(gse);
        Exiger(rapport.Valide, "le vérificateur rejette un GsE valide");
        auto corrompu = gse;
        corrompu[0] = 'X';
        Exiger(!GsPP::VerificateurGsE().Verifier(corrompu).Valide,
               "le vérificateur accepte une signature corrompue");
        corrompu = gse;
        corrompu[8] = 2;
        Exiger(!GsPP::VerificateurGsE().Verifier(corrompu).Valide,
               "le vérificateur accepte encore un GsE portant la version locale 2.0");
        corrompu = gse;
        corrompu[20] = 2;
        Exiger(!GsPP::VerificateurGsE().Verifier(corrompu).Valide,
               "le vérificateur accepte un en-tête GsE ABI 2");
        corrompu = gse;
        corrompu[5] = 1;
        Exiger(!GsPP::VerificateurGsE().Verifier(corrompu).Valide,
               "le vérificateur accepte un octet réservé de signature GsE non nul");
        corrompu = gse;
        corrompu[88] = 1;
        Exiger(!GsPP::VerificateurGsE().Verifier(corrompu).Valide,
               "le vérificateur accepte la réserve finale GsE non nulle");
        corrompu = gse;
        corrompu[116] = 6; // écriture + exécution
        Exiger(!GsPP::VerificateurGsE().Verifier(corrompu).Valide,
               "le vérificateur accepte un segment W+X");
        corrompu = gse;
        for (std::size_t index = 48; index < 56; ++index) corrompu[index] = 0;
        Exiger(!GsPP::VerificateurGsE().Verifier(corrompu).Valide,
               "le vérificateur accepte une taille d’image nulle");
        for (std::size_t taille = 0; taille < gse.size(); ++taille)
        {
            const std::vector<std::uint8_t> tronque(gse.begin(), gse.begin() + taille);
            Exiger(!GsPP::VerificateurGsE().Verifier(tronque).Valide,
                   "le vérificateur accepte une image GsE tronquée");
        }
    }

    void TesterGlobalesImportsEtExports()
    {
        auto programme = Analyser(R"(
            espace Shrine::Test
            {
                structure Etat { entier32 A; entier32 B; };
                publique entier32 CompteurPublicAvecUnNomSuperieurAQuaranteOctets = 40 + 2;
                Etat Memoire;
                externe entier32 AfficherCetteValeurDansLeJournalDuSystemeSanctuaire(entier32 valeur);

                publique entier32 Principal()
                {
                    CompteurPublicAvecUnNomSuperieurAQuaranteOctets =
                        CompteurPublicAvecUnNomSuperieurAQuaranteOctets + 1;
                    retourner AfficherCetteValeurDansLeJournalDuSystemeSanctuaire(
                        CompteurPublicAvecUnNomSuperieurAQuaranteOctets);
                }
            }
        )");
        const auto machine = GsPP::GenerateurX64().Generer(programme);
        Exiger(machine.Donnees.size() == 4, "section de données globales incorrecte");
        Exiger(machine.TailleZero == 8, "section zéro globale incorrecte");
        Exiger(machine.Relocalisations.size() >= 3, "relocalisations globales/import absentes");

        GsPP::MetadonneesGsE meta;
        meta.Nom = "Test Global";
        meta.Version = "0.10.2";
        const auto gse = GsPP::EcrivainGsE().Construire(
            machine, "Shrine::Test::Principal", meta);
        const auto rapport = GsPP::VerificateurGsE().Verifier(gse);
        Exiger(rapport.Valide, "GsE avec import/export invalide");
        const std::string contenu(gse.begin(), gse.end());
        Exiger(contenu.find("Test Global") != std::string::npos, "métadonnée Nom absente");
        Exiger(contenu.find(
            "Shrine::Test::AfficherCetteValeurDansLeJournalDuSystemeSanctuaire")
                != std::string::npos,
            "import long absent");
        Exiger(contenu.find(
            "Shrine::Test::CompteurPublicAvecUnNomSuperieurAQuaranteOctets")
                != std::string::npos,
            "export global long absent");

        auto lire32 = [&](std::size_t position)
        {
            std::uint32_t valeur = 0;
            for (int index = 0; index < 4; ++index)
                valeur |= static_cast<std::uint32_t>(gse[position + index])
                    << (index * 8);
            return valeur;
        };
        auto lire64 = [&](std::size_t position)
        {
            std::uint64_t valeur = 0;
            for (int index = 0; index < 8; ++index)
                valeur |= static_cast<std::uint64_t>(gse[position + index])
                    << (index * 8);
            return valeur;
        };
        const auto nombreSections = lire32(28);
        const auto tableSections = lire64(64);
        std::size_t fichierImports = 0;
        for (std::uint32_t index = 0; index < nombreSections; ++index)
        {
            const auto section = static_cast<std::size_t>(
                tableSections + index * 64ULL);
            if (lire32(section + 16) == GsPP::TypeSectionImportsGsE)
                fichierImports = static_cast<std::size_t>(lire64(section + 24));
        }
        Exiger(fichierImports != 0
                   && gse[fichierImports + 8] == GsPP::VersionAbiGsE
                   && gse[fichierImports + 9] == 0,
               "un import GsE n’annonce pas l’ABI 1");
        auto abiAncienne = gse;
        abiAncienne[fichierImports + 8] = 2;
        Exiger(!GsPP::VerificateurGsE().Verifier(abiAncienne).Valide,
               "le vérificateur accepte encore un import GsE ABI 2");
    }

    void TesterNomsSymbolesGsELongs()
    {
        const std::string nomMaximal(GsPP::TailleNomSymboleGsEMaximale, 'N');
        GsPP::CodeMachine machine;
        machine.Texte = {0xC3};
        machine.Symboles.push_back({
            nomMaximal, 0, 1, true, GsPP::SectionMachine::Texte, true,
            GsPP::GenreSymboleMachine::Fonction, "GsAbi:x64-ms-v1:F()->T2P0", {}});

        const auto gse = GsPP::EcrivainGsE().Construire(machine, nomMaximal);
        const auto rapport = GsPP::VerificateurGsE().Verifier(gse);
        Exiger(rapport.Valide, "un nom public GsE de 1024 octets a été rejeté");
        const auto image = GsPP::ChargeurGsE().Charger(gse, 0x10000000);
        Exiger(image.ChercherExport(nomMaximal).has_value(),
               "le chargeur ne retrouve pas le nom public GsE maximal");

        auto lire32 = [&](std::size_t position)
        {
            std::uint32_t valeur = 0;
            for (int index = 0; index < 4; ++index)
                valeur |= static_cast<std::uint32_t>(gse[position + index]) << (index * 8);
            return valeur;
        };
        auto lire64 = [&](std::size_t position)
        {
            std::uint64_t valeur = 0;
            for (int index = 0; index < 8; ++index)
                valeur |= static_cast<std::uint64_t>(gse[position + index]) << (index * 8);
            return valeur;
        };
        const auto nombreSections = lire32(28);
        const auto tableSections = lire64(64);
        std::uint64_t fichierExports = 0;
        std::uint64_t fichierChaines = 0;
        for (std::uint32_t index = 0; index < nombreSections; ++index)
        {
            const auto section = static_cast<std::size_t>(tableSections + index * 64ULL);
            const auto type = lire32(section + 16);
            if (type == GsPP::TypeSectionExportsGsE) fichierExports = lire64(section + 24);
            if (type == GsPP::TypeSectionChainesGsE) fichierChaines = lire64(section + 24);
        }
        Exiger(fichierExports != 0 && fichierChaines != 0,
               "sections de symboles GsE 1.0 absentes");

        auto corrompu = gse;
        corrompu[static_cast<std::size_t>(fichierExports + 4)] = 0;
        corrompu[static_cast<std::size_t>(fichierExports + 5)] = 0;
        Exiger(!GsPP::VerificateurGsE().Verifier(corrompu).Valide,
               "le vérificateur accepte une longueur de nom incohérente");
        corrompu = gse;
        corrompu[static_cast<std::size_t>(fichierChaines)] = 0xC0;
        Exiger(!GsPP::VerificateurGsE().Verifier(corrompu).Valide,
               "le vérificateur accepte un nom de symbole UTF-8 invalide");
        corrompu = gse;
        corrompu[static_cast<std::size_t>(fichierChaines + nomMaximal.size())] = 'X';
        Exiger(!GsPP::VerificateurGsE().Verifier(corrompu).Valide,
               "le vérificateur accepte une chaîne sans terminaison nulle");

        machine.Symboles[0].Nom.push_back('X');
        bool nomTropLongRefuse = false;
        try { (void)GsPP::EcrivainGsE().Construire(machine, machine.Symboles[0].Nom); }
        catch (const std::runtime_error& erreur)
        {
            nomTropLongRefuse = std::string(erreur.what()).find("1024")
                != std::string::npos;
        }
        Exiger(nomTropLongRefuse,
               "un nom public GsE supérieur à 1024 octets a été accepté");

        machine.Symboles[0].Nom = std::string(1, static_cast<char>(0xC0));
        bool utf8InvalideRefuse = false;
        try { (void)GsPP::EcrivainGsE().Construire(machine, machine.Symboles[0].Nom); }
        catch (const std::runtime_error& erreur)
        {
            utf8InvalideRefuse = std::string(erreur.what()).find("UTF-8")
                != std::string::npos;
        }
        Exiger(utf8InvalideRefuse,
               "l’écrivain accepte un nom public en UTF-8 invalide");
    }

    void TesterChargeurGsE()
    {
        const auto machine = Compiler(R"(
            publique entier32 CompteurDuChargeurAvecUnNomPublicSuperieurAQuaranteOctets = 7;
            entier32 ZoneZero;
            externe entier32 AfficherUneValeurDepuisLeChargeurExperimentalDeSanctuaire(entier32 valeur);
            publique entier32 Principal()
            {
                CompteurDuChargeurAvecUnNomPublicSuperieurAQuaranteOctets =
                    CompteurDuChargeurAvecUnNomPublicSuperieurAQuaranteOctets + 1;
                retourner AfficherUneValeurDepuisLeChargeurExperimentalDeSanctuaire(
                    CompteurDuChargeurAvecUnNomPublicSuperieurAQuaranteOctets);
            }
        )");
        const auto gse = GsPP::EcrivainGsE().Construire(machine, "Principal");
        constexpr std::uint64_t base = 0x10000000;
        constexpr std::uint64_t adresseAfficher = 0x10010000;
        const auto image = GsPP::ChargeurGsE().Charger(
            gse, base,
            [](std::string_view nom) -> std::optional<std::uint64_t>
            {
                if (nom == "AfficherUneValeurDepuisLeChargeurExperimentalDeSanctuaire")
                    return adresseAfficher;
                return std::nullopt;
            });

        Exiger(image.AdressePointEntree == base, "adresse du point d’entrée chargé incorrecte");
        Exiger(image.Segments.size() == 3, "les trois segments GsE ne sont pas chargés");
        Exiger(image.Imports.size() == 1 && image.Imports[0].Resolu,
               "l’import obligatoire n’est pas résolu");
        Exiger(image.ChercherExport(
            "CompteurDuChargeurAvecUnNomPublicSuperieurAQuaranteOctets").has_value(),
            "export global long chargé absent");
        Exiger(image.Memoire[static_cast<std::size_t>(image.Segments[1].Rva)] == 7,
               "donnée globale initialisée incorrecte après chargement");
        Exiger(image.Memoire[static_cast<std::size_t>(image.Segments[2].Rva)] == 0,
               "zone zéro non initialisée à zéro");

        const auto relocalisation = std::find_if(
            machine.Relocalisations.begin(), machine.Relocalisations.end(),
            [](const GsPP::CodeMachine::Relocalisation& valeur)
            {
                return valeur.Symbole
                    == "AfficherUneValeurDepuisLeChargeurExperimentalDeSanctuaire";
            });
        Exiger(relocalisation != machine.Relocalisations.end(), "relocalisation d’import absente");
        const auto position = static_cast<std::size_t>(relocalisation->Decalage);
        const auto relatif = static_cast<std::int32_t>(
            static_cast<std::uint32_t>(image.Memoire[position])
            | (static_cast<std::uint32_t>(image.Memoire[position + 1]) << 8)
            | (static_cast<std::uint32_t>(image.Memoire[position + 2]) << 16)
            | (static_cast<std::uint32_t>(image.Memoire[position + 3]) << 24));
        const auto attendu = static_cast<std::int32_t>(
            adresseAfficher - (base + relocalisation->Decalage + 4));
        Exiger(relatif == attendu, "relocalisation REL32 d’import incorrecte");

        bool importRefuse = false;
        try { (void)GsPP::ChargeurGsE().Charger(gse, base); }
        catch (const std::runtime_error& erreur)
        { importRefuse = std::string(erreur.what()).find("non résolu") != std::string::npos; }
        Exiger(importRefuse, "un import obligatoire non résolu a été accepté");
    }

    void TesterObjetNatifGsO()
    {
        const auto machine = Compiler(R"(
            publique entier32 Compteur = 7;
            externe entier32 Service(entier32 valeur);
            publique entier32 Principal()
            {
                retourner Service(Compteur);
            }
        )");
        const auto objet = GsPP::EcrivainGsO().Construire(machine);
        Exiger(objet.size() >= GsPP::TailleEnteteGsO,
               "objet GsO trop court");
        Exiger(objet[0] == 'G' && objet[1] == 'S'
                   && objet[2] == 'O' && objet[3] == 'B'
                   && objet[4] == 'J' && objet[5] == ':'
                   && objet[6] == '0',
                "signature GsObj absente");
        Exiger(objet[7] == 0,
                "octet réservé GsObj non nul");
        Exiger(objet[8] == 1 && objet[9] == 0
                   && objet[10] == 0 && objet[11] == 0,
               "format GsObj différent de 1.0");

        auto corrompu = objet;
        corrompu[5] = 0;
        corrompu[6] = 0;
        bool ancienneSignatureRefusee = false;
        try { (void)GsPP::LecteurGsO().Lire(corrompu); }
        catch (const std::runtime_error& erreur)
        { ancienneSignatureRefusee = std::string(erreur.what()).find("ancien format") != std::string::npos; }
        Exiger(ancienneSignatureRefusee,
                "l’ancienne signature locale GSOBJ\\0 a été acceptée");

        corrompu = objet;
        corrompu[2] = 'O';
        corrompu[3] = ':';
        corrompu[4] = '0';
        corrompu[5] = 0;
        corrompu[6] = 0;
        bool cibleIntermediaireRefusee = false;
        try { (void)GsPP::LecteurGsO().Lire(corrompu); }
        catch (const std::runtime_error& erreur)
        { cibleIntermediaireRefusee = std::string(erreur.what()).find("signature") != std::string::npos; }
        Exiger(cibleIntermediaireRefusee,
               "la cible intermédiaire GSO:0 a été acceptée");

        const auto relu = GsPP::LecteurGsO().Lire(objet);
        Exiger(relu.Texte == machine.Texte, "texte GsO modifié par la sérialisation");
        Exiger(relu.Donnees == machine.Donnees, "données GsO modifiées");
        Exiger(relu.TailleZero == machine.TailleZero, "taille zéro GsO modifiée");
        Exiger(relu.Symboles.size() == machine.Symboles.size(),
               "nombre de symboles GsO modifié");
        Exiger(relu.Relocalisations.size() == machine.Relocalisations.size(),
               "nombre de relocalisations GsO modifié");
        for (std::size_t index = 0; index < relu.Symboles.size(); ++index)
        {
            Exiger(relu.Symboles[index].Nom == machine.Symboles[index].Nom,
                   "nom de symbole GsO modifié");
            Exiger(relu.Symboles[index].SignatureAbi
                       == machine.Symboles[index].SignatureAbi,
                   "signature ABI GsO modifiée");
        }

        corrompu = objet;
        corrompu[8] = 99;
        bool versionRefusee = false;
        try { (void)GsPP::LecteurGsO().Lire(corrompu); }
        catch (const std::runtime_error& erreur)
        { versionRefusee = std::string(erreur.what()).find("version") != std::string::npos; }
        Exiger(versionRefusee, "une version GsO inconnue a été acceptée");

        corrompu = objet;
        corrompu[14] = 2;
        bool ancienneAbiRefusee = false;
        try { (void)GsPP::LecteurGsO().Lire(corrompu); }
        catch (const std::runtime_error& erreur)
        { ancienneAbiRefusee = std::string(erreur.what()).find("ABI") != std::string::npos; }
        Exiger(ancienneAbiRefusee,
                "un objet GsObj portant encore l’ABI 2 a été accepté");

        corrompu = objet;
        corrompu[7] = 1;
        bool reserveRefuse = false;
        try { (void)GsPP::LecteurGsO().Lire(corrompu); }
        catch (const std::runtime_error& erreur)
        { reserveRefuse = std::string(erreur.what()).find("réservé") != std::string::npos; }
        Exiger(reserveRefuse, "un octet réservé GsObj non nul a été accepté");

        corrompu = objet;
        const auto symbole = GsPP::TailleEnteteGsO;
        corrompu[symbole + 4] = 0xFF;
        corrompu[symbole + 5] = 0xFF;
        bool chaineRefusee = false;
        try { (void)GsPP::LecteurGsO().Lire(corrompu); }
        catch (const std::runtime_error& erreur)
        { chaineRefusee = std::string(erreur.what()).find("chaîne") != std::string::npos; }
        Exiger(chaineRefusee, "une référence de chaîne GsO falsifiée a été acceptée");
    }

    void TesterModeleObjet018()
    {
        auto programme = Analyser(R"(
            classe Ressource
            {
                privée:
                    entier32 Valeur;
                publique:
                    constructeur(entier32 initiale)
                    {
                        soi.Valeur = initiale;
                    }

                    destructeur()
                    {
                        soi.Valeur = 0;
                    }

                    entier32 Lire()
                    {
                        retourner soi.Valeur;
                    }

                    virtuel entier32 Avancer(entier32 delta)
                    {
                        soi.Valeur = soi.Valeur + delta;
                        retourner soi.Valeur;
                    }

                    entier32 opérateur +(entier32 delta)
                    {
                        retourner soi.Valeur + delta;
                    }
            };

            publique entier32 Choisir(entier32& valeur)
            {
                valeur = valeur + 1;
                retourner valeur;
            }

            publique entier64 Choisir(entier64& valeur)
            {
                valeur = valeur + 2;
                retourner valeur;
            }

            publique entier32 Principal()
            {
                entier32 base = 5;
                entier32& référence = base;
                Ressource ressource(référence);
                retourner ressource.Lire()
                    + ressource.Avancer(1)
                    + (ressource + 2)
                    + Choisir(référence);
            }
        )");

        Exiger(programme.Structures.size() == 1,
               "classe Ressource absente");
        const auto& classe = programme.Structures.front();
        Exiger(classe.EstClasse && classe.EstPolymorphe,
               "métadonnées de classe polymorphe absentes");
        Exiger(classe.Taille == 16 && classe.Alignement == 8
                   && classe.Champs.front().Decalage == 8,
               "disposition de classe virtuelle incorrecte");
        Exiger(classe.Champs.front().Visibilite
                   == GsPP::VisibiliteMembre::Privee,
               "visibilité privée du champ perdue");

        const auto nombreChoisir = std::count_if(
            programme.Fonctions.begin(), programme.Fonctions.end(),
            [](const GsPP::Fonction& fonction)
            { return fonction.NomSource == "Choisir"; });
        Exiger(nombreChoisir == 2,
               "surcharges de fonction absentes");
        Exiger(std::all_of(
                   programme.Fonctions.begin(), programme.Fonctions.end(),
                   [](const GsPP::Fonction& fonction)
                   {
                       return fonction.NomSource != "Choisir"
                           || fonction.NomComplet().find('$') != std::string::npos;
                   }),
               "mangling déterministe des surcharges absent");

        const auto machine = GsPP::GenerateurX64().Generer(programme);
        Exiger(std::any_of(
                   machine.Symboles.begin(), machine.Symboles.end(),
                   [](const GsPP::SymboleMachine& symbole)
                   { return symbole.Nom == "@GsVTable::Ressource"; }),
               "table virtuelle de Ressource absente");
        Exiger(std::any_of(
                   machine.Relocalisations.begin(), machine.Relocalisations.end(),
                   [](const GsPP::CodeMachine::Relocalisation& relocalisation)
                   {
                       return relocalisation.Section
                               == GsPP::SectionMachine::Donnees
                           && relocalisation.Type
                               == GsPP::TypeRelocalisationMachine::Adresse64
                           && relocalisation.Symbole.find("Avancer")
                               != std::string::npos;
                   }),
               "entrée de méthode virtuelle absente de la table");
        Exiger(std::any_of(
                   machine.Relocalisations.begin(), machine.Relocalisations.end(),
                   [](const GsPP::CodeMachine::Relocalisation& relocalisation)
                   { return relocalisation.Symbole.find("$destructeur")
                            != std::string::npos; }),
               "appel RAII du destructeur absent");
        Exiger(std::all_of(
                   machine.Symboles.begin(), machine.Symboles.end(),
                   [](const GsPP::SymboleMachine& symbole)
                   { return symbole.SignatureAbi.starts_with("GsAbi:x64-ms-v1:"); }),
               "le modèle objet quitte l’ABI canonique 1");

        const auto dispositionVirtuelleA = Compiler(R"(
            classe Polymorphe
            {
                privée:
                    virtuel entier32 Premier() { retourner 1; }
                    virtuel entier32 Second() { retourner 2; }
            };
            publique entier32 UtiliserVirtuel(Polymorphe& valeur)
            { retourner 1; }
        )");
        const auto dispositionVirtuelleB = Compiler(R"(
            classe Polymorphe
            {
                privée:
                    virtuel entier32 Second() { retourner 2; }
                    virtuel entier32 Premier() { retourner 1; }
            };
            externe entier32 UtiliserVirtuel(Polymorphe& valeur);
            publique entier32 Principal()
            {
                Polymorphe valeur;
                retourner UtiliserVirtuel(valeur);
            }
        )");
        bool dispositionVirtuelleRefusee = false;
        try
        {
            (void)GsPP::EditeurLiens().Lier({
                {"VirtuelA.GsObj", dispositionVirtuelleA},
                {"VirtuelB.GsObj", dispositionVirtuelleB}}, {}, "Principal");
        }
        catch (const std::runtime_error& erreur)
        {
            dispositionVirtuelleRefusee = std::string(erreur.what()).find(
                "incompatibilité ABI") != std::string::npos;
        }
        Exiger(dispositionVirtuelleRefusee,
               "un ordre de table virtuelle incompatible a été accepté");

        bool accesPriveRefuse = false;
        try
        {
            (void)Analyser(R"(
                classe Secret
                {
                    privée: entier32 Valeur;
                    publique: constructeur() { soi.Valeur = 1; }
                };
                publique entier32 Principal()
                {
                    Secret secret;
                    retourner secret.Valeur;
                }
            )");
        }
        catch (const GsPP::ErreurCompilation& erreur)
        {
            accesPriveRefuse = std::string(erreur.what()).find(
                "accès interdit") != std::string::npos;
        }
        Exiger(accesPriveRefuse,
               "un champ privé est accessible hors de sa classe");

        const auto francais = Compiler(R"(
            classe Compteur
            {
                privée: entier32 Valeur;
                publique:
                    constructeur(entier32 valeur) { soi.Valeur = valeur; }
                    entier32 Lire() { retourner soi.Valeur; }
            };
            publique entier32 Principal()
            {
                entier32 valeur = 42;
                entier32& référence = valeur;
                Compteur compteur(référence);
                retourner compteur.Lire();
            }
        )");
        const auto anglais = Compiler(R"(
            class Compteur
            {
                private: int32 Valeur;
                public:
                    constructor(int32 valeur) { this.Valeur = valeur; }
                    int32 Lire() { return this.Valeur; }
            };
            public int32 Principal()
            {
                int32 valeur = 42;
                int32& référence = valeur;
                Compteur compteur(référence);
                return compteur.Lire();
            }
        )");
        Exiger(francais.Texte == anglais.Texte
                   && francais.Donnees == anglais.Donnees,
               "les syntaxes objet française et anglaise divergent");
    }

    void TesterHeritage019()
    {
        auto programme = Analyser(R"(
            classe Base
            {
                protégée:
                    entier32 Valeur;
                publique:
                    constructeur() { soi.Valeur = 20; }
                    virtuel entier32 Lire() { retourner soi.Valeur; }
                    virtuel destructeur() { soi.Valeur = 0; }
            };

            classe Derivee : publique Base
            {
                privée:
                    entier32 Bonus;
                publique:
                    constructeur() { soi.Bonus = 2; }
                    remplacer entier32 Lire()
                    { retourner soi.Valeur + soi.Bonus; }
                    remplacer destructeur() { soi.Bonus = 0; }
                    virtuel entier32 Doubler()
                    { retourner soi.Lire() * 2; }
            };

            publique entier32 AppelerBase(Base& valeur)
            { retourner valeur.Lire(); }

            publique entier32 Principal()
            {
                Derivee valeur();
                Base& base = valeur;
                Base* pointeur = &valeur;
                retourner AppelerBase(base) + pointeur->Lire()
                    + valeur.Doubler();
            }
        )");

        const auto& base = programme.Structures[0];
        const auto& derivee = programme.Structures[1];
        Exiger(derivee.ClasseBaseCanonique == "Base",
               "classe de base canonique absente");
        Exiger(base.Taille == 16 && base.Champs[0].Decalage == 8,
               "disposition de la base polymorphe incorrecte");
        Exiger(derivee.Taille == 24 && derivee.Alignement == 8
                   && derivee.Champs[0].Decalage == 16,
               "la classe dérivée ne conserve pas la base au décalage zéro");
        Exiger(derivee.DecalageTableVirtuelle == 0
                   && derivee.SymbolesTableVirtuelle.size() == 3,
               "table virtuelle dérivée incorrecte");
        Exiger(derivee.SymbolesTableVirtuelle[0].find("Derivee::Lire")
                       != std::string::npos
                   && derivee.SymbolesTableVirtuelle[1].find(
                          "Derivee::$destructeur") != std::string::npos
                   && derivee.SymbolesTableVirtuelle[2].find(
                          "Derivee::Doubler") != std::string::npos,
               "les remplacements ne conservent pas leurs emplacements");

        const auto machine = GsPP::GenerateurX64().Generer(programme);
        const auto tableDerivee = std::find_if(
            machine.Symboles.begin(), machine.Symboles.end(),
            [](const GsPP::SymboleMachine& symbole)
            { return symbole.Nom == "@GsVTable::Derivee"; });
        Exiger(tableDerivee != machine.Symboles.end()
                   && tableDerivee->Taille == 24,
               "table virtuelle dérivée absente du code machine");
        Exiger(std::count_if(
                   machine.Relocalisations.begin(),
                   machine.Relocalisations.end(),
                   [](const GsPP::CodeMachine::Relocalisation& relocalisation)
                   {
                       return relocalisation.Section
                               == GsPP::SectionMachine::Donnees
                           && relocalisation.Symbole.starts_with("Derivee::");
                   }) >= 3,
               "les entrées remplacées ne ciblent pas la classe dérivée");

        const auto baseNonPolymorphe = Analyser(R"(
            classe PetiteBase { publique: entier32 Valeur; };
            classe AvecVirtuel : publique PetiteBase
            {
                publique: virtuel entier32 Lire() { retourner 1; }
            };
            publique entier32 Principal()
            { AvecVirtuel valeur; retourner valeur.Lire(); }
        )");
        Exiger(baseNonPolymorphe.Structures[0].Taille == 4
                   && baseNonPolymorphe.Structures[1]
                          .DecalageTableVirtuelle == 8
                   && baseNonPolymorphe.Structures[1].Taille == 16,
               "l’ajout d’une première table virtuelle déplace la sous-base");

        auto doitEchouer = [](std::string source, std::string motif)
        {
            bool refuse = false;
            try { (void)Analyser(source); }
            catch (const GsPP::ErreurCompilation& erreur)
            { refuse = std::string(erreur.what()).find(motif) != std::string::npos; }
            Exiger(refuse, "diagnostic d’héritage attendu absent : " + motif);
        };
        doitEchouer(R"(
            classe B { publique: virtuel entier32 Lire() { retourner 1; } };
            classe D : publique B
            { publique: entier32 Lire() { retourner 2; } };
            publique entier32 Principal() { D valeur; retourner valeur.Lire(); }
        )", "remplacer/override");
        doitEchouer(R"(
            classe B { publique: entier32 Lire() { retourner 1; } };
            classe D : publique B
            { publique: remplacer entier32 Lire() { retourner 2; } };
            publique entier32 Principal() { retourner 0; }
        )", "aucune méthode virtuelle héritée");
        doitEchouer(R"(
            classe B { protégée: entier32 Secret; };
            classe D : publique B { publique: entier32 Lire() { retourner soi.Secret; } };
            publique entier32 Principal()
            { D valeur; retourner valeur.Secret; }
        )", "accès interdit");
        doitEchouer(R"(
            classe B {};
            classe D : privée B {};
            publique entier32 Principal() { retourner 0; }
        )", "uniquement l’héritage public");
        doitEchouer(R"(
            classe A : publique B {};
            classe B : publique A {};
            publique entier32 Principal() { retourner 0; }
        )", "cycle");
        doitEchouer(R"(
            classe B {};
            classe D : publique B {};
            entier32 PrendreValeur(B valeur) { retourner 1; }
            publique entier32 Principal()
            {
                D derivee;
                retourner PrendreValeur(derivee);
            }
        )", "aucune surcharge compatible");
        doitEchouer(R"(
            classe B { privée: constructeur() {} };
            classe D : publique B {};
            publique entier32 Principal()
            { D derivee; retourner 0; }
        )", "constructeur de base inaccessible");
        doitEchouer(R"(
            classe B
            { publique: constructeur(entier32 valeur) {} };
            classe D : publique B {};
            publique entier32 Principal()
            { D derivee; retourner 0; }
        )", "aucune surcharge compatible");

        const auto remplacementA = Compiler(R"(
            classe B
            { privée: virtuel entier32 Lire() { retourner 1; } };
            classe D : publique B
            { privée: remplacer entier32 Lire() { retourner 2; } };
            publique entier32 UtiliserDerivee(D& valeur) { retourner 1; }
        )");
        const auto remplacementB = Compiler(R"(
            classe B
            { privée: virtuel entier32 Lire() { retourner 1; } };
            classe D : publique B {};
            externe entier32 UtiliserDerivee(D& valeur);
            publique entier32 Principal()
            {
                D valeur;
                retourner UtiliserDerivee(valeur);
            }
        )");
        bool remplacementAbiRefuse = false;
        try
        {
            (void)GsPP::EditeurLiens().Lier({
                {"RemplacementA.GsObj", remplacementA},
                {"RemplacementB.GsObj", remplacementB}}, {}, "Principal");
        }
        catch (const std::runtime_error& erreur)
        {
            remplacementAbiRefuse = std::string(erreur.what()).find(
                "incompatibilité ABI") != std::string::npos;
        }
        Exiger(remplacementAbiRefuse,
               "un remplacement virtuel incompatible a été accepté par l’ABI");

        const auto francais = Compiler(R"(
            classe B { publique: virtuel entier32 Lire() { retourner 20; } };
            classe D : publique B
            { publique: remplacer entier32 Lire() { retourner 22; } };
            publique entier32 Principal()
            { D valeur; B& base = valeur; retourner base.Lire(); }
        )");
        const auto anglais = Compiler(R"(
            class B { public: virtual int32 Lire() { return 20; } };
            class D : public B
            { public: override int32 Lire() { return 22; } };
            public int32 Principal()
            { D valeur; B& base = valeur; return base.Lire(); }
        )");
        Exiger(francais.Texte == anglais.Texte
                   && francais.Donnees == anglais.Donnees,
               "les syntaxes d’héritage française et anglaise divergent");
    }

    void TesterInitialisationParent020()
    {
        auto programme = Analyser(R"(
            classe Base
            {
                protégée: entier32 Valeur;
                publique:
                    constructeur(entier32 initiale)
                    { soi.Valeur = initiale; }
                    virtuel entier32 Lire()
                    { retourner soi.Valeur; }
            };

            classe Derivee : publique Base
            {
                privée: entier32 Bonus;
                publique:
                    constructeur(entier32 initiale, entier32 bonus)
                        : parent(initiale)
                    { soi.Bonus = bonus; }
                    remplacer entier32 Lire()
                    { retourner parent.Lire() + soi.Bonus; }
            };

            publique entier32 Principal()
            {
                Derivee valeur(40, 2);
                Base& vue = valeur;
                retourner vue.Lire();
            }
        )");

        const auto constructeurDerive = std::find_if(
            programme.Fonctions.begin(), programme.Fonctions.end(),
            [](const GsPP::Fonction& fonction)
            {
                return fonction.EstConstructeur
                    && fonction.ClasseProprietaire == "Derivee";
            });
        Exiger(constructeurDerive != programme.Fonctions.end(),
               "constructeur dérivé 0.20 absent");
        Exiger(constructeurDerive->InitialiseurBaseExplicite
                   && constructeurDerive->ArgumentsConstructeurBase.size() == 1,
               "initialiseur parent(...) absent de l’AST");
        Exiger(constructeurDerive->SymboleConstructeurBase.find(
                   "Base::$constructeur") != std::string::npos,
               "constructeur de base non résolu dans le prologue dérivé");
        Exiger(constructeurDerive
                       ->ArgumentsConstructeurBaseParReference.size() == 2
                   && constructeurDerive
                       ->ArgumentsConstructeurBaseParReference[0],
               "ABI des arguments du constructeur de base incorrecte");

        const auto machine = GsPP::GenerateurX64().Generer(programme);
        Exiger(std::any_of(
                   machine.Relocalisations.begin(),
                   machine.Relocalisations.end(),
                   [](const GsPP::CodeMachine::Relocalisation& relocalisation)
                   {
                       return relocalisation.Section
                               == GsPP::SectionMachine::Texte
                           && relocalisation.Symbole.find("Base::Lire")
                               != std::string::npos;
                   }),
               "parent.Lire() n’est pas généré comme appel direct");

        const auto chaineImplicite = Analyser(R"(
            publique entier32 Constructions = 0;
            classe Racine
            {
                publique: constructeur()
                { Constructions = Constructions + 1; }
            };
            classe Intermediaire : publique Racine {};
            classe Feuille : publique Intermediaire {};
            publique entier32 Principal()
            { Feuille valeur; retourner Constructions; }
        )");
        const auto machineImplicite = GsPP::GenerateurX64().Generer(
            chaineImplicite);
        Exiger(std::count_if(
                   machineImplicite.Relocalisations.begin(),
                   machineImplicite.Relocalisations.end(),
                   [](const GsPP::CodeMachine::Relocalisation& relocalisation)
                   {
                       return relocalisation.Section
                               == GsPP::SectionMachine::Texte
                           && relocalisation.Symbole.find(
                                  "Racine::$constructeur")
                               != std::string::npos;
                   }) == 1,
               "un constructeur ancestral implicite est appelé plusieurs fois");

        auto doitEchouer = [](std::string source, std::string motif)
        {
            bool refuse = false;
            try { (void)Analyser(source); }
            catch (const GsPP::ErreurCompilation& erreur)
            {
                refuse = std::string(erreur.what()).find(motif)
                    != std::string::npos;
            }
            Exiger(refuse,
                   "diagnostic d’initialisation parent attendu absent : "
                       + motif);
        };
        doitEchouer(R"(
            classe SansBase
            { publique: constructeur() : parent() {} };
            publique entier32 Principal() { retourner 0; }
        )", "sans classe de base");
        doitEchouer(R"(
            classe B {};
            classe D : publique B
            { publique: constructeur() : parent(1) {} };
            publique entier32 Principal() { retourner 0; }
        )", "ne déclare aucun constructeur");
        doitEchouer(R"(
            classe B
            { publique: constructeur(entier32 valeur) {} };
            classe D : publique B
            { publique: constructeur() {} };
            publique entier32 Principal() { retourner 0; }
        )", "aucune surcharge compatible");
        doitEchouer(R"(
            classe B
            { privée: constructeur(entier32 valeur) {} };
            classe D : publique B
            { publique: constructeur() : parent(1) {} };
            publique entier32 Principal() { retourner 0; }
        )", "constructeur de base inaccessible");
        doitEchouer(R"(
            publique entier32 Principal()
            { retourner parent.Inconnue(); }
        )", "uniquement disponible dans une méthode");

        const auto francais = Compiler(R"(
            classe B
            {
                publique:
                    constructeur(entier32 valeur) {}
                    virtuel entier32 Lire() { retourner 40; }
            };
            classe D : publique B
            {
                publique:
                    constructeur(entier32 valeur) : parent(valeur) {}
                    remplacer entier32 Lire()
                    { retourner parent.Lire() + 2; }
            };
            publique entier32 Principal()
            { D valeur(40); retourner valeur.Lire(); }
        )");
        const auto anglais = Compiler(R"(
            class B
            {
                public:
                    constructor(int32 value) {}
                    virtual int32 Lire() { return 40; }
            };
            class D : public B
            {
                public:
                    constructor(int32 value) : super(value) {}
                    override int32 Lire()
                    { return super.Lire() + 2; }
            };
            public int32 Principal()
            { D value(40); return value.Lire(); }
        )");
        Exiger(francais.Texte == anglais.Texte
                   && francais.Donnees == anglais.Donnees,
               "parent et super ne produisent pas le même code machine");
    }

    void TesterInitialiseursChamps021()
    {
        auto programme = Analyser(R"(
            structure PointInitialise
            {
                entier32 X;
                entier32 Y;
            };

            publique PointInitialise CreerPointInitialise(
                entier32 x,
                entier32 y)
            {
                PointInitialise point = {x, y};
                retourner point;
            }

            classe BaseInitialiseurs
            {
                protégée: entier32 Valeur;
                publique:
                    constructeur(entier32 valeur)
                    { soi.Valeur = valeur; }
                    virtuel entier32 Lire()
                    { retourner soi.Valeur; }
            };

            classe DeriveeInitialiseurs : publique BaseInitialiseurs
            {
                privée:
                    constante entier32 Bonus;
                    PointInitialise Position;
                    PointInitialise Copie;
                    entier32* Cible;
                    entier32 Tableau[2];
                    constante caractère* Etiquette;
                    entier32 Corps;
                    alias BonusAlias = Bonus;
                publique:
                    constructeur(
                        entier32 valeur,
                        entier32 bonus,
                        entier32* cible)
                        : parent(valeur),
                          BonusAlias(bonus),
                          Position({2, 3}),
                          Copie(CreerPointInitialise(6, 7)),
                          Cible(cible),
                          Tableau({4, 5}),
                          Etiquette("initialiseur-021")
                    { soi.Corps = 1; }
                    remplacer entier32 Lire()
                    {
                        retourner parent.Lire() + soi.Bonus
                            + soi.Position.X + soi.Position.Y
                            + soi.Copie.X + soi.Copie.Y
                            + *soi.Cible + soi.Tableau[0]
                            + soi.Tableau[1] + soi.Corps;
                    }
            };

            publique entier32 Principal()
            {
                entier32 cibleValeur = 7;
                DeriveeInitialiseurs valeur(40, 2, &cibleValeur);
                retourner valeur.Lire();
            }
        )");

        const auto constructeur = std::find_if(
            programme.Fonctions.begin(), programme.Fonctions.end(),
            [](const GsPP::Fonction& fonction)
            {
                return fonction.EstConstructeur
                    && fonction.ClasseProprietaire
                        == "DeriveeInitialiseurs";
            });
        Exiger(constructeur != programme.Fonctions.end(),
               "constructeur à initialisateurs de champs absent");
        Exiger(constructeur->InitialiseurBaseExplicite,
               "initialiseur parent 0.21 absent");
        Exiger(constructeur->InitialiseursChamps.size() == 6,
               "nombre d’initialiseurs de champs incorrect");
        Exiger(constructeur->InitialiseursChamps[0].Nom == "BonusAlias"
                   && constructeur->InitialiseursChamps[0].NomCanonique
                       == "Bonus",
               "alias d’initialiseur de champ non normalisé");
        for (const auto& initialiseur : constructeur->InitialiseursChamps)
            Exiger(initialiseur.Arguments.size() == 1,
                   "arité d’initialiseur de champ incorrecte");
        for (std::size_t index = 1;
             index < constructeur->InitialiseursChamps.size(); ++index)
            Exiger(
                constructeur->InitialiseursChamps[index - 1].Decalage
                    < constructeur->InitialiseursChamps[index].Decalage,
                "ordre ou décalage des initialisateurs de champs incorrect");

        const auto machine = GsPP::GenerateurX64().Generer(programme);
        Exiger(!machine.Texte.empty(),
               "aucun code produit pour les initialisateurs de champs");
        const std::string marqueurChaine = "initialiseur-021";
        Exiger(std::search(
                   machine.Donnees.begin(), machine.Donnees.end(),
                   marqueurChaine.begin(), marqueurChaine.end())
                   != machine.Donnees.end(),
               "chaîne d’un initialiseur de champ absente des données");

        auto doitEchouer = [](std::string source, std::string motif)
        {
            bool refuse = false;
            try { (void)Analyser(source); }
            catch (const GsPP::ErreurCompilation& erreur)
            {
                refuse = std::string(erreur.what()).find(motif)
                    != std::string::npos;
            }
            Exiger(refuse,
                   "diagnostic d’initialiseur de champ attendu absent : "
                       + motif);
        };
        doitEchouer(R"(
            classe C
            {
                entier32 A;
                publique: constructeur(entier32 v) : A(v), A(v) {}
            };
        )", "plusieurs fois");
        doitEchouer(R"(
            classe C
            {
                entier32 A;
                entier32 B;
                publique: constructeur(entier32 v) : B(v), A(v) {}
            };
        )", "ordre de déclaration");
        doitEchouer(R"(
            classe B { protégée: entier32 A; };
            classe D : publique B
            { publique: constructeur(entier32 v) : A(v) {} };
        )", "déclarés directement");
        doitEchouer(R"(
            classe C
            { publique: constructeur(entier32 v) : Inconnu(v) {} };
        )", "champ introuvable");
        doitEchouer(R"(
            classe C
            {
                entier32 A;
                publique: constructeur() : A() {}
            };
        )", "exactement une expression");
        doitEchouer(R"(
            classe Membre {};
            classe C
            {
                Membre Valeur;
                publique: constructeur() : Valeur(0) {}
            };
        )", "aucun constructeur déclaré pour le champ objet classe");
        doitEchouer(R"(
            classe B {};
            classe D : publique B
            {
                entier32 A;
                publique: constructeur() : A(1), parent() {}
            };
        )", "premier initialiseur");

        const auto francais = Compiler(R"(
            structure P { entier32 X; entier32 Y; };
            classe B
            {
                protégée: entier32 V;
                publique:
                    constructeur(entier32 v) { soi.V = v; }
            };
            classe D : publique B
            {
                privée: constante entier32 A; P Valeur;
                publique:
                    constructeur(entier32 v, entier32 a)
                        : parent(v), A(a), Valeur({2, 3}) {}
                    entier32 Lire()
                    { retourner soi.V + soi.A + soi.Valeur.X; }
            };
            publique entier32 Principal()
            { D valeur(40, 2); retourner valeur.Lire(); }
        )");
        const auto anglais = Compiler(R"(
            struct P { int32 X; int32 Y; };
            class B
            {
                protected: int32 V;
                public:
                    constructor(int32 v) { this.V = v; }
            };
            class D : public B
            {
                private: const int32 A; P Valeur;
                public:
                    constructor(int32 v, int32 a)
                        : super(v), A(a), Valeur({2, 3}) {}
                    int32 Lire()
                    { return this.V + this.A + this.Valeur.X; }
            };
            public int32 Principal()
            { D valeur(40, 2); return valeur.Lire(); }
        )");
        Exiger(francais.Texte == anglais.Texte
                   && francais.Donnees == anglais.Donnees,
               "les initialisateurs de champs français et anglais divergent");
    }

    void TesterChampsObjetsClasses022()
    {
        auto programme = Analyser(R"(
            classe A
            {
                publique:
                    constructeur(entier32 valeur) {}
                    destructeur() {}
            };
            classe B
            {
                publique:
                    constructeur() {}
                    destructeur() {}
            };
            classe Feuille
            {
                publique:
                    constructeur() {}
                    destructeur() {}
            };
            classe Enveloppe { Feuille Valeur; };
            classe Hote
            {
                A Explicite;
                B Defaut;
                Enveloppe Imbrique;
                publique:
                    constructeur(entier32 valeur) : Explicite(valeur) {}
                    destructeur() {}
            };
            publique entier32 Principal()
            {
                Hote objet(2);
                retourner 0;
            }
        )");

        const auto constructeur = std::find_if(
            programme.Fonctions.begin(), programme.Fonctions.end(),
            [](const GsPP::Fonction& fonction)
            {
                return fonction.EstConstructeur
                    && fonction.ClasseProprietaire == "Hote";
            });
        Exiger(constructeur != programme.Fonctions.end(),
               "constructeur du conteneur 0.22 absent");
        Exiger(constructeur->InitialiseursChamps.size() == 3,
               "les champs objets implicites ne sont pas tous planifiés");
        Exiger(
            constructeur->InitialiseursChamps[0].NomCanonique == "Explicite"
                && !constructeur->InitialiseursChamps[0].EstImplicite
                && constructeur->InitialiseursChamps[0].EstObjetClasse
                && !constructeur->InitialiseursChamps[0]
                        .SymboleConstructeur.empty()
                && constructeur->InitialiseursChamps[0].Arguments.size() == 1,
            "le constructeur explicite du premier champ est incorrect");
        Exiger(
            constructeur->InitialiseursChamps[1].NomCanonique == "Defaut"
                && constructeur->InitialiseursChamps[1].EstImplicite
                && !constructeur->InitialiseursChamps[1]
                        .SymboleConstructeur.empty()
                && constructeur->InitialiseursChamps[1].Arguments.empty(),
            "le constructeur par défaut du deuxième champ est incorrect");
        Exiger(
            constructeur->InitialiseursChamps[2].NomCanonique == "Imbrique"
                && constructeur->InitialiseursChamps[2].EstImplicite
                && constructeur->InitialiseursChamps[2]
                        .SymboleConstructeur.empty()
                && constructeur->InitialiseursChamps[2]
                        .EtapesConstructionImplicite.size() == 1
                && !constructeur->InitialiseursChamps[2]
                        .EtapesConstructionImplicite[0]
                        .SymboleConstructeur.empty(),
            "la construction récursive d’une classe sans constructeur est incorrecte");

        const auto principal = std::find_if(
            programme.Fonctions.begin(), programme.Fonctions.end(),
            [](const GsPP::Fonction& fonction)
            { return fonction.NomSourceComplet() == "Principal"; });
        Exiger(principal != programme.Fonctions.end(),
               "fonction principale 0.22 absente");
        const auto variable = std::find_if(
            principal->Corps->Instructions.begin(),
            principal->Corps->Instructions.end(),
            [](const std::unique_ptr<GsPP::Instruction>& instruction)
            { return instruction->Genre == GsPP::GenreInstruction::Variable; });
        Exiger(variable != principal->Corps->Instructions.end(),
               "objet local 0.22 absent");
        const auto& objet = static_cast<const GsPP::InstructionVariable&>(
            **variable);
        Exiger(objet.ActionsDestruction.size() == 4,
               "le plan de destruction récursif est incomplet");
        Exiger(
            objet.ActionsDestruction[0].ClasseRecepteur == "Hote"
                && objet.ActionsDestruction[1].ClasseRecepteur == "Feuille"
                && objet.ActionsDestruction[2].ClasseRecepteur == "B"
                && objet.ActionsDestruction[3].ClasseRecepteur == "A",
            "l’ordre de destruction objet, champs inversés, base est incorrect");
        Exiger(
            objet.ActionsDestruction[0].Decalage == 0
                && objet.ActionsDestruction[1].Decalage
                    > objet.ActionsDestruction[2].Decalage
                && objet.ActionsDestruction[2].Decalage
                    > objet.ActionsDestruction[3].Decalage,
            "les adresses relatives de destruction sont incorrectes");

        const auto machine = GsPP::GenerateurX64().Generer(programme);
        Exiger(!machine.Texte.empty(),
               "aucun code produit pour les champs objets classes");

        auto doitEchouer = [](std::string source, std::string motif)
        {
            bool refuse = false;
            try { (void)Analyser(source); }
            catch (const GsPP::ErreurCompilation& erreur)
            {
                refuse = std::string(erreur.what()).find(motif)
                    != std::string::npos;
            }
            Exiger(refuse,
                   "diagnostic de champ objet classe attendu absent : "
                       + motif);
        };
        doitEchouer(R"(
            classe Membre
            { publique: constructeur(entier32 valeur) {} };
            classe Hote
            { Membre Valeur; publique: constructeur() {} };
        )", "aucune surcharge compatible");
        doitEchouer(R"(
            classe Membre { privée: constructeur() {} };
            classe Hote
            { Membre Valeur; publique: constructeur() {} };
        )", "constructeur inaccessible");
        doitEchouer(R"(
            classe Membre { privée: destructeur() {} };
            classe Hote
            { Membre Valeur; publique: constructeur() {} };
            publique entier32 Principal()
            { Hote objet; retourner 0; }
        )", "destructeur inaccessible");
        const auto francais = Compiler(R"(
            classe M { publique: constructeur() {} destructeur() {} };
            classe C { M Valeur; publique: constructeur() {} };
            publique entier32 Principal() { C valeur; retourner 1; }
        )");
        const auto anglais = Compiler(R"(
            class M { public: constructor() {} destructor() {} };
            class C { M Value; public: constructor() {} };
            public int32 Principal() { C value; return 1; }
        )");
        Exiger(francais.Texte == anglais.Texte
                   && francais.Donnees == anglais.Donnees,
               "les champs objets classes français et anglais divergent");
    }

    void TesterTableauxObjetsClasses023()
    {
        auto programme = Analyser(R"(
            classe Element
            {
                publique:
                    entier32 Valeur;
                    constructeur() { soi.Valeur = 1; }
                    destructeur() {}
            };
            classe Hote
            {
                publique:
                    Element Valeurs[2][3];
                    constructeur() : Valeurs() {}
                    destructeur() {}
            };
            publique entier32 Principal()
            {
                Hote objet;
                Element locaux[2];
                retourner 0;
            }
        )");

        const auto constructeur = std::find_if(
            programme.Fonctions.begin(), programme.Fonctions.end(),
            [](const GsPP::Fonction& fonction)
            {
                return fonction.EstConstructeur
                    && fonction.ClasseProprietaire == "Hote";
            });
        Exiger(constructeur != programme.Fonctions.end(),
               "constructeur du conteneur 0.23 absent");
        Exiger(constructeur->InitialiseursChamps.size() == 1,
               "le tableau de champ objet n’est pas planifié");
        const auto& initialiseur = constructeur->InitialiseursChamps.front();
        Exiger(initialiseur.NomCanonique == "Valeurs"
                   && initialiseur.EstObjetClasse
                   && !initialiseur.EstImplicite
                   && initialiseur.SymboleConstructeur.empty()
                   && initialiseur.EtapesConstructionImplicite.size() == 6,
               "le plan de construction du tableau multidimensionnel est incorrect");
        for (std::size_t index = 0;
             index < initialiseur.EtapesConstructionImplicite.size();
             ++index)
        {
            const auto& etape =
                initialiseur.EtapesConstructionImplicite[index];
            Exiger(etape.Decalage == index * 4
                       && etape.ClasseRecepteur == "Element"
                       && !etape.SymboleConstructeur.empty(),
                   "l’ordre ou l’adresse d’une construction de tableau est incorrect");
        }

        const auto principal = std::find_if(
            programme.Fonctions.begin(), programme.Fonctions.end(),
            [](const GsPP::Fonction& fonction)
            { return fonction.NomSourceComplet() == "Principal"; });
        Exiger(principal != programme.Fonctions.end(),
               "fonction principale 0.23 absente");
        Exiger(principal->Corps->Instructions.size() >= 2,
               "objets locaux 0.23 absents");
        const auto& objet = static_cast<const GsPP::InstructionVariable&>(
            *principal->Corps->Instructions[0]);
        Exiger(objet.ActionsDestruction.size() == 7
                   && objet.ActionsDestruction[0].ClasseRecepteur == "Hote",
               "le plan de destruction du conteneur est incomplet");
        for (std::size_t index = 1; index < 7; ++index)
            Exiger(
                objet.ActionsDestruction[index].ClasseRecepteur == "Element"
                    && objet.ActionsDestruction[index].Decalage
                        == (6 - index) * 4,
                "la destruction du tableau n’est pas effectuée en ordre inverse");

        const auto& locaux = static_cast<const GsPP::InstructionVariable&>(
            *principal->Corps->Instructions[1]);
        Exiger(locaux.EtapesConstructionImplicite.size() == 2
                   && locaux.EtapesConstructionImplicite[0].Decalage == 0
                   && locaux.EtapesConstructionImplicite[1].Decalage == 4
                   && locaux.ActionsDestruction.size() == 2
                   && locaux.ActionsDestruction[0].Decalage == 4
                   && locaux.ActionsDestruction[1].Decalage == 0,
               "le cycle de vie du tableau local d’objets classes est incorrect");

        const auto machine = GsPP::GenerateurX64().Generer(programme);
        Exiger(!machine.Texte.empty(),
               "aucun code produit pour les tableaux d’objets classes");

        auto doitEchouer = [](std::string source, std::string motif)
        {
            bool refuse = false;
            try { (void)Analyser(source); }
            catch (const GsPP::ErreurCompilation& erreur)
            {
                refuse = std::string(erreur.what()).find(motif)
                        != std::string::npos
                    || erreur.Message(GsPP::LangueDiagnostic::Anglais)
                            .find(motif) != std::string::npos;
            }
            Exiger(refuse,
                   "diagnostic de tableau d’objets classes attendu absent : "
                       + motif);
        };
        const auto tableauAvecArguments = Analyser(R"(
            classe Membre { publique: constructeur(entier32 valeur) {} };
            classe Hote
            {
                Membre Valeurs[2];
                publique: constructeur() : Valeurs(1) {}
            };
        )");
        const auto constructeurAvecArguments = std::find_if(
            tableauAvecArguments.Fonctions.begin(),
            tableauAvecArguments.Fonctions.end(),
            [](const GsPP::Fonction& fonction)
            {
                return fonction.EstConstructeur
                    && fonction.ClasseProprietaire == "Hote";
            });
        Exiger(
            constructeurAvecArguments
                != tableauAvecArguments.Fonctions.end()
                && constructeurAvecArguments->InitialiseursChamps.size() == 1
                && constructeurAvecArguments->InitialiseursChamps[0]
                    .Arguments.size() == 1
                && constructeurAvecArguments->InitialiseursChamps[0]
                    .EtapesConstructionImplicite.size() == 2
                && constructeurAvecArguments->InitialiseursChamps[0]
                    .ArgumentsConstructeurParReference.size() == 2,
            "les arguments communs du tableau de champ ne sont pas planifiés");
        doitEchouer(R"(
            classe Membre { publique: constructeur() {} };
            publique entier32 Principal()
            {
                Membre valeurs[2] = {{}, {}};
                retourner 0;
            }
        )", "does not support aggregate initializers yet");

        const auto francais = Compiler(R"(
            classe M
            { publique: constructeur() {} destructeur() {} };
            classe C
            { M Valeurs[2]; publique: constructeur() : Valeurs() {} };
            publique entier32 Principal()
            { C valeur; retourner 1; }
        )");
        const auto anglais = Compiler(R"(
            class M
            { public: constructor() {} destructor() {} };
            class C
            { M Values[2]; public: constructor() : Values() {} };
            public int32 Principal()
            { C value; return 1; }
        )");
        Exiger(francais.Texte == anglais.Texte
                   && francais.Donnees == anglais.Donnees,
               "les tableaux d’objets classes français et anglais divergent");
    }

    void TesterInitialisationDureeVie025()
    {
        auto programme = Analyser(R"(
            classe Element
            {
                publique:
                    entier32 Valeur;
                    constructeur(entier32 valeur)
                    { soi.Valeur = valeur; }
                    destructeur() {}
            };
            classe Configuration
            {
                publique:
                    constante entier32 Identifiant = 11;
                    entier32 Valeurs[2] = {3};
                    Element Elements[2];
                    constructeur() : Elements(6) {}
                    constructeur(entier32 valeur) : soi()
                    { soi.Valeurs[0] = valeur; }
                    destructeur() {}
            };
            publique entier32 Principal()
            {
                Element locaux[2](4);
                retourner 0;
            }
        )");

        const auto constructeurDefaut = std::find_if(
            programme.Fonctions.begin(), programme.Fonctions.end(),
            [](const GsPP::Fonction& fonction)
            {
                return fonction.EstConstructeur
                    && fonction.ClasseProprietaire == "Configuration"
                    && fonction.Parametres.size() == 1;
            });
        Exiger(constructeurDefaut != programme.Fonctions.end(),
               "constructeur par défaut 0.25 absent");
        Exiger(
            constructeurDefaut->InitialiseursChamps.size() == 3
                && constructeurDefaut->InitialiseursChamps[0]
                    .InitialiseurParDefaut != nullptr
                && constructeurDefaut->InitialiseursChamps[1]
                    .InitialiseurParDefaut != nullptr
                && constructeurDefaut->InitialiseursChamps[2]
                    .Arguments.size() == 1
                && constructeurDefaut->InitialiseursChamps[2]
                    .EtapesConstructionImplicite.size() == 2,
            "les valeurs par défaut ou le tableau de champ 0.25 sont incorrects");

        const auto constructeurDelegue = std::find_if(
            programme.Fonctions.begin(), programme.Fonctions.end(),
            [](const GsPP::Fonction& fonction)
            {
                return fonction.EstConstructeur
                    && fonction.ClasseProprietaire == "Configuration"
                    && fonction.Parametres.size() == 2;
            });
        Exiger(
            constructeurDelegue != programme.Fonctions.end()
                && constructeurDelegue->DelegueConstructeur
                && !constructeurDelegue->SymboleConstructeurDelegue.empty()
                && constructeurDelegue->ArgumentsConstructeurDelegue.empty()
                && constructeurDelegue
                    ->ArgumentsConstructeurDelegueParReference.size() == 1
                && constructeurDelegue->InitialiseursChamps.empty(),
            "la délégation de constructeur 0.25 est incorrecte");

        const auto principal = std::find_if(
            programme.Fonctions.begin(), programme.Fonctions.end(),
            [](const GsPP::Fonction& fonction)
            { return fonction.NomSourceComplet() == "Principal"; });
        Exiger(principal != programme.Fonctions.end()
                   && !principal->Corps->Instructions.empty(),
               "fonction principale 0.25 absente");
        const auto& locaux = static_cast<const GsPP::InstructionVariable&>(
            *principal->Corps->Instructions.front());
        Exiger(
            locaux.ArgumentsConstruction.size() == 1
                && locaux.ArgumentsConstructionParReference.size() == 2
                && locaux.EtapesConstructionImplicite.size() == 2
                && locaux.EtapesConstructionImplicite[0].Decalage == 0
                && locaux.EtapesConstructionImplicite[1].Decalage == 4
                && locaux.ActionsDestruction.size() == 2
                && locaux.ActionsDestruction[0].Decalage == 4,
            "le cycle de vie du tableau local avec arguments est incorrect");

        const auto machine = GsPP::GenerateurX64().Generer(programme);
        Exiger(!machine.Texte.empty(),
               "aucun code produit pour l’initialisation 0.25");

        auto doitEchouer = [](std::string source, std::string motif)
        {
            bool refuse = false;
            try { (void)Analyser(source); }
            catch (const GsPP::ErreurCompilation& erreur)
            {
                refuse = std::string(erreur.what()).find(motif)
                        != std::string::npos
                    || erreur.Message(GsPP::LangueDiagnostic::Anglais)
                            .find(motif) != std::string::npos;
            }
            Exiger(refuse,
                   "diagnostic d’initialisation 0.25 attendu absent : "
                       + motif);
        };
        doitEchouer(R"(
            classe C
            {
                publique:
                    constructeur() : soi(1) {}
                    constructeur(entier32 valeur) : soi() {}
            };
            publique entier32 Principal() { retourner 0; }
        )", "cycle de délégation");
        doitEchouer(R"(
            classe C { publique: constructeur() : soi() {} };
            publique entier32 Principal() { retourner 0; }
        )", "déléguer directement vers lui-même");
        doitEchouer(R"(
            classe M { publique: constructeur() {} };
            M Globale;
            publique entier32 Principal() { retourner 0; }
        )", "objets classes globaux");
        doitEchouer(R"(
            classe C { publique: entier32 Valeur = 1; };
            publique entier32 Principal() { retourner 0; }
        )", "doit déclarer un constructeur");
        doitEchouer(R"(
            structure S { entier32 Valeur = 1; };
            publique entier32 Principal() { retourner 0; }
        )", "réservé aux classes");
        doitEchouer(R"(
            classe M { publique: constructeur(entier32 valeur) {} };
            classe C
            {
                publique:
                    M Valeur = 1;
                    constructeur() {}
            };
            publique entier32 Principal() { retourner 0; }
        )", "utilise un constructeur");
        doitEchouer(R"(
            classe C
            {
                publique:
                    entier32 Valeur;
                    constructeur() : soi(), Valeur(1) {}
            };
            publique entier32 Principal() { retourner 0; }
        )", "ne peut pas initialiser directement");

        const auto francais = Compiler(R"(
            classe E
            {
                publique:
                    entier32 V;
                    constructeur(entier32 valeur) { soi.V = valeur; }
                    destructeur() {}
            };
            classe C
            {
                publique:
                    entier32 V = 5;
                    E Elements[2];
                    constructeur() : Elements(3) {}
                    constructeur(entier32 valeur) : soi() { soi.V = valeur; }
            };
            publique entier32 Principal()
            { C valeur(8); E locaux[2](4); retourner valeur.V; }
        )");
        const auto anglais = Compiler(R"(
            class E
            {
                public:
                    int32 V;
                    constructor(int32 value) { this.V = value; }
                    destructor() {}
            };
            class C
            {
                public:
                    int32 V = 5;
                    E Elements[2];
                    constructor() : Elements(3) {}
                    constructor(int32 value) : this() { this.V = value; }
            };
            public int32 Principal()
            { C value(8); E locals[2](4); return value.V; }
        )");
        Exiger(francais.Texte == anglais.Texte
                   && francais.Donnees == anglais.Donnees,
               "l’initialisation 0.25 française et anglaise diverge");
    }

    void TesterBibliothequeEtEditionLiens()
    {
        const auto calculs = Compiler(R"(
            publique entier32 CompteurPartage = 1;
            publique entier32 Doubler(entier32 valeur)
            {
                CompteurPartage = CompteurPartage + 1;
                retourner valeur * 2;
            }
        )");
        const auto inutilise = Compiler(R"(
            externe entier32 ServiceQuiNeDoitPasEtreImporte();
            publique entier32 FonctionInutilisee()
            {
                retourner ServiceQuiNeDoitPasEtreImporte();
            }
        )");
        const auto principal = Compiler(R"(
            externe entier32 Doubler(entier32 valeur);
            externe entier32 CompteurPartage;
            publique entier32 Principal()
            {
                retourner Doubler(21) + CompteurPartage;
            }
        )");

        const auto contenuCalculs = GsPP::EcrivainGsO().Construire(calculs);
        const auto contenuInutilise = GsPP::EcrivainGsO().Construire(inutilise);
        const std::vector<GsPP::MembreGsA> membres{
            {"Calculs.GsObj", contenuCalculs},
            {"Inutilise.GsObj", contenuInutilise}
        };
        const auto archive = GsPP::EcrivainGsA().Construire(membres);
        Exiger(archive.size() >= GsPP::TailleEnteteGsA
                   && archive[0] == 'G' && archive[1] == 'S'
                   && archive[2] == 'A' && archive[3] == ':'
                   && archive[4] == '0' && archive[5] == 0
                   && archive[6] == 0 && archive[7] == 0
                   && archive[8] == GsPP::VersionMajeureGsA
                   && archive[10] == GsPP::VersionMineureGsA
                   && archive[20] == GsPP::VersionAbiGsA,
               "format GsA différent de 1.0");
        const auto archiveRelue = GsPP::LecteurGsA().Lire(archive);
        Exiger(archiveRelue.size() == 2, "membres GsA perdus");

        GsPP::BibliothequeLiaison bibliotheque;
        for (const auto& membre : archiveRelue)
            bibliotheque.push_back({membre.Nom, GsPP::LecteurGsO().Lire(membre.Objet)});
        const auto lie = GsPP::EditeurLiens().Lier(
            {{"Principal.GsObj", principal}}, {bibliotheque}, "Principal");
        Exiger(std::any_of(
                   lie.Symboles.begin(), lie.Symboles.end(),
                   [](const GsPP::SymboleMachine& symbole)
                   { return symbole.Nom == "Doubler" && symbole.EstDefini; }),
               "membre GsA requis non extrait");
        Exiger(std::none_of(
                   lie.Symboles.begin(), lie.Symboles.end(),
                   [](const GsPP::SymboleMachine& symbole)
                   { return symbole.Nom == "ServiceQuiNeDoitPasEtreImporte"; }),
               "un membre GsA inutilisé a été extrait");
        const auto gse = GsPP::EcrivainGsE().Construire(lie, "Principal");
        const auto image = GsPP::ChargeurGsE().Charger(gse, 0x10000000);
        Exiger(image.Imports.empty(), "la liaison séparée conserve des imports résolus");

        const auto principalIncompatible = Compiler(R"(
            externe entier32 Doubler(entier64 valeur);
            publique entier32 Principal()
            {
                retourner Doubler(convertir<entier64>(21));
            }
        )");
        bool abiRefusee = false;
        try
        {
            (void)GsPP::EditeurLiens().Lier(
                {{"PrincipalIncompatible.GsObj", principalIncompatible}},
                {bibliotheque}, "Principal");
        }
        catch (const std::runtime_error& erreur)
        { abiRefusee = std::string(erreur.what()).find("incompatibilité ABI") != std::string::npos; }
        Exiger(abiRefusee, "une incompatibilité ABI entre unités a été acceptée");

        const auto dispositionA = Compiler(R"(
            structure Paquet { entier32 A; entier32 B; };
            publique entier32 LirePaquet(Paquet* paquet) { retourner paquet->A; }
        )");
        const auto dispositionB = Compiler(R"(
            structure Paquet { entier64 A; };
            externe entier32 LirePaquet(Paquet* paquet);
            publique entier32 Principal()
            {
                Paquet paquet;
                retourner LirePaquet(&paquet);
            }
        )");
        bool dispositionRefusee = false;
        try
        {
            (void)GsPP::EditeurLiens().Lier({
                {"DispositionA.GsObj", dispositionA},
                {"DispositionB.GsObj", dispositionB}}, {}, "Principal");
        }
        catch (const std::runtime_error& erreur)
        { dispositionRefusee = std::string(erreur.what()).find("incompatibilité ABI") != std::string::npos; }
        Exiger(dispositionRefusee,
               "une disposition de structure incompatible a été acceptée par l’ABI");

        const auto uniteLocaleA = Compiler(R"(
            entier32 Aide() { retourner 1; }
            publique entier32 Premier() { retourner Aide(); }
        )");
        const auto uniteLocaleB = Compiler(R"(
            entier32 Aide() { retourner 2; }
            publique entier32 Second() { retourner Aide(); }
        )");
        const auto appelantLocal = Compiler(R"(
            externe entier32 Premier();
            externe entier32 Second();
            publique entier32 Principal() { retourner Premier() + Second(); }
        )");
        const auto lieLocal = GsPP::EditeurLiens().Lier({
            {"LocaleA.GsObj", uniteLocaleA},
            {"LocaleB.GsObj", uniteLocaleB},
            {"Appelant.GsObj", appelantLocal}}, {}, "Principal");
        const auto nombreAidesLocales = std::count_if(
            lieLocal.Symboles.begin(), lieLocal.Symboles.end(),
            [](const GsPP::SymboleMachine& symbole)
            { return symbole.Nom.find("@GsLocal") == 0
                  && symbole.Nom.find("::Aide") != std::string::npos; });
        Exiger(nombreAidesLocales == 2,
               "des symboles privés homonymes de deux unités sont entrés en collision");

        bool definitionDupliqueeRefusee = false;
        try
        {
            (void)GsPP::EditeurLiens().Lier({
                {"CalculsA.GsObj", calculs},
                {"CalculsB.GsObj", calculs}}, {}, "Doubler");
        }
        catch (const std::runtime_error& erreur)
        {
            definitionDupliqueeRefusee = std::string(erreur.what()).find(
                "défini plusieurs fois") != std::string::npos;
        }
        Exiger(definitionDupliqueeRefusee,
               "une définition publique dupliquée a été acceptée");

        auto archiveCorrompue = archive;
        archiveCorrompue[24] ^= 1;
        bool archiveRefusee = false;
        try { (void)GsPP::LecteurGsA().Lire(archiveCorrompue); }
        catch (const std::runtime_error& erreur)
        { archiveRefusee = std::string(erreur.what()).find("taille") != std::string::npos; }
        Exiger(archiveRefusee, "une taille totale GsA falsifiée a été acceptée");

        archiveCorrompue = archive;
        archiveCorrompue[8] = 2;
        bool ancienneVersionArchiveRefusee = false;
        try { (void)GsPP::LecteurGsA().Lire(archiveCorrompue); }
        catch (const std::runtime_error& erreur)
        {
            ancienneVersionArchiveRefusee = std::string(erreur.what()).find("version")
                != std::string::npos;
        }
        Exiger(ancienneVersionArchiveRefusee,
               "une bibliothèque GsA portant une version locale différente de 1.0 a été acceptée");

        archiveCorrompue = archive;
        archiveCorrompue[20] = 2;
        bool ancienneAbiArchiveRefusee = false;
        try { (void)GsPP::LecteurGsA().Lire(archiveCorrompue); }
        catch (const std::runtime_error& erreur)
        {
            ancienneAbiArchiveRefusee = std::string(erreur.what()).find("ABI")
                != std::string::npos;
        }
        Exiger(ancienneAbiArchiveRefusee,
               "une bibliothèque GsA portant l’ABI 2 a été acceptée");

        archiveCorrompue = archive;
        archiveCorrompue[36] = 1;
        bool reserveMembreRefusee = false;
        try { (void)GsPP::LecteurGsA().Lire(archiveCorrompue); }
        catch (const std::runtime_error& erreur)
        {
            reserveMembreRefusee = std::string(erreur.what()).find("réservé")
                != std::string::npos;
        }
        Exiger(reserveMembreRefusee,
               "une entrée de membre GsA réservée non nulle a été acceptée");
    }
}

int main()
{
    try
    {
        TesterExtensionsGalacticShrine();
        TesterAliasMotsCles();
        TesterAliasApplicatifs();
        TesterErreursAlias();
        TesterTypesSysteme();
        TesterContratDemarrage();
        TesterPrecedence();
        TesterChainesEtLogique();
        TesterBitsEtIntrinseques();
        TesterVariablesControlesEtAppel();
        TesterEspaceUnicode();
        TesterObjetCoff();
        TesterStructuresEtPointeurs();
        TesterValeursStructures();
        TesterIndexationPointeurs();
        TesterPointeursFonction();
        TesterModeleObjet018();
        TesterHeritage019();
        TesterInitialisationParent020();
        TesterInitialiseursChamps021();
        TesterChampsObjetsClasses022();
        TesterTableauxObjetsClasses023();
        TesterInitialisationDureeVie025();
        TesterGsE();
        TesterGlobalesImportsEtExports();
        TesterNomsSymbolesGsELongs();
        TesterChargeurGsE();
        TesterObjetNatifGsO();
        TesterBibliothequeEtEditionLiens();
        std::cout << "Tous les tests Gs++ 0.27.0-alpha.7 ont réussi.\n";
        return 0;
    }
    catch (const GsPP::ErreurCompilation& erreur)
    {
        std::cerr << "Échec de compilation " << erreur.Fichier()
                  << ':' << erreur.Ligne() << ':' << erreur.Colonne()
                  << " : " << erreur.what() << '\n';
        return 1;
    }
    catch (const std::exception& erreur)
    {
        std::cerr << "Échec : " << erreur.what() << '\n';
        return 1;
    }
}
