#include "GsPP/ChargeurGsE.hpp"
#include "GsPP/ErreurCompilation.hpp"
#include "GsPP/Lexeur.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined(__unix__) || defined(__APPLE__)
#include <sys/mman.h>
#include <unistd.h>
#endif

#if defined(__GNUC__) && defined(__x86_64__) && !defined(_WIN32)
#define GS_ABI_HOTE __attribute__((ms_abi))
#else
#define GS_ABI_HOTE
#endif

namespace
{
    struct VueTexteHote
    {
        const char* Donnees;
        std::uint64_t Taille;
    };

    struct RequeteFichierHote
    {
        const char* Chemin;
        std::uint8_t* Donnees;
        std::uint64_t Taille;
        std::uint64_t Capacite;
    };

    struct DiagnosticHote
    {
        std::uint32_t Niveau;
        std::uint32_t Ligne;
        std::uint32_t Colonne;
        std::uint32_t Reserve;
        VueTexteHote Fichier;
        VueTexteHote Message;
    };

    struct JetonLexeHote
    {
        std::uint32_t Genre;
        std::uint32_t Ligne;
        std::uint32_t Colonne;
        std::uint32_t Reserve;
        std::uint64_t Debut;
        std::uint64_t TailleSource;
        std::uint64_t TailleTexte;
        std::uint64_t HachageTexte;
    };

    struct ResultatLexageHote
    {
        std::uint32_t Erreur;
        std::uint32_t LigneErreur;
        std::uint32_t ColonneErreur;
        std::uint32_t Reserve;
        std::uint64_t NombreJetons;
        std::uint64_t CapaciteRequise;
    };

    struct RequeteLexageHote
    {
        const char* Source;
        std::uint64_t Taille;
        JetonLexeHote* Jetons;
        std::uint64_t Capacite;
        ResultatLexageHote Resultat;
    };

    static_assert(sizeof(VueTexteHote) == 16);
    static_assert(sizeof(RequeteFichierHote) == 32);
    static_assert(sizeof(DiagnosticHote) == 48);
    static_assert(sizeof(JetonLexeHote) == 48);
    static_assert(sizeof(ResultatLexageHote) == 32);
    static_assert(sizeof(RequeteLexageHote) == 64);

    std::string CheminLu;
    std::string CheminEcrit;
    std::vector<std::string> CheminsEcrits;
    std::vector<std::uint8_t> DonneesEcrites;
    std::uint32_t NiveauDiagnostic = 0;
    std::uint32_t LigneDiagnostic = 0;
    std::uint32_t ColonneDiagnostic = 0;
    std::string FichierDiagnostic;
    std::string MessageDiagnostic;
    std::vector<std::uint8_t*> AllocationsActives;
    std::uint64_t NombreAllocations = 0;
    std::uint64_t NombreLiberations = 0;
    bool LiberationInvalide = false;
    bool EchecTransactionTeste = false;

    std::uint8_t* GS_ABI_HOTE AllouerMemoireHote(
        std::uint64_t taille)
    {
        if (taille == 0)
            return nullptr;
        if (taille == 4097)
        {
            EchecTransactionTeste = true;
            return nullptr;
        }
        auto* resultat = new (std::nothrow) std::uint8_t[
            static_cast<std::size_t>(taille)];
        if (resultat != nullptr)
        {
            AllocationsActives.push_back(resultat);
            ++NombreAllocations;
        }
        return resultat;
    }

    void GS_ABI_HOTE LibererMemoireHote(std::uint8_t* adresse)
    {
        if (adresse == nullptr)
            return;
        const auto position = std::find(
            AllocationsActives.begin(), AllocationsActives.end(), adresse);
        if (position == AllocationsActives.end())
        {
            LiberationInvalide = true;
            return;
        }
        AllocationsActives.erase(position);
        delete[] adresse;
        ++NombreLiberations;
    }

