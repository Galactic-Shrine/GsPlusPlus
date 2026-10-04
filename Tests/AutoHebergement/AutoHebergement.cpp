#include "GsPP/ChargeurGsE.hpp"
#include "GsPP/AnalyseurSemantique.hpp"
#include "GsPP/AnalyseurSyntaxique.hpp"
#include "GsPP/ErreurCompilation.hpp"
#include "GsPP/Lexeur.hpp"
#include "GsPP/GenerateurX64.hpp"

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
#include <unordered_map>
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

    struct GlobaleEmiseHote
    {
        std::uint64_t IndexNoeud, IndexSymbole, Decalage, Taille, Alignement;
        std::uint32_t Drapeaux, Reserve;
    };

    struct RelocalisationGlobaleHote
    {
        std::uint64_t IndexNoeudGlobale, Decalage, IndexSymboleCible;
        std::uint32_t Genre, Reserve;
    };

    struct ResultatEmissionGlobalesHote
    {
        std::uint32_t Erreur, LigneErreur, ColonneErreur, Detail;
        std::uint64_t NombreGlobales, NombreOctetsDonnees, NombreOctetsZero;
        std::uint64_t NombreRelocalisations, NombreOctetsArene;
    };

    struct RequeteEmissionGlobalesHote
    {
        const char* Source;
        std::uint64_t TailleSource;
        const NoeudDeclarationHote* Noeuds;
        std::uint64_t NombreNoeuds;
        GlobaleEmiseHote* Globales;
        std::uint64_t CapaciteGlobales;
        std::uint8_t* Donnees;
        std::uint64_t CapaciteDonnees;
        RelocalisationGlobaleHote* Relocalisations;
        std::uint64_t CapaciteRelocalisations;
        ResultatEmissionGlobalesHote Resultat;
    };

    static_assert(sizeof(GlobaleEmiseHote) == 48);
    static_assert(sizeof(RelocalisationGlobaleHote) == 32);
    static_assert(sizeof(ResultatEmissionGlobalesHote) == 56);
    static_assert(sizeof(RequeteEmissionGlobalesHote) == 136);
    static_assert(offsetof(RequeteEmissionGlobalesHote, Resultat) == 80);
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
            if (courant.Genre == 10)
            {
                std::string nomQualifie;
                for (const auto& jeton : GsPP::Lexeur(std::string(nom), "nom-alias").Analyser())
                    if (jeton.Genre != GsPP::GenreJeton::Fin)
                    {
                        Exiger(jeton.Genre == GsPP::GenreJeton::Identifiant
                                   || jeton.Genre == GsPP::GenreJeton::DeuxPointsDouble,
                            "lexème non nominal dans la tranche d'un alias");
                        nomQualifie += jeton.Texte;
                    }
                Exiger(HacherTexte(nomQualifie) == courant.HachageNom,
                    "hachage de nom qualifié d'alias incohérent pour " + std::string(nomCorpus));
                continue;
            }
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
        const auto noeudsAvant = noeuds;
        auto jetons = GsPP::Lexeur(source, std::string(nomCorpus)).Analyser();
        auto programme = GsPP::AnalyseurSyntaxique(
            std::move(jetons), std::string(nomCorpus)).Analyser();
        try
        {
            GsPP::AnalyseurSemantique().Analyser(programme);
        }
        catch (const GsPP::ErreurCompilation& erreur)
        {
            throw std::runtime_error("le bootstrap refuse le corpus valide "
                + std::string(nomCorpus) + " : " + erreur.what());
        }
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
                + std::string(nomCorpus) + " : code=" + std::to_string(interrogation)
                + ", ligne=" + std::to_string(requete.Resultat.LigneErreur)
                + ", colonne=" + std::to_string(requete.Resultat.ColonneErreur)
                + ", détail=" + std::to_string(requete.Resultat.Detail));

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

        Exiger(requete.Noeuds == noeuds.data()
                   && std::memcmp(noeuds.data(), noeudsAvant.data(),
                       noeuds.size() * sizeof(noeuds[0])) == 0,
            "AST de l'appelant modifié par l'analyse sémantique pour " + std::string(nomCorpus));
        return {
            std::move(noeuds),
            std::move(symboles),
            std::move(resolutions)};
    }

    std::size_t NombreRefusSemantiquesDifferentiels = 0;

    const std::string NomCollisionLiaisonA = "TypeCollision_Bf_8190k3Dbe13aCfcn";
    const std::string NomCollisionLiaisonB = "TypeCollisioncAadlNBpc0Aabp0aAaba";
    const std::string NomCollisionRecepteurA = "TypeCollisionmNbc8KY0YC1ce13aCfcn";
    const std::string NomCollisionRecepteurB = "TypeCollisionaAaddD2paDrbbp0aAaba";
    const std::string NomCollisionCallbackA = "TypeCollisionnNbcH390k3Dbe13aCfcn";
    const std::string NomCollisionCallbackB = "TypeCollisionbAadtLBpc0Aabp0aAaba";
    const std::string NomCollisionConstanteA = "TypeCollisioneWZejA79h339acE1Jcfc";
    const std::string NomCollisionConstanteB = "TypeCollisiona0ibbBPgbPphbbBQBbab";
    const std::string NomCollisionEspaceA = "TypeCollisione5gek990e149bO5e5_nc";
    const std::string NomCollisionEspaceB = "TypeCollisiona0bbaHdpa0ohcxPagfab";
    const std::string NomCollisionQualifieA = "TypeCollisionn51c99N09F59b97Q41nc";
    const std::string NomCollisionQualifieB = "TypeCollisionb0PdghApAApjah0qpPab";
    const std::string NomCollisionMethodeQualifieA = "TypeCollisiona5ge0190k3Dbe13aCfcn";
    const std::string NomCollisionMethodeQualifieB = "TypeCollisionePbbtNBpc0Aabp0aAaba";
    const std::string NomCollisionUtf8A = "TypeCollisione3fa9GZ0ySE1bZ7c4cnc";
    const std::string NomCollisionUtf8B = "TypeCollisiona0aboB1pa0BPci0cpbab";

    std::string DeclarerTypesCollisionLiaison(const std::string& a, const std::string& b)
    {
        return "structure " + a + " {}; structure " + b + " {}; ";
    }

    void ComparerErreurSemantique(
        AnalyseurDeclarationsAutoHeberge syntaxe,
        AnalyseurSemantiqueAutoHeberge semantique,
        const std::string& source,
        std::uint32_t erreurAttendue,
        std::string_view nomCorpus)
    {
        auto noeuds = ComparerDeclarations(syntaxe, source, nomCorpus);
        const auto noeudsAvant = noeuds;
        std::uint32_t ligne = 0;
        std::uint32_t colonne = 0;
        std::string diagnosticBootstrap;
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
            diagnosticBootstrap = erreur.what();
        }
        catch (const std::exception& erreur)
        {
            throw std::runtime_error(
                "échec inattendu du bootstrap pour "
                + std::string(nomCorpus) + " : " + erreur.what());
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
        Exiger(requete.Noeuds == noeuds.data()
                   && std::memcmp(noeuds.data(), noeudsAvant.data(),
                       noeuds.size() * sizeof(noeuds[0])) == 0,
            "AST modifié après un refus sémantique pour " + std::string(nomCorpus));
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
                + "; bootstrap : " + diagnosticBootstrap + ")");
        ++NombreRefusSemantiquesDifferentiels;
    }

    using EmetteurGlobalesAutoHeberge =
        std::uint32_t (GS_ABI_HOTE *)(RequeteEmissionGlobalesHote*);

    void ComparerEmissionGlobales(
        AnalyseurDeclarationsAutoHeberge syntaxe,
        AnalyseurSemantiqueAutoHeberge semantique,
        EmetteurGlobalesAutoHeberge emettre,
        const std::string& source,
        std::string_view nomCorpus)
    {
        auto programme = GsPP::AnalyseurSyntaxique(
            GsPP::Lexeur(source, std::string(nomCorpus)).Analyser(),
            std::string(nomCorpus)).Analyser();
        GsPP::AnalyseurSemantique().Analyser(programme);
        const auto machine = GsPP::GenerateurX64().Generer(programme);
        const auto sortie = AnalyserSemantiqueValide(syntaxe, semantique, source, nomCorpus);
        const auto nombreGlobales = static_cast<std::uint64_t>(std::count_if(
            programme.VariablesGlobales.begin(), programme.VariablesGlobales.end(),
            [](const auto& variable) { return !variable.EstExterne; }));
        std::unordered_map<std::string, GsPP::Structure*> structures;
        for (auto& structure : programme.Structures)
            structures.emplace(structure.NomComplet(), &structure);
        std::vector<std::uint8_t> donneesReference;
        std::unordered_map<std::string, std::uint64_t> decalagesReference;
        std::vector<GsPP::CodeMachine::Relocalisation> relocationsReference;
        /**
         * Le backend place ses tables virtuelles avant les données utilisateur.
         * L'API du frontend émet seulement les globales : on isole leurs octets
         * du bootstrap et on recale les offsets et relocalisations, sans prétendre
         * valider l'émission des tables virtuelles du futur backend auto-hébergé.
         **/
        for (const auto& variable : programme.VariablesGlobales)
        {
            if (variable.EstExterne) continue;
            const auto symbole = std::find_if(machine.Symboles.begin(), machine.Symboles.end(),
                [&](const auto& candidat) { return candidat.Nom == variable.NomComplet(); });
            Exiger(symbole != machine.Symboles.end(), "globale absente du bootstrap");
            if (!variable.EstInitialisee)
            {
                decalagesReference.emplace(variable.NomComplet(), symbole->Decalage);
                continue;
            }
            const auto alignement = GsPP::AnalyseurSemantique::AlignementType(variable.Type, structures);
            const auto decalage = (donneesReference.size() + alignement - 1) & ~(std::size_t(alignement) - 1);
            decalagesReference.emplace(variable.NomComplet(), decalage);
            Exiger(symbole->Section == GsPP::SectionMachine::Donnees
                       && std::uint64_t(symbole->Decalage) + symbole->Taille <= machine.Donnees.size(),
                "tranche de globale hors des données du bootstrap");
            donneesReference.resize(decalage, 0);
            donneesReference.insert(donneesReference.end(), machine.Donnees.begin() + symbole->Decalage,
                machine.Donnees.begin() + symbole->Decalage + symbole->Taille);
            for (const auto& relocation : machine.Relocalisations)
                if (relocation.Section == GsPP::SectionMachine::Donnees
                    && relocation.Decalage >= symbole->Decalage
                    && std::uint64_t(relocation.Decalage) < std::uint64_t(symbole->Decalage) + symbole->Taille)
                {
                    auto reference = relocation;
                    reference.Decalage = static_cast<std::uint32_t>(decalage + relocation.Decalage - symbole->Decalage);
                    relocationsReference.push_back(std::move(reference));
                }
        }
        RequeteEmissionGlobalesHote requete{
            source.data(), source.size(), sortie.Noeuds.data(), sortie.Noeuds.size(),
            nullptr, 0, nullptr, 0, nullptr, 0, {}};
        const auto interrogationEmission = emettre(&requete);
        Exiger(interrogationEmission == (nombreGlobales ? 91U : 0U),
               "interrogation de l'émission incorrecte : " + std::string(nomCorpus)
                   + ", code=" + std::to_string(interrogationEmission)
                   + ", ligne=" + std::to_string(requete.Resultat.LigneErreur)
                   + ", détail=" + std::to_string(requete.Resultat.Detail));
        const auto besoins = requete.Resultat;
        Exiger(besoins.NombreGlobales == nombreGlobales
                   && besoins.NombreOctetsDonnees == donneesReference.size()
                   && besoins.NombreOctetsZero == machine.TailleZero
                   && besoins.NombreRelocalisations == relocationsReference.size()
                   && besoins.NombreOctetsArene != 0,
               "besoins d'émission différents du bootstrap : " + std::string(nomCorpus));

        // Une sentinelle au-delà de chaque capacité protège aussi le cas exact.
        std::vector<GlobaleEmiseHote> globales(nombreGlobales + 1);
        std::vector<std::uint8_t> donnees(donneesReference.size() + 1, 0xA5);
        std::vector<RelocalisationGlobaleHote> relocations(relocationsReference.size() + 1);
        std::memset(globales.data(), 0xA5, globales.size() * sizeof(globales[0]));
        std::memset(relocations.data(), 0xA5, relocations.size() * sizeof(relocations[0]));
        const auto globalesVierges = globales;
        const auto relocationsVierges = relocations;
        requete.Globales = globales.data();
        requete.CapaciteGlobales = nombreGlobales;
        requete.Donnees = donnees.data();
        requete.CapaciteDonnees = donneesReference.size();
        requete.Relocalisations = relocations.data();
        requete.CapaciteRelocalisations = relocationsReference.size();
        const auto sortiesIntactes = [&]
        {
            return std::memcmp(globales.data(), globalesVierges.data(),
                               globales.size() * sizeof(globales[0])) == 0
                && std::memcmp(relocations.data(), relocationsVierges.data(),
                               relocations.size() * sizeof(relocations[0])) == 0
                && std::all_of(donnees.begin(), donnees.end(),
                               [](auto octet) { return octet == 0xA5; });
        };
        const auto capaciteInsuffisante = [&](std::uint64_t& capacite, std::uint32_t code)
        {
            if (capacite == 0) return;
            --capacite;
            Exiger(emettre(&requete) == code && requete.Resultat.Erreur == code
                       && requete.Resultat.NombreGlobales == besoins.NombreGlobales
                       && requete.Resultat.NombreOctetsDonnees == besoins.NombreOctetsDonnees
                       && requete.Resultat.NombreOctetsZero == besoins.NombreOctetsZero
                       && requete.Resultat.NombreRelocalisations == besoins.NombreRelocalisations
                       && sortiesIntactes(),
                   "capacité partielle ou sortie modifiée : " + std::string(nomCorpus));
            ++capacite;
        };
        capaciteInsuffisante(requete.CapaciteGlobales, 91);
        capaciteInsuffisante(requete.CapaciteDonnees, 92);
        capaciteInsuffisante(requete.CapaciteRelocalisations, 93);
        Exiger(emettre(&requete) == 0 && requete.Resultat.Erreur == 0,
               "émission échouée : " + std::string(nomCorpus));
        Exiger(std::equal(donneesReference.begin(), donneesReference.end(), donnees.begin()),
               "octets globaux différents du bootstrap : " + std::string(nomCorpus));
        Exiger(donnees.back() == 0xA5
                   && std::memcmp(&globales.back(), &globalesVierges.back(), sizeof(globales[0])) == 0
                   && std::memcmp(&relocations.back(), &relocationsVierges.back(), sizeof(relocations[0])) == 0,
               "écriture au-delà d'une capacité d'émission");

        std::size_t indexGlobale = 0;
        for (const auto& variable : programme.VariablesGlobales)
        {
            if (variable.EstExterne) continue;
            const auto& globale = globales[indexGlobale++];
            Exiger(globale.IndexNoeud < sortie.Noeuds.size()
                       && globale.IndexSymbole < sortie.Symboles.size(),
                   "index de globale émise hors table");
            const auto& noeud = sortie.Noeuds[globale.IndexNoeud];
            const auto& symbole = sortie.Symboles[globale.IndexSymbole];
            const auto reference = std::find_if(machine.Symboles.begin(), machine.Symboles.end(),
                [&](const auto& candidat) { return candidat.Nom == variable.NomComplet(); });
            Exiger(reference != machine.Symboles.end()
                       && noeud.Genre == 3 && symbole.Genre == 3
                       && symbole.IndexNoeud == globale.IndexNoeud
                       && noeud.Ligne == variable.Position.Ligne
                       && noeud.Colonne == variable.Position.Colonne
                       && globale.Decalage == decalagesReference.at(variable.NomComplet())
                       && globale.Taille == reference->Taille
                       && globale.Alignement == GsPP::AnalyseurSemantique::AlignementType(variable.Type, structures)
                       && globale.Drapeaux == ((variable.EstInitialisee ? 1U : 0U) | (variable.EstPublique ? 2U : 0U))
                       && globale.Reserve == 0,
                   "disposition globale différente : " + variable.NomComplet());
        }
        for (std::size_t index = 0; index < relocationsReference.size(); ++index)
        {
            const auto& relocation = relocations[index];
            const auto& reference = relocationsReference[index];
            const auto globale = std::find_if(globales.begin(), globales.begin() + nombreGlobales,
                [&](const auto& candidat) { return candidat.IndexNoeud == relocation.IndexNoeudGlobale; });
            Exiger(globale != globales.begin() + nombreGlobales
                       && relocation.IndexSymboleCible < sortie.Symboles.size()
                       && relocation.Genre == 1 && relocation.Reserve == 0
                       && reference.Type == GsPP::TypeRelocalisationMachine::Adresse64
                       && globale->Decalage + relocation.Decalage == reference.Decalage,
                   "position ou genre de relocalisation différent du bootstrap");
            const auto& cible = sortie.Symboles[relocation.IndexSymboleCible];
            const auto& noeudCible = sortie.Noeuds[cible.IndexNoeud];
            const auto fonction = std::find_if(programme.Fonctions.begin(), programme.Fonctions.end(),
                [&](const auto& candidat) { return candidat.NomComplet() == reference.Symbole; });
            Exiger(cible.Genre == 2 && fonction != programme.Fonctions.end()
                       && noeudCible.Ligne == fonction->Position.Ligne
                       && noeudCible.Colonne == fonction->Position.Colonne,
                   "cible de relocalisation différente du bootstrap");
        }
        const auto premiereEmission = donnees;
        const auto premieresGlobales = globales;
        const auto premieresRelocalisations = relocations;
        Exiger(emettre(&requete) == 0 && premiereEmission == donnees
                   && std::memcmp(globales.data(), premieresGlobales.data(),
                                  globales.size() * sizeof(globales[0])) == 0
                   && std::memcmp(relocations.data(), premieresRelocalisations.data(),
                                  relocations.size() * sizeof(relocations[0])) == 0,
               "émission non déterministe lors du second appel");
    }

    std::string TraduireCorpusConversions(std::string texte);

    void TesterEmissionGlobales(
        AnalyseurDeclarationsAutoHeberge syntaxe,
        AnalyseurSemantiqueAutoHeberge semantique,
        EmetteurGlobalesAutoHeberge emettre)
    {
        const std::string francais = R"(
espace Donnees {
    énumération Etat { Negatif = -2, Suivant, Positif = 7, };
    structure Vide {};
    structure Petit { octet Etiquette; entier32 Valeur; };
    union Choix { entier16 Court; entier64 Long; };
    structure Paquet {
        octet Marque;
        Petit Elements[2];
        naturel16 Grille[2][3];
        pointeur_fonction<entier32()> Rappels[2];
        Choix Option;
    };
    publique entier32 Fonction() { retourner 42; }
    externe entier32 Importee();
    externe entier32 GlobaleImportee;
    publique entier8 S8 = -128;
    entier16 S16 = -32768;
    entier32 S32 = -2147483648;
    entier64 S64 = -9223372036854775808;
    naturel8 U8 = 255;
    naturel16 U16 = 65535;
    naturel32 U32 = 4294967295;
    naturel64 U64 = 18446744073709551615;
    caractère Caractere = -1;
    booléen CourtCircuit = faux && ((1 / 0) == 0);
    entier64 Etendu = convertir<entier64>(convertir<entier8>(-1));
    entier32 Calcul = ((20 * 3) / 4 % 7) + ((16 >> 2) ^ 3);
    entier32 Decalage = -8 >> 2;
    Etat Enumeration = Etat::Suivant;
    Petit Structure = {2, -3};
    Choix Union = {-7};
    Paquet Agregat = {9, {{1, 2}, {3}}, {{1, 2}, {3}}, {&Fonction, Importee}, {4}};
    pointeur_fonction<entier32()> Rappel = Fonction;
    entier32 ScalaireVide = {};
    Vide ObjetVide = {};
    Paquet AgregatVide = {};
    publique octet ZeroPetit;
    entier64 ZeroGrand;
    Paquet ZeroAgregat[2];
    entier32* ZeroPointeur;
}
)";
        std::string anglais = francais;
        // La traduction des seuls mots-clés garde les noms et valeurs identiques.
        for (const auto& [fr, en] : std::vector<std::pair<std::string, std::string>>{
                 {"espace", "namespace"}, {"énumération", "enumeration"},
                 {"structure", "struct"}, {"publique", "public"}, {"externe", "extern"},
                 {"retourner", "return"}, {"pointeur_fonction", "function_pointer"},
                 {"convertir", "cast"}, {"naturel", "uint"}, {"entier", "int"},
                 {"octet", "byte"}, {"caractère", "char"}, {"booléen", "bool"}, {"faux", "false"}})
        {
            std::size_t position = 0;
            while ((position = anglais.find(fr, position)) != std::string::npos)
            {
                anglais.replace(position, fr.size(), en);
                position += en.size();
            }
        }
        ComparerEmissionGlobales(syntaxe, semantique, emettre, francais, "emission-globales-francais");
        ComparerEmissionGlobales(syntaxe, semantique, emettre, anglais, "emission-globales-anglais");
        const std::string conversions =
            "entier32 Somme = (20 + 1) + convertir<entier8>(21); "
            "booléen Comparaison = convertir<entier8>(-1) < (200 + 0); "
            "entier16 Division = convertir<entier8>(-6) / convertir<entier16>(2); "
            "entier16 Reste = convertir<entier8>(-6) % convertir<entier16>(3); "
            "publique vide F() {}";
        ComparerEmissionGlobales(syntaxe, semantique, emettre,
            conversions, "emission-conversions-implicites-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre,
            TraduireCorpusConversions(conversions), "emission-conversions-implicites-en");
        const std::string composes =
            "structure Bloc { octet Etiquette; entier32***** Adresses[2][3]; "
            "pointeur_fonction<vide()>* Rappels[2]; }; "
            "Bloc Globale; entier32***** Grille[2][3]; publique vide F() {}";
        ComparerEmissionGlobales(syntaxe, semantique, emettre,
            composes, "emission-types-composes-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre,
            TraduireCorpusConversions(composes), "emission-types-composes-en");
        const std::string nomsImbriques =
            "espace N { structure S { entier32 V; }; "
            "publique entier32 Lire(constante S* s) { retourner s->V; } "
            "pointeur_fonction<entier32(constante N::S*)> Rappel = Lire; "
            "structure Registre { pointeur_fonction<entier32(constante S*)> Rappels[2]; "
            "S***** Adresses[2]; }; Registre Globale = {{Lire, Lire}}; "
            "énumération E { X = 42 }; N::E Valeur = E::X; }";
        ComparerEmissionGlobales(syntaxe, semantique, emettre,
            nomsImbriques, "emission-noms-imbriques-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre,
            TraduireCorpusConversions(nomsImbriques), "emission-noms-imbriques-en");
        const std::string signatures =
            "structure S { entier32 V; }; "
            "publique S Fabriquer(entier32 a, entier32 b, entier32 c) { retourner {a + b + c}; } "
            "pointeur_fonction<S(entier32, entier32, entier32)> Rappel = Fabriquer; "
            "publique entier32 Somme(entier32 a, entier32 b, entier32 c, entier32 d) { retourner a + b + c + d; } "
            "pointeur_fonction<entier32(entier32, entier32, entier32, entier32)> Addition = Somme;";
        ComparerEmissionGlobales(syntaxe, semantique, emettre, signatures, "emission-signatures-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre,
            TraduireCorpusConversions(signatures), "emission-signatures-en");
        const std::string aliasesChamps =
            "structure Bloc { octet Etiquette; alias Vue = Valeurs; alias Tableau = Vue; "
            "entier32 Valeurs[2]; }; "
            "union Choix { naturel64 Large; alias Copie = Vue; alias Vue = Large; entier8 Court; }; "
            "Bloc Donnees = {7, {42, -3}}; Choix Selection = {123}; publique vide F() {}";
        ComparerEmissionGlobales(syntaxe, semantique, emettre,
            aliasesChamps, "emission-aliases-champs-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre,
            TraduireCorpusConversions(aliasesChamps), "emission-aliases-champs-en");
        const std::string aliasesRacines =
            "alias Vue = Copie; alias Copie = Bloc; alias Appeler = Lire; alias Objet = Globale; "
            "structure Bloc { octet Etiquette; entier32 Valeurs[2]; }; "
            "publique entier32 Lire(Vue* p) { retourner p->Valeurs[1]; } "
            "Vue Globale = {7, {11, 42}}; Vue Zero[2]; "
            "pointeur_fonction<entier32(Bloc*)> Rappels[2] = {Appeler, Appeler}; "
            "publique entier32 F() { retourner Appeler(&Objet); }";
        ComparerEmissionGlobales(syntaxe, semantique, emettre, aliasesRacines,
            "emission-aliases-racines-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre, TraduireCorpusConversions(aliasesRacines),
            "emission-aliases-racines-en");
        const std::string prioriteAlias =
            "espace N { structure S { entier64 Y; }; alias Vue = S; Vue Globale = {42}; } "
            "structure S { entier32 X; }; N::Vue Autre = {7}; publique vide F() {}";
        ComparerEmissionGlobales(syntaxe, semantique, emettre, prioriteAlias,
            "emission-priorite-alias-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre, TraduireCorpusConversions(prioriteAlias),
            "emission-priorite-alias-en");
        const std::string aliasesMethodes =
            "espace N { classe C { publique: entier32 Lire(entier32 x) { retourner x; } }; alias Vue = C; } "
            "alias Appeler = Copie; alias Copie = N::C::Lire; "
            "pointeur_fonction<entier32(N::Vue&, entier32)> Rappels[2] = {Appeler, Copie}; "
            "structure Registre { pointeur_fonction<entier32(N::C&, entier32)> Actif; }; "
            "Registre Table = {Appeler}; publique entier32 F(N::C& objet) { retourner Appeler(objet, 42); }";
        ComparerEmissionGlobales(syntaxe, semantique, emettre, aliasesMethodes,
            "emission-aliases-methodes-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre, TraduireCorpusConversions(aliasesMethodes),
            "emission-aliases-methodes-en");
        const std::string retourAgregeMethode =
            "structure S { entier32 X; }; classe C { publique: S Creer(entier32 x, entier32 y) { retourner {x + y}; } }; "
            "alias Appeler = C::Creer; pointeur_fonction<S(C&, entier32, entier32)> Rappel = Appeler;";
        ComparerEmissionGlobales(syntaxe, semantique, emettre, retourAgregeMethode,
            "emission-alias-methode-retour-agrege-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre, TraduireCorpusConversions(retourAgregeMethode),
            "emission-alias-methode-retour-agrege-en");
        const std::string heritage =
            "espace N { classe B { publique: entier32 X; }; alias Vue = B; "
            "classe D : publique Vue {}; } entier32 A = 42; "
            "publique entier32 F(N::D& objet) { retourner objet.X; }";
        ComparerEmissionGlobales(syntaxe, semantique, emettre, heritage, "emission-heritage-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre, TraduireCorpusConversions(heritage),
            "emission-heritage-en");
        const std::string heritageQualifie =
            "espace N { espace A { classe B {}; } classe D : publique A::B {}; } "
            "entier32 A = 42; publique vide F(N::D& objet) { N::A::B& base = objet; }";
        ComparerEmissionGlobales(syntaxe, semantique, emettre, heritageQualifie,
            "emission-heritage-qualifie-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre, TraduireCorpusConversions(heritageQualifie),
            "emission-heritage-qualifie-en");
        const std::string remplacements =
            "classe B { publique: virtuel entier32 Lire(entier32 x) { retourner x; } }; "
            "classe D : publique B { publique: remplacer entier32 Lire(entier32 x) { retourner x + 1; } }; "
            "alias LireBase = B::Lire; alias LireDerivee = D::Lire; entier32 A = 42; "
            "pointeur_fonction<entier32(B&, entier32)> RappelBase = LireBase; "
            "pointeur_fonction<entier32(D&, entier32)> RappelDerive = LireDerivee;";
        ComparerEmissionGlobales(syntaxe, semantique, emettre, remplacements, "emission-remplacements-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre, TraduireCorpusConversions(remplacements),
            "emission-remplacements-en");
        const std::string destructeursVirtuels =
            "classe B { publique: virtuel destructeur() {} }; classe D : publique B { "
            "publique: remplacer destructeur() {} }; entier32 A = 42; publique vide F(D& objet) {}";
        ComparerEmissionGlobales(syntaxe, semantique, emettre, destructeursVirtuels,
            "emission-destructeurs-virtuels-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre, TraduireCorpusConversions(destructeursVirtuels),
            "emission-destructeurs-virtuels-en");
        const std::string surchargesLibres =
            "publique entier32 Lire(entier32 x) { retourner x; } "
            "publique entier64 Lire(entier64 x) { retourner x; } entier32 A = 42; "
            "publique entier32 Appeler(entier32 x) { "
            "retourner Lire(x) + convertir<entier32>(Lire(convertir<entier64>(x))); } "
            "pointeur_fonction<entier32(entier32)> Rappel = Appeler;";
        ComparerEmissionGlobales(syntaxe, semantique, emettre, surchargesLibres,
            "emission-surcharges-distinctes-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre, TraduireCorpusConversions(surchargesLibres),
            "emission-surcharges-distinctes-en");
        const std::string surchargesMethodes =
            "classe C { publique: entier32 Lire(entier32 x) { retourner x; } "
            "entier64 Lire(entier64 x) { retourner x; } }; entier32 A = 42; "
            "publique entier32 AppelerCourt(C& objet, entier32 x) { retourner objet.Lire(x); } "
            "publique entier64 AppelerLong(C& objet, entier64 x) { retourner objet.Lire(x); } "
            "pointeur_fonction<entier32(C&, entier32)> Court = AppelerCourt; "
            "pointeur_fonction<entier64(C&, entier64)> Long = AppelerLong;";
        ComparerEmissionGlobales(syntaxe, semantique, emettre, surchargesMethodes,
            "emission-surcharges-methodes-distinctes-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre, TraduireCorpusConversions(surchargesMethodes),
            "emission-surcharges-methodes-distinctes-en");
        const std::string signaturesNonLiees =
            "classe C { publique: vide F() {} }; espace C { publique vide F(C* objet) {} } "
            "entier32 A = 42; publique entier32 PointEntree() { retourner A; } "
            "pointeur_fonction<entier32()> Rappel = PointEntree;";
        ComparerEmissionGlobales(syntaxe, semantique, emettre, signaturesNonLiees,
            "emission-signatures-non-liees-distinctes-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre, TraduireCorpusConversions(signaturesNonLiees),
            "emission-signatures-non-liees-distinctes-en");
        const std::string signaturesNonLieesQualifiees =
            "espace N { classe C { publique: vide F(entier32 x) {} }; "
            "espace C { publique vide F(N::C& objet, entier64 y) {} } } "
            "entier32 A = 42; publique vide PointEntree() {} "
            "pointeur_fonction<vide()> Rappel = PointEntree;";
        ComparerEmissionGlobales(syntaxe, semantique, emettre, signaturesNonLieesQualifiees,
            "emission-signatures-non-liees-qualifiees-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre,
            TraduireCorpusConversions(signaturesNonLieesQualifiees), "emission-signatures-non-liees-qualifiees-en");
        const std::string appelsMixtes =
            "classe C { publique: entier32 Lire(entier32 x) { retourner x; } }; "
            "espace C { publique entier32 Lire(C& objet, booléen x) { retourner 2; } } "
            "entier32 A = 42; publique entier32 AppelerMethode(C& objet) { retourner C::Lire(objet, A); } "
            "publique entier32 AppelerLibre(C& objet) { retourner objet.Lire(vrai); } "
            "pointeur_fonction<entier32(C&)> Rappels[2] = {AppelerMethode, AppelerLibre};";
        ComparerEmissionGlobales(syntaxe, semantique, emettre, appelsMixtes, "emission-appels-mixtes-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre,
            TraduireCorpusConversions(appelsMixtes), "emission-appels-mixtes-en");
        const std::string appelsMixtesQualifies =
            "espace N { classe C { publique: entier32 Lire(entier32 x) { retourner x; } }; "
            "espace C { publique entier32 Lire(N::C& objet, booléen x) { retourner 2; } } } "
            "entier32 A = 42; publique entier32 Appeler(N::C* objet) { retourner objet->Lire(vrai); } "
            "pointeur_fonction<entier32(N::C*)> Rappel = Appeler;";
        ComparerEmissionGlobales(syntaxe, semantique, emettre, appelsMixtesQualifies, "emission-appels-mixtes-qualifies-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre,
            TraduireCorpusConversions(appelsMixtesQualifies), "emission-appels-mixtes-qualifies-en");
        const std::string operateursMixtes =
            "classe C { publique: entier32 opérateur+(entier32 x) { retourner x; } }; "
            "espace C { publique entier32 opérateur+(C& objet, booléen x) { retourner 2; } } "
            "entier32 A = 42; publique entier32 AppelerMembre(C& objet) { retourner objet + A; } "
            "publique entier32 AppelerLibre(C& objet) { retourner objet + vrai; } "
            "pointeur_fonction<entier32(C&)> Rappels[2] = {AppelerMembre, AppelerLibre};";
        ComparerEmissionGlobales(syntaxe, semantique, emettre, operateursMixtes, "emission-operateurs-mixtes-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre,
            TraduireCorpusConversions(operateursMixtes), "emission-operateurs-mixtes-en");
        const std::string operateursUnairesMixtes =
            "classe C { publique: booléen opérateur!() { retourner vrai; } }; "
            "espace C { publique booléen opérateur!(constante C& objet) { retourner faux; } } "
            "entier32 A = 42; publique booléen Appeler(constante C& objet) { retourner !objet; } "
            "pointeur_fonction<booléen(constante C&)> Rappel = Appeler;";
        ComparerEmissionGlobales(syntaxe, semantique, emettre, operateursUnairesMixtes,
            "emission-operateurs-unaires-mixtes-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre,
            TraduireCorpusConversions(operateursUnairesMixtes), "emission-operateurs-unaires-mixtes-en");
        const std::string instructionsOrdonnees =
            "classe L { publique: entier32 Lire(entier32 x) { retourner x; } }; "
            "espace L { publique entier32 Lire(L& objet, booléen x) { retourner 2; } } "
            "entier32 A = 42; publique entier32 G(L& objet, entier32 x) { "
            "entier32 a = objet.Lire(x); { entier32 b = objet.Lire(vrai); a = a + b; } retourner a; } "
            "publique entier32 H(L& objet, entier32 x) { si (x == 0) { retourner objet.Lire(vrai); } "
            "retourner objet.Lire(x); } "
            "pointeur_fonction<entier32(L&, entier32)> Rappels[2] = {G, H};";
        ComparerEmissionGlobales(syntaxe, semantique, emettre, instructionsOrdonnees,
            "emission-instructions-ordonnees-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre,
            TraduireCorpusConversions(instructionsOrdonnees), "emission-instructions-ordonnees-en");
        const std::string boucleOrdonnee =
            "entier32 A = 42; publique entier32 G(entier32 x) { entier32 a = 0; "
            "tantque (a < x) { a = a + 1; } retourner a; } "
            "pointeur_fonction<entier32(entier32)> Rappel = G;";
        ComparerEmissionGlobales(syntaxe, semantique, emettre, boucleOrdonnee, "emission-boucle-ordonnee-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre,
            TraduireCorpusConversions(boucleOrdonnee), "emission-boucle-ordonnee-en");
        const std::string expressionsOrdonnees =
            "entier32 A = (2 + 3) * (4 - 1); publique entier32 G(entier32 x) { "
            "entier32 valeurs[2] = {x + 1, x + 2}; retourner valeurs[0] + valeurs[1]; } "
            "pointeur_fonction<entier32(entier32)> Rappel = G;";
        ComparerEmissionGlobales(syntaxe, semantique, emettre, expressionsOrdonnees, "emission-expressions-ordonnees-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre,
            TraduireCorpusConversions(expressionsOrdonnees), "emission-expressions-ordonnees-en");
        const std::string indexationsOrdonnees =
            "entier32 Valeurs[2] = {3, 4}; publique entier32 G(entier32* p, entier32 x) { "
            "retourner p[x] + p[x + 1]; } "
            "pointeur_fonction<entier32(entier32*, entier32)> Rappel = G;";
        ComparerEmissionGlobales(syntaxe, semantique, emettre, indexationsOrdonnees, "emission-indexations-ordonnees-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre,
            TraduireCorpusConversions(indexationsOrdonnees), "emission-indexations-ordonnees-en");
        const std::string callbacksOrdonnes =
            "entier32 A = 42; publique entier32 Appliquer("
            "pointeur_fonction<entier32(entier32, entier32)> operation, entier32 x) { "
            "retourner operation(x + 1, x - 1); } "
            "pointeur_fonction<entier32(pointeur_fonction<entier32(entier32, entier32)>, entier32)> Rappel = Appliquer;";
        ComparerEmissionGlobales(syntaxe, semantique, emettre, callbacksOrdonnes, "emission-callbacks-ordonnes-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre,
            TraduireCorpusConversions(callbacksOrdonnes), "emission-callbacks-ordonnes-en");
        const std::string appelsMembresOrdonnes =
            "classe L { publique: entier32 Lire(entier32 a, entier32 b) { retourner a + b; } }; "
            "entier32 A = 42; publique entier32 G(L& objet, entier32 x) { retourner objet.Lire(x + 1, x - 1); } "
            "pointeur_fonction<entier32(L&, entier32)> Rappel = G;";
        ComparerEmissionGlobales(syntaxe, semantique, emettre, appelsMembresOrdonnes, "emission-appels-membres-ordonnes-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre,
            TraduireCorpusConversions(appelsMembresOrdonnes), "emission-appels-membres-ordonnes-en");
        const std::string candidatSuivant =
            "entier32 A = 42; publique entier32 Lire(entier32& a, entier32 b) { retourner b; } "
            "publique entier32 Lire(booléen a, entier32 b) { retourner b; } "
            "publique entier32 G(entier32 x) { retourner Lire(vrai, x + 1); } "
            "pointeur_fonction<entier32(entier32)> Rappel = G;";
        ComparerEmissionGlobales(syntaxe, semantique, emettre, candidatSuivant, "emission-candidat-suivant-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre,
            TraduireCorpusConversions(candidatSuivant), "emission-candidat-suivant-en");
        const std::string candidatMembreSuivant =
            "classe L { publique: entier32 Lire(entier32 a, entier32 b) { retourner b; } }; "
            "espace L { publique entier32 Lire(L& objet, booléen a, entier32 b) { retourner b; } } "
            "entier32 A = 42; publique entier32 G(L& objet, entier32 x) { retourner objet.Lire(vrai, x + 1); } "
            "pointeur_fonction<entier32(L&, entier32)> Rappel = G;";
        ComparerEmissionGlobales(syntaxe, semantique, emettre, candidatMembreSuivant, "emission-candidat-membre-suivant-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre,
            TraduireCorpusConversions(candidatMembreSuivant), "emission-candidat-membre-suivant-en");
        const std::string argumentsContextuels =
            "entier32 A = 42; structure Point { entier32 X; entier32 Y; }; "
            "publique entier32 Lire(Point p) { retourner p.X + p.Y; } "
            "publique entier32 G() { retourner Lire({1, {2}}); } "
            "pointeur_fonction<entier32()> Rappel = G;";
        ComparerEmissionGlobales(syntaxe, semantique, emettre, argumentsContextuels, "emission-arguments-contextuels-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre,
            TraduireCorpusConversions(argumentsContextuels), "emission-arguments-contextuels-en");
        const std::string callbackContextuel =
            "entier32 A = 42; publique entier32 Lire(entier32 x) { retourner x; } "
            "publique entier32 G(pointeur_fonction<entier32(entier32)> f) { retourner f({{Lire({3})}}); } "
            "pointeur_fonction<entier32(pointeur_fonction<entier32(entier32)>)> Rappel = G;";
        ComparerEmissionGlobales(syntaxe, semantique, emettre, callbackContextuel, "emission-callback-contextuel-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre,
            TraduireCorpusConversions(callbackContextuel), "emission-callback-contextuel-en");
        const std::string nomsLiaisonDistincts =
            DeclarerTypesCollisionLiaison(NomCollisionLiaisonA, NomCollisionLiaisonB)
            + "publique entier32 F(" + NomCollisionLiaisonA + "* x) { retourner 1; } "
              "publique entier32 G(" + NomCollisionLiaisonB + "* x) { retourner 2; } "
              "entier32 A = 42; pointeur_fonction<entier32(" + NomCollisionLiaisonA + "*)> Rappel = F;";
        ComparerEmissionGlobales(syntaxe, semantique, emettre, nomsLiaisonDistincts,
            "emission-noms-liaison-distincts-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre, TraduireCorpusConversions(nomsLiaisonDistincts),
            "emission-noms-liaison-distincts-en");
        const std::string surchargesSansCollision =
            DeclarerTypesCollisionLiaison(NomCollisionLiaisonA, NomCollisionLiaisonB)
            + "publique vide F(" + NomCollisionLiaisonA + "* x) {} "
              "publique vide F(" + NomCollisionLiaisonB + "** x) {} entier32 A = 42; "
              "publique vide PointEntree() {} pointeur_fonction<vide()> Rappel = PointEntree;";
        ComparerEmissionGlobales(syntaxe, semantique, emettre, surchargesSansCollision,
            "emission-surcharges-sans-collision-fr");
        ComparerEmissionGlobales(syntaxe, semantique, emettre, TraduireCorpusConversions(surchargesSansCollision),
            "emission-surcharges-sans-collision-en");
        ComparerEmissionGlobales(syntaxe, semantique, emettre,
            "externe entier32 Importee; publique vide F() {}", "emission-import-seul");
        ComparerEmissionGlobales(syntaxe, semantique, emettre,
            "int32 Zero; public void F() {}", "emission-zero-seul");
        ComparerEmissionGlobales(syntaxe, semantique, emettre,
            "public void F() {}", "emission-sans-globale");

        const auto verifierRefus = [&](const std::string& texte, std::uint32_t code,
                                      bool differentiel)
        {
            if (differentiel)
                ComparerErreurSemantique(syntaxe, semantique, texte, code, "emission-refusee");
            const auto ast = ComparerDeclarations(syntaxe, texte, "emission-refusee");
            std::array<GlobaleEmiseHote, 4> globales;
            std::array<RelocalisationGlobaleHote, 4> relocations;
            std::array<std::uint8_t, 256> donnees;
            std::memset(globales.data(), 0xA5, sizeof(globales));
            std::memset(relocations.data(), 0xA5, sizeof(relocations));
            donnees.fill(0xA5);
            const auto globalesAvant = globales;
            const auto relocationsAvant = relocations;
            RequeteEmissionGlobalesHote requete{
                texte.data(), texte.size(), ast.data(), ast.size(),
                globales.data(), globales.size(), donnees.data(), donnees.size(),
                relocations.data(), relocations.size(), {}};
            Exiger(emettre(&requete) == code && requete.Resultat.Erreur == code
                       && requete.Resultat.LigneErreur != 0
                       && requete.Resultat.ColonneErreur != 0
                       && std::memcmp(globales.data(), globalesAvant.data(), sizeof(globales)) == 0
                       && std::memcmp(relocations.data(), relocationsAvant.data(), sizeof(relocations)) == 0
                       && std::all_of(donnees.begin(), donnees.end(), [](auto b) { return b == 0xA5; }),
                   "refus d'émission incorrect ou sortie partiellement écrite : " + texte);
            if (differentiel)
            {
                RequeteAnalyseSemantiqueHote analyse{
                    texte.data(), texte.size(), ast.data(), ast.size(), nullptr, 0, nullptr, 0, {}};
                Exiger(semantique(&analyse) == code
                           && requete.Resultat.LigneErreur == analyse.Resultat.LigneErreur
                           && requete.Resultat.ColonneErreur == analyse.Resultat.ColonneErreur,
                       "position du diagnostic perdue pendant l'émission");
            }
        };
        verifierRefus("entier32 A = 42; entier32 B = 1 / 0; publique vide F() {}", 89, true);
        verifierRefus("int32 A = 42; int32 B = 1 / 0; public void F() {}", 89, true);
        verifierRefus("externe entier32 I; entier32* P = &I; publique vide F() {}", 83, true);
        verifierRefus("extern int32 I; int32* P = &I; public void F() {}", 83, true);
        verifierRefus("espace A { énumération E { X }; } espace B { énumération E { X }; } "
                     "A::E Valeur = B::E::X; publique vide F() {}", 45, true);
        verifierRefus("namespace A { enumeration E { X }; } namespace B { enumeration E { X }; } "
                     "A::E Value = B::E::X; public void F() {}", 45, true);
        verifierRefus("publique booléen F() { retourner vrai; } "
                     "pointeur_fonction<entier32()> R[1] = {F};", 45, true);
        verifierRefus("public bool F() { return true; } "
                     "function_pointer<int32()> R[1] = {F};", 45, true);
        for (const std::string texte : {
                 "entier32 A = 42; pointeur_fonction<vide(vide)> P; publique vide F() {}",
                 "entier32 A = 42; pointeur_fonction<vide(entier32, entier32, entier32, entier32, entier32)> P; publique vide F() {}"})
        {
            const auto code = texte.find("vide(vide)") != std::string::npos ? 102U : 103U;
            verifierRefus(texte, code, false);
            verifierRefus(TraduireCorpusConversions(texte), code, false);
        }
        for (const auto& [texte, code] : std::vector<std::pair<std::string, std::uint32_t>>{
                 {"entier32 A = 42; structure S { alias Vue = Absente; }; publique vide F() {}", 108},
                 {"entier32 A = 42; structure S { alias Vue = Copie; alias Copie = Vue; }; publique vide F() {}", 107},
                 {"entier32 A = 42; alias Vue = Absente; publique vide F() {}", 110},
                 {"entier32 A = 42; alias Vue = Copie; alias Copie = Vue; publique vide F() {}", 109},
                 {"entier32 A = 42; alias Appeler = F; publique vide F(entier32 x) {} publique vide F(entier64 x) {}", 112},
                 {"entier32 A = 42; classe C { publique: entier32 Lire() { retourner 42; } }; alias Appeler = C::Lire; "
                  "pointeur_fonction<entier32()> Rappel = Appeler;", 45},
                 {"entier32 A = 42; classe C { publique: entier32 Lire() { retourner 42; } }; alias Appeler = C::Lire; "
                  "publique entier32 F() { retourner Appeler(); }", 21},
                 {"entier32 A = 42; classe C { privée: entier32 Lire() { retourner 42; } }; alias Appeler = C::Lire; "
                  "publique entier32 F(C& objet) { retourner Appeler(objet); }", 25},
                 {"entier32 A = 42; classe B {}; classe D : privée B {}; publique vide F() {}", 113},
                 {"entier32 A = 42; classe B {}; classe D : protégée B {}; publique vide F() {}", 113},
                 {"entier32 A = 42; classe D : publique Absente {}; publique vide F() {}", 100},
                 {"entier32 A = 42; structure B {}; classe D : publique B {}; publique vide F() {}", 114},
                 {"entier32 A = 42; énumération B { X }; classe D : publique B {}; publique vide F() {}", 114},
                 {"entier32 A = 42; classe D : publique D {}; publique vide F() {}", 115},
                 {"entier32 A = 42; classe D : publique Vue {}; alias Vue = D; publique vide F() {}", 115},
                 {"entier32 A = 42; classe B : publique C {}; classe C : publique B {}; publique vide F() {}", 57},
                 {"entier32 A = 42; classe C { publique: remplacer entier32 Lire() { retourner 1; } };", 116},
                 {"entier32 A = 42; classe B { publique: virtuel entier32 Lire() { retourner 1; } }; "
                  "classe D : publique B { publique: entier32 Lire() { retourner 2; } };", 117},
                 {"entier32 A = 42; classe C { publique: remplacer destructeur() {} };", 116},
                 {"entier32 A = 42; classe B { publique: virtuel entier32 opérateur+(entier32 x) { retourner x; } }; "
                  "classe D : publique B { publique: entier32 opérateur+(entier32 x) { retourner x; } };", 117},
                 {"entier32 A = 42; publique vide F() {} publique vide F() {}", 118},
                 {"entier32 A = 42; classe C { publique: constructeur() {} constructeur() {} };", 118},
                 {"entier32 A = 42; classe C { publique: destructeur() {} destructeur() {} };", 118},
                 {"entier32 A = 42; classe C { publique: entier32 opérateur+(entier32 a) { retourner a; } "
                  "entier32 opérateur+(entier32 b) { retourner b; } };", 118},
                 {"entier32 A = 42; structure S {}; publique entier32 opérateur+(S& a, entier32 x) { retourner x; } "
                  "publique entier32 opérateur+(S& b, entier32 y) { retourner y; }", 118},
                 {"entier32 A = 42; classe C { publique: vide F() {} }; "
                  "espace C { publique vide F(C& objet) {} }", 118},
                 {"entier32 A = 42; alias Vue = C; classe C { publique: vide F() {} }; "
                  "espace C { publique vide F(Vue& objet) {} }", 118},
                 {"entier32 A = 42; classe C { publique: entier32 opérateur+(entier32 a) { retourner a; } }; "
                  "espace C { publique entier32 opérateur+(C& objet, entier32 b) { retourner b; } }", 118},
                 {"entier32 A = 42; espace C { publique booléen opérateur!(C& objet) { retourner vrai; } } "
                  "classe C { publique: booléen opérateur!() { retourner faux; } };", 118}})
        {
            verifierRefus(texte, code, true);
            verifierRefus(TraduireCorpusConversions(texte), code, true);
        }
        for (const auto& [texte, code] : std::vector<std::pair<std::string, std::uint32_t>>{
                 {"entier32 A = 42; classe C { publique: entier32 opérateur+(entier32 x) { retourner x; } }; "
                  "espace C { publique entier32 opérateur+(constante C& objet, entier32 x) { retourner x; } } "
                  "publique entier32 G(C& objet, entier32 x) { retourner objet + x; }", 22},
                 {"entier32 A = 42; classe C { publique: entier32 opérateur+(entier32 x) { retourner x; } }; "
                  "publique entier32 G(constante C& objet, entier32 x) { retourner objet + x; }", 21},
                 {"entier32 A = 42; classe C { privée: entier32 opérateur+(entier32 x) { retourner x; } }; "
                  "espace C { publique entier32 opérateur+(C& objet, entier64 x) { retourner 2; } } "
                  "publique entier32 G(C& objet) { retourner objet + 7; }", 25},
                 {"entier32 A = 42; publique vide F(entier32 a) {} publique vide G(entier32 a) {} "
                  "publique vide G(entier32 b) {} publique vide F(entier32 b) {}", 118},
                 {"entier32 A = 42; classe C { privée: entier32 opérateur+(entier32 x) { retourner x; } }; "
                  "classe D { publique: booléen opérateur!() { retourner vrai; } }; "
                  "publique entier32 F(C& objet) { retourner objet + 7; } "
                  "publique booléen G(constante D& objet) { retourner !objet; }", 25},
                 {DeclarerTypesCollisionLiaison(NomCollisionLiaisonA, NomCollisionLiaisonB)
                  + "entier32 A = 42; publique vide F(" + NomCollisionLiaisonA + "* a) {} "
                    "publique vide G(entier32 a) {} publique vide F(" + NomCollisionLiaisonB + "* b) {} "
                    "publique vide G(entier32 b) {}", 118}})
        {
            verifierRefus(texte, code, true);
            verifierRefus(TraduireCorpusConversions(texte), code, true);
        }
        for (const auto& texte : std::vector<std::string>{
                 DeclarerTypesCollisionLiaison(NomCollisionLiaisonA, NomCollisionLiaisonB)
                     + "entier32 A = 42; publique vide F(" + NomCollisionLiaisonA + "* x) {} "
                       "publique vide F(" + NomCollisionLiaisonB + "* y) {}",
                 DeclarerTypesCollisionLiaison(NomCollisionRecepteurA, NomCollisionRecepteurB)
                     + "entier32 A = 42; classe C { publique: vide F(" + NomCollisionRecepteurA + "* x) {} "
                       "vide F(" + NomCollisionRecepteurB + "* y) {} };",
                 DeclarerTypesCollisionLiaison(NomCollisionCallbackA, NomCollisionCallbackB)
                     + "entier32 A = 42; publique vide F(pointeur_fonction<vide(" + NomCollisionCallbackA + "*)> x) {} "
                       "publique vide F(pointeur_fonction<vide(" + NomCollisionCallbackB + "*)> y) {}",
                 "espace A::B { " + DeclarerTypesCollisionLiaison(NomCollisionQualifieA, NomCollisionQualifieB)
                     + "} entier32 A = 42; publique vide F(A::B::" + NomCollisionQualifieA + "* x) {} "
                       "publique vide F(A::B::" + NomCollisionQualifieB + "* y) {}"})
        {
            verifierRefus(texte, 119, true);
            verifierRefus(TraduireCorpusConversions(texte), 119, true);
        }
        for (const auto& [texte, code] : std::vector<std::pair<std::string, std::uint32_t>>{
                 {"entier32 A = 42; classe C { publique: entier32 Lire() { retourner 1; } }; "
                  "espace C { publique entier32 Lire(constante C& objet) { retourner 2; } } "
                  "publique entier32 G(C& objet) { retourner C::Lire(objet); }", 22},
                 {"entier32 A = 42; classe C { publique: entier32 Lire() { retourner 1; } }; "
                  "espace C { publique entier32 Lire(constante C& objet) { retourner 2; } } "
                  "publique entier32 G(C* objet) { retourner objet->Lire(); }", 22},
                 {"entier32 A = 42; classe C { privée: entier32 Lire(entier32 x) { retourner x; } }; "
                  "espace C { publique entier32 Lire(C& objet, booléen x) { retourner 2; } } "
                  "publique entier32 G(C& objet, entier32 x) { retourner objet.Lire(x); }", 25},
                 {"entier32 A = 42; classe C { publique: entier32 Lire(entier32 x) { retourner x; } }; "
                  "espace C { publique entier32 Lire(C& objet, booléen x) { retourner 2; } } "
                  "publique entier32 G(C& objet) { retourner objet.Lire(); }", 21}})
        {
            verifierRefus(texte, code, true);
            verifierRefus(TraduireCorpusConversions(texte), code, true);
        }
        const std::string declarationsPriorites =
            "entier32 A = 42; classe C { privée: entier32 opérateur+(entier32 x) { retourner x; } "
            "publique: booléen opérateur!() { retourner vrai; } }; "
            "publique entier32 Choisir(entier8 x) { retourner 1; } "
            "publique entier32 Choisir(naturel8 x) { retourner 2; } ";
        for (const auto& [corps, code] : std::vector<std::pair<std::string, std::uint32_t>>{
                 {"objet + 7; !autre;", 25},
                 {"!autre; objet + 7;", 21},
                 {"entier32 a = Choisir(7); objet + 7;", 22},
                 {"entier32 a = 0; a = vrai; !autre;", 73},
                 {"si (!autre) { objet + 7; } Choisir(7);", 21},
                 {"tantque (faux) { objet + 7; } !autre;", 25}})
        {
            const auto texte = declarationsPriorites + "publique vide G(C& objet, constante C& autre) { "
                + corps + " }";
            verifierRefus(texte, code, true);
            verifierRefus(TraduireCorpusConversions(texte), code, true);
        }
        for (const auto& [corps, code] : std::vector<std::pair<std::string, std::uint32_t>>{
                 {"(objet + 7) + convertir<entier32>(!autre);", 25},
                 {"convertir<entier32>(!autre) + (objet + 7);", 21},
                 {"objet.Absent[Choisir(7)];", 24},
                 {"1 = objet + 7;", 70},
                 {"convertir<C>(objet + 7);", 94},
                 {"entier32 valeurs[2] = {objet + 7, convertir<entier32>(!autre)};", 25}})
        {
            const auto texte = declarationsPriorites + "publique vide G(C& objet, constante C& autre) { "
                + corps + " }";
            verifierRefus(texte, code, true);
            verifierRefus(TraduireCorpusConversions(texte), code, true);
        }
        for (const auto& [corps, code] : std::vector<std::pair<std::string, std::uint32_t>>{
                 {"operation(objet + 7, convertir<entier32>(!autre));", 25},
                 {"operation(convertir<entier32>(!autre), objet + 7);", 21},
                 {"operation(objet + 7);", 54},
                 {"entier32 x = 0; x(objet + 7);", 53},
                 {"Absente(objet + 7);", 18},
                 {"operation(vrai, objet + 7);", 55}})
        {
            const auto texte = declarationsPriorites + "publique vide G(C& objet, constante C& autre, "
                "pointeur_fonction<entier32(entier32, entier32)> operation) { " + corps + " }";
            verifierRefus(texte, code, true);
            verifierRefus(TraduireCorpusConversions(texte), code, true);
        }
        const std::string declarationsAbandons =
            "entier32 A = 42; classe C { privée: entier32 opérateur+(entier32 x) { retourner x; } "
            "publique: entier32 Lire(entier32 a, entier32 b) { retourner b; } }; "
            "publique entier32 Unique(entier32 a, entier32 b) { retourner b; } "
            "publique entier32 Reference(entier32& a, entier32 b) { retourner b; } ";
        for (const auto& [corps, code] : std::vector<std::pair<std::string, std::uint32_t>>{
                 {"Unique(vrai, objet + 7);", 21},
                 {"Unique(objet + 7, vrai);", 25},
                 {"Reference(1, objet + 7);", 21},
                 {"objet.Lire(vrai, objet + 7);", 21},
                 {"Unique(vrai, convertir<Inconnue*>(objet + 7));", 21},
                 {"Unique(0, convertir<Inconnue*>(objet + 7));", 99}})
        {
            const auto texte = declarationsAbandons + "publique vide G(C& objet) { " + corps + " }";
            verifierRefus(texte, code, true);
            verifierRefus(TraduireCorpusConversions(texte), code, true);
        }
        const std::string declarationsContextuelles =
            "entier32 A = 42; structure P { entier32 X; }; "
            "classe C { privée: entier32 opérateur+(entier32 x) { retourner x; } }; "
            "publique vide Scalaire(entier32 a, entier32 b) {} "
            "publique vide Agrege(P p) {} publique vide Reference(entier32& x) {} "
            "publique vide Ambigu(entier32 x) {} publique vide Ambigu(entier64 x) {} ";
        for (const auto& [corps, code] : std::vector<std::pair<std::string, std::uint32_t>>{
                 {"Scalaire({Absente, 2}, 0);", 44},
                 {"Scalaire({Absente}, objet + 7);", 25},
                 {"Ambigu({Absente});", 22},
                 {"Agrege({Absente, 2});", 43},
                 {"pointeur_fonction<vide(entier32, entier32)> f = Scalaire; f({Absente, 2}, objet + 7);", 44},
                 {"pointeur_fonction<vide(entier32&)> f = Reference; f({Absente});", 69}})
        {
            const auto texte = declarationsContextuelles + "publique vide G(C& objet) { " + corps + " }";
            verifierRefus(texte, code, true);
            verifierRefus(TraduireCorpusConversions(texte), code, true);
        }
        const std::string declarationsConversions =
            "entier32 A = 42; classe C { privée: entier32 opérateur+(entier32 x) { retourner x; } }; "
            "publique vide Lire(naturel8 x, entier32 y) {} ";
        for (const auto& [corps, code] : std::vector<std::pair<std::string, std::uint32_t>>{
                 {"convertir<naturel8>(256); objet + 7;", 98},
                 {"objet + 7; convertir<naturel8>(256);", 25},
                 {"convertir<entier32>(1 / 0); objet + 7;", 89},
                 {"Lire(convertir<naturel8>(256), objet + 7);", 98},
                 {"Lire(vrai, convertir<naturel8>(256));", 21},
                 {"pointeur_fonction<vide(naturel8, entier32)> f = Lire; "
                  "f(convertir<naturel8>(256), objet + 7);", 98}})
        {
            const auto texte = declarationsConversions + "publique vide G(C& objet) { " + corps + " }";
            verifierRefus(texte, code, true);
            verifierRefus(TraduireCorpusConversions(texte), code, true);
        }
        const std::vector<std::string> conversionsValides{
            "naturel8 Octet = convertir<naturel8>(255); booléen Actif = convertir<booléen>(300); "
            "publique naturel8 G(entier32 valeur) { retourner convertir<naturel8>(valeur); }",
            "énumération E { X = convertir<naturel8>(255) }; naturel8 Octet = convertir<naturel8>(E::X); "
            "publique vide G() { convertir<naturel8>(faux && (1 / 0)); }",
        };
        for (std::size_t index = 0; index < conversionsValides.size(); ++index)
            for (const auto& texte : {conversionsValides[index], TraduireCorpusConversions(conversionsValides[index])})
                ComparerEmissionGlobales(syntaxe, semantique, emettre, texte,
                    "emission-conversion-constante-valide-" + std::to_string(index));
        // Chaque objet tient sur 32 bits, mais leur zone commune dépasse la limite.
        verifierRefus("octet A[2147483647]; octet B[2147483647]; octet C[2]; publique vide F() {}", 58, false);
        verifierRefus("byte A[2147483647] = {}; byte B[2147483647] = {}; byte C[2] = {}; public void F() {}", 58, false);
        Exiger(emettre(nullptr) == 1, "requête d'émission nulle acceptée");
        RequeteEmissionGlobalesHote invalide{};
        Exiger(emettre(&invalide) == 1 && invalide.Resultat.Erreur == 1,
               "source d'émission nulle acceptée");
        const std::string source = "entier32 Globale = 1; publique vide F() {}";
        auto noeuds = ComparerDeclarations(syntaxe, source, "emission-arguments");
        invalide = {source.data(), source.size(), noeuds.data(), noeuds.size(),
                    nullptr, 0, nullptr, 0, nullptr, 0, {}};
        for (auto* capacite : {&invalide.CapaciteGlobales, &invalide.CapaciteDonnees,
                              &invalide.CapaciteRelocalisations})
        {
            *capacite = 1;
            Exiger(emettre(&invalide) == 1 && invalide.Resultat.Erreur == 1,
                   "tampon d'émission nul avec capacité accepté");
            *capacite = 0;
        }
        noeuds[1].Parent = 1;
        Exiger(emettre(&invalide) == 2 && invalide.Resultat.Erreur == 2,
               "AST invalide accepté pendant l'émission");
    }

    std::string TraduireCorpusConversions(std::string texte)
    {
        for (const auto& [fr, en] : std::vector<std::pair<std::string, std::string>>{
                 {"pointeur_fonction", "function_pointer"}, {"convertir", "cast"},
                 {"constante", "const"}, {"énumération", "enumeration"},
                 {"structure", "struct"}, {"classe", "class"}, {"espace", "namespace"},
                 {"constructeur", "constructor"}, {"opérateur", "operator"},
                 {"destructeur", "destructor"}, {"virtuel", "virtual"}, {"remplacer", "override"},
                 {"externe", "extern"},
                 {"soi", "this"}, {"caractère", "char"}, {"octet", "byte"},
                 {"publique", "public"}, {"retourner", "return"},
                 {"privée", "private"}, {"protégée", "protected"},
                 {"naturel", "uint"}, {"entier", "int"}, {"booléen", "bool"},
                 {"vide", "void"}, {"vrai", "true"}, {"faux", "false"}})
        {
            std::size_t position = 0;
            while ((position = texte.find(fr, position)) != std::string::npos)
            {
                texte.replace(position, fr.size(), en);
                position += en.size();
            }
        }
        return texte;
    }

    void TesterConversionsSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe,
        AnalyseurSemantiqueAutoHeberge semantique)
    {
        const auto traduire = TraduireCorpusConversions;
        const std::string valide = R"(
énumération Etat { Limite = 127 };
publique entier32 Cible(entier32 valeur) { retourner valeur; }
publique constante entier32* Vue(entier32* valeur) {
    retourner convertir<constante entier32*>(convertir<vide*>(valeur));
}
publique entier32 Conversions(entier32 valeur, entier32* adresse,
    pointeur_fonction<entier32(entier32)> rappel) {
    entier8 etroit = convertir<entier8>(convertir<entier32>(127));
    entier8 dynamique = convertir<entier8>(valeur);
    booléen logique = convertir<booléen>(-3);
    Etat etat = convertir<Etat>(1);
    entier32 valeurEnum = convertir<entier32>(Etat::Limite);
    entier32 reference = convertir<entier32&>(valeur);
    constante entier32* lecture = convertir<constante entier32*>(convertir<vide*>(adresse));
    volatile entier32* materiel = convertir<volatile entier32*>(adresse);
    entier32 copie = *convertir<entier32*>(convertir<vide*>(adresse));
    pointeur_fonction<entier32(entier32)> fonction = convertir<pointeur_fonction<entier32(entier32)>>(rappel);
    pointeur_fonction<entier32(entier32)>* caseRappel = convertir<pointeur_fonction<entier32(entier32)>*>(&rappel);
    entier32 appel = (convertir<pointeur_fonction<entier32(entier32)>>(rappel))(valeur);
    entier32 direct = (convertir<pointeur_fonction<entier32(entier32)>>(Cible))(valeur);
    retourner appel + direct + copie + valeurEnum + reference;
}
entier64 Minimum = convertir<entier64>(-9223372036854775808);
naturel64 Maximum = convertir<naturel64>(18446744073709551615);
)";
        for (const auto& texte : {valide, traduire(valide)})
        {
            auto programme = GsPP::AnalyseurSyntaxique(
                GsPP::Lexeur(texte, "conversions-valides").Analyser(), "conversions-valides").Analyser();
            GsPP::AnalyseurSemantique().Analyser(programme);
            AnalyserSemantiqueValide(syntaxe, semantique, texte, "conversions-valides");
        }
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"publique vide F() { convertir<vide>(1); }", 94},
            {"structure S {}; publique vide F() { convertir<S>(1); }", 94},
            {"union U { entier32 V; }; publique vide F() { convertir<U>(1); }", 94},
            {"classe C {}; publique vide F() { convertir<C>(1); }", 94},
            {"publique vide F() { convertir<Absent*>(1); }", 99},
            {"structure S {}; publique vide F() { S valeur = {}; convertir<entier32>(valeur); }", 95},
            {"publique vide F() { entier32 valeurs[2] = {}; convertir<entier32>(valeurs); }", 95},
            {"publique vide V() {} publique vide F() { convertir<entier32>(V()); }", 95},
            {"publique vide F() { convertir<entier32*>(1); }", 96},
            {"publique vide F(entier32* p) { convertir<entier64>(p); }", 96},
            {"publique vide Cible() {} publique vide F() { convertir<entier64>(Cible); }", 96},
            {"publique vide F() { convertir<pointeur_fonction<vide()>>(1); }", 96},
            {"publique entier32 Cible() { retourner 1; } publique vide F() { convertir<pointeur_fonction<vide()>>(Cible); }", 97},
            {"publique vide Cible(entier32 v) {} publique vide F() { convertir<pointeur_fonction<vide(entier64)>>(Cible); }", 97},
            {"publique vide Cible() {} publique vide F() { convertir<vide*>(Cible); }", 97},
            {"publique vide F(vide* p) { convertir<pointeur_fonction<vide()>>(p); }", 97},
            {"publique vide Cible() {} publique vide F() { convertir<constante pointeur_fonction<vide()>>(Cible); }", 97},
            {"entier8 Globale = convertir<entier8>(128); publique vide F() {}", 98},
            {"publique vide F() { convertir<naturel64>(-1); }", 98},
            {"structure S { naturel8 V; }; S Valeur = {convertir<naturel8>(256)}; publique vide F() {}", 98},
            {"publique vide F() { convertir<entier64>(convertir<entier8>(128)); }", 98},
            {"énumération E { X = 128 }; publique vide F() { convertir<entier8>(E::X); }", 98},
            {"publique vide F() { faux && (convertir<entier8>(128) == 0); }", 98},
            {"publique vide F() { convertir<entier32>(1 / 0); }", 89},
            {"énumération E { X = convertir<entier8>(128) }; publique vide F() {}", 98},
            {"publique vide F(entier32 valeur) { entier32& r = convertir<entier32&>(valeur); }", 69},
            {"publique vide F(entier32* p) { entier32* q = convertir<constante entier32*>(p); }", 45},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
        {
            const auto& [texte, code] = refus[index];
            ComparerErreurSemantique(syntaxe, semantique, texte, code,
                "conversion-refusee-fr-" + std::to_string(index));
            ComparerErreurSemantique(syntaxe, semantique, traduire(texte), code,
                "conversion-refusee-en-" + std::to_string(index));
        }
    }

    void TesterConversionsImplicitesSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe,
        AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::vector<std::string> valides{
            "publique entier8 Prendre(entier8 v) { retourner v; } "
            "publique entier8 Prendre(booléen v) { retourner 0; } "
            "publique entier8 F() { retourner Prendre((20 + 1) * 2); }",
            "publique entier8 Prendre(constante entier8 v) { retourner v; } "
            "publique entier8 F() { retourner Prendre(42); }",
            "publique entier8 F(entier8 v) { retourner v + (20 + 1); }",
            "publique entier8 F(entier8 v) { retourner (20 + 1) + v; }",
            "classe C { publique: constructeur(entier8 v) {} "
            "entier8 Lire(entier8 v) { retourner v; } }; "
            "publique entier8 F() { C c((20 + 1) * 2); retourner c.Lire(40 + 2); }",
            "classe C { publique: entier8 opérateur+(entier8 v) { retourner v; } }; "
            "publique entier8 F() { C c; retourner c + (40 + 2); }",
            "publique entier8 F(pointeur_fonction<entier8(entier8)> rappel) { "
            "entier8 v = 40 + 2; v = 39 + 3; retourner rappel(38 + 4); }",
            "publique entier8 F(constante entier8& v, volatile entier8& w) { "
            "entier8 copie = v; copie = w; retourner copie; }",
            "classe Base {}; classe Derivee : publique Base {}; "
            "publique constante Base* F(constante Derivee* p) { retourner p; } "
            "publique vide R(constante Derivee& v) { constante Base& r = v; }",
            "publique entier8 Prendre(entier8 v) { retourner v; } "
            "publique entier8 F() { retourner Prendre(convertir<entier16>(40) + 2); }",
            "énumération E { X = 40, Y = convertir<entier32>(E::X) + 2 }; "
            "publique entier8 Prendre(entier8 v) { retourner v; } "
            "publique entier8 Prendre(booléen v) { retourner 0; } "
            "publique entier8 F() { retourner Prendre(convertir<entier32>(E::Y)); }",
            "publique entier8 Prendre(entier8 v) { retourner v; } "
            "publique entier8 Prendre(booléen v) { retourner 0; } "
            "publique entier8 F() { retourner Prendre(~(-43)); }",
            "publique naturel8 Prendre(naturel8 v) { retourner v; } "
            "publique naturel8 Prendre(booléen v) { retourner 0; } "
            "publique naturel8 F() { retourner Prendre(-0); }",
            "publique vide R(constante entier32* p, volatile entier32* q) { "
            "constante entier32*& r = p; volatile entier32*& s = q; }",
            "classe Base {}; classe Derivee : publique Base {}; "
            "publique vide Prendre(constante Base& v) {} "
            "publique vide Prendre(booléen v) {} "
            "publique vide F(constante Derivee& v) { Prendre(v); }",
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
        {
            for (const auto& texte : {valides[index], TraduireCorpusConversions(valides[index])})
                AnalyserSemantiqueValide(syntaxe, semantique, texte,
                    "implicite-valide-" + std::to_string(index));
        }
        const std::vector<std::pair<std::string, GsPP::GenreType>> selections{
            {"publique vide Choisir(entier8 v) {} publique vide Choisir(entier32 v) {} "
             "publique vide F() { Choisir(40 + 2); }", GsPP::GenreType::Entier32},
            {"publique vide Choisir(entier32 v) {} publique vide Choisir(entier8 v) {} "
             "publique vide F() { Choisir(40 + 2); }", GsPP::GenreType::Entier32},
            {"publique vide Choisir(entier8 v) {} publique vide Choisir(entier16 v) {} "
             "publique vide F() { Choisir(127 + 1); }", GsPP::GenreType::Entier16},
        };
        for (const auto& [source, genre] : selections)
        {
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
            {
                const auto resultat = AnalyserSemantiqueValide(
                    syntaxe, semantique, texte, "implicite-selection");
                std::size_t nombre = 0;
                for (const auto& resolution : resultat.Resolutions)
                {
                    if (resultat.Noeuds[resolution.IndexNoeud].HachageNom != HacherTexte("Choisir"))
                        continue;
                    const auto fonction = resultat.Symboles[resolution.IndexSymbole].IndexNoeud;
                    const auto parametre = std::find_if(resultat.Noeuds.begin(), resultat.Noeuds.end(),
                        [&](const auto& noeud) { return noeud.Genre == 2 && noeud.Parent == fonction; });
                    Exiger(parametre != resultat.Noeuds.end()
                               && parametre->HachageType == HacherTypeDeclaration(GsPP::TypeGs(genre)),
                           "mauvaise surcharge après adaptation d'une constante composée");
                    ++nombre;
                }
                Exiger(nombre == 1, "résolution de surcharge absente ou dupliquée");
            }
        }
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"publique vide Prendre(entier8 v) {} publique vide Prendre(booléen v) {} "
             "publique vide F() { Prendre(120 + 8); }", 21},
            {"publique vide Prendre(entier8 v) {} publique vide Prendre(booléen v) {} "
             "publique vide F(entier32 v) { Prendre(v); }", 21},
            {"publique vide Prendre(entier8 v) {} publique vide Prendre(entier16 v) {} "
             "publique vide F() { Prendre(40 + 2); }", 22},
            {"publique vide Prendre(entier8& v) {} publique vide Prendre(booléen v) {} "
             "publique vide F() { Prendre(40 + 2); }", 21},
            {"publique vide Prendre(entier32* p) {} "
             "publique vide Prendre(booléen v) {} "
             "publique vide F(constante entier32* p) { Prendre(p); }", 21},
            {"publique vide Prendre(entier32* p) {} "
             "publique vide Prendre(booléen v) {} "
             "publique vide F(volatile entier32* p) { Prendre(p); }", 21},
            {"publique vide F(constante entier32* p) { entier32* q = p; }", 45},
            {"publique vide F(volatile entier32* p) { entier32* q = p; }", 45},
            {"publique vide F(constante entier32& v) { entier32& r = v; }", 69},
            {"classe Base {}; classe Derivee : publique Base {}; "
             "publique Base* F(constante Derivee* p) { retourner p; }", 75},
            {"publique vide Prendre(caractère v) {} publique vide Prendre(booléen v) {} "
             "publique vide F() { Prendre(128); }", 21},
            {"publique vide Prendre(naturel8 v) {} publique vide Prendre(booléen v) {} "
             "publique vide F() { Prendre(0 - 1); }", 21},
            {"publique entier8 F(entier8 v) { retourner v + (120 + 8); }", 90},
            {"publique vide Prendre(entier8 v) {} publique vide Prendre(booléen v) {} "
             "publique vide F() { Prendre(1 / 0); }", 89},
            {"publique entier8 F(entier8 v) { retourner (1 / 0) + v; }", 89},
            {"publique entier8 F(entier8 v) { retourner (120 + 8) + v; }", 90},
            {"publique vide F(entier32* p) { constante entier32* q = p; }", 45},
            {"publique vide F(entier32* p) { volatile entier32* q = p; }", 45},
            {"publique vide F(constante entier32* p) { entier32*& r = p; }", 69},
            {"classe Base {}; classe Derivee : publique Base {}; "
             "publique vide F(constante Derivee& v) { Base& r = v; }", 69},
            {"publique vide Prendre(entier8 v) {} publique vide Prendre(booléen v) {} "
             "publique vide F() { Prendre(convertir<entier8>(128)); }", 98},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
        {
            const auto& [texte, code] = refus[index];
            ComparerErreurSemantique(syntaxe, semantique, texte, code,
                "implicite-refuse-fr-" + std::to_string(index));
            ComparerErreurSemantique(syntaxe, semantique, TraduireCorpusConversions(texte), code,
                "implicite-refuse-en-" + std::to_string(index));
        }
    }

    void TesterTypesComposesSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe,
        AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::vector<std::string> valides{
            "publique entier32 F(pointeur_fonction<entier32(entier32)>& rappel) { "
            "pointeur_fonction<entier32(entier32)>& liaison = rappel; retourner liaison(42); }",
            "publique vide Prendre(pointeur_fonction<vide()>& rappel) {} "
            "publique vide Prendre(booléen v) {} "
            "publique vide F(pointeur_fonction<vide()> rappel) { Prendre(rappel); }",
            "publique vide F(pointeur_fonction<vide(pointeur_fonction<vide()>&)> prendre, "
            "pointeur_fonction<vide()> rappel) { prendre(rappel); }",
            "publique entier32 F(pointeur_fonction<entier32(entier32)>& rappel) { "
            "pointeur_fonction<entier32(entier32)>* adresse = &rappel; retourner (*adresse)(42); }",
            "publique entier32 F(pointeur_fonction<entier32(entier32)>* adresse) { "
            "pointeur_fonction<entier32(entier32)>& liaison = *adresse; retourner liaison(42); }",
            "publique entier32 F(pointeur_fonction<pointeur_fonction<entier32(entier32)>()> fabrique) { "
            "retourner fabrique()(42); }",
            "publique entier32 Cible(entier32 v) { retourner v; } "
            "publique entier32 F() { pointeur_fonction<entier32(entier32)> rappels[2] = {Cible, Cible}; "
            "pointeur_fonction<entier32(entier32)>& liaison = rappels[0]; retourner liaison(42); }",
            "publique entier32 F(entier32*** adresse) { entier32***& liaison = adresse; "
            "entier32*** copie = liaison; retourner ***copie; }",
            "structure S { entier32 V; }; publique vide F(S*** adresse) { S***& liaison = adresse; }",
            "publique vide F(constante pointeur_fonction<vide()> rappel) { "
            "constante pointeur_fonction<vide()>& liaison = rappel; liaison(); }",
            "publique vide F(volatile pointeur_fonction<vide()> rappel) { "
            "volatile pointeur_fonction<vide()>& liaison = rappel; liaison(); }",
            "publique vide F(pointeur_fonction<vide()>* adresse) { "
            "vide* brut = convertir<vide*>(adresse); "
            "pointeur_fonction<vide()>* retour = convertir<pointeur_fonction<vide()>*>(brut); "
            "(*retour)(); }",
            "publique vide F(pointeur_fonction<pointeur_fonction<vide()>&()> obtenir) { "
            "pointeur_fonction<vide()>& liaison = obtenir(); liaison(); }",
            "publique entier32 F(entier32***** adresse) { entier32*****& liaison = adresse; "
            "entier32***** tableau[2][2] = {{adresse}, {liaison}}; retourner *****tableau[0][0]; }",
            "structure S { entier32 V; }; publique vide F(S***** adresse) { "
            "S***** tableau[2][2] = {{adresse}, {adresse}}; S*****& liaison = tableau[0][0]; }",
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
        {
            for (const auto& texte : {valides[index], TraduireCorpusConversions(valides[index])})
            {
                const auto nom = "compose-valide-" + std::to_string(index);
                try
                {
                    auto programme = GsPP::AnalyseurSyntaxique(
                        GsPP::Lexeur(texte, nom).Analyser(), nom).Analyser();
                    GsPP::AnalyseurSemantique().Analyser(programme);
                }
                catch (const std::exception& erreur)
                {
                    throw std::runtime_error(nom + " : " + erreur.what());
                }
                const auto resultat = AnalyserSemantiqueValide(syntaxe, semantique, texte, nom);
                if (index == 1)
                {
                    std::size_t nombre = 0;
                    for (const auto& resolution : resultat.Resolutions)
                    {
                        if (resultat.Noeuds[resolution.IndexNoeud].HachageNom != HacherTexte("Prendre"))
                            continue;
                        const auto fonction = resultat.Symboles[resolution.IndexSymbole].IndexNoeud;
                        const auto parametre = std::find_if(resultat.Noeuds.begin(), resultat.Noeuds.end(),
                            [&](const auto& noeud) { return noeud.Genre == 2 && noeud.Parent == fonction; });
                        Exiger(parametre != resultat.Noeuds.end()
                                   && parametre->HachageNom == HacherTexte("rappel"),
                            "la surcharge avec référence de callback n'a pas été sélectionnée");
                        ++nombre;
                    }
                    Exiger(nombre == 1, "résolution de Prendre absente ou dupliquée");
                }
            }
        }
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"publique vide Cible() {} publique vide F() { pointeur_fonction<vide()>& r = Cible; }", 69},
            {"publique vide F(pointeur_fonction<vide()> r) { "
             "pointeur_fonction<vide()>& liaison = convertir<pointeur_fonction<vide()>>(r); }", 69},
            {"publique vide F(pointeur_fonction<vide(entier32)> r) { pointeur_fonction<vide()>& liaison = r; }", 69},
            {"publique vide F(constante pointeur_fonction<vide()> r) { pointeur_fonction<vide()>& liaison = r; }", 69},
            {"publique vide F(volatile pointeur_fonction<vide()> r) { pointeur_fonction<vide()>& liaison = r; }", 69},
            {"publique vide F(pointeur_fonction<vide()> r) { constante pointeur_fonction<vide()>& liaison = r; }", 69},
            {"publique vide F(constante entier32*** p) { entier32*** q = p; }", 45},
            {"publique vide F(volatile entier32*** p) { entier32***& q = p; }", 69},
            {"publique vide F(entier32*** p) { convertir<entier64>(p); }", 96},
            {"publique vide F(pointeur_fonction<entier32(entier32)> p) { "
             "pointeur_fonction<entier32(entier64)> q = p; }", 45},
            {"publique vide F(pointeur_fonction<vide(entier32&)> p) { "
             "pointeur_fonction<vide(constante entier32&)> q = p; }", 45},
            {"publique vide F(pointeur_fonction<pointeur_fonction<entier32()>()> p) { "
             "pointeur_fonction<pointeur_fonction<vide()>()> q = p; }", 45},
            {"publique vide F(pointeur_fonction<pointeur_fonction<vide()>()> obtenir) { "
             "pointeur_fonction<vide()>& liaison = obtenir(); }", 69},
            {"publique vide Cible() {} publique vide Prendre(pointeur_fonction<vide()>& p) {} "
             "publique vide Prendre(booléen v) {} publique vide F() { Prendre(Cible); }", 21},
            {"publique vide Cible() {} "
             "publique vide F(pointeur_fonction<vide(pointeur_fonction<vide()>&)> prendre) { prendre(Cible); }", 55},
            {"publique vide F(pointeur_fonction<vide(entier32*)> p) { "
             "pointeur_fonction<vide(constante entier32*)> q = p; }", 45},
            {"publique vide F(constante entier32***** p) { entier32***** tableau[1] = {p}; }", 45},
            {"publique vide F(pointeur_fonction<vide()> p) { convertir<vide*>(p); }", 97},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
        {
            const auto& [texte, code] = refus[index];
            ComparerErreurSemantique(syntaxe, semantique, texte, code,
                "compose-refuse-fr-" + std::to_string(index));
            ComparerErreurSemantique(syntaxe, semantique, TraduireCorpusConversions(texte), code,
                "compose-refuse-en-" + std::to_string(index));
        }
    }

    void TesterTypesNommesImbriquesSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe,
        AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::vector<std::string> valides{
            "espace N { structure S {}; publique vide F(pointeur_fonction<vide(S*)> p, N::S* s) { p(s); } }",
            "espace N { structure S {}; publique vide F(pointeur_fonction<vide(S*)> p) { "
            "pointeur_fonction<vide(N::S*)> q = p; } }",
            "espace N { structure S {}; publique vide F(pointeur_fonction<pointeur_fonction<S*()>()> p) { "
            "N::S* s = p()(); } }",
            "espace N { énumération E { X }; publique vide F(pointeur_fonction<vide(E)> p) { p(N::E::X); } }",
            "espace N { structure S {}; publique vide F(pointeur_fonction<vide(S&)>& p, N::S& s) { "
            "pointeur_fonction<vide(N::S&)>& liaison = p; liaison(s); } }",
            "espace N { structure S {}; publique vide F(pointeur_fonction<vide(constante S*)> p, constante N::S* s) { "
            "convertir<pointeur_fonction<vide(constante N::S*)>>(p)(s); } }",
            "espace N { structure S { entier32 V; }; publique entier32 F(pointeur_fonction<S*()> p) { "
            "retourner p()->V; } }",
            "espace A { structure S {}; publique vide F(pointeur_fonction<vide(S*)> p, A::S* s) { p(s); } } "
            "espace B { structure S {}; publique vide F(pointeur_fonction<vide(S*)> p, B::S* s) { p(s); } }",
            "structure S {}; espace N { structure S {}; publique vide F(pointeur_fonction<vide(S*)> p) { "
            "pointeur_fonction<vide(S*)> q = p; } } "
            "publique vide G(pointeur_fonction<vide(S*)> p, S* s) { N::F(p); p(s); }",
            "espace N { espace Sous { structure S {}; } publique vide F(pointeur_fonction<vide(Sous::S*)> p) { "
            "pointeur_fonction<vide(N::Sous::S*)> q = p; } }",
            "espace N { structure S {}; structure R { pointeur_fonction<vide(S*)> Appeler; }; "
            "publique vide F(R* r, N::S* s) { r->Appeler(s); } }",
            "espace N { énumération E { X }; publique E Cible(E v) { retourner v; } "
            "pointeur_fonction<N::E(N::E)> Rappel = Cible; }",
            "espace N { structure S {}; publique S***** F(N::S***** p) { "
            "vide* brut = convertir<vide*>(p); retourner convertir<S*****>(brut); } }",
            "espace N { énumération E { X }; publique entier32 F(pointeur_fonction<pointeur_fonction<E()>()> p) { "
            "retourner convertir<entier32>(p()()); } }",
            "espace N { structure S {}; publique vide Prendre(pointeur_fonction<vide(S*)> p) {} "
            "publique vide Prendre(booléen v) {} publique vide F(pointeur_fonction<vide(N::S*)> p) { Prendre(p); } }",
            "espace N { classe C { publique: entier32 Appeler(pointeur_fonction<entier32(N::C*)> p) { "
            "retourner p(&soi); } }; }",
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
            for (const auto& texte : {valides[index], TraduireCorpusConversions(valides[index])})
            {
                auto programme = GsPP::AnalyseurSyntaxique(
                    GsPP::Lexeur(texte, "noms-bootstrap").Analyser(), "noms-bootstrap").Analyser();
                GsPP::AnalyseurSemantique().Analyser(programme);
                const auto resultat = AnalyserSemantiqueValide(syntaxe, semantique, texte,
                    "nom-imbrique-valide-" + std::to_string(index));
                if (index == 1 || index == 7)
                {
                    std::vector<std::uint64_t> types;
                    for (const auto& symbole : resultat.Symboles)
                        if (symbole.HachageNom == HacherTexte("p")
                            || (index == 1 && symbole.HachageNom == HacherTexte("q")))
                            types.push_back(symbole.HachageType);
                    Exiger(types.size() == 2 && ((types[0] == types[1]) == (index == 1)),
                        "identités canoniques incorrectes pour les signatures nommées");
                }
            }

        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"espace A { structure S {}; publique pointeur_fonction<vide(S*)> Fabrique() {} } "
             "espace B { structure S {}; publique vide F(S* s) { A::Fabrique()(s); } }", 55},
            {"espace N { structure S {}; publique vide F(pointeur_fonction<vide(S*)> p, constante S* s) { p(s); } }", 55},
            {"espace N { structure S {}; publique vide F(pointeur_fonction<vide(S*)> p) { "
             "pointeur_fonction<vide(constante N::S*)> q = p; } }", 45},
            {"publique vide F(pointeur_fonction<vide(Absent*)> p) {}", 100},
            {"publique vide F(pointeur_fonction<pointeur_fonction<Absent*()>()> p) {}", 100},
            {"espace N { structure S {}; publique vide F(pointeur_fonction<vide(S*)> p) { "
             "convertir<pointeur_fonction<vide(Absent*)>>(p); } }", 99},
            {"espace A { structure S {}; } espace B { publique vide F(pointeur_fonction<vide(S*)> p) {} }", 100},
            {"espace A { structure S {}; } espace B { structure S {}; "
             "publique vide F(pointeur_fonction<vide(S*)> p) { pointeur_fonction<vide(A::S*)> q = p; } }", 45},
            {"espace A { structure S {}; } espace B { structure S {}; "
             "publique vide F(pointeur_fonction<vide(S*)> p) { convertir<pointeur_fonction<vide(A::S*)>>(p); } }", 97},
            {"espace A { structure S {}; } espace B { structure S {}; "
             "publique vide F(pointeur_fonction<vide(pointeur_fonction<vide(S*)>)> prendre, "
             "pointeur_fonction<vide(A::S*)> p) { prendre(p); } }", 55},
            {"espace N { structure S {}; publique vide F(pointeur_fonction<vide(S*)> p) { "
             "pointeur_fonction<vide(constante N::S*)>& liaison = p; } }", 69},
            {"espace A { structure S {}; } espace B { structure S {}; "
             "publique vide F(pointeur_fonction<vide(S*)> p) { pointeur_fonction<vide(A::S*)>& liaison = p; } }", 69},
            {"espace N { structure S {}; espace Sous { publique vide F(pointeur_fonction<vide(S*)> p) {} } }", 100},
            {"structure S {}; espace N { structure S {}; publique vide F(pointeur_fonction<vide(S*)> p, N::S* s) { p(s); } }", 55},
            {"espace N { structure S {}; publique vide F(pointeur_fonction<vide(S*)> p, volatile N::S* s) { p(s); } }", 55},
            {"espace N { énumération E { X }; structure S {}; publique vide F(pointeur_fonction<vide(E)> p, S s) { p(s); } }", 55},
            {"espace N { structure S {}; structure R { pointeur_fonction<vide(S*)> Appeler; }; "
             "publique vide F(R* r) { r->Appeler(); } }", 54},
            {"espace N { structure S {}; structure R { pointeur_fonction<vide(S*)> Appeler; }; "
             "publique vide F(R* r, constante S* s) { r->Appeler(s); } }", 55},
            {"espace N { structure R { entier32 Valeur; }; publique vide F(R* r) { r->Valeur(); } }", 53},
            {"espace N { structure S {}; classe R { privée: pointeur_fonction<vide(S*)> Appeler; }; "
             "publique vide F(R* r, S* s) { r->Appeler(s); } }", 25},
            {"espace N { classe C { publique: vide F(pointeur_fonction<vide(C*)> p) {} }; }", 100},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
        {
            const auto& [texte, code] = refus[index];
            ComparerErreurSemantique(syntaxe, semantique, texte, code,
                "nom-imbrique-refuse-fr-" + std::to_string(index));
            ComparerErreurSemantique(syntaxe, semantique, TraduireCorpusConversions(texte), code,
                "nom-imbrique-refuse-en-" + std::to_string(index));
        }
    }

    void TesterContraintesSignaturesSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe,
        AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::vector<std::string> valides{
            "publique vide F(pointeur_fonction<vide(entier32, entier32, entier32, entier32)> p) {}",
            "structure S { entier32 V; }; publique vide F(pointeur_fonction<S(entier32, entier32, entier32)> p) {}",
            "structure S {}; publique vide F(pointeur_fonction<S*(entier32, entier32, entier32, entier32)> p) {}",
            "structure S {}; publique vide F(pointeur_fonction<S&(entier32, entier32, entier32, entier32)> p) {}",
            "énumération E { X }; publique vide F(pointeur_fonction<E(entier32, entier32, entier32, entier32)> p) {}",
            "publique vide F(pointeur_fonction<vide(vide*, vide*&, vide&, constante entier32&)> p) {}",
            "publique vide F(pointeur_fonction<pointeur_fonction<vide(entier32, entier32, entier32, entier32)>()> p) {}",
            "publique vide F(entier32 a, entier32 b, entier32 c, entier32 d) {}",
            "structure S { entier32 V; }; publique S F(entier32 a, entier32 b, entier32 c) { retourner {a}; }",
            "énumération E { X }; publique E F(entier32 a, entier32 b, entier32 c, entier32 d) { retourner E::X; }",
            "structure S {}; publique S* F(entier32 a, entier32 b, entier32 c, entier32 d) {}",
            "classe C { publique: vide F(entier32 a, entier32 b, entier32 c) {} };",
            "structure S { entier32 V; }; classe C { publique: S F(entier32 a, entier32 b) { retourner {a}; } };",
            "classe C { publique: constructeur(entier32 a, entier32 b, entier32 c) {} };",
            "externe vide F(vide* p, constante entier32& v);",
            "espace N { structure S { entier32 V; }; publique S opérateur+(S a, S b) { retourner {a.V + b.V}; } "
            "publique N::S F(S a, N::S b) { retourner a + b; } }",
            "structure S { entier32 V; }; union U { entier32 V; }; "
            "publique vide F(pointeur_fonction<U(entier32, entier32, entier32)> p) {}",
            "publique vide F(pointeur_fonction<vide(pointeur_fonction<vide()>)> p) {}",
            "union U { entier32 V; }; publique entier32 opérateur+(U a, U b) { retourner a.V + b.V; } "
            "publique entier32 F(U a, U b) { retourner a + b; }",
            "structure S { entier32 V; }; publique entier32 opérateur+(entier32 a, constante S& b) { retourner a + b.V; } "
            "publique entier32 F(S b) { retourner 21 + b; }",
            "structure S { entier32 V; }; publique booléen opérateur!(constante S& a) { retourner a.V == 0; } "
            "publique booléen F(S a) { retourner !a; }",
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
            for (const auto& texte : {valides[index], TraduireCorpusConversions(valides[index])})
            {
                const auto nom = "signature-valide-" + std::to_string(index);
                try
                {
                    auto programme = GsPP::AnalyseurSyntaxique(
                        GsPP::Lexeur(texte, nom).Analyser(), nom).Analyser();
                    GsPP::AnalyseurSemantique().Analyser(programme);
                }
                catch (const std::exception& erreur)
                {
                    throw std::runtime_error(nom + " : " + erreur.what());
                }
                AnalyserSemantiqueValide(syntaxe, semantique, texte, nom);
            }
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"publique vide F(pointeur_fonction<vide(entier32, entier32, entier32, entier32, entier32)> p) {}", 103},
            {"publique vide F(pointeur_fonction<vide(vide)> p) {}", 102},
            {"structure S {}; publique vide F(pointeur_fonction<S(entier32, entier32, entier32, entier32)> p) {}", 103},
            {"publique entier32& F() {}", 104},
            {"publique vide F(vide v) {}", 105},
            {"publique vide F(entier32 v[2]) {}", 105},
            {"publique vide F(entier32 a, entier32 b, entier32 c, entier32 d, entier32 e) {}", 106},
            {"structure S {}; publique S F(entier32 a, entier32 b, entier32 c, entier32 d) {}", 106},
            {"publique vide F(entier32& v[2]) {}", 101},
            {"publique vide F(vide& v) {}", 105},
            {"publique vide F(pointeur_fonction<vide(pointeur_fonction<vide(vide)>)> p) {}", 102},
            {"publique vide F(pointeur_fonction<pointeur_fonction<vide(vide)>()> p) {}", 102},
            {"publique vide F(pointeur_fonction<vide(vide, entier32, entier32, entier32, entier32)> p) {}", 103},
            {"publique vide F(pointeur_fonction<vide(vide, Absent*)> p) {}", 100},
            {"publique vide F(pointeur_fonction<pointeur_fonction<vide(vide)>(Absent*)> p) {}", 102},
            {"publique vide F(pointeur_fonction<vide()> p) { convertir<pointeur_fonction<vide(vide)>>(p); }", 102},
            {"pointeur_fonction<vide(entier32, entier32, entier32, entier32, entier32)> Rappel; publique vide F() {}", 103},
            {"structure R { pointeur_fonction<vide(vide)> Rappel; }; publique vide F() {}", 102},
            {"structure S {}; classe C { publique: S F(entier32 a, entier32 b, entier32 c) {} };", 106},
            {"classe C { publique: vide F(entier32 a, entier32 b, entier32 c, entier32 d) {} };", 106},
            {"classe C { publique: constructeur(entier32 a, entier32 b, entier32 c, entier32 d) {} };", 106},
            {"classe C { publique: entier32& F() {} };", 104},
            {"externe entier32& F();", 104},
            {"structure S {}; publique S& opérateur+(S a, S b) {}", 104},
            {"structure R { entier32& Valeurs[2]; }; publique vide F() {}", 101},
            {"externe entier32& Valeurs[2]; publique vide F() {}", 101},
            {"publique vide F() { entier32& valeurs[2]; }", 101},
            {"union U { entier32 V; }; publique vide F(pointeur_fonction<U(entier32, entier32, entier32, entier32)> p) {}", 103},
            {"espace N { structure S {}; publique vide F(pointeur_fonction<N::S(entier32, entier32, entier32, entier32)> p) {} }", 103},
            {"publique vide F(pointeur_fonction<vide(vide)> p, entier32 a, entier32 b, entier32 c, entier32 d) {}", 102},
            {"structure S {}; publique entier32 F(S a) { retourner a + 1; }", 28},
            {"structure S {}; publique entier32 opérateur+(S a, booléen b) { retourner 0; } "
             "publique entier32 F(S a) { retourner a + 1; }", 21},
            {"union U { entier32 V; }; publique booléen F(U a) { retourner !a; }", 28},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
        {
            const auto& [texte, code] = refus[index];
            ComparerErreurSemantique(syntaxe, semantique, texte, code,
                "signature-refuse-fr-" + std::to_string(index));
            ComparerErreurSemantique(syntaxe, semantique, TraduireCorpusConversions(texte), code,
                "signature-refuse-en-" + std::to_string(index));
        }
    }

    void TesterAliasesRacinesSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe,
        AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::vector<std::string> valides{
            "alias Vue = S; structure S { entier32 X; }; publique vide F() {}",
            "alias Vue = Copie; alias Copie = S; structure S { entier32 X; }; "
            "publique entier32 F(Vue* p) { retourner p->X; }",
            "alias Vue = U; union U { entier32 X; entier64 Y; }; "
            "publique entier32 F(Vue* p) { retourner p->X; }",
            "alias Vue = C; classe C { publique: entier32 X; }; "
            "publique entier32 F(Vue* p) { retourner p->X; }",
            "espace A { alias Vue = Copie; alias Copie = S; structure S { entier32 X; }; } "
            "espace B { alias Vue = A::Vue; publique entier32 F(Vue* p) { retourner p->X; } }",
            "alias API::Vue = A::S; espace A { structure S { entier32 X; }; } "
            "publique entier32 F(API::Vue* p) { retourner p->X; }",
            "espace A { alias API :: /* nom */ Vue = B :: /* cible */ S; } "
            "espace A::B { structure S { entier32 X; }; } "
            "publique entier32 F(A::API::Vue* p) { retourner p->X; }",
            "espace A { structure S { entier64 Y; }; alias Vue = S; } "
            "structure S { entier32 X; }; publique entier32 F(A::Vue* p) { retourner p->X; }",
            "alias Vue = S; structure S { entier32 X; }; "
            "publique entier32 F(Vue& p) { retourner p.X; }",
            "alias Vue = S; structure S { entier32 X; }; structure T { Vue Liste[2]; }; "
            "publique entier32 F(T* p) { retourner p->Liste[1].X; }",
            "alias Vue = S; structure S { entier32 X; }; "
            "publique entier32 F(vide* p) { retourner convertir<Vue*>(p)->X; }",
            "alias Vue = S; structure S { entier32 X; }; "
            "publique entier32 F(pointeur_fonction<entier32(Vue*)> rappel, S* p) { retourner rappel(p); }",
            "alias Vue = Base; classe Base { publique: entier32 X; }; "
            "classe Derivee : publique Vue { publique: entier32 Lire() { retourner soi.X; } }; "
            "publique entier32 F(Derivee* p) { retourner p->X; }",
            "alias Appeler = Copie; alias Copie = Cible; "
            "publique entier32 Cible(entier32 x) { retourner x; } "
            "publique entier32 F() { retourner Appeler(42); }",
            "espace A { alias Appeler = Cible; publique entier32 Cible(entier32 x) { retourner x; } } "
            "alias API::Appeler = A::Appeler; publique entier32 F() { retourner API::Appeler(42); }",
            "alias Appeler = Cible; publique entier32 Cible(entier32 x) { retourner x; } "
            "publique entier32 F() { pointeur_fonction<entier32(entier32)> rappel = Appeler; retourner rappel(42); }",
            "alias Vue = Copie; alias Copie = X; entier32 X = 42; "
            "publique entier32 F() { Vue = Vue + 1; retourner Copie; }",
            "espace A { alias Vue = X; entier32 X = 42; } "
            "alias API::Vue = A::Vue; publique entier32 F() { API::Vue = 7; retourner API::Vue; }",
            "alias Vue = X; constante entier32 X = 42; publique entier32 F() { retourner Vue; }",
            "alias Vue = X; entier32 X[2] = {1, 2}; publique entier32 F() { Vue[0] = 7; retourner Vue[1]; }",
            "alias Appeler = Rappel; pointeur_fonction<entier32(entier32)> Rappel; "
            "publique entier32 F() { retourner Appeler(42); }",
            "alias Vue = X; entier32 X; publique entier32 F() { entier32* p = &Vue; retourner *p; }",
            "alias Vue = S; alias Objet = X; structure S { entier32 Y; }; Vue X = {42}; "
            "publique entier32 F() { retourner Objet.Y; }",
            "espace A { entier64 X; alias Vue = X; } entier32 X; "
            "publique entier32 F() { retourner A::Vue; }",
            "espace A { alias Appeler = Cible; publique entier64 Cible(entier64 x) { retourner x; } } "
            "publique entier32 Cible(entier32 x) { retourner x; } "
            "publique entier32 F() { retourner A::Appeler(42); }",
            "espace A { alias Vue = X; entier64 X; publique entier32 F() { retourner X; } } entier32 X;",
            "alias Vue = X; entier32 X; énumération E { A = 1, B = convertir<entier32>(E::A) + 2 }; "
            "publique entier32 F() { retourner convertir<entier32>(E::B) + Vue; }",
            "classe C { publique: entier32 Lire() { retourner 42; } }; alias Appeler = C::Lire;",
            "alias Vue = S; structure S { entier32 X; }; "
            "publique Vue Fabriquer(entier32 x) { retourner {x}; } "
            "publique entier32 F() { Vue objet = Fabriquer(42); retourner objet.X; }",
            "alias Vue = Base; classe Base { publique: entier32 X; }; "
            "classe Derivee : publique Vue { publique: entier32 Y; }; "
            "publique entier32 F(Derivee* p) { Vue* b = p; retourner b->X; }",
            "alias Vue = C; classe C { publique: entier32 X; constructeur() : X(42) {} }; "
            "publique entier32 F() { Vue objet; retourner objet.X; }",
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
            for (const auto& texte : {valides[index], TraduireCorpusConversions(valides[index])})
            {
                const auto resultat = AnalyserSemantiqueValide(syntaxe, semantique, texte,
                    "alias-racine-valide-" + std::to_string(index));
                auto programme = GsPP::AnalyseurSyntaxique(
                    GsPP::Lexeur(texte, "types-alias-racine").Analyser(), "types-alias-racine").Analyser();
                GsPP::AnalyseurSemantique().Analyser(programme);
                std::unordered_map<std::uint64_t, std::uint64_t> typesReference;
                const auto clePosition = [](auto ligne, auto colonne)
                {
                    return (static_cast<std::uint64_t>(ligne) << 32) | colonne;
                };
                const auto ajouterType = [&](const auto& position, const auto& type)
                {
                    typesReference.emplace(clePosition(position.Ligne, position.Colonne), HacherTypeDeclaration(type));
                };
                for (const auto& fonction : programme.Fonctions)
                {
                    ajouterType(fonction.Position, fonction.TypeRetour);
                    for (const auto& parametre : fonction.Parametres)
                        if (parametre.Nom != "soi") ajouterType(parametre.Position, parametre.Type);
                }
                for (const auto& globale : programme.VariablesGlobales) ajouterType(globale.Position, globale.Type);
                for (const auto& structure : programme.Structures)
                    for (const auto& champ : structure.Champs) ajouterType(champ.Position, champ.Type);
                for (const auto& resolution : resultat.Resolutions)
                    Exiger(resolution.IndexSymbole < resultat.Symboles.size()
                               && resultat.Symboles[resolution.IndexSymbole].Genre != 4,
                        "une résolution d'expression conserve l'alias au lieu de sa déclaration canonique");
                for (const auto& symbole : resultat.Symboles)
                {
                    if (symbole.Genre == 4)
                        Exiger(symbole.HachageType == resultat.Noeuds[symbole.IndexNoeud].HachageType,
                            "l'empreinte publique de la cible d'alias a été remplacée par le cache privé");
                    if (symbole.Genre == 2 || symbole.Genre == 3 || symbole.Genre == 5 || symbole.Genre == 8)
                    {
                        const auto& noeud = resultat.Noeuds[symbole.IndexNoeud];
                        if (noeud.Genre == 13 || noeud.Genre == 14) continue;
                        const auto type = typesReference.find(clePosition(noeud.Ligne, noeud.Colonne));
                        Exiger(type != typesReference.end() && symbole.HachageType == type->second,
                            "type canonique différent du bootstrap pour alias-racine-valide-" + std::to_string(index));
                    }
                }
            }

        std::string chaine;
        for (unsigned index = 0; index < 128; ++index)
            chaine += "alias Vue" + std::to_string(index) + " = "
                + (index == 127 ? std::string("X") : "Vue" + std::to_string(index + 1)) + "; ";
        chaine += "entier32 X; publique entier32 F() { Vue0 = 42; retourner Vue0 + Vue64 + Vue127; }";
        AnalyserSemantiqueValide(syntaxe, semantique, chaine, "alias-racine-chaine-128-fr");
        AnalyserSemantiqueValide(syntaxe, semantique, TraduireCorpusConversions(chaine),
            "alias-racine-chaine-128-en");

        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"alias Vue = Absente; publique vide F() {}", 110},
            {"espace A {\n alias Vue = Copie;\n alias Copie = Absente;\n} publique vide F() {}", 110},
            {"alias Vue = Vue; publique vide F() {}", 109},
            {"alias Vue = Copie; alias Copie = Vue; publique vide F() {}", 109},
            {"alias Entree = Vue;\n alias Vue = Copie;\n alias Copie = Vue; publique vide F() {}", 109},
            {"espace A { alias Vue = B::Vue; } espace B { alias Vue = A::Vue; } publique vide F() {}", 109},
            {"alias Vue = E; énumération E { X }; publique vide F() {}", 110},
            {"alias Vue = E::X; énumération E { X }; publique vide F() {}", 110},
            {"alias Appeler = Cible; publique entier32 Cible(entier32 x) { retourner x; } "
             "publique entier64 Cible(entier64 x) { retourner x; }", 112},
            {"alias Appeler = Copie; alias Copie = Cible; "
             "publique entier32 Cible(entier32 x) { retourner x; } "
             "publique entier64 Cible(entier64 x) { retourner x; }", 112},
            {"classe C { publique: entier32 Lire(entier32 x) { retourner x; } "
             "entier64 Lire(entier64 x) { retourner x; } }; alias Appeler = C::Lire;", 112},
            {"alias Vue = X; constante entier32 X = 42; publique vide F() { Vue = 7; }", 71},
            {"alias Vue = X; entier32 X; publique vide F() { Vue[0]; }", 50},
            {"alias Vue = S; structure S { entier32 X; }; publique vide F() { Vue; }", 18},
            {"alias Vue = X; entier32 X; publique vide F(Vue* p) {}", 100},
            {"alias Vue = F; publique vide F(Vue* p) {}", 100},
            {"alias Vue = Absente; alias Vue = Absente; publique vide F() {}", 10},
            {"alias F = Absente; publique vide F() {}", 11},
            {"alias S = Absente; structure S { entier32 X; }; publique vide F() {}", 11},
            {"alias X = Absente; entier32 X; publique vide F() {}", 11},
            {"alias Vue = Absente;", 110},
            {"alias Vue = Vue;", 109},
            {"alias Vue = Absente; énumération E { X = vrai }; publique vide F() {}", 85},
            {"alias Vue = Vue; énumération E { X = Absente }; publique vide F() {}", 18},
            {"alias Vue = X; entier32 X[2]; publique vide F() { entier32* p = &Vue; }", 48},
            {"alias Appeler = Rappel; pointeur_fonction<entier32(entier32)> Rappel; "
             "publique vide F() { Appeler(vrai); }", 55},
            {"alias Appeler = Cible; publique entier32 Cible(entier32 x) { retourner x; } "
             "publique vide F() { Appeler(vrai); }", 21},
            {"espace N { alias Appeler = Cible; publique entier32 Cible() { retourner 42; } } "
             "publique entier32 Cible(entier32 x) { retourner x; } "
             "publique entier64 Cible(entier64 x) { retourner x; }", 112},
            {"alias Appeler = Cible; publique entier32 Cible(entier32 x) { retourner x; } "
             "publique vide F() { Appeler(); }", 21},
            {"alias Appeler = Cible; publique entier32 Cible(entier32 x, entier32 y) { retourner x + y; } "
             "publique vide F() { Appeler(42, vrai); }", 21},
            {"alias A::Vue = X; espace A { alias Vue = X; } entier32 X; publique vide F() {}", 10},
            {"alias A::F = Absente; espace A { publique vide F() {} }", 11},
            {"classe C { publique: entier32 Lire() { retourner 42; } }; alias C::Lire = Absente;", 11},
            {"classe C { publique: entier32 Lire() { retourner 42; } }; espace C { entier32 Lire; }", 9},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
        {
            const auto& [texte, code] = refus[index];
            ComparerErreurSemantique(syntaxe, semantique, texte, code,
                "alias-racine-refuse-fr-" + std::to_string(index));
            ComparerErreurSemantique(syntaxe, semantique, TraduireCorpusConversions(texte), code,
                "alias-racine-refuse-en-" + std::to_string(index));
        }
    }

    void TesterDeclarationsHeritageSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe,
        AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"classe B {}; classe D : privée B {}; publique vide F() {}", 113},
            {"classe B {}; classe D : protégée B {}; publique vide F() {}", 113},
            {"\n\nclasse D : publique Absente {};\npublique vide F() {}", 100},
            {"structure B {}; classe D : publique B {}; publique vide F() {}", 114},
            {"union B { entier32 X; }; classe D : publique B {}; publique vide F() {}", 114},
            {"énumération B { X }; classe D : publique B {}; publique vide F() {}", 114},
            {"classe D : publique D {}; publique vide F() {}", 115},
            {"classe D : publique Vue {}; alias Vue = D; publique vide F() {}", 115},
            {"espace N {\n  classe D : publique N::D {};\n}\npublique vide F() {}", 115},
            {"espace N { classe D : publique Copie {}; alias Vue = D; alias Copie = Vue; } "
             "publique vide F() {}", 115},
            {"espace A { classe B {}; } espace N { classe D : publique B {}; } "
             "publique vide F() {}", 100},
            {"espace N { structure B {}; classe D : publique B {}; } publique vide F() {}", 114},
            {"espace A { classe B {}; } espace N { classe D : publique A::Absente {}; } "
             "publique vide F() {}", 100},
            {"structure B {}; alias Vue = B; classe D : publique Vue {}; publique vide F() {}", 114},
            {"énumération B { X }; alias Vue = B; classe D : publique Vue {}; publique vide F() {}", 110},
            {"publique vide B() {} alias Vue = B; classe D : publique Vue {};", 100},
            {"entier32 B; alias Vue = B; classe D : publique Vue {}; publique vide F() {}", 100},
            {"classe A : publique B {}; classe B : publique A {}; publique vide F() {}", 57},
            {"classe A : publique B {}; classe B : publique C {}; classe C : publique A {}; "
             "publique vide F() {}", 57},
            {"classe A : publique Vue {}; alias Vue = B; classe B : publique A {}; "
             "publique vide F() {}", 57},
            {"classe D : privée Absente { Inconnu X; }; publique vide F() {}", 113},
            {"classe D : publique Absente { Inconnu X; }; publique vide F() {}", 100},
            {"structure B {}; classe D : publique B { Inconnu X; }; publique vide F() {}", 114},
            {"classe D : publique D { Inconnu X; }; publique vide F() {}", 115},
            {"classe B {}; classe D : privée B {}; alias Vue = Absente; publique vide F() {}", 110},
            {"classe B {}; classe D : privée B {}; énumération E { X = 1 / 0 }; publique vide F() {}", 89},
            {"classe D : publique Absente {}; classe D {}; publique vide F() {}", 6},
            {"classe D : publique Absente {};", 100},
            {"classe D : publique Absente {}; publique vide F(Inconnu p) {}", 100},
            {"espace N { classe B {}; } structure B {}; espace N { classe D : publique B {}; } "
             "publique vide F() {}", 114}};
        for (std::size_t index = 0; index < refus.size(); ++index)
        {
            const auto& [texte, code] = refus[index];
            ComparerErreurSemantique(syntaxe, semantique, texte, code,
                "heritage-refus-fr-" + std::to_string(index));
            ComparerErreurSemantique(syntaxe, semantique, TraduireCorpusConversions(texte), code,
                "heritage-refus-en-" + std::to_string(index));
        }
        const std::vector<std::string> valides{
            "classe B {}; classe D : publique B {}; publique vide F(D& objet) {}",
            "classe B {}; classe D : B {}; publique vide F(D& objet) {}",
            "classe D : publique B {}; classe B {}; publique vide F(D& objet) {}",
            "classe B {}; alias Vue = B; alias Copie = Vue; classe D : publique Copie {}; "
            "publique vide F(D& objet) { B& base = objet; }",
            "espace N { classe B {}; classe D : publique B {}; } publique vide F(N::D& objet) {}",
            "espace A { classe B {}; } espace N { classe D : publique A::B {}; } "
            "publique vide F(N::D& objet) { A::B& base = objet; }",
            "espace A { classe B {}; } espace N { alias Vue = A::B; classe D : publique Vue {}; } "
            "publique vide F(N::D& objet) { A::B& base = objet; }",
            "classe B {}; espace N { structure B {}; classe D : publique B {}; } "
            "publique vide F(N::D& objet) { B& base = objet; }",
            "espace N { structure B {}; } classe B {}; espace N { classe D : publique B {}; } "
            "publique vide F(N::D& objet) { B& base = objet; }",
            "espace N { espace A { classe B {}; } classe D : publique A::B {}; } "
            "publique vide F(N::D& objet) { N::A::B& base = objet; }",
            "classe A {}; classe B : publique A {}; classe C : publique B {}; "
            "publique vide F(C& objet) { A& base = objet; }",
            "classe B { publique: entier32 X; }; classe D : publique B {}; "
            "publique entier32 F(D& objet) { retourner objet.X; }",
            "espace A { classe B { publique: entier32 X; }; } "
            "espace N { classe B { publique: entier32 Y; }; classe D : publique B {}; } "
            "publique entier32 F(N::D& objet) { retourner objet.Y; }",
            "espace N { classe B {}; classe D : /* base */ publique /* nom */ N /* qualifie */ :: B {}; } "
            "publique vide F(N::D& objet) { N::B& base = objet; }"};
        for (std::size_t index = 0; index < valides.size(); ++index)
        {
            AnalyserSemantiqueValide(syntaxe, semantique, valides[index],
                "heritage-valide-fr-" + std::to_string(index));
            AnalyserSemantiqueValide(syntaxe, semantique, TraduireCorpusConversions(valides[index]),
                "heritage-valide-en-" + std::to_string(index));
        }
    }

    /**
     * <résumé>Compare les doublons et les surcharges distinctes aux signatures canoniques du bootstrap.</résumé>
     * @Paramètre(AnalyseurDeclarationsAutoHeberge: syntaxe) Analyseur de déclarations testé.
     * @Paramètre(AnalyseurSemantiqueAutoHeberge: semantique) Frontend auto-hébergé testé.
     **/
    void TesterDoublonsSurchargesSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe,
        AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"publique vide F() {} publique vide F() {}", 118},
            {"publique entier32 F(entier32 a) { retourner a; } "
             "publique entier32 F(entier32 b) { retourner b; }", 118},
            {"publique entier32 F() { retourner 1; } publique entier64 F() { retourner 2; }", 118},
            {"publique vide F(constante entier32 a) {} publique vide F(constante entier32 b) {}", 118},
            {"publique vide F(entier32** a) {} publique vide F(entier32** b) {}", 118},
            {"publique vide F(constante entier32& a) {} publique vide F(constante entier32& b) {}", 118},
            {"publique vide F(pointeur_fonction<entier32(entier32&)> a) {} "
             "publique vide F(pointeur_fonction<entier32(entier32&)> b) {}", 118},
            {"structure S {}; alias Vue = S; publique vide F(S* a) {} publique vide F(Vue* b) {}", 118},
            {"publique vide F(Vue* a) {} publique vide F(S* b) {} "
             "alias Vue = Copie; alias Copie = S; structure S {};", 118},
            {"espace A { structure S {}; } espace N { alias Vue = A::S; "
             "publique vide F(A::S* a) {} publique vide F(Vue* b) {} }", 118},
            {"classe C { publique: entier32 F(entier32 a) { retourner a; } "
             "entier32 F(entier32 b) { retourner b; } };", 118},
            {"classe C { publique: entier32 F() { retourner 1; } entier64 F() { retourner 2; } };", 118},
            {"classe C { publique: vide F(constante entier32& a) {} vide F(constante entier32& b) {} };", 118},
            {"classe C { publique: constructeur() {} constructeur() {} };", 118},
            {"classe C { publique: constructeur(entier32 a) {} constructeur(entier32 b) {} };", 118},
            {"classe C { publique: destructeur() {} destructeur() {} };", 118},
            {"classe C { publique: entier32 opérateur+(entier32 a) { retourner a; } "
             "entier32 opérateur+(entier32 b) { retourner b; } };", 118},
            {"classe C { publique: booléen opérateur!() { retourner vrai; } "
             "booléen opérateur!() { retourner faux; } };", 118},
            {"classe C { privée: vide F() {} publique: vide F() {} };", 118},
            {"classe C { publique: virtuel vide F() {} vide F() {} };", 118},
            {"classe B { publique: virtuel vide F() {} }; classe D : publique B { "
             "publique: remplacer vide F() {} remplacer vide F() {} };", 118},
            {"publique vide F(entier32 a) {}\npublique vide F(booléen a) {}\n"
             "publique vide F(booléen b) {}\npublique vide F(entier32 b) {}", 118},
            {"classe C {\n publique:\n vide F(\n entier32 a\n ) {}\n"
             " vide F(\n entier32 b\n ) {}\n};", 118},
            {"publique vide F() { Introuvable(); } publique vide F() {}", 118},
            {"entier32 Globale = Introuvable(); publique vide F() {} publique vide F() {}", 118},
            {"publique vide F() {} publique vide F() {} publique vide G(Introuvable x) {}", 100},
            {"classe C : publique C {}; publique vide F() {} publique vide F() {}", 115},
            {"structure S { S X; }; publique vide F() {} publique vide F() {}", 57},
            {"publique vide F(entier32 a, entier32 b, entier32 c, entier32 d, entier32 e) {} "
             "publique vide F(entier32 a, entier32 b, entier32 c, entier32 d, entier32 e) {}", 106},
            {"classe C { publique: vide F(C* a) {} vide F(Vue* b) {} }; alias Vue = C;", 118},
            {"externe entier32 F(entier32 a); publique entier32 F(entier32 b) { retourner b; }", 118},
            {"espace N { publique vide F() {} } espace N { publique vide F() {} }", 118},
            {"structure S {}; publique entier32 opérateur+(S& a, entier32 x) { retourner x; } "
             "publique entier32 opérateur+(S& b, entier32 y) { retourner y; }", 118},
            {"structure S {}; publique booléen opérateur!(constante S& a) { retourner vrai; } "
             "publique booléen opérateur!(constante S& b) { retourner faux; }", 118},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
        {
            const auto& [texte, code] = refus[index];
            ComparerErreurSemantique(syntaxe, semantique, texte, code,
                "doublon-surcharge-refuse-fr-" + std::to_string(index));
            ComparerErreurSemantique(syntaxe, semantique, TraduireCorpusConversions(texte), code,
                "doublon-surcharge-refuse-en-" + std::to_string(index));
        }

        const std::vector<std::string> valides{
            "publique vide F() {} publique vide F(entier32 a) {}",
            "publique vide F(entier32 a) {} publique vide F(entier64 a) {}",
            "publique vide F(entier32 a) {} publique vide F(constante entier32 a) {}",
            "publique vide F(entier32* a) {} publique vide F(entier32** a) {}",
            "publique vide F(entier32& a) {} publique vide F(constante entier32& a) {}",
            "publique vide F(entier32 a) {} publique vide F(entier32& a) {}",
            "publique vide F(pointeur_fonction<entier32()> a) {} "
            "publique vide F(pointeur_fonction<entier64()> a) {}",
            "publique vide F(pointeur_fonction<vide(entier32)> a) {} "
            "publique vide F(pointeur_fonction<vide(entier64)> a) {}",
            "espace A { publique vide F(entier32 a) {} } espace B { publique vide F(entier32 a) {} }",
            "classe A { publique: vide F(entier32 a) {} }; classe B { publique: vide F(entier32 a) {} };",
            "classe C { publique: vide F(entier32 a) {} }; publique vide F(C& a, entier32 b) {}",
            "classe B { publique: vide F(entier32 a) {} }; classe D : publique B { publique: vide F(entier32 a) {} };",
            "classe C { publique: constructeur() {} constructeur(entier32 a) {} };",
            "classe A { publique: destructeur() {} }; classe B { publique: destructeur() {} };",
            "classe C { publique: entier32 opérateur+(entier32 a) { retourner a; } "
            "entier64 opérateur+(entier64 a) { retourner a; } };",
            "espace A { structure S {}; } espace B { structure S {}; } "
            "publique vide F(A::S* a) {} publique vide F(B::S* a) {}",
            "publique vide F(entier32 a, entier64 b) {} publique vide F(entier64 a, entier32 b) {}",
            "publique vide F(constante entier32* a) {} publique vide F(volatile entier32* a) {}",
            "structure S {}; publique entier32 opérateur+(S& a, entier32 x) { retourner x; } "
            "publique entier64 opérateur+(S& a, entier64 x) { retourner x; }",
            "structure S {}; publique booléen opérateur!(S& a) { retourner vrai; } "
            "publique booléen opérateur!(constante S& a) { retourner faux; }",
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
        {
            AnalyserSemantiqueValide(syntaxe, semantique, valides[index],
                "surcharge-distincte-fr-" + std::to_string(index));
            AnalyserSemantiqueValide(syntaxe, semantique, TraduireCorpusConversions(valides[index]),
                "surcharge-distincte-en-" + std::to_string(index));
        }
    }

    /**
     * <résumé>Compare les collisions entre récepteurs implicites et paramètres explicites de même nom complet.</résumé>
     * @Paramètre(AnalyseurDeclarationsAutoHeberge: syntaxe) Analyseur de déclarations testé.
     * @Paramètre(AnalyseurSemantiqueAutoHeberge: semantique) Frontend auto-hébergé testé.
     **/
    void TesterCollisionsSignaturesNonLieesSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe,
        AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"classe C { publique: vide F() {} }; espace C { publique vide F(C& objet) {} }", 118},
            {"espace C { publique vide F(C& objet) {} } classe C { publique: vide F() {} };", 118},
            {"classe C { publique: vide F(entier32 a) {} }; "
             "espace C { publique vide F(C& objet, entier32 b) {} }", 118},
            {"classe C { publique: entier32 F() { retourner 1; } }; "
             "espace C { publique entier64 F(C& objet) { retourner 2; } }", 118},
            {"classe C { privée: vide F() {} }; espace C { publique vide F(C& objet) {} }", 118},
            {"espace C { externe vide F(C& objet); } classe C { publique: vide F() {} };", 118},
            {"espace N { classe C { publique: vide F() {} }; "
             "espace C { publique vide F(N::C& objet) {} } }", 118},
            {"alias Vue = Copie; alias Copie = C; classe C { publique: vide F() {} }; "
             "espace C { publique vide F(Vue& objet) {} }", 118},
            {"classe C { publique: vide F(pointeur_fonction<vide(entier32)> a) {} }; "
             "espace C { publique vide F(C& objet, pointeur_fonction<vide(entier32)> b) {} }", 118},
            {"classe C { publique: vide F(constante entier32** a) {} }; "
             "espace C { publique vide F(C& objet, constante entier32** b) {} }", 118},
            {"classe C { publique: vide F(entier32& a) {} }; "
             "espace C { publique vide F(C& objet, entier32& b) {} }", 118},
            {"classe C { publique: vide F(entier32 a, entier64 b, booléen c) {} }; "
             "espace C { publique vide F(C& objet, entier32 x, entier64 y, booléen z) {} }", 118},
            {"classe C { publique: entier32 opérateur+(entier32 a) { retourner a; } }; "
             "espace C { publique entier32 opérateur+(C& objet, entier32 b) { retourner b; } }", 118},
            {"espace C { publique booléen opérateur!(C& objet) { retourner vrai; } } "
             "classe C { publique: booléen opérateur!() { retourner faux; } };", 118},
            {"espace C { publique vide F(C& objet, entier32 x) {} }\n"
             "classe C { publique:\n vide F(booléen b) {}\n vide F(entier32 y) {} };\n"
             "espace C { publique vide F(C& objet, booléen c) {} }", 118},
            {"classe B { publique: virtuel vide F() {} }; classe C : publique B { publique: vide F() {} }; "
             "espace C { publique vide F(C& objet) {} }", 118},
            {"classe B { publique: virtuel vide F() {} }; classe C : publique B { publique: remplacer vide F() {} }; "
             "espace C { publique vide F(C& objet) {} }", 118},
            {"classe C { publique: vide F() { Introuvable(); } }; "
             "espace C { publique vide F(C& objet) {} }", 118},
            {"entier32 Globale = Introuvable(); classe C { publique: vide F() {} }; "
             "espace C { publique vide F(C& objet) {} }", 118},
            {"classe C { publique: vide F() {} }; espace C { publique vide F(C& objet) {} } "
             "alias Appeler = C::F;", 112},
            {"classe C { publique: vide F() {} }; espace C { publique vide F(C& objet) {} } "
             "publique vide G(Introuvable x) {}", 100},
            {"classe C : publique C { publique: vide F() {} }; espace C { publique vide F(C& objet) {} }", 115},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
        {
            const auto& [texte, code] = refus[index];
            ComparerErreurSemantique(syntaxe, semantique, texte, code,
                "collision-signature-non-liee-fr-" + std::to_string(index));
            ComparerErreurSemantique(syntaxe, semantique, TraduireCorpusConversions(texte), code,
                "collision-signature-non-liee-en-" + std::to_string(index));
        }

        const std::vector<std::string> valides{
            "classe C { publique: vide F() {} }; espace C { publique vide F() {} }",
            "classe C { publique: vide F() {} }; espace C { publique vide F(C* objet) {} }",
            "classe C { publique: vide F() {} }; espace C { publique vide F(constante C& objet) {} }",
            "classe C { publique: vide F() {} }; espace C { publique vide F(volatile C& objet) {} }",
            "classe C { publique: vide F(entier32 a) {} }; espace C { publique vide F(C& objet, entier64 b) {} }",
            "classe C { publique: vide F(entier32 a) {} }; espace C { publique vide F(entier32 b, C& objet) {} }",
            "classe C { publique: vide F(C& autre) {} }; espace C { publique vide F(C& objet) {} }",
            "classe B {}; classe C { publique: vide F() {} }; espace C { publique vide F(B& objet) {} }",
            "classe C { publique: vide F() {} }; alias Vue = C; espace Vue { publique vide F(C& objet) {} }",
            "espace N { classe C { publique: vide F() {} }; espace C { publique vide F(constante N::C& objet) {} } }",
            "classe C { publique: vide F(pointeur_fonction<entier32()> a) {} }; "
            "espace C { publique vide F(C& objet, pointeur_fonction<entier64()> b) {} }",
            "classe C { publique: entier32 opérateur+(entier32 a) { retourner a; } }; "
            "espace C { publique entier64 opérateur+(C& objet, entier64 b) { retourner b; } }",
            "classe C { publique: booléen opérateur!() { retourner vrai; } }; "
            "espace C { publique booléen opérateur!(constante C& objet) { retourner faux; } }",
            "classe C { publique: vide F(constante entier32& a) {} }; "
            "espace C { publique vide F(C& objet, entier32& b) {} }",
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
        {
            AnalyserSemantiqueValide(syntaxe, semantique, valides[index],
                "signature-non-liee-distincte-fr-" + std::to_string(index));
            AnalyserSemantiqueValide(syntaxe, semantique, TraduireCorpusConversions(valides[index]),
                "signature-non-liee-distincte-en-" + std::to_string(index));
        }
    }

    /**
     * <résumé>Compare les appels de groupes mêlant méthodes non liées et fonctions libres de même nom complet.</résumé>
     * @Paramètre(AnalyseurDeclarationsAutoHeberge: syntaxe) Analyseur de déclarations testé.
     * @Paramètre(AnalyseurSemantiqueAutoHeberge: semantique) Frontend auto-hébergé testé.
     **/
    void TesterAppelsGroupesMixtesSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe,
        AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"classe C { publique: entier32 Lire() { retourner 1; } }; "
             "espace C { publique entier32 Lire(constante C& objet) { retourner 2; } } "
             "publique entier32 G(C& objet) { retourner C::Lire(objet); }", 22},
            {"espace C { publique entier32 Lire(constante C& objet) { retourner 2; } } "
             "classe C { publique: entier32 Lire() { retourner 1; } }; "
             "publique entier32 G(C& objet) { retourner C::Lire(objet); }", 22},
            {"classe C { publique: entier32 Lire() { retourner 1; } }; "
             "espace C { publique entier32 Lire(constante C& objet) { retourner 2; } } "
             "publique entier32 G(C& objet) { retourner objet.Lire(); }", 22},
            {"classe C { publique: entier32 Lire() { retourner 1; } }; "
             "espace C { publique entier32 Lire(constante C& objet) { retourner 2; } } "
             "publique entier32 G(C* objet) { retourner objet->Lire(); }", 22},
            {"classe C { publique: entier32 Lire(entier32 x) { retourner x; } }; "
             "espace C { publique entier32 Lire(C& objet, booléen x) { retourner 2; } } "
             "publique entier32 G(C& objet) { retourner C::Lire(objet); }", 21},
            {"classe C { publique: entier32 Lire(entier32 x) { retourner x; } }; "
             "espace C { publique entier32 Lire(C& objet, booléen x) { retourner 2; } } "
             "publique entier32 G(C& objet) { retourner objet.Lire(); }", 21},
            {"classe C { privée: entier32 Lire(entier32 x) { retourner x; } }; "
             "espace C { publique entier32 Lire(C& objet, booléen x) { retourner 2; } } "
             "publique entier32 G(C& objet, entier32 x) { retourner C::Lire(objet, x); }", 25},
            {"classe C { privée: entier32 Lire(entier32 x) { retourner x; } }; "
             "espace C { publique entier32 Lire(C& objet, booléen x) { retourner 2; } } "
             "publique entier32 G(C& objet, entier32 x) { retourner objet.Lire(x); }", 25},
            {"classe C { publique: entier32 Lire() { retourner 1; } }; "
             "espace C { publique entier32 Lire(C& objet, booléen x) { retourner 2; } } "
             "pointeur_fonction<entier32(C&)> Rappel = C::Lire;", 19},
            {"classe C { publique: entier32 Lire() { retourner 1; } }; "
             "espace C { publique entier32 Lire(C& objet, booléen x) { retourner 2; } } "
             "alias Appeler = C::Lire;", 112},
            {"classe C { publique: entier32 Lire(entier32& x) { retourner x; } }; "
             "espace C { publique entier32 Lire(C& objet, entier32 x) { retourner x; } } "
             "publique entier32 G(C& objet, entier32 x) { retourner C::Lire(objet, x); }", 22},
            {"classe C { publique: entier32 Lire(entier32& x) { retourner x; } }; "
             "espace C { publique entier32 Lire(C& objet, entier32 x) { retourner x; } } "
             "publique entier32 G(C& objet, entier32 x) { retourner objet.Lire(x); }", 22},
            {"classe C { publique: entier32 Lire(entier8 x) { retourner 1; } }; "
             "espace C { publique entier32 Lire(C& objet, naturel8 x) { retourner 2; } } "
             "publique entier32 G(C& objet) { retourner objet.Lire(7); }", 22},
            {"classe C { publique: entier32 Lire(entier32 x) { retourner x; } }; "
             "espace C { publique entier32 Lire(C& objet, booléen x) { retourner 2; } } "
             "publique entier32 G(constante C& objet, entier32 x) { retourner objet.Lire(x); }", 21},
            {"classe C { publique: entier32 Lire(entier32 x) { retourner x; } }; "
             "espace C { publique entier32 Lire(C& objet, booléen x) { retourner 2; } } "
             "publique entier32 G(C* objet, entier32* x) { retourner objet->Lire(x); }", 21},
            {"classe C { publique: entier32 Lire(entier32 x) { retourner x; } }; "
             "espace C { publique entier32 Lire(C& objet, booléen x) { retourner 2; } } "
             "publique entier32 G(C* objet, entier32 x) { retourner C::Lire(objet, x); }", 21},
            {"classe C { privée: entier32 Lire(entier32 x) { retourner x; } }; "
             "espace C { publique entier32 Lire(C& objet, entier64 x) { retourner 2; } } "
             "publique entier32 G(C& objet) { retourner objet.Lire(7); }", 25},
            {"classe C { publique: entier32 Lire(entier32 x) { retourner x; } entier32 Lire(booléen x) { retourner 1; } }; "
             "publique entier32 G(C& objet) { retourner C::Lire(objet); }", 21},
            {"classe C { publique: entier32 Lire() { retourner 1; } }; "
             "espace C { publique entier32 Lire(C& objet, booléen x) { retourner 2; } } "
             "publique vide G() { pointeur_fonction<entier32(C&)> rappel = &C::Lire; }", 19},
            {"espace N { classe C { publique: entier32 Lire() { retourner 1; } }; "
             "espace C { publique entier32 Lire(constante N::C& objet) { retourner 2; } } } "
             "publique entier32 G(N::C& objet) { retourner N::C::Lire(objet); }", 22},
            {"classe C { privée: entier32 Lire() { retourner 1; } }; "
             "espace C { publique entier32 Lire(constante C& objet) { retourner 2; } } "
             "publique entier32 G(C& objet) { retourner objet.Lire(); }", 22},
            {"classe B { publique: entier32 Lire() { retourner 1; } }; classe C : publique B { "
             "publique: entier32 Lire(entier32 x) { retourner x; } }; "
             "espace C { publique entier32 Lire(C& objet, booléen x) { retourner 2; } } "
             "publique entier32 G(C& objet) { retourner objet.Lire(); }", 21},
            {"classe C { publique: entier32 Lire() { retourner 1; } }; "
             "publique entier32 G(constante C& objet) { retourner objet.Lire(); }", 21},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
        {
            const auto& [texte, code] = refus[index];
            ComparerErreurSemantique(syntaxe, semantique, texte, code,
                "appel-groupe-mixte-refuse-fr-" + std::to_string(index));
            ComparerErreurSemantique(syntaxe, semantique, TraduireCorpusConversions(texte), code,
                "appel-groupe-mixte-refuse-en-" + std::to_string(index));
        }

        const std::string groupe =
            "classe C { publique: entier32 Lire(entier32 x) { retourner x; } }; "
            "espace C { publique entier32 Lire(C& objet, booléen x) { retourner 2; } } ";
        const std::string inverse =
            "espace C { publique entier32 Lire(C& objet, booléen x) { retourner 2; } } "
            "classe C { publique: entier32 Lire(entier32 x) { retourner x; } }; ";
        const std::string groupeQualifie =
            "espace N { classe C { publique: entier32 Lire(entier32 x) { retourner x; } }; "
            "espace C { publique entier32 Lire(N::C& objet, booléen x) { retourner 2; } } } ";
        const std::vector<std::pair<std::string, bool>> valides{
            {groupe + "publique entier32 G(C& objet, entier32 x) { retourner C::Lire(objet, x); }", true},
            {groupe + "publique entier32 G(C& objet) { retourner C::Lire(objet, vrai); }", false},
            {inverse + "publique entier32 G(C& objet, entier32 x) { retourner C::Lire(objet, x); }", true},
            {inverse + "publique entier32 G(C& objet) { retourner C::Lire(objet, vrai); }", false},
            {groupe + "publique entier32 G(C& objet, entier32 x) { retourner objet.Lire(x); }", true},
            {groupe + "publique entier32 G(C& objet) { retourner objet.Lire(vrai); }", false},
            {groupe + "publique entier32 G(C* objet, entier32 x) { retourner objet->Lire(x); }", true},
            {groupe + "publique entier32 G(C* objet) { retourner objet->Lire(vrai); }", false},
            {"classe C { publique: entier32 Lire() { retourner 1; } }; "
             "espace C { publique entier32 Lire() { retourner 2; } } "
             "publique entier32 G() { retourner C::Lire(); }", false},
            {"classe C { publique: entier32 Lire() { retourner 1; } }; "
             "espace C { publique entier32 Lire() { retourner 2; } } "
             "publique entier32 G(C& objet) { retourner C::Lire(objet); }", true},
            {"classe C { privée: entier32 Lire(entier32 x) { retourner x; } }; "
             "espace C { publique entier32 Lire(C& objet, booléen x) { retourner 2; } } "
             "publique entier32 G(C& objet) { retourner objet.Lire(vrai); }", false},
            {"classe C { publique: entier32 Lire() { retourner 1; } }; "
             "espace C { publique entier32 Lire(constante C& objet) { retourner 2; } } "
             "publique entier32 G(constante C& objet) { retourner objet.Lire(); }", false},
            {"classe C { publique: entier32 Lire() { retourner 1; } }; "
             "espace C { publique entier32 Lire(constante C& objet) { retourner 2; } } "
             "publique entier32 G(constante C* objet) { retourner objet->Lire(); }", false},
            {groupeQualifie + "publique entier32 G(N::C& objet, entier32 x) { retourner N::C::Lire(objet, x); }", true},
            {groupeQualifie + "publique entier32 G(N::C& objet) { retourner objet.Lire(vrai); }", false},
            {groupe + "alias Vue = C; publique entier32 G(Vue& objet) { retourner C::Lire(objet, vrai); }", false},
            {groupe + "alias Vue = C; publique entier32 G(Vue* objet, entier32 x) { retourner objet->Lire(x); }", true},
            {"classe C { publique: entier32 Lire(entier32 x) { retourner x; } entier32 Lire(booléen x) { retourner 2; } }; "
             "publique entier32 G(C& objet) { retourner C::Lire(objet, vrai); }", true},
            {"classe C { publique: entier32 Lire() { retourner 1; } }; "
             "publique entier32 G(C& objet) { retourner C::Lire(objet); }", true},
            {"classe C { publique: entier32 Lire(entier32 x) { retourner x; } }; "
             "espace C { publique entier32 Lire(C& objet, entier64 x) { retourner 2; } } "
             "publique entier32 G(C& objet) { retourner objet.Lire(7); }", true},
            {"classe C { publique: entier32 Lire(entier64 x) { retourner 1; } }; "
             "espace C { publique entier32 Lire(C& objet, entier32 x) { retourner x; } } "
             "publique entier32 G(C& objet) { retourner C::Lire(objet, 7); }", false},
            {"classe C { publique: entier32 Lire(entier32& x) { retourner x; } }; "
             "espace C { publique entier32 Lire(C& objet, entier32 x) { retourner x; } } "
             "publique entier32 G(C& objet) { retourner objet.Lire(7); }", false},
            {"classe C { publique: entier32 Lire(constante entier32& x) { retourner x; } }; "
             "espace C { publique entier32 Lire(C& objet, booléen x) { retourner 2; } } "
             "publique entier32 G(C& objet, constante entier32& x) { retourner C::Lire(objet, x); }", true},
            {"classe C { publique: entier32 Lire(pointeur_fonction<entier32()> x) { retourner 1; } }; "
             "espace C { publique entier32 Lire(C& objet, pointeur_fonction<booléen()> x) { retourner 2; } } "
             "publique entier32 G(C& objet, pointeur_fonction<booléen()> x) { retourner objet.Lire(x); }", false},
            {"classe B {}; classe C : publique B { publique: entier32 Lire() { retourner 1; } }; "
             "espace C { publique entier32 Lire(B& objet) { retourner 2; } } "
             "publique entier32 G(C& objet) { retourner objet.Lire(); }", true},
            {"classe B { publique: entier32 Lire() { retourner 1; } }; classe D : publique B {}; "
             "espace B { publique entier32 Lire(D& objet) { retourner 2; } } "
             "publique entier32 G(D& objet) { retourner objet.Lire(); }", false},
            {"classe B { publique: entier32 Lire() { retourner 1; } }; classe D : publique B {}; "
             "espace B { publique entier32 Lire(D& objet) { retourner 2; } } "
             "publique entier32 G(D* objet) { retourner objet->Lire(); }", false},
            {"classe B { publique: entier32 Lire() { retourner 1; } }; classe C : publique B { "
             "publique: entier32 Lire(entier32 x) { retourner x; } }; "
             "espace C { publique entier32 Lire(C& objet, booléen x) { retourner 2; } } "
             "publique entier32 G(C& objet) { retourner objet.Lire(vrai); }", false},
            {"classe C {}; espace C { publique entier32 Lire(C& objet) { retourner 2; } } "
             "publique entier32 G(C& objet) { retourner objet.Lire(); }", false},
            {"classe C { privée: entier32 Lire(entier32 x) { retourner x; } }; "
             "espace C { entier32 Lire(C& objet, booléen x) { retourner 2; } } "
             "publique entier32 G(C& objet) { retourner objet.Lire(vrai); }", false},
            {"classe C { publique: virtuel entier32 Lire(entier32 x) { retourner x; } }; "
             "espace C { publique entier32 Lire(C& objet, booléen x) { retourner 2; } } "
             "publique entier32 G(C& objet) { retourner objet.Lire(vrai); }", false},
            {"classe C { publique: entier32 Lire(entier32 x) { retourner x; } }; "
             "espace C { publique entier32 Lire(C& objet, booléen x) { retourner 2; } } "
             "publique entier32 G(volatile C& objet, entier32 x) { retourner objet.Lire(x); }", true},
            {"classe C { publique: entier32 Lire(entier32 x) { retourner x; } }; "
             "espace C { publique entier64 Lire(C& objet, booléen x) { retourner 2; } } "
             "publique entier64 G(C& objet) { retourner C::Lire(objet, vrai); }", false},
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
            for (const auto& texte : {valides[index].first, TraduireCorpusConversions(valides[index].first)})
            {
                const auto resultat = AnalyserSemantiqueValide(syntaxe, semantique, texte,
                    "appel-groupe-mixte-valide-" + std::to_string(index));
                auto programme = GsPP::AnalyseurSyntaxique(
                    GsPP::Lexeur(texte, "appel-groupe-mixte-reference").Analyser(), "appel-groupe-mixte-reference").Analyser();
                GsPP::AnalyseurSemantique().Analyser(programme);
                const auto appelant = std::find_if(programme.Fonctions.begin(), programme.Fonctions.end(),
                    [](const auto& fonction) { return fonction.NomSourceComplet() == "G"; });
                Exiger(appelant != programme.Fonctions.end() && appelant->Corps && appelant->Corps->Instructions.size() == 1
                           && appelant->Corps->Instructions.front()->Genre == GsPP::GenreInstruction::Retour,
                    "le corpus d'appel mixte ne contient pas son retour de référence");
                const auto& retour = static_cast<const GsPP::InstructionRetour&>(*appelant->Corps->Instructions.front());
                Exiger(retour.Valeur && retour.Valeur->Genre == GsPP::GenreExpression::Appel,
                    "le retour du corpus mixte n'est pas un appel");
                const auto& appel = static_cast<const GsPP::ExpressionAppel&>(*retour.Valeur);
                const auto fonctionReference = std::find_if(programme.Fonctions.begin(), programme.Fonctions.end(),
                    [&](const auto& fonction) { return fonction.NomComplet() == appel.NomDirect; });
                Exiger(fonctionReference != programme.Fonctions.end() && fonctionReference->EstMethode == valides[index].second,
                    "la cible prévue du corpus mixte ne correspond pas au bootstrap");
                std::size_t nombreAppels = 0;
                for (const auto& resolution : resultat.Resolutions)
                {
                    const auto& cible = resultat.Noeuds[resolution.IndexNoeud];
                    if ((cible.Genre != 24 && cible.Genre != 29)
                        || resultat.Noeuds[cible.Parent].Genre != 28
                        || resolution.IndexNoeud != cible.Parent + 1)
                        continue;
                    const auto& declaration = resultat.Noeuds[resultat.Symboles[resolution.IndexSymbole].IndexNoeud];
                    Exiger((declaration.Genre == 12) == valides[index].second,
                        "la mauvaise surcharge du groupe mixte a été sélectionnée : " + std::to_string(index));
                    Exiger(declaration.Ligne == fonctionReference->Position.Ligne
                               && declaration.Colonne == fonctionReference->Position.Colonne
                               && resolution.HachageType == HacherTypeDeclaration(fonctionReference->TypeRetour),
                        "la déclaration ou le retour choisi diffère du bootstrap : " + std::to_string(index));
                    Exiger(((resolution.Drapeaux & 32U) != 0) == valides[index].second,
                        "le drapeau de méthode ne correspond pas à la cible sélectionnée");
                    ++nombreAppels;
                }
                Exiger(nombreAppels == 1, "résolution d'appel mixte absente ou dupliquée");
            }
    }

    /**
     * <résumé>Compare les groupes d'opérateurs complets et leur récepteur au bootstrap.</résumé>
     * @Paramètre(AnalyseurDeclarationsAutoHeberge: syntaxe) Analyseur des corpus bilingues.
     * @Paramètre(AnalyseurSemantiqueAutoHeberge: semantique) Frontend auto-hébergé testé.
     **/
    void TesterOperateursGroupesMixtesSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe,
        AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::string groupe =
            "classe C { publique: entier32 opérateur+(entier32 x) { retourner x; } }; "
            "espace C { publique entier32 opérateur+(C& objet, booléen x) { retourner 2; } } ";
        const std::string inverse =
            "espace C { publique entier32 opérateur+(C& objet, booléen x) { retourner 2; } } "
            "classe C { publique: entier32 opérateur+(entier32 x) { retourner x; } }; ";
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"classe C { publique: entier32 opérateur+(entier32 x) { retourner x; } }; "
             "espace C { publique entier32 opérateur+(constante C& objet, entier32 x) { retourner x; } } "
             "publique entier32 G(C& objet, entier32 x) { retourner objet + x; }", 22},
            {"classe C { privée: entier32 opérateur+(entier32 x) { retourner x; } }; "
             "espace C { publique entier32 opérateur+(constante C& objet, entier32 x) { retourner x; } } "
             "publique entier32 G(C& objet, entier32 x) { retourner objet + x; }", 22},
            {groupe + "publique entier32 G(constante C& objet, entier32 x) { retourner objet + x; }", 21},
            {groupe + "publique entier32 G(C& objet, entier32* x) { retourner objet + x; }", 21},
            {"classe C { privée: entier32 opérateur+(entier32 x) { retourner x; } }; "
             "espace C { publique entier32 opérateur+(C& objet, entier64 x) { retourner 2; } } "
             "publique entier32 G(C& objet) { retourner objet + 7; }", 25},
            {"classe C { publique: entier32 opérateur+(entier8 x) { retourner 1; } }; "
             "espace C { publique entier32 opérateur+(C& objet, naturel8 x) { retourner 2; } } "
             "publique entier32 G(C& objet) { retourner objet + 7; }", 22},
            {"classe C { publique: booléen opérateur!() { retourner vrai; } }; "
             "espace C { publique booléen opérateur!(constante C& objet) { retourner faux; } } "
             "publique booléen G(C& objet) { retourner !objet; }", 22},
            {"classe C { publique: booléen opérateur!() { retourner vrai; } }; "
             "publique booléen G(constante C& objet) { retourner !objet; }", 21},
            {"classe C { publique: entier32 opérateur+(entier32 x) { retourner x; } }; "
             "publique entier32 opérateur+(constante C& objet, booléen x) { retourner 2; } "
             "publique entier32 G(C& objet) { retourner objet + vrai; }", 21},
            {"classe B { publique: entier32 opérateur+(entier32 x) { retourner x; } }; "
             "classe D : publique B {}; espace D { publique entier32 opérateur+(D& objet, booléen x) { retourner 2; } } "
             "publique entier32 G(D& objet, entier32 x) { retourner objet + x; }", 21},
            {"classe C {}; espace C { publique entier32 opérateur+(C& objet, entier32 x) { retourner x; } } "
             "publique entier32 G(constante C& objet, entier32 x) { retourner objet + x; }", 21},
            {"classe C {}; publique entier32 opérateur+(C& objet, booléen x) { retourner 1; } "
             "espace N { publique entier32 opérateur+(C& objet, entier32 x) { retourner x; } "
             "publique entier32 G(C& objet, entier32 x) { retourner objet + x; } }", 21},
            {"classe C { publique: entier32 opérateur+(entier32 x) { retourner x; } }; "
             "externe C Creer(); publique entier32 G() { retourner Creer() + 7; }", 21},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
            for (const auto& texte : {refus[index].first, TraduireCorpusConversions(refus[index].first)})
                ComparerErreurSemantique(syntaxe, semantique, texte, refus[index].second,
                    "operateur-groupe-mixte-refuse-" + std::to_string(index));

        const std::vector<std::pair<std::string, bool>> valides{
            {groupe + "publique entier32 G(C& objet, entier32 x) { retourner objet + x; }", true},
            {groupe + "publique entier32 G(C& objet) { retourner objet + vrai; }", false},
            {inverse + "publique entier32 G(C& objet, entier32 x) { retourner objet + x; }", true},
            {inverse + "publique entier32 G(C& objet) { retourner objet + vrai; }", false},
            {groupe + "alias Vue = C; publique entier32 G(Vue& objet) { retourner objet + vrai; }", false},
            {groupe + "publique entier32 G(volatile C& objet, entier32 x) { retourner objet + x; }", true},
            {"classe C { privée: entier32 opérateur+(entier32 x) { retourner x; } }; "
             "espace C { publique entier32 opérateur+(C& objet, booléen x) { retourner 2; } } "
             "publique entier32 G(C& objet) { retourner objet + vrai; }", false},
            {"classe C { publique: entier32 opérateur+(entier32 x) { retourner x; } }; "
             "espace C { publique entier64 opérateur+(constante C& objet, entier32 x) { retourner 2; } } "
             "publique entier64 G(constante C& objet, entier32 x) { retourner objet + x; }", false},
            {"classe C { publique: entier32 opérateur+(entier64 x) { retourner 1; } }; "
             "espace C { publique entier32 opérateur+(C& objet, entier32 x) { retourner x; } } "
             "publique entier32 G(C& objet) { retourner objet + 7; }", false},
            {"classe C { publique: entier32 opérateur+(entier32& x) { retourner x; } }; "
             "espace C { publique entier32 opérateur+(C& objet, entier32 x) { retourner x; } } "
             "publique entier32 G(C& objet) { retourner objet + 7; }", false},
            {"classe C { publique: booléen opérateur!() { retourner vrai; } }; "
             "espace C { publique booléen opérateur!(constante C& objet) { retourner faux; } } "
             "publique booléen G(constante C& objet) { retourner !objet; }", false},
            {"classe C { publique: entier32 opérateur~() { retourner 1; } }; "
             "publique entier32 G(C& objet) { retourner ~objet; }", true},
            {"classe C {}; espace C { publique entier32 opérateur+(C& objet, entier32 x) { retourner x; } } "
             "publique entier32 G(C& objet, entier32 x) { retourner objet + x; }", false},
            {"classe C {}; espace C { publique booléen opérateur!(constante C& objet) { retourner faux; } } "
             "publique booléen G(constante C& objet) { retourner !objet; }", false},
            {"classe B { publique: entier32 opérateur+(entier32 x) { retourner x; } }; classe D : publique B {}; "
             "espace B { publique entier64 opérateur+(D& objet, entier32 x) { retourner 2; } } "
             "publique entier64 G(D& objet, entier32 x) { retourner objet + x; }", false},
            {"classe B {}; classe C : publique B { publique: entier32 opérateur+(entier32 x) { retourner x; } }; "
             "espace C { publique entier32 opérateur+(B& objet, entier32 x) { retourner x; } } "
             "publique entier32 G(C& objet, entier32 x) { retourner objet + x; }", true},
            {"espace N { classe C { publique: entier32 opérateur+(entier32 x) { retourner x; } }; "
             "espace C { publique entier32 opérateur+(N::C& objet, booléen x) { retourner 2; } } } "
             "publique entier32 G(N::C& objet) { retourner objet + vrai; }", false},
            {"classe C {}; publique entier32 opérateur+(entier32 x, C& objet) { retourner x; } "
             "publique entier32 G(C& objet, entier32 x) { retourner x + objet; }", false},
            {"classe C {}; espace N { publique entier32 opérateur+(C& objet, entier32 x) { retourner x; } "
             "publique entier32 G(C& objet, entier32 x) { retourner objet + x; } }", false},
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
            for (const auto& texte : {valides[index].first, TraduireCorpusConversions(valides[index].first)})
            {
                const auto nom = "operateur-groupe-mixte-valide-" + std::to_string(index);
                const auto resultat = AnalyserSemantiqueValide(syntaxe, semantique, texte, nom);
                auto programme = GsPP::AnalyseurSyntaxique(GsPP::Lexeur(texte, nom).Analyser(), nom).Analyser();
                GsPP::AnalyseurSemantique().Analyser(programme);
                const auto appelant = std::find_if(programme.Fonctions.begin(), programme.Fonctions.end(),
                    [](const auto& fonction) { return fonction.NomSource == "G"; });
                Exiger(appelant != programme.Fonctions.end(), "appelant du corpus d'opérateur absent");
                const auto& retour = static_cast<const GsPP::InstructionRetour&>(*appelant->Corps->Instructions.front());
                const auto& nomCible = retour.Valeur->Genre == GsPP::GenreExpression::Unaire
                    ? static_cast<const GsPP::ExpressionUnaire&>(*retour.Valeur).NomSurcharge
                    : static_cast<const GsPP::ExpressionBinaire&>(*retour.Valeur).NomSurcharge;
                const auto reference = std::find_if(programme.Fonctions.begin(), programme.Fonctions.end(),
                    [&](const auto& fonction) { return fonction.NomComplet() == nomCible; });
                Exiger(reference != programme.Fonctions.end() && reference->EstMethode == valides[index].second,
                    "la cible prévue de l'opérateur ne correspond pas au bootstrap : " + nom);
                std::size_t nombre = 0;
                for (const auto& resolution : resultat.Resolutions)
                {
                    if ((resolution.Drapeaux & 256U) == 0) continue;
                    const auto& declaration = resultat.Noeuds[resultat.Symboles[resolution.IndexSymbole].IndexNoeud];
                    Exiger(declaration.Ligne == reference->Position.Ligne && declaration.Colonne == reference->Position.Colonne
                               && resolution.HachageType == HacherTypeDeclaration(reference->TypeRetour)
                               && ((resolution.Drapeaux & 32U) != 0) == reference->EstMethode,
                        "cible, retour ou drapeau de méthode de l'opérateur différent du bootstrap : " + nom);
                    ++nombre;
                }
                Exiger(nombre == 1, "résolution d'opérateur absente ou dupliquée : " + nom);
            }
    }

    /**
     * <résumé>Fixe une priorité de diagnostics indépendante des tables de hachage de l'hôte.</résumé>
     * @Paramètre(AnalyseurDeclarationsAutoHeberge: syntaxe) Analyseur des corpus bilingues.
     * @Paramètre(AnalyseurSemantiqueAutoHeberge: semantique) Frontend auto-hébergé testé.
     **/
    void TesterPrioritesGroupesInvalidesSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe,
        AnalyseurSemantiqueAutoHeberge semantique)
    {
        struct CorpusPriorite { std::string Source; std::uint32_t Code; std::size_t Ligne; };
        const std::vector<CorpusPriorite> corpus{
            {"publique vide F(entier32 a) {}\n"
             "publique vide G(entier32 a) {}\n"
             "publique vide G(entier32 b) {}\n"
             "publique vide F(entier32 b) {}", 118, 4},
            {"publique vide G(entier32 a) {}\n"
             "publique vide F(entier32 a) {}\n"
             "publique vide F(entier32 b) {}\n"
             "publique vide G(entier32 b) {}", 118, 4},
            {"publique vide F(entier32 a) {}\n"
             "publique vide G(entier32 a) {}\n"
             "publique vide F(entier64 a) {}\n"
             "publique vide G(entier32 b) {}\n"
             "publique vide F(entier32 b) {}\n"
             "publique vide F(entier64 b) {}", 118, 5},
            {"espace A { publique vide Lire(entier32 a) {} }\n"
             "espace B { publique vide Lire(entier32 a) {} }\n"
             "espace B { publique vide Lire(entier32 b) {} }\n"
             "espace A { publique vide Lire(entier32 b) {} }", 118, 4},
            {"classe C { publique: entier32 Lire(entier32 a) { retourner a; } };\n"
             "publique vide F(entier32 a) {}\n"
             "publique vide F(entier32 b) {}\n"
             "espace C { publique entier32 Lire(C& objet, entier32 b) { retourner b; } }", 118, 4},
            {"classe C {};\n"
             "publique entier32 opérateur+(C& objet, entier32 a) { retourner a; }\n"
             "publique booléen opérateur!(C& objet) { retourner vrai; }\n"
             "publique booléen opérateur!(C& autre) { retourner faux; }\n"
             "publique entier32 opérateur+(C& autre, entier32 b) { retourner b; }", 118, 5},
            {"publique vide F(entier32 a) {}\n"
             "publique vide G(entier32 a) {}\n"
             "publique vide G(entier32 b) {}\n"
             "publique vide F(entier32 b) {}\n"
             "publique vide H() { Introuvable(); }", 118, 4},
            {"publique vide F() { Introuvable(); }\n"
             "publique vide G(entier32 a) {}\n"
             "publique vide G(entier32 b) {}", 118, 3},
            {"classe C { publique: entier32 opérateur+(entier8 x) { retourner 1; } };\n"
             "espace C { publique entier32 opérateur+(C& objet, naturel8 x) { retourner 2; } }\n"
             "classe D { publique: booléen opérateur!() { retourner vrai; } };\n"
             "espace D { publique booléen opérateur!(constante D& objet) { retourner faux; } }\n"
             "publique entier32 F(C& objet) { retourner objet + 7; }\n"
             "publique booléen G(D& objet) { retourner !objet; }", 22, 5},
            {"classe C { publique: entier32 opérateur+(entier8 x) { retourner 1; } };\n"
             "espace C { publique entier32 opérateur+(C& objet, naturel8 x) { retourner 2; } }\n"
             "classe D { publique: booléen opérateur!() { retourner vrai; } };\n"
             "espace D { publique booléen opérateur!(constante D& objet) { retourner faux; } }\n"
             "publique booléen G(D& objet) { retourner !objet; }\n"
             "publique entier32 F(C& objet) { retourner objet + 7; }", 22, 5},
            {"classe C { privée: entier32 opérateur+(entier32 x) { retourner x; } };\n"
             "classe D { publique: booléen opérateur!() { retourner vrai; } };\n"
             "publique entier32 F(C& objet) { retourner objet + 7; }\n"
             "publique booléen G(constante D& objet) { retourner !objet; }", 25, 3},
            {"classe C { privée: entier32 opérateur+(entier32 x) { retourner x; } };\n"
             "classe D { publique: booléen opérateur!() { retourner vrai; } };\n"
             "publique booléen G(constante D& objet) { retourner !objet; }\n"
             "publique entier32 F(C& objet) { retourner objet + 7; }", 21, 3},
            {DeclarerTypesCollisionLiaison(NomCollisionLiaisonA, NomCollisionLiaisonB) + "\n"
             "publique vide F(" + NomCollisionLiaisonA + "* a) {}\n"
             "publique vide G(" + NomCollisionLiaisonA + "* a) {}\n"
             "publique vide G(" + NomCollisionLiaisonB + "* b) {}\n"
             "publique vide F(" + NomCollisionLiaisonB + "* b) {}", 119, 4},
            {DeclarerTypesCollisionLiaison(NomCollisionLiaisonA, NomCollisionLiaisonB) + "\n"
             "publique vide F(" + NomCollisionLiaisonA + "* a) {}\n"
             "publique vide G(entier32 a) {}\n"
             "publique vide F(" + NomCollisionLiaisonB + "* b) {}\n"
             "publique vide G(entier32 b) {}", 118, 5},
        };
        for (std::size_t index = 0; index < corpus.size(); ++index)
            for (const auto& texte : {corpus[index].Source, TraduireCorpusConversions(corpus[index].Source)})
            {
                const auto nom = "priorite-groupes-invalides-" + std::to_string(index);
                bool refuse = false;
                try
                {
                    auto programme = GsPP::AnalyseurSyntaxique(GsPP::Lexeur(texte, nom).Analyser(), nom).Analyser();
                    GsPP::AnalyseurSemantique().Analyser(programme);
                }
                catch (const GsPP::ErreurCompilation& erreur)
                {
                    refuse = true;
                    Exiger(erreur.Ligne() == corpus[index].Ligne,
                        "priorité de groupe non conforme à l'ordre source : " + nom
                        + " (attendu ligne " + std::to_string(corpus[index].Ligne)
                        + ", obtenu " + std::to_string(erreur.Ligne()) + ")");
                }
                Exiger(refuse, "corpus de priorité accepté par le bootstrap : " + nom);
                ComparerErreurSemantique(syntaxe, semantique, texte, corpus[index].Code, nom);
            }
    }

    /**
     * <résumé>Compare la première erreur entre instructions et branches d'un même corps.</résumé>
     * @Paramètre(AnalyseurDeclarationsAutoHeberge: syntaxe) Analyseur des corpus bilingues.
     * @Paramètre(AnalyseurSemantiqueAutoHeberge: semantique) Frontend auto-hébergé testé.
     **/
    void TesterPrioritesInstructionsSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe,
        AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::string declarations =
            "classe C { privée: entier32 opérateur+(entier32 x) { retourner x; } "
            "publique: booléen opérateur!() { retourner vrai; } }; "
            "publique entier32 Choisir(entier8 x) { retourner 1; } "
            "publique entier32 Choisir(naturel8 x) { retourner 2; }\n";
        struct CorpusInstruction { std::string Corps; std::uint32_t Code; std::size_t Ligne; };
        const std::vector<CorpusInstruction> refus{
            {"objet + 7;\n!autre;", 25, 3},
            {"!autre;\nobjet + 7;", 21, 3},
            {"Choisir(7);\nobjet + 7;", 22, 3},
            {"objet + 7;\nChoisir(7);", 25, 3},
            {"entier32 a = Choisir(7);\n!autre;", 22, 3},
            {"!autre;\nentier32 a = Choisir(7);", 21, 3},
            {"{\nobjet + 7;\n}\n!autre;", 25, 4},
            {"!autre;\n{\nobjet + 7;\n}", 21, 3},
            {"{\nChoisir(7);\n}\n{\n!autre;\n}", 22, 4},
            {"si (!autre) {\nobjet + 7;\n}\nChoisir(7);", 21, 3},
            {"si (vrai) {\nobjet + 7;\n} sinon {\n!autre;\n}", 25, 4},
            {"si (faux) {\n!autre;\n} sinon {\nobjet + 7;\n}", 21, 4},
            {"si (vrai) {\nsi (vrai) {\nChoisir(7);\n}\n!autre;\n}\nobjet + 7;", 22, 5},
            {"tantque (!autre) {\nobjet + 7;\n}\nChoisir(7);", 21, 3},
            {"tantque (faux) {\nobjet + 7;\n}\n!autre;", 25, 4},
            {"!autre;\ntantque (faux) {\nobjet + 7;\n}", 21, 3},
            {"retourner !autre;\nobjet + 7;", 21, 3},
            {"objet + 7;\nretourner !autre;", 25, 3},
            {"entier32 a = 0;\na = vrai;\n!autre;", 73, 4},
            {"!autre;\nentier32 a = 0;\na = vrai;", 21, 3},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
        {
            const auto source = declarations + "publique vide G(C& objet, constante C& autre) {\n"
                + refus[index].Corps + "\n}";
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
            {
                const auto nom = "priorite-instructions-refusee-" + std::to_string(index);
                bool refuse = false;
                try
                {
                    auto programme = GsPP::AnalyseurSyntaxique(GsPP::Lexeur(texte, nom).Analyser(), nom).Analyser();
                    GsPP::AnalyseurSemantique().Analyser(programme);
                }
                catch (const GsPP::ErreurCompilation& erreur)
                {
                    refuse = true;
                    Exiger(erreur.Ligne() == refus[index].Ligne,
                        "la première instruction invalide n'est pas prioritaire dans le bootstrap : " + nom
                        + " (attendu ligne " + std::to_string(refus[index].Ligne)
                        + ", obtenu " + std::to_string(erreur.Ligne()) + ")");
                }
                Exiger(refuse, "corpus de priorité accepté par le bootstrap : " + nom);
                ComparerErreurSemantique(syntaxe, semantique, texte, refus[index].Code, nom);
            }
        }
        const std::vector<std::string> valides{
            "publique entier32 G(entier32 x) { entier32 a = x; { entier32 b = a; b = x; } retourner a; }",
            "publique entier32 G(entier32 x) { si (x == 0) { retourner 1; } sinon { retourner x; } }",
            "publique entier32 G(entier32 x) { entier32 a = 0; tantque (a < x) { a = a + 1; } retourner a; }",
            "publique entier32 G(entier32 x) { entier32 a = x; si (vrai) { tantque (faux) { a = x; } } retourner a; }",
            "classe L { publique: entier32 Lire() { retourner 1; } }; "
            "publique entier32 G(L& objet) { entier32 a = objet.Lire(); { entier32 b = objet.Lire(); a = a + b; } retourner a; }",
            "publique entier32 Lire(entier32 x) { retourner x; } "
            "publique entier32 G(pointeur_fonction<entier32(entier32)> rappel) { entier32 a = rappel(7); retourner Lire(a); }",
            "classe B { publique: constructeur(entier32 x) {} }; classe D : publique B { "
            "publique: constructeur(entier32 x) : parent(x) { entier32 a = x; a = a + 1; } }; publique vide G() {}",
            "classe L { publique: entier32 Valeur; constructeur(entier32 x) : Valeur(x) { "
            "entier32 a = x; a = a + 1; } }; publique vide G() {}",
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
            for (const auto& texte : {valides[index], TraduireCorpusConversions(valides[index])})
                AnalyserSemantiqueValide(syntaxe, semantique, texte,
                    "priorite-instructions-valide-" + std::to_string(index));
    }

    /**
     * <résumé>Compare les erreurs concurrentes des opérandes, indexations et contrôles préalables.</résumé>
     * @Paramètre(AnalyseurDeclarationsAutoHeberge: syntaxe) Analyseur des corpus bilingues.
     * @Paramètre(AnalyseurSemantiqueAutoHeberge: semantique) Frontend auto-hébergé testé.
     **/
    void TesterPrioritesExpressionsSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe,
        AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::string declarations =
            "classe C { privée: entier32 opérateur+(entier32 x) { retourner x; } "
            "publique: booléen opérateur!() { retourner vrai; } }; "
            "publique entier32 Choisir(entier8 x) { retourner 1; } "
            "publique entier32 Choisir(naturel8 x) { retourner 2; } ";
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"(objet + 7) + convertir<entier32>(!autre);", 25},
            {"convertir<entier32>(!autre) + (objet + 7);", 21},
            {"Choisir(7) + (objet + 7);", 22},
            {"(objet + 7) + Choisir(7);", 25},
            {"((objet + 7) + 1) * convertir<entier32>(!autre);", 25},
            {"convertir<entier32>(!autre) * ((objet + 7) + 1);", 21},
            {"(objet + 7 == 0) && !autre;", 25},
            {"!autre || (objet + 7 == 0);", 21},
            {"objet.Absent[Choisir(7)];", 24},
            {"Inconnue[objet + 7];", 18},
            {"p[convertir<entier32>(!autre)] + (objet + 7);", 21},
            {"(objet + 7) + p[convertir<entier32>(!autre)];", 25},
            {"1 = objet + 7;", 70},
            {"x = objet + 7;", 71},
            {"entier32 tableau[2]; tableau = objet + 7;", 72},
            {"(objet + 7) = convertir<entier32>(!autre);", 25},
            {"convertir<C>(objet + 7);", 94},
            {"convertir<vide>(!autre);", 94},
            {"convertir<Introuvable*>(objet + 7);", 99},
            {"entier32 valeurs[2] = {objet + 7, convertir<entier32>(!autre)};", 25},
            {"entier32 valeurs[2] = {convertir<entier32>(!autre), objet + 7};", 21},
            {"entier32 a = (objet + 7) + convertir<entier32>(!autre);", 25},
            {"retourner (objet + 7) + convertir<entier32>(!autre);", 25},
            {"si ((objet + 7 == 0) && !autre) {}", 25},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
        {
            const auto source = declarations + "publique vide G(C& objet, constante C& autre, entier32* p, constante entier32 x) { "
                + refus[index].first + " }";
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
                ComparerErreurSemantique(syntaxe, semantique, texte, refus[index].second,
                    "priorite-expression-refusee-" + std::to_string(index));
        }
        const std::string enumeration = "énumération E { A = Premiere + Seconde }; publique vide G() {}";
        ComparerErreurSemantique(syntaxe, semantique, enumeration, 18, "priorite-enumeration-fr");
        ComparerErreurSemantique(syntaxe, semantique, TraduireCorpusConversions(enumeration), 18, "priorite-enumeration-en");
        std::string expressionProfonde = "a";
        for (std::size_t profondeur = 0; profondeur < 128; ++profondeur) expressionProfonde += " + 1";
        const std::vector<std::string> valides{
            "publique entier32 G(entier32 a) { retourner " + expressionProfonde + "; }",
            "publique entier32 G(entier32 a, entier32 b) { retourner (a + b) * (a - b); }",
            "publique entier32 G(entier32* p, entier32 x) { retourner p[x] + p[x + 1]; }",
            "publique entier32 G(entier32& x, entier32 y) { x = y + 1; retourner x; }",
            "publique entier32 Lire(entier32 x) { retourner x; } "
            "publique entier32 G(entier32 x) { retourner Lire(x + 1) + Lire(x - 1); }",
            "classe L { publique: entier32 Lire(entier32 x) { retourner x; } }; "
            "publique entier32 G(L& objet, entier32 x) { retourner objet.Lire(x + 1) + objet.Lire(x - 1); }",
            "publique entier32 G(pointeur_fonction<entier32(entier32)> rappel, entier32 x) { retourner rappel(x + 1) + rappel(x - 1); }",
            "publique entier32 G(entier32 a, entier32 b) { entier32 valeurs[2] = {a + 1, b + 1}; retourner valeurs[0] + valeurs[1]; }",
            "publique entier32 G(entier32 a) { retourner convertir<entier32>(convertir<entier64>(a)) + a; }",
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
            for (const auto& texte : {valides[index], TraduireCorpusConversions(valides[index])})
                AnalyserSemantiqueValide(syntaxe, semantique, texte,
                    "priorite-expression-valide-" + std::to_string(index));
    }

    /**
     * <résumé>Compare les priorités de cible, arité et arguments à l'intérieur des appels.</résumé>
     * @Paramètre(AnalyseurDeclarationsAutoHeberge: syntaxe) Analyseur de déclarations testé.
     * @Paramètre(AnalyseurSemantiqueAutoHeberge: semantique) Frontend auto-hébergé testé.
     **/
    void TesterPrioritesAppelsSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe,
        AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::string declarations =
            "classe C { privée: entier32 opérateur+(entier32 x) { retourner x; } "
            "publique: booléen opérateur!() { retourner vrai; } "
            "entier32 Lire(entier32 a, entier32 b) { retourner a + b; } }; "
            "publique entier32 Deux(entier32 a, entier32 b) { retourner a + b; } "
            "publique entier32 Choisir(entier8 a, entier32 b) { retourner b; } "
            "publique entier32 Choisir(naturel8 a, entier32 b) { retourner b; } ";
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"operation(objet + 7, convertir<entier32>(!autre));", 25},
            {"operation(convertir<entier32>(!autre), objet + 7);", 21},
            {"Deux(objet + 7, convertir<entier32>(!autre));", 25},
            {"Deux(convertir<entier32>(!autre), objet + 7);", 21},
            {"Choisir(objet + 7, convertir<entier32>(!autre));", 25},
            {"objet.Lire(objet + 7, convertir<entier32>(!autre));", 25},
            {"p->Lire(convertir<entier32>(!autre), objet + 7);", 21},
            {"operation((objet + 7) + convertir<entier32>(!autre), 0);", 25},
            {"Deux(0, (objet + 7) + convertir<entier32>(!autre));", 25},
            {"operation(Deux(objet + 7, convertir<entier32>(!autre)), 0);", 25},
            {"Absente(objet + 7, convertir<entier32>(!autre));", 18},
            {"x(objet + 7, convertir<entier32>(!autre));", 53},
            {"objet.Absent(objet + 7);", 24},
            {"x.Absent(objet + 7);", 23},
            {"operation(objet + 7);", 54},
            {"operation(objet + 7, 0, 0);", 54},
            {"Deux(objet + 7);", 21},
            {"Choisir(objet + 7);", 21},
            {"objet.Lire(objet + 7);", 21},
            {"autre.Lire(objet + 7, 0);", 21},
            {"operation(vrai, objet + 7);", 55},
            {"operation(objet + 7, vrai);", 25},
            {"(objet + 7)(convertir<entier32>(!autre));", 25},
            {"Absente(operation(objet + 7, 0));", 18},
            {"operation(Deux(objet + 7, 0));", 54},
            {"p->Absent(operation(objet + 7, 0));", 24},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
        {
            const auto source = declarations
                + "publique vide G(C& objet, constante C& autre, C* p, "
                  "pointeur_fonction<entier32(entier32, entier32)> operation) { entier32 x = 0; "
                + refus[index].first + " }";
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
                ComparerErreurSemantique(syntaxe, semantique, texte, refus[index].second,
                    "priorite-appel-refuse-" + std::to_string(index));
        }
        std::string appelProfond = "x";
        for (std::size_t profondeur = 0; profondeur < 4; ++profondeur)
            appelProfond = "Lire(" + appelProfond + ")";
        const std::vector<std::string> valides{
            "publique entier32 Lire(entier32 x) { retourner x; } publique entier32 G(entier32 x) { retourner " + appelProfond + "; }",
            "publique entier32 G(pointeur_fonction<entier32(entier32, entier32)> operation, entier32 x) { retourner operation(x + 1, x - 1); }",
            declarations + "publique entier32 G(C& objet, entier32 x) { retourner objet.Lire(x + 1, x - 1); }",
            declarations + "publique entier32 G(C* p, entier32 x) { retourner p->Lire(x + 1, x - 1); }",
            declarations + "publique entier32 G(entier32 x) { retourner Deux(Deux(x, 1), Deux(x, 2)); }",
            declarations + "publique entier32 G(entier8 x) { retourner Choisir(x, Deux(1, 2)); }",
            "classe L { publique: pointeur_fonction<entier32(entier32, entier32)> Operation; }; "
            "publique entier32 G(L& objet, entier32 x) { retourner objet.Operation(x + 1, x - 1); }",
            "publique entier32 G(pointeur_fonction<entier32(entier32, entier32)>* operations, entier32 x) { retourner operations[0](x + 1, x - 1); }",
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
            for (const auto& texte : {valides[index], TraduireCorpusConversions(valides[index])})
                AnalyserSemantiqueValide(syntaxe, semantique, texte,
                    "priorite-appel-valide-" + std::to_string(index));
        const std::vector<std::pair<std::string, std::uint32_t>> champsRefuses{
            {"classe L { privée: pointeur_fonction<entier32(entier32)> Operation; }; "
             "publique entier32 G(L& objet) { retourner objet.Operation(Absente); }", 25},
            {"classe L { publique: entier32 Operation; }; "
             "publique entier32 G(L& objet) { retourner objet.Operation(Absente); }", 53},
            {"classe L { publique: pointeur_fonction<entier32(entier32)> Operation; }; "
             "publique entier32 G(L& objet) { retourner objet.Operation(Absente, AutreAbsente); }", 54},
            {"publique entier32 Lire(entier32 x) { retourner x; } "
             "publique entier32 G(entier32 Lire) { retourner Lire(Absente); }", 53},
        };
        for (std::size_t index = 0; index < champsRefuses.size(); ++index)
            for (const auto& texte : {champsRefuses[index].first, TraduireCorpusConversions(champsRefuses[index].first)})
                ComparerErreurSemantique(syntaxe, semantique, texte, champsRefuses[index].second,
                    "priorite-cible-appel-refusee-" + std::to_string(index));
    }

    /**
     * <résumé>Compare l'abandon ordonné des candidats avant les arguments suivants.</résumé>
     * @Paramètre(AnalyseurDeclarationsAutoHeberge: syntaxe) Analyseur de déclarations testé.
     * @Paramètre(AnalyseurSemantiqueAutoHeberge: semantique) Frontend auto-hébergé testé.
     **/
    void TesterAbandonsCandidatsAppelsSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe,
        AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::string declarations =
            "classe C { privée: entier32 opérateur+(entier32 x) { retourner x; } "
            "publique: entier32 Lire(entier32 a, entier32 b) { retourner a + b; } }; "
            "publique entier32 Unique(entier32 a, entier32 b) { retourner a + b; } "
            "publique entier32 Choisir(entier32 a, entier32 b) { retourner b; } "
            "publique entier32 Choisir(entier64 a, entier32 b) { retourner b; } "
            "publique entier32 Reference(entier32& a, entier32 b) { retourner b; } "
            "publique entier32 Trois(entier32 a, entier32 b, entier32 c) { retourner c; } ";
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"Unique(vrai, objet + 7);", 21},
            {"Unique(objet + 7, vrai);", 25},
            {"Unique(&x, objet + 7);", 21},
            {"Choisir(vrai, objet + 7);", 21},
            {"Choisir(objet + 7, vrai);", 25},
            {"Reference(1, objet + 7);", 21},
            {"constante entier32 fixe = 0; Reference(fixe, objet + 7);", 21},
            {"Reference(x, objet + 7);", 25},
            {"objet.Lire(vrai, objet + 7);", 21},
            {"p->Lire(vrai, objet + 7);", 21},
            {"C::Lire(objet, vrai, objet + 7);", 21},
            {"Trois(x, vrai, objet + 7);", 21},
            {"Unique(vrai, Absente());", 21},
            {"Unique(0, Absente());", 18},
            {"Unique(vrai, convertir<Inconnue*>(objet + 7));", 21},
            {"Unique(0, convertir<Inconnue*>(objet + 7));", 99},
            {"Unique(vrai, convertir<vide>(objet + 7));", 21},
            {"Unique(vrai, convertir<pointeur_fonction<entier32(entier32, entier32, entier32, entier32, entier32)>>(0));", 21},
            {"convertir<Inconnue*>(0); Unique(vrai, objet + 7);", 99},
            {"(objet + 7) + convertir<Inconnue*>(0);", 25},
            {"convertir<Inconnue*>(0) + (objet + 7);", 99},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
        {
            const auto source = declarations + "publique vide G(C& objet, entier32 x, C* p) { "
                + refus[index].first + " }";
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
                ComparerErreurSemantique(syntaxe, semantique, texte, refus[index].second,
                    "abandon-candidat-refuse-" + std::to_string(index));
        }
        const std::vector<std::pair<std::string, std::uint32_t>> groupesRefuses{
            {declarations + "alias Appeler = Unique; publique vide G(C& objet) { Appeler(vrai, objet + 7); }", 21},
            {declarations + "alias Appeler = C::Lire; publique vide G(C& objet) { Appeler(objet, vrai, objet + 7); }", 21},
            {declarations + "espace C { publique entier32 Lire(C& objet, entier64 a, entier32 b) { retourner b; } } "
             "publique vide G(C& objet) { objet.Lire(vrai, objet + 7); }", 21},
            {declarations + "publique entier32 Retenir(entier32& a, entier32 b) { retourner b; } "
             "publique entier32 Retenir(booléen a, entier32 b) { retourner b; } "
             "publique vide G(C& objet) { Retenir(vrai, objet + 7); }", 25},
            {declarations + "publique entier32 Ordre(entier32 a, entier32 b) { retourner b; } "
             "publique entier32 Ordre(naturel8 a, entier32 b) { retourner b; } "
             "publique vide G(C& objet) { Ordre(1 / 0, objet + 7); }", 25},
            {declarations + "publique entier32 Ordre(naturel8 a, entier32 b) { retourner b; } "
             "publique entier32 Ordre(entier32 a, entier32 b) { retourner b; } "
             "publique vide G(C& objet) { Ordre(1 / 0, objet + 7); }", 89},
            {declarations + "publique entier32 Ordre(entier32 a, entier32 b) { retourner b; } "
             "publique entier32 Ordre(naturel8 a, entier32 b) { retourner b; } "
             "publique vide G(C& objet) { Ordre(1 / 0, 0); }", 89},
        };
        for (std::size_t index = 0; index < groupesRefuses.size(); ++index)
            for (const auto& texte : {groupesRefuses[index].first, TraduireCorpusConversions(groupesRefuses[index].first)})
                ComparerErreurSemantique(syntaxe, semantique, texte, groupesRefuses[index].second,
                    "abandon-groupe-refuse-" + std::to_string(index));
        const std::vector<std::string> valides{
            declarations + "publique entier32 G(entier32 x) { retourner Choisir(x, Unique(x, 1)); }",
            declarations + "publique entier32 G(C& objet, entier32 x) { retourner objet.Lire(x, Unique(x, 1)); }",
            declarations + "publique entier32 G(C* p, entier32 x) { retourner p->Lire(x, Unique(x, 1)); }",
            declarations + "publique entier32 Retenir(entier32& a, entier32 b) { retourner b; } "
            "publique entier32 Retenir(booléen a, entier32 b) { retourner b; } "
            "publique entier32 G() { retourner Retenir(vrai, Unique(1, 2)); }",
            declarations + "alias Appeler = C::Lire; publique entier32 G(C& objet, entier32 x) { retourner Appeler(objet, x, Unique(x, 1)); }",
            declarations + "publique entier32 Ordre(entier32 a, entier32 b) { retourner b; } "
            "publique entier32 Ordre(naturel8 a, entier32 b) { retourner b; } "
            "publique entier32 G(entier32 x) { retourner Ordre(x, Unique(x, 1)); }",
            declarations + "espace C { publique entier32 Lire(C& objet, booléen a, entier32 b) { retourner b; } } "
            "publique entier32 G(C& objet, entier32 x) { retourner objet.Lire(vrai, Unique(x, 1)); }",
            declarations + "alias AliasC = C; publique entier32 Accepter(AliasC& a, entier32 b) { retourner b; } "
            "publique entier32 Accepter(booléen a, entier32 b) { retourner b; } "
            "publique entier32 G() { retourner Accepter(vrai, Unique(1, 2)); }",
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
            for (const auto& texte : {valides[index], TraduireCorpusConversions(valides[index])})
                AnalyserSemantiqueValide(syntaxe, semantique, texte,
                    "abandon-candidat-valide-" + std::to_string(index));
    }

    /**
     * <résumé>Compare les arguments agrégés dans le contexte de la signature retenue.</résumé>
     * @Paramètre(AnalyseurDeclarationsAutoHeberge: syntaxe) Analyseur de déclarations testé.
     * @Paramètre(AnalyseurSemantiqueAutoHeberge: semantique) Frontend auto-hébergé testé.
     **/
    void TesterArgumentsContextuelsAppelsSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe,
        AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::string declarations =
            "structure Point { entier32 X; entier32 Y; }; "
            "structure Bloc { Point P; naturel8 Octets[2]; }; "
            "union Choix { entier32 X; entier64 Y; }; "
            "classe C { privée: entier32 opérateur+(entier32 x) { retourner x; } "
            "vide Prive(entier32 x) {} publique: vide Lire(entier32 x, entier32 y) {} }; "
            "publique vide Scalaire(entier32 a, entier32 b) {} "
            "publique vide Petit(naturel8 a) {} "
            "publique vide Agrege(Point p, entier32 x) {} "
            "publique vide Imbrique(Bloc p) {} "
            "publique vide Union(Choix p) {} "
            "publique vide Reference(entier32& x) {} "
            "publique vide Ambigu(entier32 x) {} "
            "publique vide Ambigu(entier64 x) {} "
            "publique entier32 Identite(entier32 x) { retourner x; } "
            "alias Appeler = C::Lire; ";
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"Scalaire({1, 2}, 0);", 44},
            {"Scalaire({Absente, 2}, 0);", 44},
            {"Scalaire({vrai}, 0);", 45},
            {"Scalaire({Absente}, objet + 7);", 25},
            {"Scalaire({Absente}, vrai);", 21},
            {"Scalaire({Absente}, 0, 0);", 21},
            {"Reference({Absente});", 21},
            {"Ambigu({Absente});", 22},
            {"Petit({300});", 90},
            {"Agrege({Absente, 2, 3}, 0);", 43},
            {"Agrege({vrai, Absente}, 0);", 45},
            {"Agrege({1, objet + 7}, 0);", 25},
            {"Imbrique({{1, 2}, {Absente, 2, 3}});", 42},
            {"Imbrique({{1, 2}, {1, 300}});", 90},
            {"Union({Absente, 2});", 43},
            {"objet.Prive({Absente});", 25},
            {"objet.Lire({Absente}, vrai);", 21},
            {"p->Lire({Absente}, 0);", 18},
            {"pointeur_fonction<vide(entier32, entier32)> f = Scalaire; f({Absente, 2}, objet + 7);", 44},
            {"pointeur_fonction<vide(entier32, entier32)> f = Scalaire; f({vrai}, objet + 7);", 45},
            {"pointeur_fonction<vide(Point, entier32)> f = Agrege; f({Absente, 2, 3}, objet + 7);", 43},
            {"pointeur_fonction<vide(Point, entier32)> f = Agrege; f({vrai, Absente}, objet + 7);", 45},
            {"pointeur_fonction<vide(entier32, entier32)> f = Scalaire; f({Absente}, objet + 7);", 18},
            {"Scalaire({convertir<Inconnue*>(0)}, objet + 7);", 25},
            {"Petit({1 / 0});", 89},
            {"pointeur_fonction<vide(entier32&)> f = Reference; f({Absente});", 69},
            {"(&Scalaire)({Absente, 2}, objet + 7);", 44},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
        {
            const auto source = declarations + "publique vide G(C& objet, C* p) { " + refus[index].first + " }";
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
            {
                const auto nom = "argument-contextuel-refuse-" + std::to_string(index);
                try
                {
                    ComparerErreurSemantique(syntaxe, semantique, texte, refus[index].second, nom);
                }
                catch (const std::exception& erreur)
                {
                    throw std::runtime_error(nom + " : " + erreur.what());
                }
            }
        }
        const std::vector<std::string> valides{
            "Scalaire({}, {{7}});",
            "Scalaire({1 / 0}, 0);",
            "Agrege({1, {2}}, 0);",
            "Imbrique({{1, 2}, {3, 4}});",
            "Union({7}); Union({});",
            "objet.Lire({1}, {{2}}); p->Lire({}, {3});",
            "C::Lire(objet, {1}, {2});",
            "Appeler(objet, {1}, {2});",
            "pointeur_fonction<vide(entier32, entier32)> f = Scalaire; f({1}, {{2}});",
            "pointeur_fonction<vide(Point, entier32)> f = Agrege; f({{1}, 2}, {});",
            "pointeur_fonction<vide(Bloc)> f = Imbrique; f({{1, 2}, {3, 4}});",
            "(&Scalaire)({1}, {2});",
            "Scalaire({Identite({3})}, {Identite({4})});",
            "pointeur_fonction<vide(Point, entier32)> f[2] = {Agrege, Agrege}; f[1]({1, 2}, {});",
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
        {
            const auto source = declarations + "publique vide G(C& objet, C* p) { " + valides[index] + " }";
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
            {
                const auto nom = "argument-contextuel-valide-" + std::to_string(index);
                try
                {
                    AnalyserSemantiqueValide(syntaxe, semantique, texte, nom);
                }
                catch (const std::exception& erreur)
                {
                    throw std::runtime_error(nom + " : " + erreur.what());
                }
            }
        }
        const std::string groupeMixte =
            "classe C { publique: vide Lire(entier32 x, entier32 y) {} }; "
            "espace C { publique vide Lire(C& objet, entier32 x, booléen y) {} } ";
        const std::vector<std::pair<std::string, std::uint32_t>> groupesRefuses{
            {groupeMixte + "publique vide G(C& objet) { objet.Lire({Absente, 2}, vrai); }", 44},
            {groupeMixte + "publique vide G(C* objet) { objet->Lire({Absente, 2}, vrai); }", 44},
            {"structure P { entier32 X; }; publique vide Choisir(P x) {} publique vide Choisir(entier32 x) {} "
             "publique vide G() { Choisir({Absente}); }", 22},
            {"structure P { entier32 X; }; publique vide Choisir(P& x) {} publique vide Choisir(P x) {} "
             "publique vide G() { Choisir({Absente, 2}); }", 43},
            {"espace N { structure P { entier32 X; }; alias Vue = P; publique vide Lire(Vue x) {} } "
             "publique vide G() { N::Lire({vrai}); }", 45},
            {"classe C {}; publique vide Lire(C x) {} publique vide G() { Lire({Absente}); }", 29},
        };
        for (std::size_t index = 0; index < groupesRefuses.size(); ++index)
            for (const auto& texte : {groupesRefuses[index].first, TraduireCorpusConversions(groupesRefuses[index].first)})
                ComparerErreurSemantique(syntaxe, semantique, texte, groupesRefuses[index].second,
                    "groupe-contextuel-refuse-" + std::to_string(index));
        const std::vector<std::string> groupesValides{
            groupeMixte + "publique vide G(C& objet) { objet.Lire({3}, vrai); }",
            groupeMixte + "publique vide G(C* objet) { objet->Lire({3}, vrai); }",
            "espace N { structure P { entier32 X; }; alias Vue = P; publique vide Lire(Vue x) {} } "
            "publique vide G() { N::Lire({{3}}); }",
            "structure P { entier32 X; }; publique vide Lire(P x, entier32 y) {} "
            "structure Rappels { pointeur_fonction<vide(P, entier32)> Champ; }; "
            "publique vide G() { Rappels r = {Lire}; r.Champ({{3}}, {4}); }",
        };
        for (std::size_t index = 0; index < groupesValides.size(); ++index)
            for (const auto& texte : {groupesValides[index], TraduireCorpusConversions(groupesValides[index])})
                AnalyserSemantiqueValide(syntaxe, semantique, texte,
                    "groupe-contextuel-valide-" + std::to_string(index));
    }

    /**
     * <résumé>Compare la priorité des calculs et plages des conversions constantes.</résumé>
     * @Paramètre(AnalyseurDeclarationsAutoHeberge: syntaxe) Analyseur de déclarations testé.
     * @Paramètre(AnalyseurSemantiqueAutoHeberge: semantique) Frontend auto-hébergé testé.
     **/
    void TesterPrioritesConversionsConstantesSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe,
        AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::string declarations =
            "structure Point { entier32 X; }; "
            "classe C { privée: entier32 opérateur+(entier32 x) { retourner x; } }; "
            "publique vide Lire(naturel8 x, entier32 y) {} "
            "publique vide Scalaire(entier32 x, entier32 y) {} "
            "publique vide Agrege(Point x, entier32 y) {} ";
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"convertir<naturel8>(256); objet + 7;", 98},
            {"objet + 7; convertir<naturel8>(256);", 25},
            {"convertir<entier32>(1 / 0); objet + 7;", 89},
            {"objet + 7; convertir<entier32>(1 / 0);", 25},
            {"convertir<naturel8>(256) + (objet + 7);", 98},
            {"(objet + 7) + convertir<naturel8>(256);", 25},
            {"convertir<entier32>(convertir<naturel8>(256)); objet + 7;", 98},
            {"convertir<naturel8>(convertir<entier32>(1 / 0)); objet + 7;", 89},
            {"Lire(convertir<naturel8>(256), objet + 7);", 98},
            {"Scalaire(vrai, convertir<naturel8>(256));", 21},
            {"Lire(convertir<naturel8>(256));", 21},
            {"Absente(convertir<naturel8>(256));", 18},
            {"pointeur_fonction<vide(naturel8, entier32)> f = Lire; "
             "f(convertir<naturel8>(256), objet + 7);", 98},
            {"(&Lire)(convertir<naturel8>(256), objet + 7);", 98},
            {"Agrege({convertir<naturel8>(256)}, objet + 7);", 25},
            {"pointeur_fonction<vide(Point, entier32)> f = Agrege; "
             "f({convertir<naturel8>(256)}, objet + 7);", 98},
            {"convertir<Inconnue*>(objet + 7); convertir<naturel8>(256);", 99},
            {"convertir<Point>(convertir<naturel8>(256));", 94},
            {"convertir<naturel8>(objet + 7); convertir<naturel8>(256);", 25},
            {"naturel8 valeur = convertir<naturel8>(256); objet + 7;", 98},
            {"si (convertir<booléen>(1 / 0)) { objet + 7; }", 89},
            {"tantque (convertir<booléen>(1 / 0)) { objet + 7; }", 89},
            {"{ naturel8 i = convertir<naturel8>(256); } objet + 7;", 98},
            {"1 = objet + 7; convertir<naturel8>(256);", 70},
            {"Absente; convertir<naturel8>(256);", 18},
            {"convertir<naturel8>(-1); objet + 7;", 98},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
        {
            const auto source = declarations + "publique vide G(C& objet) { " + refus[index].first + " }";
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
            {
                const auto nom = "priorite-conversion-constante-refuse-" + std::to_string(index);
                try
                {
                    ComparerErreurSemantique(syntaxe, semantique, texte, refus[index].second, nom);
                }
                catch (const std::exception& erreur)
                {
                    throw std::runtime_error(nom + " : " + erreur.what());
                }
            }
        }
        const std::vector<std::pair<std::string, std::uint32_t>> declarationsRefusees{
            {declarations + "publique naturel8 G() { retourner convertir<naturel8>(256); } "
             "publique vide H(C& objet) { objet + 7; }", 98},
            {declarations + "publique vide G(C& objet) { objet + 7; } "
             "publique naturel8 H() { retourner convertir<naturel8>(256); }", 25},
            {declarations + "naturel8 Valeur = convertir<naturel8>(256); "
             "publique vide G(C& objet) { objet + 7; }", 98},
            {declarations + "classe S { naturel8 X = convertir<naturel8>(256); "
             "publique: constructeur() {} }; "
             "publique vide G(C& objet) { objet + 7; }", 98},
            {declarations + "énumération E { X = convertir<naturel8>(256) }; "
             "publique vide G(C& objet) { objet + 7; }", 98},
            {declarations + "énumération E { X = convertir<entier32>(1 / 0) }; "
             "publique vide G(C& objet) { objet + 7; }", 89},
        };
        for (std::size_t index = 0; index < declarationsRefusees.size(); ++index)
            for (const auto& texte : {declarationsRefusees[index].first,
                                     TraduireCorpusConversions(declarationsRefusees[index].first)})
                ComparerErreurSemantique(syntaxe, semantique, texte, declarationsRefusees[index].second,
                    "priorite-conversion-declaration-refuse-" + std::to_string(index));
        const std::vector<std::string> valides{
            "Lire(convertir<naturel8>(255), 0);",
            "Lire(convertir<naturel8>(0), 0);",
            "convertir<entier8>(-128); convertir<entier8>(127);",
            "convertir<booléen>(300);",
            "convertir<naturel8>(faux && (1 / 0));",
            "convertir<naturel8>(vrai || (1 / 0));",
            "convertir<naturel8>(convertir<entier32>(255));",
            "pointeur_fonction<vide(naturel8, entier32)> f = Lire; f(convertir<naturel8>(255), 0);",
            "Agrege({convertir<naturel8>(255)}, 0);",
            "pointeur_fonction<vide(Point, entier32)> f = Agrege; f({convertir<naturel8>(255)}, 0);",
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
        {
            const auto source = declarations + "publique vide G(C& objet) { " + valides[index] + " }";
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
                AnalyserSemantiqueValide(syntaxe, semantique, texte,
                    "priorite-conversion-constante-valide-" + std::to_string(index));
        }
        const std::vector<std::string> declarationsValides{
            "publique naturel8 G(entier32 valeur) { retourner convertir<naturel8>(valeur); }",
            "énumération E { X = convertir<naturel8>(255) }; "
            "naturel8 Valeur = convertir<naturel8>(E::X); publique vide G() {}",
        };
        for (std::size_t index = 0; index < declarationsValides.size(); ++index)
            for (const auto& texte : {declarationsValides[index], TraduireCorpusConversions(declarationsValides[index])})
                AnalyserSemantiqueValide(syntaxe, semantique, texte,
                    "conversion-declaration-valide-" + std::to_string(index));
    }

    /**
     * <résumé>Vérifie de vraies collisions d'empreintes de liaison, sans modifier le bootstrap ni l'AST public.</résumé>
     * @Paramètre(AnalyseurDeclarationsAutoHeberge: syntaxe) Analyseur de déclarations testé.
     * @Paramètre(AnalyseurSemantiqueAutoHeberge: semantique) Frontend auto-hébergé testé.
     **/
    void TesterCollisionsNomsLiaisonSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe,
        AnalyseurSemantiqueAutoHeberge semantique)
    {
        const auto verifierEmpreinte = [](const std::string& a, const std::string& b,
                                         std::string_view avant, std::string_view apres)
        {
            const auto calculer = [](const std::string& texte)
            {
                std::uint64_t empreinte = 1469598103934665603ULL;
                for (const unsigned char octet : texte)
                {
                    empreinte ^= octet;
                    empreinte *= 1099511628211ULL;
                }
                return empreinte;
            };
            Exiger(a != b && HacherTexte(a) != HacherTexte(b)
                       && calculer(std::string(avant) + a + std::string(apres))
                           == calculer(std::string(avant) + b + std::string(apres)),
                "le corpus n'expose pas une collision réelle de liaison entre deux types distincts");
        };
        verifierEmpreinte(NomCollisionLiaisonA, NomCollisionLiaisonB, "", "*;");
        verifierEmpreinte(NomCollisionRecepteurA, NomCollisionRecepteurB, "C&;", "*;");
        verifierEmpreinte(NomCollisionCallbackA, NomCollisionCallbackB, "pointeur_fonction<vide(", "*)>;");
        verifierEmpreinte(NomCollisionConstanteA, NomCollisionConstanteB, "constante ", "*;");
        verifierEmpreinte(NomCollisionEspaceA, NomCollisionEspaceB, "N::", "*;");
        verifierEmpreinte(NomCollisionQualifieA, NomCollisionQualifieB, "A::B::", "*;");
        verifierEmpreinte(NomCollisionMethodeQualifieA, NomCollisionMethodeQualifieB, "N::C&;N::", "*;");
        verifierEmpreinte(NomCollisionUtf8A, NomCollisionUtf8B, "Étage::", "*;");
        const auto& a = NomCollisionLiaisonA;
        const auto& b = NomCollisionLiaisonB;
        const auto& ma = NomCollisionRecepteurA;
        const auto& mb = NomCollisionRecepteurB;
        const auto& ca = NomCollisionCallbackA;
        const auto& cb = NomCollisionCallbackB;
        const auto& qa = NomCollisionConstanteA;
        const auto& qb = NomCollisionConstanteB;
        const auto types = DeclarerTypesCollisionLiaison(a, b);
        const auto methodes = DeclarerTypesCollisionLiaison(ma, mb);
        const auto callbacks = DeclarerTypesCollisionLiaison(ca, cb);
        const auto qualifies = DeclarerTypesCollisionLiaison(qa, qb);
        const auto paire = "publique vide F(" + a + "* x) {} publique vide F(" + b + "* y) {}";
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {types + paire, 119},
            {types + "publique vide F(" + b + "* x) {} publique vide F(" + a + "* y) {}", 119},
            {paire + types, 119},
            {types + paire + " publique vide F(booléen x) {}", 119},
            {types + "publique entier32 F(" + a + "* x) { retourner 1; } "
             "publique entier64 F(" + b + "* y) { retourner 2; }", 119},
            {types + "publique vide F(" + a + "**** x) {} publique vide F(" + b + "**** y) {}", 119},
            {types + "publique vide F(" + a + "& x) {} publique vide F(" + b + "& y) {}", 119},
            {types + "publique vide F(" + a + " x) {} publique vide F(" + b + " y) {}", 119},
            {"énumération " + a + " { X }; énumération " + b + " { Y }; "
             "publique vide F(" + a + " x) {} publique vide F(" + b + " y) {}", 119},
            {types + "alias VueA = " + a + "; alias VueB = Copie; alias Copie = " + b + "; "
             "publique vide F(VueA* x) {} publique vide F(VueB* y) {}", 119},
            {types + "espace N { alias VueA = " + a + "; alias VueB = " + b + "; "
             "publique vide F(VueA* x) {} publique vide F(VueB* y) {} }", 119},
            {types + "publique vide F(\n" + a + " /* A */ * x\n) {}\n"
             "publique vide F(\n" + b + " /* B */ * y\n) {}", 119},
            {types + "publique vide F(" + a + "* x, entier32 y) {} "
             "publique vide F(" + b + "* x, entier32 y) {}", 119},
            {callbacks + "publique vide F(pointeur_fonction<vide(" + ca + "*)> x) {} "
             "publique vide F(pointeur_fonction<vide(" + cb + "*)> y) {}", 119},
            {qualifies + "publique vide F(constante " + qa + "* x) {} "
             "publique vide F(constante " + qb + "* y) {}", 119},
            {methodes + "classe C { publique: vide F(" + ma + "* x) {} vide F(" + mb + "* y) {} };", 119},
            {methodes + "classe C { publique: constructeur(" + ma + "* x) {} constructeur(" + mb + "* y) {} };", 119},
            {types + "publique entier32 opérateur+(" + a + "& x, entier32 y) { retourner y; } "
             "publique entier32 opérateur+(" + b + "& x, entier32 y) { retourner y; }", 119},
            {methodes + "classe Base { publique: virtuel vide F(" + ma + "* x) {} }; "
             "classe C : publique Base { publique: vide F(" + ma + "* x) {} vide F(" + mb + "* y) {} };", 119},
            {types + "entier32 Globale = Introuvable(); " + paire, 119},
            {types + "publique vide F(" + a + "* x) { Introuvable(); } publique vide F(" + b + "* y) {}", 119},
            {types + "publique vide F(" + a + "* x) {}\npublique vide F(" + b + "* x, booléen y) {}\n"
             "publique vide F(" + a + "* x, booléen y) {}\npublique vide F(" + b + "* x) {}", 119},
            {types + paire + " publique vide F(" + a + "* z) {}", 118},
            {types + paire + " publique vide G(Introuvable x) {}", 100},
            {types + paire + " structure S { S X; };", 57},
            {types + paire + " alias Appeler = F;", 112},
            {"espace N { " + DeclarerTypesCollisionLiaison(NomCollisionEspaceA, NomCollisionEspaceB)
             + "publique vide F(" + NomCollisionEspaceA + "* x) {} "
               "publique vide F(N::" + NomCollisionEspaceB + "* y) {} }", 119},
            {"espace A { espace B { " + DeclarerTypesCollisionLiaison(NomCollisionQualifieA, NomCollisionQualifieB)
             + "} } publique vide F(A::B::" + NomCollisionQualifieA + "* x) {} "
               "publique vide F(A::B::" + NomCollisionQualifieB + "* y) {}", 119},
            {"espace N { " + DeclarerTypesCollisionLiaison(NomCollisionMethodeQualifieA, NomCollisionMethodeQualifieB)
             + "classe C { publique: vide F(N::" + NomCollisionMethodeQualifieA + "* x) {} "
               "vide F(N::" + NomCollisionMethodeQualifieB + "* y) {} }; }", 119},
            {"espace Étage { " + DeclarerTypesCollisionLiaison(NomCollisionUtf8A, NomCollisionUtf8B)
             + "} publique vide F(Étage::" + NomCollisionUtf8A + "* x) {} "
               "publique vide F(Étage::" + NomCollisionUtf8B + "* y) {}", 119},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
        {
            const auto& [texte, code] = refus[index];
            ComparerErreurSemantique(syntaxe, semantique, texte, code,
                "collision-nom-liaison-fr-" + std::to_string(index));
            ComparerErreurSemantique(syntaxe, semantique, TraduireCorpusConversions(texte), code,
                "collision-nom-liaison-en-" + std::to_string(index));
        }
        const std::vector<std::string> valides{
            types + "publique vide F(" + a + "* x) {} publique vide G(" + b + "* y) {}",
            types + "espace A { publique vide F(" + a + "* x) {} } espace B { publique vide F(" + b + "* y) {} }",
            types + "publique vide F(" + a + "* x) {} publique vide F(" + b + "** y) {}",
            types + "publique vide F(" + a + "& x) {} publique vide F(" + b + "* y) {}",
            "publique vide F(entier32 x) {} publique vide F(entier64 y) {}",
            "publique vide F(constante entier32* x) {} publique vide F(volatile entier32* y) {}",
            "espace N { structure S {}; } alias Vue = N::S; "
            "publique vide F(constante Vue** x) {} publique vide F(N::S& y) {}",
            "espace A::B { structure S {}; } espace N { alias Vue = A::B::S; "
            "publique vide F(Vue* x) {} publique vide F(entier32 x) {} }",
            "publique vide F(pointeur_fonction<pointeur_fonction<entier32(constante entier32*)>(entier64&)> x) {} "
            "publique vide F(pointeur_fonction<pointeur_fonction<entier64(constante entier32*)>(entier64&)> y) {}",
            "classe C { publique: constructeur() {} constructeur(entier32 x) {} vide F() {} vide F(entier32 x) {} };",
            "publique vide F() {} publique vide F(booléen x) {}",
            "publique vide F(caractère x) {} publique vide F(octet x) {} publique vide F(naturel8 x) {}",
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
        {
            AnalyserSemantiqueValide(syntaxe, semantique, valides[index],
                "nom-liaison-distinct-fr-" + std::to_string(index));
            AnalyserSemantiqueValide(syntaxe, semantique, TraduireCorpusConversions(valides[index]),
                "nom-liaison-distinct-en-" + std::to_string(index));
        }
    }

    void TesterRemplacementsVirtuelsSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe,
        AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"classe C { publique: remplacer entier32 Lire() { retourner 1; } };", 116},
            {"classe B { publique: entier32 Lire() { retourner 1; } }; classe D : publique B { "
             "publique: remplacer entier32 Lire() { retourner 2; } };", 116},
            {"classe B { publique: virtuel entier32 Lire() { retourner 1; } }; classe D : publique B { "
             "publique: remplacer entier64 Lire() { retourner 2; } };", 116},
            {"classe B { publique: virtuel entier32 Lire(entier32 x) { retourner x; } }; classe D : publique B { "
             "publique: remplacer entier32 Lire(entier64 x) { retourner 2; } };", 116},
            {"classe B { publique: virtuel entier32 Lire(entier32 x) { retourner x; } }; classe D : publique B { "
             "publique: remplacer entier32 Lire() { retourner 2; } };", 116},
            {"classe B { publique: virtuel entier32 Lire() { retourner 1; } }; classe D : publique B { "
             "publique: remplacer entier32 Autre() { retourner 2; } };", 116},
            {"classe B { publique: virtuel entier32 Lire(entier32 x) { retourner x; } }; classe D : publique B { "
             "publique: remplacer entier32 Lire(constante entier32 x) { retourner x; } };", 116},
            {"classe B { publique: virtuel entier32 Lire(entier32* x) { retourner 1; } }; classe D : publique B { "
             "publique: remplacer entier32 Lire(entier32** x) { retourner 2; } };", 116},
            {"classe B { publique: virtuel entier32 Lire(entier32& x) { retourner x; } }; classe D : publique B { "
             "publique: remplacer entier32 Lire(entier32 x) { retourner x; } };", 116},
            {"classe B { publique: virtuel entier32 Lire() { retourner 1; } }; classe D : publique B { "
             "publique: entier32 Lire() { retourner 2; } };", 117},
            {"classe B { publique: virtuel entier32 Lire() { retourner 1; } }; classe D : publique B { "
             "publique: virtuel entier32 Lire() { retourner 2; } };", 117},
            {"classe C { publique: remplacer destructeur() {} };", 116},
            {"classe B { publique: destructeur() {} }; classe D : publique B { "
             "publique: remplacer destructeur() {} };", 116},
            {"classe B { publique: virtuel destructeur() {} }; classe D : publique B { "
             "publique: destructeur() {} };", 117},
            {"classe B { publique: virtuel destructeur() {} }; classe D : publique B { "
             "publique: virtuel destructeur() {} };", 117},
            {"classe C { publique: remplacer entier32 opérateur+(entier32 x) { retourner x; } };", 116},
            {"classe B { publique: virtuel entier32 opérateur+(entier32 x) { retourner x; } }; classe D : publique B { "
             "publique: entier32 opérateur+(entier32 x) { retourner x; } };", 117},
            {"classe A { publique: virtuel entier32 Lire() { retourner 1; } }; classe B : publique A { "
             "publique: remplacer entier32 Lire() { retourner 2; } }; classe C : publique B { "
             "publique: entier32 Lire() { retourner 3; } };", 117},
            {"classe A { publique: virtuel entier32 Lire() { retourner 1; } }; classe B {}; classe D : publique B { "
             "publique: remplacer entier32 Lire() { retourner 2; } };", 116},
            {"espace A { structure S {}; } espace N { structure S {}; classe B { publique: "
             "virtuel entier32 Lire(A::S* x) { retourner 1; } }; classe D : publique B { publique: "
             "remplacer entier32 Lire(N::S* x) { retourner 2; } }; }", 116},
            {"classe B { publique: virtuel entier32 Lire(pointeur_fonction<entier32(entier32)> x) { retourner 1; } }; "
             "classe D : publique B { publique: remplacer entier32 Lire(pointeur_fonction<entier32(entier64)> x) "
             "{ retourner 2; } };", 116},
            {"classe B { privée: virtuel entier32 Lire() { retourner 1; } }; classe D : publique B { "
             "privée: entier32 Lire() { retourner 2; } };", 117},
            {"classe B { publique: virtuel entier32 Lire() { retourner 1; } }; alias Vue = B; classe D : publique Vue { "
             "publique: entier32 Lire() { retourner 2; } };", 117},
            {"classe D : publique B { publique: remplacer entier32 Lire() { retourner 1; } }; "
             "classe B { publique: remplacer entier32 Autre() { retourner 2; } };", 116},
            {"classe C {\n publique: remplacer entier32 Lire() { retourner 1; }\n};\n"
             "classe B { publique: remplacer entier32 Autre() { retourner 2; } };", 116},
            {"classe C { publique: remplacer entier32 Lire(Inconnu x) { retourner 1; } };", 100},
            {"classe C { publique: remplacer entier32 Lire(entier32 a, entier32 b, entier32 c, entier32 d) "
             "{ retourner 1; } };", 106},
            {"classe C { publique: remplacer entier32 opérateur+(entier32 x, entier32 y) { retourner x; } };", 59},
            {"classe C { C Champ; publique: remplacer entier32 Lire() { retourner 1; } };", 57},
            {"classe C { alias Vue = Absente; publique: remplacer entier32 Lire() { retourner 1; } };", 108},
            {"classe C { publique: remplacer entier32 Lire() { retourner Absente; } };", 116},
            {"classe C { publique: remplacer entier32 Lire() { retourner 1; } }; C Globale;", 116}};
        for (std::size_t index = 0; index < refus.size(); ++index)
        {
            const auto& [texte, code] = refus[index];
            ComparerErreurSemantique(syntaxe, semantique, texte, code,
                "remplacement-refus-fr-" + std::to_string(index));
            ComparerErreurSemantique(syntaxe, semantique, TraduireCorpusConversions(texte), code,
                "remplacement-refus-en-" + std::to_string(index));
        }
        const std::vector<std::string> valides{
            "classe B { publique: virtuel entier32 Lire() { retourner 1; } }; classe D : publique B { "
            "publique: remplacer entier32 Lire() { retourner 2; } };",
            "classe D : publique B { publique: remplacer entier32 Lire() { retourner 2; } }; "
            "classe B { publique: virtuel entier32 Lire() { retourner 1; } };",
            "classe A { publique: virtuel entier32 Lire() { retourner 1; } }; classe B : publique A {}; "
            "classe C : publique B { publique: remplacer entier32 Lire() { retourner 2; } };",
            "classe A { publique: virtuel entier32 Lire() { retourner 1; } }; classe B : publique A { "
            "publique: remplacer entier32 Lire() { retourner 2; } }; classe C : publique B { "
            "publique: remplacer entier32 Lire() { retourner 3; } };",
            "classe B { publique: virtuel entier32 Lire(entier32 x) { retourner x; } "
            "virtuel entier64 Lire(entier64 x) { retourner x; } }; classe D : publique B { "
            "publique: remplacer entier32 Lire(entier32 x) { retourner x; } };",
            "classe B { publique: virtuel entier32 Lire() { retourner 1; } }; classe D : publique B { "
            "publique: entier64 Lire() { retourner 2; } };",
            "classe B { publique: virtuel entier32 Lire(entier32 x) { retourner x; } }; classe D : publique B { "
            "publique: entier32 Lire(entier64 x) { retourner 2; } };",
            "classe B { privée: virtuel entier32 Lire() { retourner 1; } }; classe D : publique B { "
            "publique: remplacer entier32 Lire() { retourner 2; } };",
            "classe B { publique: virtuel entier32 Lire() { retourner 1; } }; classe D : publique B { "
            "privée: remplacer entier32 Lire() { retourner 2; } };",
            "espace N { structure S {}; alias Vue = S; classe B { publique: virtuel entier32 Lire(N::S* x) "
            "{ retourner 1; } }; classe D : publique B { publique: remplacer entier32 Lire(N::Vue* x) "
            "{ retourner 2; } }; }",
            "classe B { publique: virtuel entier32 Lire(pointeur_fonction<entier32(entier32)> x) { retourner 1; } }; "
            "classe D : publique B { publique: remplacer entier32 Lire(pointeur_fonction<entier32(entier32)> x) "
            "{ retourner 2; } };",
            "classe B { publique: virtuel entier32 Lire(constante entier32& x) { retourner x; } }; classe D : publique B { "
            "publique: remplacer entier32 Lire(constante entier32& x) { retourner x; } };",
            "classe B { publique: virtuel entier32 Lire() { retourner 1; } }; alias Vue = B; alias Copie = Vue; "
            "classe D : publique Copie { publique: remplacer entier32 Lire() { retourner 2; } };",
            "classe B { publique: virtuel entier32 Lire() { retourner 1; } }; classe D : publique B { "
            "publique: virtuel remplacer entier32 Lire() { retourner 2; } };",
            "classe B { publique: virtuel destructeur() {} }; classe D : publique B { "
            "publique: remplacer destructeur() {} };",
            "classe B { publique: virtuel entier32 opérateur+(entier32 x) { retourner x; } }; classe D : publique B { "
            "publique: remplacer entier32 opérateur+(entier32 x) { retourner x; } };",
            "classe B { publique: virtuel booléen opérateur!() { retourner vrai; } }; classe D : publique B { "
            "publique: remplacer booléen opérateur!() { retourner faux; } };",
            "classe A { publique: virtuel entier32 Lire() { retourner 1; } }; classe B : publique A { "
            "publique: entier64 Lire() { retourner 2; } }; classe C : publique B { "
            "publique: remplacer entier32 Lire() { retourner 3; } };",
            "structure S { entier32 X; }; classe B { publique: virtuel S Lire(entier32 x) { retourner {x}; } }; "
            "classe D : publique B { publique: remplacer S Lire(entier32 x) { retourner {x}; } };",
            "classe B { publique: virtuel entier32 Lire() { retourner 1; } }; "
            "classe C { publique: virtuel entier32 Lire() { retourner 2; } }; "
            "classe D : publique B { publique: remplacer entier32 Lire() { retourner 3; } };"};
        for (std::size_t index = 0; index < valides.size(); ++index)
        {
            AnalyserSemantiqueValide(syntaxe, semantique, valides[index],
                "remplacement-valide-fr-" + std::to_string(index));
            AnalyserSemantiqueValide(syntaxe, semantique, TraduireCorpusConversions(valides[index]),
                "remplacement-valide-en-" + std::to_string(index));
        }
        const auto verifierDisposition = [&](const std::string& source, std::string_view nomClasse)
        {
            auto programme = GsPP::AnalyseurSyntaxique(
                GsPP::Lexeur(source, "disposition-virtuelle").Analyser(), "disposition-virtuelle").Analyser();
            GsPP::AnalyseurSemantique().Analyser(programme);
            const auto type = std::find_if(programme.Structures.begin(), programme.Structures.end(),
                [&](const auto& structure) { return structure.Nom == nomClasse; });
            Exiger(type != programme.Structures.end() && type->EstPolymorphe,
                "classe polymorphe absente du bootstrap");
            const auto resultat = AnalyserSemantiqueValide(syntaxe, semantique, source, "disposition-virtuelle");
            const auto variable = std::find_if(resultat.Noeuds.begin(), resultat.Noeuds.end(),
                [&](const auto& noeud) { return noeud.Genre == 19 && noeud.HachageNom == HacherTexte("objets"); });
            Exiger(variable != resultat.Noeuds.end(), "tableau d'objets polymorphes absent");
            const auto indexVariable = static_cast<std::uint64_t>(std::distance(resultat.Noeuds.begin(), variable));
            std::vector<std::uint64_t> decalages;
            for (const auto& resolution : resultat.Resolutions)
                if (resolution.IndexNoeud == indexVariable && (resolution.Drapeaux & 262144U) != 0)
                {
                    const auto& declaration = resultat.Noeuds[resultat.Symboles[resolution.IndexSymbole].IndexNoeud];
                    if (declaration.Genre == 6 && declaration.HachageNom == HacherTexte(nomClasse))
                        decalages.push_back(resolution.HachageType);
                }
            Exiger(decalages == std::vector<std::uint64_t>{type->DecalageTableVirtuelle,
                       type->Taille + type->DecalageTableVirtuelle},
                "disposition polymorphe différente du bootstrap : " + std::string(nomClasse));
        };
        for (const auto& [texte, classe] : std::vector<std::pair<std::string, std::string>>{
                 {"classe B { publique: octet Marque; virtuel destructeur() {} }; "
                  "publique vide F() { B objets[2]; }", "B"},
                 {"classe B { publique: octet Marque; virtuel destructeur() {} }; classe D : publique B { "
                  "publique: entier32 Fin; remplacer destructeur() {} }; publique vide F() { D objets[2]; }", "D"},
                 {"classe B { publique: octet Marque; }; classe D : publique B { publique: octet Fin; "
                  "virtuel booléen opérateur!() { retourner vrai; } }; publique vide F() { D objets[2]; }", "D"}})
        {
            verifierDisposition(texte, classe);
            verifierDisposition(TraduireCorpusConversions(texte), classe);
        }
    }

    void TesterAliasesMethodesSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe,
        AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"classe C { publique: entier32 Lire() { retourner 42; } }; alias Appeler = C::Lire; "
             "publique entier32 F() { retourner Appeler(); }", 21},
            {"classe C { publique: entier32 Lire() { retourner 42; } }; alias Appeler = C::Lire; "
             "publique entier32 F(entier32 x) { retourner Appeler(x); }", 21},
            {"classe C { publique: entier32 Lire() { retourner 42; } }; alias Appeler = C::Lire; "
             "publique entier32 F(C* p) { retourner Appeler(p); }", 21},
            {"classe C { publique: entier32 Lire() { retourner 42; } }; alias Appeler = C::Lire; "
             "publique entier32 F(constante C& objet) { retourner Appeler(objet); }", 21},
            {"classe C { publique: entier32 Lire(entier32 x) { retourner x; } }; alias Appeler = C::Lire; "
             "publique entier32 F(C& objet) { retourner Appeler(objet); }", 21},
            {"classe C { publique: entier32 Lire(entier32 x) { retourner x; } }; alias Appeler = C::Lire; "
             "publique entier32 F(C& objet) { retourner Appeler(objet, vrai); }", 21},
            {"classe C { publique: entier32 Lire(entier32 x) { retourner x; } }; alias Appeler = C::Lire; "
             "publique entier32 F(C& objet) { retourner Appeler(objet, 1, 2); }", 21},
            {"classe C { privée: entier32 Lire() { retourner 42; } }; alias Appeler = C::Lire; "
             "publique entier32 F(C& objet) { retourner Appeler(objet); }", 25},
            {"classe C { protégée: entier32 Lire() { retourner 42; } }; alias Appeler = C::Lire; "
             "publique entier32 F(C& objet) { retourner Appeler(objet); }", 25},
            {"classe C { privée: entier32 Lire() { retourner 42; } }; alias Appeler = C::Lire; "
             "publique entier32 F(entier32 x) { retourner Appeler(x); }", 21},
            {"classe C { publique: entier32 Lire() { retourner 42; } }; alias Appeler = C::Lire; "
             "pointeur_fonction<entier32()> Rappel = Appeler;", 45},
            {"classe C { publique: entier32 Lire() { retourner 42; } }; alias Appeler = C::Lire; "
             "pointeur_fonction<entier32(C*)> Rappel = Appeler;", 45},
            {"classe C { publique: entier32 Lire() { retourner 42; } }; alias Appeler = C::Lire; "
             "pointeur_fonction<entier32(constante C&)> Rappel = Appeler;", 45},
            {"classe C { publique: entier32 Lire(entier32 x) { retourner x; } }; alias Appeler = C::Lire; "
             "pointeur_fonction<entier32(C&, entier32)> Rappel = Appeler; "
             "publique entier32 F(C& objet) { retourner Rappel(objet, vrai); }", 55},
            {"classe C { publique: entier32 Lire() { retourner 42; } }; alias Appeler = C::Lire; "
             "publique entier32 F(C& objet) { retourner (&Appeler)(); }", 54},
            {"classe C { publique: entier32 Lire(entier32 x) { retourner x; } }; alias Appeler = C::Lire; "
             "publique entier32 F(C& objet) { retourner (&Appeler)(objet, vrai); }", 55},
            {"classe C { publique: entier32 Lire() { retourner 42; } }; alias Appeler = C::Lire; "
             "publique entier32 F(C* objet) { retourner (&Appeler)(objet); }", 55},
            {"classe C { publique: entier32 Lire() { retourner 42; } }; alias Appeler = C::Lire; "
             "publique vide F() { convertir<pointeur_fonction<entier32()>>(Appeler); }", 97},
            {"classe C { publique: entier32 Lire() { retourner 42; } }; alias Appeler = C::Lire; "
             "classe Autre {}; publique entier32 F(Autre& objet) { retourner Appeler(objet); }", 21},
            {"classe C { publique: entier32 Lire() { retourner 42; } }; alias Appeler = C::Lire; "
             "publique C Creer(C objet) { retourner objet; } "
             "publique entier32 F(C& objet) { retourner Appeler(Creer(objet)); }", 21},
            {"classe C { publique: entier32 Lire(entier32& x) { retourner x; } }; alias Appeler = C::Lire; "
             "publique entier32 F(C& objet) { retourner Appeler(objet, 42); }", 21},
            {"classe C { publique: entier32 Lire() { retourner 42; } }; alias Appeler = C::Lire; "
             "publique entier32 F(constante C& objet) { retourner (&Appeler)(objet); }", 55},
            {"classe C { publique: entier8 Lire(entier8 x) { retourner x; } }; alias Appeler = C::Lire; "
             "publique entier8 F(C& objet) { retourner (&Appeler)(objet, 128); }", 90},
            {"classe C { publique: entier8 Lire(entier8 x) { retourner x; } }; alias Appeler = C::Lire; "
             "publique entier8 F(C& objet) { retourner Appeler(objet, 128); }", 21},
            {"classe C { publique: entier32 Lire(entier32 x, entier32 y, entier32 z) { retourner x + y + z; } }; "
             "alias Appeler = C::Lire; publique entier32 F(C& objet) { retourner Appeler(objet, 1, 2); }", 21},
            {"classe Base { publique: entier32 Lire() { retourner 42; } }; classe Derivee : publique Base {}; "
             "alias Appeler = Base::Lire; pointeur_fonction<entier32(Derivee&)> Rappel = Appeler;", 45},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
        {
            const auto& [texte, code] = refus[index];
            ComparerErreurSemantique(syntaxe, semantique, texte, code,
                "alias-methode-refuse-fr-" + std::to_string(index));
            ComparerErreurSemantique(syntaxe, semantique, TraduireCorpusConversions(texte), code,
                "alias-methode-refuse-en-" + std::to_string(index));
        }

        const std::vector<std::string> valides{
            "classe C { publique: entier32 X; entier32 Lire() { retourner soi.X; } }; "
            "alias Appeler = C::Lire; publique entier32 F(C& objet) { retourner Appeler(objet); }",
            "classe C { publique: entier32 X; entier32 Lire() { retourner soi.X; } }; "
            "alias Appeler = C::Lire; publique entier32 F(C* objet) { retourner Appeler(*objet); }",
            "classe C { publique: entier32 X; constructeur() : X(42) {} entier32 Lire() { retourner soi.X; } }; "
            "alias Appeler = Copie; alias Copie = C::Lire; "
            "publique entier32 F() { C objet; retourner Appeler(objet); }",
            "espace N { classe C { publique: entier32 Lire(entier32 x) { retourner x; } }; "
            "alias Vue = C; alias Appeler = C::Lire; } alias API::Appeler = N::Appeler; "
            "publique entier32 F(N::Vue& objet) { retourner API::Appeler(objet, 42); }",
            "classe C { publique: entier32 Somme(entier32 x, entier32 y, entier32 z) { retourner x + y + z; } }; "
            "alias Appeler = C::Somme; publique entier32 F(C& objet) { retourner Appeler(objet, 1, 2, 3); }",
            "classe Base { publique: entier32 Lire() { retourner 42; } }; classe Derivee : publique Base {}; "
            "alias Appeler = Base::Lire; publique entier32 F(Derivee& objet) { retourner Appeler(objet); }",
            "classe Base { protégée: entier32 Lire() { retourner 42; } }; alias Appeler = Base::Lire; "
            "classe Derivee : publique Base { publique: entier32 F() { retourner Appeler(soi); } };",
            "classe C { privée: entier32 Lire() { retourner 42; } publique: entier32 F() { retourner Appeler(soi); } }; "
            "alias Appeler = C::Lire;",
            "classe C { publique: entier32 Lire(entier32 x) { retourner x; } }; alias Appeler = C::Lire; "
            "publique entier32 Lire(entier32 x) { retourner x; } "
            "publique entier32 F(C& objet) { retourner Appeler(objet, 42) + Lire(7); }",
            "classe C { publique: entier32 Lire() { retourner 42; } }; alias Appeler = C::Lire; "
            "pointeur_fonction<entier32(C&)> Rappel = Appeler; "
            "publique entier32 F(C& objet) { retourner Rappel(objet); }",
            "classe C { publique: entier32 Lire(entier32 x) { retourner x; } }; alias Appeler = C::Lire; "
            "publique entier32 F(C& objet) { pointeur_fonction<entier32(C&, entier32)> rappel = Appeler; "
            "retourner rappel(objet, 42); }",
            "classe C { publique: entier32 Lire() { retourner 42; } }; alias Appeler = C::Lire; "
            "publique entier32 F(C& objet) { retourner (&Appeler)(objet); }",
            "classe C { publique: entier32 Lire() { retourner 42; } }; alias Appeler = C::Lire; "
            "publique entier32 F(C& objet) { retourner (convertir<pointeur_fonction<entier32(C&)>>(Appeler))(objet); }",
            "classe C { publique: entier32 Lire() { retourner 42; } }; alias Appeler = C::Lire; "
            "publique pointeur_fonction<entier32(C&)> Fournir() { retourner Appeler; } "
            "publique entier32 F(C& objet) { retourner Fournir()(objet); }",
            "structure S { entier32 X; }; classe C { publique: S Creer(entier32 x) { retourner {x}; } }; "
            "alias Appeler = C::Creer; publique entier32 F(C& objet) { S valeur = Appeler(objet, 42); retourner valeur.X; }",
            "classe C { publique: entier32 Lire(entier32& x) { retourner x; } }; alias Appeler = C::Lire; "
            "publique entier32 F(C& objet, entier32& x) { retourner Appeler(objet, x); }",
            "espace A { classe C { publique: entier32 Lire() { retourner 42; } }; } "
            "espace B { classe C { publique: entier64 Lire() { retourner 42; } }; } "
            "alias AppelerA = A::C::Lire; alias AppelerB = B::C::Lire; "
            "publique entier64 F(A::C& a, B::C& b) { retourner convertir<entier64>(AppelerA(a)) + AppelerB(b); }",
            "classe C { publique: entier32 Lire() { retourner 42; } }; alias Appeler = C::Lire; "
            "publique entier32 Lire(entier32 x) { retourner x; } publique entier64 Lire(entier64 x) { retourner x; } "
            "pointeur_fonction<entier32(C&)> Rappel = Appeler; "
            "publique entier32 F(C& objet) { retourner Rappel(objet); }",
            "classe C { publique: entier32 Lire() { retourner 42; } }; alias Appeler = C::Lire; "
            "pointeur_fonction<entier32(C&)> Rappel = Appeler; alias Autre = Rappel; "
            "publique entier32 F(C& objet) { retourner Autre(objet); }",
            "classe C { privée: entier32 Lire() { retourner 42; } }; alias Appeler = C::Lire; "
            "pointeur_fonction<entier32(C&)> Rappel = Appeler; "
            "publique entier32 F(C& objet) { retourner Rappel(objet); }",
            "classe Base { publique: entier32 Lire() { retourner 42; } }; classe Derivee : publique Base {}; "
            "alias Appeler = Base::Lire; pointeur_fonction<entier32(Base&)> Rappel = Appeler; "
            "publique entier32 F(Derivee& objet) { retourner Rappel(objet); }",
            "classe C { publique: entier32 Lire() { retourner 42; } }; alias Appeler = C::Lire; "
            "publique entier32 F(C& objet) { pointeur_fonction<entier32(C&)> rappel = &Appeler; retourner rappel(objet); }",
            "classe C { publique: entier32 Somme(entier32 x, entier32 y, entier32 z) { retourner x + y + z; } }; "
            "alias Appeler = C::Somme; pointeur_fonction<entier32(C&, entier32, entier32, entier32)> Rappel = Appeler; "
            "publique entier32 F(C& objet) { retourner Rappel(objet, 1, 2, 3); }",
            "structure S { entier32 X; }; classe C { publique: S Creer(entier32 x, entier32 y) { retourner {x + y}; } }; "
            "alias Appeler = C::Creer; pointeur_fonction<S(C&, entier32, entier32)> Rappel = Appeler; "
            "publique entier32 F(C& objet) { S valeur = Rappel(objet, 1, 2); retourner valeur.X; }",
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
            for (const auto& texte : {valides[index], TraduireCorpusConversions(valides[index])})
            {
                const auto resultat = AnalyserSemantiqueValide(syntaxe, semantique, texte,
                    "alias-methode-valide-" + std::to_string(index));
                for (const auto& resolution : resultat.Resolutions)
                {
                    Exiger(resolution.IndexSymbole < resultat.Symboles.size()
                               && resultat.Symboles[resolution.IndexSymbole].Genre != 4,
                        "un alias de méthode conserve une cible intermédiaire au lieu de sa déclaration");
                    const auto& noeud = resultat.Noeuds[resolution.IndexNoeud];
                    if (noeud.Genre == 24 && resultat.Noeuds[noeud.Parent].Genre == 28
                        && resolution.IndexNoeud == noeud.Parent + 1
                        && resultat.Noeuds[resultat.Symboles[resolution.IndexSymbole].IndexNoeud].Genre == 12)
                        Exiger((resolution.Drapeaux & 32U) != 0,
                            "l'appel direct d'un alias n'identifie pas la méthode canonique");
                }
            }
    }

    void TesterAliasesChampsSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe,
        AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::vector<std::string> valides{
            "structure S { entier32 X; alias Vue = X; }; publique vide F() {}",
            "structure S { alias Vue = Copie; alias Copie = X; entier32 X; }; "
            "publique entier32 F(S* p) { retourner p->Vue; }",
            "structure S { alias Vue = Copie; alias Autre = Copie; alias Copie = X; entier32 X; }; "
            "publique entier32 F(S* p) { p->Autre = 42; retourner p->Vue + p->Copie; }",
            "espace A { structure S { entier32 X; alias Vue = Copie; alias Copie = X; }; } "
            "espace B { structure S { entier64 Y; alias Vue = Copie; alias Copie = Y; }; } "
            "publique entier64 F(A::S* a, B::S* b) { retourner convertir<entier64>(a->Vue) + b->Vue; }",
            "union U { entier32 X; entier64 Y; alias Vue = Copie; alias Copie = X; }; "
            "publique entier32 F(U* p) { p->Vue = 7; retourner p->Copie; }",
            "classe C { privée: entier32 X; alias Vue = Copie; alias Copie = X; publique: "
            "constructeur() : Vue(42) {} entier32 Lire() { retourner soi.Vue; } };",
            "classe Base { publique: entier32 X; alias Vue = Copie; alias Copie = X; }; "
            "classe Derivee : publique Base { publique: entier32 Lire() { retourner soi.Vue; } }; "
            "publique entier32 F(Derivee* p) { retourner p->Vue; }",
            "structure S { entier32 X[2]; alias Vue = Copie; alias Copie = X; }; "
            "publique entier32 F(S* p) { p->Vue[0] = 7; retourner p->Copie[1]; }",
            "structure S { pointeur_fonction<entier32(entier32)> X; alias Vue = Copie; alias Copie = X; }; "
            "publique entier32 F(S* p) { retourner p->Vue(42); }",
            "structure S { entier32* X; alias Vue = Copie; alias Copie = X; }; "
            "publique entier32 F(S* p) { retourner *p->Vue; }",
            "structure S { constante entier32 X; alias Vue = Copie; alias Copie = X; }; "
            "publique entier32 F(S* p) { retourner p->Vue; }",
            "structure S { entier32 X; alias Vue = Copie; alias Copie = X; }; "
            "publique entier32 F(S* p) { entier32* adresse = &p->Vue; retourner *adresse; }",
            "structure Interne { entier32 X; }; structure S { Interne X; alias Vue = Copie; alias Copie = X; }; "
            "publique entier32 F(S* p) { retourner p->Vue.X; }",
            "classe C { privée: entier32 X; entier32 Y; alias Premier = Copie; alias Copie = X; alias Second = Y; "
            "publique: constructeur() : Premier(1), Second(2) {} "
            "entier32 Lire() { retourner soi.Premier + soi.Second; } };",
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
            for (const auto& texte : {valides[index], TraduireCorpusConversions(valides[index])})
            {
                const auto resultat = AnalyserSemantiqueValide(syntaxe, semantique, texte,
                    "alias-champ-valide-" + std::to_string(index));
                for (const auto& resolution : resultat.Resolutions)
                {
                    if ((resolution.Drapeaux & (8U | 2048U)) == 0
                        || (resolution.Drapeaux & 32768U) != 0)
                        continue;
                    Exiger(resolution.IndexSymbole < resultat.Symboles.size(),
                        "cible membre hors de l'index lors d'une résolution d'alias");
                    const auto& cible = resultat.Symboles[resolution.IndexSymbole];
                    Exiger(cible.Genre != 6 && resolution.HachageType == cible.HachageType,
                        "un accès membre ou initialiseur utilise encore un alias au lieu du stockage canonique");
                }
            }

        // Une longue chaîne et un accès répété exercent le parcours itératif et son cache.
        std::string chaine = "structure S { ";
        for (unsigned index = 0; index < 128; ++index)
            chaine += "alias Vue" + std::to_string(index) + " = "
                + (index == 127 ? std::string("X") : "Vue" + std::to_string(index + 1)) + "; ";
        chaine += "entier32 X; }; publique entier32 F(S* p) { "
            "p->Vue0 = 42; retourner p->Vue0 + p->Vue64 + p->Vue127; }";
        AnalyserSemantiqueValide(syntaxe, semantique, chaine, "alias-champ-chaine-128-fr");
        AnalyserSemantiqueValide(syntaxe, semantique, TraduireCorpusConversions(chaine),
            "alias-champ-chaine-128-en");

        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"structure S {\n  alias Vue = Absente;\n};\npublique vide F() {}", 108},
            {"classe C { publique: alias Vue = Absente; }; publique vide F() {}", 108},
            {"union U { alias Vue = Absente; }; publique vide F() {}", 108},
            {"structure S { alias Vue = Vue; }; publique vide F() {}", 107},
            {"structure S { alias Vue = Copie; alias Copie = Vue; }; publique vide F() {}", 107},
            {"structure S {\n  alias Entree = Vue;\n  alias Vue = Copie;\n  alias Copie = Vue;\n};\n"
             "publique vide F() {}", 107},
            {"union U { alias Vue = Copie; alias Copie = Vue; }; publique vide F() {}", 107},
            {"classe C { publique: alias Vue = Copie; alias Copie = Vue; }; publique vide F() {}", 107},
            {"structure S { alias Vue = Copie; alias Copie = Absente; }; publique vide F() {}", 108},
            {"entier32 X; structure S { alias Vue = X; }; publique vide F() {}", 108},
            {"classe Base { publique: entier32 X; }; "
             "classe Derivee : publique Base { publique: alias Vue = X; }; publique vide F() {}", 108},
            {"structure S { entier32 X; alias Vue = F; }; publique vide F() {}", 108},
            {"classe C { privée: entier32 X; publique: alias Vue = Copie; alias Copie = X; }; "
             "publique entier32 F(C* p) { retourner p->Vue; }", 25},
            {"classe C { privée: entier32 X; alias Vue = Copie; alias Copie = X; publique: "
             "constructeur() : Vue(1), X(2) {} };", 34},
            {"classe C { privée: entier32 X; entier32 Y; alias Vue = Copie; alias Copie = Y; "
             "publique: constructeur() : Vue(1), X(2) {} };", 35},
            {"classe C { privée: entier32 X; alias Vue = Copie; alias Copie = X; "
             "publique: constructeur() : Vue(\"texte\") {} };", 37},
            {"structure S { alias Vue = Absente; alias Vue = Absente; }; publique vide F() {}", 13},
            {"structure S { entier32 X; alias X = Absente; }; publique vide F() {}", 14},
            {"structure S { constante entier32 X; alias Vue = Copie; alias Copie = X; }; "
             "publique vide F(S* p) { p->Vue = 42; }", 71},
            {"structure S { pointeur_fonction<entier32(entier32)> X; alias Vue = Copie; alias Copie = X; }; "
             "publique entier32 F(S* p) { retourner p->Vue(vrai); }", 55},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
        {
            const auto& [texte, code] = refus[index];
            ComparerErreurSemantique(syntaxe, semantique, texte, code,
                "alias-champ-refuse-fr-" + std::to_string(index));
            ComparerErreurSemantique(syntaxe, semantique, TraduireCorpusConversions(texte), code,
                "alias-champ-refuse-en-" + std::to_string(index));
        }
    }

    void TesterAnalyseurSemantique(
        const std::string& chemin,
        const std::string& cheminSyntaxe)
    {
        NombreRefusSemantiquesDifferentiels = 0;
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
        const auto adresseEmission = image.ChercherExport(
            "GalacticShrine::GsPP::Autohebergement::EmettreGlobales");
        const auto aliasEmission = image.ChercherExport(
            "GalacticShrine::GsPP::Autohebergement::EmitGlobals");
        Exiger(adresseEmission.has_value() && aliasEmission == adresseEmission,
               "exports français et anglais de l'émission absents ou différents");
        TesterEmissionGlobales(syntaxe, semantique,
            reinterpret_cast<EmetteurGlobalesAutoHeberge>(*adresseEmission));
        TesterConversionsSemantiques(syntaxe, semantique);
        TesterConversionsImplicitesSemantiques(syntaxe, semantique);
        TesterTypesComposesSemantiques(syntaxe, semantique);
        TesterTypesNommesImbriquesSemantiques(syntaxe, semantique);
        TesterContraintesSignaturesSemantiques(syntaxe, semantique);
        TesterAliasesChampsSemantiques(syntaxe, semantique);
        TesterAliasesRacinesSemantiques(syntaxe, semantique);
        TesterAliasesMethodesSemantiques(syntaxe, semantique);
        TesterDeclarationsHeritageSemantiques(syntaxe, semantique);
        TesterDoublonsSurchargesSemantiques(syntaxe, semantique);
        TesterCollisionsSignaturesNonLieesSemantiques(syntaxe, semantique);
        TesterCollisionsNomsLiaisonSemantiques(syntaxe, semantique);
        TesterAppelsGroupesMixtesSemantiques(syntaxe, semantique);
        TesterOperateursGroupesMixtesSemantiques(syntaxe, semantique);
        TesterPrioritesGroupesInvalidesSemantiques(syntaxe, semantique);
        TesterPrioritesInstructionsSemantiques(syntaxe, semantique);
        TesterPrioritesExpressionsSemantiques(syntaxe, semantique);
        TesterPrioritesAppelsSemantiques(syntaxe, semantique);
        TesterAbandonsCandidatsAppelsSemantiques(syntaxe, semantique);
        TesterArgumentsContextuelsAppelsSemantiques(syntaxe, semantique);
        TesterPrioritesConversionsConstantesSemantiques(syntaxe, semantique);
        TesterRemplacementsVirtuelsSemantiques(syntaxe, semantique);

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
            "  publique entier32 LireConstante(constante entier32& valeur) { "
            "retourner valeur; }\n"
            "  publique entier8 ConserverEtroit(entier8 valeur) { "
            "retourner valeur; }\n"
            "  publique vide ConsommerConstante(constante entier32& valeur) {}\n"
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
            "  publique vide TesterConversions() {\n"
            "    constante entier32 fixe = 4;\n"
            "    constante entier32& referenceFixe = fixe;\n"
            "    entier8 etroite = ConserverEtroit(-1);\n"
            "    LireConstante(referenceFixe);\n"
            "    pointeur_fonction<vide(constante entier32&)> rappel = "
            "&ConsommerConstante;\n"
            "    rappel(fixe);\n"
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
            "  public int32 LireConstante(const int32& valeur) { "
            "return valeur; }\n"
            "  public int8 ConserverEtroit(int8 valeur) { return valeur; }\n"
            "  public void ConsommerConstante(const int32& valeur) {}\n"
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
            "  public void TesterConversions() {\n"
            "    const int32 fixe = 4;\n"
            "    const int32& referenceFixe = fixe;\n"
            "    int8 etroite = ConserverEtroit(-1);\n"
            "    LireConstante(referenceFixe);\n"
            "    function_pointer<void(const int32&)> rappel = "
            "&ConsommerConstante;\n"
            "    rappel(fixe);\n"
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

        const std::string globalesAutoriseesFrancais =
            "classe ObjetGlobal { publique: constructeur() {} };\n"
            "structure PaquetGlobal { entier32 Valeur; };\n"
            "énumération EtatGlobal { Actif = 3, Inactif, };\n"
            "externe constante entier32 Importee;\n"
            "constante entier32 Initialisee = 1;\n"
            "constante entier32 Calculee = convertir<entier32>(1 + 2 * 3);\n"
            "constante entier8 SommeEtroite = 60 + 67;\n"
            "constante entier32 Arithmetique = (20 * 3) / 4 % 7;\n"
            "constante entier32 Bits = ((1 << 4) | 3) ^ 1;\n"
            "constante entier32 Unaires = ~(-1);\n"
            "constante booléen Comparaison = (-8 >> 2) < 0;\n"
            "constante booléen CourtCircuitEt = faux && ((1 / 0) == 0);\n"
            "constante booléen CourtCircuitOu = vrai || ((1 / 0) == 0);\n"
            "PaquetGlobal PaquetInitial = {1 + 2};\n"
            "EtatGlobal EtatInitial = EtatGlobal::Inactif;\n"
            "ObjetGlobal* PointeurObjet;\n"
            "constante entier32* PointeurConstant;\n"
            "pointeur_fonction<vide()> Rappel;\n"
            "pointeur_fonction<vide()> RappelInitial = &CibleGlobale;\n"
            "publique vide CibleGlobale() {}\n"
            "publique vide TesterGlobalesAutorisees() {}\n";
        const std::string globalesAutoriseesAnglais =
            "class ObjetGlobal { public: constructor() {} };\n"
            "struct PaquetGlobal { int32 Valeur; };\n"
            "enumeration EtatGlobal { Actif = 3, Inactif, };\n"
            "extern const int32 Importee;\n"
            "const int32 Initialisee = 1;\n"
            "const int32 Calculee = cast<int32>(1 + 2 * 3);\n"
            "const int8 SommeEtroite = 60 + 67;\n"
            "const int32 Arithmetique = (20 * 3) / 4 % 7;\n"
            "const int32 Bits = ((1 << 4) | 3) ^ 1;\n"
            "const int32 Unaires = ~(-1);\n"
            "const bool Comparaison = (-8 >> 2) < 0;\n"
            "const bool CourtCircuitEt = false && ((1 / 0) == 0);\n"
            "const bool CourtCircuitOu = true || ((1 / 0) == 0);\n"
            "PaquetGlobal PaquetInitial = {1 + 2};\n"
            "EtatGlobal EtatInitial = EtatGlobal::Inactif;\n"
            "ObjetGlobal* PointeurObjet;\n"
            "const int32* PointeurConstant;\n"
            "function_pointer<void()> Rappel;\n"
            "function_pointer<void()> RappelInitial = &CibleGlobale;\n"
            "public void CibleGlobale() {}\n"
            "public void TesterGlobalesAutorisees() {}\n";
        const auto resultatGlobalesAutoriseesFrancais =
            AnalyserSemantiqueValide(
                syntaxe,
                semantique,
                globalesAutoriseesFrancais,
                "globales-autorisees-francais");
        const auto resultatGlobalesAutoriseesAnglais =
            AnalyserSemantiqueValide(
                syntaxe,
                semantique,
                globalesAutoriseesAnglais,
                "globales-autorisees-anglais");
        Exiger(
            resultatGlobalesAutoriseesFrancais.Symboles.size()
                    == resultatGlobalesAutoriseesAnglais.Symboles.size()
                && resultatGlobalesAutoriseesFrancais.Resolutions.size()
                    == resultatGlobalesAutoriseesAnglais.Resolutions.size(),
            "les globales autorisées bilingues divergent");

        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "structure Paquet { entier32 Valeur; }; "
            "publique Paquet Produire() { retourner {1}; } "
            "Paquet Globale = Produire(); publique vide F() {}",
            81,
            "initialiseur-global-agrege-requis-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "struct Paquet { int32 Valeur; }; "
            "public Paquet Produire() { return {1}; } "
            "Paquet Globale = Produire(); public void F() {}",
            81,
            "initialiseur-global-agrege-requis-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide Cible() {} "
            "pointeur_fonction<vide()> Source = &Cible; "
            "pointeur_fonction<vide()> Globale = Source;",
            82,
            "cible-pointeur-fonction-global-invalide-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public void Cible() {} "
            "function_pointer<void()> Source = &Cible; "
            "function_pointer<void()> Globale = Source;",
            82,
            "cible-pointeur-fonction-global-invalide-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "entier32 Valeur = 1; entier32* Globale = &Valeur; "
            "publique vide F() {}",
            83,
            "initialiseur-pointeur-donnees-global-interdit-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "int32 Valeur = 1; int32* Globale = &Valeur; "
            "public void F() {}",
            83,
            "initialiseur-pointeur-donnees-global-interdit-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique entier32 Produire() { retourner 1; } "
            "entier32 Globale = Produire(); publique vide F() {}",
            84,
            "initialiseur-global-non-constant-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public int32 Produire() { return 1; } "
            "int32 Globale = Produire(); public void F() {}",
            84,
            "initialiseur-global-non-constant-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "structure Paquet { entier32 Valeur; }; "
            "publique entier32 Produire() { retourner 1; } "
            "Paquet Globale = {Produire()}; publique vide F() {}",
            84,
            "element-global-agrege-non-constant-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "struct Paquet { int32 Valeur; }; "
            "public int32 Produire() { return 1; } "
            "Paquet Globale = {Produire()}; public void F() {}",
            84,
            "element-global-agrege-non-constant-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "énumération E { V = vrai, }; publique vide F() {}",
            85,
            "valeur-enumeration-non-entiere-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "enumeration E { V = true, }; public void F() {}",
            85,
            "valeur-enumeration-non-entiere-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "énumération E { V = \"a\"[0], }; publique vide F() {}",
            86,
            "valeur-enumeration-non-constante-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "enumeration E { V = \"a\"[0], }; public void F() {}",
            86,
            "valeur-enumeration-non-constante-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "énumération E { V = 2_147_483_648, }; publique vide F() {}",
            87,
            "valeur-enumeration-hors-plage-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "enumeration E { V = 2_147_483_648, }; public void F() {}",
            87,
            "valeur-enumeration-hors-plage-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "énumération E { V = 2_147_483_647, Suivante, }; "
            "publique vide F() {}",
            88,
            "debordement-enumeration-suivante-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "enumeration E { V = 2_147_483_647, Next, }; "
            "public void F() {}",
            88,
            "debordement-enumeration-suivante-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "entier32 Globale = 4 / 0; publique vide F() {}",
            89,
            "division-constante-par-zero-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "int32 Globale = 4 / 0; public void F() {}",
            89,
            "division-constante-par-zero-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "entier32 Globale = 4 % 0; publique vide F() {}",
            89,
            "modulo-constant-par-zero-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "int32 Globale = 4 % 0; public void F() {}",
            89,
            "modulo-constant-par-zero-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "structure Paquet { entier32 Valeur; }; "
            "Paquet Globale = {4 / 0}; publique vide F() {}",
            89,
            "division-constante-agregat-par-zero-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "struct Paquet { int32 Valeur; }; "
            "Paquet Globale = {4 / 0}; public void F() {}",
            89,
            "division-constante-agregat-par-zero-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "entier8 Globale = 64 + 64; publique vide F() {}",
            90,
            "constante-signee-hors-plage-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "int8 Globale = 64 + 64; public void F() {}",
            90,
            "constante-signee-hors-plage-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "naturel8 Globale = 1 - 2; publique vide F() {}",
            90,
            "constante-non-signee-negative-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "uint8 Globale = 1 - 2; public void F() {}",
            90,
            "constante-non-signee-negative-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "énumération E { A = E::B, B = 1, }; publique vide F() {}",
            18,
            "enumerateur-futur-introuvable-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "enumeration E { A = E::B, B = 1, }; public void F() {}",
            18,
            "enumerateur-futur-introuvable-anglais");

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
            syntaxe,
            semantique,
            "publique vide F() { entier32& reference = 1; }",
            69,
            "liaison-reference-temporaire-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public void F() { int32& reference = 1; }",
            69,
            "liaison-reference-temporaire-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide F() { constante entier32 valeur = 1; "
            "entier32& reference = valeur; }",
            69,
            "liaison-reference-constante-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public void F() { const int32 valeur = 1; "
            "int32& reference = valeur; }",
            69,
            "liaison-reference-constante-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide F() { 1 = 2; }",
            70,
            "cible-affectation-non-modifiable-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public void F() { 1 = 2; }",
            70,
            "cible-affectation-non-modifiable-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide F() { constante entier32 valeur = 1; valeur = 2; }",
            71,
            "affectation-valeur-constante-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public void F() { const int32 valeur = 1; valeur = 2; }",
            71,
            "affectation-valeur-constante-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide F() { entier32 valeurs[2]; valeurs = valeurs; }",
            72,
            "affectation-tableau-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public void F() { int32 valeurs[2]; valeurs = valeurs; }",
            72,
            "affectation-tableau-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide F() { entier32 cible = 0; entier64 source = 1; "
            "cible = source; }",
            73,
            "type-affectation-incompatible-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public void F() { int32 cible = 0; int64 source = 1; "
            "cible = source; }",
            73,
            "type-affectation-incompatible-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique entier32 F() { retourner; }",
            74,
            "valeur-retour-attendue-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public int32 F() { return; }",
            74,
            "valeur-retour-attendue-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide F() { retourner 1; }",
            75,
            "type-retour-incompatible-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public void F() { return 1; }",
            75,
            "type-retour-incompatible-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "classe Objet { publique: constructeur() {} }; "
            "Objet Globale; publique vide F() {}",
            76,
            "objet-classe-global-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "class Objet { public: constructor() {} }; "
            "Objet Globale; public void F() {}",
            76,
            "objet-classe-global-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "entier32& Globale; publique vide F() {}",
            77,
            "reference-globale-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "int32& Globale; public void F() {}",
            77,
            "reference-globale-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "vide Globale; publique vide F() {}",
            78,
            "globale-vide-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "void Globale; public void F() {}",
            78,
            "globale-vide-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique externe entier32 Globale; publique vide F() {}",
            79,
            "globale-externe-publique-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public extern int32 Globale; public void F() {}",
            79,
            "globale-externe-publique-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "constante entier32 Globale; publique vide F() {}",
            80,
            "globale-constante-non-initialisee-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "const int32 Globale; public void F() {}",
            80,
            "globale-constante-non-initialisee-anglais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "publique vide Recevoir(entier32& valeur) {} "
            "publique vide F() { constante entier32 valeur = 1; "
            "pointeur_fonction<vide(entier32&)> rappel = &Recevoir; "
            "rappel(valeur); }",
            55,
            "appel-indirect-reference-constante-francais");
        ComparerErreurSemantique(
            syntaxe,
            semantique,
            "public void Recevoir(int32& valeur) {} "
            "public void F() { const int32 valeur = 1; "
            "function_pointer<void(int32&)> rappel = &Recevoir; "
            "rappel(valeur); }",
            55,
            "appel-indirect-reference-constante-anglais");
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
        std::cout << "Sémantique différentielle : " << NombreRefusSemantiquesDifferentiels
                  << " corpus refusés, code/ligne/colonne et AST intact vérifiés.\n";
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
