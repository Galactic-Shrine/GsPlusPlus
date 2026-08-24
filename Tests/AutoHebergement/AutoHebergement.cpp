#include "GsPP/ChargeurGsE.hpp"
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

    static_assert(sizeof(VueTexteHote) == 16);
    static_assert(sizeof(RequeteFichierHote) == 32);
    static_assert(sizeof(DiagnosticHote) == 48);
    static_assert(sizeof(JetonLexeHote) == 48);
    static_assert(sizeof(ResultatLexageHote) == 32);
    static_assert(sizeof(RequeteLexageHote) == 64);
    static_assert(sizeof(NoeudDeclarationHote) == 64);
    static_assert(sizeof(ResultatAnalyseDeclarationsHote) == 48);
    static_assert(sizeof(RequeteAnalyseDeclarationsHote) == 80);

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
            type.Genre == GsPP::GenreType::Structure
                ? HacherTexte(type.Nom)
                : HacherTexte({}));
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
                            for (const auto& argument :
                                 fonction.ArgumentsConstructeurDelegue)
                                AjouterExpressionReference(
                                    *argument,
                                    indexFonction,
                                    HacherTexte(structure.Espace),
                                    resultat);
                        }
                        else
                        {
                            for (const auto& argument :
                                 fonction.ArgumentsConstructeurBase)
                                AjouterExpressionReference(
                                    *argument,
                                    indexFonction,
                                    HacherTexte(structure.Espace),
                                    resultat);
                            for (const auto& initialiseur :
                                 fonction.InitialiseursChamps)
                                for (const auto& argument :
                                     initialiseur.Arguments)
                                    AjouterExpressionReference(
                                        *argument,
                                        indexFonction,
                                        HacherTexte(structure.Espace),
                                        resultat);
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
                std::uint32_t drapeaux = 0;
                if (fonction.EstPublique) drapeaux |= 1U;
                if (fonction.EstExterne) drapeaux |= 2U;
                if (fonction.Corps) drapeaux |= 4U;
                resultat.push_back({
                    1,
                    static_cast<std::uint32_t>(fonction.Position.Ligne),
                    static_cast<std::uint32_t>(fonction.Position.Colonne),
                    drapeaux,
                    0,
                    0,
                    0,
                    HacherTexte(fonction.Nom),
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
                + std::string(nomCorpus));

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
                || courant.Genre == 32;
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
            if (nom == "Gs::Hote::AllouerMemoire") return allouer;
            if (nom == "Gs::Hote::LibererMemoire") return liberer;
            return std::nullopt;
        };
        const auto image = GsPP::ChargeurGsE().Charger(
            contenu, zone.Base(), resolveur);
        zone.Copier(image.Memoire);

        const auto adresse = image.ChercherExport(
            "Gs::Autohebergement::AnalyserDeclarationsSource");
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
            "  publique Gs::Noeud* Copier("
            "constante Gs::Noeud& source) { retourner source; }\n"
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
                || genreParent == 21;
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
        if (argc != 5)
            throw std::runtime_error(
                "quatre images GsE de test sont attendues");
        TesterClassificateur(argv[1]);
        TesterLexeur(argv[2]);
        TesterAnalyseurDeclarations(argv[3]);
        TesterBibliothequeHebergee(argv[4]);
        std::cout
            << "Auto-hébergement 0.27 : "
            << "83 classifications, lexeur différentiel, "
            << "AST différentiel des fonctions et données "
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