    std::uint32_t GS_ABI_HOTE LireFichierHote(
        RequeteFichierHote* requete)
    {
        if (requete == nullptr || requete->Chemin == nullptr)
            return 0;
        CheminLu = requete->Chemin;
        constexpr std::array<std::uint8_t, 6> contenu{
            'G', 's', '+', '+', '\n', '!'};
        if (requete->Donnees == nullptr && requete->Capacite == 0)
        {
            requete->Taille = contenu.size();
            return 1;
        }
        if (requete->Donnees == nullptr
            || requete->Capacite < contenu.size())
            return 0;
        std::memcpy(requete->Donnees, contenu.data(), contenu.size());
        requete->Taille = contenu.size();
        return 1;
    }

    std::uint32_t GS_ABI_HOTE EcrireFichierHote(
        RequeteFichierHote* requete)
    {
        if (requete == nullptr || requete->Chemin == nullptr
            || requete->Taille > requete->Capacite
            || (requete->Taille != 0 && requete->Donnees == nullptr))
            return 0;
        CheminEcrit = requete->Chemin;
        CheminsEcrits.push_back(CheminEcrit);
        DonneesEcrites.assign(
            requete->Donnees,
            requete->Donnees + static_cast<std::ptrdiff_t>(requete->Taille));
        return 1;
    }

    void GS_ABI_HOTE EmettreDiagnosticHote(
        DiagnosticHote* diagnostic)
    {
        if (diagnostic == nullptr)
            return;
        NiveauDiagnostic = diagnostic->Niveau;
        LigneDiagnostic = diagnostic->Ligne;
        ColonneDiagnostic = diagnostic->Colonne;
        FichierDiagnostic.assign(
            diagnostic->Fichier.Donnees,
            static_cast<std::size_t>(diagnostic->Fichier.Taille));
        MessageDiagnostic.assign(
            diagnostic->Message.Donnees,
            static_cast<std::size_t>(diagnostic->Message.Taille));
    }

    void Exiger(bool condition, const std::string& message)
    {
        if (!condition) throw std::runtime_error(message);
    }

    std::vector<std::uint8_t> LireFichier(const std::string& chemin)
    {
        std::ifstream flux(chemin, std::ios::binary);
        if (!flux)
            throw std::runtime_error(
                "impossible de lire l’image GsE : " + chemin);
        return {
            std::istreambuf_iterator<char>(flux),
            std::istreambuf_iterator<char>()};
    }

    std::uint64_t Lire64(
        const std::vector<std::uint8_t>& contenu,
        std::size_t position)
    {
        if (position + 8 > contenu.size())
            throw std::runtime_error("entête GsE tronqué");
        std::uint64_t valeur = 0;
        for (unsigned index = 0; index < 8; ++index)
            valeur |= static_cast<std::uint64_t>(
                contenu[position + index]) << (index * 8);
        return valeur;
    }

