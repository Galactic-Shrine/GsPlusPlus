#include "GsPP/ChargeurGsE.hpp"
#include "GsPP/AnalyseurSemantique.hpp"
#include "GsPP/AnalyseurSyntaxique.hpp"
#include "GsPP/ErreurCompilation.hpp"
#include "GsPP/Lexeur.hpp"

#include <algorithm>
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

    struct NoeudDeclarationHote
    {
        std::uint32_t Genre;
        std::uint32_t Ligne;
        std::uint32_t Colonne;
        std::uint32_t Drapeaux;
        std::uint64_t Parent;
        std::uint64_t DebutNom;
        std::uint64_t TailleNom;
        std::uint64_t HachageNom;
        std::uint64_t HachageEspace;
        std::uint64_t HachageType;
    };

    struct ResultatAnalyseDeclarationsHote
    {
        std::uint32_t Erreur;
        std::uint32_t LigneErreur;
        std::uint32_t ColonneErreur;
        std::uint32_t Detail;
        std::uint64_t NombreNoeuds;
        std::uint64_t CapaciteRequise;
        std::uint64_t NombreOctetsArene;
        std::uint64_t Reserve;
    };

    struct RequeteAnalyseDeclarationsHote
    {
        const char* Source;
        std::uint64_t Taille;
        NoeudDeclarationHote* Noeuds;
        std::uint64_t Capacite;
        ResultatAnalyseDeclarationsHote Resultat;
    };

    struct SymboleSemantiqueHote
    {
        std::uint64_t IndexNoeud;
        std::uint64_t IndexPortee;
        std::uint64_t HachageNom;
        std::uint64_t HachageEspace;
        std::uint64_t HachageType;
        std::uint32_t Genre;
        std::uint32_t Drapeaux;
    };

    struct ResolutionSemantiqueHote
    {
        std::uint64_t IndexNoeud;
        std::uint64_t IndexSymbole;
        std::uint64_t HachageType;
        std::uint32_t GenreCible;
        std::uint32_t Drapeaux;
    };

    struct ResultatAnalyseSemantiqueHote
    {
        std::uint32_t Erreur;
        std::uint32_t LigneErreur;
        std::uint32_t ColonneErreur;
        std::uint32_t Detail;
        std::uint64_t NombreSymboles;
        std::uint64_t CapaciteSymbolesRequise;
        std::uint64_t NombreResolutions;
        std::uint64_t CapaciteResolutionsRequise;
        std::uint64_t NombreOctetsArene;
    };

    struct RequeteAnalyseSemantiqueHote
    {
        const char* Source;
        std::uint64_t TailleSource;
        const NoeudDeclarationHote* Noeuds;
        std::uint64_t NombreNoeuds;
        SymboleSemantiqueHote* Symboles;
        std::uint64_t CapaciteSymboles;
        ResolutionSemantiqueHote* Resolutions;
        std::uint64_t CapaciteResolutions;
        ResultatAnalyseSemantiqueHote Resultat;
    };

    static_assert(sizeof(VueTexteHote) == 16);
    static_assert(sizeof(RequeteFichierHote) == 32);
    static_assert(sizeof(DiagnosticHote) == 48);
    static_assert(sizeof(JetonLexeHote) == 48);
    static_assert(sizeof(ResultatLexageHote) == 32);
    static_assert(sizeof(RequeteLexageHote) == 64);
    static_assert(sizeof(NoeudDeclarationHote) == 64);
    static_assert(sizeof(ResultatAnalyseDeclarationsHote) == 48);
    static_assert(sizeof(RequeteAnalyseDeclarationsHote) == 80);
    static_assert(sizeof(SymboleSemantiqueHote) == 48);
    static_assert(sizeof(ResolutionSemantiqueHote) == 32);
    static_assert(sizeof(ResultatAnalyseSemantiqueHote) == 56);
    static_assert(sizeof(RequeteAnalyseSemantiqueHote) == 120);

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

    std::uint64_t AjouterNaturel32Empreinte(
        std::uint64_t hachage,
        std::uint32_t valeur)
    {
        for (unsigned index = 0; index < 4; ++index)
        {
            hachage ^= static_cast<std::uint8_t>(valeur & 0xFFU);
            hachage *= 1'099'511'628'211ULL;
            valeur >>= 8U;
        }
        return hachage;
    }

    std::uint64_t AjouterNaturel64Empreinte(
        std::uint64_t hachage,
        std::uint64_t valeur)
    {
        for (unsigned index = 0; index < 8; ++index)
        {
            hachage ^= static_cast<std::uint8_t>(valeur & 0xFFULL);
            hachage *= 1'099'511'628'211ULL;
            valeur >>= 8U;
        }
        return hachage;
    }

    std::uint32_t CodeTypeDeclaration(const GsPP::TypeGs& type)
    {
        switch (type.Genre)
        {
            case GsPP::GenreType::Entier8: return 1;
            case GsPP::GenreType::Entier16: return 2;
            case GsPP::GenreType::Entier32: return 3;
            case GsPP::GenreType::Entier64: return 4;
            case GsPP::GenreType::Naturel8: return 5;
            case GsPP::GenreType::Naturel16: return 6;
            case GsPP::GenreType::Naturel32: return 7;
            case GsPP::GenreType::Naturel64: return 8;
            case GsPP::GenreType::Booleen: return 9;
            case GsPP::GenreType::Octet: return 10;
            case GsPP::GenreType::Caractere: return 11;
            case GsPP::GenreType::Vide: return 12;
            case GsPP::GenreType::Structure: return 13;
            case GsPP::GenreType::PointeurFonction: return 14;
            default:
                throw std::runtime_error(
                    "type absent de la tranche AST des déclarations");
        }
    }

    std::uint64_t HacherTypeDeclaration(const GsPP::TypeGs& type)
    {
        std::uint32_t qualificatifs = 0;
        if (type.EstConstante) qualificatifs |= 1U;
        if (type.EstVolatile) qualificatifs |= 2U;
        if (type.EstReference) qualificatifs |= 4U;

        std::uint64_t dimensions = HacherTexte({});
        for (const auto dimension : type.DimensionsTableau)
            dimensions = AjouterNaturel64Empreinte(dimensions, dimension);

        std::uint64_t hachageNom = HacherTexte({});
        if (type.Genre == GsPP::GenreType::Structure)
            hachageNom = HacherTexte(type.Nom);
        else if (type.Genre == GsPP::GenreType::PointeurFonction)
        {
            if (!type.RetourFonction)
                throw std::runtime_error(
                    "retour absent du pointeur de fonction compact");
            hachageNom = AjouterNaturel64Empreinte(
                hachageNom,
                HacherTypeDeclaration(*type.RetourFonction));
            for (const auto& parametre : type.ParametresFonction)
                hachageNom = AjouterNaturel64Empreinte(
                    hachageNom,
                    HacherTypeDeclaration(parametre));
            hachageNom = AjouterNaturel32Empreinte(
                hachageNom,
                static_cast<std::uint32_t>(
                    type.ParametresFonction.size()));
        }

        std::uint64_t hachage = HacherTexte({});
        hachage = AjouterNaturel32Empreinte(
            hachage, CodeTypeDeclaration(type));
        hachage = AjouterNaturel32Empreinte(hachage, qualificatifs);
        hachage = AjouterNaturel32Empreinte(
            hachage, type.NiveauPointeur);
        hachage = AjouterNaturel32Empreinte(
            hachage,
            static_cast<std::uint32_t>(type.DimensionsTableau.size()));
        hachage = AjouterNaturel64Empreinte(
            hachage,
            hachageNom);
        hachage = AjouterNaturel64Empreinte(hachage, dimensions);
        return hachage;
    }

    void AjouterExpressionReference(
        const GsPP::Expression& expression,
        std::size_t parent,
        std::uint64_t hachageEspace,
        std::vector<NoeudDeclarationHote>& resultat)
    {
        const auto ligne = static_cast<std::uint32_t>(
            expression.Position.Ligne);
        const auto colonne = static_cast<std::uint32_t>(
            expression.Position.Colonne);
        const auto ajouter = [&](
            std::uint32_t genre,
            std::uint32_t drapeaux = 0,
            std::uint64_t hachageNom = 0,
            std::uint64_t hachageType = 0)
        {
            const auto index = resultat.size();
            resultat.push_back({
                genre, ligne, colonne, drapeaux, parent,
                0, 0, hachageNom, hachageEspace, hachageType});
            return index;
        };

        switch (expression.Genre)
        {
            case GsPP::GenreExpression::Entier:
            {
                const auto& entier = static_cast<
                    const GsPP::ExpressionEntier&>(expression);
                ajouter(
                    22,
                    entier.EstLitteralBooleen ? 16'384U : 0U,
                    0,
                    entier.Valeur);
                return;
            }
            case GsPP::GenreExpression::Chaine:
            {
                const auto& chaine = static_cast<
                    const GsPP::ExpressionChaine&>(expression);
                ajouter(23, 0, HacherTexte(chaine.Valeur));
                return;
            }
            case GsPP::GenreExpression::Variable:
            {
                const auto& variable = static_cast<
                    const GsPP::ExpressionVariable&>(expression);
                ajouter(
                    24,
                    variable.EstBase ? 65'536U : 0U,
                    HacherTexte(variable.Nom));
                return;
            }
            case GsPP::GenreExpression::Unaire:
            {
                const auto& unaire = static_cast<
                    const GsPP::ExpressionUnaire&>(expression);
                const auto index = ajouter(
                    25, 0, HacherTexte(unaire.Operateur));
                AjouterExpressionReference(
                    *unaire.Operande, index, hachageEspace, resultat);
                return;
            }
            case GsPP::GenreExpression::Binaire:
            {
                const auto& binaire = static_cast<
                    const GsPP::ExpressionBinaire&>(expression);
                const auto index = ajouter(
                    26, 0, HacherTexte(binaire.Operateur));
                AjouterExpressionReference(
                    *binaire.Gauche, index, hachageEspace, resultat);
                AjouterExpressionReference(
                    *binaire.Droite, index, hachageEspace, resultat);
                return;
            }
            case GsPP::GenreExpression::Affectation:
            {
                const auto& affectation = static_cast<
                    const GsPP::ExpressionAffectation&>(expression);
                const auto index = ajouter(27);
                AjouterExpressionReference(
                    *affectation.Cible, index, hachageEspace, resultat);
                AjouterExpressionReference(
                    *affectation.Valeur, index, hachageEspace, resultat);
                return;
            }
            case GsPP::GenreExpression::Appel:
            {
                const auto& appel = static_cast<
                    const GsPP::ExpressionAppel&>(expression);
                const auto index = ajouter(28);
                AjouterExpressionReference(
                    *appel.Cible, index, hachageEspace, resultat);
                for (const auto& argument : appel.Arguments)
                    AjouterExpressionReference(
                        *argument, index, hachageEspace, resultat);
                return;
            }
            case GsPP::GenreExpression::Membre:
            {
                const auto& membre = static_cast<
                    const GsPP::ExpressionMembre&>(expression);
                const auto index = ajouter(
                    29,
                    membre.ViaPointeur ? 32'768U : 0U,
                    HacherTexte(membre.Membre));
                AjouterExpressionReference(
                    *membre.Objet, index, hachageEspace, resultat);
                return;
            }
            case GsPP::GenreExpression::Index:
            {
                const auto& indexation = static_cast<
                    const GsPP::ExpressionIndex&>(expression);
                const auto index = ajouter(30);
                AjouterExpressionReference(
                    *indexation.Objet, index, hachageEspace, resultat);
                AjouterExpressionReference(
                    *indexation.Indice, index, hachageEspace, resultat);
                return;
            }
            case GsPP::GenreExpression::Conversion:
            {
                const auto& conversion = static_cast<
                    const GsPP::ExpressionConversion&>(expression);
                const auto index = ajouter(
                    31, 0, 0, HacherTypeDeclaration(conversion.TypeCible));
                AjouterExpressionReference(
                    *conversion.Valeur, index, hachageEspace, resultat);
                return;
            }
            case GsPP::GenreExpression::Agregat:
            {
                const auto& agregat = static_cast<
                    const GsPP::ExpressionAgregat&>(expression);
                const auto index = ajouter(32);
                for (const auto& element : agregat.Elements)
                    AjouterExpressionReference(
                        *element, index, hachageEspace, resultat);
                return;
            }
        }
        throw std::runtime_error(
            "expression absente de la tranche AST auto-hébergée");
    }

    void AjouterInstructionReference(
        const GsPP::Instruction& instruction,
        std::size_t parent,
        std::uint64_t hachageEspace,
        std::vector<NoeudDeclarationHote>& resultat)
    {
        const auto ligne = static_cast<std::uint32_t>(
            instruction.Position.Ligne);
        const auto colonne = static_cast<std::uint32_t>(
            instruction.Position.Colonne);
        switch (instruction.Genre)
        {
            case GsPP::GenreInstruction::Bloc:
            {
                const auto indexBloc = resultat.size();
                resultat.push_back({
                    16, ligne, colonne, 0, parent,
                    0, 0, 0, hachageEspace, 0});
                const auto& bloc = static_cast<
                    const GsPP::InstructionBloc&>(instruction);
                for (const auto& enfant : bloc.Instructions)
                    AjouterInstructionReference(
                        *enfant, indexBloc, hachageEspace, resultat);
                return;
            }
            case GsPP::GenreInstruction::Retour:
            {
                const auto& retour = static_cast<
                    const GsPP::InstructionRetour&>(instruction);
                const auto indexRetour = resultat.size();
                resultat.push_back({
                    17, ligne, colonne, retour.Valeur ? 2048U : 0U,
                    parent, 0, 0, 0, hachageEspace, 0});
                if (retour.Valeur)
                    AjouterExpressionReference(
                        *retour.Valeur,
                        indexRetour,
                        hachageEspace,
                        resultat);
                return;
            }
            case GsPP::GenreInstruction::Expression:
            {
                const auto& instructionExpression = static_cast<
                    const GsPP::InstructionExpression&>(instruction);
                const auto indexExpression = resultat.size();
                resultat.push_back({
                    18, ligne, colonne, 2048U, parent,
                    0, 0, 0, hachageEspace, 0});
                AjouterExpressionReference(
                    *instructionExpression.Valeur,
                    indexExpression,
                    hachageEspace,
                    resultat);
                return;
            }
            case GsPP::GenreInstruction::Variable:
            {
                const auto& variable = static_cast<
                    const GsPP::InstructionVariable&>(instruction);
                std::uint32_t drapeaux = 0;
                if (variable.Initialiseur) drapeaux |= 4U | 2048U;
                if (variable.ConstructionExplicite) drapeaux |= 8192U;
                const auto indexVariable = resultat.size();
                resultat.push_back({
                    19, ligne, colonne, drapeaux, parent,
                    0, 0, HacherTexte(variable.Nom), hachageEspace,
                    HacherTypeDeclaration(variable.Type)});
                if (variable.Initialiseur)
                    AjouterExpressionReference(
                        *variable.Initialiseur,
                        indexVariable,
                        hachageEspace,
                        resultat);
                else
                    for (const auto& argument : variable.ArgumentsConstruction)
                        AjouterExpressionReference(
                            *argument,
                            indexVariable,
                            hachageEspace,
                            resultat);
                return;
            }
            case GsPP::GenreInstruction::Si:
            {
                const auto& conditionnelle = static_cast<
                    const GsPP::InstructionSi&>(instruction);
                const auto indexConditionnelle = resultat.size();
                resultat.push_back({
                    20, ligne, colonne,
                    2048U | (conditionnelle.Sinon ? 4096U : 0U),
                    parent, 0, 0, 0, hachageEspace, 0});
                AjouterExpressionReference(
                    *conditionnelle.Condition,
                    indexConditionnelle,
                    hachageEspace,
                    resultat);
                AjouterInstructionReference(
                    *conditionnelle.Alors,
                    indexConditionnelle,
                    hachageEspace,
                    resultat);
                if (conditionnelle.Sinon)
                    AjouterInstructionReference(
                        *conditionnelle.Sinon,
                        indexConditionnelle,
                        hachageEspace,
                        resultat);
                return;
            }
            case GsPP::GenreInstruction::TantQue:
            {
                const auto& boucle = static_cast<
                    const GsPP::InstructionTantQue&>(instruction);
                const auto indexBoucle = resultat.size();
                resultat.push_back({
                    21, ligne, colonne, 2048U, parent,
                    0, 0, 0, hachageEspace, 0});
                AjouterExpressionReference(
                    *boucle.Condition,
                    indexBoucle,
                    hachageEspace,
                    resultat);
                AjouterInstructionReference(
                    *boucle.Corps, indexBoucle, hachageEspace, resultat);
                return;
            }
        }
        throw std::runtime_error(
            "instruction absente de la tranche AST auto-hébergée");
    }

    std::vector<NoeudDeclarationHote> ConstruireDeclarationsReference(
        const GsPP::Programme& programme)
    {
        enum class GenreRacine
        {
            Structure,
            Enumeration,
            VariableGlobale,
            Fonction,
            Alias
        };
        struct Racine
        {
            GenreRacine Genre;
            const void* Declaration;
            std::size_t Ligne;
            std::size_t Colonne;
        };
        std::vector<Racine> racines;
        for (const auto& structure : programme.Structures)
            racines.push_back({
                GenreRacine::Structure,
                &structure,
                structure.Position.Ligne,
                structure.Position.Colonne});
        for (const auto& enumeration : programme.Enumerations)
            racines.push_back({
                GenreRacine::Enumeration,
                &enumeration,
                enumeration.Position.Ligne,
                enumeration.Position.Colonne});
        for (const auto& variable : programme.VariablesGlobales)
            racines.push_back({
                GenreRacine::VariableGlobale,
                &variable,
                variable.Position.Ligne,
                variable.Position.Colonne});
        for (const auto& fonction : programme.Fonctions)
            if (!fonction.EstMethode)
                racines.push_back({
                    GenreRacine::Fonction,
                    &fonction,
                    fonction.Position.Ligne,
                    fonction.Position.Colonne});
        for (const auto& alias : programme.Aliases)
            racines.push_back({
                GenreRacine::Alias,
                &alias,
                alias.Position.Ligne,
                alias.Position.Colonne});
        std::stable_sort(
            racines.begin(), racines.end(),
            [](const Racine& gauche, const Racine& droite)
            {
                if (gauche.Ligne != droite.Ligne)
                    return gauche.Ligne < droite.Ligne;
                return gauche.Colonne < droite.Colonne;
            });

        std::vector<NoeudDeclarationHote> resultat;
        resultat.push_back({
            0, 1, 1, 0, 0, 0, 0, 0, HacherTexte({}), 0});
        for (const auto& racine : racines)
        {
            if (racine.Genre == GenreRacine::Structure)
            {
                const auto& structure = *static_cast<const GsPP::Structure*>(
                    racine.Declaration);
                const auto indexStructure = resultat.size();
                std::uint32_t genre = 4;
                if (structure.EstUnion) genre = 5;
                else if (structure.EstClasse) genre = 6;
                std::uint32_t drapeaux = 4U;
                std::uint64_t hachageBase = 0;
                if (!structure.ClasseBase.empty())
                {
                    drapeaux |= 32U;
                    if (structure.VisibiliteHeritage
                        == GsPP::VisibiliteMembre::Publique)
                        drapeaux |= 1U;
                    else if (structure.VisibiliteHeritage
                        == GsPP::VisibiliteMembre::Protegee)
                        drapeaux |= 8U;
                    else
                        drapeaux |= 16U;
                    hachageBase = HacherTexte(structure.ClasseBase);
                }
                resultat.push_back({
                    genre,
                    static_cast<std::uint32_t>(structure.Position.Ligne),
                    static_cast<std::uint32_t>(structure.Position.Colonne),
                    drapeaux,
                    0,
                    0,
                    0,
                    HacherTexte(structure.Nom),
                    HacherTexte(structure.Espace),
                    hachageBase});

                enum class GenreMembre
                {
                    Champ,
                    Alias,
                    Fonction
                };
                struct Membre
                {
                    GenreMembre Genre;
                    const void* Declaration;
                    std::size_t Ligne;
                    std::size_t Colonne;
                };
                std::vector<Membre> membres;
                for (const auto& champ : structure.Champs)
                    membres.push_back({
                        GenreMembre::Champ, &champ,
                        champ.Position.Ligne, champ.Position.Colonne});
                for (const auto& alias : structure.AliasesChamps)
                    membres.push_back({
                        GenreMembre::Alias, &alias,
                        alias.Position.Ligne, alias.Position.Colonne});
                for (const auto& fonction : programme.Fonctions)
                    if (fonction.EstMethode
                        && fonction.ClasseProprietaire
                            == structure.NomComplet())
                        membres.push_back({
                            GenreMembre::Fonction,
                            &fonction,
                            fonction.Position.Ligne,
                            fonction.Position.Colonne});
                std::stable_sort(
                    membres.begin(), membres.end(),
                    [](const Membre& gauche, const Membre& droite)
                    {
                        if (gauche.Ligne != droite.Ligne)
                            return gauche.Ligne < droite.Ligne;
                        return gauche.Colonne < droite.Colonne;
                });
                for (const auto& membre : membres)
                {
                    if (membre.Genre == GenreMembre::Alias)
                    {
                        const auto& alias =
                            *static_cast<const GsPP::AliasChamp*>(
                                membre.Declaration);
                        resultat.push_back({
                            11,
                            static_cast<std::uint32_t>(alias.Position.Ligne),
                            static_cast<std::uint32_t>(alias.Position.Colonne),
                            0,
                            indexStructure,
                            0,
                            0,
                            HacherTexte(alias.Nom),
                            HacherTexte(structure.Espace),
                            HacherTexte(alias.Cible)});
                        continue;
                    }
                    if (membre.Genre == GenreMembre::Fonction)
                    {
                        const auto& fonction =
                            *static_cast<const GsPP::Fonction*>(
                                membre.Declaration);
                        const auto indexFonction = resultat.size();
                        std::uint32_t genreFonction = 12;
                        std::uint64_t hachageNom =
                            HacherTexte(fonction.Nom);
                        if (fonction.EstConstructeur)
                        {
                            genreFonction = 13;
                            hachageNom = HacherTexte(structure.Nom);
                        }
                        else if (fonction.EstDestructeur)
                        {
                            genreFonction = 14;
                            hachageNom = HacherTexte(structure.Nom);
                        }
                        else if (fonction.EstOperateur)
                        {
                            genreFonction = 15;
                            hachageNom = HacherTexte(fonction.Operateur);
                        }

                        std::uint32_t drapeauxFonction = 0;
                        if (fonction.Visibilite
                            == GsPP::VisibiliteMembre::Publique)
                            drapeauxFonction |= 1U;
                        else if (fonction.Visibilite
                            == GsPP::VisibiliteMembre::Protegee)
                            drapeauxFonction |= 8U;
                        else
                            drapeauxFonction |= 16U;
                        if (fonction.Corps) drapeauxFonction |= 4U;
                        if (fonction.EstVirtuelle) drapeauxFonction |= 64U;
                        if (fonction.EstRemplacement)
                            drapeauxFonction |= 128U;
                        if (fonction.InitialiseurBaseExplicite)
                            drapeauxFonction |= 256U;
                        if (fonction.DelegueConstructeur)
                            drapeauxFonction |= 512U;
                        if (!fonction.InitialiseursChamps.empty())
                            drapeauxFonction |= 1024U;

                        resultat.push_back({
                            genreFonction,
                            static_cast<std::uint32_t>(
                                fonction.Position.Ligne),
                            static_cast<std::uint32_t>(
                                fonction.Position.Colonne),
                            drapeauxFonction,
                            indexStructure,
                            0,
                            0,
                            hachageNom,
                            HacherTexte(structure.Espace),
                            HacherTypeDeclaration(fonction.TypeRetour)});
                         for (std::size_t indexParametre = 1;
                              indexParametre < fonction.Parametres.size();
                             ++indexParametre)
                        {
                            const auto& parametre =
                                fonction.Parametres[indexParametre];
                            resultat.push_back({
                                2,
                                static_cast<std::uint32_t>(
                                    parametre.Position.Ligne),
                                static_cast<std::uint32_t>(
                                    parametre.Position.Colonne),
                                0,
                                indexFonction,
                                0,
                                0,
                                HacherTexte(parametre.Nom),
                                 HacherTexte(structure.Espace),
                                 HacherTypeDeclaration(parametre.Type)});
                         }
                        if (fonction.DelegueConstructeur)
                        {
                            const auto indexInitialiseur = resultat.size();
                            resultat.push_back({
                                33,
                                static_cast<std::uint32_t>(
                                    fonction.Position.Ligne),
                                static_cast<std::uint32_t>(
                                    fonction.Position.Colonne),
                                0,
                                indexFonction,
                                0,
                                0,
                                0,
                                HacherTexte(structure.Espace),
                                0});
                            for (const auto& argument :
                                 fonction.ArgumentsConstructeurDelegue)
                                AjouterExpressionReference(
                                    *argument,
                                    indexInitialiseur,
                                    HacherTexte(structure.Espace),
                                    resultat);
                        }
                        else
                        {
                            if (fonction.InitialiseurBaseExplicite)
                            {
                                const auto indexInitialiseur = resultat.size();
                                resultat.push_back({
                                    34,
                                    static_cast<std::uint32_t>(
                                        fonction.Position.Ligne),
                                    static_cast<std::uint32_t>(
                                        fonction.Position.Colonne),
                                    0,
                                    indexFonction,
                                    0,
                                    0,
                                    0,
                                    HacherTexte(structure.Espace),
                                    0});
                                for (const auto& argument :
                                     fonction.ArgumentsConstructeurBase)
                                    AjouterExpressionReference(
                                        *argument,
                                        indexInitialiseur,
                                        HacherTexte(structure.Espace),
                                        resultat);
                            }
                            for (const auto& initialiseur :
                                 fonction.InitialiseursChamps)
                            {
                                const auto indexInitialiseur = resultat.size();
                                resultat.push_back({
                                    35,
                                    static_cast<std::uint32_t>(
                                        initialiseur.Position.Ligne),
                                    static_cast<std::uint32_t>(
                                        initialiseur.Position.Colonne),
                                    0,
                                    indexFonction,
                                    0,
                                    0,
                                    HacherTexte(initialiseur.Nom),
                                    HacherTexte(structure.Espace),
                                    0});
                                for (const auto& argument :
                                     initialiseur.Arguments)
                                    AjouterExpressionReference(
                                        *argument,
                                        indexInitialiseur,
                                        HacherTexte(structure.Espace),
                                        resultat);
                            }
                        }
                         if (fonction.Corps)
                             AjouterInstructionReference(
                                *fonction.Corps,
                                indexFonction,
                                HacherTexte(structure.Espace),
                                resultat);
                        continue;
                    }
                    const auto& champ =
                        *static_cast<const GsPP::ChampStructure*>(
                            membre.Declaration);
                    std::uint32_t drapeauxChamp = 0;
                    if (champ.Visibilite
                        == GsPP::VisibiliteMembre::Publique)
                        drapeauxChamp |= 1U;
                    else if (champ.Visibilite
                        == GsPP::VisibiliteMembre::Protegee)
                        drapeauxChamp |= 8U;
                    else
                        drapeauxChamp |= 16U;
                    if (champ.InitialiseurParDefaut) drapeauxChamp |= 4U;
                    const auto indexChamp = resultat.size();
                    resultat.push_back({
                        7,
                        static_cast<std::uint32_t>(champ.Position.Ligne),
                        static_cast<std::uint32_t>(champ.Position.Colonne),
                        drapeauxChamp,
                        indexStructure,
                        0,
                        0,
                        HacherTexte(champ.Nom),
                        HacherTexte(structure.Espace),
                        HacherTypeDeclaration(champ.Type)});
                    if (champ.InitialiseurParDefaut)
                        AjouterExpressionReference(
                            *champ.InitialiseurParDefaut,
                            indexChamp,
                            HacherTexte(structure.Espace),
                            resultat);
                }
                continue;
            }
            if (racine.Genre == GenreRacine::Enumeration)
            {
                const auto& enumeration =
                    *static_cast<const GsPP::Enumeration*>(
                        racine.Declaration);
                const auto indexEnumeration = resultat.size();
                resultat.push_back({
                    8,
                    static_cast<std::uint32_t>(enumeration.Position.Ligne),
                    static_cast<std::uint32_t>(enumeration.Position.Colonne),
                    4,
                    0,
                    0,
                    0,
                    HacherTexte(enumeration.Nom),
                    HacherTexte(enumeration.Espace),
                    0});
                for (const auto& valeur : enumeration.Valeurs)
                {
                    const auto indexEnumerateur = resultat.size();
                    resultat.push_back({
                        9,
                        static_cast<std::uint32_t>(valeur.Position.Ligne),
                        static_cast<std::uint32_t>(valeur.Position.Colonne),
                        valeur.Initialiseur ? 4U : 0U,
                        indexEnumeration,
                        0,
                        0,
                        HacherTexte(valeur.Nom),
                        HacherTexte(enumeration.Espace),
                        0});
                    if (valeur.Initialiseur)
                        AjouterExpressionReference(
                            *valeur.Initialiseur,
                            indexEnumerateur,
                            HacherTexte(enumeration.Espace),
                            resultat);
                }
                continue;
            }
            if (racine.Genre == GenreRacine::VariableGlobale)
            {
                const auto& variable =
                    *static_cast<const GsPP::VariableGlobale*>(
                        racine.Declaration);
                std::uint32_t drapeaux = 0;
                if (variable.EstPublique) drapeaux |= 1U;
                if (variable.EstExterne) drapeaux |= 2U;
                if (variable.Initialiseur) drapeaux |= 4U;
                const auto indexVariable = resultat.size();
                resultat.push_back({
                    3,
                    static_cast<std::uint32_t>(variable.Position.Ligne),
                    static_cast<std::uint32_t>(variable.Position.Colonne),
                    drapeaux,
                    0,
                    0,
                    0,
                    HacherTexte(variable.Nom),
                    HacherTexte(variable.Espace),
                    HacherTypeDeclaration(variable.Type)});
                if (variable.Initialiseur)
                    AjouterExpressionReference(
                        *variable.Initialiseur,
                        indexVariable,
                        HacherTexte(variable.Espace),
                        resultat);
                continue;
            }
            if (racine.Genre == GenreRacine::Fonction)
            {
                const auto& fonction = *static_cast<const GsPP::Fonction*>(
                    racine.Declaration);
                const auto indexFonction = resultat.size();
                const auto genreFonction = fonction.EstOperateur ? 15U : 1U;
                const auto hachageNom = fonction.EstOperateur
                    ? HacherTexte(fonction.Operateur)
                    : HacherTexte(fonction.Nom);
                std::uint32_t drapeaux = 0;
                if (fonction.EstPublique) drapeaux |= 1U;
                if (fonction.EstExterne) drapeaux |= 2U;
                if (fonction.Corps) drapeaux |= 4U;
                resultat.push_back({
                    genreFonction,
                    static_cast<std::uint32_t>(fonction.Position.Ligne),
                    static_cast<std::uint32_t>(fonction.Position.Colonne),
                    drapeaux,
                    0,
                    0,
                    0,
                    hachageNom,
                    HacherTexte(fonction.Espace),
                    HacherTypeDeclaration(fonction.TypeRetour)});
                for (const auto& parametre : fonction.Parametres)
                    resultat.push_back({
                        2,
                        static_cast<std::uint32_t>(parametre.Position.Ligne),
                        static_cast<std::uint32_t>(parametre.Position.Colonne),
                        0,
                        indexFonction,
                        0,
                        0,
                        HacherTexte(parametre.Nom),
                        HacherTexte(fonction.Espace),
                        HacherTypeDeclaration(parametre.Type)});
                if (fonction.Corps)
                    AjouterInstructionReference(
                        *fonction.Corps,
                        indexFonction,
                        HacherTexte(fonction.Espace),
                        resultat);
                continue;
            }
            const auto& alias = *static_cast<const GsPP::DeclarationAlias*>(
                racine.Declaration);
            resultat.push_back({
                10,
                static_cast<std::uint32_t>(alias.Position.Ligne),
                static_cast<std::uint32_t>(alias.Position.Colonne),
                0,
                0,
                0,
                0,
                HacherTexte(alias.Nom),
                HacherTexte(alias.Espace),
                HacherTexte(alias.Cible)});
        }
        return resultat;
    }

    bool MemeStructureDeclaration(
        const NoeudDeclarationHote& gauche,
        const NoeudDeclarationHote& droite)
    {
        return gauche.Genre == droite.Genre
            && gauche.Drapeaux == droite.Drapeaux
            && gauche.Parent == droite.Parent
            && gauche.HachageNom == droite.HachageNom
            && gauche.HachageEspace == droite.HachageEspace
            && gauche.HachageType == droite.HachageType;
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
            if (nom == "GalacticShrine::GsPP::Hote::AllouerMemoire") return allouer;
            if (nom == "GalacticShrine::GsPP::Hote::LibererMemoire") return liberer;
            return std::nullopt;
        };
        const auto image = GsPP::ChargeurGsE().Charger(
            contenu, zone.Base(), resolveur);
        zone.Copier(image.Memoire);

        const auto adresse = image.ChercherExport(
            "GalacticShrine::GsPP::Autohebergement::AnalyserSource");
        Exiger(adresse.has_value(), "export du lexeur Gs++ absent");
        const auto lexer = reinterpret_cast<LexeurAutoHeberge>(*adresse);

        const std::vector<std::pair<std::string, std::string>> corpus{
            {"vide", ""},
            {"programme", "espace Démonstration { publique entier32 Principal() { naturel64 valeur_1 = 12_345; si (valeur_1 >= 42 && vrai || faux) retourner 7; } }"},
            {"commentaires", "// ligne ignorée\n/**\n * bloc\n * étendu\n **/ classe Exemple : Base { protégée virtuel vide Executer() remplacer; constructeur(); destructeur(); opérateur(); soi; parent; }"},
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

    using AnalyseurDeclarationsAutoHeberge =
        std::uint32_t (GS_ABI_HOTE *)(RequeteAnalyseDeclarationsHote*);

    std::vector<NoeudDeclarationHote> ComparerDeclarations(
        AnalyseurDeclarationsAutoHeberge analyseur,
        const std::string& source,
        std::string_view nomCorpus)
    {
        const auto jetons = GsPP::Lexeur(
            source, std::string(nomCorpus)).Analyser();
        const auto programme = GsPP::AnalyseurSyntaxique(
            jetons, std::string(nomCorpus)).Analyser();
        const auto reference = ConstruireDeclarationsReference(programme);

        RequeteAnalyseDeclarationsHote requete{
            source.data(),
            static_cast<std::uint64_t>(source.size()),
            nullptr,
            0,
            {}};
        const auto interrogation = analyseur(&requete);
        Exiger(
            interrogation == 1
                && requete.Resultat.Erreur == 1
                && requete.Resultat.NombreNoeuds == reference.size()
                && requete.Resultat.CapaciteRequise == reference.size()
                && requete.Resultat.NombreOctetsArene != 0,
            "interrogation de capacité de l’AST Gs++ incorrecte pour "
                + std::string(nomCorpus)
                + " (erreur " + std::to_string(interrogation)
                + " à " + std::to_string(requete.Resultat.LigneErreur)
                + ":" + std::to_string(requete.Resultat.ColonneErreur)
                + ", noeuds "
                + std::to_string(requete.Resultat.NombreNoeuds)
                + "/" + std::to_string(reference.size()) + ")");

        if (reference.size() > 1)
        {
            std::vector<NoeudDeclarationHote> partiel(
                reference.size() - 1);
            requete.Noeuds = partiel.data();
            requete.Capacite = partiel.size();
            const auto capacitePartielle = analyseur(&requete);
            Exiger(
                capacitePartielle == 1
                    && requete.Resultat.NombreNoeuds == reference.size()
                    && requete.Resultat.CapaciteRequise == reference.size(),
                "capacité partielle de l’AST Gs++ mal diagnostiquée pour "
                    + std::string(nomCorpus));
        }

        std::vector<NoeudDeclarationHote> obtenu(reference.size());
        requete.Noeuds = obtenu.data();
        requete.Capacite = obtenu.size();
        const auto resultat = analyseur(&requete);
        Exiger(
            resultat == 0
                && requete.Resultat.Erreur == 0
                && requete.Resultat.NombreNoeuds == reference.size()
                && requete.Resultat.CapaciteRequise == reference.size()
                && requete.Resultat.Reserve == 0,
            "analyse des déclarations Gs++ échouée pour "
                + std::string(nomCorpus));

        for (std::size_t index = 0; index < reference.size(); ++index)
        {
            const auto& attendu = reference[index];
            const auto& courant = obtenu[index];
            Exiger(
                MemeStructureDeclaration(courant, attendu)
                    && courant.Ligne == attendu.Ligne
                    && courant.Colonne == attendu.Colonne,
                "nœud de déclaration différent au rang "
                    + std::to_string(index) + " de "
                    + std::string(nomCorpus));
            const bool nomAnonyme = courant.Genre == 0
                || courant.Genre == 16
                || courant.Genre == 17
                || courant.Genre == 18
                || courant.Genre == 20
                || courant.Genre == 21
                || courant.Genre == 22
                || courant.Genre == 27
                || courant.Genre == 28
                || courant.Genre == 30
                || courant.Genre == 31
                || courant.Genre == 32
                || courant.Genre == 33
                || courant.Genre == 34;
            if (nomAnonyme)
            {
                Exiger(
                    courant.DebutNom == 0
                        && courant.TailleNom == 0
                        && courant.HachageNom == 0,
                    "nœud syntaxique anonyme Gs++ non canonique pour "
                        + std::string(nomCorpus));
                continue;
            }
            Exiger(
                courant.DebutNom <= source.size()
                    && courant.TailleNom
                        <= source.size() - courant.DebutNom,
                "tranche de nom invalide dans l’AST Gs++ pour "
                    + std::string(nomCorpus));
            const auto nom = std::string_view(source).substr(
                static_cast<std::size_t>(courant.DebutNom),
                static_cast<std::size_t>(courant.TailleNom));
            if (courant.Genre == 23)
            {
                const auto jetonsChaine = GsPP::Lexeur(
                    std::string(nom), "chaine-expression").Analyser();
                Exiger(
                    jetonsChaine.size() == 2
                        && jetonsChaine.front().Genre
                            == GsPP::GenreJeton::ChaineCaracteres
                        && HacherTexte(jetonsChaine.front().Texte)
                            == courant.HachageNom,
                    "hachage de chaîne incohérent dans l’AST Gs++ pour "
                        + std::string(nomCorpus));
                continue;
            }
            if (courant.Genre == 24
                && courant.HachageNom == HacherTexte("soi")
                && (nom == "soi" || nom == "this"
                    || nom == "parent" || nom == "super"))
                continue;
            Exiger(
                HacherTexte(nom) == courant.HachageNom,
                "hachage de nom incohérent dans l’AST Gs++ pour "
                    + std::string(nomCorpus));
        }
        return obtenu;
    }

    void ComparerErreurDeclarations(
        AnalyseurDeclarationsAutoHeberge analyseur,
        const std::string& source,
        std::uint32_t erreurAttendue,
        std::string_view nomCorpus)
    {
        std::uint32_t ligne = 0;
        std::uint32_t colonne = 0;
        bool bootstrapRefuse = false;
        try
        {
            const auto jetons = GsPP::Lexeur(
                source, std::string(nomCorpus)).Analyser();
            (void)GsPP::AnalyseurSyntaxique(
                jetons, std::string(nomCorpus)).Analyser();
        }
        catch (const GsPP::ErreurCompilation& erreur)
        {
            bootstrapRefuse = true;
            ligne = static_cast<std::uint32_t>(erreur.Ligne());
            colonne = static_cast<std::uint32_t>(erreur.Colonne());
        }
        Exiger(
            bootstrapRefuse,
            "le bootstrap a accepté le corpus syntaxique invalide "
                + std::string(nomCorpus));

        RequeteAnalyseDeclarationsHote requete{
            source.data(),
            static_cast<std::uint64_t>(source.size()),
            nullptr,
            0,
            {}};
        const auto resultat = analyseur(&requete);
        Exiger(
            resultat == erreurAttendue
                && requete.Resultat.Erreur == erreurAttendue
                && requete.Resultat.LigneErreur == ligne
                && requete.Resultat.ColonneErreur == colonne
                && requete.Resultat.NombreOctetsArene != 0,
            "diagnostic de déclaration différent du bootstrap pour "
                + std::string(nomCorpus));
    }

    void TesterAnalyseurDeclarations(const std::string& chemin)
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
            if (nom == "GalacticShrine::GsPP::Hote::AllouerMemoire") return allouer;
            if (nom == "GalacticShrine::GsPP::Hote::LibererMemoire") return liberer;
            return std::nullopt;
        };
        const auto image = GsPP::ChargeurGsE().Charger(
            contenu, zone.Base(), resolveur);
        zone.Copier(image.Memoire);

        const auto adresse = image.ChercherExport(
            "GalacticShrine::GsPP::Autohebergement::AnalyserDeclarationsSource");
        Exiger(adresse.has_value(),
               "export de l’analyseur de déclarations Gs++ absent");
        const auto analyseur =
            reinterpret_cast<AnalyseurDeclarationsAutoHeberge>(*adresse);

        const std::string francais =
            "espace Demo {\n"
            "  publique entier32 Addition(entier32 gauche, "
            "constante naturel64* droite) { retourner gauche; }\n"
            "  externe vide Journaliser(caractère* texte);\n"
            "}\n";
        const std::string anglais =
            "namespace Demo {\n"
            "  public int32 Addition(int32 gauche, "
            "const uint64* droite) { return gauche; }\n"
            "  extern void Journaliser(char* texte);\n"
            "}\n";
        const auto astFrancais = ComparerDeclarations(
            analyseur, francais, "declarations-francaises");
        const auto astAnglais = ComparerDeclarations(
            analyseur, anglais, "declarations-anglaises");
        Exiger(
            astFrancais.size() == astAnglais.size(),
            "les AST français et anglais ont des tailles différentes");
        for (std::size_t index = 0; index < astFrancais.size(); ++index)
            Exiger(
                MemeStructureDeclaration(
                    astFrancais[index], astAnglais[index]),
                "les AST français et anglais divergent au rang "
                    + std::to_string(index));

        ComparerDeclarations(analyseur, "", "declarations-vides");
        ComparerDeclarations(
            analyseur,
            "espace Galactic { espace Frontend { "
            "vide Executer() {} } }",
            "espaces-imbriques");
        ComparerDeclarations(
            analyseur,
            "namespace Galactic::Frontend {\n"
            "  public uint64 Compter(const byte donnees[4], "
            "uint32 dimensions[2][3]) { return 0; }\n"
            "}\n",
            "espace-qualifie-tableaux");
        ComparerDeclarations(
            analyseur,
            "espace Galactic {\n"
            "  publique GalacticShrine::GsPP::Noeud* Copier("
            "constante GalacticShrine::GsPP::Noeud& source) { retourner source; }\n"
            "}\n",
            "types-qualifies");

        const std::string donneesFrancaises =
            "espace Demo {\n"
            "  publique naturel64 Compteur = 1 + (2 * 3);\n"
            "  externe entier32 ValeurExterne;\n"
            "  entier32 Valeurs[2] = {1, 2};\n"
            "  structure Point {\n"
            "    entier32 X;\n"
            "    entier32 Y;\n"
            "    alias Abscisse = X;\n"
            "  };\n"
            "  structure Vide {};\n"
            "  union Valeur { octet OctetBrut; entier32 Entier; };\n"
            "  énumération Couleur { Rouge = 1, Vert, Bleu = 2 + 3, };\n"
            "  énumération VideEnum {};\n"
            "  alias API::CompteurPublic = Demo::Compteur;\n"
            "  classe Base { publique: entier32 Id; };\n"
            "  classe Objet : publique Base {\n"
            "    privée: entier32 Secret = 7;\n"
            "    protégée: naturel64 Donnees[2];\n"
            "    publique: alias Identifiant = Secret;\n"
            "  };\n"
            "}\n";
        const std::string donneesAnglaises =
            "namespace Demo {\n"
            "  public uint64 Compteur = 1 + (2 * 3);\n"
            "  extern int32 ValeurExterne;\n"
            "  int32 Valeurs[2] = {1, 2};\n"
            "  struct Point {\n"
            "    int32 X;\n"
            "    int32 Y;\n"
            "    alias Abscisse = X;\n"
            "  };\n"
            "  struct Vide {};\n"
            "  union Valeur { byte OctetBrut; int32 Entier; };\n"
            "  enum Couleur { Rouge = 1, Vert, Bleu = 2 + 3, };\n"
            "  enum VideEnum {};\n"
            "  alias API::CompteurPublic = Demo::Compteur;\n"
            "  class Base { public: int32 Id; };\n"
            "  class Objet : public Base {\n"
            "    private: int32 Secret = 7;\n"
            "    protected: uint64 Donnees[2];\n"
            "    public: alias Identifiant = Secret;\n"
            "  };\n"
            "}\n";
        const auto astDonneesFrancais = ComparerDeclarations(
            analyseur, donneesFrancaises, "donnees-francaises");
        const auto astDonneesAnglais = ComparerDeclarations(
            analyseur, donneesAnglaises, "donnees-anglaises");
        Exiger(
            astDonneesFrancais.size() == astDonneesAnglais.size(),
            "les AST de données français et anglais ont des tailles différentes");
        for (std::size_t index = 0;
             index < astDonneesFrancais.size();
             ++index)
            Exiger(
                MemeStructureDeclaration(
                    astDonneesFrancais[index], astDonneesAnglais[index]),
                "les AST de données français et anglais divergent au rang "
                    + std::to_string(index));

        const std::string membresFrancais =
            "espace Demo {\n"
            "  classe Base {\n"
            "    publique:\n"
            "    constructeur(entier32 valeur) {}\n"
            "    virtuel destructeur() {}\n"
            "    virtuel entier32 Lire(entier32 delta) {}\n"
            "    entier32 opérateur+(entier32 droite) {}\n"
            "  };\n"
            "  classe Service : publique Base {\n"
            "    privée: entier32 Etat;\n"
            "    protégée:\n"
            "    remplacer entier32 Lire(entier32 delta) {}\n"
            "    publique:\n"
            "    constructeur(entier32 valeur)\n"
            "      : parent(valeur), Etat((valeur + 1)) {}\n"
            "    constructeur() : soi(1) {}\n"
            "    remplacer destructeur() {}\n"
            "    booléen opérateur==(constante Service& autre) {}\n"
            "  };\n"
            "}\n";
        const std::string membresAnglais =
            "namespace Demo {\n"
            "  class Base {\n"
            "    public:\n"
            "    constructor(int32 valeur) {}\n"
            "    virtual destructor() {}\n"
            "    virtual int32 Lire(int32 delta) {}\n"
            "    int32 operator+(int32 droite) {}\n"
            "  };\n"
            "  class Service : public Base {\n"
            "    private: int32 Etat;\n"
            "    protected:\n"
            "    override int32 Lire(int32 delta) {}\n"
            "    public:\n"
            "    constructor(int32 valeur)\n"
            "      : super(valeur), Etat((valeur + 1)) {}\n"
            "    constructor() : this(1) {}\n"
            "    override destructor() {}\n"
            "    bool operator==(const Service& autre) {}\n"
            "  };\n"
            "}\n";
        const auto astMembresFrancais = ComparerDeclarations(
            analyseur, membresFrancais, "membres-classes-francais");
        const auto astMembresAnglais = ComparerDeclarations(
            analyseur, membresAnglais, "membres-classes-anglais");
        Exiger(
            astMembresFrancais.size() == astMembresAnglais.size(),
            "les AST de membres français et anglais ont des tailles différentes");
        for (std::size_t index = 0;
             index < astMembresFrancais.size();
             ++index)
            Exiger(
                MemeStructureDeclaration(
                    astMembresFrancais[index], astMembresAnglais[index]),
                "les AST de membres français et anglais divergent au rang "
                    + std::to_string(index));
        Exiger(
            std::count_if(
                astMembresFrancais.begin(),
                astMembresFrancais.end(),
                [](const NoeudDeclarationHote& noeud)
                {
                    return noeud.Genre >= 12 && noeud.Genre <= 15;
                }) == 9,
            "le corpus de classes ne contient pas tous ses membres exécutables");
        Exiger(
            std::count_if(
                astMembresFrancais.begin(),
                astMembresFrancais.end(),
                [](const NoeudDeclarationHote& noeud)
                {
                    return noeud.Genre == 2;
                }) == 6,
            "l’AST expose un paramètre implicite soi ou perd un paramètre source");
        for (const auto& noeud : astMembresFrancais)
            if (noeud.Genre >= 12 && noeud.Genre <= 15)
                Exiger(
                    noeud.Parent < astMembresFrancais.size()
                        && astMembresFrancais[noeud.Parent].Genre == 6,
                    "un membre exécutable n’est pas rattaché à sa classe");
        Exiger(
            std::any_of(
                astMembresFrancais.begin(),
                astMembresFrancais.end(),
                [](const NoeudDeclarationHote& noeud)
                {
                    return noeud.Genre == 13
                        && (noeud.Drapeaux & (256U | 1024U))
                            == (256U | 1024U);
                }),
            "le constructeur de base et de champ n’est pas décrit");
        Exiger(
            std::any_of(
                astMembresFrancais.begin(),
                astMembresFrancais.end(),
                [](const NoeudDeclarationHote& noeud)
                {
                    return noeud.Genre == 13
                        && (noeud.Drapeaux & 512U) != 0;
                }),
            "le constructeur délégué n’est pas décrit");
        for (std::uint32_t genre = 33; genre <= 35; ++genre)
            Exiger(
                std::any_of(
                    astMembresFrancais.begin(),
                    astMembresFrancais.end(),
                    [genre](const NoeudDeclarationHote& noeud)
                    { return noeud.Genre == genre; }),
                "genre d’initialiseur de constructeur absent : "
                    + std::to_string(genre));
        for (const auto& noeud : astMembresFrancais)
            if (noeud.Genre >= 33 && noeud.Genre <= 35)
                Exiger(
                    noeud.Parent < astMembresFrancais.size()
                        && astMembresFrancais[noeud.Parent].Genre == 13,
                    "initialiseur non rattaché à son constructeur");

        const std::string instructionsFrancaises =
            "espace Demo {\n"
            "  structure Point { entier32 X; entier32 Y; };\n"
            "  publique entier32 Calculer(entier32 limite) {\n"
            "    entier32 somme = 0;\n"
            "    entier32 index(0);\n"
            "    Point point = {1, 2};\n"
            "    { entier32 local; somme = somme + local; }\n"
            "    tantque (index < limite) {\n"
            "      si (index == 2) { retourner somme; }\n"
            "      sinon si (index == 3) retourner;\n"
            "      sinon { somme = somme + index; }\n"
            "      index = index + 1;\n"
            "    }\n"
            "    retourner somme;\n"
            "  }\n"
            "}\n";
        const std::string instructionsAnglaises =
            "namespace Demo {\n"
            "  struct Point { int32 X; int32 Y; };\n"
            "  public int32 Calculer(int32 limite) {\n"
            "    int32 somme = 0;\n"
            "    int32 index(0);\n"
            "    Point point = {1, 2};\n"
            "    { int32 local; somme = somme + local; }\n"
            "    while (index < limite) {\n"
            "      if (index == 2) { return somme; }\n"
            "      else if (index == 3) return;\n"
            "      else { somme = somme + index; }\n"
            "      index = index + 1;\n"
            "    }\n"
            "    return somme;\n"
            "  }\n"
            "}\n";
        const auto astInstructionsFrancais = ComparerDeclarations(
            analyseur,
            instructionsFrancaises,
            "instructions-francaises");
        const auto astInstructionsAnglais = ComparerDeclarations(
            analyseur,
            instructionsAnglaises,
            "instructions-anglaises");
        Exiger(
            astInstructionsFrancais.size() == astInstructionsAnglais.size(),
            "les AST d’instructions français et anglais ont des tailles différentes");
        for (std::size_t index = 0;
             index < astInstructionsFrancais.size();
             ++index)
            Exiger(
                MemeStructureDeclaration(
                    astInstructionsFrancais[index],
                    astInstructionsAnglais[index]),
                "les AST d’instructions français et anglais divergent au rang "
                    + std::to_string(index));

        const auto compterGenre = [&](std::uint32_t genre)
        {
            return std::count_if(
                astInstructionsFrancais.begin(),
                astInstructionsFrancais.end(),
                [genre](const NoeudDeclarationHote& noeud)
                {
                    return noeud.Genre == genre;
                });
        };
        Exiger(compterGenre(16) == 5, "nombre de blocs incorrect");
        Exiger(compterGenre(17) == 3, "nombre de retours incorrect");
        Exiger(compterGenre(18) == 3, "nombre d’expressions incorrect");
        Exiger(compterGenre(19) == 4, "nombre de variables locales incorrect");
        Exiger(compterGenre(20) == 2, "nombre de conditionnelles incorrect");
        Exiger(compterGenre(21) == 1, "nombre de boucles incorrect");
        Exiger(
            std::any_of(
                astInstructionsFrancais.begin(),
                astInstructionsFrancais.end(),
                [](const NoeudDeclarationHote& noeud)
                {
                    return noeud.Genre == 19
                        && (noeud.Drapeaux & (4U | 2048U))
                            == (4U | 2048U);
                }),
            "l’initialiseur d’une variable locale n’est pas décrit");
        Exiger(
            std::any_of(
                astInstructionsFrancais.begin(),
                astInstructionsFrancais.end(),
                [](const NoeudDeclarationHote& noeud)
                {
                    return noeud.Genre == 19
                        && (noeud.Drapeaux & 8192U) != 0;
                }),
            "la construction explicite d’une variable locale n’est pas décrite");
        Exiger(
            std::any_of(
                astInstructionsFrancais.begin(),
                astInstructionsFrancais.end(),
                [](const NoeudDeclarationHote& noeud)
                {
                    return noeud.Genre == 17
                        && (noeud.Drapeaux & 2048U) == 0;
                })
                && std::any_of(
                    astInstructionsFrancais.begin(),
                    astInstructionsFrancais.end(),
                    [](const NoeudDeclarationHote& noeud)
                    {
                        return noeud.Genre == 17
                            && (noeud.Drapeaux & 2048U) != 0;
                    }),
            "les deux formes de retour ne sont pas décrites");
        Exiger(
            std::count_if(
                astInstructionsFrancais.begin(),
                astInstructionsFrancais.end(),
                [](const NoeudDeclarationHote& noeud)
                {
                    return noeud.Genre == 20
                        && (noeud.Drapeaux & 4096U) != 0;
                }) == 2,
            "une branche sinon n’est pas décrite");
        for (const auto& noeud : astInstructionsFrancais)
        {
            if (noeud.Genre < 16 || noeud.Genre > 21) continue;
            Exiger(
                noeud.Parent < astInstructionsFrancais.size(),
                "parent d’instruction hors limites");
            const auto genreParent =
                astInstructionsFrancais[noeud.Parent].Genre;
            if (noeud.Genre == 16)
                Exiger(
                    genreParent == 1
                        || (genreParent >= 12 && genreParent <= 16)
                        || genreParent == 20
                        || genreParent == 21,
                    "bloc rattaché à un parent syntaxique invalide");
            else
                Exiger(
                    genreParent == 16
                        || genreParent == 20
                        || genreParent == 21,
                    "instruction rattachée à un parent syntaxique invalide");
        }

        const std::string expressionsFrancaises =
            "espace Expressions {\n"
            "  entier32 Globale = convertir<entier32>(1 + 2);\n"
            "  énumération Drapeau { Premier = 1 << 2, Second = 3, };\n"
            "  classe Objet {\n"
            "    publique:\n"
            "    entier32 Champ = 7;\n"
            "    constructeur(entier32 valeur) : Champ(valeur + 1) {}\n"
            "    entier32 Evaluer(Objet* pointeur, entier32 entree) {\n"
            "      entier32 a = 1_234;\n"
            "      entier32 b = vrai;\n"
            "      caractère* texte = \"Gs++\\n\";\n"
            "      entier32 tableau[3] = {1, 2, 3};\n"
            "      a = b = 4;\n"
            "      a = +a + -b + !faux + ~a + &a + *pointeur;\n"
            "      a = (a || b) && ((a | b) ^ (a & b));\n"
            "      a = (a == b) + (a != b) + (a < b) + (a <= b);\n"
            "      a = (a > b) + (a >= b) + (a << 1) + (b >> 2);\n"
            "      a = a + b - a * b / 2 % 3;\n"
            "      a = Calculer(tableau[1], convertir<entier32>(entree));\n"
            "      soi.Champ = parent.Champ;\n"
            "      pointeur->Champ = soi.Champ;\n"
            "      retourner texte[0] + API::Valeur;\n"
            "    }\n"
            "  };\n"
            "}\n";
        const std::string expressionsAnglaises =
            "namespace Expressions {\n"
            "  int32 Globale = cast<int32>(1 + 2);\n"
            "  enum Drapeau { Premier = 1 << 2, Second = 3, };\n"
            "  class Objet {\n"
            "    public:\n"
            "    int32 Champ = 7;\n"
            "    constructor(int32 valeur) : Champ(valeur + 1) {}\n"
            "    int32 Evaluer(Objet* pointeur, int32 entree) {\n"
            "      int32 a = 1_234;\n"
            "      int32 b = true;\n"
            "      char* texte = \"Gs++\\n\";\n"
            "      int32 tableau[3] = {1, 2, 3};\n"
            "      a = b = 4;\n"
            "      a = +a + -b + !false + ~a + &a + *pointeur;\n"
            "      a = (a || b) && ((a | b) ^ (a & b));\n"
            "      a = (a == b) + (a != b) + (a < b) + (a <= b);\n"
            "      a = (a > b) + (a >= b) + (a << 1) + (b >> 2);\n"
            "      a = a + b - a * b / 2 % 3;\n"
            "      a = Calculer(tableau[1], cast<int32>(entree));\n"
            "      this.Champ = super.Champ;\n"
            "      pointeur->Champ = this.Champ;\n"
            "      return texte[0] + API::Valeur;\n"
            "    }\n"
            "  };\n"
            "}\n";
        const auto astExpressionsFrancais = ComparerDeclarations(
            analyseur,
            expressionsFrancaises,
            "expressions-francaises");
        const auto astExpressionsAnglais = ComparerDeclarations(
            analyseur,
            expressionsAnglaises,
            "expressions-anglaises");
        Exiger(
            astExpressionsFrancais.size() == astExpressionsAnglais.size(),
            "les AST d’expressions français et anglais ont des tailles différentes");
        for (std::size_t index = 0;
             index < astExpressionsFrancais.size();
             ++index)
            Exiger(
                MemeStructureDeclaration(
                    astExpressionsFrancais[index],
                    astExpressionsAnglais[index]),
                "les AST d’expressions français et anglais divergent au rang "
                    + std::to_string(index));

        for (std::uint32_t genre = 22; genre <= 32; ++genre)
            Exiger(
                std::any_of(
                    astExpressionsFrancais.begin(),
                    astExpressionsFrancais.end(),
                    [genre](const NoeudDeclarationHote& noeud)
                    {
                        return noeud.Genre == genre;
                    }),
                "genre d’expression auto-hébergé absent : "
                    + std::to_string(genre));

        const std::array<std::string_view, 6> operateursUnaires{
            "+", "-", "!", "~", "&", "*"};
        for (const auto operateur : operateursUnaires)
            Exiger(
                std::any_of(
                    astExpressionsFrancais.begin(),
                    astExpressionsFrancais.end(),
                    [operateur](const NoeudDeclarationHote& noeud)
                    {
                        return noeud.Genre == 25
                            && noeud.HachageNom == HacherTexte(operateur);
                    }),
                "opérateur unaire absent de l’AST : "
                    + std::string(operateur));
        const std::array<std::string_view, 18> operateursBinaires{
            "||", "&&", "|", "^", "&", "==", "!=", "<", "<=",
            ">", ">=", "<<", ">>", "+", "-", "*", "/", "%"};
        for (const auto operateur : operateursBinaires)
            Exiger(
                std::any_of(
                    astExpressionsFrancais.begin(),
                    astExpressionsFrancais.end(),
                    [operateur](const NoeudDeclarationHote& noeud)
                    {
                        return noeud.Genre == 26
                            && noeud.HachageNom == HacherTexte(operateur);
                    }),
                "opérateur binaire absent de l’AST : "
                    + std::string(operateur));

        Exiger(
            std::any_of(
                astExpressionsFrancais.begin(),
                astExpressionsFrancais.end(),
                [](const NoeudDeclarationHote& noeud)
                {
                    return noeud.Genre == 22
                        && (noeud.Drapeaux & 16'384U) != 0;
                }),
            "les littéraux booléens ne sont pas distingués des entiers");
        Exiger(
            std::any_of(
                astExpressionsFrancais.begin(),
                astExpressionsFrancais.end(),
                [](const NoeudDeclarationHote& noeud)
                {
                    return noeud.Genre == 29
                        && (noeud.Drapeaux & 32'768U) != 0;
                }),
            "l’accès membre par pointeur n’est pas distingué");
        Exiger(
            std::any_of(
                astExpressionsFrancais.begin(),
                astExpressionsFrancais.end(),
                [](const NoeudDeclarationHote& noeud)
                {
                    return noeud.Genre == 24
                        && (noeud.Drapeaux & 65'536U) != 0
                        && noeud.HachageNom == HacherTexte("soi");
                }),
            "la référence de base parent/super n’est pas distinguée");
        Exiger(
            std::any_of(
                astExpressionsFrancais.begin(),
                astExpressionsFrancais.end(),
                [&astExpressionsFrancais](const NoeudDeclarationHote& noeud)
                {
                    return noeud.Genre == 27
                        && noeud.Parent < astExpressionsFrancais.size()
                        && astExpressionsFrancais[noeud.Parent].Genre == 27;
                }),
            "l’affectation associative à droite n’est pas préservée");

        for (std::size_t index = 0;
             index < astExpressionsFrancais.size();
             ++index)
        {
            const auto& noeud = astExpressionsFrancais[index];
            if (noeud.Genre < 22 || noeud.Genre > 32) continue;
            Exiger(
                noeud.Parent < index,
                "l’ordre préfixe parent/enfant d’une expression est invalide");
            const auto genreParent =
                astExpressionsFrancais[noeud.Parent].Genre;
            const bool parentExpression =
                genreParent >= 25 && genreParent <= 32;
            const bool parentSyntaxique = genreParent == 3
                || genreParent == 7
                || genreParent == 9
                || genreParent == 13
                || genreParent == 17
                || genreParent == 18
                || genreParent == 19
                || genreParent == 20
                || genreParent == 21
                || (genreParent >= 33 && genreParent <= 35);
            Exiger(
                parentExpression || parentSyntaxique,
                "expression rattachée à un parent syntaxique invalide");
        }

        ComparerErreurDeclarations(
            analyseur,
            "publique entier32 F(entier32 valeur { retourner valeur; }",
            8,
            "parenthese-parametres-manquante");
        ComparerErreurDeclarations(
            analyseur,
            "publique entier32 (entier32 valeur) { retourner valeur; }",
            5,
            "nom-fonction-manquant");
        ComparerErreurDeclarations(
            analyseur,
            "publique entier32 F() { retourner 1;",
            10,
            "accolade-corps-manquante");
        ComparerErreurDeclarations(
            analyseur,
            "externe entier32 Valeur = 1;",
            15,
            "initialiseur-globale-externe");
        ComparerErreurDeclarations(
            analyseur,
            "structure Invalide { entier32 Valeur = 1; };",
            20,
            "initialiseur-champ-structure");
        ComparerErreurDeclarations(
            analyseur,
            "structure Invalide { entier32 Valeurs[2; };",
            17,
            "crochet-champ-manquant");
        ComparerErreurDeclarations(
            analyseur,
            "alias Nom Cible;",
            18,
            "egal-alias-manquant");
        ComparerErreurDeclarations(
            analyseur,
            "classe Invalide { publique entier32 Valeur; };",
            19,
            "deux-points-visibilite-manquant");

        ComparerErreurDeclarations(
            analyseur,
            "classe C { publique: entier32 opérateur() {} };",
            21,
            "operateur-surchargeable-manquant");
        ComparerErreurDeclarations(
            analyseur,
            "classe C { publique: virtuel virtuel vide F() {} };",
            22,
            "modificateur-membre-duplique");
        ComparerErreurDeclarations(
            analyseur,
            "classe C { publique: virtuel constructeur() {} };",
            23,
            "constructeur-virtuel");
        ComparerErreurDeclarations(
            analyseur,
            "classe C { publique: destructeur(entier32 valeur) {} };",
            24,
            "parametre-destructeur");
        ComparerErreurDeclarations(
            analyseur,
            "classe C { publique: vide F() : Champ() {} };",
            25,
            "liste-initialisation-methode");
        ComparerErreurDeclarations(
            analyseur,
            "classe C { publique: constructeur() : soi(), Valeur() {} };",
            26,
            "delegation-constructeur-melangee");
        ComparerErreurDeclarations(
            analyseur,
            "publique vide F() { si () retourner; }",
            14,
            "condition-si-vide");
        ComparerErreurDeclarations(
            analyseur,
            "publique vide F() { tantque () {} }",
            14,
            "condition-tantque-vide");
        ComparerErreurDeclarations(
            analyseur,
            "publique vide F() { si vrai) retourner; }",
            7,
            "parenthese-condition-ouvrante-manquante");
        ComparerErreurDeclarations(
            analyseur,
            "publique vide F() { entier32 ; }",
            5,
            "nom-variable-locale-manquant");
        ComparerErreurDeclarations(
            analyseur,
            "publique vide F() { ; }",
            14,
            "instruction-expression-vide");
        ComparerErreurDeclarations(
            analyseur,
            "publique vide F() { entier32 valeur }",
            11,
            "point-virgule-variable-locale-manquant");
        ComparerErreurDeclarations(
            analyseur,
            "publique vide F() { retourner valeur }",
            11,
            "point-virgule-retour-manquant");
        ComparerErreurDeclarations(
            analyseur,
            "publique vide F() { valeur }",
            11,
            "point-virgule-expression-manquant");
        ComparerErreurDeclarations(
            analyseur,
            "publique vide F() { retourner valeur + ; }",
            14,
            "operande-binaire-manquant");
        ComparerErreurDeclarations(
            analyseur,
            "publique vide F() { retourner (1 + 2; }",
            8,
            "parenthese-expression-manquante");
        ComparerErreurDeclarations(
            analyseur,
            "publique vide F() { retourner valeurs[1; }",
            17,
            "crochet-indexation-manquant");
        ComparerErreurDeclarations(
            analyseur,
            "publique vide F() { retourner valeurs[]; }",
            14,
            "indice-manquant");
        ComparerErreurDeclarations(
            analyseur,
            "publique vide F() { retourner objet.; }",
            5,
            "nom-membre-manquant");
        ComparerErreurDeclarations(
            analyseur,
            "publique vide F() { retourner Appeler(1; }",
            8,
            "parenthese-appel-manquante");
        ComparerErreurDeclarations(
            analyseur,
            "publique vide F() { retourner convertir entier32(1); }",
            27,
            "chevron-conversion-ouvrant-manquant");
        ComparerErreurDeclarations(
            analyseur,
            "publique vide F() { retourner cast<int32(1); }",
            28,
            "chevron-conversion-fermant-manquant");
        ComparerErreurDeclarations(
            analyseur,
            "publique vide F() { retourner cast<int32> 1; }",
            7,
            "parenthese-conversion-ouvrante-manquante");
        ComparerErreurDeclarations(
            analyseur,
            "publique vide F() { retourner {1, 2; }",
            10,
            "accolade-agregat-manquante");
        ComparerErreurDeclarations(
            analyseur,
            "publique naturel64 F() { retourner 18446744073709551616; }",
            29,
            "litteral-entier-debordant");

        const std::string lexicalementInvalide = "@";
        RequeteAnalyseDeclarationsHote requeteLexicale{
            lexicalementInvalide.data(),
            static_cast<std::uint64_t>(lexicalementInvalide.size()),
            nullptr,
            0,
            {}};
        Exiger(
            analyseur(&requeteLexicale) == 2
                && requeteLexicale.Resultat.Detail == 7
                && requeteLexicale.Resultat.LigneErreur == 1
                && requeteLexicale.Resultat.ColonneErreur == 1,
            "l’erreur lexicale n’a pas été propagée par l’analyseur");

        Exiger(
            analyseur(nullptr) == 3,
            "une requête d’analyse nulle aurait dû être refusée");
        RequeteAnalyseDeclarationsHote requeteInvalide{
            nullptr, 1, nullptr, 0, {}};
        Exiger(
            analyseur(&requeteInvalide) == 3
                && requeteInvalide.Resultat.LigneErreur == 1
                && requeteInvalide.Resultat.ColonneErreur == 1,
            "une source d’analyse nulle aurait dû être refusée");
        NoeudDeclarationHote* aucunNoeud = nullptr;
        RequeteAnalyseDeclarationsHote sortieInvalide{
            francais.data(),
            static_cast<std::uint64_t>(francais.size()),
            aucunNoeud,
            1,
            {}};
        Exiger(
            analyseur(&sortieInvalide) == 3,
            "une capacité sans tampon AST aurait dû être refusée");
        Exiger(
            !LiberationInvalide
                && AllocationsActives.empty()
                && NombreAllocations == NombreLiberations
                && NombreAllocations != 0,
            "l’AST auto-hébergé ne libère pas proprement son arène");
    }

    using AnalyseurSemantiqueAutoHeberge =
        std::uint32_t (GS_ABI_HOTE *)(RequeteAnalyseSemantiqueHote*);

    struct SortieSemantiqueHote
    {
        std::vector<NoeudDeclarationHote> Noeuds;
        std::vector<SymboleSemantiqueHote> Symboles;
        std::vector<ResolutionSemantiqueHote> Resolutions;
    };

    SortieSemantiqueHote AnalyserSemantiqueValide(
        AnalyseurDeclarationsAutoHeberge syntaxe,
        AnalyseurSemantiqueAutoHeberge semantique,
        const std::string& source,
        std::string_view nomCorpus)
    {
        auto noeuds = ComparerDeclarations(syntaxe, source, nomCorpus);
        RequeteAnalyseSemantiqueHote requete{
            source.data(),
            static_cast<std::uint64_t>(source.size()),
            noeuds.data(),
            static_cast<std::uint64_t>(noeuds.size()),
            nullptr,
            0,
            nullptr,
            0,
            {}};
        const auto interrogation = semantique(&requete);
        Exiger(
            interrogation == 4
                && requete.Resultat.Erreur == 4
                && requete.Resultat.NombreSymboles != 0
                && requete.Resultat.CapaciteSymbolesRequise
                    == requete.Resultat.NombreSymboles
                && requete.Resultat.CapaciteResolutionsRequise
                    == requete.Resultat.NombreResolutions
                && requete.Resultat.NombreOctetsArene != 0,
            "interrogation de capacité sémantique incorrecte pour "
                + std::string(nomCorpus));

        std::vector<SymboleSemantiqueHote> symboles(
            static_cast<std::size_t>(requete.Resultat.NombreSymboles));
        std::vector<ResolutionSemantiqueHote> resolutions(
            static_cast<std::size_t>(requete.Resultat.NombreResolutions));

        if (symboles.size() > 1)
        {
            requete.Symboles = symboles.data();
            requete.CapaciteSymboles = symboles.size() - 1;
            requete.Resolutions = resolutions.data();
            requete.CapaciteResolutions = resolutions.size();
            Exiger(
                semantique(&requete) == 4
                    && requete.Resultat.Erreur == 4
                    && requete.Resultat.CapaciteSymbolesRequise
                        == symboles.size(),
                "capacité partielle des symboles mal diagnostiquée pour "
                    + std::string(nomCorpus));
        }

        if (!resolutions.empty())
        {
            requete.Symboles = symboles.data();
            requete.CapaciteSymboles = symboles.size();
            requete.Resolutions = nullptr;
            requete.CapaciteResolutions = 0;
            Exiger(
                semantique(&requete) == 5
                    && requete.Resultat.Erreur == 5
                    && requete.Resultat.CapaciteResolutionsRequise
                        == resolutions.size(),
                "capacité partielle des résolutions mal diagnostiquée pour "
                    + std::string(nomCorpus));
        }

        requete.Symboles = symboles.data();
        requete.CapaciteSymboles = symboles.size();
        requete.Resolutions = resolutions.data();
        requete.CapaciteResolutions = resolutions.size();
        const auto resultat = semantique(&requete);
        Exiger(
            resultat == 0
                && requete.Resultat.Erreur == 0
                && requete.Resultat.NombreSymboles == symboles.size()
                && requete.Resultat.NombreResolutions == resolutions.size(),
            "analyse sémantique auto-hébergée échouée pour "
                + std::string(nomCorpus));

        auto jetons = GsPP::Lexeur(source, std::string(nomCorpus)).Analyser();
        auto programme = GsPP::AnalyseurSyntaxique(
            std::move(jetons), std::string(nomCorpus)).Analyser();
        GsPP::AnalyseurSemantique().Analyser(programme);

        return {
            std::move(noeuds),
            std::move(symboles),
            std::move(resolutions)};
    }

    void ComparerErreurSemantique(
        AnalyseurDeclarationsAutoHeberge syntaxe,
        AnalyseurSemantiqueAutoHeberge semantique,
        const std::string& source,
        std::uint32_t erreurAttendue,
        std::string_view nomCorpus)
    {
        auto noeuds = ComparerDeclarations(syntaxe, source, nomCorpus);
        std::uint32_t ligne = 0;
        std::uint32_t colonne = 0;
        try
        {
            auto jetons = GsPP::Lexeur(
                source, std::string(nomCorpus)).Analyser();
            auto programme = GsPP::AnalyseurSyntaxique(
                std::move(jetons), std::string(nomCorpus)).Analyser();
            GsPP::AnalyseurSemantique().Analyser(programme);
        }
        catch (const GsPP::ErreurCompilation& erreur)
        {
            ligne = static_cast<std::uint32_t>(erreur.Ligne());
            colonne = static_cast<std::uint32_t>(erreur.Colonne());
        }
        Exiger(
            ligne != 0 && colonne != 0,
            "le bootstrap aurait dû refuser le corpus sémantique "
                + std::string(nomCorpus));

        RequeteAnalyseSemantiqueHote requete{
            source.data(),
            static_cast<std::uint64_t>(source.size()),
            noeuds.data(),
            static_cast<std::uint64_t>(noeuds.size()),
            nullptr,
            0,
            nullptr,
            0,
            {}};
        const auto obtenu = semantique(&requete);
        Exiger(
            obtenu == erreurAttendue
                && requete.Resultat.Erreur == erreurAttendue
                && requete.Resultat.LigneErreur == ligne
                && requete.Resultat.ColonneErreur == colonne,
            "diagnostic sémantique différent du bootstrap pour "
                + std::string(nomCorpus)
                + " (attendu " + std::to_string(erreurAttendue)
                + " à " + std::to_string(ligne)
                + ":" + std::to_string(colonne)
                + ", obtenu " + std::to_string(obtenu)
                + " à " + std::to_string(requete.Resultat.LigneErreur)
                + ":" + std::to_string(requete.Resultat.ColonneErreur)
                + ")");
    }

    void TesterAnalyseurSemantique(
        const std::string& chemin,
        const std::string& cheminSyntaxe)
    {
        AllocationsActives.clear();
        NombreAllocations = 0;
        NombreLiberations = 0;
        LiberationInvalide = false;

        const auto contenuSyntaxe = LireFichier(cheminSyntaxe);
        const auto tailleSyntaxe = Lire64(contenuSyntaxe, 48);
        const auto trampolinesSyntaxe = AlignerPage(tailleSyntaxe);
        ZoneExecutable zoneSyntaxe(trampolinesSyntaxe + 4096);
        const auto allouerSyntaxe = zoneSyntaxe.AjouterTrampoline(
            trampolinesSyntaxe,
            reinterpret_cast<std::uintptr_t>(&AllouerMemoireHote));
        const auto libererSyntaxe = zoneSyntaxe.AjouterTrampoline(
            trampolinesSyntaxe + 16,
            reinterpret_cast<std::uintptr_t>(&LibererMemoireHote));
        const auto resolveurSyntaxe =
            [&](std::string_view nom) -> std::optional<std::uint64_t>
        {
            if (nom == "GalacticShrine::GsPP::Hote::AllouerMemoire") return allouerSyntaxe;
            if (nom == "GalacticShrine::GsPP::Hote::LibererMemoire") return libererSyntaxe;
            return std::nullopt;
        };
        const auto imageSyntaxe = GsPP::ChargeurGsE().Charger(
            contenuSyntaxe, zoneSyntaxe.Base(), resolveurSyntaxe);
        zoneSyntaxe.Copier(imageSyntaxe.Memoire);
        const auto exportSyntaxe = imageSyntaxe.ChercherExport(
            "GalacticShrine::GsPP::Autohebergement::AnalyserDeclarationsSource");
        Exiger(exportSyntaxe.has_value(),
               "export syntaxique requis par la sémantique absent");
        const auto syntaxe = reinterpret_cast<AnalyseurDeclarationsAutoHeberge>(
            *exportSyntaxe);

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
            if (nom == "GalacticShrine::GsPP::Hote::AllouerMemoire") return allouer;
            if (nom == "GalacticShrine::GsPP::Hote::LibererMemoire") return liberer;
            return std::nullopt;
        };
        const auto image = GsPP::ChargeurGsE().Charger(
            contenu, zone.Base(), resolveur);
        zone.Copier(image.Memoire);
        const auto adresse = image.ChercherExport(
            "GalacticShrine::GsPP::Autohebergement::AnalyserSemantique");
        Exiger(adresse.has_value(),
               "export de l’analyseur sémantique Gs++ absent");
        const auto semantique =
            reinterpret_cast<AnalyseurSemantiqueAutoHeberge>(*adresse);

        const std::string francais =
            "espace Semantique {\n"
            "  structure Point { entier32 X; };\n"
            "  classe Base { publique: entier32 Heritee; "
            "entier32 Lire() { retourner 1; } "
            "entier32 Transformer(entier32 valeur) { retourner valeur; } "
            "entier64 Transformer(entier64 valeur) { retourner valeur; } };\n"
            "  classe Objet : publique Base { publique: entier32 Valeur; "
            "entier32 LireValeur() { retourner soi.Valeur; } "
            "entier32 LireBase() { retourner parent.Lire(); } "
            "entier32 LireHeritee() { retourner soi.Heritee; } "
            "entier64 Transformer64(entier32 valeur) { "
            "retourner soi.Transformer(convertir<entier64>(valeur)); } };\n"
            "  entier32 Globale = 3;\n"
            "  publique entier32 Choisir(entier32 valeur) { "
            "retourner valeur; }\n"
            "  publique entier64 Choisir(entier64 valeur) { "
            "retourner valeur; }\n"
            "  publique entier32 LireReference(entier32& valeur) { "
            "retourner valeur; }\n"
            "  publique entier64 LireReference(entier64& valeur) { "
            "retourner valeur; }\n"
            "  publique entier32 Mesurer(Point valeur) { "
            "retourner valeur.X; }\n"
            "  publique entier32 Mesurer(Point& valeur) { "
            "retourner valeur.X; }\n"
            "  publique entier32 LireBaseReference(Base& valeur) { "
            "retourner valeur.Heritee; }\n"
            "  publique naturel64 LireBaseReference(naturel64 valeur) { "
            "retourner valeur; }\n"
            "  publique entier32 PrendreBase(Base* valeur) { "
            "retourner valeur->Heritee; }\n"
            "  publique naturel64 PrendreBase(naturel64 valeur) { "
            "retourner valeur; }\n"
            "  publique entier32 TesterMembres(Objet* objet, entier32 valeur) {\n"
            "    Objet locale;\n"
            "    retourner objet->Transformer(valeur) "
            "+ LireReference(valeur) + Mesurer({1}) "
            "+ LireBaseReference(locale) + PrendreBase(objet);\n"
            "  }\n"
            "  publique entier32 Calculer(entier32 gauche, entier32 droite) {\n"
            "    entier32 somme = gauche + droite;\n"
            "    { entier32 copie = somme; somme = copie; }\n"
            "    entier64 etendue = Choisir(convertir<entier64>(somme));\n"
            "    retourner Choisir(somme);\n"
            "  }\n"
            "}\n"
            "publique entier32 LireGlobale() { "
            "retourner Semantique::Globale; }\n";
        const std::string anglais =
            "namespace Semantique {\n"
            "  struct Point { int32 X; };\n"
            "  class Base { public: int32 Heritee; "
            "int32 Lire() { return 1; } "
            "int32 Transformer(int32 valeur) { return valeur; } "
            "int64 Transformer(int64 valeur) { return valeur; } };\n"
            "  class Objet : public Base { public: int32 Valeur; "
            "int32 LireValeur() { return this.Valeur; } "
            "int32 LireBase() { return super.Lire(); } "
            "int32 LireHeritee() { return this.Heritee; } "
            "int64 Transformer64(int32 valeur) { "
            "return this.Transformer(cast<int64>(valeur)); } };\n"
            "  int32 Globale = 3;\n"
            "  public int32 Choisir(int32 valeur) { return valeur; }\n"
            "  public int64 Choisir(int64 valeur) { return valeur; }\n"
            "  public int32 LireReference(int32& valeur) { return valeur; }\n"
            "  public int64 LireReference(int64& valeur) { return valeur; }\n"
            "  public int32 Mesurer(Point valeur) { return valeur.X; }\n"
            "  public int32 Mesurer(Point& valeur) { return valeur.X; }\n"
            "  public int32 LireBaseReference(Base& valeur) { "
            "return valeur.Heritee; }\n"
            "  public uint64 LireBaseReference(uint64 valeur) { "
            "return valeur; }\n"
            "  public int32 PrendreBase(Base* valeur) { "
            "return valeur->Heritee; }\n"
            "  public uint64 PrendreBase(uint64 valeur) { return valeur; }\n"
            "  public int32 TesterMembres(Objet* objet, int32 valeur) {\n"
            "    Objet locale;\n"
            "    return objet->Transformer(valeur) "
            "+ LireReference(valeur) + Mesurer({1}) "
            "+ LireBaseReference(locale) + PrendreBase(objet);\n"
            "  }\n"
            "  public int32 Calculer(int32 gauche, int32 droite) {\n"
            "    int32 somme = gauche + droite;\n"
            "    { int32 copie = somme; somme = copie; }\n"
            "    int64 etendue = Choisir(cast<int64>(somme));\n"
            "    return Choisir(somme);\n"
            "  }\n"
            "}\n"
            "public int32 LireGlobale() { return Semantique::Globale; }\n";

        const auto resultatFrancais = AnalyserSemantiqueValide(
            syntaxe, semantique, francais, "semantique-francaise");
        const auto resultatAnglais = AnalyserSemantiqueValide(
            syntaxe, semantique, anglais, "semantique-anglaise");
        Exiger(
            resultatFrancais.Symboles.size()
                    == resultatAnglais.Symboles.size()
                && resultatFrancais.Resolutions.size()
                    == resultatAnglais.Resolutions.size(),
            "les sorties sémantiques bilingues ont des tailles différentes");

        const auto nombreCiblesSemantiques = std::count_if(
            resultatFrancais.Noeuds.begin(),
            resultatFrancais.Noeuds.end(),
            [](const NoeudDeclarationHote& noeud)
            { return noeud.Genre == 24 || noeud.Genre == 29; });
        Exiger(
            resultatFrancais.Resolutions.size()
                == static_cast<std::size_t>(nombreCiblesSemantiques),
            "toutes les références et tous les membres n’ont pas été résolus");
        for (const auto& resolution : resultatFrancais.Resolutions)
        {
            Exiger(
                resolution.IndexNoeud < resultatFrancais.Noeuds.size()
                    && resolution.IndexSymbole
                        < resultatFrancais.Symboles.size(),
                "résolution sémantique hors limites");
            const auto& reference =
                resultatFrancais.Noeuds[resolution.IndexNoeud];
            const auto& symbole =
                resultatFrancais.Symboles[resolution.IndexSymbole];
            Exiger(
                (reference.Genre == 24 || reference.Genre == 29)
                    && resolution.GenreCible == symbole.Genre,
                "genre de cible sémantique incohérent");
            if (reference.Genre == 29)
                Exiger(
                    (resolution.Drapeaux & 8U) != 0,
                    "une résolution de membre ne porte pas son drapeau");
        }
        const auto typeEntier32 = HacherTypeDeclaration(
            GsPP::TypeGs(GsPP::GenreType::Entier32));
        const auto typeEntier64 = HacherTypeDeclaration(
            GsPP::TypeGs(GsPP::GenreType::Entier64));
        bool choisirEntier32 = false;
        bool choisirEntier64 = false;
        std::size_t nombreAppelsChoisir = 0;
        for (const auto& resolution : resultatFrancais.Resolutions)
        {
            const auto& reference =
                resultatFrancais.Noeuds[resolution.IndexNoeud];
            if (reference.HachageNom != HacherTexte("Choisir")
                || (resolution.Drapeaux & 1U) == 0)
                continue;
            ++nombreAppelsChoisir;
            const auto indexFonction = resultatFrancais.Symboles[
                resolution.IndexSymbole].IndexNoeud;
            const auto parametre = std::find_if(
                resultatFrancais.Noeuds.begin(),
                resultatFrancais.Noeuds.end(),
                [&](const NoeudDeclarationHote& noeud)
                {
                    return noeud.Genre == 2
                        && noeud.Parent == indexFonction;
                });
            Exiger(
                parametre != resultatFrancais.Noeuds.end(),
                "la surcharge Choisir sélectionnée n’a aucun paramètre");
            choisirEntier32 |= parametre->HachageType == typeEntier32;
            choisirEntier64 |= parametre->HachageType == typeEntier64;
        }
        Exiger(
            nombreAppelsChoisir == 2
                && choisirEntier32
                && choisirEntier64,
            "la sélection typée des surcharges Choisir est incorrecte");
        const auto recepteur = std::find_if(
            resultatFrancais.Resolutions.begin(),
            resultatFrancais.Resolutions.end(),
            [](const ResolutionSemantiqueHote& resolution)
            { return (resolution.Drapeaux & 2U) != 0; });
        Exiger(
            recepteur != resultatFrancais.Resolutions.end(),
            "le récepteur de classe soi/this n’a pas été résolu");
        const auto base = std::find_if(
            resultatFrancais.Resolutions.begin(),
            resultatFrancais.Resolutions.end(),
            [](const ResolutionSemantiqueHote& resolution)
            { return (resolution.Drapeaux & 4U) != 0; });
        Exiger(
            base != resultatFrancais.Resolutions.end(),
            "le récepteur de base parent/super n’a pas été résolu");

        const auto methodeHeritee = std::find_if(
            resultatFrancais.Resolutions.begin(),
            resultatFrancais.Resolutions.end(),
            [&](const ResolutionSemantiqueHote& resolution)
            {
                const auto& noeud =
                    resultatFrancais.Noeuds[resolution.IndexNoeud];
                return noeud.Genre == 29
                    && noeud.HachageNom == HacherTexte("Transformer")
                    && (resolution.Drapeaux & (1U | 8U | 16U | 32U))
                        == (1U | 8U | 16U | 32U);
            });
        Exiger(
            methodeHeritee != resultatFrancais.Resolutions.end(),
            "la surcharge de méthode héritée n’a pas été sélectionnée");

        const auto champHerite = std::find_if(
            resultatFrancais.Resolutions.begin(),
            resultatFrancais.Resolutions.end(),
            [&](const ResolutionSemantiqueHote& resolution)
            {
                const auto& noeud =
                    resultatFrancais.Noeuds[resolution.IndexNoeud];
                return noeud.Genre == 29
                    && noeud.HachageNom == HacherTexte("Heritee")
                    && (resolution.Drapeaux & (8U | 16U))
                        == (8U | 16U)
                    && (resolution.Drapeaux & 32U) == 0;
            });
        Exiger(
            champHerite != resultatFrancais.Resolutions.end(),
            "le champ hérité n’a pas été résolu");

        const auto hachageParametreCible =
            [&](const ResolutionSemantiqueHote& resolution)
            -> std::uint64_t
        {
            const auto indexFonction = resultatFrancais.Symboles[
                resolution.IndexSymbole].IndexNoeud;
            const auto parametre = std::find_if(
                resultatFrancais.Noeuds.begin(),
                resultatFrancais.Noeuds.end(),
                [&](const NoeudDeclarationHote& noeud)
                {
                    return noeud.Genre == 2
                        && noeud.Parent == indexFonction;
                });
            Exiger(
                parametre != resultatFrancais.Noeuds.end(),
                "la fonction sélectionnée n’a aucun paramètre explicite");
            return parametre->HachageType;
        };

        const auto exigerSurcharge =
            [&](std::string_view nom, std::uint64_t typeParametre)
        {
            const auto resolution = std::find_if(
                resultatFrancais.Resolutions.begin(),
                resultatFrancais.Resolutions.end(),
                [&](const ResolutionSemantiqueHote& valeur)
                {
                    return resultatFrancais.Noeuds[valeur.IndexNoeud]
                                .HachageNom == HacherTexte(nom)
                        && (valeur.Drapeaux & 1U) != 0
                        && hachageParametreCible(valeur) == typeParametre;
                });
            Exiger(
                resolution != resultatFrancais.Resolutions.end(),
                "la surcharge exacte n’a pas été sélectionnée pour "
                    + std::string(nom));
        };

        auto entier32Reference = GsPP::TypeGs(GsPP::GenreType::Entier32);
        entier32Reference.EstReference = true;
        auto pointValeur = GsPP::TypeGs(GsPP::GenreType::Structure, "Point");
        auto baseReference = GsPP::TypeGs(GsPP::GenreType::Structure, "Base");
        baseReference.EstReference = true;
        auto basePointeur = GsPP::TypeGs(
            GsPP::GenreType::Structure, "Base", 1);
        exigerSurcharge(
            "LireReference", HacherTypeDeclaration(entier32Reference));
        exigerSurcharge("Mesurer", HacherTypeDeclaration(pointValeur));
        exigerSurcharge(
            "LireBaseReference", HacherTypeDeclaration(baseReference));
        exigerSurcharge(
            "PrendreBase", HacherTypeDeclaration(basePointeur));

        bool methodeEntier32 = false;
        bool methodeEntier64 = false;
        for (const auto& resolution : resultatFrancais.Resolutions)
        {
            const auto& noeud =
                resultatFrancais.Noeuds[resolution.IndexNoeud];
            if (noeud.Genre != 29
                || noeud.HachageNom != HacherTexte("Transformer"))
                continue;
            const auto type = hachageParametreCible(resolution);
            methodeEntier32 |= type == typeEntier32;
            methodeEntier64 |= type == typeEntier64;
        }
        Exiger(
            methodeEntier32 && methodeEntier64,
            "les deux surcharges de méthode n’ont pas été distinguées");

        const std::string objetFrancais =
            "classe BaseAcces {\n"
            "  protégée: entier32 Protegee; "
            "entier32 LireProtegee() { retourner soi.Protegee; }\n"
            "  privée: entier32 PriveeBase;\n"
            "  publique: constructeur() {}\n"
            "};\n"
            "classe ObjetComplet : publique BaseAcces {\n"
            "  privée: entier32 Secret;\n"
            "  publique:\n"
            "    constructeur() {}\n"
            "    constructeur(entier32 initiale) { soi.Secret = initiale; }\n"
            "    entier32 opérateur +(entier32 delta) { "
            "retourner soi.Secret + delta; }\n"
            "    entier64 opérateur +(entier64 delta) { "
            "retourner convertir<entier64>(soi.Secret) + delta; }\n"
            "    booléen opérateur !() { retourner faux; }\n"
            "    entier32 LireAcces() { "
            "retourner soi.Secret + soi.Protegee + soi.LireProtegee(); }\n"
            "};\n"
            "publique entier32 TesterObjet(entier32 valeur) {\n"
            "  ObjetComplet explicite(valeur);\n"
            "  ObjetComplet implicite;\n"
            "  entier32 somme32 = explicite + valeur;\n"
            "  entier64 somme64 = explicite + convertir<entier64>(valeur);\n"
            "  booléen inverse = !implicite;\n"
            "  retourner somme32;\n"
            "}\n";
        const std::string objetAnglais =
            "class BaseAcces {\n"
            "  protected: int32 Protegee; "
            "int32 LireProtegee() { return this.Protegee; }\n"
            "  private: int32 PriveeBase;\n"
            "  public: constructor() {}\n"
            "};\n"
            "class ObjetComplet : public BaseAcces {\n"
            "  private: int32 Secret;\n"
            "  public:\n"
            "    constructor() {}\n"
            "    constructor(int32 initiale) { this.Secret = initiale; }\n"
            "    int32 operator +(int32 delta) { "
            "return this.Secret + delta; }\n"
            "    int64 operator +(int64 delta) { "
            "return cast<int64>(this.Secret) + delta; }\n"
            "    bool operator !() { return false; }\n"
            "    int32 LireAcces() { "
            "return this.Secret + this.Protegee + this.LireProtegee(); }\n"
            "};\n"
            "public int32 TesterObjet(int32 valeur) {\n"
            "  ObjetComplet explicite(valeur);\n"
            "  ObjetComplet implicite;\n"
            "  int32 somme32 = explicite + valeur;\n"
            "  int64 somme64 = explicite + cast<int64>(valeur);\n"
            "  bool inverse = !implicite;\n"
            "  return somme32;\n"
            "}\n";
        const auto resultatObjetFrancais = AnalyserSemantiqueValide(
            syntaxe, semantique, objetFrancais, "objet-semantique-francais");
        const auto resultatObjetAnglais = AnalyserSemantiqueValide(
            syntaxe, semantique, objetAnglais, "objet-semantique-anglais");
        Exiger(
            resultatObjetFrancais.Symboles.size()
                    == resultatObjetAnglais.Symboles.size()
                && resultatObjetFrancais.Resolutions.size()
                    == resultatObjetAnglais.Resolutions.size(),
            "les résolutions objet bilingues ont des tailles différentes");
        for (std::size_t index = 0;
             index < resultatObjetFrancais.Resolutions.size();
             ++index)
        {
            const auto& francaisResolution =
                resultatObjetFrancais.Resolutions[index];
            const auto& anglaisResolution =
                resultatObjetAnglais.Resolutions[index];
            Exiger(
                francaisResolution.IndexNoeud == anglaisResolution.IndexNoeud
                    && francaisResolution.IndexSymbole
                        == anglaisResolution.IndexSymbole
                    && francaisResolution.HachageType
                        == anglaisResolution.HachageType
                    && francaisResolution.GenreCible
                        == anglaisResolution.GenreCible
                    && francaisResolution.Drapeaux
                        == anglaisResolution.Drapeaux,
                "une résolution objet diffère entre français et anglais");
        }

        const auto nombreParametresCible =
            [&](const SortieSemantiqueHote& resultat,
                const ResolutionSemantiqueHote& resolution)
        {
            const auto indexFonction =
                resultat.Symboles[resolution.IndexSymbole].IndexNoeud;
            return std::count_if(
                resultat.Noeuds.begin(),
                resultat.Noeuds.end(),
                [&](const NoeudDeclarationHote& noeud)
                { return noeud.Genre == 2 && noeud.Parent == indexFonction; });
        };
        std::size_t nombreConstructeurs = 0;
        std::size_t nombreBasesImplicites = 0;
        bool constructeurDefaut = false;
        bool constructeurEntier32 = false;
        std::size_t nombreOperateurs = 0;
        bool operateurEntier32 = false;
        bool operateurEntier64 = false;
        bool operateurUnaire = false;
        bool accesPriveAutorise = false;
        bool accesProtegeHeriteAutorise = false;
        for (const auto& resolution : resultatObjetFrancais.Resolutions)
        {
            const auto& noeud =
                resultatObjetFrancais.Noeuds[resolution.IndexNoeud];
            const auto& cible = resultatObjetFrancais.Symboles[
                resolution.IndexSymbole];
            const auto& declarationCible =
                resultatObjetFrancais.Noeuds[cible.IndexNoeud];
            if ((resolution.Drapeaux & 64U) != 0)
            {
                if (noeud.Genre == 19)
                {
                    ++nombreConstructeurs;
                    Exiger(
                        declarationCible.Genre == 13,
                        "une construction ne cible pas un constructeur");
                    const auto nombre = nombreParametresCible(
                        resultatObjetFrancais, resolution);
                    constructeurDefaut |= nombre == 0
                        && (resolution.Drapeaux & 128U) == 0;
                    if (nombre == 1)
                    {
                        const auto parametre = std::find_if(
                            resultatObjetFrancais.Noeuds.begin(),
                            resultatObjetFrancais.Noeuds.end(),
                            [&](const NoeudDeclarationHote& valeur)
                            {
                                return valeur.Genre == 2
                                    && valeur.Parent == cible.IndexNoeud;
                            });
                        constructeurEntier32 |=
                            parametre != resultatObjetFrancais.Noeuds.end()
                            && parametre->HachageType == typeEntier32
                            && (resolution.Drapeaux & 128U) != 0;
                    }
                }
                else if (noeud.Genre == 13
                    && (resolution.Drapeaux & 1024U) != 0
                    && (resolution.Drapeaux & 128U) == 0)
                {
                    ++nombreBasesImplicites;
                    Exiger(
                        declarationCible.Genre == 13,
                        "une base implicite ne cible pas un constructeur");
                }
            }
            if ((resolution.Drapeaux & 256U) != 0)
            {
                ++nombreOperateurs;
                Exiger(
                    (noeud.Genre == 25 || noeud.Genre == 26)
                        && declarationCible.Genre == 15,
                    "une expression d’opérateur cible un symbole invalide");
                const auto nombre = nombreParametresCible(
                    resultatObjetFrancais, resolution);
                if (nombre == 0) operateurUnaire = noeud.Genre == 25;
                if (nombre == 1)
                {
                    const auto parametre = std::find_if(
                        resultatObjetFrancais.Noeuds.begin(),
                        resultatObjetFrancais.Noeuds.end(),
                        [&](const NoeudDeclarationHote& valeur)
                        {
                            return valeur.Genre == 2
                                && valeur.Parent == cible.IndexNoeud;
                        });
                    if (parametre != resultatObjetFrancais.Noeuds.end())
                    {
                        operateurEntier32 |=
                            parametre->HachageType == typeEntier32;
                        operateurEntier64 |=
                            parametre->HachageType == typeEntier64;
                    }
                }
            }
            accesPriveAutorise |= noeud.Genre == 29
                && noeud.HachageNom == HacherTexte("Secret")
                && (declarationCible.Drapeaux & 16U) != 0;
            accesProtegeHeriteAutorise |= noeud.Genre == 29
                && (noeud.HachageNom == HacherTexte("Protegee")
                    || noeud.HachageNom == HacherTexte("LireProtegee"))
                && (declarationCible.Drapeaux & 8U) != 0
                && (resolution.Drapeaux & 16U) != 0;
        }
        Exiger(
            nombreConstructeurs == 2
                && constructeurDefaut
                && constructeurEntier32,
            "la sélection des constructeurs locaux est incorrecte");
        Exiger(
            nombreBasesImplicites == 2,
            "les constructeurs de base implicites sont incomplets");
        Exiger(
            nombreOperateurs == 3
                && operateurEntier32
                && operateurEntier64
                && operateurUnaire,
            "la sélection des opérateurs membres est incorrecte");
        Exiger(
            accesPriveAutorise && accesProtegeHeriteAutorise,
            "un accès privé ou protégé valide n’a pas été conservé");

        const std::string operateursLibresFrancais =
            "espace Calcul {\n"
            "  classe ValeurLibre { publique: constructeur() {} };\n"
            "  publique entier32 opérateur +(constante ValeurLibre& gauche, "
            "entier32 droite) { retourner droite; }\n"
            "  publique entier64 opérateur +(constante ValeurLibre& gauche, "
            "entier64 droite) { retourner droite; }\n"
            "  publique entier32 opérateur +(entier32 gauche, "
            "constante ValeurLibre& droite) { retourner gauche; }\n"
            "  publique booléen opérateur !(constante ValeurLibre& valeur) { "
            "retourner faux; }\n"
            "  publique vide TesterLibre(entier32 valeur) {\n"
            "    ValeurLibre objet;\n"
            "    entier32 somme32 = objet + valeur;\n"
            "    entier64 somme64 = objet + convertir<entier64>(valeur);\n"
            "    entier32 inverse = valeur + objet;\n"
            "    booléen negation = !objet;\n"
            "  }\n"
            "}\n";
        const std::string operateursLibresAnglais =
            "namespace Calcul {\n"
            "  class ValeurLibre { public: constructor() {} };\n"
            "  public int32 operator +(const ValeurLibre& gauche, "
            "int32 droite) { return droite; }\n"
            "  public int64 operator +(const ValeurLibre& gauche, "
            "int64 droite) { return droite; }\n"
            "  public int32 operator +(int32 gauche, "
            "const ValeurLibre& droite) { return gauche; }\n"
            "  public bool operator !(const ValeurLibre& valeur) { "
            "return false; }\n"
            "  public void TesterLibre(int32 valeur) {\n"
            "    ValeurLibre objet;\n"
            "    int32 somme32 = objet + valeur;\n"
            "    int64 somme64 = objet + cast<int64>(valeur);\n"
            "    int32 inverse = valeur + objet;\n"
            "    bool negation = !objet;\n"
            "  }\n"
            "}\n";
        const auto resultatOperateursLibresFrancais =
            AnalyserSemantiqueValide(
                syntaxe,
                semantique,
                operateursLibresFrancais,
                "operateurs-libres-francais");
        const auto resultatOperateursLibresAnglais =
            AnalyserSemantiqueValide(
                syntaxe,
                semantique,
                operateursLibresAnglais,
                "operateurs-libres-anglais");
        Exiger(
            resultatOperateursLibresFrancais.Symboles.size()
                    == resultatOperateursLibresAnglais.Symboles.size()
                && resultatOperateursLibresFrancais.Resolutions.size()
                    == resultatOperateursLibresAnglais.Resolutions.size(),
            "les opérateurs libres bilingues divergent");
        std::size_t nombreOperateursLibres = 0;
        std::size_t nombreOperateursLibresSurcharges = 0;
        bool operateurLibreUnaire = false;
        bool operateurLibreDroite = false;
        bool retourLibreEntier32 = false;
        bool retourLibreEntier64 = false;
        for (const auto& resolution :
             resultatOperateursLibresFrancais.Resolutions)
        {
            if ((resolution.Drapeaux & 256U) == 0
                || (resolution.Drapeaux & 32U) != 0)
                continue;
            ++nombreOperateursLibres;
            const auto& expressionLibre =
                resultatOperateursLibresFrancais.Noeuds[
                    resolution.IndexNoeud];
            const auto& symboleLibre =
                resultatOperateursLibresFrancais.Symboles[
                    resolution.IndexSymbole];
            const auto& declarationLibre =
                resultatOperateursLibresFrancais.Noeuds[
                    symboleLibre.IndexNoeud];
            Exiger(
                symboleLibre.IndexPortee == 0
                    && declarationLibre.Genre == 15
                    && (resolution.Drapeaux & 8U) == 0,
                "un opérateur libre a été marqué comme membre");
            nombreOperateursLibresSurcharges +=
                (resolution.Drapeaux & 1U) != 0;
            operateurLibreUnaire |= expressionLibre.Genre == 25;
            retourLibreEntier32 |= resolution.HachageType == typeEntier32;
            retourLibreEntier64 |= resolution.HachageType == typeEntier64;
            if (expressionLibre.Genre == 26)
            {
                const auto gaucheLibre = std::find_if(
                    resultatOperateursLibresFrancais.Noeuds.begin(),
                    resultatOperateursLibresFrancais.Noeuds.end(),
                    [&](const NoeudDeclarationHote& noeud)
                    {
                        return noeud.Parent == resolution.IndexNoeud;
                    });
                operateurLibreDroite |=
                    gaucheLibre
                        != resultatOperateursLibresFrancais.Noeuds.end()
                    && gaucheLibre->Genre == 24
                    && gaucheLibre->HachageNom == HacherTexte("valeur");
            }
        }
        Exiger(
            nombreOperateursLibres == 4
                && nombreOperateursLibresSurcharges == 3
                && operateurLibreUnaire
                && operateurLibreDroite
                && retourLibreEntier32
                && retourLibreEntier64,
            "la sélection typée des opérateurs libres est incomplète");

        const std::string intrinsequesFrancais =
            "publique entier32 IdentiteIntrinseque(entier32 valeur) { "
            "retourner valeur; }\n"
            "publique vide TesterIntrinseques(entier32 signe, naturel32 bits, "
            "entier64 large, entier32* pointeur) {\n"
            "  booléen etat = vrai;\n"
            "  pointeur_fonction<entier32(entier32)> rappel = "
            "&IdentiteIntrinseque;\n"
            "  entier32 plusUnaire = +signe; entier32 moinsUnaire = -signe;\n"
            "  naturel32 inverse = ~bits; booléen negation = !pointeur;\n"
            "  entier64 adapteGauche = 1 + large;\n"
            "  entier64 adapteDroite = large + 1;\n"
            "  entier32 somme = signe + 1; entier32 difference = signe - 1;\n"
            "  entier32 produit = signe * 2; entier32 quotient = signe / 2;\n"
            "  entier32 reste = signe % 2; naturel32 gauche = bits << 2;\n"
            "  naturel32 droite = bits >> 1; naturel32 etBits = bits & 3;\n"
            "  naturel32 ouBits = bits | 4; naturel32 xorBits = bits ^ 5;\n"
            "  booléen etLogique = signe && pointeur;\n"
            "  booléen ouLogique = etat || signe;\n"
            "  booléen egal = signe == 0; booléen different = signe != 0;\n"
            "  booléen inferieur = signe < 1; booléen inferieurEgal = signe <= 1;\n"
            "  booléen superieur = signe > -1; booléen superieurEgal = signe >= -1;\n"
            "  booléen pointeursEgaux = pointeur == pointeur;\n"
            "  booléen rappelNul = !rappel;\n"
            "  booléen rappelsEgaux = rappel == rappel;\n"
            "  booléen booleensDifferents = etat != faux;\n"
            "}\n";
        const std::string intrinsequesAnglais =
            "public int32 IdentiteIntrinseque(int32 valeur) { "
            "return valeur; }\n"
            "public void TesterIntrinseques(int32 signe, uint32 bits, "
            "int64 large, int32* pointeur) {\n"
            "  bool etat = true;\n"
            "  function_pointer<int32(int32)> rappel = "
            "&IdentiteIntrinseque;\n"
            "  int32 plusUnaire = +signe; int32 moinsUnaire = -signe;\n"
            "  uint32 inverse = ~bits; bool negation = !pointeur;\n"
            "  int64 adapteGauche = 1 + large;\n"
            "  int64 adapteDroite = large + 1;\n"
            "  int32 somme = signe + 1; int32 difference = signe - 1;\n"
            "  int32 produit = signe * 2; int32 quotient = signe / 2;\n"
            "  int32 reste = signe % 2; uint32 gauche = bits << 2;\n"
            "  uint32 droite = bits >> 1; uint32 etBits = bits & 3;\n"
            "  uint32 ouBits = bits | 4; uint32 xorBits = bits ^ 5;\n"
            "  bool etLogique = signe && pointeur;\n"
            "  bool ouLogique = etat || signe;\n"
            "  bool egal = signe == 0; bool different = signe != 0;\n"
            "  bool inferieur = signe < 1; bool inferieurEgal = signe <= 1;\n"
            "  bool superieur = signe > -1; bool superieurEgal = signe >= -1;\n"
            "  bool pointeursEgaux = pointeur == pointeur;\n"
            "  bool rappelNul = !rappel;\n"
            "  bool rappelsEgaux = rappel == rappel;\n"
            "  bool booleensDifferents = etat != false;\n"
            "}\n";
        const auto resultatIntrinsequesFrancais = AnalyserSemantiqueValide(
            syntaxe,
            semantique,
            intrinsequesFrancais,
            "intrinseques-francais");
        const auto resultatIntrinsequesAnglais = AnalyserSemantiqueValide(
            syntaxe,
            semantique,
            intrinsequesAnglais,
            "intrinseques-anglais");
        Exiger(
            resultatIntrinsequesFrancais.Symboles.size()
                    == resultatIntrinsequesAnglais.Symboles.size()
                && resultatIntrinsequesFrancais.Resolutions.size()
                    == resultatIntrinsequesAnglais.Resolutions.size(),
            "les opérateurs intrinsèques bilingues divergent");

        const std::string initialisationConstructeursFrancais =
            "classe BaseInitialisee {\n"
            "  protégée: constructeur(entier32 valeur) {}\n"
            "};\n"
            "classe ObjetInitialise : publique BaseInitialisee {\n"
            "  privée: entier32 Premier; entier32 Second;\n"
            "  publique:\n"
            "    constructeur() : soi(1) {}\n"
            "    constructeur(entier32 valeur)\n"
            "      : parent(valeur), Premier(valeur), Second(valeur + 1) {}\n"
            "};\n"
            "publique vide ConstruireInitialise() { ObjetInitialise objet; }\n";
        const std::string initialisationConstructeursAnglais =
            "class BaseInitialisee {\n"
            "  protected: constructor(int32 valeur) {}\n"
            "};\n"
            "class ObjetInitialise : public BaseInitialisee {\n"
            "  private: int32 Premier; int32 Second;\n"
            "  public:\n"
            "    constructor() : this(1) {}\n"
            "    constructor(int32 valeur)\n"
            "      : super(valeur), Premier(valeur), Second(valeur + 1) {}\n"
            "};\n"
            "public void ConstruireInitialise() { ObjetInitialise objet; }\n";
        const auto resultatInitialisationFrancais = AnalyserSemantiqueValide(
            syntaxe,
            semantique,
            initialisationConstructeursFrancais,
            "initialisation-constructeurs-francais");
        const auto resultatInitialisationAnglais = AnalyserSemantiqueValide(
            syntaxe,
            semantique,
            initialisationConstructeursAnglais,
            "initialisation-constructeurs-anglais");
        Exiger(
            resultatInitialisationFrancais.Symboles.size()
                    == resultatInitialisationAnglais.Symboles.size()
                && resultatInitialisationFrancais.Resolutions.size()
                    == resultatInitialisationAnglais.Resolutions.size(),
            "les initialisations de constructeurs bilingues divergent");
        std::size_t nombreDelegationsConstructeurs = 0;
        std::size_t nombreInitialisationsBase = 0;
        std::size_t nombreInitialisationsChamps = 0;
        for (const auto& resolution :
             resultatInitialisationFrancais.Resolutions)
        {
            const auto& noeud = resultatInitialisationFrancais.Noeuds[
                resolution.IndexNoeud];
            const auto& cible = resultatInitialisationFrancais.Symboles[
                resolution.IndexSymbole];
            const auto& declarationCible =
                resultatInitialisationFrancais.Noeuds[cible.IndexNoeud];
            if (noeud.Genre == 33)
            {
                ++nombreDelegationsConstructeurs;
                Exiger(
                    (resolution.Drapeaux & (64U | 128U | 512U))
                            == (64U | 128U | 512U)
                        && declarationCible.Genre == 13,
                    "la délégation ne cible pas un constructeur local");
            }
            else if (noeud.Genre == 34)
            {
                ++nombreInitialisationsBase;
                Exiger(
                    (resolution.Drapeaux & (64U | 128U | 1024U))
                            == (64U | 128U | 1024U)
                        && declarationCible.Genre == 13,
                    "l’initialisation de base ne cible pas son constructeur");
            }
            else if (noeud.Genre == 35)
            {
                ++nombreInitialisationsChamps;
                Exiger(
                    (resolution.Drapeaux & (8U | 2048U))
                            == (8U | 2048U)
                        && declarationCible.Genre == 7,
                    "l’initialisation de champ ne cible pas un champ direct");
            }
        }
        Exiger(
            nombreDelegationsConstructeurs == 1
                && nombreInitialisationsBase == 1
                && nombreInitialisationsChamps == 2,
            "les résolutions d’initialisation de constructeurs sont incomplètes");

        const std::string sousObjetsFrancais =
            "classe BaseAutomatique {\n"
            "  publique: constructeur() {}\n"
            "};\n"
            "classe Composant {\n"
            "  publique: constructeur() {} "
            "constructeur(entier32 valeur) {}\n"
            "};\n"
            "classe Conteneur : publique BaseAutomatique {\n"
            "  privée: Composant Element; constante entier32 Code; "
            "entier8 Petit; entier8 Negatif;\n"
            "  publique: constructeur(entier32 valeur)\n"
            "    : Element(valeur), Code(valeur + 1), Petit(127), "
            "Negatif(-128) {}\n"
            "};\n"
            "publique vide ConstruireSousObjets(entier32 valeur) { "
            "Conteneur objet(valeur); }\n";
        const std::string sousObjetsAnglais =
            "class BaseAutomatique {\n"
            "  public: constructor() {}\n"
            "};\n"
            "class Composant {\n"
            "  public: constructor() {} constructor(int32 valeur) {}\n"
            "};\n"
            "class Conteneur : public BaseAutomatique {\n"
            "  private: Composant Element; const int32 Code; int8 Petit; "
            "int8 Negatif;\n"
            "  public: constructor(int32 valeur)\n"
            "    : Element(valeur), Code(valeur + 1), Petit(127), "
            "Negatif(-128) {}\n"
            "};\n"
            "public void ConstruireSousObjets(int32 valeur) { "
            "Conteneur objet(valeur); }\n";
        const auto resultatSousObjetsFrancais = AnalyserSemantiqueValide(
            syntaxe,
            semantique,
            sousObjetsFrancais,
            "sous-objets-francais");
        const auto resultatSousObjetsAnglais = AnalyserSemantiqueValide(
            syntaxe,
            semantique,
            sousObjetsAnglais,
            "sous-objets-anglais");
        Exiger(
            resultatSousObjetsFrancais.Symboles.size()
                    == resultatSousObjetsAnglais.Symboles.size()
                && resultatSousObjetsFrancais.Resolutions.size()
                    == resultatSousObjetsAnglais.Resolutions.size(),
            "les constructions de sous-objets bilingues divergent");

        std::size_t nombreChampsSousObjets = 0;
        std::size_t nombreConstructeursChamps = 0;
        std::size_t nombreBasesSousObjets = 0;
        for (const auto& resolution : resultatSousObjetsFrancais.Resolutions)
        {
            const auto& noeud = resultatSousObjetsFrancais.Noeuds[
                resolution.IndexNoeud];
            const auto& cible = resultatSousObjetsFrancais.Symboles[
                resolution.IndexSymbole];
            const auto& declarationCible = resultatSousObjetsFrancais.Noeuds[
                cible.IndexNoeud];
            if (noeud.Genre == 35
                && (resolution.Drapeaux & (8U | 2048U))
                    == (8U | 2048U))
            {
                ++nombreChampsSousObjets;
                Exiger(
                    declarationCible.Genre == 7,
                    "un initialiseur de sous-objet ne cible pas son champ");
            }
            if (noeud.Genre == 35
                && (resolution.Drapeaux & (64U | 128U | 2048U))
                    == (64U | 128U | 2048U))
            {
                ++nombreConstructeursChamps;
                Exiger(
                    declarationCible.Genre == 13,
                    "un champ objet ne cible pas son constructeur");
            }
            if (noeud.Genre == 13
                && (resolution.Drapeaux & (64U | 1024U))
                    == (64U | 1024U)
                && (resolution.Drapeaux & 128U) == 0)
            {
                ++nombreBasesSousObjets;
                Exiger(
                    declarationCible.Genre == 13,
                    "la construction implicite de base cible un symbole invalide");
            }
        }
        Exiger(
            nombreChampsSousObjets == 4
                && nombreConstructeursChamps == 1
                && nombreBasesSousObjets == 1,
            "les résolutions des sous-objets sont incomplètes");

        const std::string champsImplicitesFrancais =
            "classe ElementImplicite {\n"
            "  publique: constructeur() {} "
            "constructeur(entier32 valeur) {}\n"
            "};\n"
            "classe ConfigurationImplicite {\n"
            "  privée: ElementImplicite Objet; "
            "ElementImplicite Elements[2];\n"
            "  entier32 Code = 7; constante entier32 Limite = 9; "
            "entier32 Codes[3] = {1, 2}; entier32 Remplacee = 5;\n"
            "  publique: constructeur() {}\n"
            "  constructeur(entier32 valeur) "
            ": Elements(valeur), Codes({valeur, 2}), "
            "Remplacee(valeur) {}\n"
            "};\n"
            "publique vide ConstruireConfiguration(entier32 valeur) { "
            "ConfigurationImplicite a; ConfigurationImplicite b(valeur); "
            "ElementImplicite locaux[2](valeur); "
            "ElementImplicite defauts[2]; "
            "ElementImplicite grille /* dimensions */ [1_0][2]; }\n";
        const std::string champsImplicitesAnglais =
            "class ElementImplicite {\n"
            "  public: constructor() {} constructor(int32 valeur) {}\n"
            "};\n"
            "class ConfigurationImplicite {\n"
            "  private: ElementImplicite Objet; "
            "ElementImplicite Elements[2];\n"
            "  int32 Code = 7; const int32 Limite = 9; "
            "int32 Codes[3] = {1, 2}; int32 Remplacee = 5;\n"
            "  public: constructor() {}\n"
            "  constructor(int32 valeur) "
            ": Elements(valeur), Codes({valeur, 2}), "
            "Remplacee(valeur) {}\n"
            "};\n"
            "public void ConstruireConfiguration(int32 valeur) { "
            "ConfigurationImplicite a; ConfigurationImplicite b(valeur); "
            "ElementImplicite locaux[2](valeur); "
            "ElementImplicite defauts[2]; "
            "ElementImplicite grille /* dimensions */ [1_0][2]; }\n";
        const auto resultatChampsImplicitesFrancais =
            AnalyserSemantiqueValide(
                syntaxe,
                semantique,
                champsImplicitesFrancais,
                "champs-implicites-francais");
        const auto resultatChampsImplicitesAnglais =
            AnalyserSemantiqueValide(
                syntaxe,
                semantique,
                champsImplicitesAnglais,
                "champs-implicites-anglais");
        Exiger(
            resultatChampsImplicitesFrancais.Symboles.size()
                    == resultatChampsImplicitesAnglais.Symboles.size()
                && resultatChampsImplicitesFrancais.Resolutions.size()
                    == resultatChampsImplicitesAnglais.Resolutions.size(),
            "les champs implicites bilingues divergent");

        std::size_t nombreValeursParDefaut = 0;
        std::size_t nombreChampsObjetsImplicites = 0;
        std::size_t nombreConstructeursObjetsImplicites = 0;
        std::size_t nombreChampsTableauxExplicites = 0;
        std::size_t nombreConstructeursTableauxExplicites = 0;
        std::size_t nombreTableauxLocauxExplicites = 0;
        std::size_t nombreTableauxLocauxImplicites = 0;
        for (const auto& resolution :
             resultatChampsImplicitesFrancais.Resolutions)
        {
            const auto& noeud = resultatChampsImplicitesFrancais.Noeuds[
                resolution.IndexNoeud];
            const auto& cible = resultatChampsImplicitesFrancais.Symboles[
                resolution.IndexSymbole];
            const auto& declarationCible =
                resultatChampsImplicitesFrancais.Noeuds[cible.IndexNoeud];
            if (noeud.Genre == 13
                && declarationCible.Genre == 7
                && (resolution.Drapeaux
                    & (8U | 2048U | 4096U | 16384U))
                    == (8U | 2048U | 4096U | 16384U))
                ++nombreValeursParDefaut;
            if (noeud.Genre == 13
                && declarationCible.Genre == 7
                && (resolution.Drapeaux & (8U | 2048U | 4096U))
                    == (8U | 2048U | 4096U)
                && (resolution.Drapeaux & 16384U) == 0)
                ++nombreChampsObjetsImplicites;
            if (noeud.Genre == 13
                && declarationCible.Genre == 13
                && (resolution.Drapeaux & (64U | 2048U | 4096U))
                    == (64U | 2048U | 4096U))
            {
                ++nombreConstructeursObjetsImplicites;
                Exiger(
                    nombreParametresCible(
                        resultatChampsImplicitesFrancais,
                        resolution) == 0,
                    "un champ implicite ne cible pas le constructeur sans argument");
            }
            if (noeud.Genre == 35
                && declarationCible.Genre == 7
                && (resolution.Drapeaux & (8U | 2048U | 8192U))
                    == (8U | 2048U | 8192U))
                ++nombreChampsTableauxExplicites;
            if (noeud.Genre == 35
                && declarationCible.Genre == 13
                && (resolution.Drapeaux & (64U | 128U | 2048U | 8192U))
                    == (64U | 128U | 2048U | 8192U))
            {
                ++nombreConstructeursTableauxExplicites;
                Exiger(
                    nombreParametresCible(
                        resultatChampsImplicitesFrancais,
                        resolution) == 1,
                    "le tableau explicite ne cible pas son constructeur uniforme");
            }
            if (noeud.Genre == 19
                && declarationCible.Genre == 13
                && (resolution.Drapeaux & (64U | 128U | 8192U))
                    == (64U | 128U | 8192U))
            {
                ++nombreTableauxLocauxExplicites;
                Exiger(
                    nombreParametresCible(
                        resultatChampsImplicitesFrancais,
                        resolution) == 1,
                    "le tableau local explicite cible une mauvaise surcharge");
            }
            if (noeud.Genre == 19
                && declarationCible.Genre == 13
                && (resolution.Drapeaux & (64U | 4096U | 8192U))
                    == (64U | 4096U | 8192U)
                && (resolution.Drapeaux & 128U) == 0)
            {
                ++nombreTableauxLocauxImplicites;
                Exiger(
                    nombreParametresCible(
                        resultatChampsImplicitesFrancais,
                        resolution) == 0,
                    "le tableau local implicite cible une mauvaise surcharge");
            }
        }
        Exiger(
            nombreValeursParDefaut == 6
                && nombreChampsObjetsImplicites == 3
                && nombreConstructeursObjetsImplicites == 3
                && nombreChampsTableauxExplicites == 2
                && nombreConstructeursTableauxExplicites == 1
                && nombreTableauxLocauxExplicites == 1
                && nombreTableauxLocauxImplicites == 2,
            "les résolutions des champs implicites sont incomplètes");

        const std::string dureeVieFrancais =
            "classe RacineDureeVie {\n"
            "  protégée: entier8 Marque;\n"
            "  publique: constructeur() {} destructeur() {}\n"
            "  virtuel entier32 Lire() { retourner 1; }\n"
            "};\n"
            "classe ElementDureeVie {\n"
            "  privée: entier32 Valeur;\n"
            "  publique: constructeur() {} destructeur() {}\n"
            "};\n"
            "classe IntermediaireDureeVie : publique RacineDureeVie {\n"
            "  privée: ElementDureeVie Premier; ElementDureeVie Elements[2]; "
            "entier8 Fin; pointeur_fonction<entier32(entier32)> Rappel;\n"
            "};\n"
            "classe HoteDureeVie : publique IntermediaireDureeVie {\n"
            "  privée: ElementDureeVie Champ;\n"
            "  publique: constructeur() {} destructeur() {}\n"
            "};\n"
            "publique vide TesterDureeVie() {\n"
            "  HoteDureeVie objet; ElementDureeVie tableau[3]; "
            "IntermediaireDureeVie implicite;\n"
            "}\n";
        const std::string dureeVieAnglais =
            "class RacineDureeVie {\n"
            "  protected: int8 Marque;\n"
            "  public: constructor() {} destructor() {}\n"
            "  virtual int32 Lire() { return 1; }\n"
            "};\n"
            "class ElementDureeVie {\n"
            "  private: int32 Valeur;\n"
            "  public: constructor() {} destructor() {}\n"
            "};\n"
            "class IntermediaireDureeVie : public RacineDureeVie {\n"
            "  private: ElementDureeVie Premier; ElementDureeVie Elements[2]; "
            "int8 Fin; function_pointer<int32(int32)> Rappel;\n"
            "};\n"
            "class HoteDureeVie : public IntermediaireDureeVie {\n"
            "  private: ElementDureeVie Champ;\n"
            "  public: constructor() {} destructor() {}\n"
            "};\n"
            "public void TesterDureeVie() {\n"
            "  HoteDureeVie objet; ElementDureeVie tableau[3]; "
            "IntermediaireDureeVie implicite;\n"
            "}\n";
        const auto resultatDureeVieFrancais = AnalyserSemantiqueValide(
            syntaxe,
            semantique,
            dureeVieFrancais,
            "duree-vie-francais");
        const auto resultatDureeVieAnglais = AnalyserSemantiqueValide(
            syntaxe,
            semantique,
            dureeVieAnglais,
            "duree-vie-anglais");
        Exiger(
            resultatDureeVieFrancais.Symboles.size()
                    == resultatDureeVieAnglais.Symboles.size()
                && resultatDureeVieFrancais.Resolutions.size()
                    == resultatDureeVieAnglais.Resolutions.size(),
            "les plans de durée de vie bilingues divergent");
        for (std::size_t index = 0;
             index < resultatDureeVieFrancais.Resolutions.size();
             ++index)
        {
            const auto& resolutionDureeVieFrancais =
                resultatDureeVieFrancais.Resolutions[index];
            const auto& resolutionDureeVieAnglais =
                resultatDureeVieAnglais.Resolutions[index];
            Exiger(
                resolutionDureeVieFrancais.IndexNoeud
                        == resolutionDureeVieAnglais.IndexNoeud
                    && resolutionDureeVieFrancais.IndexSymbole
                        == resolutionDureeVieAnglais.IndexSymbole
                    && resolutionDureeVieFrancais.HachageType
                        == resolutionDureeVieAnglais.HachageType
                    && resolutionDureeVieFrancais.GenreCible
                        == resolutionDureeVieAnglais.GenreCible
                    && resolutionDureeVieFrancais.Drapeaux
                        == resolutionDureeVieAnglais.Drapeaux,
                "une étape de durée de vie diffère entre français et anglais");
        }

        const auto plansPourNom =
            [&](std::string_view nom)
        {
            const auto hachage = HacherTexte(nom);
            const auto noeud = std::find_if(
                resultatDureeVieFrancais.Noeuds.begin(),
                resultatDureeVieFrancais.Noeuds.end(),
                [&](const NoeudDeclarationHote& valeur)
                {
                    return valeur.Genre == 19
                        && valeur.HachageNom == hachage;
                });
            Exiger(
                noeud != resultatDureeVieFrancais.Noeuds.end(),
                "variable locale de durée de vie introuvable");
            const auto indexNoeud = static_cast<std::uint64_t>(
                std::distance(
                    resultatDureeVieFrancais.Noeuds.begin(), noeud));
            std::vector<const ResolutionSemantiqueHote*> plans;
            for (const auto& resolution : resultatDureeVieFrancais.Resolutions)
                if (resolution.IndexNoeud == indexNoeud
                    && (resolution.Drapeaux & 32768U) != 0)
                    plans.push_back(&resolution);
            return plans;
        };
        const auto exigerPlan =
            [&](const std::vector<const ResolutionSemantiqueHote*>& plans,
                const std::vector<std::uint64_t>& decalages,
                const std::vector<std::uint32_t>& genres,
                std::string_view nom)
        {
            Exiger(
                plans.size() == decalages.size()
                    && plans.size() == genres.size(),
                "nombre d’étapes incorrect pour " + std::string(nom));
            for (std::size_t index = 0; index < plans.size(); ++index)
            {
                const auto& cible = resultatDureeVieFrancais.Symboles[
                    plans[index]->IndexSymbole];
                const auto& declaration = resultatDureeVieFrancais.Noeuds[
                    cible.IndexNoeud];
                Exiger(
                    plans[index]->HachageType == decalages[index]
                        && declaration.Genre == genres[index],
                    "ordre, cible ou décalage incorrect pour "
                        + std::string(nom));
            }
        };

        const auto plansObjet = plansPourNom("objet");
        exigerPlan(
            plansObjet,
            {0, 0, 40, 24, 20, 16, 0},
            {13, 14, 14, 14, 14, 14, 14},
            "l’objet dérivé");
        Exiger(
            (plansObjet[0]->Drapeaux & 65536U) != 0
                && (plansObjet[1]->Drapeaux & 131072U) != 0
                && (plansObjet[2]->Drapeaux & 1048576U) != 0
                && (plansObjet[3]->Drapeaux & 2097152U) != 0
                && (plansObjet[6]->Drapeaux & 524288U) != 0,
            "les catégories du plan de l’objet dérivé sont incomplètes");

        const auto plansTableau = plansPourNom("tableau");
        exigerPlan(
            plansTableau,
            {0, 4, 8, 8, 4, 0},
            {13, 13, 13, 14, 14, 14},
            "le tableau local");
        Exiger(
            std::all_of(
                plansTableau.begin(),
                plansTableau.end(),
                [](const ResolutionSemantiqueHote* resolution)
                { return (resolution->Drapeaux & 2097152U) != 0; }),
            "une étape du tableau n’est pas identifiée comme élément");

        const auto plansImplicites = plansPourNom("implicite");
        exigerPlan(
            plansImplicites,
            {0, 0, 16, 20, 24, 24, 20, 16, 0},
            {13, 6, 13, 13, 13, 14, 14, 14, 14},
            "l’objet sans constructeur propre");
        Exiger(
            (plansImplicites[1]->Drapeaux & (65536U | 262144U))
                    == (65536U | 262144U)
                && (plansImplicites[1]->Drapeaux & 524288U) == 0,
            "le remplacement de table virtuelle intermédiaire est incorrect");

        const auto typeHote = std::find_if(
            resultatDureeVieFrancais.Noeuds.begin(),
            resultatDureeVieFrancais.Noeuds.end(),
            [&](const NoeudDeclarationHote& noeud)
            {
                return noeud.Genre == 6
                    && noeud.HachageNom == HacherTexte("HoteDureeVie");
            });
        Exiger(
            typeHote != resultatDureeVieFrancais.Noeuds.end(),
            "type hôte du plan de constructeur introuvable");
        const auto indexTypeHote = static_cast<std::uint64_t>(
            std::distance(
                resultatDureeVieFrancais.Noeuds.begin(), typeHote));
        const auto constructeurHote = std::find_if(
            resultatDureeVieFrancais.Noeuds.begin(),
            resultatDureeVieFrancais.Noeuds.end(),
            [&](const NoeudDeclarationHote& noeud)
            { return noeud.Genre == 13 && noeud.Parent == indexTypeHote; });
        Exiger(
            constructeurHote != resultatDureeVieFrancais.Noeuds.end(),
            "constructeur hôte du plan introuvable");
        const auto indexConstructeurHote = static_cast<std::uint64_t>(
            std::distance(
                resultatDureeVieFrancais.Noeuds.begin(), constructeurHote));
        std::vector<const ResolutionSemantiqueHote*> plansConstructeurHote;
        for (const auto& resolution : resultatDureeVieFrancais.Resolutions)
            if (resolution.IndexNoeud == indexConstructeurHote
                && (resolution.Drapeaux & 32768U) != 0)
                plansConstructeurHote.push_back(&resolution);
        exigerPlan(
            plansConstructeurHote,
            {0, 0, 16, 20, 24, 0, 40},
            {13, 6, 13, 13, 13, 6, 13},
            "le constructeur de l’objet dérivé");
        Exiger(
            (plansConstructeurHote[1]->Drapeaux & (262144U | 524288U))
                    == (262144U | 524288U)
                && (plansConstructeurHote[5]->Drapeaux & 262144U) != 0
                && (plansConstructeurHote[5]->Drapeaux & 524288U) == 0,
            "l’ordre des tables virtuelles de base et dérivée est incorrect");

        const std::string agregatsRecursifsFrancais =
            "structure CoordonneesAgregat { entier32 X; "
            "entier32 Valeurs[2]; };\n"
            "union ChoixAgregat { entier32 Entier; booléen Etat; };\n"
            "classe ConteneurAgregat { privée: entier32 Grille[2][2] = "
            "{{1}, {2}}; publique: constructeur() {} "
            "constructeur(entier32 valeur) : "
            "Grille({{valeur}, {2}}) {} };\n"
            "publique entier32 MatriceGlobale[2][2] = "
            "{{1, 2}, {3}};\n"
            "publique CoordonneesAgregat PositionGlobale = "
            "{7, {8, 9}};\n"
            "publique vide TesterAgregatsRecursifs() { "
            "entier32 matrice[2][3] = {{1, 2}, {3}}; "
            "CoordonneesAgregat position = {4, {5, 6}}; "
            "ChoixAgregat choix = {9}; "
            "entier32 scalaire = {{{10}}}; ConteneurAgregat defaut; "
            "ConteneurAgregat explicite(3); }\n";
        const std::string agregatsRecursifsAnglais =
            "struct CoordonneesAgregat { int32 X; int32 Valeurs[2]; };\n"
            "union ChoixAgregat { int32 Entier; bool Etat; };\n"
            "class ConteneurAgregat { private: int32 Grille[2][2] = "
            "{{1}, {2}}; public: constructor() {} "
            "constructor(int32 valeur) : Grille({{valeur}, {2}}) {} };\n"
            "public int32 MatriceGlobale[2][2] = {{1, 2}, {3}};\n"
            "public CoordonneesAgregat PositionGlobale = "
            "{7, {8, 9}};\n"
            "public void TesterAgregatsRecursifs() { "
            "int32 matrice[2][3] = {{1, 2}, {3}}; "
            "CoordonneesAgregat position = {4, {5, 6}}; "
            "ChoixAgregat choix = {9}; int32 scalaire = {{{10}}}; "
            "ConteneurAgregat defaut; ConteneurAgregat explicite(3); }\n";
        const auto resultatAgregatsFrancais = AnalyserSemantiqueValide(
            syntaxe,
            semantique,
            agregatsRecursifsFrancais,
            "agregats-recursifs-francais");
        const auto resultatAgregatsAnglais = AnalyserSemantiqueValide(
            syntaxe,
            semantique,
            agregatsRecursifsAnglais,
            "agregats-recursifs-anglais");
        std::size_t nombreAgregatsFrancais = 0;
        std::size_t nombreAgregatsAnglais = 0;
        for (const auto& noeud : resultatAgregatsFrancais.Noeuds)
            if (noeud.Genre == 32) ++nombreAgregatsFrancais;
        for (const auto& noeud : resultatAgregatsAnglais.Noeuds)
            if (noeud.Genre == 32) ++nombreAgregatsAnglais;
        Exiger(
            nombreAgregatsFrancais == 20
                && nombreAgregatsAnglais == nombreAgregatsFrancais
                && resultatAgregatsFrancais.Symboles.size()
                    == resultatAgregatsAnglais.Symboles.size()
                && resultatAgregatsFrancais.Resolutions.size()
                    == resultatAgregatsAnglais.Resolutions.size(),
            "les agrégats récursifs bilingues divergent");

        const std::string expressionsRecursivesFrancais =
            "classe ValeurRecursive {\n"
            "  privée: entier32 Valeur;\n"
            "  publique: constructeur(entier32 valeur) : Valeur(valeur) {}\n"
            "  entier32 Lire() { retourner soi.Valeur; }\n"
            "  entier64 opérateur +(entier32 delta) { "
            "retourner convertir<entier64>(soi.Valeur + delta); }\n"
            "};\n"
            "publique entier32 Produire(entier32 valeur) { retourner valeur; }\n"
            "publique entier64 Produire(entier64 valeur) { retourner valeur; }\n"
            "publique vide TesterExpressionsRecursives() {\n"
            "  ValeurRecursive objet(4);\n"
            "  entier32 entiers[3] = {Produire(objet.Lire()), "
            "objet.Lire(), Produire(1)};\n"
            "  entier64 longs[2] = {Produire(objet + 2), objet + 3};\n"
            "  booléen etats[2] = {Produire(1) == 1, !faux};\n"
            "}\n";
        const std::string expressionsRecursivesAnglais =
            "class ValeurRecursive {\n"
            "  private: int32 Valeur;\n"
            "  public: constructor(int32 valeur) : Valeur(valeur) {}\n"
            "  int32 Lire() { return this.Valeur; }\n"
            "  int64 operator +(int32 delta) { "
            "return cast<int64>(this.Valeur + delta); }\n"
            "};\n"
            "public int32 Produire(int32 valeur) { return valeur; }\n"
            "public int64 Produire(int64 valeur) { return valeur; }\n"
            "public void TesterExpressionsRecursives() {\n"
            "  ValeurRecursive objet(4);\n"
            "  int32 entiers[3] = {Produire(objet.Lire()), "
            "objet.Lire(), Produire(1)};\n"
            "  int64 longs[2] = {Produire(objet + 2), objet + 3};\n"
            "  bool etats[2] = {Produire(1) == 1, !false};\n"
            "}\n";
        const auto resultatExpressionsFrancais = AnalyserSemantiqueValide(
            syntaxe,
            semantique,
            expressionsRecursivesFrancais,
            "expressions-recursives-francais");
        const auto resultatExpressionsAnglais = AnalyserSemantiqueValide(
            syntaxe,
            semantique,
            expressionsRecursivesAnglais,
            "expressions-recursives-anglais");
        Exiger(
            resultatExpressionsFrancais.Symboles.size()
                    == resultatExpressionsAnglais.Symboles.size()
                && resultatExpressionsFrancais.Resolutions.size()
                    == resultatExpressionsAnglais.Resolutions.size(),
            "les expressions récursives bilingues divergent");

        std::size_t appelsProduireEntier32 = 0;
        std::size_t appelsProduireEntier64 = 0;
        std::size_t operateursRecursifs = 0;
        for (const auto& resolution :
             resultatExpressionsFrancais.Resolutions)
        {
            const auto& noeudExpression = resultatExpressionsFrancais.Noeuds[
                resolution.IndexNoeud];
            if (noeudExpression.HachageNom == HacherTexte("Produire")
                && (resolution.Drapeaux & 1U) != 0)
            {
                const auto indexFonction = resultatExpressionsFrancais.Symboles[
                    resolution.IndexSymbole].IndexNoeud;
                const auto parametre = std::find_if(
                    resultatExpressionsFrancais.Noeuds.begin(),
                    resultatExpressionsFrancais.Noeuds.end(),
                    [&](const NoeudDeclarationHote& valeur)
                    {
                        return valeur.Genre == 2
                            && valeur.Parent == indexFonction;
                    });
                Exiger(
                    parametre != resultatExpressionsFrancais.Noeuds.end(),
                    "une surcharge Produire n’expose pas son paramètre");
                appelsProduireEntier32 +=
                    parametre->HachageType == typeEntier32;
                appelsProduireEntier64 +=
                    parametre->HachageType == typeEntier64;
            }
            if ((resolution.Drapeaux & 256U) != 0)
                ++operateursRecursifs;
        }
        Exiger(
            appelsProduireEntier32 == 3
                && appelsProduireEntier64 == 1
                && operateursRecursifs == 2,
            "la propagation récursive des appels et opérateurs est incomplète");

        const std::string adressesIndexationsFrancais =
            "publique entier32 DoublerType(entier32 valeur) { "
            "retourner valeur * 2; }\n"
            "publique vide IncrementerType(entier32& valeur) { "
            "valeur = valeur + 1; }\n"
            "publique entier32 ChoisirType(entier32 valeur) { "
            "retourner valeur; }\n"
            "publique entier64 ChoisirType(entier64 valeur) { "
            "retourner valeur; }\n"
            "publique vide TesterAdressesIndexations() {\n"
            "  entier32 valeur = 7; entier32 matrice[2][2] = {{1, 2}, {3, 4}};\n"
            "  pointeur_fonction<entier32(entier32)> operation = &DoublerType;\n"
            "  pointeur_fonction<vide(entier32&)> mutation = &IncrementerType;\n"
            "  pointeur_fonction<entier32(entier32)>* adresseOperation = "
            "&operation;\n"
            "  pointeur_fonction<entier32(entier32)> operations[1] = "
            "{&DoublerType};\n"
            "  pointeur_fonction<pointeur_fonction<entier32(entier32)>()> "
            "fournisseur;\n"
            "  mutation(valeur);\n"
            "  entier32 appelsExplicites[2] = {"
            "(&DoublerType)(6), (*adresseOperation)(7)};\n"
            "  entier32 resultats[5] = {ChoisirType(matrice[0][1]), "
            "ChoisirType(*(&valeur)), ChoisirType(operation(3)), "
            "ChoisirType(operations[0](4)), "
            "ChoisirType(fournisseur()(5))};\n"
            "}\n";
        const std::string adressesIndexationsAnglais =
            "public int32 DoublerType(int32 valeur) { return valeur * 2; }\n"
            "public void IncrementerType(int32& valeur) { "
            "valeur = valeur + 1; }\n"
            "public int32 ChoisirType(int32 valeur) { return valeur; }\n"
            "public int64 ChoisirType(int64 valeur) { return valeur; }\n"
            "public void TesterAdressesIndexations() {\n"
            "  int32 valeur = 7; int32 matrice[2][2] = {{1, 2}, {3, 4}};\n"
            "  function_pointer<int32(int32)> operation = &DoublerType;\n"
            "  function_pointer<void(int32&)> mutation = &IncrementerType;\n"
            "  function_pointer<int32(int32)>* adresseOperation = "
            "&operation;\n"
            "  function_pointer<int32(int32)> operations[1] = "
            "{&DoublerType};\n"
            "  function_pointer<function_pointer<int32(int32)>()> "
            "fournisseur;\n"
            "  mutation(valeur);\n"
            "  int32 appelsExplicites[2] = {"
            "(&DoublerType)(6), (*adresseOperation)(7)};\n"
            "  int32 resultats[5] = {ChoisirType(matrice[0][1]), "
            "ChoisirType(*(&valeur)), ChoisirType(operation(3)), "
            "ChoisirType(operations[0](4)), "
            "ChoisirType(fournisseur()(5))};\n"
            "}\n";
        const auto resultatAdressesIndexationsFrancais =
            AnalyserSemantiqueValide(
                syntaxe,
                semantique,
                adressesIndexationsFrancais,
                "adresses-indexations-francais");
        const auto resultatAdressesIndexationsAnglais =
            AnalyserSemantiqueValide(
                syntaxe,
                semantique,
                adressesIndexationsAnglais,
                "adresses-indexations-anglais");
        Exiger(
            resultatAdressesIndexationsFrancais.Symboles.size()
                    == resultatAdressesIndexationsAnglais.Symboles.size()
                && resultatAdressesIndexationsFrancais.Resolutions.size()
                    == resultatAdressesIndexationsAnglais.Resolutions.size(),
            "les adresses, indexations et appels indirects bilingues divergent");

        std::size_t appelsChoisirTypeEntier32 = 0;
        std::size_t appelsChoisirTypeEntier64 = 0;
        for (const auto& resolution :
             resultatAdressesIndexationsFrancais.Resolutions)
        {
            const auto& noeudType = resultatAdressesIndexationsFrancais.Noeuds[
                resolution.IndexNoeud];
            if (noeudType.HachageNom == HacherTexte("ChoisirType")
                && (resolution.Drapeaux & 1U) != 0)
            {
                const auto indexFonctionType =
                    resultatAdressesIndexationsFrancais.Symboles[
                        resolution.IndexSymbole].IndexNoeud;
                const auto parametreType = std::find_if(
                    resultatAdressesIndexationsFrancais.Noeuds.begin(),
                    resultatAdressesIndexationsFrancais.Noeuds.end(),
                    [&](const NoeudDeclarationHote& valeurType)
                    {
                        return valeurType.Genre == 2
                            && valeurType.Parent == indexFonctionType;
                    });
                Exiger(
                    parametreType
                        != resultatAdressesIndexationsFrancais.Noeuds.end(),
                    "une surcharge ChoisirType n'expose pas son paramètre");
                appelsChoisirTypeEntier32 +=
                    parametreType->HachageType == typeEntier32;
                appelsChoisirTypeEntier64 +=
                    parametreType->HachageType == typeEntier64;
            }
        }
        Exiger(
            appelsChoisirTypeEntier32 == 5
                && appelsChoisirTypeEntier64 == 0,
            "la propagation des indexations, adresses, déréférencements "
            "ou appels indirects est incomplète");

        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide F() { entier32* valeur = &(1 + 2); }",
            47,
            "adresse-valeur-non-adressable-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public void F() { int32* valeur = &(1 + 2); }",
            47,
            "adresse-valeur-non-adressable-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide F() { entier32 valeurs[2] = {1, 2}; "
            "entier32* adresse = &valeurs; }",
            48,
            "adresse-tableau-complet-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public void F() { int32 valeurs[2] = {1, 2}; "
            "int32* adresse = &valeurs; }",
            48,
            "adresse-tableau-complet-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide F() { entier32 valeur = *1; }",
            49,
            "dereferencement-sans-pointeur-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public void F() { int32 valeur = *1; }",
            49,
            "dereferencement-sans-pointeur-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide F() { entier32 valeur = 1[0]; }",
            50,
            "cible-indexation-invalide-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public void F() { int32 valeur = 1[0]; }",
            50,
            "cible-indexation-invalide-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide F(entier32* valeurs) { "
            "entier32 valeur = valeurs[vrai]; }",
            51,
            "indice-non-entier-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public void F(int32* valeurs) { "
            "int32 valeur = valeurs[true]; }",
            51,
            "indice-non-entier-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide F(vide* valeurs) { octet valeur = valeurs[0]; }",
            52,
            "indexation-pointeur-vide-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public void F(void* valeurs) { byte valeur = valeurs[0]; }",
            52,
            "indexation-pointeur-vide-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide F() { entier32 valeur = 1; valeur(); }",
            53,
            "cible-appel-indirect-invalide-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public void F() { int32 valeur = 1; valeur(); }",
            53,
            "cible-appel-indirect-invalide-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique entier32 Identite(entier32 valeur) { retourner valeur; } "
            "publique vide F() { "
            "pointeur_fonction<entier32(entier32)> operation = &Identite; "
            "operation(); }",
            54,
            "arite-appel-indirect-invalide-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public int32 Identite(int32 valeur) { return valeur; } "
            "public void F() { "
            "function_pointer<int32(int32)> operation = &Identite; "
            "operation(); }",
            54,
            "arite-appel-indirect-invalide-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique entier32 Identite(entier32 valeur) { retourner valeur; } "
            "publique vide F() { "
            "pointeur_fonction<entier32(entier32)> operation = &Identite; "
            "operation(vrai); }",
            55,
            "argument-appel-indirect-incompatible-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public int32 Identite(int32 valeur) { return valeur; } "
            "public void F() { "
            "function_pointer<int32(int32)> operation = &Identite; "
            "operation(true); }",
            55,
            "argument-appel-indirect-incompatible-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide Modifier(entier32& valeur) { valeur = 1; } "
            "publique vide F() { "
            "pointeur_fonction<vide(entier32&)> operation = &Modifier; "
            "operation(1); }",
            55,
            "reference-appel-indirect-temporaire-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public void Modifier(int32& valeur) { valeur = 1; } "
            "public void F() { "
            "function_pointer<void(int32&)> operation = &Modifier; "
            "operation(1); }",
            55,
            "reference-appel-indirect-temporaire-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide F() { "
            "pointeur_fonction<entier32(entier32)>* operations; "
            "operations(1); }",
            53,
            "pointeur-vers-pointeur-fonction-non-appelable-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public void F() { "
            "function_pointer<int32(int32)>* operations; "
            "operations(1); }",
            53,
            "pointeur-vers-pointeur-fonction-non-appelable-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique entier32 Identite(entier32 valeur) { retourner valeur; } "
            "publique vide F() { "
            "pointeur_fonction<entier32(entier32)> operation = &Identite; "
            "operation[0]; }",
            50,
            "pointeur-fonction-pur-non-indexable-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public int32 Identite(int32 valeur) { return valeur; } "
            "public void F() { "
            "function_pointer<int32(int32)> operation = &Identite; "
            "operation[0]; }",
            50,
            "pointeur-fonction-pur-non-indexable-anglais");

        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique entier64 Long() { retourner 1; } "
            "publique vide F() { entier32 X[1] = {Long()}; }",
            45,
            "agregat-retour-appel-incompatible");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "classe A { publique: constructeur() {} "
            "entier64 opérateur +(entier32 valeur) { "
            "retourner convertir<entier64>(valeur); } }; "
            "publique vide F() { A objet; entier32 X[1] = {objet + 1}; }",
            45,
            "agregat-retour-operateur-incompatible");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide F() { entier32 X[1] = {1 == 1}; }",
            45,
            "agregat-retour-comparaison-incompatible");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide F() { entier64 source[1] = {1}; "
            "entier32 cible[1] = {source[0]}; }",
            45,
            "agregat-indexation-incompatible");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide F() { entier64 valeur = 1; "
            "entier32* cible[1] = {&valeur}; }",
            45,
            "agregat-adresse-incompatible");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide F(entier64* valeur) { "
            "entier32 cible[1] = {*valeur}; }",
            45,
            "agregat-dereferencement-incompatible");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique entier64 LongCallback(entier64 valeur) { retourner valeur; } "
            "publique vide F() { "
            "pointeur_fonction<entier64(entier64)> operation = &LongCallback; "
            "entier32 cible[1] = {operation(1)}; }",
            45,
            "agregat-appel-indirect-incompatible");

        ComparerErreurSemantique(
            syntaxe, semantique,
            "publique vide F() { entier32 X[2] = {1, 2, 3}; }",
            42, "agregat-tableau-local-trop-long");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "publique vide F() { entier32 X[2][1] = {{1, 2}, {3}}; }",
            42, "agregat-tableau-imbrique-trop-long");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "structure Paire { entier32 A; entier32 B; }; "
            "publique vide F() { Paire X = {1, 2, 3}; }",
            43, "agregat-structure-trop-long");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "union Choix { entier32 A; entier32 B; }; "
            "publique vide F() { Choix X = {1, 2}; }",
            43, "agregat-union-trop-long");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "publique vide F() { entier32 X = {1, 2}; }",
            44, "agregat-scalaire-trop-long");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "publique vide F() { entier32 X[2] = {1, \"texte\"}; }",
            45, "element-agregat-tableau-incompatible");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "publique vide F() { entier32 X[1][1] = {1}; }",
            45, "dimension-agregat-tableau-manquante");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "structure Paire { entier32 A; entier32 B; }; "
            "publique vide F() { Paire X = {1, \"texte\"}; }",
            45, "element-agregat-structure-incompatible");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "publique vide F() { entier32 X[2] = 1; }",
            46, "initialiseur-tableau-local-non-agrege");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "entier32 X[2] = 1; publique vide F() {}",
            46, "initialiseur-tableau-global-non-agrege");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A { privée: entier32 X[2]; publique: "
            "constructeur() : X({1, 2, 3}) {} };",
            42, "agregat-champ-explicite-trop-long");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A { privée: entier32 X[1][1] = {{1, 2}}; "
            "publique: constructeur() {} };",
            42, "agregat-champ-par-defaut-trop-long");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "entier32 X[2] = {1, 2, 3}; publique vide F() {}",
            42, "agregat-tableau-global-trop-long");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "structure Ligne { entier32 Valeurs[2]; }; "
            "publique vide F() { Ligne X = {{1, \"texte\"}}; }",
            45, "element-agregat-champ-tableau-incompatible");

        ComparerErreurSemantique(
            syntaxe, semantique,
            "structure A {}; structure A {}; publique vide F() {}",
            6, "type-duplique");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "structure A {}; énumération A { V, }; publique vide F() {}",
            7, "structure-en-conflit-avec-enumeration");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "publique vide V() {} entier32 V;",
            9, "globale-fonction-conflit");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "entier32 V; publique vide V() {}",
            9, "globale-avant-fonction-conflit");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "entier32 V; entier32 V; publique vide F() {}",
            8, "globale-dupliquee");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "publique vide F() {} alias F = F;",
            11, "alias-en-conflit");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "publique vide F() {} alias A = F; alias A = F;",
            10, "alias-duplique");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "structure S { entier32 X; entier32 X; }; "
            "publique vide F() {}",
            12, "champ-duplique");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "structure S { entier32 X; alias X = X; }; "
            "publique vide F() {}",
            14, "alias-champ-en-conflit");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "énumération E { A, A, }; publique vide F() {}",
            15, "enumerateur-duplique");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "publique vide F(entier32 X, entier32 X) {}",
            16, "parametre-duplique");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "publique vide F(entier32 X) { entier32 X = 0; }",
            17, "locale-dupliquee");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "publique entier32 F() { retourner Inconnue; }",
            18, "symbole-introuvable");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "publique entier32 F(entier32 X) { retourner X; } "
            "publique entier64 F(entier64 X) { retourner X; } "
            "publique vide G() { F; }",
            19, "adresse-surcharge-ambigue");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "publique entier32 F(entier32 X) { retourner X; } "
            "publique entier64 F(entier64 X) { retourner X; } "
            "publique vide G() { F(vrai); }",
            21, "aucune-surcharge-compatible");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "publique entier64 F(entier64 X, naturel64 Y) { retourner X; } "
            "publique entier64 F(naturel64 X, entier64 Y) { retourner Y; } "
            "publique vide G() { F(1, 1); }",
            22, "appel-surcharge-ambigu");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "publique entier32 F(entier32 X) { retourner X.Y; }",
            23, "recepteur-membre-invalide");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "structure S { entier32 X; }; "
            "publique entier32 F(S valeur) { retourner valeur.Y; }",
            24, "membre-introuvable");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A { publique: entier32 M(entier32 X) { retourner X; } "
            "entier64 M(entier64 X) { retourner X; } "
            "entier32 F() { retourner soi.M(vrai); } };",
            21, "aucune-surcharge-methode-compatible");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A { publique: "
            "entier64 M(entier64 X, naturel64 Y) { retourner X; } "
            "entier64 M(naturel64 X, entier64 Y) { retourner Y; } "
            "entier64 F() { retourner soi.M(1, 1); } };",
            22, "appel-methode-surcharge-ambigu");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A { privée: entier32 Secret; "
            "publique: constructeur() {} }; "
            "publique entier32 F() { A valeur; retourner valeur.Secret; }",
            25, "champ-prive-inaccessible");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A { protégée: entier32 Protegee; "
            "publique: constructeur() {} }; "
            "publique entier32 F() { A valeur; retourner valeur.Protegee; }",
            25, "champ-protege-inaccessible");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A { privée: entier32 M() { retourner 1; } "
            "publique: constructeur() {} }; "
            "publique entier32 F() { A valeur; retourner valeur.M(); }",
            25, "methode-privee-inaccessible");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A { privée: constructeur() {} }; "
            "publique vide F() { A valeur; }",
            26, "constructeur-prive-inaccessible");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A { publique: constructeur() : soi() {} };",
            31, "delegation-constructeur-directe");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A { publique: "
            "constructeur() : soi(1) {} "
            "constructeur(entier32 valeur) : soi() {} };",
            32, "cycle-delegation-constructeur");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A { publique: constructeur() : parent() {} };",
            30, "initialiseur-base-sans-classe-base");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe Base { publique: constructeur(entier32 valeur) {} }; "
            "classe A : publique Base { publique: "
            "constructeur() : parent(vrai) {} };",
            21, "aucun-constructeur-base-compatible");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe Base { privée: constructeur() {} }; "
            "classe A : publique Base { publique: "
            "constructeur() : parent() {} };",
            26, "constructeur-base-prive-inaccessible");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A { publique: constructeur() : Inconnu(1) {} };",
            33, "champ-initialiseur-introuvable");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A { privée: entier32 X; publique: "
            "constructeur() : X(1), X(2) {} };",
            34, "champ-initialise-plusieurs-fois");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A { privée: entier32 X; entier32 Y; publique: "
            "constructeur() : Y(1), X(2) {} };",
            35, "ordre-initialisation-champ-invalide");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A { privée: entier32 X; publique: "
            "constructeur() : X() {} };",
            36, "arite-initialiseur-champ-invalide");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A { privée: entier32 X; publique: "
            "constructeur() : X(vrai) {} };",
            37, "type-initialiseur-champ-incompatible");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A { privée: entier8 X; publique: "
            "constructeur() : X(128) {} };",
            37, "constante-initialiseur-champ-hors-plage");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe Membre {}; classe A { privée: Membre X; publique: "
            "constructeur() : X(1) {} };",
            27, "constructeur-champ-non-declare");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe Membre { publique: constructeur(entier32 valeur) {} }; "
            "classe A { privée: Membre X; publique: "
            "constructeur() : X(vrai) {} };",
            21, "aucun-constructeur-champ-compatible");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe Membre { privée: constructeur(entier32 valeur) {} }; "
            "classe A { privée: Membre X; publique: "
            "constructeur() : X(1) {} };",
            26, "constructeur-champ-prive-inaccessible");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe Base { publique: constructeur(entier32 valeur) {} }; "
            "classe A : publique Base { publique: constructeur() {} };",
            21, "aucun-constructeur-base-implicite-compatible");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe Base { privée: constructeur() {} }; "
            "classe A : publique Base { publique: constructeur() {} };",
            26, "constructeur-base-implicite-prive");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe Element { publique: constructeur() {} }; "
            "classe A { privée: Element X = {}; publique: constructeur() {} };",
            38, "valeur-par-defaut-objet-classe-interdite");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A { privée: entier32 X = 1; }; publique vide F() {}",
            39, "valeur-par-defaut-sans-constructeur");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A { privée: constante entier32 X; publique: "
            "constructeur() {} };",
            40, "champ-constant-non-initialise");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A { privée: entier32 X[2]; publique: "
            "constructeur() : X(1) {} };",
            41, "initialiseur-explicite-tableau-non-agrege");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A { privée: entier32 X = vrai; publique: "
            "constructeur() {} };",
            37, "valeur-par-defaut-type-incompatible");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe Membre { publique: constructeur(entier32 valeur) {} }; "
            "classe A { privée: Membre X; publique: constructeur() {} };",
            21, "constructeur-champ-implicite-incompatible");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe Membre { privée: constructeur() {} }; "
            "classe A { privée: Membre X; publique: constructeur() {} };",
            26, "constructeur-champ-implicite-prive");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe Element { publique: constructeur(entier32 valeur) {} }; "
            "classe A { privée: Element X[2]; publique: "
            "constructeur() : X(vrai) {} };",
            21, "constructeur-tableau-explicite-incompatible");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe Element {}; classe A { privée: Element X[2]; publique: "
            "constructeur() : X(1) {} };",
            27, "constructeur-tableau-explicite-non-declare");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A { privée: entier32 X[2] = 1; publique: "
            "constructeur() {} };",
            41, "valeur-par-defaut-tableau-non-agregee");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe Element { publique: constructeur(entier32 valeur) {} }; "
            "publique vide F() { Element valeurs[2]; }",
            21, "constructeur-tableau-local-implicite-incompatible");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe Element { privée: constructeur() {} }; "
            "publique vide F() { Element valeurs[2]; }",
            26, "constructeur-tableau-local-prive");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe Element {}; publique vide F() { Element valeurs[2](1); }",
            27, "constructeur-tableau-local-non-declare");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe Element { publique: constructeur() {} }; "
            "publique vide F() { Element valeurs[2] = {{}, {}}; }",
            29, "agregat-tableau-local-objet-interdit");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A {}; publique vide F() { A valeur(); }",
            27, "constructeur-non-declare");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A { publique: constructeur() {} }; "
            "publique vide F() { A valeur = {}; }",
            29, "initialiseur-classe-interdit");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A { publique: constructeur(entier32 valeur) {} }; "
            "publique vide F() { A valeur(vrai); }",
            21, "aucun-constructeur-compatible");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A { publique: "
            "constructeur(entier64 x, naturel64 y) {} "
            "constructeur(naturel64 x, entier64 y) {} }; "
            "publique vide F() { A valeur(1, 1); }",
            22, "appel-constructeur-ambigu");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A { publique: constructeur() {} }; "
            "publique vide F() { A valeur; valeur + 1; }",
            28, "operateur-membre-introuvable");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A { publique: constructeur() {} "
            "entier32 opérateur +(entier32 valeur) { retourner valeur; } }; "
            "publique vide F() { A objet; objet + vrai; }",
            21, "aucun-operateur-compatible");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "classe A { publique: constructeur() {} }; "
            "publique entier32 opérateur +(A& gauche, entier32 droite) { "
            "retourner droite; } "
            "publique vide F() { A objet; objet + vrai; }",
            21,
            "aucun-operateur-libre-compatible-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "class A { public: constructor() {} }; "
            "public int32 operator +(A& gauche, int32 droite) { "
            "return droite; } "
            "public void F() { A objet; objet + true; }",
            21,
            "aucun-operateur-libre-compatible-anglais");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A { publique: constructeur() {} "
            "entier64 opérateur +(entier64 valeur) { retourner valeur; } "
            "naturel64 opérateur +(naturel64 valeur) { retourner valeur; } }; "
            "publique vide F() { A objet; objet + 1; }",
            22, "appel-operateur-ambigu");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "classe A { publique: constructeur() {} }; "
            "publique entier64 opérateur +(A& gauche, entier64 droite) { "
            "retourner droite; } "
            "publique naturel64 opérateur +(A& gauche, naturel64 droite) { "
            "retourner droite; } "
            "publique vide F() { A objet; objet + 1; }",
            22,
            "appel-operateur-libre-ambigu-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "class A { public: constructor() {} }; "
            "public int64 operator +(A& gauche, int64 droite) { "
            "return droite; } "
            "public uint64 operator +(A& gauche, uint64 droite) { "
            "return droite; } "
            "public void F() { A objet; objet + 1; }",
            22,
            "appel-operateur-libre-ambigu-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "classe A { publique: booléen opérateur !(entier32 valeur) { "
            "retourner faux; } }; publique vide F() {}",
            59,
            "arite-operateur-membre-invalide-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "class A { public: bool operator !(int32 valeur) { "
            "return false; } }; public void F() {}",
            59,
            "arite-operateur-membre-invalide-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "classe A {}; publique entier32 opérateur +(A& valeur) { "
            "retourner 0; } publique vide F() {}",
            59,
            "arite-operateur-libre-invalide-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "class A {}; public int32 operator +(A& valeur) { "
            "return 0; } public void F() {}",
            59,
            "arite-operateur-libre-invalide-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide F() { entier32 valeurs[2]; !valeurs; }",
            60,
            "negation-tableau-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public void F() { int32 valeurs[2]; !valeurs; }",
            60,
            "negation-tableau-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide F(entier32* pointeur) { ~pointeur; }",
            61,
            "unaire-pointeur-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public void F(int32* pointeur) { ~pointeur; }",
            61,
            "unaire-pointeur-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide F() { entier32 valeurs[2]; valeurs && vrai; }",
            62,
            "logique-tableau-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public void F() { int32 valeurs[2]; valeurs && true; }",
            62,
            "logique-tableau-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide F(entier32 gauche, entier64 droite) { "
            "gauche + droite; }",
            63,
            "types-operandes-differents-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public void F(int32 gauche, int64 droite) { gauche + droite; }",
            63,
            "types-operandes-differents-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide F(entier32* pointeur) { pointeur << 1; }",
            64,
            "decalage-pointeur-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public void F(int32* pointeur) { pointeur << 1; }",
            64,
            "decalage-pointeur-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide F(entier32* pointeur) { pointeur + pointeur; }",
            65,
            "calcul-pointeur-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public void F(int32* pointeur) { pointeur + pointeur; }",
            65,
            "calcul-pointeur-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide F() { entier32 valeurs[2]; valeurs == valeurs; }",
            66,
            "comparaison-tableau-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public void F() { int32 valeurs[2]; valeurs == valeurs; }",
            66,
            "comparaison-tableau-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide F(entier32* pointeur) { pointeur < pointeur; }",
            67,
            "comparaison-pointeur-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public void F(int32* pointeur) { pointeur < pointeur; }",
            67,
            "comparaison-pointeur-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide F() { vrai < faux; }",
            68,
            "comparaison-booleenne-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public void F() { true < false; }",
            68,
            "comparaison-booleenne-anglais");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "classe A { privée: "
            "entier32 opérateur +(entier32 valeur) { retourner valeur; } "
            "publique: constructeur() {} }; "
            "publique vide F() { A objet; objet + 1; }",
            25, "operateur-prive-inaccessible");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "classe ObjetDestructeurPrive { privée: destructeur() {} }; "
            "publique vide F() { ObjetDestructeurPrive objet; }",
            56,
            "destructeur-prive-inaccessible");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "structure CycleA { CycleB ValeurB; }; "
            "structure CycleB { CycleA ValeurA; }; publique vide F() {}",
            57,
            "cycle-structure-par-valeur");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "structure ObjetTailleInvalide { vide Valeur; }; "
            "publique vide F() {}",
            58,
            "champ-vide-taille-invalide");
        ComparerErreurSemantique(
            syntaxe, semantique,
            "structure SansFonction {};",
            20, "fonction-absente");

        Exiger(
            semantique(nullptr) == 1,
            "une requête sémantique nulle aurait dû être refusée");
        RequeteAnalyseSemantiqueHote requeteInvalide{};
        Exiger(
            semantique(&requeteInvalide) == 1,
            "une source sémantique nulle aurait dû être refusée");
        const std::string sourceAst = "publique vide F() {}";
        auto astInvalide = ComparerDeclarations(
            syntaxe, sourceAst, "ast-semantique-invalide");
        astInvalide[1].Parent = 1;
        RequeteAnalyseSemantiqueHote requeteAst{
            sourceAst.data(),
            static_cast<std::uint64_t>(sourceAst.size()),
            astInvalide.data(),
            static_cast<std::uint64_t>(astInvalide.size()),
            nullptr, 0, nullptr, 0, {}};
        Exiger(
            semantique(&requeteAst) == 2
                && requeteAst.Resultat.Erreur == 2,
            "un AST sémantique invalide aurait dû être refusé");

        Exiger(
            !LiberationInvalide
                && AllocationsActives.empty()
                && NombreAllocations == NombreLiberations
                && NombreAllocations != 0,
            "l’analyse sémantique auto-hébergée laisse une allocation active");
    }

    void TesterClassificateur(const std::string& chemin)
    {
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
            if (nom == "GalacticShrine::GsPP::Hote::AllouerMemoire")
                return allouer;
            if (nom == "GalacticShrine::GsPP::Hote::LibererMemoire")
                return liberer;
            return std::nullopt;
        };
        const auto image = GsPP::ChargeurGsE().Charger(
            contenu, zone.Base(), resolveur);
        zone.Copier(image.Memoire);

        const auto adresse = image.ChercherExport(
            "GalacticShrine::GsPP::Autohebergement::ClassifierMotCle");
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
            if (nom == "GalacticShrine::GsPP::Hote::AllouerMemoire") return allouer;
            if (nom == "GalacticShrine::GsPP::Hote::LibererMemoire") return liberer;
            if (nom == "GalacticShrine::GsPP::Hote::LireFichier") return lire;
            if (nom == "GalacticShrine::GsPP::Hote::EcrireFichier") return ecrire;
            if (nom == "GalacticShrine::GsPP::Hote::EmettreDiagnostic")
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
        if (argc != 3)
            throw std::runtime_error(
                "Frontend.GsE et l’image GsE de test sont attendus");
        TesterClassificateur(argv[1]);
        TesterLexeur(argv[1]);
        TesterAnalyseurDeclarations(argv[1]);
        TesterAnalyseurSemantique(argv[1], argv[1]);
        TesterBibliothequeHebergee(argv[2]);
        std::cout
            << "Auto-hébergement 0.27 : "
            << "Frontend.GsE unique, 83 classifications, lexeur différentiel, "
            << "AST et premières résolutions sémantiques différentielles "
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