    class ZoneExecutable final
    {
    public:
        explicit ZoneExecutable(std::size_t taille) : _Taille(taille)
        {
#if defined(_WIN32)
            _Adresse = VirtualAlloc(
                nullptr, _Taille, MEM_COMMIT | MEM_RESERVE,
                PAGE_EXECUTE_READWRITE);
#elif defined(__unix__) || defined(__APPLE__)
            _Adresse = mmap(
                nullptr, _Taille,
                PROT_READ | PROT_WRITE | PROT_EXEC,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (_Adresse == MAP_FAILED) _Adresse = nullptr;
#endif
            if (_Adresse == nullptr)
                throw std::runtime_error(
                    "mémoire exécutable indisponible pour le test");
        }

        ZoneExecutable(const ZoneExecutable&) = delete;
        ZoneExecutable& operator=(const ZoneExecutable&) = delete;

        ~ZoneExecutable()
        {
#if defined(_WIN32)
            if (_Adresse != nullptr)
                VirtualFree(_Adresse, 0, MEM_RELEASE);
#elif defined(__unix__) || defined(__APPLE__)
            if (_Adresse != nullptr)
                munmap(_Adresse, _Taille);
#endif
        }

        [[nodiscard]] std::uint64_t Base() const
        {
            return reinterpret_cast<std::uintptr_t>(_Adresse);
        }

        void Copier(const std::vector<std::uint8_t>& contenu)
        {
            if (contenu.size() > _Taille)
                throw std::runtime_error("image GsE trop grande");
            std::memcpy(_Adresse, contenu.data(), contenu.size());
        }

        [[nodiscard]] std::uint64_t AjouterTrampoline(
            std::size_t position,
            std::uint64_t cible)
        {
            if (position + 12 > _Taille)
                throw std::runtime_error("zone de trampoline dépassée");
            auto* code = static_cast<std::uint8_t*>(_Adresse) + position;
            code[0] = 0x48;
            code[1] = 0xB8;
            for (unsigned index = 0; index < 8; ++index)
                code[2 + index] = static_cast<std::uint8_t>(
                    cible >> (index * 8));
            code[10] = 0xFF;
            code[11] = 0xE0;
#if defined(__GNUC__)
            __builtin___clear_cache(
                reinterpret_cast<char*>(code),
                reinterpret_cast<char*>(code + 12));
#endif
            return Base() + position;
        }

    private:
        void* _Adresse = nullptr;
        std::size_t _Taille = 0;
    };

    std::size_t AlignerPage(std::uint64_t taille)
    {
        constexpr std::uint64_t page = 4096;
        return static_cast<std::size_t>((taille + page - 1) & ~(page - 1));
    }

    std::uint64_t HacherTexte(std::string_view texte)
    {
        std::uint64_t valeur = 14'695'981'039'346'656'037ULL;
        for (const unsigned char octet : texte)
        {
            valeur ^= octet;
            valeur *= 1'099'511'628'211ULL;
        }
        return valeur;
    }

    std::size_t PositionSource(
        std::string_view source,
        std::uint32_t ligneCible,
        std::uint32_t colonneCible)
    {
        std::size_t position = 0;
        if (source.size() >= 3
            && static_cast<unsigned char>(source[0]) == 0xEF
            && static_cast<unsigned char>(source[1]) == 0xBB
            && static_cast<unsigned char>(source[2]) == 0xBF)
            position = 3;

        std::uint32_t ligne = 1;
        std::uint32_t colonne = 1;
        while (position < source.size()
               && (ligne != ligneCible || colonne != colonneCible))
        {
            const char valeur = source[position++];
            if (valeur == '\n')
            {
                ++ligne;
                colonne = 1;
            }
            else
            {
                ++colonne;
            }
        }
        if (ligne != ligneCible || colonne != colonneCible)
            throw std::runtime_error(
                "position de jeton Gs++ absente de la source");
        return position;
    }

    std::string TexteJetonAutoHeberge(
        std::string_view source,
        const JetonLexeHote& jeton)
    {
        if (jeton.Debut > source.size()
            || jeton.TailleSource > source.size() - jeton.Debut)
            throw std::runtime_error("tranche source du jeton Gs++ invalide");
        const auto brut = source.substr(
            static_cast<std::size_t>(jeton.Debut),
            static_cast<std::size_t>(jeton.TailleSource));
        if (jeton.Genre != static_cast<std::uint32_t>(
                GsPP::GenreJeton::ChaineCaracteres))
            return std::string(brut);

        if (brut.size() < 2 || brut.front() != '"' || brut.back() != '"')
            throw std::runtime_error("tranche de chaîne Gs++ invalide");
        std::string texte;
        for (std::size_t index = 1; index + 1 < brut.size(); ++index)
        {
            const char valeur = brut[index];
            if (valeur != '\\')
            {
                texte.push_back(valeur);
                continue;
            }
            if (++index + 1 > brut.size())
                throw std::runtime_error("échappement Gs++ tronqué");
            switch (brut[index])
            {
                case '\\': texte.push_back('\\'); break;
                case '"': texte.push_back('"'); break;
                case 'n': texte.push_back('\n'); break;
                case 'r': texte.push_back('\r'); break;
                case 't': texte.push_back('\t'); break;
                case '0': texte.push_back('\0'); break;
                default:
                    throw std::runtime_error(
                        "échappement Gs++ inattendu dans un jeton valide");
            }
        }
        return texte;
    }

    using LexeurAutoHeberge =
        std::uint32_t (GS_ABI_HOTE *)(RequeteLexageHote*);

    void ComparerLexage(
        LexeurAutoHeberge lexer,
        const std::string& source,
        std::string_view nomCorpus)
    {
        const auto reference = GsPP::Lexeur(source, std::string(nomCorpus)).Analyser();

        RequeteLexageHote requete{
            source.data(),
            static_cast<std::uint64_t>(source.size()),
            nullptr,
            0,
            {}};
        const auto interrogation = lexer(&requete);
        Exiger(
            interrogation == 1
                && requete.Resultat.Erreur == 1
                && requete.Resultat.NombreJetons == reference.size()
                && requete.Resultat.CapaciteRequise == reference.size(),
            "interrogation de capacité du lexeur Gs++ incorrecte pour "
                + std::string(nomCorpus));

        if (reference.size() > 1)
        {
            std::vector<JetonLexeHote> insuffisant(reference.size() - 1);
            requete.Jetons = insuffisant.data();
            requete.Capacite = insuffisant.size();
            const auto erreurCapacite = lexer(&requete);
            Exiger(
                erreurCapacite == 1
                    && requete.Resultat.NombreJetons == reference.size()
                    && requete.Resultat.CapaciteRequise == reference.size(),
                "capacité partielle du lexeur Gs++ mal diagnostiquée pour "
                    + std::string(nomCorpus));
        }

        std::vector<JetonLexeHote> jetons(reference.size());
        requete.Jetons = jetons.data();
        requete.Capacite = jetons.size();
        const auto resultat = lexer(&requete);
        Exiger(
            resultat == 0
                && requete.Resultat.Erreur == 0
                && requete.Resultat.NombreJetons == reference.size()
                && requete.Resultat.CapaciteRequise == reference.size(),
            "lexage Gs++ échoué pour " + std::string(nomCorpus));

        for (std::size_t index = 0; index < reference.size(); ++index)
        {
            const auto& attendu = reference[index];
            const auto& obtenu = jetons[index];
            const auto genre = static_cast<std::uint32_t>(attendu.Genre);
            Exiger(
                obtenu.Genre == genre
                    && obtenu.Ligne == attendu.Ligne
                    && obtenu.Colonne == attendu.Colonne
                    && obtenu.Reserve == 0,
                "genre ou position différente au jeton "
                    + std::to_string(index) + " de "
                    + std::string(nomCorpus));

            const auto position = PositionSource(
                source, obtenu.Ligne, obtenu.Colonne);
            Exiger(
                obtenu.Debut == position,
                "décalage source différent au jeton "
                    + std::to_string(index) + " de "
                    + std::string(nomCorpus));

            const std::string texte = TexteJetonAutoHeberge(source, obtenu);
            Exiger(
                texte == attendu.Texte
                    && obtenu.TailleTexte == attendu.Texte.size()
                    && obtenu.HachageTexte == HacherTexte(attendu.Texte),
                "texte différent au jeton " + std::to_string(index)
                    + " de " + std::string(nomCorpus));
        }
    }

    void ComparerErreurLexage(
        LexeurAutoHeberge lexer,
        const std::string& source,
        std::uint32_t erreurAttendue,
        std::string_view nomCorpus)
    {
        std::uint32_t ligne = 0;
        std::uint32_t colonne = 0;
        try
        {
            (void)GsPP::Lexeur(source, std::string(nomCorpus)).Analyser();
            throw std::runtime_error(
                "le lexeur C++ a accepté le corpus invalide "
                    + std::string(nomCorpus));
        }
        catch (const GsPP::ErreurCompilation& erreur)
        {
            ligne = static_cast<std::uint32_t>(erreur.Ligne());
            colonne = static_cast<std::uint32_t>(erreur.Colonne());
        }

        RequeteLexageHote requete{
            source.data(),
            static_cast<std::uint64_t>(source.size()),
            nullptr,
            0,
            {}};
        const auto resultat = lexer(&requete);
        Exiger(
            resultat == erreurAttendue
                && requete.Resultat.Erreur == erreurAttendue
                && requete.Resultat.LigneErreur == ligne
                && requete.Resultat.ColonneErreur == colonne,
            "diagnostic Gs++ différent du bootstrap pour "
                + std::string(nomCorpus));
    }

    void TesterLexeur(const std::string& chemin)
    {
        AllocationsActives.clear();
        NombreAllocations = 0;
        NombreLiberations = 0;
        LiberationInvalide = false;

        const auto contenu = LireFichier(chemin);
        const auto tailleImage = Lire64(contenu, 48);
        const auto debutTrampolines = AlignerPage(tailleImage);
        ZoneExecutable zone(debutTrampolines + 4096);
        const auto allouer = zone.AjouterTrampoline(
            debutTrampolines,
            reinterpret_cast<std::uintptr_t>(&AllouerMemoireHote));
        const auto liberer = zone.AjouterTrampoline(
            debutTrampolines + 16,
            reinterpret_cast<std::uintptr_t>(&LibererMemoireHote));
        const auto resolveur =
            [&](std::string_view nom) -> std::optional<std::uint64_t>
        {
            if (nom == "Gs::Hote::AllouerMemoire") return allouer;
            if (nom == "Gs::Hote::LibererMemoire") return liberer;
            return std::nullopt;
        };
        const auto image = GsPP::ChargeurGsE().Charger(
            contenu, zone.Base(), resolveur);
        zone.Copier(image.Memoire);

        const auto adresse = image.ChercherExport(
            "Gs::Autohebergement::AnalyserSource");
        Exiger(adresse.has_value(), "export du lexeur Gs++ absent");
        const auto lexer = reinterpret_cast<LexeurAutoHeberge>(*adresse);

        const std::vector<std::pair<std::string, std::string>> corpus{
            {"vide", ""},
            {"programme", "espace Démonstration { publique entier32 Principal() { naturel64 valeur_1 = 12_345; si (valeur_1 >= 42 && vrai || faux) retourner 7; } }"},
            {"commentaires", "// ligne ignorée\n/* bloc\nétendu */ classe Exemple : Base { protégée virtuel vide Executer() remplacer; constructeur(); destructeur(); opérateur(); soi; parent; }"},
            {"symboles", "( ) { } [ ] ; , . -> :: : = == != < <= > >= ! && || & | ^ ~ << >> + - * / %"},
            {"chaines", R"gs("Gs++\n\t\r\0\"\\ étendu" "simple")gs"},
            {"bom", std::string("\xEF\xBB\xBF", 3) + "namespace Shrine { public uint64 Compteur = 99; }"}
        };
        for (const auto& [nom, source] : corpus)
            ComparerLexage(lexer, source, nom);

        ComparerErreurLexage(
            lexer, std::string("\xC0\xAF", 2), 2, "utf8-invalide");
        ComparerErreurLexage(
            lexer, "/* commentaire", 3, "commentaire-non-termine");
        ComparerErreurLexage(
            lexer, "\"chaine", 4, "chaine-non-terminee");
        ComparerErreurLexage(
            lexer, "\"chaine\\", 5, "echappement-non-termine");
        ComparerErreurLexage(
            lexer, "\"chaine\\q\"", 6, "echappement-inconnu");
        ComparerErreurLexage(
            lexer, "\n  @", 7, "caractere-inattendu");

        Exiger(
            lexer(nullptr) == 8,
            "une requête de lexage nulle aurait dû être refusée");
        RequeteLexageHote requeteInvalide{nullptr, 1, nullptr, 0, {}};
        Exiger(
            lexer(&requeteInvalide) == 8
                && requeteInvalide.Resultat.LigneErreur == 1
                && requeteInvalide.Resultat.ColonneErreur == 1,
            "une source de lexage nulle aurait dû être refusée");
        Exiger(
            !LiberationInvalide && AllocationsActives.empty(),
            "le lexeur auto-hébergé a altéré l’état mémoire de l’hôte");
    }

    void TesterClassificateur(const std::string& chemin)
    {
        const auto contenu = LireFichier(chemin);
        const auto tailleImage = Lire64(contenu, 48);
        const auto debutTrampolines = AlignerPage(tailleImage);
        ZoneExecutable zone(debutTrampolines + 4096);
        const auto image = GsPP::ChargeurGsE().Charger(
            contenu, zone.Base());
        zone.Copier(image.Memoire);

        const auto adresse = image.ChercherExport(
            "Gs::Autohebergement::ClassifierMotCle");
        Exiger(adresse.has_value(),
               "export du classificateur Gs++ absent");

        using Classificateur =
            std::uint32_t (GS_ABI_HOTE *)(const char*, std::uint64_t);
        const auto classifier = reinterpret_cast<Classificateur>(*adresse);

        const std::array<std::string_view, 83> mots{{
            "espace", "namespace", "structure", "struct", "union",
            "énumération", "enumeration", "enum", "alias",
            "externe", "extern", "publique", "public", "privée",
            "private", "constante", "const", "volatile", "entier8",
            "int8", "entier16", "int16", "entier32", "int32",
            "entier64", "int64", "naturel8", "uint8", "naturel16",
            "uint16", "naturel32", "uint32", "naturel64", "uint64",
            "booléen", "booleen", "bool", "octet", "byte",
            "caractère", "caractere", "char", "vide", "void",
            "pointeur_fonction", "function_pointer", "convertir",
            "cast", "retourner", "return", "si", "if", "sinon",
            "else", "tantque", "while", "vrai", "true", "faux",
            "false", "classe", "class", "protégée", "protegee",
            "protected", "virtuel", "virtual", "constructeur",
            "constructor", "destructeur", "destructor", "opérateur",
            "operateur", "operator", "soi", "this", "remplacer",
            "override", "parent", "super", "identifiant",
            "publiqueX", "étoile"
        }};
        for (const auto mot : mots)
        {
            std::string copie(mot);
            const auto reference = static_cast<std::uint32_t>(
                GsPP::ClassifierMotCle(mot));
            const auto resultat = classifier(
                copie.data(), static_cast<std::uint64_t>(copie.size()));
            Exiger(
                resultat == reference,
                "classification différente pour « " + copie
                    + " » : C++=" + std::to_string(reference)
                    + ", Gs++=" + std::to_string(resultat));
        }
    }

    void TesterBibliothequeHebergee(const std::string& chemin)
    {
        CheminLu.clear();
        CheminEcrit.clear();
        CheminsEcrits.clear();
        DonneesEcrites.clear();
        AllocationsActives.clear();
        NombreAllocations = 0;
        NombreLiberations = 0;
        LiberationInvalide = false;
        EchecTransactionTeste = false;

        const auto contenu = LireFichier(chemin);
        const auto tailleImage = Lire64(contenu, 48);
        const auto debutTrampolines = AlignerPage(tailleImage);
        ZoneExecutable zone(debutTrampolines + 4096);

        const auto allouer = zone.AjouterTrampoline(
            debutTrampolines,
            reinterpret_cast<std::uintptr_t>(&AllouerMemoireHote));
        const auto liberer = zone.AjouterTrampoline(
            debutTrampolines + 16,
            reinterpret_cast<std::uintptr_t>(&LibererMemoireHote));
        const auto lire = zone.AjouterTrampoline(
            debutTrampolines + 32,
            reinterpret_cast<std::uintptr_t>(&LireFichierHote));
        const auto ecrire = zone.AjouterTrampoline(
            debutTrampolines + 48,
            reinterpret_cast<std::uintptr_t>(&EcrireFichierHote));
        const auto diagnostiquer = zone.AjouterTrampoline(
            debutTrampolines + 64,
            reinterpret_cast<std::uintptr_t>(&EmettreDiagnosticHote));

        const auto resolveur =
            [&](std::string_view nom) -> std::optional<std::uint64_t>
        {
            if (nom == "Gs::Hote::AllouerMemoire") return allouer;
            if (nom == "Gs::Hote::LibererMemoire") return liberer;
            if (nom == "Gs::Hote::LireFichier") return lire;
            if (nom == "Gs::Hote::EcrireFichier") return ecrire;
            if (nom == "Gs::Hote::EmettreDiagnostic")
                return diagnostiquer;
            return std::nullopt;
        };
        const auto image = GsPP::ChargeurGsE().Charger(
            contenu, zone.Base(), resolveur);
        zone.Copier(image.Memoire);

        using TestHeberge = std::int32_t (GS_ABI_HOTE *)();
        const auto tester = reinterpret_cast<TestHeberge>(
            image.AdressePointEntree);
        Exiger(tester() == 260,
               "le test exécutable de la bibliothèque hébergée a échoué");
        Exiger(CheminLu == "source.GsPP",
               "chemin de lecture hébergé incorrect");
        Exiger(
            CheminsEcrits.size() == 2
                && CheminsEcrits[0] == "sortie.GsObj"
                && CheminsEcrits[1] == "sortie-allouee.GsObj"
                && CheminEcrit == "sortie-allouee.GsObj",
            "chemins d’écriture hébergés incorrects");
        const std::vector<std::uint8_t> attendu{
            'G', 's', '+', '+', '\n', '!'};
        Exiger(DonneesEcrites == attendu,
               "le flux hébergé a altéré les octets");
        Exiger(
            NiveauDiagnostic == 2
                && LigneDiagnostic == 7
                && ColonneDiagnostic == 9,
            "position du diagnostic hébergé incorrecte");
        Exiger(FichierDiagnostic == "auto.GsPP",
               "fichier du diagnostic hébergé incorrect");
        Exiger(MessageDiagnostic == "diagnostic auto-hébergé",
               "message du diagnostic hébergé incorrect");
        Exiger(EchecTransactionTeste,
               "l’échec d’allocation transactionnel n’a pas été exercé");
        Exiger(!LiberationInvalide,
               "la bibliothèque a tenté une libération invalide");
        Exiger(
            AllocationsActives.empty()
                && NombreAllocations == NombreLiberations
                && NombreAllocations != 0,
            "la bibliothèque hébergée laisse une allocation active");
    }
}

int main(int argc, char** argv)
{
    try
    {
        if (argc != 4)
            throw std::runtime_error(
                "trois images GsE de test sont attendues");
        TesterClassificateur(argv[1]);
        TesterLexeur(argv[2]);
        TesterBibliothequeHebergee(argv[3]);
        std::cout
            << "Auto-hébergement 0.27 : "
            << "83 classifications, lexeur différentiel "
            << "et bibliothèque hébergée validés.\n";
        return 0;
    }
    catch (const std::exception& erreur)
    {
        std::cerr << "Échec auto-hébergement : "
                  << erreur.what() << '\n';
        return 1;
    }
}
