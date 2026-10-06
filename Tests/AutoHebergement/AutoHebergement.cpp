#include "GsPP/ChargeurGsE.hpp"
#include "GsPP/Compilation.hpp"
#include "GsPP/AnalyseurSemantique.hpp"
#include "GsPP/AnalyseurSyntaxique.hpp"
#include "GsPP/ErreurCompilation.hpp"
#include "GsPP/EcrivainGsE.hpp"
#include "GsPP/Lexeur.hpp"
#include "GsPP/GenerateurX64.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
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

    struct UniteDeclarationsPrepareeHote
    {
        const char* Source;
        std::uint64_t Taille;
        std::uint32_t EstInterface, Reserve;
    };

    struct OrigineUniteDeclarationsHote
    {
        std::uint64_t DebutOctets, TailleOctets;
        std::uint32_t PremiereLigne, NombreLignes, OctetsBom, Reserve;
    };

    struct ResultatAssemblageDeclarationsHote
    {
        std::uint32_t Erreur, LigneErreur, ColonneErreur, Detail;
        std::uint64_t IndexUniteErreur, NombreNoeuds, NombreOctetsSource, NombreOrigines, NombreOctetsArene;
        std::uint32_t DetailLexical, Reserve;
    };

    struct RequeteAssemblageDeclarationsHote
    {
        const UniteDeclarationsPrepareeHote* Unites;
        std::uint64_t NombreUnites;
        char* SourceAssemblee;
        std::uint64_t CapaciteSource;
        NoeudDeclarationHote* Noeuds;
        std::uint64_t CapaciteNoeuds;
        OrigineUniteDeclarationsHote* Origines;
        std::uint64_t CapaciteOrigines;
        ResultatAssemblageDeclarationsHote Resultat;
    };

    static_assert(sizeof(UniteDeclarationsPrepareeHote) == 24);
    static_assert(sizeof(OrigineUniteDeclarationsHote) == 32);
    static_assert(sizeof(ResultatAssemblageDeclarationsHote) == 64);
    static_assert(sizeof(RequeteAssemblageDeclarationsHote) == 128);
    static_assert(offsetof(RequeteAssemblageDeclarationsHote, Resultat) == 64);

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

    struct RequeteAnalyseSemantiqueUnitesHote
    {
        RequeteAnalyseSemantiqueHote* Analyse;
        const OrigineUniteDeclarationsHote* Origines;
        std::uint64_t NombreOrigines, IndexUniteErreur;
        std::uint32_t LigneLocaleErreur, ColonneLocaleErreur;
    };

    static_assert(sizeof(RequeteAnalyseSemantiqueUnitesHote) == 40);

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
    std::optional<std::uint64_t> LimiteAllocationsAssemblage;

    std::uint8_t* GS_ABI_HOTE AllouerMemoireHote(
        std::uint64_t taille)
    {
        if (taille == 0)
            return nullptr;
        if (LimiteAllocationsAssemblage && NombreAllocations >= *LimiteAllocationsAssemblage)
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
            Alias,
            Utilisation
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
        for (const auto& utilisation : programme.Utilisations)
            racines.push_back({GenreRacine::Utilisation, &utilisation,
                utilisation.Position.Ligne, utilisation.Position.Colonne});
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
                        if (fonction.EstExterne) drapeauxFonction |= 2U;
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
            if (racine.Genre == GenreRacine::Utilisation)
            {
                const auto& utilisation = *static_cast<const GsPP::Programme::UtilisationEspace*>(racine.Declaration);
                resultat.push_back({36, static_cast<std::uint32_t>(utilisation.Position.Ligne),
                    static_cast<std::uint32_t>(utilisation.Position.Colonne), 0, 0, 0, 0,
                    HacherTexte(utilisation.Cible), HacherTexte(utilisation.Espace), 0});
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
        if (jeton.Genre == static_cast<std::uint32_t>(GsPP::GenreJeton::DirectiveInclure)
            || jeton.Genre == static_cast<std::uint32_t>(GsPP::GenreJeton::DirectivePragma))
        {
            const auto debutNom = brut.find_first_not_of(" \t", 1);
            return "#" + std::string(brut.substr(debutNom));
        }
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
        ComparerLexage(lexer, "#inclure \"Types.HGsPP\"\n#include \"Point.HGsPP\"\n#pragma once\n",
            "directives-inclusion-bilingues");
        ComparerLexage(lexer, "# \tinclude \"é/Types.HGsPP\"\n#  pragma once\nutilisant espace A; using namespace B;",
            "directives-espaces-et-noms");
        ComparerLexage(lexer, "\xEF\xBB\xBF#pragma once\r\n/** commentaire **/\r\n#inclure \"Point.HGsPP\" // fin\r\n",
            "directives-bom-crlf-commentaires");
        for (const auto& source : {"#", "#\n", "#123", "#define X", "#using namespace A;", "#utilisant espace A;"})
            ComparerErreurLexage(lexer, source, 7, "directive-inconnue");

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
        std::string_view nomCorpus,
        bool estInterface = false)
    {
        const auto jetons = GsPP::Lexeur(
            source, std::string(nomCorpus)).Analyser();
        const auto programme = GsPP::AnalyseurSyntaxique(
            jetons, std::string(nomCorpus), estInterface).Analyser();
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
            if (courant.Genre == 10 || courant.Genre == 24 || courant.Genre == 36)
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
        std::string_view nomCorpus,
        bool estInterface = false)
    {
        std::uint32_t ligne = 0;
        std::uint32_t colonne = 0;
        bool bootstrapRefuse = false;
        try
        {
            const auto jetons = GsPP::Lexeur(
                source, std::string(nomCorpus)).Analyser();
            (void)GsPP::AnalyseurSyntaxique(
                jetons, std::string(nomCorpus), estInterface).Analyser();
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
        std::string_view nomCorpus,
        bool estInterface = false)
    {
        auto noeuds = ComparerDeclarations(syntaxe, source, nomCorpus, estInterface);
        const auto noeudsAvant = noeuds;
        auto jetons = GsPP::Lexeur(source, std::string(nomCorpus)).Analyser();
        auto programme = GsPP::AnalyseurSyntaxique(
            std::move(jetons), std::string(nomCorpus), estInterface).Analyser();
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
        std::string_view nomCorpus,
        bool estInterface = false)
    {
        auto noeuds = ComparerDeclarations(syntaxe, source, nomCorpus, estInterface);
        const auto noeudsAvant = noeuds;
        std::uint32_t ligne = 0;
        std::uint32_t colonne = 0;
        std::string diagnosticBootstrap;
        try
        {
            auto jetons = GsPP::Lexeur(
                source, std::string(nomCorpus)).Analyser();
            auto programme = GsPP::AnalyseurSyntaxique(
                std::move(jetons), std::string(nomCorpus), estInterface).Analyser();
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
        for (const auto& source : std::vector<std::string>{
            "espace N { structure P { entier32 X; }; énumération E { Actif = 42 }; publique entier32 F() { retourner 42; } } utilisant espace N; P Valeur = {42}; E Etat = E::Actif; pointeur_fonction<entier32()> Rappel = F;",
            "espace N { publique entier32 F() { retourner 42; } } espace M { utilisant espace N; } utilisant espace M; alias Appeler = F; pointeur_fonction<entier32()> Rappels[2] = {Appeler, F};",
            "espace N { structure P { entier32 X; entier32 Y; }; } utilisant espace N; P Valeurs[2] = {{20, 22}, {1, 2}}; publique entier32 F() { retourner Valeurs[0].X; }",
            "espace N { externe entier32 F(); } utilisant espace N; pointeur_fonction<entier32()> Rappel = F; publique entier32 Principal() { retourner Rappel(); }"})
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
                ComparerEmissionGlobales(syntaxe, semantique, emettre, texte, "emission-espaces-utilises");
        for (const auto& [source, code] : std::vector<std::pair<std::string, std::uint32_t>>{
            {"utilisant espace Absent; entier32 X = 42; publique vide F() {}", 120},
            {"espace A { structure P {}; } espace B { structure P {}; } utilisant espace A; utilisant espace B; entier32 X = 42; P Objet; publique vide F() {}", 121},
            {"espace A { publique entier32 F() { retourner 1; } } espace B { publique entier32 F() { retourner 2; } } utilisant espace A; utilisant espace B; entier32 X = 42; pointeur_fonction<entier32()> Rappel = F;", 19}})
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
                verifierRefus(texte, code, true);
        for (const auto& source : std::vector<std::string>{
            "entier32 X = 42; classe C { publique: constructeur() {} destructeur() {} }; publique vide F() { C a; C b[2](); }",
            "entier32 X = 42; structure P { entier32 X; }; classe C { publique: constructeur(P p) {} }; publique vide F() { C a({42}); }",
            "entier32 X = 42; classe B { publique: constructeur() {} destructeur() {} }; classe D : publique B {}; publique vide F() { D a; D b[2]; }"})
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
                ComparerEmissionGlobales(syntaxe, semantique, emettre, texte, "emission-constructions-locales");
        for (const auto& [source, code] : std::vector<std::pair<std::string, std::uint32_t>>{
            {"entier32 X = 42; classe C { publique: constructeur(entier32 x) {} }; publique vide F() { C a; Absente; }", 21},
            {"entier32 X = 42; classe C { privée: constructeur() {} }; publique vide F() { C a; Absente; }", 26},
            {"entier32 X = 42; classe C { publique: constructeur() {} privée: destructeur() {} }; publique vide F() { C a[2]; Absente; }", 56},
            {"entier32 X = 42; structure P { entier32 X; }; classe C { publique: constructeur(P p) {} }; publique vide F() { C a({Absente, 2}); }", 43},
            {"entier32 X = 42; classe M { privée: constructeur() {} }; classe C { M m; }; publique vide F() { C a; Absente; }", 26},
            {"entier32 X = 42; classe C { publique: constructeur(entier32 x) {} }; publique vide F() { C a(Absente, 1); }", 21}})
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
                verifierRefus(texte, code, true);
        verifierRefus("entier32 A = 42; entier32 B = 1 / 0; publique vide F() {}", 89, true);
        for (const auto& source : std::vector<std::string>{
            "entier32 X = 42; publique vide F() { constante entier32 x = 42; constante entier32& r = x; }",
            "entier32 X = 42; publique vide F(booléen choix) { si (choix) entier32 x = 1; sinon entier32 x = 2; entier32 x = 3; }",
            "entier32 X = 42; classe C { publique: constructeur() {} }; publique vide F() { C c; C a[2](); }"})
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
                ComparerEmissionGlobales(syntaxe, semantique, emettre, texte, "emission-declarations-locales");
        for (const auto& [source, code] : std::vector<std::pair<std::string, std::uint32_t>>{
            {"entier32 X = 42; publique vide F() { entier32 x(Absente); }", 122},
            {"entier32 X = 42; publique vide F() { vide x = Absente; }", 123},
            {"entier32 X = 42; publique vide F() { entier32& x; Absente; }", 124},
            {"entier32 X = 42; publique vide F() { constante entier32 x; Absente; }", 125},
            {"entier32 X = 42; publique vide F() { Absente; Inconnu x; }", 18},
            {"entier32 X = 42; publique vide F() { Absente; entier32 x = 1; entier32 x = 2; }", 18}})
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
                verifierRefus(texte, code, true);
        for (const auto& source : std::vector<std::string>{
            "entier32 X = 42; classe M { publique: constructeur() {} }; classe B { M m; }; classe D : publique B { publique: constructeur() : parent() {} };",
            "entier32 X = 42; structure P { entier32 X; }; classe B { publique: constructeur(P p) {} }; classe D : publique B { publique: constructeur() : parent({42}) {} };",
            "entier32 X = 42; classe M { publique: constructeur() {} constructeur(entier32 x) {} }; classe C { M a; M b; publique: constructeur() : b(42) {} };",
            "entier32 X = 42; structure P { entier32 X; }; classe C { publique: constructeur() : soi({42}) {} constructeur(P p) {} };"})
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
                ComparerEmissionGlobales(syntaxe, semantique, emettre, texte, "emission-priorites-plans-constructeurs");
        for (const auto& [source, code] : std::vector<std::pair<std::string, std::uint32_t>>{
            {"entier32 X = 42; classe M { privée: constructeur() {} }; classe B { M m; }; classe D : publique B { entier32 Y = Absente; publique: constructeur() {} };", 26},
            {"entier32 X = 42; classe M { privée: constructeur() {} }; classe Interne { M m; }; classe C { Interne i; entier32 Y = Absente; publique: constructeur() {} };", 26},
            {"entier32 X = 42; classe M { privée: constructeur() {} }; classe Interne { M m; }; classe C { entier32 Y = Absente; Interne i; publique: constructeur() {} };", 18},
            {"entier32 X = 42; classe B { publique: constructeur(entier32 x) {} }; classe D : publique B { publique: constructeur() : parent(Absente, 1) {} };", 21},
            {"entier32 X = 42; structure P { entier32 X; }; classe M { privée: constructeur(P p) {} }; classe C { M m; publique: constructeur() : m({Absente, 2}) {} };", 26},
            {"entier32 X = 42; structure P { entier32 X; }; classe B { publique: constructeur(P p) {} }; classe D : publique B { publique: constructeur() : parent({Absente, 2}) {} };", 43}})
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
                verifierRefus(texte, code, true);
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
        for (const auto& [declaration, code] : std::vector<std::pair<std::string, std::uint32_t>>{
                 {"classe C { entier32 X = valeur; publique: constructeur() {} };", 18},
                 {"classe C { entier32 X = valeur; publique: constructeur(booléen valeur) {} };", 37},
                 {"classe C { entier32 X[1] = {Absente, 2}; publique: constructeur() {} };", 42},
                 {"classe C { entier32 X = Absente; publique: constructeur() : Manquant(Absente) {} };", 33}})
        {
            const auto texte = "entier32 Temoin = 42; " + declaration;
            verifierRefus(texte, code, true);
            verifierRefus(TraduireCorpusConversions(texte), code, true);
        }
        const std::vector<std::string> champsContextuelsEmis{
            "entier32 Temoin = 42; classe C { entier32 X = valeur; publique: constructeur(entier32 valeur) {} };",
            "entier32 Temoin = 42; classe C { entier32 X = Absente; publique: constructeur() : X(42) {} };",
            "entier32 Temoin = 42; classe C { entier32 X[2] = {valeur, valeur}; publique: constructeur(entier32 valeur) {} };",
            "entier32 Temoin = 42; publique entier32 Lire(entier32 x) { retourner 11; } publique entier32 Lire(entier64 x) { retourner 31; } classe C { entier32 X = Lire(valeur); publique: constructeur(entier32 valeur) {} constructeur(entier64 valeur) {} };",
        };
        for (std::size_t index = 0; index < champsContextuelsEmis.size(); ++index)
            for (const auto& texte : {champsContextuelsEmis[index], TraduireCorpusConversions(champsContextuelsEmis[index])})
                ComparerEmissionGlobales(syntaxe, semantique, emettre, texte,
                    "emission-champ-contextuel-valide-" + std::to_string(index));

        const std::string declarationsInitialiseursLocaux =
            "entier32 A = 42; structure Point { entier32 X; entier32 Y; }; "
            "classe C { privée: entier32 opérateur+(entier32 x) { retourner x; } }; ";
        for (const auto& [corps, code] : std::vector<std::pair<std::string, std::uint32_t>>{
                 {"entier32 x = {Absente, 2};", 44},
                 {"entier32 x[1] = {Absente, 2};", 42},
                 {"Point p = {vrai, objet + 7};", 45},
                 {"naturel8 x = {300}; objet + 7;", 90},
                 {"objet + 7; entier32 x = {Absente, 2};", 25},
                 {"C copie = {Absente};", 29}})
        {
            const auto texte = declarationsInitialiseursLocaux + "publique vide G(C& objet) { " + corps + " }";
            verifierRefus(texte, code, true);
            verifierRefus(TraduireCorpusConversions(texte), code, true);
        }
        const std::vector<std::string> initialiseursLocauxValides{
            "structure Point { entier32 X; entier32 Y; }; entier32 Globale = 42; "
            "publique vide G() { Point p = {{1}, {2}}; naturel8 x = {255}; }",
            "publique entier32 Lire() { retourner 42; } pointeur_fonction<entier32()> Rappel = Lire; "
            "publique vide G() { pointeur_fonction<entier32()> f = {Lire}; entier32 x = f(); }",
        };
        for (std::size_t index = 0; index < initialiseursLocauxValides.size(); ++index)
            for (const auto& texte : {initialiseursLocauxValides[index],
                                     TraduireCorpusConversions(initialiseursLocauxValides[index])})
                ComparerEmissionGlobales(syntaxe, semantique, emettre, texte,
                    "emission-initialiseur-local-valide-" + std::to_string(index));
        const std::string declarationsInitialiseursGlobaux =
            "entier32 Temoin = 42; structure Point { entier32 X; entier32 Y; }; "
            "publique entier32 Lire() { retourner 42; } ";
        for (const auto& [declaration, code] : std::vector<std::pair<std::string, std::uint32_t>>{
                 {"entier32 X = {Absente, 2};", 44},
                 {"entier32 X[1] = {Absente, 2};", 42},
                 {"Point P = {vrai, Absente};", 45},
                 {"entier32 X = 1 / 0; entier32 Y = Absente;", 89},
                 {"Point P = {1 / 0, vrai};", 45},
                 {"Point P = {Lire(), 0};", 84},
                 {"externe entier32 I; entier32* P = &I; entier32 Y = Absente;", 83},
                 {"classe D { entier32 V = Absente; }; entier32 X = {Absente, 2};", 39},
                 {"entier32 X = {Absente, 2}; classe D { entier32 V = Absente; publique: constructeur() {} };", 44},
                 {"vide X; classe D { entier32 V = Absente; publique: constructeur() {} };", 78}})
        {
            const auto texte = declarationsInitialiseursGlobaux + declaration;
            verifierRefus(texte, code, true);
            verifierRefus(TraduireCorpusConversions(texte), code, true);
        }
        const std::vector<std::string> initialiseursGlobauxValides{
            "structure Point { entier32 X; entier32 Y; }; Point Points[2] = {{{1}, {2}}, {3, 4}}; "
            "naturel8 Octets[2][2] = {{1, 255}, {2, 3}}; publique vide G() {}",
            "publique entier32 Lire() { retourner 42; } "
            "structure Rappel { pointeur_fonction<entier32()> F; entier32 X; }; "
            "Rappel R = {{Lire}, 7}; pointeur_fonction<entier32()> Fonctions[2] = {Lire, Lire};",
            "booléen X = {faux && (1 / 0)}; booléen Y = {vrai || (1 / 0)}; "
            "entier32 Z = {{{7}}}; publique vide G() {}",
            "espace N { structure Point { entier32 X; entier32 Y; }; alias VuePoint = Point; "
            "VuePoint Points[2] = {{1, 2}, {3, 4}}; } "
            "classe D { entier32 V = 7; publique: constructeur() {} }; publique vide G() {}",
        };
        for (std::size_t index = 0; index < initialiseursGlobauxValides.size(); ++index)
            for (const auto& texte : {initialiseursGlobauxValides[index],
                                     TraduireCorpusConversions(initialiseursGlobauxValides[index])})
                ComparerEmissionGlobales(syntaxe, semantique, emettre, texte,
                    "emission-initialiseur-global-contextuel-valide-" + std::to_string(index));
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
                 {"tantque", "while"}, {"sinon", "else"}, {"si", "if"}})
        {
            const auto caractereNom = [](unsigned char caractere)
            {
                return std::isalnum(caractere) != 0 || caractere == '_' || caractere >= 128;
            };
            std::size_t position = 0;
            while ((position = texte.find(fr, position)) != std::string::npos)
            {
                if ((position == 0 || !caractereNom(static_cast<unsigned char>(texte[position - 1])))
                    && (position + fr.size() == texte.size()
                        || !caractereNom(static_cast<unsigned char>(texte[position + fr.size()]))))
                {
                    texte.replace(position, fr.size(), en);
                    position += en.size();
                }
                else position += fr.size();
            }
        }
        for (const auto& [fr, en] : std::vector<std::pair<std::string, std::string>>{
                 {"pointeur_fonction", "function_pointer"}, {"convertir", "cast"},
                 {"constante", "const"}, {"énumération", "enumeration"},
                 {"structure", "struct"}, {"classe", "class"}, {"espace", "namespace"},
                 {"constructeur", "constructor"}, {"opérateur", "operator"},
                 {"destructeur", "destructor"}, {"virtuel", "virtual"}, {"remplacer", "override"},
                 {"externe", "extern"}, {"utilisant", "using"},
                 {"soi", "this"}, {"parent", "super"}, {"caractère", "char"}, {"octet", "byte"},
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
            "publique vide G(pointeur_fonction<vide(N::S*)> p, N::S* s) { N::F(p); p(s); }",
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
            "espace N { structure S {}; espace Sous { publique vide F(pointeur_fonction<vide(S*)> p) {} } }",
            "structure S {}; espace N { structure S {}; publique vide F(pointeur_fonction<vide(S*)> p, N::S* s) { p(s); } }",
            "espace N { classe C { publique: vide F(pointeur_fonction<vide(C*)> p) {} }; }",
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
            {"espace N { espace Autre { structure S {}; } espace Sous { publique vide F(pointeur_fonction<vide(S*)> p) {} } }", 100},
            {"structure S {}; alias Racine = S; espace N { structure S {}; publique vide F(pointeur_fonction<vide(Racine*)> p, N::S* s) { p(s); } }", 55},
            {"espace N { structure S {}; publique vide F(pointeur_fonction<vide(S*)> p, volatile N::S* s) { p(s); } }", 55},
            {"espace N { énumération E { X }; structure S {}; publique vide F(pointeur_fonction<vide(E)> p, S s) { p(s); } }", 55},
            {"espace N { structure S {}; structure R { pointeur_fonction<vide(S*)> Appeler; }; "
             "publique vide F(R* r) { r->Appeler(); } }", 54},
            {"espace N { structure S {}; structure R { pointeur_fonction<vide(S*)> Appeler; }; "
             "publique vide F(R* r, constante S* s) { r->Appeler(s); } }", 55},
            {"espace N { structure R { entier32 Valeur; }; publique vide F(R* r) { r->Valeur(); } }", 53},
            {"espace N { structure S {}; classe R { privée: pointeur_fonction<vide(S*)> Appeler; }; "
             "publique vide F(R* r, S* s) { r->Appeler(s); } }", 25},
            {"espace N { classe C { publique: vide F(pointeur_fonction<vide(Inconnue*)> p) {} }; }", 100},
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
            "structure S { entier32 X; }; publique entier32 F(A::Vue* p) { retourner convertir<entier32>(p->Y); }",
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
            "publique entier32 F() { retourner convertir<entier32>(A::Vue); }",
            "espace A { alias Appeler = Cible; publique entier64 Cible(entier64 x) { retourner x; } } "
            "publique entier32 Cible(entier32 x) { retourner x; } "
            "publique entier32 F() { retourner convertir<entier32>(A::Appeler(convertir<entier64>(42))); }",
            "espace A { alias Vue = X; entier64 X; publique entier32 F() { retourner convertir<entier32>(X); } } entier32 X;",
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
            "espace N { alias Appeler = Cible; publique entier32 Cible() { retourner 42; } } "
            "publique entier32 Cible(entier32 x) { retourner x; } "
            "publique entier64 Cible(entier64 x) { retourner x; }",
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
            {"espace N { alias Appeler = Cible; publique entier32 Cible(entier32 x) { retourner x; } "
             "publique entier64 Cible(entier64 x) { retourner x; } } "
             "publique entier32 Cible() { retourner 42; }", 112},
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
            {"espace N { structure B {}; } classe B {}; espace N { classe D : publique B {}; } "
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
            "structure B {}; espace N { classe B {}; classe D : publique B {}; } "
            "publique vide F(N::D& objet) { N::B& base = objet; }",
            "espace N { classe B {}; } structure B {}; espace N { classe D : publique B {}; } "
            "publique vide F(N::D& objet) { N::B& base = objet; }",
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
            {"classe C {}; publique entier32 opérateur+(C& objet, entier32 x) { retourner x; } "
             "espace N { publique entier32 opérateur+(C& objet, booléen x) { retourner 1; } "
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
            {"classe C {}; publique entier32 opérateur+(C& objet, booléen x) { retourner 1; } "
             "espace N { publique entier32 opérateur+(C& objet, entier32 x) { retourner x; } "
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
     * <résumé>Compare la forme et les feuilles des initialiseurs locaux dans l'ordre source.</résumé>
     * @Paramètre(AnalyseurDeclarationsAutoHeberge: syntaxe) Analyseur de déclarations testé.
     * @Paramètre(AnalyseurSemantiqueAutoHeberge: semantique) Frontend auto-hébergé testé.
     **/
    void TesterPrioritesInitialiseursLocauxSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe,
        AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::string declarations =
            "structure Point { entier32 X; entier32 Y; }; "
            "structure Bloc { Point P; naturel8 Octets[2]; }; "
            "union Choix { entier32 X; entier64 Y; }; "
            "classe C { privée: entier32 opérateur+(entier32 x) { retourner x; } }; "
            "publique entier32 Identite(entier32 x) { retourner x; } "
            "publique entier32 Lire() { retourner 42; } publique vide SansRetour() {} ";
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"entier32 x = {Absente, 2};", 44},
            {"entier32 x = {objet + 7, 2};", 44},
            {"entier32 x[1] = {Absente, 2};", 42},
            {"entier32 x[1] = Absente;", 46},
            {"Point p = {Absente, 2, 3};", 43},
            {"Point p = {vrai, Absente};", 45},
            {"Point p = {Absente, vrai};", 18},
            {"Point p = {objet + 7, vrai};", 25},
            {"Point p = {vrai, objet + 7};", 45},
            {"Choix c = {Absente, 2};", 43},
            {"Bloc b = {{1, 2}, {Absente, 2, 3}};", 42},
            {"Bloc b = {{1, 2}, {1, 300}}; objet + 7;", 90},
            {"naturel8 x = {300}; objet + 7;", 90},
            {"naturel8 x = 300; objet + 7;", 90},
            {"objet + 7; naturel8 x = 300;", 25},
            {"entier32 x = vrai; objet + 7;", 45},
            {"entier32 x = 1 / 0; objet + 7;", 25},
            {"naturel8 x = 1 / 0; objet + 7;", 89},
            {"entier32* x = {vrai, Absente};", 44},
            {"entier32& x = {Absente};", 69},
            {"entier32& x = vrai; objet + 7;", 69},
            {"pointeur_fonction<entier32()> f = {Absente, vrai};", 44},
            {"pointeur_fonction<entier32()> f = {SansRetour}; objet + 7;", 45},
            {"C copie = {Absente};", 29},
            {"C copies[1] = {Absente};", 29},
            {"C copie = Absente;", 29},
            {"C& copie = {Absente};", 69},
            {"tantque (vrai) { entier32 x = {Absente, 2}; } objet + 7;", 44},
            {"{ { Point p = {vrai, Absente}; } } objet + 7;", 45},
            {"objet + 7; entier32 x = {Absente, 2};", 25},
            {"entier32 x = vrai; entier32 y = {Absente, 2};", 45},
            {"convertir<naturel8>(256); entier32 x = {Absente, 2};", 98},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
        {
            const auto source = declarations + "publique vide G(C& objet) { " + refus[index].first + " }";
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
            {
                const auto nom = "priorite-initialiseur-local-refuse-" + std::to_string(index);
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
        const std::vector<std::pair<std::string, std::uint32_t>> fonctionsRefusees{
            {declarations + "publique vide G() { entier32 x = {Absente, 2}; } "
             "publique vide H(C& objet) { objet + 7; }", 44},
            {declarations + "publique vide G(C& objet) { objet + 7; } "
             "publique vide H() { entier32 x = {Absente, 2}; }", 25},
        };
        for (std::size_t index = 0; index < fonctionsRefusees.size(); ++index)
            for (const auto& texte : {fonctionsRefusees[index].first,
                                     TraduireCorpusConversions(fonctionsRefusees[index].first)})
                ComparerErreurSemantique(syntaxe, semantique, texte, fonctionsRefusees[index].second,
                    "priorite-initialiseur-fonction-refuse-" + std::to_string(index));
        const std::vector<std::string> valides{
            "entier32 x = {}; entier32 y = {{{7}}};",
            "Point p = {{1}, {2}}; Point copie = p;",
            "Point points[2] = {{1, 2}, {3, 4}};",
            "Bloc b = {{1, 2}, {3, 4}};",
            "Choix c = {7}; Choix videChoix = {};",
            "naturel8 x = 255; entier8 y = -128;",
            "naturel8 x = {convertir<naturel8>(255)};",
            "entier32 x = Identite({7}); Point p = {Identite({1}), Identite({2})};",
            "pointeur_fonction<entier32()> f = {Lire}; entier32 x = f();",
            "entier32 x = 7; entier32& r = x; constante entier32& c = r;",
            "C x; C& r = x; C* p = &x; C* pointeurs[2] = {p, &r};",
            "naturel8 x = {1 + 2}; booléen b = {vrai}; entier32 valeurs[2][2] = {{1, 2}, {3, 4}};",
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
        {
            const auto source = declarations + "publique vide G() { " + valides[index] + " }";
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
            {
                const auto nom = "initialiseur-local-contextuel-valide-" + std::to_string(index);
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
    }

    /**
     * <résumé>Compare les priorités des initialiseurs globaux et des contrôles structurels des champs par défaut.</résumé>
     * @Paramètre(AnalyseurDeclarationsAutoHeberge: syntaxe) Analyseur de déclarations testé.
     * @Paramètre(AnalyseurSemantiqueAutoHeberge: semantique) Frontend auto-hébergé testé.
     **/
    void TesterPrioritesInitialiseursGlobauxSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe,
        AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::string declarations =
            "structure Point { entier32 X; entier32 Y; }; "
            "structure Bloc { Point P; naturel8 Octets[2]; }; "
            "structure Adresse { entier32* P; entier32 X; }; "
            "union Choix { entier32 X; entier64 Y; }; "
            "classe C { privée: entier32 opérateur+(entier32 x) { retourner x; } }; "
            "publique entier32 Lire() { retourner 42; } publique vide SansRetour() {} ";
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"entier32 X = {Absente, 2};", 44},
            {"entier32 X[1] = {Absente, 2};", 42},
            {"entier32 X[1] = Absente;", 46},
            {"Point P = {Absente, 2, 3};", 43},
            {"Point P = {vrai, Absente};", 45},
            {"Point P = {Absente, vrai};", 18},
            {"Choix X = {Absente, 2};", 43},
            {"Bloc B = {{1, 2}, {Absente, 2, 3}};", 42},
            {"Bloc B = {{1, 2}, {1, 300}};", 90},
            {"naturel8 X = {300};", 90},
            {"entier32* X = {vrai, Absente};", 44},
            {"pointeur_fonction<entier32()> X = {Absente, vrai};", 44},
            {"pointeur_fonction<entier32()> X = {SansRetour};", 45},
            {"C X = {Absente};", 76},
            {"C X[1] = {Absente};", 76},
            {"entier32& X = {Absente};", 77},
            {"vide X = Absente;", 78},
            {"publique externe entier32 X;", 79},
            {"constante entier32 X;", 80},
            {"entier32 X = Lire(); entier32 Y = {Absente, 2};", 84},
            {"entier32 X = 1 / 0; entier32 Y = {Absente, 2};", 89},
            {"entier32 X = {Absente, 2}; vide Y;", 44},
            {"entier32 X = vrai; constante entier32 Y;", 45},
            {"constante entier32 X; entier32 Y = {Absente, 2};", 80},
            {"naturel8 X = 300; entier32 Y = Absente;", 90},
            {"entier32 X = Absente; naturel8 Y = 300;", 18},
            {"Point P = {Lire(), vrai};", 45},
            {"Point P = {1 / 0, vrai};", 45},
            {"Point P = {Lire(), 0};", 84},
            {"Point P = {1 / 0, 0};", 89},
            {"externe entier32 I; Adresse A = {&I, vrai};", 45},
            {"externe entier32 I; Adresse A = {&I, 0};", 83},
            {"Point Q; Point P = Q;", 81},
            {"externe pointeur_fonction<entier32()> F; pointeur_fonction<entier32()> X = F;", 82},
            {"entier32 X = {Absente, 2}; publique vide G(C& objet) { objet + 7; }", 44},
            {"publique vide G(C& objet) { objet + 7; } entier32 X = 1 / 0;", 89},
            {"entier32 X = Lire(); publique vide G(C& objet) { objet + 7; }", 84},
            {"naturel8 X = convertir<naturel8>(256); entier32 Y = {Absente, 2};", 98},
            {"entier32 X = {Absente, 2}; naturel8 Y = convertir<naturel8>(256);", 44},
            {"entier32 X = {Absente, 2}; classe D { entier32 V = Absente; };", 39},
            {"classe D { entier32 V = Absente; }; entier32 X = {Absente, 2};", 39},
            {"classe D { C V = Absente; publique: constructeur() {} }; entier32 X = Absente;", 38},
            {"entier32 X = Absente; classe D { C V[1] = {Absente}; publique: constructeur() {} };", 38},
            {"publique vide G(C& objet) { objet + 7; } classe D { entier32 V = Absente; };", 39},
            {"classe D { entier32 V = Absente; }; publique vide G() {} publique vide G() {}", 118},
            {"espace N { entier32 X = {Absente, 2}; } vide Y;", 44},
            {"alias VuePoint = Point; VuePoint P = {Absente, 2, 3}; vide Y;", 43},
            {"entier32 X = {Absente, 2}; classe D { entier32 V = Absente; publique: constructeur() {} };", 44},
            {"classe D { entier32 V = Absente; publique: constructeur() {} }; entier32 X = {Absente, 2};", 44},
            {"vide X; classe D { entier32 V = Absente; publique: constructeur() {} };", 78},
            {"entier32 X = Lire(); classe D { entier32 V = Absente; publique: constructeur() {} };", 84},
            {"classe D { entier32 V = Absente; publique: constructeur() {} }; entier32 X = 1 / 0;", 89},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
        {
            const auto source = declarations + refus[index].first;
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
            {
                const auto nom = "priorite-initialiseur-global-refuse-" + std::to_string(index);
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
            "entier32 X = {}; entier32 Y = {{{7}}};",
            "Point P = {{1}, {2}}; Point Points[2] = {{1, 2}, {3, 4}};",
            "Bloc B = {{1, 2}, {3, 4}}; Choix X = {7}; Choix Y = {};",
            "naturel8 X = 255; entier8 Y = -128;",
            "pointeur_fonction<entier32()> F = {Lire};",
            "booléen X = {faux && (1 / 0)}; booléen Y = {vrai || (1 / 0)};",
            "espace N { alias VuePoint = Point; VuePoint P[2] = {{1, 2}, {3, 4}}; }",
            "classe D { entier32 V = 7; publique: constructeur() {} };",
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
        {
            const auto source = declarations + valides[index];
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
            {
                const auto nom = "initialiseur-global-contextuel-valide-" + std::to_string(index);
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

    void TesterDeclarationsLocalesContextuellesSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe, AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::vector<std::string> valides{
            "publique vide F() { entier32 x; vide* p; constante entier32* q; }",
            "publique vide F() { constante entier32 x = 42; constante entier32& r = x; }",
            "publique vide F(entier32 x) { entier32& r = x; r = 42; }",
            "publique vide F() { { entier32 x = 1; } { entier32 x = 2; } }",
            "publique vide F(booléen choix) { si (choix) { entier32 x = 1; } sinon { entier32 x = 2; } }",
            "publique vide F(entier32 x) {} publique vide G(entier32 x) { entier32 y = x; }",
            "structure P { entier32 X; }; publique vide F() { P p = {42}; P* q; }",
            "classe C { publique: constructeur() {} }; publique vide F() { C c; C a[2](); }",
            "publique entier32 Lire() { retourner 42; } publique vide F() { constante pointeur_fonction<entier32()> rappel; }",
            "espace N { structure P { entier32 X; }; } utilisant espace N; alias Point = P; publique vide F() { Point p = {42}; }",
            "publique vide F() { constante entier32 x[2] = {1, 2}; vide* p[2]; }",
            "classe C { publique: constructeur(entier32 x) {} vide F(entier32 x) { entier32 y = x; } };",
            "publique vide F(booléen choix) { si (choix) entier32 x = 1; sinon entier32 x = 2; entier32 x = 3; }",
            "publique vide F() { tantque (faux) entier32 x = 1; entier32 x = 2; }",
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
            for (const auto& texte : {valides[index], TraduireCorpusConversions(valides[index])})
            {
                AnalyserSemantiqueValide(syntaxe, semantique, texte,
                    "declaration-locale-contextuelle-valide-" + std::to_string(index));
                auto programme = GsPP::AnalyseurSyntaxique(GsPP::Lexeur(texte).Analyser()).Analyser();
                GsPP::AnalyseurSemantique().Analyser(programme);
                Exiger(!GsPP::GenerateurX64().Generer(programme).Texte.empty(),
                    "la déclaration locale valide n'atteint pas la génération machine");
            }

        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"publique vide F() { entier32 x(); }", 122},
            {"publique vide F() { entier32 x(Absente); }", 122},
            {"structure P { entier32 X; }; publique vide F() { P p(Absente); }", 122},
            {"énumération E { A }; publique vide F() { E e(); }", 122},
            {"classe C {}; publique vide F() { C* p(Absente); }", 122},
            {"publique vide F() { pointeur_fonction<entier32()> rappel(); }", 122},
            {"publique vide F() { entier32 x[2](Absente); }", 122},
            {"publique vide F() { vide v = Absente; }", 123},
            {"publique vide F() { vide& r; }", 123},
            {"publique vide F() { vide v[2]; }", 123},
            {"publique vide F() { entier32& r; Absente; }", 124},
            {"publique vide F() { constante entier32& r(); }", 124},
            {"publique vide F() { constante entier32 x; Absente; }", 125},
            {"publique vide F() { constante entier32 x(Absente); }", 125},
            {"publique vide F() { constante entier32 x[2]; }", 125},
            {"classe C { privée: constructeur() {} }; publique vide F() { constante C c(); }", 125},
            {"publique vide F() { entier32 x = 1; entier32 x = Absente; }", 17},
            {"publique vide F(entier32 x) { entier32 x(); }", 17},
            {"publique vide F() { entier32 x = 1; { entier32 x = Absente; } }", 17},
            {"publique vide F() { Absente; entier32 x = 1; entier32 x = 2; }", 18},
            {"publique vide F() { entier32 x = 1; convertir<vide>(0); entier32 x = 2; }", 94},
            {"publique vide F() { entier32 x = 1; vide x; }", 123},
            {"publique vide F() { entier32 x = 1; Inconnu x; }", 100},
            {"publique vide F() { Absente; Inconnu x; }", 18},
            {"publique vide F() { Inconnu x; Absente; }", 100},
            {"publique vide F() { Absente; entier32& x[2]; }", 18},
            {"publique vide F() { entier32& x[2]; Absente; }", 101},
            {"publique vide F() { Absente; pointeur_fonction<vide(vide)> rappel; }", 18},
            {"publique vide F() { pointeur_fonction<vide(vide)> rappel; Absente; }", 102},
            {"publique vide F() { entier32 x(); } publique vide G() { Inconnu x; }", 122},
            {"publique vide G() { Inconnu x; } publique vide F() { entier32 x(); }", 100},
            {"publique vide F() { Absente; } publique vide G(entier32 x, entier32 x) {}", 18},
            {"publique vide G(entier32 x, entier32 x) {} publique vide F() { Absente; }", 16},
            {"classe C { entier32 X = Absente; publique: constructeur(entier32 x, entier32 x) {} };", 16},
            {"classe C { publique: constructeur() { entier32 x(); } };", 122},
            {"publique vide F() { si (vrai) { entier32 x(); } sinon { Absente; } }", 122},
            {"publique vide F() { tantque (faux) { constante entier32 x; } Absente; }", 125},
            {"espace A { structure P {}; } espace B { structure P {}; } utilisant espace A; utilisant espace B; publique vide F() { Absente; P p; }", 18},
            {"espace A { structure P {}; } espace B { structure P {}; } utilisant espace A; utilisant espace B; publique vide F() { P p; Absente; }", 121},
            {"entier32 X = Absente; publique vide F() { entier32 x(); }", 18},
            {"publique vide F() { si (vrai) entier32 x = 42; sinon x; }", 18},
            {"publique vide F() { entier32 x = 42; si (vrai) entier32 x = 1; }", 17},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
            for (const auto& texte : {refus[index].first, TraduireCorpusConversions(refus[index].first)})
                ComparerErreurSemantique(syntaxe, semantique, texte, refus[index].second,
                    "declaration-locale-contextuelle-refuse-" + std::to_string(index));
    }

    /**
     * <résumé>Exécute les programmes utilisant un nom local dans plusieurs portées distinctes.</résumé>
     * @Paramètre(AnalyseurDeclarationsAutoHeberge: syntaxe) AST comparé au bootstrap.
     * @Paramètre(AnalyseurSemantiqueAutoHeberge: semantique) Contrôle des portées et types locaux.
     * @etc. Les traces publiques contrôlent aussi les destructions de branches, tableaux et retours anticipés.
     **/
    void TesterPorteesLocalesBackend(
        AnalyseurDeclarationsAutoHeberge syntaxe, AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::vector<std::pair<std::string, std::uint32_t>> corpus{
            {"structure P { entier32 X; }; publique entier32 Principal() { entier32 total = 0; "
             "{ entier8 x = 11; entier8& r = x; total = total + convertir<entier32>(r); } "
             "{ P x = {31}; entier32& r = x.X; total = total + r; } retourner total; }", 0},
            {"publique entier32 Choisir(booléen choix) { entier32 resultat = 0; "
             "si (choix) { entier32 x = 11; resultat = x; } sinon { entier32 x = 31; resultat = x; } "
             "entier32 x = 0; retourner resultat + x; } "
             "publique entier32 Principal() { retourner Choisir(vrai) + Choisir(faux); }", 0},
            {"entier32 Compteur = 0; classe C { publique: constructeur(entier32 n) { Compteur = Compteur + n; } }; "
             "publique vide Choisir(booléen choix) { si (choix) C objet(11); sinon C objet(31); C objet(0); } "
             "publique entier32 Principal() { Choisir(vrai); Choisir(faux); retourner Compteur; }", 0},
            {"entier32 Compteur = 0; classe C { publique: constructeur(entier32 n) { Compteur = Compteur + n; } }; "
             "publique entier32 Principal() { tantque (Compteur < 2) C objet(1); C objet(40); retourner Compteur; }", 0},
            {"publique entier32 Principal() { entier32 i = 0; entier32 total = 0; tantque (i < 2) { "
             "entier32 x[2] = {10, 11}; entier32& r = x[0]; total = total + r + x[1]; i = i + 1; } "
             "entier32 x = 0; retourner total + x; }", 0},
            {"publique entier32 LireA() { retourner 11; } publique entier32 LireB() { retourner 31; } "
             "publique entier32 Principal() { entier32 total = 0; "
             "{ pointeur_fonction<entier32()> rappel = LireA; total = total + rappel(); } "
             "{ pointeur_fonction<entier32()> rappel = LireB; total = total + rappel(); } retourner total; }", 0},
            {"entier32 x = 10; publique entier32 Principal() { entier32 total = 0; "
             "{ entier32 x = 11; total = total + x; } total = total + x; "
             "entier32 x = 21; retourner total + x; }", 0},
            {"publique entier32 Trace = 0; classe C { entier32 Id; publique: "
             "constructeur(entier32 n) { soi.Id = n; } destructeur() { Trace = Trace * 10 + soi.Id; } }; "
             "publique entier32 Principal() { { C objet(1); } { C objet(2); } C objet(3); retourner 42; }", 123},
            {"publique entier32 Trace = 0; classe C { entier32 Id; publique: "
             "constructeur(entier32 n) { soi.Id = n; } destructeur() { Trace = Trace * 10 + soi.Id; } }; "
             "publique entier32 Principal() { { C objets[2](1); } { C objets[2](2); } retourner 42; }", 1122},
            {"publique entier32 Trace = 0; classe C { entier32 Id; publique: "
             "constructeur(entier32 n) { soi.Id = n; } destructeur() { Trace = Trace * 10 + soi.Id; } }; "
             "publique entier32 Choisir(booléen choix) { C racine(1); "
             "si (choix) { C objet(2); retourner 21; } C objet(3); retourner 21; } "
             "publique entier32 Principal() { retourner Choisir(vrai) + Choisir(faux); }", 2131},
        };
        for (std::size_t index = 0; index < corpus.size(); ++index)
        {
            std::optional<GsPP::CodeMachine> reference;
            for (const auto& source : {corpus[index].first, TraduireCorpusConversions(corpus[index].first)})
            {
                const auto nom = "portees-locales-backend-" + std::to_string(index);
                AnalyserSemantiqueValide(syntaxe, semantique, source, nom);
                auto programme = GsPP::AnalyseurSyntaxique(GsPP::Lexeur(source, nom).Analyser(), nom).Analyser();
                GsPP::AnalyseurSemantique().Analyser(programme);
                const auto machine = GsPP::GenerateurX64().Generer(programme);
                if (reference)
                    Exiger(machine.Texte == reference->Texte && machine.Donnees == reference->Donnees
                            && machine.TailleZero == reference->TailleZero,
                        "les portées locales produisent des codes bilingues différents : " + nom);
                else reference = machine;
                const auto contenu = GsPP::EcrivainGsE().Construire(machine, "Principal");
                Exiger(contenu == GsPP::EcrivainGsE().Construire(
                        GsPP::GenerateurX64().Generer(programme), "Principal"),
                    "les portées locales produisent une image non reproductible : " + nom);
                ZoneExecutable zone(AlignerPage(Lire64(contenu, 48)));
                const auto image = GsPP::ChargeurGsE().Charger(contenu, zone.Base());
                Exiger(image.Imports.empty(), "les portées locales ajoutent un import d'hôte : " + nom);
                zone.Copier(image.Memoire);
                using FonctionTest = std::int32_t (GS_ABI_HOTE *)();
                const auto executer = reinterpret_cast<FonctionTest>(image.AdressePointEntree);
                Exiger(executer() == 42, "résultat d'exécution des portées locales incorrect : " + nom);
                if (corpus[index].second != 0)
                {
                    const auto trace = image.ChercherExport("Trace");
                    Exiger(trace.has_value(), "trace de destruction locale absente : " + nom);
                    std::uint32_t valeur = 0;
                    std::memcpy(&valeur, reinterpret_cast<const void*>(static_cast<std::uintptr_t>(*trace)), sizeof(valeur));
                    Exiger(valeur == corpus[index].second,
                        "ordre ou nombre de destructions locales incorrect : " + nom);
                }
            }
        }
        std::cout << "Portées locales du backend : " << corpus.size()
                  << " corpus bilingues exécutés, sorties reproductibles et traces de destruction vérifiées.\n";
    }

    /**
     * <résumé>Vérifie la remontée lexicale des noms sans directive d'import.</résumé>
     * @Paramètre(AnalyseurDeclarationsAutoHeberge: syntaxe) AST comparé au bootstrap.
     * @Paramètre(AnalyseurSemantiqueAutoHeberge: semantique) Résolution des noms et diagnostics comparés.
     **/
    void TesterNomsDansEspacesParents(
        AnalyseurDeclarationsAutoHeberge syntaxe, AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::vector<std::pair<std::string, std::string>> valides{
            {"espace N { entier32 Valeur = 42; classe C { publique: entier32 Lire() { retourner Valeur; } }; "
             "espace Interne { publique entier32 Principal() { C c; retourner c.Lire(); } } }", "N::Interne::Principal"},
            {"entier32 Valeur = 1; espace N { entier32 Valeur = 42; espace A::B { "
             "publique entier32 Principal() { retourner Valeur; } } }", "N::A::B::Principal"},
            {"structure P { entier64 Mauvais; }; espace N { structure P { entier32 X; }; espace A::B { "
             "publique entier32 Principal() { P p = {42}; retourner p.X; } } }", "N::A::B::Principal"},
            {"énumération E { Autre = 1 }; espace N { énumération E { Actif = 42 }; espace A::B { "
             "publique entier32 Principal() { E e = E::Actif; retourner convertir<entier32>(e); } } }", "N::A::B::Principal"},
            {"publique entier32 Lire() { retourner 1; } espace N { publique entier32 Lire() { retourner 42; } "
             "espace A::B { publique entier32 Principal() { retourner Lire(); } } }", "N::A::B::Principal"},
            {"espace N { structure P { entier32 X; }; alias Point = P; espace A { alias Position = Point; "
             "publique Position Creer(Position p) { retourner p; } espace B { "
             "publique entier32 Principal() { Position p = Creer({42}); retourner p.X; } } } }", "N::A::B::Principal"},
            {"espace N { publique entier32 Lire() { retourner 42; } pointeur_fonction<entier32()> Rappel = Lire; "
             "espace A::B { publique entier32 Principal() { retourner Rappel(); } } }", "N::A::B::Principal"},
            {"espace N { espace Types { structure P { entier32 X; }; } espace A::B { "
             "publique entier32 Principal() { Types::P p = {42}; retourner p.X; } } }", "N::A::B::Principal"},
            {"entier32 Valeur = 1; espace N { entier32 Valeur = 42; classe C { publique: "
             "entier32 Lire() { entier32 Valeur = 42; retourner Valeur; } }; "
             "publique entier32 Principal() { C c; retourner c.Lire(); } }", "N::Principal"},
            {"publique entier32 Lire(entier32 x) { retourner 1; } espace N { alias Appeler = Lire; "
             "espace A::B { publique entier32 Principal() { retourner Appeler(42) + 41; } } }", "N::A::B::Principal"},
            {"espace N { entier32 Trace = 0; classe C { publique: constructeur() {} "
             "destructeur() { Trace = Trace + 42; } }; espace A::B { "
             "publique entier32 Principal() { { C c; } retourner Trace; } } }", "N::A::B::Principal"},
            {"structure P {}; publique entier32 opérateur+(P p, booléen x) { retourner 1; } "
             "espace N { publique entier32 opérateur+(P p, entier32 x) { retourner x; } espace A::B { "
             "publique entier32 Principal() { P p; retourner p + 42; } } }", "N::A::B::Principal"},
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
        {
            std::optional<GsPP::CodeMachine> reference;
            for (const auto& texte : {valides[index].first, TraduireCorpusConversions(valides[index].first)})
            {
                const auto nom = "noms-espaces-parents-valide-" + std::to_string(index);
                AnalyserSemantiqueValide(syntaxe, semantique, texte, nom);
                auto programme = GsPP::AnalyseurSyntaxique(GsPP::Lexeur(texte, nom).Analyser(), nom).Analyser();
                GsPP::AnalyseurSemantique().Analyser(programme);
                const auto machine = GsPP::GenerateurX64().Generer(programme);
                if (reference)
                    Exiger(machine.Texte == reference->Texte && machine.Donnees == reference->Donnees,
                        "les espaces parents produisent des codes bilingues différents : " + nom);
                else reference = machine;
                const auto contenu = GsPP::EcrivainGsE().Construire(machine, valides[index].second);
                Exiger(contenu == GsPP::EcrivainGsE().Construire(
                        GsPP::GenerateurX64().Generer(programme), valides[index].second),
                    "la sortie des espaces parents n'est pas reproductible : " + nom);
                ZoneExecutable zone(AlignerPage(Lire64(contenu, 48)));
                const auto image = GsPP::ChargeurGsE().Charger(contenu, zone.Base());
                Exiger(image.Imports.empty(), "la résolution des espaces parents ajoute un import d'hôte");
                zone.Copier(image.Memoire);
                using FonctionTest = std::int32_t (GS_ABI_HOTE *)();
                Exiger(reinterpret_cast<FonctionTest>(image.AdressePointEntree)() == 42,
                    "un nom a été résolu dans le mauvais espace : " + nom);
            }
        }
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"espace N { entier32 X = 42; } espace Autre { publique entier32 F() { retourner X; } }", 18},
            {"espace N { structure P {}; } espace Autre { publique vide F() { P p; } }", 100},
            {"espace N { publique entier32 F(entier32 x) { retourner x; } espace A { "
             "publique entier32 F() { retourner 42; } espace B { publique entier32 G() { retourner F(1); } } } }", 21},
            {"publique entier32 F() { retourner 42; } espace N { entier32 F = 1; espace A { "
             "publique entier32 G() { retourner F(); } } }", 53},
            {"espace N { structure P { entier32 X; }; espace A { entier32 P = 1; espace B { "
             "publique vide G() { P p; } } } }", 100},
            {"espace N { entier32 X = 42; espace A { publique vide G() { Absente; Inconnu p; } } }", 18},
            {"espace N { entier32 X = 42; espace A { publique vide G() { Inconnu p; Absente; } } }", 100},
            {"espace N { classe C { publique: entier32 Lire() { retourner Absente; } }; }", 18},
            {"structure P {}; espace N { entier32 P = 1; espace A { publique vide G() { P p; } } }", 100},
            {"espace N { classe C {}; } utilisant espace N::C; publique vide G() {}", 120},
            {"espace N { énumération E { A = convertir<entier32>(E::B), B = 42 }; publique vide G() {} }", 18},
            {"espace N { énumération E { A = convertir<entier32>(E::B), B = 42 }; } "
             "utilisant espace N; publique vide G() {}", 18},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
            for (const auto& texte : {refus[index].first, TraduireCorpusConversions(refus[index].first)})
                ComparerErreurSemantique(syntaxe, semantique, texte, refus[index].second,
                    "noms-espaces-parents-refuse-" + std::to_string(index));
        std::cout << "Noms des espaces parents : " << valides.size()
                  << " corpus bilingues exécutés et masquage lexical vérifié.\n";
    }

    /**
     * <résumé>Compare la recherche des noms d'une méthode à la portée de sa classe.</résumé>
     * @Paramètre(AnalyseurDeclarationsAutoHeberge: syntaxe) AST comparé au bootstrap.
     * @Paramètre(AnalyseurSemantiqueAutoHeberge: semantique) Résolution des méthodes et diagnostics.
     **/
    void TesterNomsDansMethodesSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe, AnalyseurSemantiqueAutoHeberge semantique)
    {
        struct CorpusMethode { std::string Source; std::string Reference; std::uint32_t GenreCible; };
        const std::vector<CorpusMethode> valides{
            {"espace N { publique entier32 Lire() { retourner 1; } classe C { publique: "
             "entier32 Lire() { retourner 42; } entier32 Tester() { retourner Lire(soi); } }; "
             "publique entier32 Principal() { C c; retourner c.Tester(); } }", "Lire", 12},
            {"espace N { publique entier32 Lire() { retourner 1; } classe C { privée: "
             "entier32 Lire() { retourner 42; } publique: entier32 Tester() { retourner Lire(soi); } }; "
             "publique entier32 Principal() { C c; retourner c.Tester(); } }", "Lire", 12},
            {"espace N { publique entier32 Lire(entier32 x) { retourner 1; } classe C { publique: "
             "entier32 Lire(entier32 x) { retourner x; } entier32 Lire(booléen x) { retourner 1; } "
             "entier32 Tester() { retourner Lire(soi, 42); } }; "
             "publique entier32 Principal() { C c; retourner c.Tester(); } }", "Lire", 12},
            {"espace N { publique entier32 Lire(entier32 x) { retourner 1; } classe C { publique: "
             "entier32 Lire(entier32 x) { retourner x; } entier32 Tester() { retourner Lire(42); } }; "
             "espace C { publique entier32 Lire(entier32 x) { retourner x; } } "
             "publique entier32 Principal() { C c; retourner c.Tester(); } }", "Lire", 1},
            {"espace N { publique entier32 Lire() { retourner 1; } classe C { publique: "
             "entier32 Lire() { retourner 42; } entier32 Tester() { "
             "pointeur_fonction<entier32(C&)> rappel = Lire; retourner rappel(soi); } }; "
             "publique entier32 Principal() { C c; retourner c.Tester(); } }", "Lire", 12},
            {"espace N { publique entier32 Lire() { retourner 1; } classe C { entier32 X = Lire(soi); "
             "publique: constructeur() {} entier32 Lire() { retourner 42; } "
             "entier32 Tester() { retourner soi.X; } }; "
             "publique entier32 Principal() { C c; retourner c.Tester(); } }", "Lire", 12},
            {"espace N { publique entier32 Lire() { retourner 1; } classe C { entier32 X; "
             "publique: constructeur() : X(Lire(soi)) {} entier32 Lire() { retourner 42; } "
             "entier32 Tester() { retourner soi.X; } }; "
             "publique entier32 Principal() { C c; retourner c.Tester(); } }", "Lire", 12},
            {"espace N { publique entier32 Lire() { retourner 1; } classe C { entier32 X; "
             "publique: constructeur() { soi.X = Lire(soi); } entier32 Lire() { retourner 42; } "
             "entier32 Tester() { retourner soi.X; } }; "
             "publique entier32 Principal() { C c; retourner c.Tester(); } }", "Lire", 12},
            {"espace N { publique entier32 Lire() { retourner 42; } classe C { publique: "
             "entier32 Lire() { retourner 1; } entier32 Tester() { retourner N::Lire(); } }; "
             "publique entier32 Principal() { C c; retourner c.Tester(); } }", "N::Lire", 1},
            {"espace N { publique entier32 Source() { retourner 42; } classe C { publique: "
             "entier32 Lire() { retourner 1; } entier32 Tester(pointeur_fonction<entier32()> Lire) { "
             "retourner Lire(); } }; publique entier32 Principal() { C c; retourner c.Tester(Source); } }", "Lire", 2},
            {"espace N { entier32 Trace = 0; publique vide Lire() { Trace = 1; } classe C { privée: "
             "vide Lire() { Trace = 42; } publique: constructeur() {} destructeur() { Lire(soi); } }; "
             "publique entier32 Principal() { { C c; } retourner Trace; } }", "Lire", 12},
            {"espace N { publique entier32 Tester(entier32 n) { retourner 1; } classe C { publique: "
             "entier32 Tester(entier32 n) { si (n == 0) retourner 42; retourner Tester(soi, n - 1); } }; "
             "publique entier32 Principal() { C c; retourner c.Tester(2); } }", "Tester", 12},
            {"espace A { publique entier32 Lire() { retourner 1; } } utilisant espace A; "
             "espace N { classe C { publique: entier32 Lire() { retourner 42; } "
             "entier32 Tester() { retourner Lire(soi); } }; "
             "publique entier32 Principal() { C c; retourner c.Tester(); } }", "Lire", 12},
            {"espace N { publique entier32 Source() { retourner 42; } classe C { entier32 X = Lire(); "
             "publique: constructeur(pointeur_fonction<entier32()> Lire) {} entier32 Lire() { retourner 1; } "
             "entier32 Tester() { retourner soi.X; } }; "
             "publique entier32 Principal() { C c(Source); retourner c.Tester(); } }", "Lire", 2},
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
        {
            std::optional<GsPP::CodeMachine> reference;
            for (const auto& texte : {valides[index].Source, TraduireCorpusConversions(valides[index].Source)})
            {
                const auto nom = "noms-methodes-valide-" + std::to_string(index);
                const auto resultat = AnalyserSemantiqueValide(syntaxe, semantique, texte, nom);
                auto programme = GsPP::AnalyseurSyntaxique(GsPP::Lexeur(texte, nom).Analyser(), nom).Analyser();
                GsPP::AnalyseurSemantique().Analyser(programme);
                std::unordered_map<std::uint64_t, const GsPP::Fonction*> fonctionsBootstrap;
                std::vector<NoeudDeclarationHote> referencesBootstrap;
                for (const auto& fonction : programme.Fonctions)
                {
                    fonctionsBootstrap.emplace(HacherTexte(fonction.NomComplet()), &fonction);
                    if (fonction.Corps) AjouterInstructionReference(*fonction.Corps, 0, 0, referencesBootstrap);
                    for (const auto& champ : fonction.InitialiseursChamps)
                    {
                        for (const auto& argument : champ.Arguments)
                            AjouterExpressionReference(*argument, 0, 0, referencesBootstrap);
                        if (champ.InitialiseurParDefaut)
                            AjouterExpressionReference(*champ.InitialiseurParDefaut, 0, 0, referencesBootstrap);
                    }
                }
                std::unordered_map<std::uint64_t, const GsPP::Fonction*> ciblesBootstrap;
                for (const auto& noeud : referencesBootstrap)
                    if (noeud.Genre == 24 && fonctionsBootstrap.contains(noeud.HachageNom))
                        ciblesBootstrap.emplace((static_cast<std::uint64_t>(noeud.Ligne) << 32) | noeud.Colonne,
                            fonctionsBootstrap.at(noeud.HachageNom));
                std::size_t cibles = 0;
                for (const auto& resolution : resultat.Resolutions)
                {
                    const auto& noeud = resultat.Noeuds[resolution.IndexNoeud];
                    if (noeud.Genre != 24 || noeud.HachageNom != HacherTexte(valides[index].Reference)) continue;
                    const auto& cible = resultat.Noeuds[resultat.Symboles[resolution.IndexSymbole].IndexNoeud];
                    Exiger(cible.Genre == valides[index].GenreCible,
                        "la référence a sélectionné une autre famille de déclaration : " + nom);
                    if (cible.Genre == 12)
                        Exiger(resultat.Noeuds[cible.Parent].HachageNom == HacherTexte("C"),
                            "la référence n'a pas sélectionné la méthode de sa propre classe : " + nom);
                    if (cible.Genre == 1)
                        Exiger(cible.HachageEspace == HacherTexte(index == 3 ? "N::C" : "N"),
                            "la référence a sélectionné une fonction du mauvais espace : " + nom);
                    if (cible.Genre == 1 || cible.Genre == 12)
                    {
                        const auto referenceCible = ciblesBootstrap.find(
                            (static_cast<std::uint64_t>(noeud.Ligne) << 32) | noeud.Colonne);
                        Exiger(referenceCible != ciblesBootstrap.end(),
                            "la cible de référence du bootstrap est absente : " + nom);
                        Exiger(cible.Ligne == referenceCible->second->Position.Ligne
                                && cible.Colonne == referenceCible->second->Position.Colonne
                                && resolution.HachageType == HacherTypeDeclaration(referenceCible->second->TypeRetour),
                            "la surcharge ou le type de retour choisi diffère du bootstrap : " + nom);
                    }
                    ++cibles;
                }
                Exiger(cibles != 0, "la référence de méthode n'a publié aucune résolution : " + nom);
                const auto machine = GsPP::GenerateurX64().Generer(programme);
                if (reference)
                    Exiger(machine.Texte == reference->Texte && machine.Donnees == reference->Donnees,
                        "les méthodes produisent des codes bilingues différents : " + nom);
                else reference = machine;
                const auto contenu = GsPP::EcrivainGsE().Construire(machine, "N::Principal");
                Exiger(contenu == GsPP::EcrivainGsE().Construire(
                        GsPP::GenerateurX64().Generer(programme), "N::Principal"),
                    "la sortie des méthodes n'est pas reproductible : " + nom);
                ZoneExecutable zone(AlignerPage(Lire64(contenu, 48)));
                const auto image = GsPP::ChargeurGsE().Charger(contenu, zone.Base());
                Exiger(image.Imports.empty(), "la résolution des méthodes ajoute un import d'hôte");
                zone.Copier(image.Memoire);
                using FonctionTest = std::int32_t (GS_ABI_HOTE *)();
                Exiger(reinterpret_cast<FonctionTest>(image.AdressePointEntree)() == 42,
                    "résultat d'exécution des noms de méthodes incorrect : " + nom);
            }
        }
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"espace N { publique entier32 Lire() { retourner 42; } classe C { publique: "
             "entier32 Lire() { retourner 1; } entier32 Tester() { retourner Lire(); } }; }", 21},
            {"espace N { publique entier32 Lire(entier32 x) { retourner x; } classe C { publique: "
             "entier32 Lire(entier32 x) { retourner x; } entier32 Tester() { retourner Lire(42); } }; }", 21},
            {"espace N { publique entier32 Lire() { retourner 42; } classe C { publique: "
             "entier32 Lire(entier32 x) { retourner x; } entier32 Tester() { retourner Lire(soi, vrai); } }; }", 21},
            {"espace N { publique entier32 Lire() { retourner 42; } classe C { publique: "
             "entier32 Lire() { retourner 1; } vide Tester() { pointeur_fonction<entier32()> rappel = Lire; } }; }", 45},
            {"espace N { publique entier32 Lire() { retourner 42; } classe C { publique: "
             "entier32 Lire(entier32 x) { retourner x; } entier32 Lire(booléen x) { retourner 1; } "
             "vide Tester() { Lire; } }; }", 19},
            {"espace N { publique entier32 Lire() { retourner 42; } classe C { publique: "
             "entier32 Lire() { retourner 1; } entier32 Tester() { retourner (&Lire)(); } }; }", 54},
            {"espace N { publique entier32 Lire() { retourner 42; } classe C { publique: "
             "entier32 Lire(entier32 x) { retourner x; } entier32 Tester() { retourner (&Lire)(soi, vrai); } }; }", 55},
            {"espace N { publique entier32 Lire() { retourner 42; } classe C { publique: "
             "entier32 Lire() { retourner 1; } vide Tester() { convertir<pointeur_fonction<entier32()>>(Lire); } }; }", 97},
            {"espace N { entier32 Lire = 42; classe C { publique: entier32 Lire() { retourner 1; } "
             "vide Tester() { Lire = 42; } }; }", 70},
            {"espace N { classe C { publique: entier32 Lire() { retourner 42; } "
             "entier32 Tester() { entier32 Lire = 1; retourner Lire(); } }; }", 53},
            {"espace N { publique entier32 Lire() { retourner 42; } classe C { entier32 X = Lire(); "
             "publique: constructeur() {} entier32 Lire() { retourner 1; } }; }", 21},
            {"espace N { publique entier32 Lire() { retourner 42; } classe C { entier32 X; "
             "publique: constructeur() : X(Lire()) {} entier32 Lire() { retourner 1; } }; }", 21},
            {"espace N { publique entier32 Lire() { retourner 42; } classe C { publique: "
             "constructeur() { Lire(); } entier32 Lire() { retourner 1; } }; }", 21},
            {"espace N { publique entier32 Lire() { retourner 42; } classe C { publique: "
             "destructeur() { Lire(); } entier32 Lire() { retourner 1; } }; }", 21},
            {"espace N { publique entier32 Lire() { retourner 42; } classe C { publique: "
             "entier32 Lire() { retourner 1; } vide Tester() { Lire(); Absente; } }; }", 21},
            {"espace N { publique entier32 Lire() { retourner 42; } classe C { publique: "
             "entier32 Lire() { retourner 1; } vide Tester() { Absente; Lire(); } }; }", 18},
            {"espace N { classe C { privée: entier32 Lire() { retourner 42; } }; classe D { publique: "
             "entier32 Tester(C& c) { retourner C::Lire(c); } }; }", 25},
            {"espace A { publique entier32 Lire() { retourner 42; } } utilisant espace A; "
             "espace N { classe C { publique: entier32 Lire() { retourner 1; } "
             "entier32 Tester() { retourner Lire(); } }; }", 21},
            {"espace A { publique entier32 Lire() { retourner 42; } } "
             "espace N { utilisant espace A; classe C { publique: entier32 Lire() { retourner 1; } "
             "entier32 Tester() { retourner Lire(); } }; }", 21},
            {"espace N { publique entier32 Tester(entier32 n) { retourner 42; } classe C { publique: "
             "entier32 Tester(entier32 n) { retourner Tester(n - 1); } }; }", 21},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
            for (const auto& texte : {refus[index].first, TraduireCorpusConversions(refus[index].first)})
                ComparerErreurSemantique(syntaxe, semantique, texte, refus[index].second,
                    "noms-methodes-refuse-" + std::to_string(index));
        std::cout << "Noms des méthodes : " << valides.size()
                  << " corpus bilingues exécutés et cibles de résolution vérifiées.\n";
    }

    /**
     * <résumé>Compare les portées d'opérateurs dans les méthodes et la durée de vie des objets.</résumé>
     * @Paramètre(AnalyseurDeclarationsAutoHeberge: syntaxe) AST bilingue comparé au bootstrap.
     * @Paramètre(AnalyseurSemantiqueAutoHeberge: semantique) Cibles exactes et diagnostics publics.
     **/
    void TesterOperateursDansMethodesSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe, AnalyseurSemantiqueAutoHeberge semantique)
    {
        struct CorpusOperateur { std::string Source; std::string EspaceCible; bool Methode; };
        const std::vector<CorpusOperateur> valides{
            {"structure P {}; espace N { publique entier32 opérateur+(P& p, entier32 x) { retourner 1; } "
             "classe C { publique: entier32 Tester(P& p) { retourner p + 42; } }; "
             "espace C { publique entier32 opérateur+(P& p, entier32 x) { retourner x; } } "
             "publique entier32 Principal() { C c; P p; retourner c.Tester(p); } }", "N::C", false},
            {"espace N { classe C { privée: entier32 opérateur+(entier32 x) { retourner x; } publique: "
             "entier32 Tester() { retourner soi + 42; } }; "
             "publique entier32 opérateur+(C& c, entier32 x) { retourner 1; } "
             "publique entier32 Principal() { C c; retourner c.Tester(); } }", "N::C", true},
            {"structure P {}; espace N { publique entier32 opérateur+(P& p, entier32 x) { retourner 1; } "
             "classe C { publique: entier32 opérateur+(P& p) { retourner 1; } "
             "entier32 Tester(P& p) { retourner p + 42; } }; "
             "espace C { publique entier32 opérateur+(P& p, entier32 x) { retourner x; } } "
             "publique entier32 Principal() { C c; P p; retourner c.Tester(p); } }", "N::C", false},
            {"structure P {}; espace N { publique booléen opérateur!(P& p) { retourner faux; } "
             "classe C { publique: booléen opérateur!() { retourner faux; } "
             "entier32 Tester(P& p) { retourner convertir<entier32>(!p) + 41; } }; "
             "espace C { publique booléen opérateur!(P& p) { retourner vrai; } } "
             "publique entier32 Principal() { C c; P p; retourner c.Tester(p); } }", "N::C", false},
            {"structure P {}; espace N { publique entier32 opérateur+(P& p, entier32 x) { retourner x; } "
             "espace Interne { classe C { publique: entier32 Tester(P& p) { retourner p + 42; } }; "
             "publique entier32 Principal() { C c; P p; retourner c.Tester(p); } } "
             "publique entier32 Principal() { retourner Interne::Principal(); } }", "N", false},
            {"structure P {}; espace A { publique entier32 opérateur+(P& p, entier32 x) { retourner x; } } "
             "espace N { utilisant espace A; classe C { publique: "
             "entier32 Tester(P& p) { retourner p + 42; } }; "
             "publique entier32 Principal() { C c; P p; retourner c.Tester(p); } }", "A", false},
            {"structure P {}; publique entier32 opérateur+(P& p, entier32 x) { retourner x; } "
             "espace N { classe C { publique: entier32 Tester(P& p) { retourner p + 42; } }; "
             "publique entier32 Principal() { C c; P p; retourner c.Tester(p); } }", "", false},
            {"espace N { classe C { entier32 X = soi + 42; privée: "
             "entier32 opérateur+(entier32 x) { retourner x; } publique: constructeur() {} "
             "entier32 Tester() { retourner soi.X; } }; "
             "publique entier32 Principal() { C c; retourner c.Tester(); } }", "N::C", true},
            {"espace N { classe C { privée: booléen opérateur!() { retourner vrai; } publique: "
             "entier32 Tester() { retourner convertir<entier32>(!soi) + 41; } }; "
             "publique entier32 Principal() { C c; retourner c.Tester(); } }", "N::C", true},
            {"espace N { classe C { entier32 X; privée: entier32 opérateur+(entier32 x) { retourner x; } "
             "publique: constructeur() : X(soi + 42) {} entier32 Tester() { retourner soi.X; } }; "
             "publique entier32 Principal() { C c; retourner c.Tester(); } }", "N::C", true},
            {"espace N { classe C { entier32 X; privée: entier32 opérateur+(entier32 x) { retourner x; } "
             "publique: constructeur() { soi.X = soi + 42; } entier32 Tester() { retourner soi.X; } }; "
             "publique entier32 Principal() { C c; retourner c.Tester(); } }", "N::C", true},
            {"espace N { entier32 Trace = 0; classe C { privée: "
             "entier32 opérateur+(entier32 x) { retourner x; } publique: "
             "destructeur() { Trace = soi + 42; } }; "
             "publique entier32 Principal() { { C c; } retourner Trace; } }", "N::C", true},
            {"structure P {}; espace N { publique entier32 opérateur+(P& p, entier32 x) { retourner 1; } "
             "classe C { entier32 X = p + 42; publique: constructeur(P& p) {} "
             "entier32 opérateur+(P& p) { retourner 1; } entier32 Tester() { retourner soi.X; } }; "
             "espace C { publique entier32 opérateur+(P& p, entier32 x) { retourner x; } } "
             "publique entier32 Principal() { P p; C c(p); retourner c.Tester(); } }", "N::C", false},
            {"classe P { publique: entier32 opérateur+(entier32 x) { retourner x; } }; "
             "espace N { classe C { publique: entier32 opérateur+(P& p) { retourner 1; } "
             "entier32 Tester(P& p) { retourner p + 42; } }; "
             "publique entier32 Principal() { C c; P p; retourner c.Tester(p); } }", "P", true},
            {"espace N { classe B { protégée: entier32 opérateur+(entier32 x) { retourner x; } }; "
             "classe C : publique B { publique: entier32 Tester() { retourner soi + 42; } }; "
             "publique entier32 Principal() { C c; retourner c.Tester(); } }", "N::B", true},
            {"structure P {}; structure Q {}; espace N { "
             "publique entier32 opérateur+(P& p, entier32 x) { retourner 1; } "
             "classe C { entier32 X = p + 21; publique: constructeur(P& p) {} constructeur(Q& p) {} "
             "entier32 opérateur+(C& c) { retourner 1; } entier32 Tester() { retourner soi.X; } }; "
             "espace C { publique entier32 opérateur+(P& p, entier32 x) { retourner x; } "
             "publique entier32 opérateur+(Q& p, entier32 x) { retourner x; } } "
             "publique entier32 Principal() { P p; Q q; C a(p); C b(q); retourner a.Tester() + b.Tester(); } }",
             "N::C", false},
            {"structure P {}; espace N { classe C { entier32 X = convertir<entier32>(!p) + 41; "
             "publique: constructeur(P& p) {} constructeur(booléen p) {} "
             "booléen opérateur!() { retourner faux; } entier32 Tester() { retourner soi.X; } }; "
             "espace C { publique booléen opérateur!(P& p) { retourner vrai; } } "
             "publique entier32 Principal() { P p; C a(p); C b(faux); retourner a.Tester() + b.Tester() - 42; } }",
             "N::C", false},
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
        {
            std::optional<GsPP::CodeMachine> machineReference;
            for (const auto& texte : {valides[index].Source, TraduireCorpusConversions(valides[index].Source)})
            {
                const auto nom = "operateurs-methodes-valide-" + std::to_string(index);
                const auto resultat = AnalyserSemantiqueValide(syntaxe, semantique, texte, nom);
                auto programme = GsPP::AnalyseurSyntaxique(GsPP::Lexeur(texte, nom).Analyser(), nom).Analyser();
                GsPP::AnalyseurSemantique().Analyser(programme);
                std::unordered_map<std::uint64_t, std::vector<const GsPP::Fonction*>> cibles;
                const auto visiter = [&](auto&& self, const GsPP::Expression& expression) -> void {
                    std::string nomSurcharge;
                    switch (expression.Genre)
                    {
                        case GsPP::GenreExpression::Unaire:
                        {
                            const auto& unaire = static_cast<const GsPP::ExpressionUnaire&>(expression);
                            nomSurcharge = unaire.NomSurcharge;
                            self(self, *unaire.Operande);
                            break;
                        }
                        case GsPP::GenreExpression::Binaire:
                        {
                            const auto& binaire = static_cast<const GsPP::ExpressionBinaire&>(expression);
                            nomSurcharge = binaire.NomSurcharge;
                            self(self, *binaire.Gauche);
                            self(self, *binaire.Droite);
                            break;
                        }
                        case GsPP::GenreExpression::Affectation:
                            self(self, *static_cast<const GsPP::ExpressionAffectation&>(expression).Valeur);
                            break;
                        case GsPP::GenreExpression::Conversion:
                            self(self, *static_cast<const GsPP::ExpressionConversion&>(expression).Valeur);
                            break;
                        default: break;
                    }
                    if (nomSurcharge.empty()) return;
                    const auto cible = std::find_if(programme.Fonctions.begin(), programme.Fonctions.end(),
                        [&](const auto& fonction) { return fonction.NomComplet() == nomSurcharge; });
                    Exiger(cible != programme.Fonctions.end() && cible->Espace == valides[index].EspaceCible
                               && cible->EstMethode == valides[index].Methode,
                        "cible prévue de l'opérateur différente du bootstrap : " + nom);
                    cibles[(static_cast<std::uint64_t>(expression.Position.Ligne) << 32U)
                        | expression.Position.Colonne].push_back(&*cible);
                };
                for (const auto& fonction : programme.Fonctions)
                {
                    if (fonction.Corps)
                        for (const auto& instruction : fonction.Corps->Instructions)
                        {
                            if (instruction->Genre == GsPP::GenreInstruction::Retour)
                            {
                                const auto& valeur = static_cast<const GsPP::InstructionRetour&>(*instruction).Valeur;
                                if (valeur) visiter(visiter, *valeur);
                            }
                            else if (instruction->Genre == GsPP::GenreInstruction::Expression)
                                visiter(visiter, *static_cast<const GsPP::InstructionExpression&>(*instruction).Valeur);
                        }
                    for (const auto& initialiseur : fonction.InitialiseursChamps)
                    {
                        for (const auto& argument : initialiseur.Arguments) visiter(visiter, *argument);
                        if (initialiseur.InitialiseurParDefaut) visiter(visiter, *initialiseur.InitialiseurParDefaut);
                    }
                }
                Exiger(!cibles.empty(), "opérateur absent du corpus : " + nom);
                std::size_t nombreAttendu = 0;
                for (const auto& [position, references] : cibles) nombreAttendu += references.size();
                std::size_t nombre = 0;
                for (const auto& resolution : resultat.Resolutions)
                {
                    if ((resolution.Drapeaux & 256U) == 0) continue;
                    const auto& expression = resultat.Noeuds[resolution.IndexNoeud];
                    const auto cible = cibles.find((static_cast<std::uint64_t>(expression.Ligne) << 32U) | expression.Colonne);
                    Exiger(cible != cibles.end(), "opérateur publié absent du bootstrap : " + nom);
                    const auto& declaration = resultat.Noeuds[resultat.Symboles[resolution.IndexSymbole].IndexNoeud];
                    const auto reference = std::find_if(cible->second.begin(), cible->second.end(),
                        [&](const auto* fonction) {
                            return declaration.Ligne == fonction->Position.Ligne
                                && declaration.Colonne == fonction->Position.Colonne
                                && resolution.HachageType == HacherTypeDeclaration(fonction->TypeRetour)
                                && ((resolution.Drapeaux & 32U) != 0) == fonction->EstMethode;
                        });
                    Exiger(reference != cible->second.end(),
                        "cible, retour ou drapeau d'opérateur différent du bootstrap : " + nom);
                    cible->second.erase(reference);
                    ++nombre;
                }
                Exiger(nombre == nombreAttendu, "résolution d'opérateur manquante ou dupliquée : " + nom);
                const auto machine = GsPP::GenerateurX64().Generer(programme);
                if (machineReference)
                    Exiger(machine.Texte == machineReference->Texte && machine.Donnees == machineReference->Donnees,
                        "les opérateurs produisent des codes bilingues différents : " + nom);
                else machineReference = machine;
                const auto contenu = GsPP::EcrivainGsE().Construire(machine, "N::Principal");
                Exiger(contenu == GsPP::EcrivainGsE().Construire(
                        GsPP::GenerateurX64().Generer(programme), "N::Principal"),
                    "la sortie des opérateurs n'est pas reproductible : " + nom);
                ZoneExecutable zone(AlignerPage(Lire64(contenu, 48)));
                const auto image = GsPP::ChargeurGsE().Charger(contenu, zone.Base());
                Exiger(image.Imports.empty(), "la recherche d'opérateurs ajoute un import d'hôte : " + nom);
                zone.Copier(image.Memoire);
                using FonctionTest = std::int32_t (GS_ABI_HOTE *)();
                Exiger(reinterpret_cast<FonctionTest>(image.AdressePointEntree)() == 42,
                    "résultat d'exécution des opérateurs incorrect : " + nom);
            }
        }
        const std::string masque = "structure P {}; espace N { "
            "publique entier32 opérateur+(P& p, entier32 x) { retourner x; } "
            "classe C { publique: entier32 opérateur+(P& p) { retourner 1; } ";
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {masque + "entier32 Tester(P& p) { retourner p + 42; } }; }", 21},
            {"structure P {}; espace N { publique booléen opérateur!(P& p) { retourner vrai; } "
             "classe C { publique: booléen opérateur!() { retourner faux; } "
             "booléen Tester(P& p) { retourner !p; } }; }", 21},
            {"structure P {}; espace N { publique entier32 opérateur+(P& p, entier32 x) { retourner x; } "
             "classe C { entier32 X = p + 42; publique: constructeur(P& p) {} "
             "entier32 opérateur+(P& p) { retourner 1; } }; }", 21},
            {masque + "entier32 X; constructeur(P& p) : X(p + 42) {} }; }", 21},
            {masque + "constructeur(P& p) { p + 42; } }; }", 21},
            {"structure P {}; espace N { P p; publique entier32 opérateur+(P& p, entier32 x) { retourner x; } "
             "classe C { publique: entier32 opérateur+(P& p) { retourner 1; } destructeur() { p + 42; } }; }", 21},
            {"structure P {}; espace N { publique entier32 opérateur+(P& p, entier32 x) { retourner x; } "
             "classe C { publique: entier32 Tester(P& p) { retourner p + 42; } }; "
             "espace C { publique entier32 opérateur+(P& p, booléen x) { retourner 1; } } }", 21},
            {"structure P {}; espace A { publique entier32 opérateur+(P& p, entier32 x) { retourner x; } } "
             "espace N { utilisant espace A; classe C { publique: entier32 opérateur+(P& p) { retourner 1; } "
             "entier32 Tester(P& p) { retourner p + 42; } }; }", 21},
            {"classe P { privée: entier32 opérateur+(entier32 x) { retourner x; } }; "
             "espace N { classe C { publique: entier32 Tester(P& p) { retourner p + 42; } }; }", 25},
            {"classe P { publique: entier32 opérateur+(entier32 x) { retourner x; } }; "
             "espace N { classe C { publique: entier32 Tester(constante P& p) { retourner p + 42; } }; }", 21},
            {"structure P {}; espace N { classe C { publique: "
             "entier32 Tester(P& p) { retourner p + 42; } }; }", 28},
            {"espace N { classe C { publique: entier32 opérateur+(entier32 x) { retourner x; } "
             "entier32 Tester() { retourner 42 + soi; } }; "
             "publique entier32 opérateur+(entier32 x, C& c) { retourner x; } }", 21},
            {masque + "vide Tester(P& p) { Absente; p + 42; } }; }", 18},
            {masque + "vide Tester(P& p) { p + 42; Absente; } }; }", 21},
            {"espace N { classe B { privée: entier32 opérateur+(entier32 x) { retourner x; } }; "
             "classe C : publique B { publique: entier32 Tester() { retourner soi + 42; } }; }", 25},
            {"espace N { classe C { entier32 X = soi + 42; privée: "
             "entier32 opérateur+(entier32 x) { retourner x; } }; }", 39},
            {"structure P {}; structure Q {}; espace N { classe C { entier32 X = p + 42; "
             "publique: constructeur(P& p) {} constructeur(Q& p) {} }; espace C { "
             "publique entier32 opérateur+(P& p, entier32 x) { retourner x; } } }", 21},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
            for (const auto& texte : {refus[index].first, TraduireCorpusConversions(refus[index].first)})
                ComparerErreurSemantique(syntaxe, semantique, texte, refus[index].second,
                    "operateurs-methodes-refuse-" + std::to_string(index));
        std::cout << "Opérateurs dans les méthodes : " << valides.size()
                  << " corpus bilingues exécutés et cibles de résolution vérifiées.\n";
    }

    /**
     * <résumé>Compare les types qualifiés et conversions lors de la construction des bases et champs.</résumé>
     * @Paramètre(AnalyseurDeclarationsAutoHeberge: syntaxe) AST bilingue intact.
     * @Paramètre(AnalyseurSemantiqueAutoHeberge: semantique) Sélections et diagnostics du frontend Gs++.
     **/
    void TesterQualificationsConstructionsSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe, AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::vector<std::string> valides{
            "espace N { structure P { entier32 X; }; structure Q { entier32 X; }; alias Vue = P; "
            "classe C { constante Q* X = convertir<constante Vue*>(p); publique: constructeur(Q* p) {} "
            "entier32 Lire() { retourner soi.X->X; } }; espace C { alias Vue = Q; } "
            "publique entier32 Principal() { Q q = {42}; C c(&q); retourner c.Lire(); } }",
            "espace N { structure P { entier32 X; }; structure Q { entier32 X; }; alias Vue = P; "
            "structure Donnees { constante Q* Pointeur; }; classe C { Donnees V = {convertir<constante Vue*>(p)}; "
            "publique: constructeur(Q* p) {} entier32 Lire() { retourner soi.V.Pointeur->X; } }; "
            "espace C { alias Vue = Q; } publique entier32 Principal() { Q q = {42}; C c(&q); retourner c.Lire(); } }",
            "espace N { classe M { publique: entier32 X; constructeur(entier32* p) : X(1) {} "
            "constructeur(constante entier32* p) : X(*p) {} }; classe C { M m; publique: "
            "constructeur(constante entier32* p) : m(p) {} entier32 Lire() { retourner soi.m.X; } }; "
            "publique entier32 Principal() { entier32 x = 42; constante entier32* p = convertir<constante entier32*>(&x); "
            "C c(p); retourner c.Lire(); } }",
            "espace N { classe B { protégée: entier32 X; publique: constructeur(entier32* p) : X(1) {} "
            "constructeur(constante entier32* p) : X(*p) {} }; classe C : publique B { publique: "
            "constructeur(constante entier32* p) : parent(p) {} entier32 Lire() { retourner parent.X; } }; "
            "publique entier32 Principal() { entier32 x = 42; constante entier32* p = convertir<constante entier32*>(&x); "
            "C c(p); retourner c.Lire(); } }",
            "espace N { entier32 Valeur = 42; classe C { entier32 X; publique: "
            "constructeur() : soi(convertir<constante entier32*>(&Valeur)) {} "
            "constructeur(entier32* p) : X(1) {} constructeur(constante entier32* p) : X(*p) {} "
            "entier32 Lire() { retourner soi.X; } }; publique entier32 Principal() { C c; retourner c.Lire(); } }",
            "espace N { classe B { protégée: entier32 X; publique: constructeur(entier32& x) : X(1) {} "
            "constructeur(constante entier32& x) : X(x) {} }; classe C : publique B { publique: "
            "constructeur(constante entier32& x) : parent(x) {} entier32 Lire() { retourner parent.X; } }; "
            "publique entier32 Principal() { constante entier32 x = 42; C c(x); retourner c.Lire(); } }",
            "espace N { classe P { publique: entier32 X; }; classe Q : publique P { publique: "
            "virtuel entier32 Tester() { retourner 1; } }; classe M { publique: entier32 X; "
            "constructeur(constante P* p) : X(p->X) {} }; classe C { M m; publique: "
            "constructeur(Q* p) : m(p) {} entier32 Lire() { retourner soi.m.X; } }; "
            "publique entier32 Principal() { Q q; q.X = 42; C c(&q); retourner c.Lire(); } }",
            "espace N { classe P { publique: entier32 X; }; classe Q : publique P { publique: "
            "virtuel entier32 Tester() { retourner 1; } }; classe M { publique: entier32 X; "
            "constructeur(constante P& p) : X(p.X) {} }; classe C { M m; publique: "
            "constructeur(Q& p) : m(p) {} entier32 Lire() { retourner soi.m.X; } }; "
            "publique entier32 Principal() { Q q; q.X = 42; C c(q); retourner c.Lire(); } }",
            "espace N { structure Q { entier32 X; }; classe C { "
            "constante Q* X = convertir<constante Inconnue*>(p); publique: "
            "constructeur(Q* p) : X(convertir<constante Q*>(p)) {} "
            "entier32 Lire() { retourner soi.X->X; } }; "
            "publique entier32 Principal() { Q q = {42}; C c(&q); retourner c.Lire(); } }",
            "espace N { structure Q { entier32 X; }; Q Valeur = {42}; classe C { "
            "constante Q* X = convertir<constante Inconnue*>(p); publique: constructeur() : soi(&Valeur) {} "
            "constructeur(Q* p) : X(convertir<constante Q*>(p)) {} entier32 Lire() { retourner soi.X->X; } }; "
            "publique entier32 Principal() { C c; retourner c.Lire(); } }",
            "espace N { structure P { entier32 X; }; structure Q { entier32 X; }; alias Vue = P; "
            "classe B { protégée: entier32 X; publique: constructeur(constante Q* p) : X(p->X) {} }; "
            "classe C : publique B { publique: constructeur(Q* p) : parent(convertir<constante Vue*>(p)) {} "
            "entier32 Lire() { retourner parent.X; } }; espace C { alias Vue = Q; } "
            "publique entier32 Principal() { Q q = {42}; C c(&q); retourner c.Lire(); } }",
            "espace N { structure P { entier32 X; }; structure Q { entier32 X; }; alias Vue = P; "
            "classe C { constante Q* X; publique: constructeur(Q* p) : soi(convertir<constante Vue*>(p)) {} "
            "constructeur(constante Q* p) : X(p) {} entier32 Lire() { retourner soi.X->X; } }; "
            "espace C { alias Vue = Q; } publique entier32 Principal() { Q q = {42}; C c(&q); retourner c.Lire(); } }",
            "espace N { structure P { entier32 X; }; structure Q { entier32 X; }; alias Vue = P; "
            "classe C { constante Q* X = convertir<constante Vue*>(p); publique: constructeur(Q* p) {} "
            "constructeur(constante Q* p) {} entier32 Lire() { retourner soi.X->X; } }; espace C { alias Vue = Q; } "
            "publique entier32 Principal() { Q q = {42}; constante Q* p = convertir<constante Q*>(&q); C a(&q); C b(p); "
            "retourner a.Lire() + b.Lire() - 42; } }",
            "espace A { structure P { entier32 X; }; alias Vue = P; } espace N { utilisant espace A; "
            "structure Q { entier32 X; }; classe C { constante Q* X = convertir<constante Vue*>(p); "
            "publique: constructeur(Q* p) {} entier32 Lire() { retourner soi.X->X; } }; espace C { alias Vue = Q; } "
            "publique entier32 Principal() { Q q = {42}; C c(&q); retourner c.Lire(); } }",
            "espace N { structure P { entier32 X; }; structure Q { entier32 X; }; alias Vue = P; "
            "publique entier32 Source(constante Q* p) { retourner p->X; } classe C { "
            "pointeur_fonction<entier32(constante Q*)> F = "
            "convertir<pointeur_fonction<entier32(constante Vue*)>>(N::Source); publique: constructeur() {} "
            "entier32 Lire(constante Q* p) { retourner soi.F(p); } }; espace C { alias Vue = Q; } "
            "publique entier32 Principal() { Q q = {42}; C c; retourner c.Lire(convertir<constante Q*>(&q)); } }",
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
        {
            std::optional<GsPP::CodeMachine> reference;
            for (const auto& texte : {valides[index], TraduireCorpusConversions(valides[index])})
            {
                const auto nom = "qualifications-constructions-valide-" + std::to_string(index);
                const auto resultat = AnalyserSemantiqueValide(syntaxe, semantique, texte, nom);
                auto programme = GsPP::AnalyseurSyntaxique(GsPP::Lexeur(texte, nom).Analyser(), nom).Analyser();
                GsPP::AnalyseurSemantique().Analyser(programme);
                struct CibleConstruction { std::uint32_t Genre; const GsPP::Fonction* Fonction; };
                std::unordered_map<std::uint64_t, std::vector<CibleConstruction>> cibles;
                const auto ajouter = [&](const GsPP::PositionSource& position, std::uint32_t genre, const std::string& symbole) {
                    if (symbole.empty()) return;
                    const auto fonction = std::find_if(programme.Fonctions.begin(), programme.Fonctions.end(),
                        [&](const auto& candidate) { return candidate.NomComplet() == symbole; });
                    Exiger(fonction != programme.Fonctions.end() && fonction->EstConstructeur,
                        "cible de construction absente du bootstrap : " + nom);
                    cibles[(static_cast<std::uint64_t>(position.Ligne) << 32U) | position.Colonne].push_back({genre, &*fonction});
                };
                for (const auto& fonction : programme.Fonctions)
                {
                    ajouter(fonction.Position, 512, fonction.SymboleConstructeurDelegue);
                    ajouter(fonction.Position, 1024, fonction.SymboleConstructeurBase);
                    for (const auto& champ : fonction.InitialiseursChamps)
                    {
                        Exiger(!champ.EstImplicite || champ.SymboleConstructeur.empty(),
                            "étendre le comparateur pour les champs implicitement construits : " + nom);
                        ajouter(champ.Position, 2048, champ.SymboleConstructeur);
                    }
                    if (fonction.Corps)
                        for (const auto& instruction : fonction.Corps->Instructions)
                            if (instruction->Genre == GsPP::GenreInstruction::Variable)
                            {
                                const auto& variable = static_cast<const GsPP::InstructionVariable&>(*instruction);
                                ajouter(variable.Position, 0, variable.SymboleConstructeur);
                            }
                }
                std::size_t nombreAttendu = 0;
                for (const auto& [position, attendues] : cibles) nombreAttendu += attendues.size();
                std::size_t nombre = 0;
                for (const auto& resolution : resultat.Resolutions)
                {
                    if ((resolution.Drapeaux & 64U) == 0 || (resolution.Drapeaux & 32768U) != 0) continue;
                    const auto& origine = resultat.Noeuds[resolution.IndexNoeud];
                    const auto cible = cibles.find((static_cast<std::uint64_t>(origine.Ligne) << 32U) | origine.Colonne);
                    Exiger(cible != cibles.end(), "construction publiée absente du bootstrap : " + nom);
                    const auto& declaration = resultat.Noeuds[resultat.Symboles[resolution.IndexSymbole].IndexNoeud];
                    const auto genre = resolution.Drapeaux & (512U | 1024U | 2048U);
                    const auto attendue = std::find_if(cible->second.begin(), cible->second.end(),
                        [&](const auto& referenceCible) {
                            return referenceCible.Genre == genre && declaration.Genre == 13
                                && declaration.Ligne == referenceCible.Fonction->Position.Ligne
                                && declaration.Colonne == referenceCible.Fonction->Position.Colonne
                                && resolution.HachageType == HacherTypeDeclaration(referenceCible.Fonction->TypeRetour);
                        });
                    Exiger(attendue != cible->second.end(), "constructeur ou contexte différent du bootstrap : " + nom);
                    cible->second.erase(attendue);
                    ++nombre;
                }
                Exiger(nombre == nombreAttendu && nombre != 0, "cible de construction omise ou dupliquée : " + nom);
                const auto machine = GsPP::GenerateurX64().Generer(programme);
                if (reference)
                    Exiger(machine.Texte == reference->Texte && machine.Donnees == reference->Donnees,
                        "les conversions de construction produisent des octets bilingues différents : " + nom);
                else reference = machine;
                const auto contenu = GsPP::EcrivainGsE().Construire(machine, "N::Principal");
                Exiger(contenu == GsPP::EcrivainGsE().Construire(
                        GsPP::GenerateurX64().Generer(programme), "N::Principal"),
                    "les constructions qualifiées ne sont pas reproductibles : " + nom);
                ZoneExecutable zone(AlignerPage(Lire64(contenu, 48)));
                const auto image = GsPP::ChargeurGsE().Charger(contenu, zone.Base());
                Exiger(image.Imports.empty(), "les constructions qualifiées ajoutent un import d'hôte : " + nom);
                zone.Copier(image.Memoire);
                using FonctionTest = std::int32_t (GS_ABI_HOTE *)();
                Exiger(reinterpret_cast<FonctionTest>(image.AdressePointEntree)() == 42,
                    "résultat des constructions qualifiées incorrect : " + nom);
            }
        }
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"espace N { structure Q {}; classe C { Q* X = convertir<Inconnue*>(p); "
             "publique: constructeur(Q* p) {} }; }", 99},
            {"espace N { structure Q {}; alias Vue = Q; classe C { Q* X = convertir<Vue*>(p); "
             "publique: constructeur(Q* p) {} }; espace C { entier32 Vue; } }", 99},
            {"espace N { structure P {}; structure Q {}; alias Vue = P; classe C { "
             "constante Vue* X = convertir<constante Vue*>(p); publique: constructeur(Q* p) {} }; "
             "espace C { alias Vue = Q; } }", 37},
            {"classe B { publique: constructeur(entier32* p) {} }; classe C : publique B { "
             "publique: constructeur(constante entier32* p) : parent(p) {} };", 21},
            {"classe M { publique: constructeur(entier32& x) {} }; classe C { M m; publique: "
             "constructeur(constante entier32& x) : m(x) {} };", 21},
            {"classe B { publique: constructeur(constante entier32& x) {} }; classe C : publique B { "
             "publique: constructeur() : parent(42) {} };", 21},
            {"classe P {}; classe Q : publique P {}; classe M { publique: constructeur(P* p) {} }; "
             "classe C { M m; publique: constructeur(constante Q* p) : m(p) {} };", 21},
            {"classe P {}; classe Q : publique P {}; classe M { publique: constructeur(P p) {} }; "
             "classe C { M m; publique: constructeur(Q& p) : m(p) {} };", 21},
            {"classe P {}; classe Q : publique P {}; classe M { publique: constructeur(P** p) {} }; "
             "classe C { M m; publique: constructeur(Q** p) : m(p) {} };", 21},
            {"classe B { privée: constructeur(constante entier32* p) {} }; classe C : publique B { "
             "entier32 X = Absente; publique: constructeur(constante entier32* p) : parent(p) {} };", 26},
            {"classe M { privée: constructeur(constante entier32* p) {} }; classe C { M m; "
             "entier32 X = Absente; publique: constructeur(constante entier32* p) : m(p) {} };", 26},
            {"classe B { publique: constructeur(constante entier32* p) {} }; classe C : publique B { "
             "publique: constructeur() : parent(Absente, 42) {} };", 21},
            {"classe M { publique: constructeur(constante entier32* p) {} }; classe C { M m; "
             "publique: constructeur() : m(Absente, 42) {} };", 21},
            {"structure P { entier32* X; }; classe B { publique: constructeur(P p) {} }; "
             "classe C : publique B { publique: constructeur(constante entier32* p) : parent({p}) {} };", 45},
            {"structure Q {}; classe C { Q* X = p; publique: constructeur(Q* p) {} "
             "constructeur(constante Q* p) {} };", 37},
            {"espace N { structure Q {}; classe C { Q* X = convertir<Vue*>(p); publique: "
             "constructeur(Q* p) { Absente; } }; espace C { entier32 Vue; } }", 99},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
            for (const auto& texte : {refus[index].first, TraduireCorpusConversions(refus[index].first)})
                ComparerErreurSemantique(syntaxe, semantique, texte, refus[index].second,
                    "qualifications-constructions-refuse-" + std::to_string(index));
        std::cout << "Qualifications des constructions : " << valides.size()
                  << " corpus bilingues exécutés et conversions vérifiées.\n";
    }

    void TesterCallbacksChampsContextuelsSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe, AnalyseurSemantiqueAutoHeberge semantique)
    {
        struct Corpus { std::string Texte; std::size_t ReferencesParConstructeur; bool Executer = true; };
        const std::vector<Corpus> valides{
            {"publique entier32 A(entier32 x) { retourner x; } publique entier32 B(entier64 x) { retourner convertir<entier32>(x); } "
             "classe C { entier32 X = rappel(42); publique: constructeur(pointeur_fonction<entier32(entier32)> rappel) {} "
             "constructeur(pointeur_fonction<entier32(entier64)> rappel) {} entier32 Lire() { retourner soi.X; } }; "
             "publique entier32 Principal() { C a(A); C b(B); retourner a.Lire() + b.Lire() - 42; }", 1},
            {"publique entier32 A(entier32 x) { retourner x; } publique entier32 B(entier64 x) { retourner convertir<entier32>(x); } "
             "classe C { entier32 X = (*rappel)(42); publique: constructeur(pointeur_fonction<entier32(entier32)>* rappel) {} "
             "constructeur(pointeur_fonction<entier32(entier64)>* rappel) {} entier32 Lire() { retourner soi.X; } }; "
             "publique entier32 Principal() { pointeur_fonction<entier32(entier32)> x = A; "
             "pointeur_fonction<entier32(entier64)> y = B; C a(&x); C b(&y); retourner a.Lire() + b.Lire() - 42; }", 1},
            {"publique entier32 A(entier32 x) { retourner x; } publique entier32 B(entier64 x) { retourner convertir<entier32>(x); } "
             "classe C { entier32 X = rappel[0](42); publique: constructeur(pointeur_fonction<entier32(entier32)>* rappel) {} "
             "constructeur(pointeur_fonction<entier32(entier64)>* rappel) {} entier32 Lire() { retourner soi.X; } }; "
             "publique entier32 Principal() { pointeur_fonction<entier32(entier32)> x[1] = {A}; "
             "pointeur_fonction<entier32(entier64)> y[1] = {B}; C a(&x[0]); C b(&y[0]); retourner a.Lire() + b.Lire() - 42; }", 1},
            {"publique entier32 A(entier32 x) { retourner x; } publique entier32 B(entier64 x) { retourner convertir<entier32>(x); } "
             "publique pointeur_fonction<entier32(entier32)> ObtenirA() { retourner A; } "
             "publique pointeur_fonction<entier32(entier64)> ObtenirB() { retourner B; } "
             "classe C { entier32 X = fabrique()(42); publique: constructeur(pointeur_fonction<pointeur_fonction<entier32(entier32)>()> fabrique) {} "
             "constructeur(pointeur_fonction<pointeur_fonction<entier32(entier64)>()> fabrique) {} entier32 Lire() { retourner soi.X; } }; "
             "publique entier32 Principal() { C a(ObtenirA); C b(ObtenirB); retourner a.Lire() + b.Lire() - 42; }", 1},
            {"publique entier32 A(entier32 x) { retourner x; } publique entier32 B(entier64 x) { retourner convertir<entier32>(x); } "
             "pointeur_fonction<entier32(entier32)> RA = A; pointeur_fonction<entier32(entier64)> RB = B; "
             "publique pointeur_fonction<entier32(entier32)>* ObtenirA() { retourner &RA; } "
             "publique pointeur_fonction<entier32(entier64)>* ObtenirB() { retourner &RB; } "
             "classe C { entier32 X = (*fabrique())(42); publique: constructeur(pointeur_fonction<pointeur_fonction<entier32(entier32)>*()> fabrique) {} "
             "constructeur(pointeur_fonction<pointeur_fonction<entier32(entier64)>*()> fabrique) {} entier32 Lire() { retourner soi.X; } }; "
             "publique entier32 Principal() { C a(ObtenirA); C b(ObtenirB); retourner a.Lire() + b.Lire() - 42; }", 1},
            {"publique entier32 A(entier32 x) { retourner x; } publique entier32 B(entier64 x) { retourner convertir<entier32>(x); } "
             "classe C { entier32 X = rappel(42); publique: constructeur(pointeur_fonction<entier32(entier32)>& rappel) {} "
             "constructeur(pointeur_fonction<entier32(entier64)>& rappel) {} entier32 Lire() { retourner soi.X; } }; "
             "publique entier32 Principal() { pointeur_fonction<entier32(entier32)> x = A; "
             "pointeur_fonction<entier32(entier64)> y = B; "
             "C a(x); C b(y); retourner a.Lire() + b.Lire() - 42; }", 1},
            {"publique entier32 A(entier32& x) { x = x + 1; retourner x; } classe C { entier32 X = rappel(valeur); "
             "publique: constructeur(pointeur_fonction<entier32(entier32&)> rappel, entier32& valeur) {} "
             "entier32 Lire() { retourner soi.X; } }; publique entier32 Principal() { entier32 x = 41; C a(A, x); "
             "retourner a.Lire() + x - 42; }", 1},
            {"publique entier32 A() { retourner 42; } publique pointeur_fonction<entier32()> ObtenirA(entier32 x) { retourner A; } "
             "publique pointeur_fonction<entier32()> ObtenirB(entier64 x) { retourner A; } structure P { pointeur_fonction<entier32()> F; }; "
             "classe C { P V = {fabrique(42)}; publique: constructeur(pointeur_fonction<pointeur_fonction<entier32()>(entier32)> fabrique) {} "
             "constructeur(pointeur_fonction<pointeur_fonction<entier32()>(entier64)> fabrique) {} entier32 Lire() { retourner soi.V.F(); } }; "
             "publique entier32 Principal() { C a(ObtenirA); C b(ObtenirB); retourner a.Lire() + b.Lire() - 42; }", 1},
            {"publique entier32 A() { retourner 1; } classe C { entier32 X = rappel(Absente); alias Vue = X; "
             "publique: constructeur() : soi(A) {} constructeur(pointeur_fonction<entier32()> rappel) : Vue(42) {} "
             "entier32 Lire() { retourner soi.X; } }; publique entier32 Principal() { C a; retourner a.Lire(); }", 0},
            {"publique entier32 A() { retourner 42; } classe C { pointeur_fonction<entier32()> F[2] = {rappel, rappel}; "
             "publique: constructeur(pointeur_fonction<entier32()> rappel) {} "
             "constructeur(pointeur_fonction<entier32()>& rappel, entier32 marqueur) {} "
             "entier32 Lire() { retourner soi.F[0]() + soi.F[1]() - 42; } }; "
             "publique entier32 Principal() { pointeur_fonction<entier32()> x = A; C a(A); C b(x, 0); "
             "retourner a.Lire() + b.Lire() - 42; }", 2},
            {"classe C { entier32 X = rappel(42); publique: constructeur(constante pointeur_fonction<entier32(entier32)>& rappel) {} "
             "constructeur(volatile pointeur_fonction<entier32(entier32)>& rappel) {} entier32 Lire() { retourner soi.X; } }; "
             "publique entier32 Verifier(constante pointeur_fonction<entier32(entier32)>& x, "
             "volatile pointeur_fonction<entier32(entier32)>& y) { C a(x); C b(y); retourner a.Lire() + b.Lire() - 42; }", 1, false},
            {"structure P { entier32 X; }; structure Q { entier64 X; }; "
             "publique entier32 A(P p) { retourner p.X; } publique entier32 B(Q p) { retourner convertir<entier32>(p.X); } "
             "classe C { entier32 X = rappel({42}); publique: constructeur(pointeur_fonction<entier32(P)> rappel) {} "
             "constructeur(pointeur_fonction<entier32(Q)> rappel) {} entier32 Lire() { retourner soi.X; } }; "
             "publique entier32 Principal() { C a(A); C b(B); retourner a.Lire() + b.Lire() - 42; }", 1},
            {"structure P { entier32 X[2]; }; structure Q { entier64 X[2]; }; "
             "publique entier32 A(P p) { retourner p.X[0] + p.X[1]; } "
             "publique entier32 B(Q p) { retourner convertir<entier32>(p.X[0] + p.X[1]); } "
             "classe C { entier32 X = rappel({{20, 22}}); publique: constructeur(pointeur_fonction<entier32(P)> rappel) {} "
             "constructeur(pointeur_fonction<entier32(Q)> rappel) {} entier32 Lire() { retourner soi.X; } }; "
             "publique entier32 Principal() { C a(A); C b(B); retourner a.Lire() + b.Lire() - 42; }", 1},
            {"structure P { entier32 X; }; structure Q { entier64 X; }; "
             "publique entier32 A(P p) { retourner p.X; } publique entier32 B(Q p) { retourner convertir<entier32>(p.X); } "
             "publique pointeur_fonction<entier32(P)> ObtenirA() { retourner A; } "
             "publique pointeur_fonction<entier32(Q)> ObtenirB() { retourner B; } "
             "classe C { entier32 X = fabrique()({42}); publique: constructeur(pointeur_fonction<pointeur_fonction<entier32(P)>()> fabrique) {} "
             "constructeur(pointeur_fonction<pointeur_fonction<entier32(Q)>()> fabrique) {} entier32 Lire() { retourner soi.X; } }; "
             "publique entier32 Principal() { C a(ObtenirA); C b(ObtenirB); retourner a.Lire() + b.Lire() - 42; }", 1},
            {"structure P { constante entier32* X; }; structure Q { constante entier32* X; }; "
             "publique entier32 A(P p) { retourner *p.X; } publique entier32 B(Q p) { retourner *p.X; } "
             "classe C { entier32 X = rappel({convertir<constante entier32*>(adresse)}); publique: "
             "constructeur(pointeur_fonction<entier32(P)> rappel, entier32* adresse) {} "
             "constructeur(pointeur_fonction<entier32(Q)> rappel, entier32* adresse) {} entier32 Lire() { retourner soi.X; } }; "
             "publique entier32 Principal() { entier32 x = 42; C a(A, &x); C b(B, &x); retourner a.Lire() + b.Lire() - 42; }", 1},
            {"publique entier32 Identite(entier32 x) { retourner x; } structure P { pointeur_fonction<entier32(entier32)> F[2]; }; "
             "structure Q { pointeur_fonction<entier32(entier32)> F[2]; entier32 Z; }; "
             "publique entier32 A(P p) { retourner p.F[0](42) + p.F[1](42) - 42; } "
             "publique entier32 B(Q p) { retourner p.F[0](42) + p.F[1](42) - 42 + p.Z; } "
             "classe C { entier32 X = rappel({{Identite, Identite}}); publique: constructeur(pointeur_fonction<entier32(P)> rappel) {} "
             "constructeur(pointeur_fonction<entier32(Q)> rappel) {} entier32 Lire() { retourner soi.X; } }; "
             "publique entier32 Principal() { C a(A); C b(B); retourner a.Lire() + b.Lire() - 42; }", 1},
            {"structure P { entier32 X; }; structure Q { entier64 X; }; "
             "publique entier32 A(P p) { retourner p.X; } publique entier64 B(Q p) { retourner p.X; } "
             "classe M { publique: entier32 X; constructeur(entier32 x) : X(x) {} constructeur(entier64 x) : X(convertir<entier32>(x)) {} }; "
             "classe C { M m; publique: constructeur(pointeur_fonction<entier32(P)> rappel) : m(rappel({42})) {} "
             "constructeur(pointeur_fonction<entier64(Q)> rappel) : m(rappel({42})) {} entier32 Lire() { retourner soi.m.X; } }; "
             "publique entier32 Principal() { C a(A); C b(B); retourner a.Lire() + b.Lire() - 42; }", 0},
            {"structure P { entier32 X; }; structure Q { entier64 X; }; "
             "publique entier32 A(P p) { retourner p.X; } publique entier64 B(Q p) { retourner p.X; } "
             "classe Base { protégée: entier32 X; publique: constructeur(entier32 x) : X(x) {} "
             "constructeur(entier64 x) : X(convertir<entier32>(x)) {} }; classe C : publique Base { publique: "
             "constructeur(pointeur_fonction<entier32(P)> rappel) : parent(rappel({42})) {} "
             "constructeur(pointeur_fonction<entier64(Q)> rappel) : parent(rappel({42})) {} entier32 Lire() { retourner parent.X; } }; "
             "publique entier32 Principal() { C a(A); C b(B); retourner a.Lire() + b.Lire() - 42; }", 0},
            {"structure P { entier32 X; }; structure Q { entier64 X; }; "
             "publique entier32 A(P p) { retourner p.X; } publique entier64 B(Q p) { retourner p.X; } "
             "classe C { entier32 X; publique: constructeur(entier32 x) : X(x) {} "
             "constructeur(entier64 x) : X(convertir<entier32>(x)) {} "
             "constructeur(pointeur_fonction<entier32(P)> rappel) : soi(rappel({42})) {} "
             "constructeur(pointeur_fonction<entier64(Q)> rappel) : soi(rappel({42})) {} entier32 Lire() { retourner soi.X; } }; "
             "publique entier32 Principal() { C a(A); C b(B); retourner a.Lire() + b.Lire() - 42; }", 0},
            {"structure P { entier32 X; }; structure Q { entier64 X; }; classe C { entier32 X = fabrique()({42}); publique: "
             "constructeur(pointeur_fonction<pointeur_fonction<entier32(P)>&()> fabrique) {} "
             "constructeur(pointeur_fonction<pointeur_fonction<entier32(Q)>&()> fabrique) {} entier32 Lire() { retourner soi.X; } }; "
             "publique entier32 Verifier(pointeur_fonction<pointeur_fonction<entier32(P)>&()> x, "
             "pointeur_fonction<pointeur_fonction<entier32(Q)>&()> y) { C a(x); C b(y); retourner a.Lire() + b.Lire() - 42; }", 1, false},
            {"publique booléen A(entier32 x) { retourner x == 42; } publique booléen B(entier64 x) { retourner x == 42; } "
             "classe C { booléen X = rappel({42}); publique: constructeur(pointeur_fonction<booléen(entier32)> rappel) {} "
             "constructeur(pointeur_fonction<booléen(entier64)> rappel) {} "
             "entier32 Lire() { retourner convertir<entier32>(soi.X) + 41; } }; "
             "publique entier32 Principal() { C a(A); C b(B); retourner a.Lire() + b.Lire() - 42; }", 1},
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
        {
            std::optional<GsPP::CodeMachine> reference;
            for (const auto& texte : {valides[index].Texte, TraduireCorpusConversions(valides[index].Texte)})
            {
                const auto nom = "callbacks-champs-contextuels-valide-" + std::to_string(index);
                const auto resultat = AnalyserSemantiqueValide(syntaxe, semantique, texte, nom);
                auto programme = GsPP::AnalyseurSyntaxique(GsPP::Lexeur(texte, nom).Analyser(), nom).Analyser();
                GsPP::AnalyseurSemantique().Analyser(programme);
                struct ParametreAttendu { std::uint64_t Type; std::size_t Nombre; };
                std::unordered_map<std::uint64_t, ParametreAttendu> attendus;
                for (const auto& fonction : programme.Fonctions)
                    if (fonction.EstConstructeur)
                        for (const auto& parametre : fonction.Parametres)
                            if (parametre.Nom == "rappel" || parametre.Nom == "fabrique")
                                attendus.emplace((static_cast<std::uint64_t>(parametre.Position.Ligne) << 32U) | parametre.Position.Colonne,
                                    ParametreAttendu{HacherTypeDeclaration(parametre.Type), valides[index].ReferencesParConstructeur});
                std::size_t nombre = 0;
                for (const auto& resolution : resultat.Resolutions)
                {
                    auto ancetre = resolution.IndexNoeud;
                    const auto& origine = resultat.Noeuds[ancetre];
                    if (origine.Genre != 24 || (origine.HachageNom != HacherTexte("rappel")
                            && origine.HachageNom != HacherTexte("fabrique"))) continue;
                    while (ancetre != 0 && ancetre < resultat.Noeuds.size() && resultat.Noeuds[ancetre].Genre != 7)
                        ancetre = resultat.Noeuds[ancetre].Parent;
                    if (ancetre == 0 || ancetre >= resultat.Noeuds.size()) continue;
                    const auto& symbole = resultat.Symboles[resolution.IndexSymbole];
                    const auto& declaration = resultat.Noeuds[symbole.IndexNoeud];
                    const auto attendu = attendus.find((static_cast<std::uint64_t>(declaration.Ligne) << 32U) | declaration.Colonne);
                    Exiger(symbole.Genre == 8 && declaration.Genre == 2 && attendu != attendus.end()
                            && symbole.HachageType == attendu->second.Type && attendu->second.Nombre != 0,
                        "le callback du champ réutilise le type ou le paramètre d'un autre constructeur : " + nom);
                    --attendu->second.Nombre;
                    ++nombre;
                }
                Exiger(!attendus.empty() && std::all_of(attendus.begin(), attendus.end(),
                        [](const auto& attendu) { return attendu.second.Nombre == 0; }),
                    "résolution de callback de champ omise ou dupliquée : " + nom);
                Exiger(valides[index].ReferencesParConstructeur == 0 || nombre != 0,
                    "callback contextuel absent : " + nom);
                if (!valides[index].Executer) continue;
                const auto machine = GsPP::GenerateurX64().Generer(programme);
                if (reference)
                    Exiger(machine.Texte == reference->Texte && machine.Donnees == reference->Donnees,
                        "octets des callbacks de champs différents entre français et anglais : " + nom);
                else reference = machine;
                const auto contenu = GsPP::EcrivainGsE().Construire(machine, "Principal");
                Exiger(contenu == GsPP::EcrivainGsE().Construire(GsPP::GenerateurX64().Generer(programme), "Principal"),
                    "image des callbacks de champs non reproductible : " + nom);
                ZoneExecutable zone(AlignerPage(Lire64(contenu, 48)));
                const auto image = GsPP::ChargeurGsE().Charger(contenu, zone.Base());
                Exiger(image.Imports.empty(), "import d'hôte ajouté par les callbacks de champs : " + nom);
                zone.Copier(image.Memoire);
                using FonctionTest = std::int32_t (GS_ABI_HOTE *)();
                Exiger(reinterpret_cast<FonctionTest>(image.AdressePointEntree)() == 42,
                    "résultat des callbacks de champs incorrect : " + nom);
            }
        }
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"classe C { entier32 X = rappel(42); publique: constructeur(pointeur_fonction<entier32(entier32)> rappel) {} "
             "constructeur(pointeur_fonction<entier32(booléen)> rappel) {} };", 55},
            {"classe C { entier32 X = rappel(42); publique: constructeur(pointeur_fonction<entier32(booléen)> rappel) {} "
             "constructeur(pointeur_fonction<entier32(entier32)> rappel) {} };", 55},
            {"classe C { entier32 X = fabrique()(42); publique: constructeur(pointeur_fonction<pointeur_fonction<entier32(entier32)>()> fabrique) {} "
             "constructeur(pointeur_fonction<entier32()> fabrique) {} };", 53},
            {"classe C { entier32 X = rappel(Absente); publique: constructeur(pointeur_fonction<entier32(entier32, entier32)> rappel) {} };", 54},
            {"classe C { entier32 X = rappel(42); publique: constructeur(pointeur_fonction<entier32(entier32)> rappel) {} "
             "constructeur(pointeur_fonction<entier32()> rappel) {} };", 54},
            {"classe C { entier32 X = (*rappel)(42); publique: constructeur(pointeur_fonction<entier32(entier32)>* rappel) {} "
             "constructeur(entier32* rappel) {} };", 53},
            {"classe C { pointeur_fonction<entier32()> F[2] = {rappel, rappel}; publique: "
             "constructeur(pointeur_fonction<entier32()> rappel) {} constructeur(pointeur_fonction<vide()> rappel) {} };", 45},
            {"publique vide A() {} classe C { entier32 X = rappel(A); publique: "
             "constructeur(pointeur_fonction<entier32(pointeur_fonction<vide()>&)> rappel) {} };", 55},
            {"classe C { entier32 X = rappel(valeur); publique: constructeur(pointeur_fonction<entier32(entier32&)> rappel, "
             "constante entier32& valeur) {} };", 55},
            {"classe C { pointeur_fonction<entier32(entier64)> F = convertir<pointeur_fonction<entier32(entier64)>>(rappel); "
             "publique: constructeur(pointeur_fonction<entier32(entier32)> rappel) {} };", 97},
            {"classe C { entier32 X = rappel(42); publique: constructeur(pointeur_fonction<entier32(entier32)> rappel) {} "
             "constructeur() {} };", 18},
            {"classe C { entier32 X = rappel(Absente); publique: constructeur(pointeur_fonction<entier32()> rappel) "
             ": X(convertir<vide>(0)) {} };", 94},
            {"classe C { entier32 X = rappel(42); publique: constructeur(pointeur_fonction<entier32(booléen)> rappel) { Absente; } };", 55},
            {"classe C { entier32 X = rappel(convertir<Inconnue>(42)); publique: "
             "constructeur(pointeur_fonction<entier32(entier32)> rappel) { Absente; } };", 99},
            {"structure P { entier32 X; }; structure Q { booléen X; }; classe C { entier32 X = rappel({42}); publique: "
             "constructeur(pointeur_fonction<entier32(P)> rappel) {} constructeur(pointeur_fonction<entier32(Q)> rappel) {} };", 45},
            {"structure P { entier32 X; }; structure Q { booléen X; }; classe C { entier32 X = rappel({42}); publique: "
             "constructeur(pointeur_fonction<entier32(Q)> rappel) {} constructeur(pointeur_fonction<entier32(P)> rappel) {} };", 45},
            {"structure P { entier32 X[2]; }; structure Q { entier32 X[1]; }; classe C { entier32 X = rappel({{20, 22}}); publique: "
             "constructeur(pointeur_fonction<entier32(P)> rappel) {} constructeur(pointeur_fonction<entier32(Q)> rappel) {} };", 42},
            {"structure P { entier32 X[2]; }; structure Q { entier32 X[1]; }; classe C { entier32 X = rappel({{20, 22}}); publique: "
             "constructeur(pointeur_fonction<entier32(Q)> rappel) {} constructeur(pointeur_fonction<entier32(P)> rappel) {} };", 42},
            {"structure P { entier32 X; }; structure Q { booléen X; }; classe C { entier32 X = fabrique()({42}); publique: "
             "constructeur(pointeur_fonction<pointeur_fonction<entier32(P)>()> fabrique) {} "
             "constructeur(pointeur_fonction<pointeur_fonction<entier32(Q)>()> fabrique) {} };", 45},
            {"structure P { entier32 X; }; classe C { entier32 X = rappel({Absente}); publique: "
             "constructeur(pointeur_fonction<entier32(P&)> rappel) {} };", 69},
            {"structure P { entier32* X; }; classe C { entier32 X = rappel({p}); publique: "
             "constructeur(pointeur_fonction<entier32(P)> rappel, constante entier32* p) {} };", 45},
            {"publique entier32 A(entier32 x) { retourner x; } structure P { pointeur_fonction<entier32(entier64)> F; }; "
             "classe C { entier32 X = rappel({convertir<pointeur_fonction<entier32(entier64)>>(A)}); publique: "
             "constructeur(pointeur_fonction<entier32(P)> rappel) {} };", 97},
            {"structure P { entier32 X; }; classe M { publique: constructeur(entier32 x) {} }; classe C { M m; publique: "
             "constructeur(pointeur_fonction<entier32(P)> rappel) : m(rappel({Absente, 22})) { Absente; } };", 43},
            {"structure P { booléen X; }; classe Base { privée: constructeur(entier32 x) {} }; classe C : publique Base { publique: "
             "constructeur(pointeur_fonction<entier32(P)> rappel) : parent(rappel({42})) { Absente; } };", 45},
            {"structure P { entier32 X; }; classe C { publique: constructeur(entier32 x) {} "
             "constructeur(pointeur_fonction<entier32(P)> rappel) : soi(rappel({Absente, 22})) { Absente; } };", 43},
            {"structure P { entier32 X; }; classe M { publique: constructeur(entier32 x) {} }; classe C { M m; publique: "
             "constructeur(pointeur_fonction<vide(P)> rappel) : m(rappel({42})) {} };", 21},
            {"structure P { entier32 X; }; classe C { entier32 X = fabrique()({Absente}); publique: "
             "constructeur(pointeur_fonction<P*()> fabrique) {} };", 53},
            {"structure P { entier32 X; }; classe C { entier32 X = rappel({vrai}); entier32 Y = Absente; publique: "
             "constructeur(pointeur_fonction<entier32(P)> rappel) {} };", 45},
            {"structure P { entier32 X; }; classe C { entier32 Y = Absente; entier32 X = rappel({vrai}); publique: "
             "constructeur(pointeur_fonction<entier32(P)> rappel) {} };", 18},
            {"structure P { entier32 X; }; classe C { entier32 X = Absente; publique: "
             "constructeur(pointeur_fonction<entier32(P)> rappel) : X(rappel({vrai})) { Absente; } };", 45},
            {"structure P { booléen X; }; publique entier32 Lire(P p) { retourner 42; } "
             "classe C { entier32 X = Lire({42}); publique: constructeur() {} };", 45},
            {"structure P { booléen X; }; publique entier32 Lire(P p) { retourner 42; } "
             "classe C { entier32 X = convertir<entier32>(Lire({42})); publique: constructeur() {} };", 45},
            {"classe C { entier32 X = rappel({vrai}); publique: "
             "constructeur(pointeur_fonction<entier32(entier32)> rappel) { Absente; } };", 45},
            {"classe C { entier32 X = fabrique()({vrai}); publique: "
             "constructeur(pointeur_fonction<pointeur_fonction<entier32(entier32)>()> fabrique) {} };", 45},
            {"classe C { booléen X = rappel({42}); publique: "
             "constructeur(pointeur_fonction<entier32(entier32)> rappel) {} };", 37},
            {"structure P { entier32 X; }; publique entier32 Lire(P p) { retourner p.X; } "
             "classe C { booléen X = Lire({42}); publique: constructeur() {} };", 37},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
            for (const auto& texte : {refus[index].first, TraduireCorpusConversions(refus[index].first)})
                ComparerErreurSemantique(syntaxe, semantique, texte, refus[index].second,
                    "callbacks-champs-contextuels-refuse-" + std::to_string(index));
        const auto nombreExecutes = std::count_if(valides.begin(), valides.end(), [](const auto& corpus) { return corpus.Executer; });
        std::cout << "Callbacks et agrégats des constructions : " << nombreExecutes << " corpus bilingues exécutés et "
                  << valides.size() - nombreExecutes << " corpus bilingues sémantiques, types et paramètres vérifiés.\n";
    }

    using CallbackReferenceValeurHote = std::int32_t (GS_ABI_HOTE *)();

    struct DonneesReferencesCallbacksHote
    {
        std::int32_t X;
        std::int32_t Y[2];
        CallbackReferenceValeurHote F;
    };

    std::uint32_t NombreLecturesReferencesCallbacksHote = 0;

    std::int32_t GS_ABI_HOTE CibleReferenceCallbackHote() { return 42; }
    std::int32_t& GS_ABI_HOTE LireReferenceEntierHote(DonneesReferencesCallbacksHote* donnees) {
        ++NombreLecturesReferencesCallbacksHote;
        return donnees->X;
    }
    const std::int32_t& GS_ABI_HOTE LireReferenceEntierConstantHote(DonneesReferencesCallbacksHote* donnees) {
        ++NombreLecturesReferencesCallbacksHote;
        return donnees->X;
    }
    volatile std::int32_t& GS_ABI_HOTE LireReferenceEntierVolatileHote(DonneesReferencesCallbacksHote* donnees) {
        ++NombreLecturesReferencesCallbacksHote;
        return donnees->X;
    }
    DonneesReferencesCallbacksHote& GS_ABI_HOTE LireReferenceAgregeeHote(DonneesReferencesCallbacksHote* donnees) {
        ++NombreLecturesReferencesCallbacksHote;
        return *donnees;
    }
    const DonneesReferencesCallbacksHote& GS_ABI_HOTE LireReferenceAgregeeConstanteHote(DonneesReferencesCallbacksHote* donnees) {
        ++NombreLecturesReferencesCallbacksHote;
        return *donnees;
    }
    volatile DonneesReferencesCallbacksHote& GS_ABI_HOTE LireReferenceAgregeeVolatileHote(DonneesReferencesCallbacksHote* donnees) {
        ++NombreLecturesReferencesCallbacksHote;
        return *donnees;
    }
    const volatile DonneesReferencesCallbacksHote& GS_ABI_HOTE LireReferenceAgregeeConstanteVolatileHote(DonneesReferencesCallbacksHote* donnees) {
        ++NombreLecturesReferencesCallbacksHote;
        return *donnees;
    }
    const volatile std::int32_t& GS_ABI_HOTE LireReferenceEntierConstantVolatileHote(DonneesReferencesCallbacksHote* donnees) {
        ++NombreLecturesReferencesCallbacksHote;
        return donnees->X;
    }
    CallbackReferenceValeurHote& GS_ABI_HOTE LireReferenceCallbackHote(DonneesReferencesCallbacksHote* donnees) {
        ++NombreLecturesReferencesCallbacksHote;
        return donnees->F;
    }

    /**
     * Compare les signatures de callbacks retournant une référence, puis leur
     * lecture/adressage machine avec des callbacks C++ respectant l'ABI Gs++.
     * Cela ne définit pas de fonction ordinaire Gs++ retournant une référence.
     **/
    void TesterRetoursReferencesCallbacks(
        AnalyseurDeclarationsAutoHeberge syntaxe, AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::string declarations =
            "structure P { entier32 X; entier32 Y[2]; pointeur_fonction<entier32()> F; }; ";
        struct Corpus {
            std::string Texte;
            std::uint64_t Callback;
            std::int32_t X;
            std::int32_t Y1 = 0;
            std::uint32_t NombreLectures = 1;
        };
        const auto entier = reinterpret_cast<std::uint64_t>(&LireReferenceEntierHote);
        const auto agrege = reinterpret_cast<std::uint64_t>(&LireReferenceAgregeeHote);
        const auto rappel = reinterpret_cast<std::uint64_t>(&LireReferenceCallbackHote);
        const std::vector<Corpus> valides{
            {"publique entier32 Principal(pointeur_fonction<entier32&(P*)> lire, P* donnees) { retourner lire(donnees) + 1; }", entier, 41},
            {"publique entier32 Principal(pointeur_fonction<entier32&(P*)> lire, P* donnees) { "
             "entier32& liaison = lire(donnees); liaison = 42; retourner liaison; }", entier, 42},
            {"publique entier32 Principal(pointeur_fonction<entier32&(P*)> lire, P* donnees) { "
             "lire(donnees) = 42; retourner lire(donnees); }", entier, 42, 0, 2},
            {"publique entier32 Principal(pointeur_fonction<entier32&(P*)> lire, P* donnees) { "
             "entier32* adresse = &lire(donnees); *adresse = 42; retourner lire(donnees); }", entier, 42, 0, 2},
            {"classe C { entier32 X; publique: constructeur(entier32& x) : X(x + 1) {} entier32 Lire() { retourner soi.X; } }; "
             "publique entier32 Principal(pointeur_fonction<entier32&(P*)> lire, P* donnees) { C c(lire(donnees)); retourner c.Lire(); }", entier, 41},
            {"publique entier32 Principal(pointeur_fonction<P&(P*)> lire, P* donnees) { "
             "P& liaison = lire(donnees); liaison.X = 42; retourner liaison.X; }", agrege, 42},
            {"publique entier32 Principal(pointeur_fonction<P&(P*)> lire, P* donnees) { "
             "lire(donnees).Y[1] = 2; retourner lire(donnees).X + lire(donnees).Y[1] - 1; }", agrege, 41, 2, 3},
            {"publique entier32 Principal(pointeur_fonction<constante entier32&(P*)> lire, P* donnees) { "
             "constante entier32& liaison = lire(donnees); retourner liaison + 1; }",
             reinterpret_cast<std::uint64_t>(&LireReferenceEntierConstantHote), 41},
            {"publique entier32 Principal(pointeur_fonction<volatile entier32&(P*)> lire, P* donnees) { "
             "volatile entier32& liaison = lire(donnees); liaison = 42; retourner liaison; }",
             reinterpret_cast<std::uint64_t>(&LireReferenceEntierVolatileHote), 42},
            {"publique entier32 Principal(pointeur_fonction<pointeur_fonction<entier32()>&(P*)> lire, P* donnees) { "
             "pointeur_fonction<entier32()>& liaison = lire(donnees); retourner liaison(); }", rappel, 41},
            {"publique entier32 Principal(pointeur_fonction<pointeur_fonction<entier32()>&(P*)> lire, P* donnees) { "
             "retourner lire(donnees)(); }", rappel, 41},
            {"classe C { entier32 X = lire(donnees).X + 1; publique: constructeur(pointeur_fonction<P&(P*)> lire, P* donnees) {} "
             "entier32 Lire() { retourner soi.X; } }; publique entier32 Principal(pointeur_fonction<P&(P*)> lire, P* donnees) { "
             "C c(lire, donnees); retourner c.Lire(); }", agrege, 41},
            {"publique entier32 Principal(pointeur_fonction<constante entier32&(P*)> lire, P* donnees) { "
             "constante entier32* adresse = &lire(donnees); retourner *adresse + 1; }",
             reinterpret_cast<std::uint64_t>(&LireReferenceEntierConstantHote), 41},
            {"publique entier32 Principal(pointeur_fonction<constante P&(P*)> lire, P* donnees) { "
             "constante entier32* adresse = &lire(donnees).X; retourner *adresse + 1; }",
             reinterpret_cast<std::uint64_t>(&LireReferenceAgregeeConstanteHote), 41},
            {"publique entier32 Principal(pointeur_fonction<constante P&(P*)> lire, P* donnees) { "
             "constante entier32* adresse = &lire(donnees).Y[1]; retourner *adresse + 42; }",
             reinterpret_cast<std::uint64_t>(&LireReferenceAgregeeConstanteHote), 41},
            {"publique entier32 Principal(pointeur_fonction<volatile entier32&(P*)> lire, P* donnees) { "
             "volatile entier32* adresse = &lire(donnees); retourner *adresse + 1; }",
             reinterpret_cast<std::uint64_t>(&LireReferenceEntierVolatileHote), 41},
            {"publique entier32 Principal(pointeur_fonction<volatile P&(P*)> lire, P* donnees) { "
             "volatile entier32* adresse = &lire(donnees).X; *adresse = 42; retourner *adresse; }",
             reinterpret_cast<std::uint64_t>(&LireReferenceAgregeeVolatileHote), 42},
            {"publique entier32 Principal(pointeur_fonction<volatile P&(P*)> lire, P* donnees) { "
             "volatile entier32* adresse = &lire(donnees).Y[1]; *adresse = 2; retourner *adresse + 40; }",
             reinterpret_cast<std::uint64_t>(&LireReferenceAgregeeVolatileHote), 41, 2},
            {"publique entier32 Principal(pointeur_fonction<constante volatile entier32&(P*)> lire, P* donnees) { "
             "constante volatile entier32* adresse = &lire(donnees); retourner *adresse + 1; }",
             reinterpret_cast<std::uint64_t>(&LireReferenceEntierConstantVolatileHote), 41},
            {"publique entier32 Principal(pointeur_fonction<constante volatile P&(P*)> lire, P* donnees) { "
             "constante volatile entier32* adresse = &lire(donnees).X; retourner *adresse + 1; }",
             reinterpret_cast<std::uint64_t>(&LireReferenceAgregeeConstanteVolatileHote), 41},
            {"publique entier32 Principal(pointeur_fonction<constante volatile P&(P*)> lire, P* donnees) { "
             "constante volatile entier32* adresse = &lire(donnees).Y[1]; retourner *adresse + 42; }",
             reinterpret_cast<std::uint64_t>(&LireReferenceAgregeeConstanteVolatileHote), 41},
            {"publique entier32 Principal(pointeur_fonction<volatile P&(P*)> lire, P* donnees) { "
             "volatile P* objet = &lire(donnees); volatile entier32* adresse = &objet->X; retourner *adresse + 1; }",
             reinterpret_cast<std::uint64_t>(&LireReferenceAgregeeVolatileHote), 41},
            {"publique entier32 Principal(pointeur_fonction<constante volatile P&(P*)> lire, P* donnees) { "
             "constante volatile P* objet = &lire(donnees); constante volatile entier32* adresse = &objet->Y[1]; "
             "retourner *adresse + 42; }", reinterpret_cast<std::uint64_t>(&LireReferenceAgregeeConstanteVolatileHote), 41},
            {"publique entier32 Principal(pointeur_fonction<volatile P&(P*)> lire, P* donnees) { "
             "pointeur_fonction<entier32()> rappel = lire(donnees).F; retourner rappel(); }",
             reinterpret_cast<std::uint64_t>(&LireReferenceAgregeeVolatileHote), 41},
            {"publique entier32 Principal(pointeur_fonction<constante volatile P&(P*)> lire, P* donnees) { "
             "pointeur_fonction<entier32()> rappel = lire(donnees).F; retourner rappel(); }",
             reinterpret_cast<std::uint64_t>(&LireReferenceAgregeeConstanteVolatileHote), 41},
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
        {
            std::optional<GsPP::CodeMachine> reference;
            const auto source = declarations + valides[index].Texte;
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
            {
                const auto nom = "retour-reference-callback-valide-" + std::to_string(index);
                const auto resultat = AnalyserSemantiqueValide(syntaxe, semantique, texte, nom);
                auto programme = GsPP::AnalyseurSyntaxique(GsPP::Lexeur(texte, nom).Analyser(), nom).Analyser();
                GsPP::AnalyseurSemantique().Analyser(programme);
                const auto structure = std::find_if(programme.Structures.begin(), programme.Structures.end(),
                    [](const auto& candidate) { return candidate.NomComplet() == "P"; });
                Exiger(structure != programme.Structures.end() && structure->Taille == sizeof(DonneesReferencesCallbacksHote)
                        && structure->Alignement == alignof(DonneesReferencesCallbacksHote) && structure->Champs.size() == 3
                        && structure->Champs[0].Decalage == offsetof(DonneesReferencesCallbacksHote, X)
                        && structure->Champs[1].Decalage == offsetof(DonneesReferencesCallbacksHote, Y)
                        && structure->Champs[2].Decalage == offsetof(DonneesReferencesCallbacksHote, F),
                    "la disposition du pont de test ne respecte pas la structure Gs++ : " + nom);
                std::unordered_map<std::uint64_t, std::uint64_t> types;
                for (const auto& fonction : programme.Fonctions)
                    for (const auto& parametre : fonction.Parametres)
                        if (parametre.Nom != "soi")
                            types.emplace((static_cast<std::uint64_t>(parametre.Position.Ligne) << 32U) | parametre.Position.Colonne,
                                HacherTypeDeclaration(parametre.Type));
                std::size_t nombreParametres = 0;
                for (const auto& symbole : resultat.Symboles)
                    if (symbole.Genre == 8)
                    {
                        ++nombreParametres;
                        const auto& declaration = resultat.Noeuds[symbole.IndexNoeud];
                        const auto type = types.find((static_cast<std::uint64_t>(declaration.Ligne) << 32U) | declaration.Colonne);
                        Exiger(type != types.end() && symbole.HachageType == type->second,
                            "signature de retour par référence différente du bootstrap : " + nom);
                    }
                Exiger(nombreParametres == types.size(), "paramètres de callbacks manquants : " + nom);
                const auto machine = GsPP::GenerateurX64().Generer(programme);
                if (reference)
                    Exiger(machine.Texte == reference->Texte && machine.Donnees == reference->Donnees,
                        "octets bilingues des retours par référence différents : " + nom);
                else reference = machine;
                const auto contenu = GsPP::EcrivainGsE().Construire(machine, "Principal");
                Exiger(contenu == GsPP::EcrivainGsE().Construire(GsPP::GenerateurX64().Generer(programme), "Principal"),
                    "image des retours par référence non reproductible : " + nom);
                ZoneExecutable zone(AlignerPage(Lire64(contenu, 48)));
                const auto image = GsPP::ChargeurGsE().Charger(contenu, zone.Base());
                Exiger(image.Imports.empty(), "les callbacks fournis par argument ajoutent un import : " + nom);
                zone.Copier(image.Memoire);
                DonneesReferencesCallbacksHote donnees{41, {0, 0}, &CibleReferenceCallbackHote};
                using FonctionTest = std::int32_t (GS_ABI_HOTE *)(std::uint64_t, DonneesReferencesCallbacksHote*);
                NombreLecturesReferencesCallbacksHote = 0;
                const auto valeur = reinterpret_cast<FonctionTest>(image.AdressePointEntree)(valides[index].Callback, &donnees);
                Exiger(valeur == 42, "résultat du retour par référence incorrect : " + nom + " (obtenu " + std::to_string(valeur) + ")");
                Exiger(donnees.X == valides[index].X && donnees.Y[0] == 0 && donnees.Y[1] == valides[index].Y1
                        && donnees.F == &CibleReferenceCallbackHote,
                    "le retour par référence ne lit/modifie pas le stockage fourni par l'hôte : " + nom);
                Exiger(NombreLecturesReferencesCallbacksHote == valides[index].NombreLectures,
                    "l'adressage ou la lecture du retour par référence réévalue le callback : " + nom);
            }
        }
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"publique vide F(pointeur_fonction<constante entier32&(P*)> lire, P* p) { entier32& x = lire(p); }", 69},
            {"publique vide F(pointeur_fonction<constante P&(P*)> lire, P* p) { P& x = lire(p); }", 69},
            {"publique vide F(pointeur_fonction<constante entier32&(P*)> lire, P* p) { lire(p) = 42; Absente; }", 71},
            {"publique vide F(pointeur_fonction<constante P&(P*)> lire, P* p) { lire(p).X = 42; Absente; }", 71},
            {"publique vide F(pointeur_fonction<entier32(P*)> lire, P* p) { entier32& x = lire(p); }", 69},
            {"publique vide F(pointeur_fonction<P(P*)> lire, P* p) { P& x = lire(p); }", 69},
            {"publique vide F(pointeur_fonction<constante pointeur_fonction<entier32()>&(P*)> lire, P* p) { "
             "pointeur_fonction<entier32()>& x = lire(p); }", 69},
            {"publique vide F(pointeur_fonction<booléen&(P*)> lire, P* p) { entier32& x = lire(p); }", 69},
            {"publique vide F(pointeur_fonction<entier32&(P*)> lire, P* p) { lire(Absente, 0) = 42; }", 54},
            {"publique vide F(pointeur_fonction<entier32(P*)> lire, P* p) { lire(p)(); Absente; }", 53},
            {"publique vide F(pointeur_fonction<P&(P*)> lire, P* p) { lire(p).Inconnue; Absente; }", 24},
            {"publique vide F(pointeur_fonction<entier32&(P*)> lire, P* p) { convertir<Inconnue>(lire(Absente)); }", 99},
            {"publique vide F(pointeur_fonction<entier32&(P*)> lire, P* p) { lire(p) = vrai; Absente; }", 73},
            {"publique vide F(pointeur_fonction<constante P&(P*)> lire, P* p) { entier32* x = &lire(p).X; }", 45},
            {"publique vide F(pointeur_fonction<constante P&(P*)> lire, P* p) { entier32* x = &lire(p).Y[1]; }", 45},
            {"publique vide F(pointeur_fonction<volatile P&(P*)> lire, P* p) { entier32* x = &lire(p).X; Absente; }", 45},
            {"publique vide F(pointeur_fonction<volatile P&(P*)> lire, P* p) { constante entier32* x = &lire(p).X; }", 45},
            {"publique vide F(pointeur_fonction<constante volatile P&(P*)> lire, P* p) { constante entier32* x = &lire(p).X; }", 45},
            {"publique vide F(pointeur_fonction<constante volatile P&(P*)> lire, P* p) { volatile entier32* x = &lire(p).X; }", 45},
            {"publique vide F(pointeur_fonction<volatile P&(P*)> lire, P* p) { entier32* x = &lire(p).Y[1]; }", 45},
            {"publique vide F(pointeur_fonction<constante volatile P&(P*)> lire, P* p) { volatile entier32* x = &lire(p).Y[1]; }", 45},
            {"publique vide F(pointeur_fonction<constante volatile entier32&(P*)> lire, P* p) { volatile entier32* x = &lire(p); }", 45},
            {"publique vide F(pointeur_fonction<volatile entier32&(P*)> lire, P* p) { entier32* x = &lire(p); }", 45},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
        {
            const auto source = declarations + refus[index].first;
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
                ComparerErreurSemantique(syntaxe, semantique, texte, refus[index].second,
                    "retour-reference-callback-refuse-" + std::to_string(index));
        }
        std::cout << "Retours par référence des callbacks : " << valides.size()
                  << " corpus bilingues exécutés avec stockage d'hôte vérifié.\n";
    }

    struct DonneesReferencesPointeursHote {
        std::int32_t Valeurs[2];
        std::int32_t* Adresse;
        const std::int32_t* AdresseConstante;
    };

    std::uint32_t NombreLecturesReferencesPointeursHote = 0;

    std::int32_t*& GS_ABI_HOTE LireReferencePointeurHote(DonneesReferencesPointeursHote* donnees) {
        ++NombreLecturesReferencesPointeursHote;
        return donnees->Adresse;
    }
    const std::int32_t*& GS_ABI_HOTE LireReferencePointeurConstantHote(DonneesReferencesPointeursHote* donnees) {
        ++NombreLecturesReferencesPointeursHote;
        return donnees->AdresseConstante;
    }

    /**
     * <résumé>Vérifie un retour de référence vers un pointeur, distinct de son référent.</résumé>
     * Les callbacks C++ retournent les emplacements réels du stockage hôte.
     * La cible du pointeur et les données pointées sont contrôlées séparément.
     **/
    void TesterRetoursReferencesPointeursCallbacks(
        AnalyseurDeclarationsAutoHeberge syntaxe, AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::string declarations =
            "structure D { entier32 Valeurs[2]; entier32* Adresse; constante entier32* AdresseConstante; }; ";
        struct Corpus {
            std::string Texte;
            std::uint64_t Callback;
            std::int32_t X = 41;
            std::int32_t Y = 1;
            bool AdresseSecondElement = false;
            bool AdresseConstanteSecondElement = false;
            std::uint32_t NombreLectures = 1;
        };
        const auto mutablePointeur = reinterpret_cast<std::uint64_t>(&LireReferencePointeurHote);
        const auto constantPointeur = reinterpret_cast<std::uint64_t>(&LireReferencePointeurConstantHote);
        const std::vector<Corpus> valides{
            {"publique entier32 Principal(pointeur_fonction<entier32*&(D*)> lire, D* d) { retourner *lire(d) + 1; }", mutablePointeur},
            {"publique entier32 Principal(pointeur_fonction<entier32*&(D*)> lire, D* d) { "
             "entier32*& liaison = lire(d); *liaison = 42; retourner *liaison; }", mutablePointeur, 42},
            {"publique entier32 Principal(pointeur_fonction<entier32*&(D*)> lire, D* d) { "
             "lire(d) = &d->Valeurs[1]; *lire(d) = 42; retourner d->Valeurs[1]; }", mutablePointeur, 41, 42, true, false, 2},
            {"publique entier32 Principal(pointeur_fonction<entier32*&(D*)> lire, D* d) { "
             "entier32** adresse = &lire(d); *adresse = &d->Valeurs[1]; **adresse = 42; retourner d->Valeurs[1]; }",
             mutablePointeur, 41, 42, true},
            {"publique entier32 Principal(pointeur_fonction<entier32*&(D*)> lire, D* d) { "
             "entier32* valeur = lire(d); *valeur = 42; retourner d->Valeurs[0]; }", mutablePointeur, 42},
            {"classe C { entier32 X; publique: constructeur(entier32*& p) : X(*p + 1) {} "
             "entier32 Lire() { retourner soi.X; } }; publique entier32 Principal(pointeur_fonction<entier32*&(D*)> lire, D* d) { "
             "C c(lire(d)); retourner c.Lire(); }", mutablePointeur},
            {"publique entier32 Principal(pointeur_fonction<entier32*&(D*)> lire, D* d) { "
             "entier32* tableau[2] = {lire(d), &d->Valeurs[1]}; *tableau[0] = 42; retourner *tableau[0]; }", mutablePointeur, 42},
            {"vide Fixer(entier32*& p, entier32* remplacement) { p = remplacement; } "
             "publique entier32 Principal(pointeur_fonction<entier32*&(D*)> lire, D* d) { "
             "Fixer(lire(d), &d->Valeurs[1]); *lire(d) = 42; retourner d->Valeurs[1]; }", mutablePointeur, 41, 42, true, false, 2},
            {"entier32* LireValeur(entier32*& p) { retourner p; } "
             "publique entier32 Principal(pointeur_fonction<entier32*&(D*)> lire, D* d) { "
             "entier32* valeur = LireValeur(lire(d)); retourner *valeur + 1; }", mutablePointeur},
            {"publique entier32 Principal(pointeur_fonction<constante entier32*&(D*)> lire, D* d) { "
             "constante entier32* valeur = lire(d); retourner *valeur + 1; }", constantPointeur},
            {"publique entier32 Principal(pointeur_fonction<constante entier32*&(D*)> lire, D* d) { "
             "constante entier32*& liaison = lire(d); liaison = convertir<constante entier32*>(&d->Valeurs[1]); "
             "retourner *liaison + 41; }", constantPointeur, 41, 1, false, true},
            {"publique entier32 Principal(pointeur_fonction<constante entier32*&(D*)> lire, D* d) { "
             "constante entier32** adresse = &lire(d); *adresse = convertir<constante entier32*>(&d->Valeurs[1]); "
             "retourner **adresse + 41; }", constantPointeur, 41, 1, false, true},
            {"publique entier32 Principal(pointeur_fonction<constante entier32*&(D*)> lire, D* d) { "
             "lire(d) = convertir<constante entier32*>(&d->Valeurs[1]); retourner *lire(d) + 41; }",
             constantPointeur, 41, 1, false, true, 2},
            {"classe C { entier32 X = *lire(d) + 1; publique: constructeur(pointeur_fonction<constante entier32*&(D*)> lire, D* d) {} "
             "entier32 Lire() { retourner soi.X; } }; publique entier32 Principal(pointeur_fonction<constante entier32*&(D*)> lire, D* d) { "
             "C c(lire, d); retourner c.Lire(); }", constantPointeur},
            {"publique entier32 Principal(pointeur_fonction<entier32*&(D*)> lire, D* d) { "
             "lire(d)[1] = 42; retourner lire(d)[1]; }", mutablePointeur, 41, 42, false, false, 2},
            {"publique entier32 Principal(pointeur_fonction<entier32*&(D*)> lire, D* d) { "
             "entier32* adresse = &lire(d)[1]; *adresse = 42; retourner *adresse; }", mutablePointeur, 41, 42},
            {"publique entier32 Principal(pointeur_fonction<entier32*&(D*)> lire, D* d) { "
             "entier32* adresse = &*lire(d); *adresse = 42; retourner *adresse; }", mutablePointeur, 42},
            {"publique entier32 Principal(pointeur_fonction<constante entier32*&(D*)> lire, D* d) { "
             "constante entier32* adresse = &*lire(d); retourner *adresse + 1; }", constantPointeur},
            {"publique entier32 Principal(pointeur_fonction<entier32*&(D*)> lire, D* d) { "
             "entier32* tableau[1][2] = {{lire(d), &d->Valeurs[1]}}; *tableau[0][0] = 42; retourner *tableau[0][0]; }",
             mutablePointeur, 42},
            {"publique entier32 Principal(pointeur_fonction<entier32*&(D*)> lire, D* d) { "
             "entier32* tableau[1][2] = {{lire(d), &d->Valeurs[1]}}; tableau[0][1] = &d->Valeurs[0]; "
             "*tableau[0][1] = 42; retourner d->Valeurs[0]; }", mutablePointeur, 42},
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
        {
            const auto source = declarations + valides[index].Texte;
            std::optional<GsPP::CodeMachine> reference;
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
            {
                const auto nom = "retour-reference-pointeur-callback-valide-" + std::to_string(index);
                const auto resultat = AnalyserSemantiqueValide(syntaxe, semantique, texte, nom);
                auto programme = GsPP::AnalyseurSyntaxique(GsPP::Lexeur(texte, nom).Analyser(), nom).Analyser();
                GsPP::AnalyseurSemantique().Analyser(programme);
                const auto structure = std::find_if(programme.Structures.begin(), programme.Structures.end(),
                    [](const auto& candidate) { return candidate.NomComplet() == "D"; });
                Exiger(structure != programme.Structures.end() && structure->Taille == sizeof(DonneesReferencesPointeursHote)
                        && structure->Alignement == alignof(DonneesReferencesPointeursHote) && structure->Champs.size() == 3
                        && structure->Champs[0].Decalage == offsetof(DonneesReferencesPointeursHote, Valeurs)
                        && structure->Champs[1].Decalage == offsetof(DonneesReferencesPointeursHote, Adresse)
                        && structure->Champs[2].Decalage == offsetof(DonneesReferencesPointeursHote, AdresseConstante),
                    "la disposition des emplacements de pointeurs ne respecte pas l'ABI : " + nom);
                std::unordered_map<std::uint64_t, std::uint64_t> types;
                for (const auto& fonction : programme.Fonctions)
                    for (const auto& parametre : fonction.Parametres)
                        if (parametre.Nom != "soi")
                            types.emplace((static_cast<std::uint64_t>(parametre.Position.Ligne) << 32U) | parametre.Position.Colonne,
                                HacherTypeDeclaration(parametre.Type));
                std::size_t nombreParametres = 0;
                for (const auto& symbole : resultat.Symboles)
                    if (symbole.Genre == 8)
                    {
                        ++nombreParametres;
                        const auto& declaration = resultat.Noeuds[symbole.IndexNoeud];
                        const auto type = types.find((static_cast<std::uint64_t>(declaration.Ligne) << 32U) | declaration.Colonne);
                        Exiger(type != types.end() && symbole.HachageType == type->second,
                            "signature de référence de pointeur différente du bootstrap : " + nom);
                    }
                Exiger(nombreParametres == types.size(), "paramètres de référence de pointeur manquants : " + nom);
                const auto machine = GsPP::GenerateurX64().Generer(programme);
                if (reference)
                    Exiger(machine.Texte == reference->Texte && machine.Donnees == reference->Donnees,
                        "octets bilingues des références de pointeurs différents : " + nom);
                else reference = machine;
                const auto contenu = GsPP::EcrivainGsE().Construire(machine, "Principal");
                Exiger(contenu == GsPP::EcrivainGsE().Construire(GsPP::GenerateurX64().Generer(programme), "Principal"),
                    "image des références de pointeurs non reproductible : " + nom);
                ZoneExecutable zone(AlignerPage(Lire64(contenu, 48)));
                const auto image = GsPP::ChargeurGsE().Charger(contenu, zone.Base());
                Exiger(image.Imports.empty(), "les références de pointeurs ajoutent un import statique : " + nom);
                zone.Copier(image.Memoire);
                DonneesReferencesPointeursHote donnees{{41, 1}, nullptr, nullptr};
                donnees.Adresse = &donnees.Valeurs[0];
                donnees.AdresseConstante = &donnees.Valeurs[0];
                NombreLecturesReferencesPointeursHote = 0;
                using FonctionTest = std::int32_t (GS_ABI_HOTE *)(std::uint64_t, DonneesReferencesPointeursHote*);
                const auto valeur = reinterpret_cast<FonctionTest>(image.AdressePointEntree)(valides[index].Callback, &donnees);
                Exiger(valeur == 42 && donnees.Valeurs[0] == valides[index].X && donnees.Valeurs[1] == valides[index].Y,
                    "lecture ou mutation du référent incorrecte : " + nom);
                Exiger(donnees.Adresse == &donnees.Valeurs[valides[index].AdresseSecondElement ? 1 : 0]
                        && donnees.AdresseConstante == &donnees.Valeurs[valides[index].AdresseConstanteSecondElement ? 1 : 0],
                    "mutation de l'emplacement du pointeur incorrecte : " + nom);
                Exiger(NombreLecturesReferencesPointeursHote == valides[index].NombreLectures,
                    "une référence de pointeur réévalue le callback : " + nom);
            }
        }
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"publique vide F(pointeur_fonction<constante entier32*&(D*)> lire, D* d) { entier32*& x = lire(d); }", 69},
            {"publique vide F(pointeur_fonction<entier32*&(D*)> lire, D* d) { constante entier32*& x = lire(d); }", 69},
            {"publique vide F(pointeur_fonction<entier32*(D*)> lire, D* d) { entier32*& x = lire(d); }", 69},
            {"publique vide F(pointeur_fonction<entier32*&(D*)> lire, D* d) { entier32& x = lire(d); }", 69},
            {"publique vide F(pointeur_fonction<constante entier32*&(D*)> lire, D* d) { *lire(d) = 42; Absente; }", 71},
            {"publique vide F(pointeur_fonction<constante entier32*&(D*)> lire, D* d) { entier32* x = lire(d); }", 45},
            {"publique vide F(pointeur_fonction<entier32*&(D*)> lire, D* d) { lire(d) = 42; Absente; }", 73},
            {"publique vide F(pointeur_fonction<entier32*&(D*)> lire, D* d) { lire(Absente, 0); }", 54},
            {"publique vide F(pointeur_fonction<entier32*&(D*)> lire, D* d) { lire(d)(); Absente; }", 53},
            {"publique vide F(pointeur_fonction<constante entier32*&(D*)> lire, D* d) { entier32** x = &lire(d); }", 45},
            {"publique vide F(pointeur_fonction<entier32*&(D*)> lire, D* d) { lire(d)[vrai]; Absente; }", 51},
            {"publique vide F(pointeur_fonction<entier32*&(D*)> lire, D* d, entier64* autre) { lire(d) = autre; Absente; }", 73},
            {"publique vide F(pointeur_fonction<entier32*&(D*)> lire, D* d) { "
             "entier32* tableau[2] = {lire(d), &d->Valeurs[1]}; tableau = Absente; }", 72},
            {"publique vide F(pointeur_fonction<entier32*&(D*)> lire, D* d) { "
             "entier32* tableau[1][2] = {{lire(d), &d->Valeurs[1]}}; tableau[0] = Absente; }", 72},
            {"publique vide F(pointeur_fonction<entier32*&(D*)> lire, D* d) { "
             "entier32* tableau[2] = {lire(d), &d->Valeurs[1]}; *tableau[0] = vrai; Absente; }", 73},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
        {
            const auto source = declarations + refus[index].first;
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
                ComparerErreurSemantique(syntaxe, semantique, texte, refus[index].second,
                    "retour-reference-pointeur-callback-refuse-" + std::to_string(index));
        }
        std::cout << "Références de pointeurs des callbacks : " << valides.size()
                  << " corpus bilingues exécutés, pointeurs et référents vérifiés séparément.\n";
    }

    using CallbackParametreReferenceHote = std::int32_t (GS_ABI_HOTE *)(std::int32_t);

    struct DonneesReferencesCallbacksParametresHote {
        CallbackParametreReferenceHote Actif;
        CallbackParametreReferenceHote Alternative;
    };

    std::uint32_t NombreLecturesCallbacksParametresHote = 0;
    std::uint32_t NombreAppelsCallbacksParametresHote = 0;
    std::int32_t DernierArgumentCallbackParametreHote = 0;

    std::int32_t GS_ABI_HOTE CibleCallbackParametreHote(std::int32_t valeur) {
        ++NombreAppelsCallbacksParametresHote;
        DernierArgumentCallbackParametreHote = valeur;
        return valeur + 1;
    }
    std::int32_t GS_ABI_HOTE AutreCibleCallbackParametreHote(std::int32_t valeur) {
        ++NombreAppelsCallbacksParametresHote;
        DernierArgumentCallbackParametreHote = valeur;
        return valeur + 2;
    }
    CallbackParametreReferenceHote& GS_ABI_HOTE LireCallbackParametreHote(DonneesReferencesCallbacksParametresHote* donnees) {
        ++NombreLecturesCallbacksParametresHote;
        return donnees->Actif;
    }
    const CallbackParametreReferenceHote& GS_ABI_HOTE LireCallbackParametreConstantHote(DonneesReferencesCallbacksParametresHote* donnees) {
        ++NombreLecturesCallbacksParametresHote;
        return donnees->Actif;
    }
    volatile CallbackParametreReferenceHote& GS_ABI_HOTE LireCallbackParametreVolatileHote(DonneesReferencesCallbacksParametresHote* donnees) {
        ++NombreLecturesCallbacksParametresHote;
        return donnees->Actif;
    }
    const volatile CallbackParametreReferenceHote& GS_ABI_HOTE LireCallbackParametreConstantVolatileHote(DonneesReferencesCallbacksParametresHote* donnees) {
        ++NombreLecturesCallbacksParametresHote;
        return donnees->Actif;
    }

    /**
     * <résumé>Distingue l'emplacement d'un callback paramétré de sa cible et de ses copies.</résumé>
     * Le pont hôte utilise des références C++ réelles et compte les lectures et appels séparément.
     **/
    void TesterReferencesCallbacksParametres(
        AnalyseurDeclarationsAutoHeberge syntaxe, AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::string declarations =
            "structure R { pointeur_fonction<entier32(entier32)> Actif; pointeur_fonction<entier32(entier32)> Alternative; }; ";
        struct Corpus {
            std::string Texte;
            std::uint64_t Callback;
            bool Remplace = false;
            std::uint32_t NombreLectures = 1;
            std::int32_t Argument = 41;
        };
        const auto mutableCallback = reinterpret_cast<std::uint64_t>(&LireCallbackParametreHote);
        const auto constantCallback = reinterpret_cast<std::uint64_t>(&LireCallbackParametreConstantHote);
        const auto volatileCallback = reinterpret_cast<std::uint64_t>(&LireCallbackParametreVolatileHote);
        const auto constantVolatileCallback = reinterpret_cast<std::uint64_t>(&LireCallbackParametreConstantVolatileHote);
        const std::vector<Corpus> valides{
            {"publique entier32 Principal(pointeur_fonction<pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "retourner lire(d)(41); }", mutableCallback},
            {"publique entier32 Principal(pointeur_fonction<pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "pointeur_fonction<entier32(entier32)>& liaison = lire(d); retourner liaison(41); }", mutableCallback},
            {"publique entier32 Principal(pointeur_fonction<pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "lire(d) = d->Alternative; retourner lire(d)(40); }", mutableCallback, true, 2, 40},
            {"publique entier32 Principal(pointeur_fonction<pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "pointeur_fonction<entier32(entier32)>& liaison = lire(d); liaison = d->Alternative; retourner liaison(40); }",
             mutableCallback, true, 1, 40},
            {"publique entier32 Principal(pointeur_fonction<pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "pointeur_fonction<entier32(entier32)>* adresse = &lire(d); *adresse = d->Alternative; retourner (*adresse)(40); }",
             mutableCallback, true, 1, 40},
            {"publique entier32 Principal(pointeur_fonction<pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "pointeur_fonction<entier32(entier32)> copie = lire(d); lire(d) = d->Alternative; retourner copie(41); }",
             mutableCallback, true, 2},
            {"vide Fixer(pointeur_fonction<entier32(entier32)>& cible, pointeur_fonction<entier32(entier32)> autre) { cible = autre; } "
             "publique entier32 Principal(pointeur_fonction<pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "Fixer(lire(d), d->Alternative); retourner lire(d)(40); }", mutableCallback, true, 2, 40},
            {"structure S { pointeur_fonction<entier32(entier32)> F; }; "
             "publique entier32 Principal(pointeur_fonction<pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "S s = {lire(d)}; retourner s.F(41); }", mutableCallback},
            {"publique entier32 Principal(pointeur_fonction<pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "pointeur_fonction<entier32(entier32)> tableau[1] = {lire(d)}; retourner tableau[0](41); }", mutableCallback},
            {"publique entier32 Principal(pointeur_fonction<pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "pointeur_fonction<entier32(entier32)> tableau[1][2] = {{lire(d), d->Alternative}}; "
             "tableau[0][0] = tableau[0][1]; retourner tableau[0][0](40); }", mutableCallback, false, 1, 40},
            {"classe C { entier32 X; publique: constructeur(pointeur_fonction<entier32(entier32)>& f) : X(f(41)) {} "
             "entier32 Lire() { retourner soi.X; } }; "
             "publique entier32 Principal(pointeur_fonction<pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "C c(lire(d)); retourner c.Lire(); }", mutableCallback},
            {"classe C { entier32 X = lire(d)(41); publique: "
             "constructeur(pointeur_fonction<pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) {} "
             "entier32 Lire() { retourner soi.X; } }; "
             "publique entier32 Principal(pointeur_fonction<pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "C c(lire, d); retourner c.Lire(); }", mutableCallback},
            {"publique entier32 Principal(pointeur_fonction<constante pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "constante pointeur_fonction<entier32(entier32)>& liaison = lire(d); retourner liaison(41); }", constantCallback},
            {"publique entier32 Principal(pointeur_fonction<volatile pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "volatile pointeur_fonction<entier32(entier32)>& liaison = lire(d); retourner liaison(41); }", volatileCallback},
            {"publique entier32 Principal(pointeur_fonction<constante volatile pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "constante volatile pointeur_fonction<entier32(entier32)>& liaison = lire(d); retourner liaison(41); }",
             constantVolatileCallback},
            {"publique entier32 Principal(pointeur_fonction<pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "retourner (*lire(d))(41); }", mutableCallback},
            {"publique entier32 Principal(pointeur_fonction<constante pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "constante pointeur_fonction<entier32(entier32)>* adresse = &lire(d); retourner (*adresse)(41); }", constantCallback},
            {"publique entier32 Principal(pointeur_fonction<constante pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "constante pointeur_fonction<entier32(entier32)>* adresse = &lire(d); retourner adresse[0](41); }", constantCallback},
            {"publique entier32 Principal(pointeur_fonction<pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "pointeur_fonction<entier32(entier32)>* adresse = &lire(d); adresse[0] = d->Alternative; retourner adresse[0](40); }",
             mutableCallback, true, 1, 40},
            {"publique entier32 Principal(pointeur_fonction<constante volatile pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "constante volatile pointeur_fonction<entier32(entier32)>* adresse = &lire(d); retourner (*adresse)(41); }",
             constantVolatileCallback},
            {"publique entier32 Principal(pointeur_fonction<constante pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "constante pointeur_fonction<entier32(entier32)> copie = lire(d); retourner copie(41); }", constantCallback},
            {"publique entier32 Principal(pointeur_fonction<pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "pointeur_fonction<pointeur_fonction<entier32(entier32)>&(R*)>* adresse = &lire; retourner (*adresse)(d)(41); }",
             mutableCallback},
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
        {
            std::optional<GsPP::CodeMachine> reference;
            const auto source = declarations + valides[index].Texte;
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
            {
                const auto nom = "reference-callback-parametre-valide-" + std::to_string(index);
                const auto resultat = AnalyserSemantiqueValide(syntaxe, semantique, texte, nom);
                auto programme = GsPP::AnalyseurSyntaxique(GsPP::Lexeur(texte, nom).Analyser(), nom).Analyser();
                GsPP::AnalyseurSemantique().Analyser(programme);
                const auto structure = std::find_if(programme.Structures.begin(), programme.Structures.end(),
                    [](const auto& candidate) { return candidate.NomComplet() == "R"; });
                Exiger(structure != programme.Structures.end()
                        && structure->Taille == sizeof(DonneesReferencesCallbacksParametresHote)
                        && structure->Alignement == alignof(DonneesReferencesCallbacksParametresHote)
                        && structure->Champs.size() == 2
                        && structure->Champs[0].Decalage == offsetof(DonneesReferencesCallbacksParametresHote, Actif)
                        && structure->Champs[1].Decalage == offsetof(DonneesReferencesCallbacksParametresHote, Alternative),
                    "disposition du stockage des callbacks paramétrés différente : " + nom);
                std::unordered_map<std::uint64_t, std::uint64_t> types;
                for (const auto& fonction : programme.Fonctions)
                    for (const auto& parametre : fonction.Parametres)
                        if (parametre.Nom != "soi")
                            types.emplace((static_cast<std::uint64_t>(parametre.Position.Ligne) << 32U) | parametre.Position.Colonne,
                                HacherTypeDeclaration(parametre.Type));
                std::size_t nombreParametres = 0;
                for (const auto& symbole : resultat.Symboles)
                    if (symbole.Genre == 8)
                    {
                        ++nombreParametres;
                        const auto& declaration = resultat.Noeuds[symbole.IndexNoeud];
                        const auto type = types.find((static_cast<std::uint64_t>(declaration.Ligne) << 32U) | declaration.Colonne);
                        Exiger(type != types.end() && symbole.HachageType == type->second,
                            "signature des callbacks paramétrés différente : " + nom);
                    }
                Exiger(nombreParametres == types.size(), "paramètres des callbacks paramétrés manquants : " + nom);
                const auto machine = GsPP::GenerateurX64().Generer(programme);
                if (reference)
                    Exiger(machine.Texte == reference->Texte && machine.Donnees == reference->Donnees,
                        "octets bilingues des callbacks paramétrés différents : " + nom);
                else reference = machine;
                const auto contenu = GsPP::EcrivainGsE().Construire(machine, "Principal");
                Exiger(contenu == GsPP::EcrivainGsE().Construire(GsPP::GenerateurX64().Generer(programme), "Principal"),
                    "image des callbacks paramétrés non reproductible : " + nom);
                ZoneExecutable zone(AlignerPage(Lire64(contenu, 48)));
                const auto image = GsPP::ChargeurGsE().Charger(contenu, zone.Base());
                Exiger(image.Imports.empty(), "les callbacks paramétrés ajoutent un import statique : " + nom);
                zone.Copier(image.Memoire);
                DonneesReferencesCallbacksParametresHote donnees{&CibleCallbackParametreHote, &AutreCibleCallbackParametreHote};
                NombreLecturesCallbacksParametresHote = 0;
                NombreAppelsCallbacksParametresHote = 0;
                DernierArgumentCallbackParametreHote = 0;
                using FonctionTest = std::int32_t (GS_ABI_HOTE *)(std::uint64_t, DonneesReferencesCallbacksParametresHote*);
                const auto valeur = reinterpret_cast<FonctionTest>(image.AdressePointEntree)(valides[index].Callback, &donnees);
                Exiger(valeur == 42 && donnees.Actif == (valides[index].Remplace ? &AutreCibleCallbackParametreHote : &CibleCallbackParametreHote)
                        && donnees.Alternative == &AutreCibleCallbackParametreHote,
                    "copie ou remplacement du callback paramétré incorrect : " + nom);
                Exiger(NombreLecturesCallbacksParametresHote == valides[index].NombreLectures
                        && NombreAppelsCallbacksParametresHote == 1 && DernierArgumentCallbackParametreHote == valides[index].Argument,
                    "lectures, appels ou argument du callback paramétré incorrects : " + nom);
            }
        }
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"publique vide F(pointeur_fonction<constante pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "lire(d) = d->Alternative; Absente; }", 71},
            {"publique vide F(pointeur_fonction<constante pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "pointeur_fonction<entier32(entier32)>& liaison = lire(d); }", 69},
            {"publique vide F(pointeur_fonction<pointeur_fonction<entier32(entier32)>(R*)> lire, R* d) { "
             "pointeur_fonction<entier32(entier32)>& liaison = lire(d); }", 69},
            {"publique vide F(pointeur_fonction<pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "pointeur_fonction<entier32(entier64)>& liaison = lire(d); }", 69},
            {"publique vide F(pointeur_fonction<pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "lire(d)(Absente, 0); }", 54},
            {"publique vide F(pointeur_fonction<pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "lire(Absente, 0)(Absente); }", 54},
            {"publique vide F(pointeur_fonction<pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "lire(d)(vrai); Absente; }", 55},
            {"publique vide F(pointeur_fonction<pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "lire(d) = 42; Absente; }", 73},
            {"publique vide F(pointeur_fonction<pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d, "
             "pointeur_fonction<entier32(entier64)> autre) { lire(d) = autre; Absente; }", 73},
            {"publique vide F(pointeur_fonction<constante pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "pointeur_fonction<entier32(entier32)>* adresse = &lire(d); }", 45},
            {"publique vide F(pointeur_fonction<pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "pointeur_fonction<entier32(entier32)>* adresse = &lire(d); adresse(Absente); }", 53},
            {"publique vide F(pointeur_fonction<pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "pointeur_fonction<entier32(entier32)> tableau[1] = {lire(d)}; tableau = Absente; }", 72},
            {"publique vide F(pointeur_fonction<pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "pointeur_fonction<entier32(entier32)> tableau[1] = {lire(d)}; tableau[0](Absente, 0); }", 54},
            {"classe C { entier32 X = lire(d)(vrai); publique: "
             "constructeur(pointeur_fonction<pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { Absente; } };", 55},
            {"publique vide F(pointeur_fonction<constante pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d, "
             "constante pointeur_fonction<entier32(entier32)> autre) { lire(d) = autre; Absente; }", 71},
            {"publique vide F(pointeur_fonction<constante pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d, "
             "constante pointeur_fonction<entier32(entier32)> autre) { "
             "constante pointeur_fonction<entier32(entier32)> copie = lire(d); copie = autre; Absente; }", 71},
            {"publique vide F(pointeur_fonction<constante pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d, "
             "constante pointeur_fonction<entier32(entier32)> autre) { "
             "constante pointeur_fonction<entier32(entier32)>& liaison = lire(d); liaison = autre; Absente; }", 71},
            {"publique vide F(pointeur_fonction<constante pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d, "
             "constante pointeur_fonction<entier32(entier32)> autre) { "
             "constante pointeur_fonction<entier32(entier32)>* adresse = &lire(d); *adresse = autre; Absente; }", 71},
            {"structure Q { constante pointeur_fonction<entier32(entier32)> F; }; "
             "publique vide F(Q* q, constante pointeur_fonction<entier32(entier32)> autre) { q->F = autre; Absente; }", 71},
            {"publique vide F(constante pointeur_fonction<entier32(entier32)>* adresse, "
             "constante pointeur_fonction<entier32(entier32)> autre) { adresse[0] = autre; Absente; }", 71},
            {"publique vide F(pointeur_fonction<constante pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d, "
             "constante pointeur_fonction<entier32(entier32)> autre) { "
             "constante pointeur_fonction<entier32(entier32)> tableau[1] = {lire(d)}; tableau[0] = autre; Absente; }", 71},
            {"publique vide F(pointeur_fonction<constante volatile pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d, "
             "constante volatile pointeur_fonction<entier32(entier32)> autre) { lire(d) = autre; Absente; }", 71},
            {"publique vide F(pointeur_fonction<constante pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d, "
             "constante pointeur_fonction<entier32(entier32)> autre) { *lire(d) = autre; Absente; }", 71},
            {"publique vide F(pointeur_fonction<pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "pointeur_fonction<entier32(entier32)> tableau[1] = {lire(d)}; tableau(Absente); }", 53},
            {"publique vide F(pointeur_fonction<pointeur_fonction<entier32(entier32)>&(R*)> lire, R* d) { "
             "pointeur_fonction<entier32(entier32)> tableau[1][1] = {{lire(d)}}; tableau[0](Absente); }", 53},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
        {
            const auto source = declarations + refus[index].first;
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
                ComparerErreurSemantique(syntaxe, semantique, texte, refus[index].second,
                    "reference-callback-parametre-refuse-" + std::to_string(index));
        }
        std::cout << "Références de callbacks paramétrés : " << valides.size()
                  << " corpus bilingues exécutés, stockage, cible, lectures et appels vérifiés.\n";
    }

    using CallbackMutationReferenceHote = std::int32_t& (GS_ABI_HOTE *)(std::int32_t&);
    using CallbackLectureReferenceHote = std::int32_t (GS_ABI_HOTE *)(const std::int32_t&);

    struct DonneesArgumentsReferencesHote {
        std::int32_t Valeurs[2];
        std::int32_t* Adresse;
        CallbackMutationReferenceHote Mutation;
        CallbackLectureReferenceHote Lecture;
    };

    struct EvenementArgumentReferenceHote {
        const std::int32_t* Adresse;
        std::int32_t Avant;
        std::int32_t Apres;
        bool LectureSeule;
    };

    std::array<EvenementArgumentReferenceHote, 4> TraceArgumentsReferencesHote{};
    std::uint32_t NombreArgumentsReferencesHote = 0;
    std::uint32_t NombreLecturesArgumentsReferencesHote = 0;

    std::int32_t& GS_ABI_HOTE MuterArgumentReferenceHote(std::int32_t& valeur) {
        const auto avant = valeur;
        ++valeur;
        if (NombreArgumentsReferencesHote < TraceArgumentsReferencesHote.size())
            TraceArgumentsReferencesHote[NombreArgumentsReferencesHote] = {&valeur, avant, valeur, false};
        ++NombreArgumentsReferencesHote;
        return valeur;
    }
    std::int32_t GS_ABI_HOTE LireArgumentReferenceHote(const std::int32_t& valeur) {
        if (NombreArgumentsReferencesHote < TraceArgumentsReferencesHote.size())
            TraceArgumentsReferencesHote[NombreArgumentsReferencesHote] = {&valeur, valeur, valeur, true};
        ++NombreArgumentsReferencesHote;
        return valeur + 1;
    }
    CallbackMutationReferenceHote& GS_ABI_HOTE LireMutateurReferencesHote(DonneesArgumentsReferencesHote* donnees) {
        ++NombreLecturesArgumentsReferencesHote;
        return donnees->Mutation;
    }
    const CallbackMutationReferenceHote& GS_ABI_HOTE LireMutateurReferencesConstantHote(DonneesArgumentsReferencesHote* donnees) {
        ++NombreLecturesArgumentsReferencesHote;
        return donnees->Mutation;
    }
    CallbackMutationReferenceHote GS_ABI_HOTE CopierMutateurReferencesHote(DonneesArgumentsReferencesHote* donnees) {
        ++NombreLecturesArgumentsReferencesHote;
        return donnees->Mutation;
    }
    const CallbackLectureReferenceHote& GS_ABI_HOTE LireLecteurReferencesHote(DonneesArgumentsReferencesHote* donnees) {
        ++NombreLecturesArgumentsReferencesHote;
        return donnees->Lecture;
    }

    /**
     * <résumé>Compose références de callbacks, paramètres par référence et retours par référence.</résumé>
     * Le pont vérifie l'adresse réelle de chaque argument et l'ordre des mutations, sans copie cachée.
     **/
    void TesterArgumentsReferencesCallbacksImbriques(
        AnalyseurDeclarationsAutoHeberge syntaxe, AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::string declarations = "structure V { entier32 Valeurs[2]; entier32* Adresse; "
            "pointeur_fonction<entier32&(entier32&)> Mutation; pointeur_fonction<entier32(constante entier32&)> Lecture; }; ";
        const std::string typeMutation = "pointeur_fonction<pointeur_fonction<entier32&(entier32&)>&(V*)>";
        const std::string typeMutationConstante = "pointeur_fonction<constante pointeur_fonction<entier32&(entier32&)>&(V*)>";
        const std::string typeCopieMutation = "pointeur_fonction<pointeur_fonction<entier32&(entier32&)>(V*)>";
        const std::string typeLecture = "pointeur_fonction<constante pointeur_fonction<entier32(constante entier32&)>&(V*)>";
        const auto principal = [](std::string_view type, std::string_view corps) {
            return "publique entier32 Principal(" + std::string(type) + " lire, V* d) { " + std::string(corps) + " }";
        };
        struct EvenementAttendu {
            std::size_t IndexValeur;
            std::int32_t Avant;
            std::int32_t Apres;
            bool LectureSeule = false;
        };
        struct Corpus {
            std::string Texte;
            std::uint64_t Callback;
            std::int32_t XAvant = 41;
            std::int32_t YAvant = 1;
            std::int32_t XApres = 42;
            std::int32_t YApres = 1;
            std::uint32_t NombreLectures = 1;
            std::vector<EvenementAttendu> Evenements{{0, 41, 42}};
        };
        const auto mutateur = reinterpret_cast<std::uint64_t>(&LireMutateurReferencesHote);
        const auto mutateurConstant = reinterpret_cast<std::uint64_t>(&LireMutateurReferencesConstantHote);
        const auto copieMutateur = reinterpret_cast<std::uint64_t>(&CopierMutateurReferencesHote);
        const auto lecteur = reinterpret_cast<std::uint64_t>(&LireLecteurReferencesHote);
        const std::vector<Corpus> valides{
            {principal(typeMutation, "retourner lire(d)(d->Valeurs[0]);"), mutateur},
            {principal(typeMutation, "lire(d)(d->Valeurs[0]) = 42; retourner d->Valeurs[0];"),
             mutateur, 40, 1, 42, 1, 1, {{0, 40, 41}}},
            {principal(typeMutation, "entier32& liaison = lire(d)(d->Valeurs[0]); liaison = 42; retourner liaison;"),
             mutateur, 40, 1, 42, 1, 1, {{0, 40, 41}}},
            {principal(typeMutation, "entier32* adresse = &lire(d)(d->Valeurs[0]); *adresse = 42; retourner *adresse;"),
             mutateur, 40, 1, 42, 1, 1, {{0, 40, 41}}},
            {principal(typeMutation, "retourner lire(d)(*d->Adresse);"), mutateur},
            {principal(typeMutation, "retourner lire(d)(d->Adresse[1]) + 1;"),
             mutateur, 41, 40, 41, 41, 1, {{1, 40, 41}}},
            {"entier32 Ajouter(entier32& valeur) { valeur = valeur + 1; retourner valeur; } "
             + principal(typeMutation, "retourner Ajouter(lire(d)(d->Valeurs[0]));"),
             mutateur, 40, 1, 42, 1, 1, {{0, 40, 41}}},
            {"classe C { entier32 X; publique: constructeur(entier32& valeur) : X(valeur + 1) { valeur = 42; } "
             "entier32 Lire() { retourner soi.X; } }; "
             + principal(typeMutation, "C c(lire(d)(d->Valeurs[0])); retourner c.Lire();"),
             mutateur, 40, 1, 42, 1, 1, {{0, 40, 41}}},
            {"classe C { entier32 X = lire(d)(d->Valeurs[0]) + 1; publique: constructeur(" + typeMutation
             + " lire, V* d) {} entier32 Lire() { retourner soi.X; } }; "
             + principal(typeMutation, "C c(lire, d); retourner c.Lire();"),
             mutateur, 40, 1, 41, 1, 1, {{0, 40, 41}}},
            {"classe Base { publique: entier32 X; constructeur(entier32& valeur) : X(valeur) {} }; "
             "classe C : publique Base { publique: constructeur(" + typeMutation
             + " lire, V* d) : parent(lire(d)(d->Valeurs[0])) {} }; "
             + principal(typeMutation, "C c(lire, d); retourner c.X;"), mutateur},
            {"structure S { entier32 X; }; "
             + principal(typeMutation, "S s = {lire(d)(d->Valeurs[0])}; retourner s.X;"), mutateur},
            {principal(typeMutation, "pointeur_fonction<entier32&(entier32&)>* adresse = &lire(d); "
                "retourner (*adresse)(d->Valeurs[0]);"), mutateur},
            {principal(typeMutation, "retourner lire(d)(d->Valeurs[0]) + lire(d)(d->Valeurs[1]);"),
             mutateur, 20, 20, 21, 21, 2, {{0, 20, 21}, {1, 20, 21}}},
            {principal(typeCopieMutation, "retourner lire(d)(d->Valeurs[0]);"), copieMutateur},
            {principal(typeCopieMutation, "lire(d)(d->Valeurs[0]) = 42; retourner d->Valeurs[0];"),
             copieMutateur, 40, 1, 42, 1, 1, {{0, 40, 41}}},
            {principal(typeMutationConstante, "retourner lire(d)(d->Valeurs[0]);"), mutateurConstant},
            {principal(typeMutationConstante, "constante pointeur_fonction<entier32&(entier32&)>& liaison = lire(d); "
                "retourner liaison(d->Valeurs[0]);"), mutateurConstant},
            {principal(typeLecture, "retourner lire(d)(d->Valeurs[0]);"),
             lecteur, 41, 1, 41, 1, 1, {{0, 41, 41, true}}},
            {principal(typeLecture, "constante entier32* adresse = convertir<constante entier32*>(&d->Valeurs[0]); "
                "retourner lire(d)(*adresse);"), lecteur, 41, 1, 41, 1, 1, {{0, 41, 41, true}}},
            {principal(typeLecture, "retourner lire(d)(d->Mutation(d->Valeurs[0]));"),
             lecteur, 40, 1, 41, 1, 1, {{0, 40, 41}, {0, 41, 41, true}}},
            {principal(typeMutation, "retourner lire(d)(lire(d)(d->Valeurs[0]));"),
             mutateur, 40, 1, 42, 1, 2, {{0, 40, 41}, {0, 41, 42}}},
            {principal(typeMutation, "retourner lire(d)(d->Mutation(d->Valeurs[0]));"),
             mutateur, 40, 1, 42, 1, 1, {{0, 40, 41}, {0, 41, 42}}},
            {principal(typeMutation, "faux && lire(d)(d->Valeurs[0]); retourner 42;"),
             mutateur, 41, 1, 41, 1, 0, {}},
            {principal(typeMutation, "vrai || lire(d)(d->Valeurs[0]); retourner 42;"),
             mutateur, 41, 1, 41, 1, 0, {}},
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
        {
            const auto source = declarations + valides[index].Texte;
            std::optional<GsPP::CodeMachine> reference;
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
            {
                const auto nom = "argument-reference-callback-imbrique-valide-" + std::to_string(index);
                const auto resultat = AnalyserSemantiqueValide(syntaxe, semantique, texte, nom);
                auto programme = GsPP::AnalyseurSyntaxique(GsPP::Lexeur(texte, nom).Analyser(), nom).Analyser();
                GsPP::AnalyseurSemantique().Analyser(programme);
                const auto structure = std::find_if(programme.Structures.begin(), programme.Structures.end(),
                    [](const auto& candidate) { return candidate.NomComplet() == "V"; });
                Exiger(structure != programme.Structures.end() && structure->Taille == sizeof(DonneesArgumentsReferencesHote)
                        && structure->Alignement == alignof(DonneesArgumentsReferencesHote) && structure->Champs.size() == 4
                        && structure->Champs[0].Decalage == offsetof(DonneesArgumentsReferencesHote, Valeurs)
                        && structure->Champs[1].Decalage == offsetof(DonneesArgumentsReferencesHote, Adresse)
                        && structure->Champs[2].Decalage == offsetof(DonneesArgumentsReferencesHote, Mutation)
                        && structure->Champs[3].Decalage == offsetof(DonneesArgumentsReferencesHote, Lecture),
                    "disposition des arguments référencés différente du pont hôte : " + nom);
                std::unordered_map<std::uint64_t, std::uint64_t> types;
                for (const auto& fonction : programme.Fonctions)
                    for (const auto& parametre : fonction.Parametres)
                        if (parametre.Nom != "soi")
                            types.emplace((static_cast<std::uint64_t>(parametre.Position.Ligne) << 32U) | parametre.Position.Colonne,
                                HacherTypeDeclaration(parametre.Type));
                std::size_t nombreParametres = 0;
                for (const auto& symbole : resultat.Symboles)
                    if (symbole.Genre == 8)
                    {
                        ++nombreParametres;
                        const auto& declaration = resultat.Noeuds[symbole.IndexNoeud];
                        const auto type = types.find((static_cast<std::uint64_t>(declaration.Ligne) << 32U) | declaration.Colonne);
                        Exiger(type != types.end() && symbole.HachageType == type->second,
                            "signature d'argument référencé différente : " + nom);
                    }
                Exiger(nombreParametres == types.size(), "paramètres des arguments référencés manquants : " + nom);
                const auto machine = GsPP::GenerateurX64().Generer(programme);
                if (reference)
                    Exiger(machine.Texte == reference->Texte && machine.Donnees == reference->Donnees,
                        "octets bilingues des arguments référencés différents : " + nom);
                else reference = machine;
                const auto contenu = GsPP::EcrivainGsE().Construire(machine, "Principal");
                Exiger(contenu == GsPP::EcrivainGsE().Construire(GsPP::GenerateurX64().Generer(programme), "Principal"),
                    "image des arguments référencés non reproductible : " + nom);
                ZoneExecutable zone(AlignerPage(Lire64(contenu, 48)));
                const auto image = GsPP::ChargeurGsE().Charger(contenu, zone.Base());
                Exiger(image.Imports.empty(), "les arguments référencés ajoutent un import statique : " + nom);
                zone.Copier(image.Memoire);
                DonneesArgumentsReferencesHote donnees{{valides[index].XAvant, valides[index].YAvant}, nullptr,
                    &MuterArgumentReferenceHote, &LireArgumentReferenceHote};
                donnees.Adresse = &donnees.Valeurs[0];
                TraceArgumentsReferencesHote = {};
                NombreArgumentsReferencesHote = 0;
                NombreLecturesArgumentsReferencesHote = 0;
                using FonctionTest = std::int32_t (GS_ABI_HOTE *)(std::uint64_t, DonneesArgumentsReferencesHote*);
                const auto valeur = reinterpret_cast<FonctionTest>(image.AdressePointEntree)(valides[index].Callback, &donnees);
                Exiger(valeur == 42 && donnees.Valeurs[0] == valides[index].XApres && donnees.Valeurs[1] == valides[index].YApres
                        && donnees.Adresse == &donnees.Valeurs[0]
                        && donnees.Mutation == &MuterArgumentReferenceHote && donnees.Lecture == &LireArgumentReferenceHote,
                    "mutation de l'argument ou du stockage inattendue : " + nom);
                Exiger(NombreLecturesArgumentsReferencesHote == valides[index].NombreLectures
                        && NombreArgumentsReferencesHote == valides[index].Evenements.size(),
                    "réévaluation d'un callback ou appel manquant : " + nom);
                for (std::size_t evenement = 0; evenement < valides[index].Evenements.size(); ++evenement)
                {
                    const auto& attendu = valides[index].Evenements[evenement];
                    const auto& obtenu = TraceArgumentsReferencesHote[evenement];
                    Exiger(obtenu.Adresse == &donnees.Valeurs[attendu.IndexValeur]
                            && obtenu.Avant == attendu.Avant && obtenu.Apres == attendu.Apres
                            && obtenu.LectureSeule == attendu.LectureSeule,
                        "argument copié ou ordre des mutations différent : " + nom);
                }
            }
        }
        const auto fonction = [](std::string_view type, std::string_view corps) {
            return "publique vide F(" + std::string(type) + " lire, V* d) { " + std::string(corps) + " }";
        };
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {fonction(typeMutation, "lire(d)(42); Absente;"), 55},
            {fonction(typeMutation, "lire(d)(d->Valeurs[0] + 1); Absente;"), 55},
            {fonction(typeMutation, "lire(d)({42}); Absente;"), 69},
            {fonction(typeMutation, "constante entier32& valeur = d->Valeurs[0]; lire(d)(valeur); Absente;"), 55},
            {fonction(typeMutation, "entier64 valeur = 41; lire(d)(valeur); Absente;"), 55},
            {fonction(typeMutation, "lire(d)(Absente, 0);"), 54},
            {fonction(typeMutation, "lire(Absente, 0)(Absente);"), 54},
            {fonction(typeMutation, "lire(d)(d->Valeurs[vrai]); Absente;"), 51},
            {fonction(typeLecture, "lire(d)(42); Absente;"), 55},
            {fonction(typeLecture, "lire(d)({42}); Absente;"), 69},
            {fonction(typeMutation, "lire(d)(d->Lecture(d->Valeurs[0])); Absente;"), 55},
            {fonction(typeLecture, "lire(d)(d->Mutation(42)); Absente;"), 55},
            {fonction(typeMutation, "constante entier32* valeur = convertir<constante entier32*>(&d->Valeurs[0]); "
                "lire(d)(*valeur); Absente;"), 55},
            {fonction(typeMutation, "booléen valeur = vrai; lire(d)(valeur); Absente;"), 55},
            {fonction(typeMutation, "lire(d)(d->Valeurs[0]) = vrai; Absente;"), 73},
            {fonction(typeLecture, "entier32& liaison = lire(d)(d->Valeurs[0]); Absente;"), 69},
            {fonction(typeMutation, "pointeur_fonction<entier32(entier32&)>& liaison = lire(d); Absente;"), 69},
            {"classe C { entier32 X = lire(d)(42); publique: constructeur(" + typeMutation + " lire, V* d) { Absente; } };", 55},
            {"classe Base { publique: constructeur(entier32& valeur) {} }; classe C : publique Base { publique: "
             "constructeur(" + typeMutation + " lire, V* d) : parent(lire(d)(42)) { Absente; } };", 55},
            {"classe C { publique: constructeur(entier32& valeur) {} constructeur(" + typeMutation
             + " lire, V* d) : soi(lire(d)(42)) { Absente; } };", 55},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
        {
            const auto source = declarations + refus[index].first;
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
                ComparerErreurSemantique(syntaxe, semantique, texte, refus[index].second,
                    "argument-reference-callback-imbrique-refuse-" + std::to_string(index));
        }
        std::cout << "Arguments référencés des callbacks imbriqués : " << valides.size()
                  << " corpus bilingues exécutés, adresses et traces de mutation vérifiées.\n";
    }

    struct StructureReferenceHote {
        std::int32_t X;
        std::int32_t Y[2];
        std::int32_t* Adresse;
    };

    using CallbackStructureReferenceHote = StructureReferenceHote& (GS_ABI_HOTE *)(StructureReferenceHote&);
    using CallbackStructureConstanteHote = const StructureReferenceHote& (GS_ABI_HOTE *)(const StructureReferenceHote&);
    using CallbackPointeurReferenceHote = std::int32_t*& (GS_ABI_HOTE *)(std::int32_t*&, std::int32_t*);
    using CallbackPointeurConstantReferenceHote = const std::int32_t*& (GS_ABI_HOTE *)(const std::int32_t*&, const std::int32_t*);

    struct DonneesStructuresPointeursHote {
        StructureReferenceHote Objet;
        StructureReferenceHote Second;
        std::int32_t* Cible;
        const std::int32_t* CibleConstante;
        CallbackStructureReferenceHote Mutation;
        CallbackStructureConstanteHote Lecture;
        CallbackPointeurReferenceHote Redirection;
        CallbackPointeurConstantReferenceHote RedirectionConstante;
    };

    struct EvenementStructurePointeurHote {
        std::uint32_t Genre;
        const void* Adresse;
        std::uintptr_t Avant;
        std::uintptr_t Apres;
    };

    std::array<EvenementStructurePointeurHote, 4> TraceStructuresPointeursHote{};
    std::uint32_t NombreStructuresPointeursHote = 0;
    std::uint32_t NombreLecturesStructuresPointeursHote = 0;

    void TracerStructurePointeurHote(std::uint32_t genre, const void* adresse, std::uintptr_t avant, std::uintptr_t apres) {
        if (NombreStructuresPointeursHote < TraceStructuresPointeursHote.size())
            TraceStructuresPointeursHote[NombreStructuresPointeursHote] = {genre, adresse, avant, apres};
        ++NombreStructuresPointeursHote;
    }
    StructureReferenceHote& GS_ABI_HOTE MuterStructureReferenceHote(StructureReferenceHote& objet) {
        const auto avant = objet.X;
        ++objet.X;
        TracerStructurePointeurHote(1, &objet, avant, objet.X);
        return objet;
    }
    const StructureReferenceHote& GS_ABI_HOTE LireStructureConstanteHote(const StructureReferenceHote& objet) {
        TracerStructurePointeurHote(2, &objet, objet.X, objet.X);
        return objet;
    }
    std::int32_t*& GS_ABI_HOTE RedirigerPointeurReferenceHote(std::int32_t*& cible, std::int32_t* remplacement) {
        const auto avant = reinterpret_cast<std::uintptr_t>(cible);
        cible = remplacement;
        TracerStructurePointeurHote(3, &cible, avant, reinterpret_cast<std::uintptr_t>(cible));
        return cible;
    }
    const std::int32_t*& GS_ABI_HOTE RedirigerPointeurConstantReferenceHote(const std::int32_t*& cible, const std::int32_t* remplacement) {
        const auto avant = reinterpret_cast<std::uintptr_t>(cible);
        cible = remplacement;
        TracerStructurePointeurHote(4, &cible, avant, reinterpret_cast<std::uintptr_t>(cible));
        return cible;
    }
    CallbackStructureReferenceHote& GS_ABI_HOTE LireMutateurStructureHote(DonneesStructuresPointeursHote* donnees) {
        ++NombreLecturesStructuresPointeursHote;
        return donnees->Mutation;
    }
    CallbackStructureConstanteHote& GS_ABI_HOTE LireLecteurStructureHote(DonneesStructuresPointeursHote* donnees) {
        ++NombreLecturesStructuresPointeursHote;
        return donnees->Lecture;
    }
    CallbackPointeurReferenceHote& GS_ABI_HOTE LireRedirectionPointeurHote(DonneesStructuresPointeursHote* donnees) {
        ++NombreLecturesStructuresPointeursHote;
        return donnees->Redirection;
    }
    CallbackPointeurConstantReferenceHote& GS_ABI_HOTE LireRedirectionPointeurConstantHote(DonneesStructuresPointeursHote* donnees) {
        ++NombreLecturesStructuresPointeursHote;
        return donnees->RedirectionConstante;
    }

    /**
     * <résumé>Vérifie l'identité des agrégats et des emplacements de pointeurs traversant les callbacks.</résumé>
     * La qualification de la donnée pointée ne rend pas constant l'emplacement du pointeur.
     **/
    void TesterReferencesStructuresPointeursCallbacksImbriques(
        AnalyseurDeclarationsAutoHeberge syntaxe, AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::string declarations = "structure P { entier32 X; entier32 Y[2]; entier32* Adresse; }; "
            "structure Z { P Objet; P Second; entier32* Cible; constante entier32* CibleConstante; "
            "pointeur_fonction<P&(P&)> Mutation; pointeur_fonction<constante P&(constante P&)> Lecture; "
            "pointeur_fonction<entier32*&(entier32*&, entier32*)> Redirection; "
            "pointeur_fonction<constante entier32*&(constante entier32*&, constante entier32*)> RedirectionConstante; }; ";
        const std::string typeMutation = "pointeur_fonction<pointeur_fonction<P&(P&)>&(Z*)>";
        const std::string typeLecture = "pointeur_fonction<pointeur_fonction<constante P&(constante P&)>&(Z*)>";
        const std::string typePointeur = "pointeur_fonction<pointeur_fonction<entier32*&(entier32*&, entier32*)>&(Z*)>";
        const std::string typePointeurConstant = "pointeur_fonction<pointeur_fonction<constante entier32*&(constante entier32*&, constante entier32*)>&(Z*)>";
        const auto principal = [](std::string_view type, std::string_view corps) {
            return "publique entier32 Principal(" + std::string(type) + " lire, Z* z) { " + std::string(corps) + " }";
        };
        struct EvenementAttendu {
            std::uint32_t Genre;
            std::size_t IndexArgument;
            std::uintptr_t Avant;
            std::uintptr_t Apres;
        };
        struct Corpus {
            std::string Texte;
            std::uint64_t Callback;
            std::int32_t XAvant = 41;
            std::int32_t SecondAvant = 41;
            std::int32_t XApres = 42;
            std::int32_t SecondApres = 41;
            bool CibleSeconde = false;
            bool CibleConstanteSeconde = false;
            std::uint32_t NombreLectures = 1;
            std::vector<EvenementAttendu> Evenements{{1, 0, 41, 42}};
            std::int32_t YApres = 9;
        };
        const auto mutateur = reinterpret_cast<std::uint64_t>(&LireMutateurStructureHote);
        const auto lecteur = reinterpret_cast<std::uint64_t>(&LireLecteurStructureHote);
        const auto redirection = reinterpret_cast<std::uint64_t>(&LireRedirectionPointeurHote);
        const auto redirectionConstante = reinterpret_cast<std::uint64_t>(&LireRedirectionPointeurConstantHote);
        const std::vector<Corpus> valides{
            {principal(typeMutation, "retourner lire(z)(z->Objet).X;"), mutateur},
            {principal(typeMutation, "P& liaison = lire(z)(z->Objet); liaison.X = 42; retourner liaison.X;"),
             mutateur, 40, 41, 42, 41, false, false, 1, {{1, 0, 40, 41}}},
            {principal(typeMutation, "P* adresse = &lire(z)(z->Objet); adresse->X = 42; retourner adresse->X;"),
             mutateur, 40, 41, 42, 41, false, false, 1, {{1, 0, 40, 41}}},
            {principal(typeMutation, "P copie = lire(z)(z->Objet); copie.X = 1; retourner copie.X + z->Objet.X - 1;"), mutateur},
            {principal(typeMutation, "lire(z)(z->Objet).Y[1] = 42; retourner z->Objet.Y[1];"),
             mutateur, 41, 41, 42, 41, false, false, 1, {{1, 0, 41, 42}}, 42},
            {principal(typeMutation, "P* objet = &z->Objet; retourner lire(z)(*objet).X;"), mutateur},
            {principal(typeMutation, "retourner lire(z)(z->Objet).X + lire(z)(z->Second).X;"),
             mutateur, 20, 20, 21, 21, false, false, 2, {{1, 0, 20, 21}, {1, 1, 20, 21}}},
            {principal(typeMutation, "retourner lire(z)(lire(z)(z->Objet)).X;"),
             mutateur, 40, 41, 42, 41, false, false, 2, {{1, 0, 40, 41}, {1, 0, 41, 42}}},
            {"classe C { entier32 X; publique: constructeur(P& objet) : X(objet.X) { objet.Y[1] = 42; } "
             "entier32 Lire() { retourner soi.X; } }; "
             + principal(typeMutation, "C c(lire(z)(z->Objet)); retourner c.Lire();"),
             mutateur, 41, 41, 42, 41, false, false, 1, {{1, 0, 41, 42}}, 42},
            {"classe Base { publique: entier32 X; constructeur(P& objet) : X(objet.X) {} }; "
             "classe C : publique Base { publique: constructeur(" + typeMutation
             + " lire, Z* z) : parent(lire(z)(z->Objet)) {} }; "
             + principal(typeMutation, "C c(lire, z); retourner c.X;"), mutateur},
            {"classe C { entier32 X = lire(z)(z->Objet).X; publique: constructeur(" + typeMutation
             + " lire, Z* z) {} entier32 Lire() { retourner soi.X; } }; "
             + principal(typeMutation, "C c(lire, z); retourner c.Lire();"), mutateur},
            {principal(typeLecture, "retourner lire(z)(z->Objet).X + 1;"),
             lecteur, 41, 41, 41, 41, false, false, 1, {{2, 0, 41, 41}}},
            {principal(typeLecture, "constante P& liaison = lire(z)(z->Objet); retourner liaison.X + 1;"),
             lecteur, 41, 41, 41, 41, false, false, 1, {{2, 0, 41, 41}}},
            {principal(typeLecture, "constante P* objet = &lire(z)(z->Objet); "
                "constante entier32* adresse = &objet->Y[1]; retourner *adresse + 33;"),
             lecteur, 41, 41, 41, 41, false, false, 1, {{2, 0, 41, 41}}},
            {principal(typeLecture, "P copie = lire(z)(z->Objet); copie.X = 42; retourner copie.X;"),
             lecteur, 41, 41, 41, 41, false, false, 1, {{2, 0, 41, 41}}},
            {principal(typePointeur, "retourner *lire(z)(z->Cible, &z->Second.X) + 1;"),
             redirection, 41, 41, 41, 41, true, false, 1, {{3, 2, 0, 1}}},
            {principal(typePointeur, "entier32*& liaison = lire(z)(z->Cible, &z->Second.X); "
                "*liaison = 42; retourner z->Second.X;"),
             redirection, 41, 41, 41, 42, true, false, 1, {{3, 2, 0, 1}}},
            {principal(typePointeur, "entier32** adresse = &lire(z)(z->Cible, &z->Second.X); "
                "**adresse = 42; retourner z->Second.X;"),
             redirection, 41, 41, 41, 42, true, false, 1, {{3, 2, 0, 1}}},
            {"classe C { entier32 X = *lire(z)(z->Cible, &z->Second.X) + 1; publique: constructeur(" + typePointeur
             + " lire, Z* z) {} entier32 Lire() { retourner soi.X; } }; "
             + principal(typePointeur, "C c(lire, z); retourner c.Lire();"),
             redirection, 41, 41, 41, 41, true, false, 1, {{3, 2, 0, 1}}},
            {"vide Fixer(entier32*& cible, entier32* remplacement) { cible = remplacement; } "
             + principal(typePointeur, "Fixer(lire(z)(z->Cible, &z->Second.X), &z->Objet.X); retourner *z->Cible + 1;"),
             redirection, 41, 41, 41, 41, false, false, 1, {{3, 2, 0, 1}}},
            {principal(typePointeurConstant, "retourner *lire(z)(z->CibleConstante, "
                "convertir<constante entier32*>(&z->Second.X)) + 1;"),
             redirectionConstante, 41, 41, 41, 41, false, true, 1, {{4, 3, 0, 1}}},
            {principal(typePointeurConstant, "constante entier32*& liaison = lire(z)(z->CibleConstante, "
                "convertir<constante entier32*>(&z->Second.X)); liaison = convertir<constante entier32*>(&z->Objet.X); "
                "retourner *liaison + 1;"),
             redirectionConstante, 41, 41, 41, 41, false, false, 1, {{4, 3, 0, 1}}},
            {principal(typePointeurConstant, "constante entier32** adresse = &lire(z)(z->CibleConstante, "
                "convertir<constante entier32*>(&z->Second.X)); retourner **adresse + 1;"),
             redirectionConstante, 41, 41, 41, 41, false, true, 1, {{4, 3, 0, 1}}},
            {principal(typePointeur, "lire(z)(z->Cible, &z->Second.X); retourner *lire(z)(z->Cible, &z->Objet.X) + 1;"),
             redirection, 41, 41, 41, 41, false, false, 2, {{3, 2, 0, 1}, {3, 2, 1, 0}}},
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
        {
            const auto source = declarations + valides[index].Texte;
            std::optional<GsPP::CodeMachine> reference;
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
            {
                const auto nom = "structure-pointeur-reference-callback-valide-" + std::to_string(index);
                const auto resultat = AnalyserSemantiqueValide(syntaxe, semantique, texte, nom);
                auto programme = GsPP::AnalyseurSyntaxique(GsPP::Lexeur(texte, nom).Analyser(), nom).Analyser();
                GsPP::AnalyseurSemantique().Analyser(programme);
                const auto verifierDisposition = [&](std::string_view nomStructure, std::size_t taille,
                    std::size_t alignement, const std::vector<std::size_t>& decalages) {
                    const auto structure = std::find_if(programme.Structures.begin(), programme.Structures.end(),
                        [&](const auto& candidate) { return candidate.NomComplet() == nomStructure; });
                    Exiger(structure != programme.Structures.end() && structure->Taille == taille
                            && structure->Alignement == alignement && structure->Champs.size() == decalages.size(),
                        "disposition de structure référencée différente du pont hôte : " + nom);
                    for (std::size_t champ = 0; champ < decalages.size(); ++champ)
                        Exiger(structure->Champs[champ].Decalage == decalages[champ],
                            "décalage du stockage référencé différent : " + nom);
                };
                verifierDisposition("P", sizeof(StructureReferenceHote), alignof(StructureReferenceHote),
                    {offsetof(StructureReferenceHote, X), offsetof(StructureReferenceHote, Y), offsetof(StructureReferenceHote, Adresse)});
                verifierDisposition("Z", sizeof(DonneesStructuresPointeursHote), alignof(DonneesStructuresPointeursHote),
                    {offsetof(DonneesStructuresPointeursHote, Objet), offsetof(DonneesStructuresPointeursHote, Second),
                     offsetof(DonneesStructuresPointeursHote, Cible), offsetof(DonneesStructuresPointeursHote, CibleConstante),
                     offsetof(DonneesStructuresPointeursHote, Mutation), offsetof(DonneesStructuresPointeursHote, Lecture),
                     offsetof(DonneesStructuresPointeursHote, Redirection), offsetof(DonneesStructuresPointeursHote, RedirectionConstante)});
                std::unordered_map<std::uint64_t, std::uint64_t> types;
                for (const auto& fonction : programme.Fonctions)
                    for (const auto& parametre : fonction.Parametres)
                        if (parametre.Nom != "soi")
                            types.emplace((static_cast<std::uint64_t>(parametre.Position.Ligne) << 32U) | parametre.Position.Colonne,
                                HacherTypeDeclaration(parametre.Type));
                std::size_t nombreParametres = 0;
                for (const auto& symbole : resultat.Symboles)
                    if (symbole.Genre == 8)
                    {
                        ++nombreParametres;
                        const auto& declaration = resultat.Noeuds[symbole.IndexNoeud];
                        const auto type = types.find((static_cast<std::uint64_t>(declaration.Ligne) << 32U) | declaration.Colonne);
                        Exiger(type != types.end() && symbole.HachageType == type->second,
                            "signature de structure ou pointeur référencé différente : " + nom);
                    }
                Exiger(nombreParametres == types.size(), "paramètres de structures ou pointeurs référencés manquants : " + nom);
                const auto machine = GsPP::GenerateurX64().Generer(programme);
                if (reference)
                    Exiger(machine.Texte == reference->Texte && machine.Donnees == reference->Donnees,
                        "octets bilingues des structures ou pointeurs référencés différents : " + nom);
                else reference = machine;
                const auto contenu = GsPP::EcrivainGsE().Construire(machine, "Principal");
                Exiger(contenu == GsPP::EcrivainGsE().Construire(GsPP::GenerateurX64().Generer(programme), "Principal"),
                    "image des structures ou pointeurs référencés non reproductible : " + nom);
                ZoneExecutable zone(AlignerPage(Lire64(contenu, 48)));
                const auto image = GsPP::ChargeurGsE().Charger(contenu, zone.Base());
                Exiger(image.Imports.empty(), "un import statique a été ajouté : " + nom);
                zone.Copier(image.Memoire);
                DonneesStructuresPointeursHote donnees{{valides[index].XAvant, {7, 9}, nullptr},
                    {valides[index].SecondAvant, {11, 13}, nullptr}, nullptr, nullptr,
                    &MuterStructureReferenceHote, &LireStructureConstanteHote,
                    &RedirigerPointeurReferenceHote, &RedirigerPointeurConstantReferenceHote};
                donnees.Objet.Adresse = &donnees.Objet.Y[0];
                donnees.Second.Adresse = &donnees.Second.Y[0];
                donnees.Cible = &donnees.Objet.X;
                donnees.CibleConstante = &donnees.Objet.X;
                TraceStructuresPointeursHote = {};
                NombreStructuresPointeursHote = 0;
                NombreLecturesStructuresPointeursHote = 0;
                using FonctionTest = std::int32_t (GS_ABI_HOTE *)(std::uint64_t, DonneesStructuresPointeursHote*);
                const auto valeur = reinterpret_cast<FonctionTest>(image.AdressePointEntree)(valides[index].Callback, &donnees);
                Exiger(valeur == 42 && donnees.Objet.X == valides[index].XApres && donnees.Second.X == valides[index].SecondApres
                        && donnees.Objet.Y[0] == 7 && donnees.Objet.Y[1] == valides[index].YApres
                        && donnees.Second.Y[0] == 11 && donnees.Second.Y[1] == 13
                        && donnees.Objet.Adresse == &donnees.Objet.Y[0] && donnees.Second.Adresse == &donnees.Second.Y[0]
                        && donnees.Cible == (valides[index].CibleSeconde ? &donnees.Second.X : &donnees.Objet.X)
                        && donnees.CibleConstante == (valides[index].CibleConstanteSeconde ? &donnees.Second.X : &donnees.Objet.X)
                        && donnees.Mutation == &MuterStructureReferenceHote && donnees.Lecture == &LireStructureConstanteHote
                        && donnees.Redirection == &RedirigerPointeurReferenceHote
                        && donnees.RedirectionConstante == &RedirigerPointeurConstantReferenceHote,
                    "identité ou mutation des structures et pointeurs inattendue : " + nom);
                Exiger(NombreLecturesStructuresPointeursHote == valides[index].NombreLectures
                        && NombreStructuresPointeursHote == valides[index].Evenements.size()
                        && NombreStructuresPointeursHote <= TraceStructuresPointeursHote.size(),
                    "callback réévalué ou événement de structure/pointeur manquant : " + nom);
                const std::array<const void*, 4> arguments{&donnees.Objet, &donnees.Second, &donnees.Cible, &donnees.CibleConstante};
                const std::array<std::uintptr_t, 2> cibles{reinterpret_cast<std::uintptr_t>(&donnees.Objet.X),
                    reinterpret_cast<std::uintptr_t>(&donnees.Second.X)};
                for (std::size_t evenement = 0; evenement < valides[index].Evenements.size(); ++evenement)
                {
                    const auto& attendu = valides[index].Evenements[evenement];
                    const auto& obtenu = TraceStructuresPointeursHote[evenement];
                    const auto avant = attendu.Genre <= 2 ? attendu.Avant : cibles.at(attendu.Avant);
                    const auto apres = attendu.Genre <= 2 ? attendu.Apres : cibles.at(attendu.Apres);
                    Exiger(obtenu.Genre == attendu.Genre && obtenu.Adresse == arguments.at(attendu.IndexArgument)
                            && obtenu.Avant == avant && obtenu.Apres == apres,
                        "structure copiée, mauvais emplacement de pointeur ou ordre différent : " + nom);
                }
            }
        }
        const auto fonction = [](std::string_view type, std::string_view corps) {
            return "publique vide F(" + std::string(type) + " lire, Z* z) { " + std::string(corps) + " }";
        };
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {fonction(typeMutation, "lire(z)({42}); Absente;"), 69},
            {fonction(typeMutation, "constante P& valeur = z->Objet; lire(z)(valeur); Absente;"), 55},
            {"structure Q { entier32 X; entier32 Y[2]; entier32* Adresse; }; "
             + fonction(typeMutation, "Q autre = {42, {7, 9}, &z->Objet.X}; lire(z)(autre); Absente;"), 55},
            {fonction(typeMutation, "lire(z)(z->Objet.Inconnue); Absente;"), 24},
            {fonction(typeMutation, "lire(z)(Absente, 0);"), 54},
            {fonction(typeMutation, "lire(Absente, 0)(Absente);"), 54},
            {fonction(typeLecture, "lire(z)(z->Objet).X = 42; Absente;"), 71},
            {fonction(typeLecture, "lire(z)(z->Objet).Y[1] = 42; Absente;"), 71},
            {fonction(typeLecture, "P& liaison = lire(z)(z->Objet); Absente;"), 69},
            {fonction(typeLecture, "P* adresse = &lire(z)(z->Objet); Absente;"), 45},
            {fonction(typeMutation, "P& liaison = lire(z); Absente;"), 69},
            {fonction(typeMutation, "lire(z)(z->Objet).X = vrai; Absente;"), 73},
            {fonction(typePointeur, "lire(z)(&z->Objet.X, &z->Second.X); Absente;"), 55},
            {fonction(typePointeur, "lire(z)(z->CibleConstante, &z->Second.X); Absente;"), 55},
            {fonction(typePointeur, "entier64 autre = 41; lire(z)(z->Cible, &autre); Absente;"), 55},
            {fonction(typePointeur, "lire(z)(z->Cible, Absente, 0);"), 54},
            {fonction(typePointeur, "lire(z)(z->Cible, &z->Second.X) = vrai; Absente;"), 73},
            {fonction(typePointeurConstant, "*lire(z)(z->CibleConstante, convertir<constante entier32*>(&z->Second.X)) = 42; Absente;"), 71},
            {fonction(typePointeurConstant, "entier32*& liaison = lire(z)(z->CibleConstante, "
                "convertir<constante entier32*>(&z->Second.X)); Absente;"), 69},
            {fonction(typePointeurConstant, "lire(z)(z->Cible, convertir<constante entier32*>(&z->Second.X)); Absente;"), 55},
            {fonction(typePointeurConstant, "lire(z)(z->CibleConstante, &z->Second.X); Absente;"), 55},
            {"classe C { entier32 X = lire(z)(z->Objet).Inconnue; publique: constructeur(" + typeLecture
             + " lire, Z* z) { Absente; } };", 24},
            {"classe C { entier32 X = *lire(z)(z->CibleConstante); publique: constructeur(" + typePointeurConstant
             + " lire, Z* z) { Absente; } };", 54},
            {"classe Base { publique: constructeur(P& objet) {} }; classe C : publique Base { publique: constructeur("
             + typeMutation + " lire, Z* z) : parent(lire(z)({42})) { Absente; } };", 69},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
        {
            const auto source = declarations + refus[index].first;
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
                ComparerErreurSemantique(syntaxe, semantique, texte, refus[index].second,
                    "structure-pointeur-reference-callback-refuse-" + std::to_string(index));
        }
        std::cout << "Structures et pointeurs référencés des callbacks imbriqués : " << valides.size()
                  << " corpus bilingues exécutés, identité, qualifications et redirections vérifiées.\n";
    }

    /**
     * <résumé>Compare conversions, qualifications et priorités des groupes mêlant méthodes et fonctions.</résumé>
     * Les corpus exécutés contrôlent la déclaration choisie et les mutations des référents.
     **/
    void TesterReferencesGroupesMixtesConstructions(
        AnalyseurDeclarationsAutoHeberge syntaxe, AnalyseurSemantiqueAutoHeberge semantique)
    {
        const auto groupe = [](std::string_view typeMethode, std::string_view typeLibre,
            std::string_view corpsMethode, std::string_view corpsLibre) {
            return "classe C { publique: entier32 Choisir(" + std::string(typeMethode) + " valeur) { "
                + std::string(corpsMethode) + " } }; espace C { publique entier32 Choisir(C& objet, "
                + std::string(typeLibre) + " valeur) { " + std::string(corpsLibre) + " } } ";
        };
        const auto principal = [](std::string_view corps) {
            return "publique entier32 Principal() { " + std::string(corps) + " }";
        };
        const std::string scalaire = groupe("entier32&", "constante entier32&",
            "valeur = valeur + 1; retourner valeur;", "retourner valeur + 1;");
        const std::string pointeur = groupe("entier32*&", "constante entier32*&",
            "*valeur = 42; retourner *valeur;", "retourner *valeur + 1;");
        const std::string heritage = "classe B { publique: entier32 X; }; classe D : publique B { publique: "
            "virtuel entier32 Marqueur() { retourner 1; } }; ";
        struct Corpus { std::string Texte; bool Methode; };
        const std::vector<Corpus> valides{
            {groupe("entier32&", "booléen", "valeur = 42; retourner valeur;", "retourner 1;")
             + principal("C c; entier32 x = 41; entier32 resultat = c.Choisir(x); retourner resultat + x - 42;"), true},
            {groupe("booléen", "constante entier32&", "retourner 1;", "retourner valeur + 1;")
             + principal("C c; constante entier32 x = 41; retourner c.Choisir(x);"), false},
            {scalaire + principal("C c; constante entier32 x = 41; retourner C::Choisir(c, x);"), false},
            {groupe("constante entier32&", "booléen", "retourner valeur;", "retourner 1;")
             + principal("C c; entier32 x = 42; retourner c.Choisir(x);"), true},
            {groupe("constante entier32&", "booléen", "retourner valeur;", "retourner 1;")
             + principal("C c; constante entier32 x = 42; retourner c.Choisir(x);"), true},
            {groupe("entier32&", "entier32", "retourner 1;", "retourner valeur;")
             + principal("C c; retourner c.Choisir(42);"), false},
            {groupe("entier32&", "entier64", "valeur = 42; retourner valeur;", "retourner 1;")
             + principal("C c; entier32 x = 41; entier32 resultat = C::Choisir(c, x); retourner resultat + x - 42;"), true},
            {pointeur + principal("C c; entier32 x = 41; entier32* p = &x; "
                "entier32 resultat = c.Choisir(p); retourner resultat + x - 42;"), true},
            {pointeur + principal("C c; entier32 x = 41; constante entier32* p = convertir<constante entier32*>(&x); "
                "retourner c.Choisir(p) + x - 41;"), false},
            {groupe("entier32*", "constante entier32*", "retourner 1;", "retourner *valeur + 1;")
             + principal("C c; entier32 x = 41; retourner c.Choisir(convertir<constante entier32*>(&x));"), false},
            {heritage + groupe("B&", "D&", "retourner 1;", "retourner valeur.X;")
             + principal("C c; D d; d.X = 42; retourner c.Choisir(d);"), false},
            {heritage + groupe("D&", "B&", "retourner valeur.X;", "retourner 1;")
             + principal("C c; D d; d.X = 42; retourner C::Choisir(c, d);"), true},
            {"classe C { publique: entier32 Choisir(entier32& valeur) { retourner 1; } }; "
             "espace C { publique entier32 Choisir(constante C& objet, entier32& valeur) { valeur = 42; retourner valeur; } } "
             + principal("C c; constante C* p = convertir<constante C*>(&c); entier32 x = 41; "
                "entier32 resultat = p->Choisir(x); retourner resultat + x - 42;"), false},
            {groupe("constante entier32&", "booléen", "retourner valeur;", "retourner 1;")
             + principal("volatile C c; entier32 x = 42; retourner c.Choisir(x);"), true},
            {pointeur + "alias Vue = C; " + principal("Vue c; entier32 x = 41; "
                "constante entier32* p = convertir<constante entier32*>(&x); retourner C::Choisir(c, p);"), false},
            {scalaire + "classe H { entier32 X = objet.Choisir(x); publique: constructeur(C& objet, constante entier32& x) {} "
             "entier32 Lire() { retourner soi.X; } }; "
             + principal("C c; constante entier32 x = 41; H h(c, x); retourner h.Lire();"), false},
            {scalaire + "classe H { entier32 X; publique: constructeur(C& objet, constante entier32& x) : X(objet.Choisir(x)) {} "
             "entier32 Lire() { retourner soi.X; } }; "
             + principal("C c; constante entier32 x = 41; H h(c, x); retourner h.Lire();"), false},
            {scalaire + "classe Base { publique: entier32 X; constructeur(entier32 x) : X(x) {} }; "
             "classe H : publique Base { publique: constructeur(C& objet, constante entier32& x) : parent(objet.Choisir(x)) {} }; "
             + principal("C c; constante entier32 x = 41; H h(c, x); retourner h.X;"), false},
            {scalaire + "classe M { publique: entier32 X; constructeur(entier32 x) : X(x) {} }; "
             "classe H { M m; publique: constructeur(C& objet, constante entier32& x) : m(objet.Choisir(x)) {} "
             "entier32 Lire() { retourner soi.m.X; } }; "
             + principal("C c; constante entier32 x = 41; H h(c, x); retourner h.Lire();"), false},
            {scalaire + "classe H { entier32 X; publique: constructeur(C& objet, constante entier32& x) : soi(objet.Choisir(x)) {} "
             "constructeur(entier32 x) : X(x) {} entier32 Lire() { retourner soi.X; } }; "
             + principal("C c; constante entier32 x = 41; H h(c, x); retourner h.Lire();"), false},
            {heritage + groupe("B&", "booléen", "retourner valeur.X;", "retourner 1;")
             + principal("C c; D d; d.X = 42; retourner c.Choisir(d);"), true},
            {heritage + groupe("booléen", "constante B&", "retourner 1;", "retourner valeur.X;")
             + principal("C c; D d; d.X = 42; constante D& vue = d; retourner c.Choisir(vue);"), false},
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
        {
            std::optional<GsPP::CodeMachine> reference;
            for (const auto& texte : {valides[index].Texte, TraduireCorpusConversions(valides[index].Texte)})
            {
                const auto nom = "reference-groupe-mixte-construction-valide-" + std::to_string(index);
                const auto resultat = AnalyserSemantiqueValide(syntaxe, semantique, texte, nom);
                auto programme = GsPP::AnalyseurSyntaxique(GsPP::Lexeur(texte, nom).Analyser(), nom).Analyser();
                GsPP::AnalyseurSemantique().Analyser(programme);
                std::unordered_map<std::uint64_t, std::vector<const GsPP::Fonction*>> cibles;
                const auto visiter = [&](auto&& self, const GsPP::Expression& expression) -> void {
                    switch (expression.Genre)
                    {
                        case GsPP::GenreExpression::Appel:
                        {
                            const auto& appel = static_cast<const GsPP::ExpressionAppel&>(expression);
                            const auto cible = std::find_if(programme.Fonctions.begin(), programme.Fonctions.end(),
                                [&](const auto& fonction) { return fonction.NomComplet() == appel.NomDirect; });
                            if (cible != programme.Fonctions.end() && cible->Nom == "Choisir")
                            {
                                Exiger(cible->EstMethode == valides[index].Methode,
                                    "cible prévue du groupe mixte différente du bootstrap : " + nom);
                                const auto& origine = *appel.Cible;
                                cibles[(static_cast<std::uint64_t>(origine.Position.Ligne) << 32U)
                                    | origine.Position.Colonne].push_back(&*cible);
                            }
                            self(self, *appel.Cible);
                            for (const auto& argument : appel.Arguments) self(self, *argument);
                            break;
                        }
                        case GsPP::GenreExpression::Binaire:
                        {
                            const auto& binaire = static_cast<const GsPP::ExpressionBinaire&>(expression);
                            self(self, *binaire.Gauche);
                            self(self, *binaire.Droite);
                            break;
                        }
                        case GsPP::GenreExpression::Unaire:
                            self(self, *static_cast<const GsPP::ExpressionUnaire&>(expression).Operande);
                            break;
                        case GsPP::GenreExpression::Conversion:
                            self(self, *static_cast<const GsPP::ExpressionConversion&>(expression).Valeur);
                            break;
                        case GsPP::GenreExpression::Membre:
                            self(self, *static_cast<const GsPP::ExpressionMembre&>(expression).Objet);
                            break;
                        case GsPP::GenreExpression::Affectation:
                        {
                            const auto& affectation = static_cast<const GsPP::ExpressionAffectation&>(expression);
                            self(self, *affectation.Cible);
                            self(self, *affectation.Valeur);
                            break;
                        }
                        default: break;
                    }
                };
                for (const auto& fonction : programme.Fonctions)
                {
                    if (fonction.Corps)
                        for (const auto& instruction : fonction.Corps->Instructions)
                        {
                            if (instruction->Genre == GsPP::GenreInstruction::Retour)
                            {
                                const auto& valeur = static_cast<const GsPP::InstructionRetour&>(*instruction).Valeur;
                                if (valeur) visiter(visiter, *valeur);
                            }
                            else if (instruction->Genre == GsPP::GenreInstruction::Expression)
                                visiter(visiter, *static_cast<const GsPP::InstructionExpression&>(*instruction).Valeur);
                            else if (instruction->Genre == GsPP::GenreInstruction::Variable)
                            {
                                const auto& variable = static_cast<const GsPP::InstructionVariable&>(*instruction);
                                if (variable.Initialiseur) visiter(visiter, *variable.Initialiseur);
                                for (const auto& argument : variable.ArgumentsConstruction) visiter(visiter, *argument);
                            }
                        }
                    for (const auto& champ : fonction.InitialiseursChamps)
                    {
                        for (const auto& argument : champ.Arguments) visiter(visiter, *argument);
                        if (champ.InitialiseurParDefaut) visiter(visiter, *champ.InitialiseurParDefaut);
                    }
                    for (const auto& argument : fonction.ArgumentsConstructeurBase) visiter(visiter, *argument);
                    for (const auto& argument : fonction.ArgumentsConstructeurDelegue) visiter(visiter, *argument);
                }
                std::size_t nombreAttendu = 0;
                for (const auto& [position, selections] : cibles) nombreAttendu += selections.size();
                Exiger(nombreAttendu != 0, "appel mixte absent du corpus : " + nom);
                std::size_t nombre = 0;
                for (const auto& resolution : resultat.Resolutions)
                {
                    const auto& origine = resultat.Noeuds[resolution.IndexNoeud];
                    if ((origine.Genre != 24 && origine.Genre != 29)
                        || origine.Parent >= resultat.Noeuds.size()
                        || resultat.Noeuds[origine.Parent].Genre != 28
                        || resolution.IndexNoeud != origine.Parent + 1) continue;
                    const auto& declaration = resultat.Noeuds[resultat.Symboles[resolution.IndexSymbole].IndexNoeud];
                    if (declaration.HachageNom != HacherTexte("Choisir")) continue;
                    const auto selection = cibles.find((static_cast<std::uint64_t>(origine.Ligne) << 32U) | origine.Colonne);
                    Exiger(selection != cibles.end(), "appel mixte publié absent du bootstrap : " + nom);
                    const auto attendu = std::find_if(selection->second.begin(), selection->second.end(),
                        [&](const auto* fonction) {
                            return declaration.Ligne == fonction->Position.Ligne && declaration.Colonne == fonction->Position.Colonne
                                && resolution.HachageType == HacherTypeDeclaration(fonction->TypeRetour)
                                && ((resolution.Drapeaux & 32U) != 0) == fonction->EstMethode;
                        });
                    Exiger(attendu != selection->second.end(), "cible ou retour du groupe mixte différent : " + nom);
                    selection->second.erase(attendu);
                    ++nombre;
                }
                Exiger(nombre == nombreAttendu, "sélection mixte omise ou dupliquée : " + nom);
                const auto machine = GsPP::GenerateurX64().Generer(programme);
                if (reference)
                    Exiger(machine.Texte == reference->Texte && machine.Donnees == reference->Donnees,
                        "octets bilingues des groupes mixtes différents : " + nom);
                else reference = machine;
                const auto contenu = GsPP::EcrivainGsE().Construire(machine, "Principal");
                Exiger(contenu == GsPP::EcrivainGsE().Construire(GsPP::GenerateurX64().Generer(programme), "Principal"),
                    "image des groupes mixtes non reproductible : " + nom);
                ZoneExecutable zone(AlignerPage(Lire64(contenu, 48)));
                const auto image = GsPP::ChargeurGsE().Charger(contenu, zone.Base());
                Exiger(image.Imports.empty(), "import d'hôte ajouté par les groupes mixtes : " + nom);
                zone.Copier(image.Memoire);
                using FonctionTest = std::int32_t (GS_ABI_HOTE *)();
                Exiger(reinterpret_cast<FonctionTest>(image.AdressePointEntree)() == 42,
                    "surcharge exécutée ou mutation du référent incorrecte : " + nom);
            }
        }
        const auto fonction = [](std::string_view type, std::string_view corps) {
            return "publique vide F(C& objet, " + std::string(type) + " x) { " + std::string(corps) + " }";
        };
        const std::string ambigu = groupe("entier32&", "entier32", "retourner 1;", "retourner 2;");
        const std::string nonCompatible = groupe("entier32&", "booléen", "retourner 1;", "retourner 2;");
        const std::string prive = "classe C { privée: entier32 Choisir(entier32& valeur) { retourner valeur; } }; "
            "espace C { publique entier32 Choisir(C& objet, booléen valeur) { retourner 2; } } ";
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {scalaire + fonction("entier32&", "objet.Choisir(x); Absente;"), 22},
            {scalaire + fonction("entier32&", "C::Choisir(objet, x); Absente;"), 22},
            {ambigu + fonction("entier32&", "objet.Choisir(x); Absente;"), 22},
            {nonCompatible + fonction("constante entier32&", "objet.Choisir(x); Absente;"), 21},
            {nonCompatible + fonction("entier64&", "objet.Choisir(x); Absente;"), 21},
            {nonCompatible + fonction("entier32&", "objet.Choisir(42); Absente;"), 21},
            {nonCompatible + fonction("entier32&", "objet.Choisir(x + 1); Absente;"), 21},
            {prive + fonction("entier32&", "objet.Choisir(x); Absente;"), 25},
            {prive + fonction("entier32&", "objet.Choisir(Absente, 0);"), 21},
            {nonCompatible + fonction("entier32&", "objet.Choisir(x, Absente);"), 21},
            {pointeur + fonction("entier32**&", "objet.Choisir(x); Absente;"), 21},
            {pointeur + fonction("entier32&", "objet.Choisir(&x); Absente;"), 21},
            {heritage + groupe("B&", "booléen", "retourner 1;", "retourner 2;")
             + fonction("constante D&", "objet.Choisir(x); Absente;"), 21},
            {heritage + groupe("B**", "booléen", "retourner 1;", "retourner 2;")
             + fonction("D**", "objet.Choisir(x); Absente;"), 21},
            {scalaire + "classe H { entier32 X = objet.Choisir(x); publique: constructeur(C& objet, entier32& x) { Absente; } };", 22},
            {nonCompatible + "classe H { entier32 X = objet.Choisir(x); publique: constructeur(C& objet, constante entier32& x) { Absente; } };", 21},
            {prive + "classe H { entier32 X = objet.Choisir(x); publique: constructeur(C& objet, entier32& x) { Absente; } };", 25},
            {scalaire + "classe Base { publique: constructeur(entier32 x) {} }; classe H : publique Base { publique: "
             "constructeur(C& objet, entier32& x) : parent(objet.Choisir(x)) { Absente; } };", 22},
            {scalaire + "classe M { publique: constructeur(entier32 x) {} }; classe H { M m; publique: "
             "constructeur(C& objet, entier32& x) : m(objet.Choisir(x)) { Absente; } };", 22},
            {scalaire + "classe H { publique: constructeur(C& objet, entier32& x) : soi(objet.Choisir(x)) { Absente; } "
             "constructeur(entier32 x) {} };", 22},
            {nonCompatible + "classe Base { publique: constructeur(entier32 x) {} }; classe H : publique Base { publique: "
             "constructeur(C& objet, constante entier32& x) : parent(objet.Choisir(x)) { Absente; } };", 21},
            {prive + "classe Base { publique: constructeur(entier32 x) {} }; classe H : publique Base { publique: "
             "constructeur(C& objet, entier32& x) : parent(objet.Choisir(x)) { Absente; } };", 25},
            {scalaire + "classe H { entier32 X = objet.Choisir(x); entier32 Y = Absente; publique: "
             "constructeur(C& objet, entier32& x) {} };", 22},
            {nonCompatible + "classe H { entier32 X = objet.Choisir(x, Absente); publique: "
             "constructeur(C& objet, entier32& x) {} };", 21},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
            for (const auto& texte : {refus[index].first, TraduireCorpusConversions(refus[index].first)})
                ComparerErreurSemantique(syntaxe, semantique, texte, refus[index].second,
                    "reference-groupe-mixte-construction-refuse-" + std::to_string(index));
        std::cout << "Références des groupes mixtes et constructions : " << valides.size()
                  << " corpus bilingues exécutés, surcharges et mutations vérifiées.\n";
    }

    /**
     * <résumé>Compare les opérateurs des corpus bilingues et contrôle leur exécution native.</résumé>
     * Le parcours couvre aussi les éléments d'agrégats et les expressions des constructions.
     **/
    void VerifierOperateursReferencesBilingues(
        AnalyseurDeclarationsAutoHeberge syntaxe, AnalyseurSemantiqueAutoHeberge semantique,
        const std::string& source, bool methode, std::int32_t traceAttendue,
        std::size_t nombreOperateursAttendu, const std::string& nom)
    {
        std::optional<GsPP::CodeMachine> reference;
        for (const auto& texte : {source, TraduireCorpusConversions(source)})
        {
            const auto resultat = AnalyserSemantiqueValide(syntaxe, semantique, texte, nom);
            auto programme = GsPP::AnalyseurSyntaxique(GsPP::Lexeur(texte, nom).Analyser(), nom).Analyser();
            GsPP::AnalyseurSemantique().Analyser(programme);
            std::unordered_map<std::uint64_t, std::vector<const GsPP::Fonction*>> cibles;
            const auto visiter = [&](auto&& self, const GsPP::Expression& expression) -> void {
                std::string symbole;
                switch (expression.Genre)
                {
                    case GsPP::GenreExpression::Agregat:
                        for (const auto& element : static_cast<const GsPP::ExpressionAgregat&>(expression).Elements)
                            self(self, *element);
                        break;
                    case GsPP::GenreExpression::Unaire:
                    {
                        const auto& unaire = static_cast<const GsPP::ExpressionUnaire&>(expression);
                        symbole = unaire.NomSurcharge;
                        self(self, *unaire.Operande);
                        break;
                    }
                    case GsPP::GenreExpression::Binaire:
                    {
                        const auto& binaire = static_cast<const GsPP::ExpressionBinaire&>(expression);
                        symbole = binaire.NomSurcharge;
                        self(self, *binaire.Gauche);
                        self(self, *binaire.Droite);
                        break;
                    }
                    case GsPP::GenreExpression::Appel:
                    {
                        const auto& appel = static_cast<const GsPP::ExpressionAppel&>(expression);
                        self(self, *appel.Cible);
                        for (const auto& argument : appel.Arguments) self(self, *argument);
                        break;
                    }
                    case GsPP::GenreExpression::Affectation:
                    {
                        const auto& affectation = static_cast<const GsPP::ExpressionAffectation&>(expression);
                        self(self, *affectation.Cible);
                        self(self, *affectation.Valeur);
                        break;
                    }
                    case GsPP::GenreExpression::Conversion:
                        self(self, *static_cast<const GsPP::ExpressionConversion&>(expression).Valeur);
                        break;
                    case GsPP::GenreExpression::Membre:
                        self(self, *static_cast<const GsPP::ExpressionMembre&>(expression).Objet);
                        break;
                    case GsPP::GenreExpression::Index:
                    {
                        const auto& indexation = static_cast<const GsPP::ExpressionIndex&>(expression);
                        self(self, *indexation.Objet);
                        self(self, *indexation.Indice);
                        break;
                    }
                    default: break;
                }
                if (symbole.empty()) return;
                const auto cible = std::find_if(programme.Fonctions.begin(), programme.Fonctions.end(),
                    [&](const auto& fonction) { return fonction.NomComplet() == symbole; });
                Exiger(cible != programme.Fonctions.end() && cible->EstMethode == methode,
                    "cible prévue de l'opérateur référencé différente du bootstrap : " + nom);
                cibles[(static_cast<std::uint64_t>(expression.Position.Ligne) << 32U)
                    | expression.Position.Colonne].push_back(&*cible);
            };
            for (const auto& fonction : programme.Fonctions)
            {
                if (fonction.Corps)
                    for (const auto& instruction : fonction.Corps->Instructions)
                    {
                        if (instruction->Genre == GsPP::GenreInstruction::Retour)
                        {
                            const auto& valeur = static_cast<const GsPP::InstructionRetour&>(*instruction).Valeur;
                            if (valeur) visiter(visiter, *valeur);
                        }
                        else if (instruction->Genre == GsPP::GenreInstruction::Expression)
                            visiter(visiter, *static_cast<const GsPP::InstructionExpression&>(*instruction).Valeur);
                        else if (instruction->Genre == GsPP::GenreInstruction::Variable)
                        {
                            const auto& variable = static_cast<const GsPP::InstructionVariable&>(*instruction);
                            if (variable.Initialiseur) visiter(visiter, *variable.Initialiseur);
                            for (const auto& argument : variable.ArgumentsConstruction) visiter(visiter, *argument);
                        }
                    }
                for (const auto& champ : fonction.InitialiseursChamps)
                {
                    for (const auto& argument : champ.Arguments) visiter(visiter, *argument);
                    if (champ.InitialiseurParDefaut) visiter(visiter, *champ.InitialiseurParDefaut);
                }
                for (const auto& argument : fonction.ArgumentsConstructeurBase) visiter(visiter, *argument);
                for (const auto& argument : fonction.ArgumentsConstructeurDelegue) visiter(visiter, *argument);
            }
            std::size_t nombreAttendu = 0;
            for (const auto& [position, selections] : cibles) nombreAttendu += selections.size();
            Exiger(nombreAttendu == nombreOperateursAttendu,
                "nombre d'opérateurs prévu différent du bootstrap : " + nom);
            std::size_t nombre = 0;
            for (const auto& resolution : resultat.Resolutions)
            {
                if ((resolution.Drapeaux & 256U) == 0) continue;
                const auto& origine = resultat.Noeuds[resolution.IndexNoeud];
                const auto selection = cibles.find((static_cast<std::uint64_t>(origine.Ligne) << 32U) | origine.Colonne);
                Exiger(selection != cibles.end(), "opérateur publié absent du bootstrap : " + nom);
                const auto& declaration = resultat.Noeuds[resultat.Symboles[resolution.IndexSymbole].IndexNoeud];
                const auto attendu = std::find_if(selection->second.begin(), selection->second.end(),
                    [&](const auto* fonction) {
                        return declaration.Ligne == fonction->Position.Ligne && declaration.Colonne == fonction->Position.Colonne
                            && resolution.HachageType == HacherTypeDeclaration(fonction->TypeRetour)
                            && ((resolution.Drapeaux & 32U) != 0) == fonction->EstMethode;
                    });
                Exiger(attendu != selection->second.end(), "opérateur, retour ou drapeau différent du bootstrap : " + nom);
                selection->second.erase(attendu);
                ++nombre;
            }
            Exiger(nombre == nombreAttendu, "opérateur référencé omis ou dupliqué : " + nom);
            const auto machine = GsPP::GenerateurX64().Generer(programme);
            if (reference)
                Exiger(machine.Texte == reference->Texte && machine.Donnees == reference->Donnees,
                    "octets bilingues des opérateurs référencés différents : " + nom);
            else reference = machine;
            const auto contenu = GsPP::EcrivainGsE().Construire(machine, "Principal");
            Exiger(contenu == GsPP::EcrivainGsE().Construire(GsPP::GenerateurX64().Generer(programme), "Principal"),
                "image des opérateurs référencés non reproductible : " + nom);
            ZoneExecutable zone(AlignerPage(Lire64(contenu, 48)));
            const auto image = GsPP::ChargeurGsE().Charger(contenu, zone.Base());
            Exiger(image.Imports.empty(), "import ajouté par les opérateurs référencés : " + nom);
            zone.Copier(image.Memoire);
            using FonctionTest = std::int32_t (GS_ABI_HOTE *)();
            Exiger(reinterpret_cast<FonctionTest>(image.AdressePointEntree)() == 42,
                "résultat ou mutation des opérateurs référencés incorrect : " + nom);
            const auto trace = image.ChercherExport("Trace");
            Exiger(trace.has_value(), "trace des opérateurs référencés absente : " + nom);
            std::int32_t valeurTrace = 0;
            std::memcpy(&valeurTrace, reinterpret_cast<const void*>(static_cast<std::uintptr_t>(*trace)), sizeof(valeurTrace));
            Exiger(valeurTrace == traceAttendue, "ordre ou nombre d'appels d'opérateurs incorrect : " + nom);
        }
    }

    /**
     * <résumé>Compare les opérateurs mixtes référencés, leurs mutations et leurs contextes de construction.</résumé>
     * Une trace exportée sépare les résolutions sémantiques des appels réellement exécutés.
     **/
    void TesterReferencesOperateursMixtesConstructions(
        AnalyseurDeclarationsAutoHeberge syntaxe, AnalyseurSemantiqueAutoHeberge semantique)
    {
        const auto groupe = [](std::string_view typeMethode, std::string_view typeLibre,
            std::string_view corpsMethode, std::string_view corpsLibre) {
            return "classe C { publique: entier32* Cible; entier32 opérateur+(" + std::string(typeMethode)
                + " valeur) { Trace = Trace * 10 + 1; " + std::string(corpsMethode)
                + " } }; espace C { publique entier32 opérateur+(C& objet, " + std::string(typeLibre)
                + " valeur) { Trace = Trace * 10 + 2; " + std::string(corpsLibre) + " } } ";
        };
        const auto principal = [](std::string_view corps) {
            return "publique entier32 Principal() { " + std::string(corps) + " }";
        };
        const std::string mutation = groupe("entier32&", "booléen",
            "valeur = valeur + 1; retourner valeur;", "retourner 1;");
        const std::string scalaire = groupe("entier32&", "constante entier32&",
            "valeur = valeur + 1; retourner valeur;", "retourner valeur + 1;");
        const std::string pointeur = groupe("entier32*&", "constante entier32*&",
            "valeur = soi.Cible; retourner *valeur;",
            "valeur = convertir<constante entier32*>(objet.Cible); retourner *valeur;");
        const std::string heritage = "classe B { publique: entier32 X; }; classe D : publique B { publique: "
            "virtuel entier32 Marqueur() { retourner 1; } }; ";
        struct Corpus {
            std::string Texte;
            bool Methode;
            std::int32_t Trace;
            std::size_t NombreOperateurs = 1;
        };
        const std::vector<Corpus> valides{
            {mutation + principal("C c; entier32 x = 41; entier32 resultat = c + x; retourner resultat + x - 42;"), true, 1},
            {mutation + principal("C c; entier32 valeurs[1] = {41}; entier32 resultat = c + valeurs[0]; "
                "retourner resultat + valeurs[0] - 42;"), true, 1},
            {mutation + principal("C c; entier32 x = 41; entier32* p = &x; entier32 resultat = c + *p; "
                "retourner resultat + x - 42;"), true, 1},
            {groupe("constante entier32&", "booléen", "retourner valeur;", "retourner 1;")
             + principal("C c; entier32 x = 42; retourner c + x;"), true, 1},
            {groupe("constante entier32&", "booléen", "retourner valeur;", "retourner 1;")
             + principal("C c; constante entier32 x = 42; retourner c + x;"), true, 1},
            {scalaire + principal("C c; constante entier32 x = 41; retourner c + x;"), false, 2},
            {groupe("entier32&", "entier32", "retourner 1;", "retourner valeur;")
             + principal("C c; retourner c + 42;"), false, 2},
            {groupe("entier32&", "entier32", "retourner 1;", "Trace = Trace * 10 + valeur; retourner valeur + 1;")
             + principal("C c; retourner c + (c + 40);"), false, 6061, 2},
            {groupe("entier32&", "booléen", "Trace = Trace * 10 + valeur; valeur = valeur + 1; retourner valeur;", "retourner 1;")
             + principal("C c; entier32 x = 19; entier32 y = 21; entier32 resultat = (c + x) + (c + y); "
                "retourner resultat + x + y - 42;"), true, 2931, 2},
            {pointeur + principal("C c; entier32 x = 41; entier32 y = 42; c.Cible = &y; entier32* p = &x; "
                "entier32 resultat = c + p; retourner resultat + *p + convertir<entier32>(p == &y) + x - 84;"), true, 1},
            {pointeur + principal("C c; entier32 x = 41; entier32 y = 42; c.Cible = &y; "
                "constante entier32* p = convertir<constante entier32*>(&x); entier32 resultat = c + p; "
                "retourner resultat + *p + convertir<entier32>(p == convertir<constante entier32*>(&y)) + x - 84;"), false, 2},
            {heritage + groupe("B&", "D&", "retourner 1;", "retourner valeur.X;")
             + principal("C c; D d; d.X = 42; retourner c + d;"), false, 2},
            {heritage + groupe("D&", "B&", "retourner valeur.X;", "retourner 1;")
             + principal("C c; D d; d.X = 42; retourner c + d;"), true, 1},
            {"classe C { publique: entier32 opérateur+(entier32& valeur) { Trace = Trace * 10 + 1; retourner 1; } }; "
             "espace C { publique entier32 opérateur+(constante C& objet, entier32& valeur) { "
             "Trace = Trace * 10 + 2; valeur = 42; retourner valeur; } } "
             + principal("C c; constante C& vue = c; entier32 x = 41; entier32 resultat = vue + x; "
                "retourner resultat + x - 42;"), false, 2},
            {mutation + principal("volatile C c; entier32 x = 41; entier32 resultat = c + x; "
                "retourner resultat + x - 42;"), true, 1},
            {scalaire + "classe H { entier32 X = objet + x; publique: constructeur(C& objet, constante entier32& x) {} "
             "entier32 Lire() { retourner soi.X; } }; "
             + principal("C c; constante entier32 x = 41; H h(c, x); retourner h.Lire();"), false, 2},
            {mutation + "classe H { entier32 X; publique: constructeur(C& objet, entier32& x) : X(objet + x) {} "
             "entier32 Lire() { retourner soi.X; } }; "
             + principal("C c; entier32 x = 41; H h(c, x); retourner h.Lire() + x - 42;"), true, 1},
            {scalaire + "classe Base { publique: entier32 X; constructeur(entier32 x) : X(x) {} }; "
             "classe H : publique Base { publique: constructeur(C& objet, constante entier32& x) : parent(objet + x) {} }; "
             + principal("C c; constante entier32 x = 41; H h(c, x); retourner h.X;"), false, 2},
            {mutation + "classe M { publique: entier32 X; constructeur(entier32 x) : X(x) {} }; "
             "classe H { M m; publique: constructeur(C& objet, entier32& x) : m(objet + x) {} "
             "entier32 Lire() { retourner soi.m.X; } }; "
             + principal("C c; entier32 x = 41; H h(c, x); retourner h.Lire() + x - 42;"), true, 1},
            {scalaire + "classe H { entier32 X; publique: constructeur(C& objet, constante entier32& x) : soi(objet + x) {} "
             "constructeur(entier32 x) : X(x) {} entier32 Lire() { retourner soi.X; } }; "
             + principal("C c; constante entier32 x = 41; H h(c, x); retourner h.Lire();"), false, 2},
            {"classe Base {}; classe C : publique Base { publique: entier32 opérateur~() { "
             "Trace = Trace * 10 + 1; retourner 42; } }; espace C { publique entier32 opérateur~(Base& objet) { "
             "Trace = Trace * 10 + 2; retourner 1; } } " + principal("C c; retourner ~c;"), true, 1},
            {"classe C { publique: entier32 opérateur~() { Trace = Trace * 10 + 1; retourner 1; } }; "
             "espace C { publique entier32 opérateur~(constante C& objet) { Trace = Trace * 10 + 2; retourner 42; } } "
             + principal("C c; constante C& vue = c; retourner ~vue;"), false, 2},
            {mutation + principal("C c; entier32 x = 41; faux && (c + x); retourner 42 + x - 41;"), true, 0},
            {scalaire + principal("C c; constante entier32 x = 41; vrai || (c + x); retourner 42 + x - 41;"), false, 0},
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
            VerifierOperateursReferencesBilingues(syntaxe, semantique,
                "publique entier32 Trace = 0; " + valides[index].Texte, valides[index].Methode,
                valides[index].Trace, valides[index].NombreOperateurs,
                "reference-operateur-mixte-construction-valide-" + std::to_string(index));
        const auto fonction = [](std::string_view type, std::string_view corps) {
            return "publique vide F(C& objet, " + std::string(type) + " x) { " + std::string(corps) + " }";
        };
        const std::string prive = "classe C { privée: entier32 opérateur+(entier32& valeur) { retourner valeur; } }; "
            "espace C { publique entier32 opérateur+(C& objet, booléen valeur) { retourner 2; } } ";
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {scalaire + fonction("entier32&", "objet + x; Absente;"), 22},
            {groupe("entier32&", "entier32", "retourner 1;", "retourner 2;")
             + fonction("entier32&", "objet + x; Absente;"), 22},
            {mutation + fonction("constante entier32&", "objet + x; Absente;"), 21},
            {mutation + fonction("entier32&", "faux && (objet + (x + 1)); Absente;"), 21},
            {mutation + fonction("entier32&", "objet + 42; Absente;"), 21},
            {mutation + fonction("entier32&", "objet + (x + 1); Absente;"), 21},
            {pointeur + fonction("entier32&", "objet + &x; Absente;"), 21},
            {pointeur + fonction("entier32**&", "objet + x; Absente;"), 21},
            {groupe("entier32*&", "booléen", "retourner 1;", "retourner 2;")
             + fonction("constante entier32*&", "objet + x; Absente;"), 21},
            {heritage + groupe("B&", "booléen", "retourner 1;", "retourner 2;")
             + fonction("constante D&", "objet + x; Absente;"), 21},
            {prive + fonction("entier32&", "objet + x; Absente;"), 25},
            {"classe C { privée: entier32 opérateur+(entier32& valeur) { retourner valeur; } }; "
             "espace C { publique entier32 opérateur+(C& objet, constante entier32& valeur) { retourner valeur; } } "
             + fonction("entier32&", "objet + x; Absente;"), 22},
            {scalaire + fonction("entier32&", "objet + (objet + x); Absente;"), 22},
            {scalaire + fonction("entier32&", "(objet + x) + Absente;"), 22},
            {scalaire + fonction("entier32&", "Absente + (objet + x);"), 18},
            {mutation + fonction("constante entier32&", "(objet + x)(Absente);"), 21},
            {scalaire + "classe H { entier32 X = objet + x; publique: constructeur(C& objet, entier32& x) { Absente; } };", 22},
            {mutation + "classe H { entier32 X = objet + x; publique: constructeur(C& objet, constante entier32& x) { Absente; } };", 21},
            {prive + "classe H { entier32 X = objet + x; publique: constructeur(C& objet, entier32& x) { Absente; } };", 25},
            {scalaire + "classe Base { publique: constructeur(entier32 x) {} }; classe H : publique Base { publique: "
             "constructeur(C& objet, entier32& x) : parent(objet + x) { Absente; } };", 22},
            {scalaire + "classe M { publique: constructeur(entier32 x) {} }; classe H { M m; publique: "
             "constructeur(C& objet, entier32& x) : m(objet + x) { Absente; } };", 22},
            {scalaire + "classe H { publique: constructeur(C& objet, entier32& x) : soi(objet + x) { Absente; } "
             "constructeur(entier32 x) {} };", 22},
            {scalaire + fonction("entier32&", "constante entier32 fixe = 0; fixe = objet + x;"), 71},
            {scalaire + fonction("entier32&", "entier32 tableau[1]; tableau = objet + x;"), 72},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
        {
            const auto source = "publique entier32 Trace = 0; " + refus[index].first;
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
                ComparerErreurSemantique(syntaxe, semantique, texte, refus[index].second,
                    "reference-operateur-mixte-construction-refuse-" + std::to_string(index));
        }
        std::cout << "Références des opérateurs mixtes et constructions : " << valides.size()
                  << " corpus bilingues exécutés, cibles, mutations et traces vérifiées.\n";
    }

    /**
     * <résumé>Vérifie les opérateurs dans les agrégats, leur stockage et la priorité entre éléments.</résumé>
     * Les traces distinguent la visite sémantique, l'exécution et la capture de chaque valeur.
     **/
    void TesterOperateursInitialiseursAgreges(
        AnalyseurDeclarationsAutoHeberge syntaxe, AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::string declarations = "publique entier32 Trace = 0; structure P { entier32 X; entier32 Y; }; "
            "structure Bloc { P Couple; entier32 Valeurs[2]; }; union U { entier32 X; entier64 Y; }; ";
        const std::string mutation = "classe C { publique: entier32 opérateur+(entier32& valeur) { "
            "Trace = Trace * 10 + valeur; valeur = valeur + 1; retourner valeur; } }; "
            "espace C { publique entier32 opérateur+(C& objet, booléen valeur) { Trace = 9; retourner 1; } } ";
        const std::string lecture = "classe C { publique: entier32 opérateur+(booléen valeur) { Trace = 9; retourner 1; } }; "
            "espace C { publique entier32 opérateur+(C& objet, constante entier32& valeur) { "
            "Trace = Trace * 10 + valeur; retourner valeur + 1; } } ";
        const std::string adresses = "structure Adresses { entier32* Mutable; constante entier32* Lecture; }; "
            "classe C { publique: entier32* Cible; entier32* opérateur+(entier32*& valeur) { "
            "Trace = Trace * 10 + *valeur; valeur = soi.Cible; retourner valeur; } }; "
            "espace C { publique entier32* opérateur+(C& objet, booléen valeur) { Trace = 9; retourner objet.Cible; } } ";
        const auto principal = [](std::string_view corps) {
            return "publique entier32 Principal() { " + std::string(corps) + " }";
        };
        const std::string depart = "C c; entier32 x = 1; entier32 y = 2; ";
        const std::string verifierP = "retourner 42 * convertir<entier32>(p.X == 2 && p.Y == 3 && x == 2 && y == 3);";
        struct Corpus {
            std::string Texte;
            std::int32_t Trace;
            std::size_t NombreOperateurs;
            bool Methode = true;
        };
        const std::vector<Corpus> valides{
            {mutation + principal(depart + "P p = {c + x, c + y}; " + verifierP), 12, 2},
            {mutation + principal("C c; entier32 x = 41; entier32 valeur = {{c + x}}; "
                "retourner 42 * convertir<entier32>(valeur == 42 && x == 42);"), 41, 1},
            {mutation + principal(depart + "P p = {{{c + x}}, {c + y}}; " + verifierP), 12, 2},
            {mutation + principal(depart + "entier32 valeurs[2] = {c + x, c + y}; "
                "retourner 42 * convertir<entier32>(valeurs[0] == 2 && valeurs[1] == 3 && x == 2 && y == 3);"), 12, 2},
            {mutation + principal(depart + "entier32 z = 3; entier32 w = 4; "
                "entier32 valeurs[2][2] = {{c + x, c + y}, {c + z, c + w}}; retourner 42 * convertir<entier32>("
                "valeurs[0][0] == 2 && valeurs[0][1] == 3 && valeurs[1][0] == 4 && valeurs[1][1] == 5 "
                "&& x == 2 && y == 3 && z == 4 && w == 5);"), 1234, 4},
            {mutation + principal(depart + "entier32 z = 3; entier32 w = 4; "
                "Bloc b = {{c + x, c + y}, {c + z, c + w}}; retourner 42 * convertir<entier32>("
                "b.Couple.X == 2 && b.Couple.Y == 3 && b.Valeurs[0] == 4 && b.Valeurs[1] == 5 "
                "&& x == 2 && y == 3 && z == 4 && w == 5);"), 1234, 4},
            {mutation + principal(depart + "entier32 z = 3; entier32 w = 4; "
                "P valeurs[2] = {{c + x, c + y}, {c + z, c + w}}; retourner 42 * convertir<entier32>("
                "valeurs[0].X == 2 && valeurs[0].Y == 3 && valeurs[1].X == 4 && valeurs[1].Y == 5 "
                "&& x == 2 && y == 3 && z == 4 && w == 5);"), 1234, 4},
            {mutation + principal("C c; entier32 x = 41; entier32 valeurs[3] = {c + x}; "
                "retourner 42 * convertir<entier32>(valeurs[0] == 42 && valeurs[1] == 0 && valeurs[2] == 0 && x == 42);"), 41, 1},
            {mutation + principal("C c; entier32 x = 41; P p = {c + x}; "
                "retourner 42 * convertir<entier32>(p.X == 42 && p.Y == 0 && x == 42);"), 41, 1},
            {mutation + principal("C c; entier32 x = 41; U u = {c + x}; "
                "retourner 42 * convertir<entier32>(u.X == 42 && x == 42);"), 41, 1},
            {mutation + "structure Alignee { naturel8 Petit; P Paire; entier64 Grand; }; "
             + principal(depart + "Alignee a = {7, {c + x, c + y}, 42}; retourner 42 * convertir<entier32>("
                "a.Petit == 7 && a.Paire.X == 2 && a.Paire.Y == 3 && a.Grand == 42 && x == 2 && y == 3);"), 12, 2},
            {mutation + principal(depart + "P p = {c + x, c + y}; P copie = p; copie.X = 0; "
                "retourner 42 * convertir<entier32>(p.X == 2 && p.Y == 3 && copie.X == 0 && copie.Y == 3 && x == 2 && y == 3);"), 12, 2},
            {mutation + principal(depart + "P p = {}; p = {c + x, c + y}; " + verifierP), 12, 2},
            {mutation + "entier32 Somme(P p) { retourner p.X + p.Y; } "
             + principal(depart + "entier32 valeur = Somme({c + x, c + y}); "
                "retourner 42 * convertir<entier32>(valeur == 5 && x == 2 && y == 3);"), 12, 2},
            {mutation + "entier32 Somme(P p) { retourner p.X + p.Y; } "
             + principal(depart + "pointeur_fonction<entier32(P)> rappel = Somme; entier32 valeur = rappel({c + x, c + y}); "
                "retourner 42 * convertir<entier32>(valeur == 5 && x == 2 && y == 3);"), 12, 2},
            {mutation + "P Produire(C& c, entier32& x, entier32& y) { retourner {c + x, c + y}; } "
             + principal(depart + "P p = Produire(c, x, y); " + verifierP), 12, 2},
            {mutation + "Bloc Produire(C& c, entier32& x, entier32& y) { retourner {{c + x, c + y}, {}}; } "
             + principal(depart + "Bloc b = Produire(c, x, y); retourner 42 * convertir<entier32>("
                "b.Couple.X == 2 && b.Couple.Y == 3 && b.Valeurs[0] == 0 && b.Valeurs[1] == 0 && x == 2 && y == 3);"), 12, 2},
            {mutation + "classe H { P p = {c + x, c + y}; publique: constructeur(C& c, entier32& x, entier32& y) {} "
             "entier32 Lire() { retourner soi.p.X + soi.p.Y; } }; "
             + principal(depart + "H h(c, x, y); retourner 42 * convertir<entier32>(h.Lire() == 5 && x == 2 && y == 3);"), 12, 2},
            {mutation + "classe H { P p; publique: constructeur(C& c, entier32& x, entier32& y) : p({c + x, c + y}) {} "
             "entier32 Lire() { retourner soi.p.X + soi.p.Y; } }; "
             + principal(depart + "H h(c, x, y); retourner 42 * convertir<entier32>(h.Lire() == 5 && x == 2 && y == 3);"), 12, 2},
            {mutation + "classe Base { publique: entier32 X; constructeur(P p) : X(p.X + p.Y) {} }; "
             "classe H : publique Base { publique: constructeur(C& c, entier32& x, entier32& y) : parent({c + x, c + y}) {} }; "
             + principal(depart + "H h(c, x, y); retourner 42 * convertir<entier32>(h.X == 5 && x == 2 && y == 3);"), 12, 2},
            {mutation + "classe M { publique: entier32 X; constructeur(P p) : X(p.X + p.Y) {} }; "
             "classe H { M m; publique: constructeur(C& c, entier32& x, entier32& y) : m({c + x, c + y}) {} "
             "entier32 Lire() { retourner soi.m.X; } }; "
             + principal(depart + "H h(c, x, y); retourner 42 * convertir<entier32>(h.Lire() == 5 && x == 2 && y == 3);"), 12, 2},
            {mutation + "classe H { entier32 X; publique: constructeur(C& c, entier32& x, entier32& y) : soi({c + x, c + y}) {} "
             "constructeur(P p) : X(p.X + p.Y) {} entier32 Lire() { retourner soi.X; } }; "
             + principal(depart + "H h(c, x, y); retourner 42 * convertir<entier32>(h.Lire() == 5 && x == 2 && y == 3);"), 12, 2},
            {lecture + principal("C c; constante entier32 x = 1; constante entier32 y = 2; P p = {c + x, c + y}; "
                "retourner 42 * convertir<entier32>(p.X == 2 && p.Y == 3 && x == 1 && y == 2);"), 12, 2, false},
            {mutation + "structure Etat { booléen Ignore; entier32 Valeur; }; "
             + principal("C c; entier32 x = 41; entier32 y = 41; Etat e = {faux && (c + x), c + y}; "
                "retourner 42 * convertir<entier32>(e.Ignore == faux && e.Valeur == 42 && x == 41 && y == 42);"), 41, 2},
            {mutation + principal(depart + "P p = {c + x, x}; "
                "retourner 42 * convertir<entier32>(p.X == 2 && p.Y == 2 && x == 2 && y == 2);"), 1, 1},
            {mutation + principal("C c; entier32 x = 40; P p = {c + x, c + x}; "
                "retourner 42 * convertir<entier32>(p.X == 41 && p.Y == 42 && x == 42);"), 441, 2},
            {adresses + principal("C c; entier32 x = 1; entier32 y = 3; entier32 cible = 42; c.Cible = &cible; "
                "entier32* a = &x; entier32* b = &y; Adresses p = {c + a, convertir<constante entier32*>(c + b)}; "
                "retourner 42 * convertir<entier32>(p.Mutable == &cible && p.Lecture == convertir<constante entier32*>(&cible) "
                "&& a == &cible && b == &cible && x == 1 && y == 3 && *p.Mutable == 42 && *p.Lecture == 42);"), 13, 2},
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
            VerifierOperateursReferencesBilingues(syntaxe, semantique, declarations + valides[index].Texte,
                valides[index].Methode, valides[index].Trace, valides[index].NombreOperateurs,
                "operateur-initialiseur-agrege-valide-" + std::to_string(index));
        const auto fonction = [](std::string_view corps) {
            return "publique vide F(C& objet, constante entier32& x) { " + std::string(corps) + " }";
        };
        const std::string prive = "classe C { privée: entier32 opérateur+(entier32& valeur) { retourner valeur; } }; "
            "espace C { publique entier32 opérateur+(C& objet, booléen valeur) { retourner 1; } } ";
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {mutation + fonction("P p = {objet + x, Absente};"), 21},
            {mutation + fonction("P p = {Absente, objet + x};"), 18},
            {mutation + fonction("P p = {vrai, objet + x};"), 45},
            {mutation + fonction("P p = {objet + x, vrai};"), 21},
            {mutation + fonction("P p = {objet + x, Absente, 0};"), 43},
            {mutation + fonction("entier32 valeurs[1] = {objet + x, Absente};"), 42},
            {mutation + fonction("entier32 valeurs[2] = {objet + x, vrai};"), 21},
            {mutation + fonction("entier32 valeurs[2] = {vrai, objet + x};"), 45},
            {mutation + fonction("Bloc b = {{objet + x, 0}, {Absente, 0, 0}};"), 21},
            {mutation + fonction("Bloc b = {{0, 0}, {Absente, 0, 0}};"), 42},
            {mutation + fonction("Bloc b = {{vrai, objet + x}, {Absente, 0, 0}};"), 45},
            {mutation + fonction("U u = {objet + x, Absente};"), 43},
            {mutation + fonction("entier32 valeur = {{objet + x, 0}};"), 44},
            {mutation + fonction("P& p = {objet + x, Absente};"), 69},
            {mutation + fonction("constante P p = {0, 0}; p = {objet + x, Absente};"), 71},
            {mutation + fonction("P p = {}; p = {objet + x, Absente, 0};"), 43},
            {mutation + fonction("naturel8 valeurs[2] = {300, objet + x};"), 90},
            {mutation + fonction("naturel8 valeurs[2] = {objet + x, 300};"), 21},
            {mutation + "classe H { P p = {objet + x, vrai}; publique: constructeur(C& objet, constante entier32& x) { Absente; } };", 21},
            {mutation + "classe H { P p = {vrai, objet + x}; publique: constructeur(C& objet, constante entier32& x) { Absente; } };", 45},
            {prive + "classe H { P p = {objet + x, Absente}; publique: constructeur(C& objet, entier32& x) {} };", 25},
            {mutation + "classe H { P p; publique: constructeur(C& objet, constante entier32& x) : p({objet + x, Absente}) { Absente; } };", 21},
            {mutation + "classe Base { publique: constructeur(P p) {} }; classe H : publique Base { publique: "
             "constructeur(C& objet, constante entier32& x) : parent({objet + x, vrai}) { Absente; } };", 21},
            {mutation + "classe M { publique: constructeur(P p) {} }; classe H { M m; publique: "
             "constructeur(C& objet, constante entier32& x) : m({objet + x, vrai}) { Absente; } };", 21},
            {mutation + "classe H { publique: constructeur(C& objet, constante entier32& x) : soi({objet + x, vrai}) { Absente; } "
             "constructeur(P p) {} };", 21},
            {mutation + "vide Somme(P p) {} vide Somme(entier32 p) {} "
             + fonction("Somme({objet + x, Absente});"), 22},
            {mutation + "vide Somme(P p) {} " + fonction("pointeur_fonction<vide(P)> rappel = Somme; rappel({vrai, objet + x});"), 45},
            {mutation + "vide Somme(P p) {} " + fonction("pointeur_fonction<vide(P)> rappel = Somme; rappel({objet + x, Absente}, 0);"), 54},
            {mutation + "publique P Produire(C& objet, constante entier32& x) { retourner {objet + x, vrai}; }", 21},
            {mutation + fonction("P p = {}; p = {vrai, objet + x};"), 45},
            {mutation + fonction("Bloc b = {}; b = {{0, 0}, {objet + x, Absente, 0}};"), 42},
            {mutation + fonction("U u = {}; u = {objet + x, Absente};"), 43},
            {mutation + fonction("entier32 valeur = 0; valeur = {objet + x, Absente};"), 44},
            {mutation + fonction("entier32 valeur = 0; valeur = {vrai};"), 45},
            {mutation + fonction("P p = {}; P* adresse = &p; *adresse = {objet + x, Absente, 0};"), 43},
            {mutation + "structure Contenant { P Paire; }; " + fonction("Contenant c = {}; c.Paire = {objet + x, Absente, 0};"), 43},
            {mutation + "publique P Produire(C& objet, constante entier32& x) { retourner {objet + x, Absente, 0}; }", 43},
            {mutation + "publique P Produire(C& objet, constante entier32& x) { retourner {vrai, objet + x}; }", 45},
            {mutation + "publique entier32 Produire(C& objet, constante entier32& x) { retourner {objet + x, Absente}; }", 44},
            {mutation + "publique entier32 Produire(C& objet, constante entier32& x) { retourner {vrai}; }", 45},
            {mutation + "publique U Produire(C& objet, constante entier32& x) { retourner {objet + x, Absente}; }", 43},
            {mutation + "publique Bloc Produire(C& objet, constante entier32& x) { retourner {{0, 0}, {objet + x, Absente, 0}}; }", 42},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
        {
            const auto source = declarations + refus[index].first;
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
                ComparerErreurSemantique(syntaxe, semantique, texte, refus[index].second,
                    "operateur-initialiseur-agrege-refuse-" + std::to_string(index));
        }
        std::cout << "Opérateurs des initialiseurs agrégés : " << valides.size()
                  << " corpus bilingues exécutés, stockage, mutations et ordre des éléments vérifiés.\n";
    }

    void TesterPrioritesPlansConstructeursSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe, AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::vector<std::string> valides{
            "classe M { publique: entier32 X; constructeur() {} }; classe B { M m; }; classe D : publique B { M n; entier32 X = 42; publique: constructeur() {} };",
            "classe B {}; classe D : publique B { publique: constructeur() : parent() {} };",
            "classe M { publique: constructeur() {} }; classe B { M m; }; classe D : publique B { publique: constructeur() : parent() {} };",
            "classe M { publique: entier32 X; constructeur() {} }; classe Interne { M m; }; classe C { Interne i[2]; entier32 X = 42; publique: constructeur() : i() {} };",
            "structure P { entier32 X; }; classe M { publique: constructeur(P p) {} }; classe C { M m; publique: constructeur() : m({42}) {} };",
            "structure P { entier32 X; }; classe B { publique: constructeur(P p) {} }; classe D : publique B { publique: constructeur() : parent({42}) {} };",
            "structure P { entier32 X; }; classe C { publique: constructeur() : soi({42}) {} constructeur(P p) {} };",
            "classe B { publique: virtuel entier32 Lire() { retourner 1; } }; classe D : publique B { publique: constructeur() : parent() {} remplacer entier32 Lire() { retourner 42; } };",
            "classe M { publique: entier32 X; constructeur() {} constructeur(entier32 x) {} }; classe C { M a; M b; publique: constructeur() : b(42) {} };",
            "classe M { publique: constructeur(entier32 x) {} }; classe C { M a; alias Vue = a; publique: constructeur() : Vue(42) {} };",
            "classe M { privée: constructeur() {} }; classe C { M* m; entier32 X = 42; publique: constructeur() {} };",
            "classe M { protégée: constructeur() {} }; classe B : publique M {}; classe D : publique B { publique: constructeur() : parent() {} };",
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
            for (const auto& texte : {valides[index], TraduireCorpusConversions(valides[index])})
            {
                const auto resultat = AnalyserSemantiqueValide(syntaxe, semantique, texte,
                    "priorite-plan-constructeur-valide-" + std::to_string(index));
                if (index == 0 || index == 1 || index == 2 || index == 3 || index == 7 || index == 8 || index == 10)
                {
                    const auto nom = index <= 2 || index == 7 ? "D" : "C";
                    const auto classe = std::find_if(resultat.Noeuds.begin(), resultat.Noeuds.end(),
                        [&](const auto& noeud) { return noeud.Genre == 6 && noeud.HachageNom == HacherTexte(nom); });
                    Exiger(classe != resultat.Noeuds.end(), "classe du plan contextuel introuvable");
                    const auto indexClasse = static_cast<std::uint64_t>(classe - resultat.Noeuds.begin());
                    const auto construction = std::find_if(resultat.Noeuds.begin(), resultat.Noeuds.end(),
                        [&](const auto& noeud) { return noeud.Genre == 13 && noeud.Parent == indexClasse; });
                    Exiger(construction != resultat.Noeuds.end(), "constructeur du plan contextuel introuvable");
                    const auto indexConstruction = static_cast<std::uint64_t>(construction - resultat.Noeuds.begin());
                    std::vector<const ResolutionSemantiqueHote*> plans;
                    for (const auto& resolution : resultat.Resolutions)
                        if (resolution.IndexNoeud == indexConstruction && (resolution.Drapeaux & 32768U) != 0)
                            plans.push_back(&resolution);
                    const auto nombreAttendu = index == 1 || index == 10 ? 0U : index == 2 ? 1U : 2U;
                    Exiger(plans.size() == nombreAttendu,
                        "le contrôle préalable publie des étapes, ou le plan final est incomplet : " + texte);
                    if (index == 7)
                        Exiger(std::all_of(plans.begin(), plans.end(),
                                [](const auto* plan) { return (plan->Drapeaux & 262144U) != 0; }),
                            "une base sans constructeur doit produire ses étapes de table virtuelle, pas une cible fictive");
                    if (index == 0 || index == 3 || index == 8)
                        Exiger(plans[0]->HachageType == 0 && plans[1]->HachageType == 4,
                            "l'ordre canonique des bases/champs a changé");
                    if (index == 8)
                    {
                        const auto parametres = [&](const auto* plan)
                        {
                            const auto cible = resultat.Symboles[plan->IndexSymbole].IndexNoeud;
                            return std::count_if(resultat.Noeuds.begin(), resultat.Noeuds.end(),
                                [&](const auto& noeud) { return noeud.Genre == 2 && noeud.Parent == cible; });
                        };
                        Exiger(parametres(plans[0]) == 0 && parametres(plans[1]) == 1,
                            "le plan de champ ne réutilise pas le constructeur effectivement sélectionné");
                    }
                }
            }
        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"classe M { privée: constructeur() {} }; classe B { M m; }; classe D : publique B { entier32 X = Absente; publique: constructeur() {} };", 26},
            {"classe M { privée: constructeur() {} }; classe B { M m; }; classe D : publique B { entier32 X; publique: constructeur() : Inconnu(Absente) {} };", 26},
            {"classe M { privée: constructeur() {} }; classe B { M m; }; classe D : publique B { entier32 X; publique: constructeur() : X(convertir<vide>(0)) {} };", 26},
            {"classe M { privée: constructeur() {} }; classe B { M m; }; classe D : publique B { entier32 X = Absente; publique: constructeur() : parent() {} };", 26},
            {"classe B {}; classe D : publique B { publique: constructeur() : parent(Absente) {} };", 27},
            {"classe B { publique: constructeur(entier32 x) {} }; classe D : publique B { publique: constructeur() : parent(Absente, 1) {} };", 21},
            {"classe B { publique: constructeur(entier32 x, entier32 y) {} }; classe D : publique B { publique: constructeur() : parent(vrai, Absente) {} };", 21},
            {"structure P { entier32 X; }; classe B { privée: constructeur(P p) {} }; classe D : publique B { publique: constructeur() : parent({Absente, 2}) {} };", 26},
            {"structure P { entier32 X; }; classe B { publique: constructeur(P p) {} }; classe D : publique B { publique: constructeur() : parent({Absente, 2}) {} };", 43},
            {"classe M { privée: constructeur() {} }; classe Interne { M m; }; classe C { Interne i; entier32 X = Absente; publique: constructeur() {} };", 26},
            {"classe M { privée: constructeur() {} }; classe Interne { M m; }; classe C { entier32 X = Absente; Interne i; publique: constructeur() {} };", 18},
            {"classe M { privée: constructeur() {} }; classe Interne { M m; }; classe C { Interne i[2]; entier32 X = Absente; publique: constructeur() {} };", 26},
            {"classe M { privée: constructeur() {} }; classe Interne { M m; }; classe C { Interne i; entier32 X; publique: constructeur() : i(), X(Absente) {} };", 26},
            {"classe M { privée: constructeur() {} }; classe Interne { M m; }; classe C { Interne i[2]; entier32 X; publique: constructeur() : i(), X(Absente) {} };", 26},
            {"classe M { privée: constructeur() {} }; classe Interne { M m; }; classe C { Interne i; entier32 X = Absente; publique: constructeur() : i() {} };", 26},
            {"classe M { publique: constructeur(entier32 x) {} }; classe C { M m; entier32 X; publique: constructeur() : m(Absente, 1), X(0) {} };", 21},
            {"classe M { publique: constructeur(entier32 x, entier32 y) {} }; classe C { M m; publique: constructeur() : m(vrai, Absente) {} };", 21},
            {"structure P { entier32 X; }; classe M { privée: constructeur(P p) {} }; classe C { M m; publique: constructeur() : m({Absente, 2}) {} };", 26},
            {"structure P { entier32 X; }; classe M { publique: constructeur(P p) {} }; classe C { M m; publique: constructeur() : m({Absente, 2}) {} };", 43},
            {"classe M { privée: constructeur() {} }; classe Interne { M m; }; classe C { Interne i; constante entier32 X; publique: constructeur() {} };", 26},
            {"classe M { privée: constructeur() {} }; classe Interne { M m; }; classe C { constante entier32 X; Interne i; publique: constructeur() {} };", 40},
            {"classe M { privée: constructeur() {} }; classe B { M m; }; classe D : publique B { publique: constructeur() {} }; publique vide F() { Absente; }", 26},
            {"classe M { privée: constructeur() {} }; classe B { M m; }; publique vide F() { Absente; } classe D : publique B { publique: constructeur() {} };", 18},
            {"classe C { publique: constructeur() : soi(Absente, 1) {} constructeur(entier32 x) {} };", 21},
            {"classe C { publique: constructeur() : parent(Absente) {} };", 30},
            {"structure P { entier32 X; }; classe B { publique: constructeur(P p) {} constructeur(entier32 x) {} }; classe D : publique B { publique: constructeur() : parent({Absente}) {} };", 22},
            {"structure P { entier32 X; }; classe M { publique: constructeur(P p) {} constructeur(entier32 x) {} }; classe C { M m; publique: constructeur() : m({Absente}) {} };", 22},
            {"structure P { entier32 X; }; classe C { publique: constructeur(P p) : soi({Absente, 2}) {} };", 43},
            {"classe M { privée: constructeur() {} }; classe Interne { M m; }; classe C { Interne i; publique: constructeur() : i(Absente) {} };", 27},
            {"classe M { privée: constructeur() {} }; classe Interne { M m; }; classe C { Interne i; entier32 X[1] = {Absente, 2}; publique: constructeur() {} };", 26},
            {"classe M { privée: constructeur() {} }; classe Interne { M m; }; classe C { entier32 X[1] = {Absente, 2}; Interne i; publique: constructeur() {} };", 42},
            {"structure P { entier32 X; }; classe B { publique: constructeur(P p) {} }; classe D : publique B { publique: constructeur() : parent({Absente, 2}), Inconnu(0) {} };", 43},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
            for (const auto& texte : {refus[index].first, TraduireCorpusConversions(refus[index].first)})
                ComparerErreurSemantique(syntaxe, semantique, texte, refus[index].second,
                    "priorite-plan-constructeur-refuse-" + std::to_string(index));
    }

    void TesterConstructionsLocalesContextuellesSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe, AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::vector<std::string> valides{
            "classe C { publique: constructeur() {} destructeur() {} }; publique vide F() { C a; C b(); }",
            "classe C { publique: constructeur(entier32 x) {} destructeur() {} }; publique vide F() { C a(42); C b[2](42); }",
            "classe C {}; publique vide F() { C a; C b[2]; C c[2](); }",
            "structure P { entier32 X; }; classe C { publique: constructeur(P p) {} }; publique vide F() { C a({42}); }",
            "classe C { publique: constructeur(entier32& x) {} }; publique vide F() { entier32 x = 42; C a(x); }",
            "classe B { publique: constructeur() {} destructeur() {} }; classe D : publique B {}; publique vide F() { D a; D b[2]; }",
            "classe M { publique: constructeur() {} destructeur() {} }; classe C { M m; }; publique vide F() { C a; C b[2]; }",
            "classe C { privée: constructeur() {} destructeur() {} publique: vide F() { C a; } };",
            "classe C { publique: constructeur() {} destructeur() {} }; publique vide F(booléen choix) { si (choix) { C a; } sinon { C b; } tantque (choix) { C c; choix = faux; } }",
            "classe C { publique: constructeur() {} constructeur(entier32 x) {} }; publique vide F() { C a(42); C b; }",
            "classe C { publique: constructeur() {} destructeur() {} }; publique vide F() { C a[2][2]; }",
            "classe C { publique: constructeur() {} }; alias Objet = C; publique vide F() { Objet a; }",
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
            for (const auto& texte : {valides[index], TraduireCorpusConversions(valides[index])})
            {
                const auto resultat = AnalyserSemantiqueValide(syntaxe, semantique, texte,
                    "construction-locale-contextuelle-valide-" + std::to_string(index));
                if (index == 0 || index == 1)
                    for (std::uint64_t variable = 0; variable < resultat.Noeuds.size(); ++variable)
                        if (resultat.Noeuds[variable].Genre == 19)
                        {
                            std::size_t selections = 0;
                            std::vector<std::uint32_t> etapes;
                            for (const auto& resolution : resultat.Resolutions)
                                if (resolution.IndexNoeud == variable)
                                {
                                    if ((resolution.Drapeaux & 64) != 0
                                        && (resolution.Drapeaux & 32768) == 0) ++selections;
                                    if ((resolution.Drapeaux & 32768) != 0) etapes.push_back(resolution.Drapeaux);
                                }
                            const auto nombreElements = index == 1
                                && resultat.Noeuds[variable].HachageNom == HacherTexte("b") ? 2U : 1U;
                            Exiger(selections == 1 && etapes.size() == 2 * nombreElements,
                                "la construction locale est sélectionnée ou planifiée plusieurs fois");
                            for (std::size_t etape = 0; etape < etapes.size(); ++etape)
                                Exiger((etapes[etape] & (etape < nombreElements ? 65536U : 131072U)) != 0,
                                    "la destruction locale doit suivre toutes les étapes de construction");
                        }
            }

        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"classe C { publique: constructeur(entier32 x) {} }; publique vide F() { C a; Absente; }", 21},
            {"classe C { publique: constructeur(entier32 x) {} }; publique vide F() { Absente; C a; }", 18},
            {"classe C { privée: constructeur() {} }; publique vide F() { C a; convertir<vide>(0); }", 26},
            {"classe C {}; publique vide F() { C a(); Absente; }", 27},
            {"classe C { publique: constructeur() {} privée: destructeur() {} }; publique vide F() { C a; Absente; }", 56},
            {"classe C { publique: constructeur(entier32 x) {} }; publique vide F() { C a; } publique vide G() { Absente; }", 21},
            {"classe C { publique: constructeur(entier32 x) {} }; publique vide G() { Absente; } publique vide F() { C a; }", 18},
            {"classe C { publique: constructeur(entier32 x) {} }; publique vide F() { { C a; } Absente; }", 21},
            {"classe C { publique: constructeur(entier32 x) {} }; publique vide F() { si (vrai) { C a; } sinon { Absente; } }", 21},
            {"classe C { publique: constructeur(entier32 x) {} }; publique vide F() { tantque (faux) { C a; } Absente; }", 21},
            {"classe C { publique: constructeur(entier32 x) {} }; publique vide F() { C a(Absente, 1); }", 21},
            {"classe C { publique: constructeur(entier32 x, entier32 y) {} }; publique vide F() { C a(vrai, Absente); }", 21},
            {"classe C { privée: constructeur(entier32 x) {} }; publique vide F() { C a(Absente); }", 18},
            {"classe C { publique: constructeur(entier32 x) {} }; publique vide F() { C a(Absente); }", 18},
            {"classe C { publique: constructeur(entier32 x) {} }; publique vide F() { C a(vrai); Absente; }", 21},
            {"classe C { publique: constructeur(entier8 x) {} constructeur(entier16 x) {} }; publique vide F() { C a(1); Absente; }", 22},
            {"classe C { publique: constructeur(entier32 x) {} }; publique vide F() { C a = Absente; }", 29},
            {"classe C { privée: constructeur() {} }; publique vide F() { C a[2]; Absente; }", 26},
            {"classe C { publique: constructeur() {} privée: destructeur() {} }; publique vide F() { C a[2]; Absente; }", 56},
            {"classe B { privée: constructeur() {} }; classe D : publique B {}; publique vide F() { D a(); Absente; }", 26},
            {"classe M { privée: constructeur() {} }; classe C { M m; }; publique vide F() { C a; Absente; }", 26},
            {"classe M { publique: constructeur() {} privée: destructeur() {} }; classe C { M m; }; publique vide F() { C a; Absente; }", 56},
            {"classe B { publique: constructeur() {} privée: destructeur() {} }; classe D : publique B {}; publique vide F() { D a; Absente; }", 56},
            {"structure P { entier32 X; }; classe C { publique: constructeur(P p) {} }; publique vide F() { C a({Absente, 2}); }", 43},
            {"structure P { entier32 X; }; classe C { privée: constructeur(P p) {} }; publique vide F() { C a({Absente, 2}); }", 26},
            {"structure P { entier32 X; }; classe C { publique: constructeur(P p) {} constructeur(entier32 x) {} }; publique vide F() { C a({Absente}); }", 22},
            {"classe C { publique: constructeur(entier32& x) {} }; publique vide F() { C a(42); Absente; }", 21},
            {"classe C { publique: constructeur(entier32 x) {} }; classe D { publique: constructeur() { C a; Absente; } };", 21},
            {"classe C {}; publique vide F() { C a(Absente); }", 27},
            {"classe C {}; publique vide F() { C a[2](Absente); }", 27},
            {"classe A { publique: constructeur() {} privée: destructeur() {} }; classe B { privée: constructeur() {} }; publique vide F() { A a; B b; }", 56},
            {"classe A { publique: constructeur() {} privée: destructeur() {} }; classe B { privée: constructeur() {} }; publique vide F() { B b; A a; }", 26},
            {"classe M { privée: constructeur() {} }; classe B { M m; }; classe D : publique B { publique: constructeur() { Absente; } };", 26},
            {"classe M { privée: constructeur() {} }; classe Interne { M m; }; classe C { Interne i; publique: constructeur() { Absente; } };", 26},
            {"classe M { privée: constructeur() {} }; classe Interne { M m; }; classe C { Interne i; publique: constructeur() : i() { Absente; } };", 26},
            {"classe C { publique: constructeur(entier32 x) {} }; alias Objet = C; publique vide F() { Objet a; Absente; }", 21},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
            for (const auto& texte : {refus[index].first, TraduireCorpusConversions(refus[index].first)})
                ComparerErreurSemantique(syntaxe, semantique, texte, refus[index].second,
                    "construction-locale-contextuelle-refuse-" + std::to_string(index));
    }

    void TesterChampsParDefautContextuelsSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe, AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::vector<std::string> valides{
            "classe C { entier32 X = valeur; publique: constructeur(entier32 valeur) {} };",
            "classe C { entier32 X = Absente; publique: constructeur() : X(42) {} };",
            "classe C { entier32 X = Absente; publique: constructeur() : soi(42) {} constructeur(entier32 valeur) : X(valeur) {} };",
            "classe C { entier32 X = valeur; publique: constructeur(entier32 valeur) {} constructeur() : X(42) {} };",
            "classe C { entier32 X = valeur; publique: constructeur(entier32 valeur) {} constructeur(entier32 valeur, entier32 autre) {} };",
            "classe C { entier32 X = Absente; publique: constructeur() : soi(42) {} constructeur(entier32 valeur) : X(valeur) {} constructeur(entier32 valeur, entier32 autre) : X(autre) {} };",
            "classe C { entier32 X = 1; entier32 Y = soi.X + 1; publique: constructeur() {} };",
            "classe C { entier32 X = soi.Lire(); publique: constructeur() {} entier32 Lire() { retourner 42; } };",
            "classe C { entier32 X = Absente; alias Vue = X; publique: constructeur() : Vue(42) {} };",
            "classe C { entier32 X[2] = {valeur, valeur + 1}; publique: constructeur(entier32 valeur) {} };",
            "classe C { entier32 X[2] = {Absente, 2, 3}; publique: constructeur() : X({1, 2}) {} };",
            "structure P { entier32 X; }; classe C { P V = {valeur}; publique: constructeur(entier32 valeur) {} };",
            "classe B { protégée: entier32 X; }; classe C : publique B { entier32 Y = parent.X; publique: constructeur() {} };",
            "classe C { entier32 X = 1; pointeur_fonction<entier32(entier32)> F = fonction; publique: constructeur(pointeur_fonction<entier32(entier32)> fonction) {} };",
            "classe C { naturel8 X = convertir<naturel8>(valeur); publique: constructeur(entier32 valeur) {} };",
            "publique entier32 Lire(entier32 x) { retourner x; } publique entier32 Lire(entier64 x) { retourner 42; } classe C { entier32 X = Lire(valeur); publique: constructeur(entier32 valeur) {} constructeur(entier64 valeur) {} };",
            "classe B {}; classe D : publique B {}; classe C { constante B* X = valeur; publique: constructeur(D* valeur) {} };",
            "classe C { constante entier32* X = convertir<constante entier32*>(valeur); publique: constructeur(entier32* valeur) {} };",
            "classe C { entier32 X = valeur; publique: constructeur(entier32& valeur) {} };",
            "espace N { structure P { entier32 X; }; } utilisant espace N; classe C { P X = {valeur}; publique: constructeur(entier32 valeur) {} };",
            "publique entier32 Lire() { retourner 42; } classe C { pointeur_fonction<entier32()> X = fonction(); publique: constructeur(pointeur_fonction<pointeur_fonction<entier32()>()> fonction) {} };",
            "classe C { entier32 X = Absente; entier32 Y = 7; publique: constructeur() : X(42) {} };",
            "classe C { entier32 X = valeur; entier32 Y = Absente; publique: constructeur(entier32 valeur) : Y(1) {} };",
            "classe C { entier32 X = soi.Lire(); publique: constructeur() {} constructeur(entier32 valeur) {} entier32 Lire() { retourner 42; } };",
            "classe C { entier32 X = valeur; publique: constructeur(entier32 valeur) {} constructeur(entier32& valeur) {} };",
            "publique entier32 Lire(entier32 x) { retourner x; } publique entier32 Lire(entier64 x) { retourner 42; } classe C { entier32 X[2] = {Lire(valeur), convertir<entier32>(valeur)}; publique: constructeur(entier32 valeur) {} constructeur(entier64 valeur) {} };",
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
            for (const auto& texte : {valides[index], TraduireCorpusConversions(valides[index])})
            {
                const auto resultat = AnalyserSemantiqueValide(syntaxe, semantique, texte,
                    "champ-defaut-contextuel-valide-" + std::to_string(index));
                if (index == 4 || index == 15)
                {
                    std::vector<std::uint64_t> constructeursParametres;
                    std::vector<std::uint64_t> fonctionsChoisies;
                    for (const auto& resolution : resultat.Resolutions)
                    {
                        const auto& reference = resultat.Noeuds[resolution.IndexNoeud];
                        const auto& cible = resultat.Symboles[resolution.IndexSymbole];
                        if (reference.Genre == 24 && reference.HachageNom == HacherTexte("valeur"))
                        {
                            Exiger(cible.Genre == 8, "le champ par défaut doit référencer un paramètre");
                            constructeursParametres.push_back(resultat.Noeuds[cible.IndexNoeud].Parent);
                        }
                        if (reference.Genre == 24 && reference.HachageNom == HacherTexte("Lire") && cible.Genre == 2)
                            fonctionsChoisies.push_back(cible.IndexNoeud);
                    }
                    Exiger(constructeursParametres.size() == 2
                            && constructeursParametres[0] != constructeursParametres[1],
                        "le champ par défaut réutilise le paramètre d'un autre constructeur");
                    if (index == 15)
                        Exiger(fonctionsChoisies.size() == 2 && fonctionsChoisies[0] != fonctionsChoisies[1],
                            "les surcharges du champ par défaut ne suivent pas le type du paramètre : nombre="
                                + std::to_string(fonctionsChoisies.size())
                                + ", première=" + (fonctionsChoisies.empty() ? "absente" : std::to_string(fonctionsChoisies[0]))
                                + ", dernière=" + (fonctionsChoisies.empty() ? "absente" : std::to_string(fonctionsChoisies.back())));
                }
            }

        const std::vector<std::pair<std::string, std::uint32_t>> refus{
            {"classe C { entier32 X = valeur; publique: constructeur(entier32 valeur) {} constructeur() {} };", 18},
            {"classe C { entier32 X = valeur; publique: constructeur() {} constructeur(entier32 valeur) {} };", 18},
            {"classe C { entier32 X = valeur; publique: constructeur(entier32 valeur) {} constructeur(booléen valeur) {} };", 37},
            {"classe C { entier32 X = valeur; publique: constructeur(entier32 autre) { entier32 valeur = 42; } };", 18},
            {"classe C { entier32 X = Absente; publique: constructeur() { entier32 y = convertir<vide>(0); } };", 18},
            {"classe C { entier32 X = Absente; publique: constructeur() : X(convertir<vide>(0)) {} };", 94},
            {"classe C { entier32 X = Absente; publique: constructeur() : Manquant(Absente) {} };", 33},
            {"classe C { entier32 X; publique: constructeur() : X(1), X(Absente) {} };", 34},
            {"classe C { entier32 X; entier32 Y; publique: constructeur() : Y(1), X(Absente) {} };", 35},
            {"classe C { entier32 X; publique: constructeur() : X(Absente, 1) {} };", 36},
            {"classe C { entier32 X[1] = {Absente, 2}; publique: constructeur() {} };", 42},
            {"classe C { entier32 X[1]; publique: constructeur() : X({Absente, 2}) {} };", 42},
            {"classe C { naturel8 X = 256; publique: constructeur() {} };", 90},
            {"classe C { entier32 X = convertir<vide>(Absente); publique: constructeur() {} };", 94},
            {"classe C { entier32 X = valeur; publique: constructeur(entier32 valeur) {} constructeur(entier32 autre, booléen valeur) {} };", 37},
            {"classe C { entier32 X = Absente; publique: constructeur() : soi(1) {} constructeur(entier32 valeur) {} };", 18},
            {"classe C { entier32 X = Absente; entier32 Y; publique: constructeur() : Y(convertir<vide>(0)) {} };", 94},
            {"classe C { entier32 X = convertir<vide>(0); entier32 Y; publique: constructeur() : Y(Absente) {} };", 18},
            {"classe C { entier32 X = convertir<vide>(0); publique: constructeur() {} }; publique vide F() { Absente; }", 94},
            {"publique vide F() { Absente; } classe C { entier32 X = convertir<vide>(0); publique: constructeur() {} };", 18},
            {"classe C { entier32 X = convertir<vide>(0); publique: constructeur() : X(1) { Absente; } constructeur(entier32 valeur) {} };", 18},
            {"classe C { entier32 X = valeur; publique: constructeur(entier32 valeur) { Absente; } constructeur(booléen valeur) {} };", 18},
            {"classe C { entier32 X = Absente; publique: constructeur() : soi(convertir<vide>(0)) {} constructeur(entier32 valeur) : X(valeur) {} };", 94},
            {"classe C { entier32 X = convertir<naturel8>(256); publique: constructeur() {} };", 98},
            {"classe C { entier32 X[1] = {convertir<vide>(0)}; publique: constructeur() {} };", 94},
            {"structure P { entier32 X; }; classe C { P X = {Absente, 2}; publique: constructeur() {} };", 43},
            {"classe C { pointeur_fonction<entier32()> X = fonction; publique: constructeur(pointeur_fonction<vide()> fonction) {} };", 37},
            {"classe C { entier32 X = valeur; publique: constructeur(entier32* valeur) {} };", 37},
        };
        for (std::size_t index = 0; index < refus.size(); ++index)
            for (const auto& texte : {refus[index].first, TraduireCorpusConversions(refus[index].first)})
                ComparerErreurSemantique(syntaxe, semantique, texte, refus[index].second,
                    "champ-defaut-contextuel-refuse-" + std::to_string(index));

        for (const auto& [source, code] : std::vector<std::pair<std::string, std::uint32_t>>{
                 {"classe C { entier32 X = valeur; publique: constructeur(entier32 valeur); constructeur(entier32 autre, entier32 valeur) {} };", 0},
                 {"classe C { entier32 X; publique: constructeur(); };", 0},
                 {"classe C { entier32 X = Absente; publique: constructeur(); };", 39}})
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
            {
                auto jetons = GsPP::Lexeur(texte, "contexte-interface.HGsPP").Analyser();
                bool prototype = false;
                for (auto& jeton : jetons)
                {
                    if (jeton.Genre == GsPP::GenreJeton::Constructeur) prototype = true;
                    if (prototype) jeton.EstInterface = true;
                    if (prototype && jeton.Genre == GsPP::GenreJeton::PointVirgule) break;
                }
                auto programme = GsPP::AnalyseurSyntaxique(std::move(jetons), "contexte-interface.HGsPP").Analyser();
                auto noeuds = ConstruireDeclarationsReference(programme);
                const auto jetonsOrigine = GsPP::Lexeur(texte).Analyser();
                for (auto& noeud : noeuds)
                {
                    if (noeud.Genre == 13)
                    {
                        if ((noeud.Drapeaux & 4) == 0) noeud.Drapeaux |= 2;
                        noeud.DebutNom = noeuds[noeud.Parent].DebutNom;
                        noeud.TailleNom = noeuds[noeud.Parent].TailleNom;
                    }
                    else if (noeud.HachageNom != 0)
                        for (const auto& jeton : jetonsOrigine)
                            if (jeton.Ligne == noeud.Ligne && jeton.Colonne >= noeud.Colonne
                                && HacherTexte(jeton.Texte) == noeud.HachageNom)
                            {
                                noeud.DebutNom = jeton.Colonne - 1;
                                noeud.TailleNom = jeton.Texte.size();
                                break;
                            }
                }
                const auto avant = noeuds;
                std::uint32_t ligne = 0;
                std::uint32_t colonne = 0;
                try { GsPP::AnalyseurSemantique().Analyser(programme); }
                catch (const GsPP::ErreurCompilation& erreur)
                {
                    ligne = static_cast<std::uint32_t>(erreur.Ligne());
                    colonne = static_cast<std::uint32_t>(erreur.Colonne());
                }
                Exiger((code == 0) == (ligne == 0), "contrat bootstrap incorrect pour le prototype de constructeur");
                RequeteAnalyseSemantiqueHote requete{
                    texte.data(), texte.size(), noeuds.data(), noeuds.size(), nullptr, 0, nullptr, 0, {}};
                const auto mesure = semantique(&requete);
                if (code == 0)
                {
                    Exiger(mesure == 4, "le prototype de constructeur n'est pas ignoré pendant l'analyse : code="
                        + std::to_string(mesure) + ", détail=" + std::to_string(requete.Resultat.Detail)
                        + ", ligne=" + std::to_string(requete.Resultat.LigneErreur)
                        + ", colonne=" + std::to_string(requete.Resultat.ColonneErreur) + ", source=" + texte);
                    std::vector<SymboleSemantiqueHote> symboles(requete.Resultat.NombreSymboles);
                    std::vector<ResolutionSemantiqueHote> resolutions(requete.Resultat.NombreResolutions);
                    requete.Symboles = symboles.data();
                    requete.CapaciteSymboles = symboles.size();
                    requete.Resolutions = resolutions.data();
                    requete.CapaciteResolutions = resolutions.size();
                    Exiger(semantique(&requete) == 0, "le corpus avec prototype n'est pas analysé");
                    for (const auto& resolution : resolutions)
                        Exiger(!(noeuds[resolution.IndexNoeud].Genre == 13
                                && (noeuds[resolution.IndexNoeud].Drapeaux & 2) != 0),
                            "un prototype de constructeur ne doit pas produire un plan de corps");
                }
                else
                {
                    Exiger(mesure == code && requete.Resultat.LigneErreur == ligne
                            && requete.Resultat.ColonneErreur == colonne,
                        "le prototype seul ne satisfait pas l'exigence de constructeur défini");
                    ++NombreRefusSemantiquesDifferentiels;
                }
                Exiger(std::memcmp(noeuds.data(), avant.data(), noeuds.size() * sizeof(noeuds[0])) == 0,
                    "l'AST public de l'interface a été modifié");
            }
    }

    /**
     * <résumé>Analyse réellement les interfaces en Gs++ sans reconstruire leur AST côté hôte.</résumé>
     * Les textes sont préparés : la lecture et l'expansion des inclusions restent hors de cette API.
     **/
    void TesterInterfacesEnMemoire(
        AnalyseurDeclarationsAutoHeberge source, AnalyseurDeclarationsAutoHeberge interface,
        AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::vector<std::string> valides{
            "entier32 Lire();",
            "publique entier32 Lire(entier32 valeur); publique entier32 Globale;",
            "externe vide Journaliser(constante caractère* texte); constante entier32 Limite;",
            "entier32 Lire(entier32 valeur); entier64 Lire(entier64 valeur);",
            "structure P { entier32 X; entier32 Y; }; P Lire(P p); P Globale;",
            "union U { entier32 X; entier64 Y; }; U Lire(U u);",
            "énumération Etat { Actif = 1, Suivant }; Etat Lire(Etat valeur);",
            "entier32 Modifier(entier32& valeur, constante entier32& lecture);",
            "entier32* Rediriger(entier32*& valeur, constante entier32* lecture);",
            "pointeur_fonction<entier32(entier32)> Creer(); pointeur_fonction<entier32(entier32)> Rappel;",
            "pointeur_fonction<entier32&(entier32&)> Creer();",
            "classe C { publique: constructeur(); destructeur(); entier32 Lire(); };",
            "classe C { privée: entier32 X; alias Valeur = X; entier32 Lire(); protégée: constructeur(entier32 x); publique: destructeur(); };",
            "classe C { publique: entier32 opérateur+(entier32 valeur); entier32 opérateur!(); };",
            "classe Base { publique: constructeur(); virtuel entier32 Lire(); }; classe D : publique Base { publique: constructeur(); remplacer entier32 Lire(); };",
            "classe C { entier32 X; publique: constructeur(entier32 valeur); entier32 Modifier(entier32& valeur); };",
            "classe C { publique: constructeur(); constructeur(entier32 valeur); destructeur(); };",
            "espace N { structure P { entier32 X; }; alias Vue = P; entier32 Lire(Vue p); } utilisant espace N; Vue Globale;",
            "entier32 Valeurs[2][3]; vide Recevoir(entier32* valeurs);",
            "espace A::B { structure P { entier32 X; }; P Lire(constante P& p); } alias Vue = A::B::P;",
            "classe C { publique: constructeur(); }; espace C { entier32 opérateur+(C& objet, entier32 valeur); }",
            "\xEF\xBB\xBF/** Interface UTF-8. **/\r\nespace Démo {\r\nstructure Point { entier32 X; };\r\nPoint Créer(constante Point& valeur);\r\n}\r\n",
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
        {
            std::optional<std::vector<NoeudDeclarationHote>> precedent;
            for (const auto& texte : {valides[index], TraduireCorpusConversions(valides[index])})
            {
                const auto resultat = AnalyserSemantiqueValide(interface, semantique, texte,
                    "interface-memoire-valide-" + std::to_string(index), true);
                NoeudDeclarationHote sentinelle;
                std::memset(&sentinelle, 0xA5, sizeof(sentinelle));
                auto tampon = resultat.Noeuds;
                tampon.push_back(sentinelle);
                RequeteAnalyseDeclarationsHote requete{
                    texte.data(), texte.size(), tampon.data(), resultat.Noeuds.size(), {}};
                Exiger(interface(&requete) == 0
                        && std::memcmp(tampon.data(), resultat.Noeuds.data(), resultat.Noeuds.size() * sizeof(sentinelle)) == 0
                        && std::memcmp(&tampon.back(), &sentinelle, sizeof(sentinelle)) == 0,
                    "AST d'interface non déterministe ou écriture au-delà de la capacité exacte");
                std::fill(tampon.begin(), tampon.end(), sentinelle);
                --requete.Capacite;
                Exiger(interface(&requete) == 1 && requete.Resultat.CapaciteRequise == resultat.Noeuds.size()
                        && std::memcmp(tampon.data(), resultat.Noeuds.data(), requete.Capacite * sizeof(sentinelle)) == 0
                        && std::memcmp(&tampon[requete.Capacite], &sentinelle, sizeof(sentinelle)) == 0
                        && std::memcmp(&tampon.back(), &sentinelle, sizeof(sentinelle)) == 0,
                    "préfixe d'AST d'interface incorrect ou écriture après la capacité partielle");
                for (const auto& noeud : resultat.Noeuds)
                {
                    const bool fonction = noeud.Genre == 1 || (noeud.Genre >= 12 && noeud.Genre <= 15);
                    if (fonction)
                        Exiger((noeud.Drapeaux & 2U) != 0 && (noeud.Drapeaux & 4U) == 0,
                            "un prototype d'interface n'est pas externe ou possède un corps");
                    if (noeud.Genre == 1 || noeud.Genre == 3 || (noeud.Genre == 15 && noeud.Parent == 0))
                        Exiger((noeud.Drapeaux & 1U) == 0,
                            "une déclaration racine d'interface ne doit pas exporter une définition publique");
                }
                for (const auto& resolution : resultat.Resolutions)
                    Exiger(!(resultat.Noeuds[resolution.IndexNoeud].Genre == 13
                            && (resolution.Drapeaux & 32768U) != 0),
                        "un prototype de constructeur produit un plan de corps");
                if (precedent)
                {
                    Exiger(precedent->size() == resultat.Noeuds.size(), "taille d'AST d'interface bilingue différente");
                    for (std::size_t noeud = 0; noeud < precedent->size(); ++noeud)
                        Exiger(MemeStructureDeclaration((*precedent)[noeud], resultat.Noeuds[noeud]),
                            "structure d'AST d'interface bilingue différente");
                }
                precedent = resultat.Noeuds;
            }
        }
        const std::vector<std::string> validesSyntaxiques{
            "structure Point { entier32 X; entier32 Y; };",
            "énumération Etat { Actif = 1, Suivant };",
            "union U { entier32 X; entier64 Y; };",
            "structure P { entier32 X; }; alias Vue = P;",
            "publique constante entier32 Limite; entier32 Valeurs[2];",
            "espace N { structure P { entier32 X; }; } utilisant espace N; alias Vue = P;",
        };
        for (std::size_t index = 0; index < validesSyntaxiques.size(); ++index)
            for (const auto& texte : {validesSyntaxiques[index], TraduireCorpusConversions(validesSyntaxiques[index])})
                ComparerDeclarations(interface, texte, "interface-memoire-types-" + std::to_string(index), true);
        const std::vector<std::pair<std::string, std::uint32_t>> refusSyntaxiques{
            {"publique entier32 Lire() { retourner 42; }", 11},
            {"publique entier32 Globale = 42;", 15},
            {"classe C { publique: constructeur() {} };", 11},
            {"classe C { entier32 X; publique: constructeur() : X(42); };", 15},
            {"classe C { publique: constructeur() : soi(42); constructeur(entier32 valeur); };", 15},
            {"classe Base {}; classe C : publique Base { publique: constructeur() : parent(); };", 15},
            {"classe C { publique: destructeur(entier32 valeur); };", 24},
            {"classe C { publique: entier32 Lire() : X(42); };", 25},
            {"entier32 Lire() : X(42);", 25},
            {"structure P { entier32 X = 42; };", 20},
            {"entier32 Lire()", 11},
            {"entier32 Lire(entier32);", 5},
            {"classe C { publique: entier32 Lire() { retourner 42; } };", 11},
            {"classe C { publique: destructeur() {} };", 11},
        };
        for (std::size_t index = 0; index < refusSyntaxiques.size(); ++index)
            for (const auto& texte : {refusSyntaxiques[index].first, TraduireCorpusConversions(refusSyntaxiques[index].first)})
                ComparerErreurDeclarations(interface, texte, refusSyntaxiques[index].second,
                    "interface-memoire-syntaxe-refuse-" + std::to_string(index), true);
        const std::vector<std::pair<std::string, std::uint32_t>> refusSemantiques{
            {"Inconnu Lire();", 100},
            {"vide Lire(vide valeur);", 105},
            {"entier32 Lire(entier32 a, entier32 b, entier32 c, entier32 d, entier32 e);", 106},
            {"entier32& Lire();", 104},
            {"pointeur_fonction<entier32(vide)> Rappel;", 102},
            {"pointeur_fonction<entier32(entier32, entier32, entier32, entier32, entier32)> Rappel;", 103},
            {"vide Globale;", 78},
            {"alias Vue = Inconnu;", 110},
            {"structure P { P Valeur; };", 57},
            {"classe C : publique C {};", 115},
            {"classe Base { publique: virtuel entier32 Lire(); }; classe D : publique Base { publique: remplacer entier64 Lire(); };", 116},
            {"classe C { publique: entier32 opérateur/(); };", 59},
            {"classe C { entier32 X = valeur; publique: constructeur(entier32 valeur); };", 39},
            {"classe C { entier32 X = Absente; publique: constructeur(); };", 39},
            {"vide Recevoir(entier32 valeurs[2]);", 105},
        };
        for (std::size_t index = 0; index < refusSemantiques.size(); ++index)
            for (const auto& texte : {"vide Temoin();\n" + refusSemantiques[index].first,
                                     TraduireCorpusConversions("vide Temoin();\n" + refusSemantiques[index].first)})
                ComparerErreurSemantique(interface, semantique, texte, refusSemantiques[index].second,
                    "interface-memoire-semantique-refuse-" + std::to_string(index), true);

        ComparerDeclarations(interface, "", "interface-memoire-vide", true);
        ComparerErreurDeclarations(source, "entier32 Lire();", 9, "prototype-hors-interface");
        ComparerDeclarations(source, "publique entier32 Lire() { retourner 42; }", "source-apres-interface");
        for (const auto& texteListe : {std::string("entier32 Lire() : X(42);"), std::string("int32 Lire() : X(42);")})
            ComparerErreurDeclarations(source, texteListe, 25, "liste-fonction-hors-constructeur");
        ComparerDeclarations(interface, "entier32 Lire();", "interface-apres-source", true);
        Exiger(interface(nullptr) == 3, "requête d'interface nulle acceptée");
        RequeteAnalyseDeclarationsHote invalide{nullptr, 1, nullptr, 0, {}};
        Exiger(interface(&invalide) == 3 && invalide.Resultat.LigneErreur == 1 && invalide.Resultat.ColonneErreur == 1,
            "source d'interface nulle acceptée");
        const std::string texte = "entier32 Lire();";
        invalide = {texte.data(), texte.size(), nullptr, 1, {}};
        Exiger(interface(&invalide) == 3, "capacité d'interface sans tampon acceptée");
        std::cout << "Interfaces en mémoire : " << valides.size() << " corpus bilingues analysés, "
                  << validesSyntaxiques.size() << " interfaces de types/données analysées, "
                  << refusSyntaxiques.size() << " refus syntaxiques et " << refusSemantiques.size()
                  << " refus sémantiques bilingues, prototypes et AST intacts vérifiés.\n";
    }

    using AssembleurDeclarationsAutoHeberge =
        std::uint32_t (GS_ABI_HOTE *)(RequeteAssemblageDeclarationsHote*);
    using AnalyseurSemantiqueUnitesAutoHeberge =
        std::uint32_t (GS_ABI_HOTE *)(RequeteAnalyseSemantiqueUnitesHote*);

    struct CorpusAssemblage
    {
        std::vector<std::pair<std::string, bool>> Unites;
        std::uint32_t ErreurSemantique = 0;
    };

    /**
     * <résumé>Compare l'assemblage réel aux analyses bootstrap indépendantes et retrouve l'origine des diagnostics.</résumé>
     **/
    void TesterAssemblageDeclarationsPreparees(
        AssembleurDeclarationsAutoHeberge assembler, AnalyseurDeclarationsAutoHeberge source,
        AnalyseurDeclarationsAutoHeberge interface, AnalyseurSemantiqueUnitesAutoHeberge semantique)
    {
        const std::vector<CorpusAssemblage> valides{
            {{{"structure P { entier32 X; };", true}, {"publique entier32 F() { P p = {42}; retourner p.X; }", false}}},
            {{{"énumération Etat { Actif = 42 };", true}, {"publique entier32 F() { retourner convertir<entier32>(Etat::Actif); }", false}}},
            {{{"union U { entier32 X; entier64 Y; };", true}, {"publique entier32 F(U u) { retourner u.X; }", false}}},
            {{{"structure P { entier32 X; }; alias Vue = P;", true}, {"publique entier32 F(constante Vue& p) { retourner p.X; }", false}}},
            {{{"classe C { publique: entier32 X; };", true}, {"publique entier32 F(C c) { retourner c.X; }", false}}},
            {{{"structure P { entier32 X; };", true}, {"énumération E { V = 42 };", true},
              {"publique entier32 Lire(P p) { retourner p.X; }", false},
              {"publique entier32 F() { P p = {42}; retourner Lire(p); }", false}}},
            {{{"", true}, {"\xEF\xBB\xBF", false}, {"publique entier32 F() { retourner 42; }", false}}},
            {{{"\xEF\xBB\xBF// Début\r\nespace Démo { structure Point { entier32 X; }; }\r\n", true},
              {"\xEF\xBB\xBF\r\nespace Démo { publique entier32 Créer() { Point p = {42}; retourner p.X; } }", false}}},
            {{{"publique entier32 Lire() { retourner 42; }", false}, {"structure P { entier32 X; };", true},
              {"publique entier32 F() { retourner Lire(); }", false}}},
            {{{"pointeur_fonction<entier32()> Rappel;", true}, {"publique entier32 F() { retourner Rappel(); }", false}}},
            {{{"espace N { structure P { entier32 X; }; } utilisant espace N;", true},
              {"utilisant espace N; publique entier32 F() { P p = {42}; retourner p.X; }", false}}},
            {{{"structure P { entier32 X; }; // sans LF final", true},
              {"publique entier32 F() { P p = {42}; retourner p.X; } // fin", false}}},
            {{{"espace A { structure P { entier32 X; }; }", true},
              {"espace B { utilisant espace A; } utilisant espace B; publique entier32 F() { P p = {42}; retourner p.X; }", false}}},
        };
        const std::vector<CorpusAssemblage> invalides{
            {{{"Inconnu Lire();", true}, {"publique vide F() {}", false}}, 100},
            {{{"vide Globale;", true}, {"publique vide F() {}", false}}, 78},
            {{{"alias Vue = Inconnu;", true}, {"publique vide F() {}", false}}, 110},
            {{{"structure P { P Valeur; };", true}, {"publique vide F() {}", false}}, 57},
            {{{"classe C : publique C {};", true}, {"publique vide F() {}", false}}, 115},
            {{{"structure P { entier32 X; };", true}, {"\r\npublique entier32 F() { retourner Absente; }", false}}, 18},
            {{{"structure P { entier32 X; };", true}, {"publique vide F() { vide x; }", false}}, 123},
            {{{"structure P { entier32 X; };", true}, {"publique vide F() { constante entier32 x; }", false}}, 125},
            {{{"espace N { structure P { entier32 X; }; } utilisant espace N;", true},
              {"publique entier32 F() { P p = {42}; retourner p.X; }", false}}, 100},
            {{{"espace N { entier32 Lire(); } utilisant espace N;", true},
              {"publique entier32 F() { retourner Lire(); }", false}}, 18},
            {{{"espace A { entier32 Lire(); } espace B { utilisant espace A; }", true},
              {"utilisant espace B; publique entier32 F() { retourner Lire(); }", false}}, 18},
            {{{"espace N { structure P { entier32 X; }; } utilisant espace N;", true},
              {"P Globale; publique vide F() {}", false}}, 100},
        };
        auto tester = [&](const CorpusAssemblage& corpus, bool anglais)
        {
            std::vector<std::string> textes;
            for (const auto& unite : corpus.Unites)
                textes.push_back(anglais ? TraduireCorpusConversions(unite.first) : unite.first);
            const auto textesAvant = textes;
            std::vector<UniteDeclarationsPrepareeHote> unites;
            std::vector<OrigineUniteDeclarationsHote> originesAttendues;
            std::vector<NoeudDeclarationHote> attendus;
            std::string texteAttendu;
            std::uint32_t ligne = 1;
            GsPP::Programme programme;
            for (std::size_t index = 0; index < textes.size(); ++index)
            {
                const auto& texte = textes[index];
                const bool estInterface = corpus.Unites[index].second;
                unites.push_back({texte.data(), texte.size(), estInterface ? 1U : 0U, 0});
                auto noeuds = ComparerDeclarations(estInterface ? interface : source,
                    texte, "assemblage-unite-" + std::to_string(index), estInterface);
                const bool bom = texte.starts_with("\xEF\xBB\xBF");
                const auto tailleBom = bom ? 3U : 0U;
                const auto nombreLignes = 1U + static_cast<std::uint32_t>(std::count(texte.begin(), texte.end(), '\n'));
                originesAttendues.push_back({texteAttendu.size(), texte.size() - tailleBom, ligne, nombreLignes, tailleBom, 0});
                if (attendus.empty()) attendus.push_back(noeuds.front());
                const auto decalageNoeuds = attendus.size() - 1;
                for (std::size_t n = 1; n < noeuds.size(); ++n)
                {
                    auto noeud = noeuds[n];
                    if (noeud.Parent != 0) noeud.Parent += decalageNoeuds;
                    noeud.Ligne += ligne - 1;
                    if (noeud.TailleNom != 0) noeud.DebutNom = noeud.DebutNom - tailleBom + texteAttendu.size();
                    attendus.push_back(noeud);
                }
                texteAttendu += texte.substr(tailleBom) + '\n';
                ligne += nombreLignes;
                const auto fichier = "unite-" + std::to_string(index);
                auto partie = GsPP::AnalyseurSyntaxique(GsPP::Lexeur(texte, fichier).Analyser(), fichier, estInterface).Analyser();
                auto ajouter = [](auto& destination, auto& origine)
                {
                    destination.insert(destination.end(), std::make_move_iterator(origine.begin()), std::make_move_iterator(origine.end()));
                };
                ajouter(programme.Structures, partie.Structures);
                ajouter(programme.Enumerations, partie.Enumerations);
                ajouter(programme.VariablesGlobales, partie.VariablesGlobales);
                ajouter(programme.Fonctions, partie.Fonctions);
                ajouter(programme.Aliases, partie.Aliases);
                ajouter(programme.Utilisations, partie.Utilisations);
                ajouter(programme.EspacesNoms, partie.EspacesNoms);
            }
            const auto unitesAvant = unites;
            RequeteAssemblageDeclarationsHote requete{unites.data(), unites.size(), nullptr, 0, nullptr, 0, nullptr, 0, {}};
            Exiger(assembler(&requete) == 1 && requete.Resultat.NombreNoeuds == attendus.size()
                && requete.Resultat.NombreOctetsSource == texteAttendu.size() && requete.Resultat.NombreOrigines == unites.size()
                && requete.Resultat.IndexUniteErreur == unites.size() && requete.Resultat.NombreOctetsArene != 0,
                "tailles requises de l'assemblage incorrectes");
            std::vector<char> texteSortie(texteAttendu.size() + 1, '\x5A');
            NoeudDeclarationHote gardeNoeud;
            OrigineUniteDeclarationsHote gardeOrigine;
            std::memset(&gardeNoeud, 0xA5, sizeof(gardeNoeud));
            std::memset(&gardeOrigine, 0xA5, sizeof(gardeOrigine));
            std::vector<NoeudDeclarationHote> noeudsSortie(attendus.size() + 1, gardeNoeud);
            std::vector<OrigineUniteDeclarationsHote> originesSortie(unites.size() + 1, gardeOrigine);
            requete.SourceAssemblee = texteSortie.data();
            requete.CapaciteSource = texteAttendu.size();
            requete.Noeuds = noeudsSortie.data();
            requete.CapaciteNoeuds = attendus.size();
            requete.Origines = originesSortie.data();
            requete.CapaciteOrigines = unites.size();
            auto intact = [&]()
            {
                Exiger(std::all_of(texteSortie.begin(), texteSortie.end(), [](char c) { return c == '\x5A'; }), "texte partiel publié");
                for (const auto& noeud : noeudsSortie)
                    Exiger(std::memcmp(&noeud, &gardeNoeud, sizeof(noeud)) == 0, "AST partiel publié");
                for (const auto& origine : originesSortie)
                    Exiger(std::memcmp(&origine, &gardeOrigine, sizeof(origine)) == 0, "origines partielles publiées");
            };
            for (auto* capacite : {&requete.CapaciteSource, &requete.CapaciteNoeuds, &requete.CapaciteOrigines})
            {
                --*capacite;
                Exiger(assembler(&requete) == 1, "une sortie d'assemblage trop petite n'est pas refusée");
                ++*capacite;
                intact();
            }
            // Injecter un échec à chaque allocation jusqu'au premier assemblage complet.
            bool injectionTerminee = false;
            for (std::uint64_t budget = 0; budget < 128; ++budget)
            {
                LimiteAllocationsAssemblage = NombreAllocations + budget;
                const auto code = assembler(&requete);
                LimiteAllocationsAssemblage.reset();
                Exiger(AllocationsActives.empty() && !LiberationInvalide && NombreAllocations == NombreLiberations,
                    "une allocation refusée laisse une arène vivante");
                if (code == 0) { injectionTerminee = true; break; }
                Exiger(code == 3, "échec d'allocation non propagé par l'assemblage");
                intact();
            }
            Exiger(injectionTerminee && assembler(&requete) == 0, "assemblage complet impossible");
            Exiger(std::string(texteSortie.data(), texteAttendu.size()) == texteAttendu && texteSortie.back() == '\x5A', "texte assemblé incorrect ou non borné");
            Exiger(std::memcmp(noeudsSortie.data(), attendus.data(), attendus.size() * sizeof(gardeNoeud)) == 0
                && std::memcmp(&noeudsSortie.back(), &gardeNoeud, sizeof(gardeNoeud)) == 0, "AST assemblé incorrect ou non borné");
            Exiger(std::memcmp(originesSortie.data(), originesAttendues.data(), unites.size() * sizeof(gardeOrigine)) == 0
                && std::memcmp(&originesSortie.back(), &gardeOrigine, sizeof(gardeOrigine)) == 0, "origines incorrectes ou non bornées");
            Exiger(textes == textesAvant && std::memcmp(unites.data(), unitesAvant.data(), unites.size() * sizeof(unites[0])) == 0, "unités d'entrée modifiées");

            std::optional<std::tuple<std::string, std::uint32_t, std::uint32_t>> diagnostic;
            std::string messageBootstrap;
            try { GsPP::AnalyseurSemantique().Analyser(programme); }
            catch (const GsPP::ErreurCompilation& erreur)
            {
                diagnostic = {erreur.Fichier(), static_cast<std::uint32_t>(erreur.Ligne()), static_cast<std::uint32_t>(erreur.Colonne())};
                messageBootstrap = erreur.what();
            }
            Exiger(diagnostic.has_value() == (corpus.ErreurSemantique != 0),
                "contrat sémantique bootstrap de l'assemblage incorrect : " + messageBootstrap + ", source=" + texteAttendu);
            const auto astAvant = noeudsSortie;
            RequeteAnalyseSemantiqueHote analyse{texteSortie.data(), texteAttendu.size(), noeudsSortie.data(), attendus.size(), nullptr, 0, nullptr, 0, {}};
            RequeteAnalyseSemantiqueUnitesHote analyseUnites{&analyse, originesSortie.data(), unites.size(), 0, 0, 0};
            const auto code = semantique(&analyseUnites);
            if (diagnostic)
            {
                std::size_t indexOrigine = originesAttendues.size();
                for (std::size_t i = 0; i < originesAttendues.size(); ++i)
                    if (analyse.Resultat.LigneErreur >= originesAttendues[i].PremiereLigne
                        && analyse.Resultat.LigneErreur - originesAttendues[i].PremiereLigne < originesAttendues[i].NombreLignes)
                        indexOrigine = i;
                Exiger(indexOrigine < unites.size() && code == corpus.ErreurSemantique, "diagnostic assemblé sans origine ou code incorrect : " + std::to_string(code));
                Exiger(std::get<0>(*diagnostic) == "unite-" + std::to_string(indexOrigine)
                    && std::get<1>(*diagnostic) == analyse.Resultat.LigneErreur - originesAttendues[indexOrigine].PremiereLigne + 1
                    && std::get<2>(*diagnostic) == analyse.Resultat.ColonneErreur
                    && analyseUnites.IndexUniteErreur == indexOrigine
                    && analyseUnites.LigneLocaleErreur == std::get<1>(*diagnostic)
                    && analyseUnites.ColonneLocaleErreur == std::get<2>(*diagnostic), "origine du diagnostic différente du bootstrap");
                ++NombreRefusSemantiquesDifferentiels;
            }
            else
            {
                Exiger(code == 4, "mesure sémantique de l'assemblage valide refusée : " + std::to_string(code));
                std::vector<SymboleSemantiqueHote> symboles(analyse.Resultat.NombreSymboles);
                std::vector<ResolutionSemantiqueHote> resolutions(analyse.Resultat.NombreResolutions);
                analyse.Symboles = symboles.data(); analyse.CapaciteSymboles = symboles.size();
                analyse.Resolutions = resolutions.data(); analyse.CapaciteResolutions = resolutions.size();
                Exiger(semantique(&analyseUnites) == 0 && analyseUnites.IndexUniteErreur == unites.size()
                    && analyseUnites.LigneLocaleErreur == 0 && analyseUnites.ColonneLocaleErreur == 0,
                    "validation sémantique de l'assemblage échouée");
            }
            Exiger(std::memcmp(noeudsSortie.data(), astAvant.data(), astAvant.size() * sizeof(gardeNoeud)) == 0, "AST assemblé modifié par la sémantique");
            analyse.Symboles = nullptr; analyse.CapaciteSymboles = 0;
            analyse.Resolutions = nullptr; analyse.CapaciteResolutions = 0;
            const auto originesAvant = originesSortie;
            for (int champ = 0; champ < 8; ++champ)
            {
                auto mauvaisesOrigines = originesSortie;
                if (champ == 0) ++mauvaisesOrigines[0].DebutOctets;
                if (champ == 1) mauvaisesOrigines[0].TailleOctets = UINT64_MAX;
                if (champ == 2) ++mauvaisesOrigines[0].PremiereLigne;
                if (champ == 3) ++mauvaisesOrigines[0].NombreLignes;
                if (champ == 4) mauvaisesOrigines[0].OctetsBom = 1;
                if (champ == 5) mauvaisesOrigines[0].Reserve = 1;
                analyseUnites.Origines = champ == 6 ? nullptr : mauvaisesOrigines.data();
                analyseUnites.NombreOrigines = champ == 7 ? 0 : unites.size();
                Exiger(semantique(&analyseUnites) == 1 && analyse.Resultat.Erreur == 1
                    && analyseUnites.IndexUniteErreur == analyseUnites.NombreOrigines
                    && analyseUnites.LigneLocaleErreur == 0 && analyseUnites.ColonneLocaleErreur == 0,
                    "table d'origines incohérente acceptée ou diagnostic périmé conservé");
            }
            analyseUnites.Origines = originesSortie.data(); analyseUnites.NombreOrigines = unites.size();
            const auto separateur = static_cast<std::size_t>(originesSortie[0].TailleOctets);
            texteSortie[separateur] = ' ';
            Exiger(semantique(&analyseUnites) == 1, "séparateur d'unité absent accepté");
            texteSortie[separateur] = '\n';
            --analyse.TailleSource;
            Exiger(semantique(&analyseUnites) == 1, "source assemblée tronquée acceptée");
            ++analyse.TailleSource;
            Exiger(std::memcmp(noeudsSortie.data(), astAvant.data(), astAvant.size() * sizeof(gardeNoeud)) == 0
                && std::memcmp(originesSortie.data(), originesAvant.data(), originesAvant.size() * sizeof(gardeOrigine)) == 0,
                "AST ou origines modifiés après rejet d'une table d'origines");
        };
        for (const auto& corpus : valides) for (bool anglais : {false, true}) tester(corpus, anglais);
        for (const auto& corpus : invalides) for (bool anglais : {false, true}) tester(corpus, anglais);

        // L'assemblage ne prétend pas normaliser une déclaration et sa définition.
        for (bool anglais : {false, true})
        {
            const auto prototype = anglais ? std::string("int32 Lire();") : std::string("entier32 Lire();");
            const auto definition = anglais ? std::string("public int32 Lire() { return 42; }")
                                            : std::string("publique entier32 Lire() { retourner 42; }");
            UniteDeclarationsPrepareeHote unites[]{{prototype.data(), prototype.size(), 1, 0}, {definition.data(), definition.size(), 0, 0}};
            RequeteAssemblageDeclarationsHote analyse{unites, 2, nullptr, 0, nullptr, 0, nullptr, 0, {}};
            Exiger(assembler(&analyse) == 1, "assemblage prototype/définition non mesuré");
            std::vector<char> texte(analyse.Resultat.NombreOctetsSource);
            std::vector<NoeudDeclarationHote> noeuds(analyse.Resultat.NombreNoeuds);
            std::vector<OrigineUniteDeclarationsHote> origines(2);
            analyse.SourceAssemblee = texte.data(); analyse.CapaciteSource = texte.size();
            analyse.Noeuds = noeuds.data(); analyse.CapaciteNoeuds = noeuds.size();
            analyse.Origines = origines.data(); analyse.CapaciteOrigines = origines.size();
            Exiger(assembler(&analyse) == 0 && std::count_if(noeuds.begin(), noeuds.end(),
                [](const auto& n) { return n.Genre == 1 && n.Parent == 0; }) == 2,
                "une normalisation implicite a supprimé un prototype ou sa définition");
        }

        Exiger(assembler(nullptr) == 2, "requête d'assemblage nulle acceptée");
        Exiger(semantique(nullptr) == 1, "requête de sémantique par unité nulle acceptée");
        RequeteAnalyseSemantiqueUnitesHote analyseNulle{};
        Exiger(semantique(&analyseNulle) == 1, "analyse par unité sans requête sémantique acceptée");
        RequeteAssemblageDeclarationsHote requete{};
        Exiger(assembler(&requete) == 2, "assemblage sans unité accepté");
        const std::string valide = "structure P { entier32 X; };";
        const std::vector<std::pair<std::string, bool>> erreursSyntaxiques{
            {"entier32 Lire() { retourner 42; }", true}, {"entier32 X = 42;", true},
            {"structure P { entier32 X;", true}, {"entier32 F() { retourner ;", false},
            {"/** commentaire inachevé", true}, {"\xFF", false},
        };
        for (const auto& [texteInitial, estInterface] : erreursSyntaxiques)
            for (const auto& texte : {texteInitial, TraduireCorpusConversions(texteInitial)})
            {
                UniteDeclarationsPrepareeHote unites[]{{valide.data(), valide.size(), 1, 0}, {texte.data(), texte.size(), estInterface ? 1U : 0U, 0}};
                NoeudDeclarationHote garde;
                OrigineUniteDeclarationsHote origine;
                std::memset(&garde, 0xA5, sizeof(garde)); std::memset(&origine, 0xA5, sizeof(origine));
                const auto gardeAvant = garde; const auto origineAvant = origine;
                char sortie = 'Z';
                requete = {unites, 2, &sortie, 1, &garde, 1, &origine, 1, {}};
                RequeteAnalyseDeclarationsHote analyse{texte.data(), texte.size(), nullptr, 0, {}};
                const auto code = (estInterface ? interface : source)(&analyse);
                Exiger(assembler(&requete) == 4 && requete.Resultat.IndexUniteErreur == 1
                    && requete.Resultat.Detail == code && requete.Resultat.DetailLexical == analyse.Resultat.Detail
                    && requete.Resultat.LigneErreur == analyse.Resultat.LigneErreur && requete.Resultat.ColonneErreur == analyse.Resultat.ColonneErreur,
                    "diagnostic syntaxique de l'unité perdu pendant l'assemblage");
                std::uint32_t ligne = 0, colonne = 0;
                try { (void)GsPP::AnalyseurSyntaxique(GsPP::Lexeur(texte, "unite-1").Analyser(), "unite-1", estInterface).Analyser(); }
                catch (const GsPP::ErreurCompilation& erreur) { ligne = static_cast<std::uint32_t>(erreur.Ligne()); colonne = static_cast<std::uint32_t>(erreur.Colonne()); }
                Exiger(ligne == requete.Resultat.LigneErreur && colonne == requete.Resultat.ColonneErreur && ligne != 0,
                    "diagnostic syntaxique assemblé différent du bootstrap");
                Exiger(sortie == 'Z' && std::memcmp(&garde, &gardeAvant, sizeof(garde)) == 0
                    && std::memcmp(&origine, &origineAvant, sizeof(origine)) == 0, "sorties altérées après refus syntaxique");
            }
        for (const auto unite : std::vector<UniteDeclarationsPrepareeHote>{
                 {nullptr, 1, 0, 0}, {valide.data(), valide.size(), 2, 0}, {valide.data(), valide.size(), 1, 1},
                 {valide.data(), 1'000'000'000, 0, 0}, {valide.data(), UINT64_MAX, 0, 0}})
        {
            requete = {&unite, 1, nullptr, 0, nullptr, 0, nullptr, 0, {}};
            Exiger(assembler(&requete) == (unite.Taille >= 1'000'000'000 ? 5U : 2U)
                && requete.Resultat.IndexUniteErreur == 0, "unité incohérente ou taille excessive acceptée");
        }
        UniteDeclarationsPrepareeHote unite{valide.data(), valide.size(), 1, 0};
        for (auto capacite : {0, 1, 2})
        {
            requete = {&unite, 1, nullptr, 0, nullptr, 0, nullptr, 0, {}};
            if (capacite == 0) requete.CapaciteSource = 1;
            if (capacite == 1) requete.CapaciteNoeuds = 1;
            if (capacite == 2) requete.CapaciteOrigines = 1;
            Exiger(assembler(&requete) == 2, "capacité sans tampon d'assemblage acceptée");
        }
        requete = {&unite, 1'000'001, nullptr, 0, nullptr, 0, nullptr, 0, {}};
        Exiger(assembler(&requete) == 5, "nombre excessif d'unités accepté");
        UniteDeclarationsPrepareeHote tropGrandes[]{{valide.data(), 999'999'999, 0, 0}, {valide.data(), 1, 0, 0}};
        requete = {tropGrandes, 2, nullptr, 0, nullptr, 0, nullptr, 0, {}};
        Exiger(assembler(&requete) == 5 && requete.Resultat.IndexUniteErreur == 1,
            "taille cumulée excessive acceptée avant l'analyse des textes");
        UniteDeclarationsPrepareeHote vides[]{{nullptr, 0, 1, 0}, {nullptr, 0, 0, 0}};
        char texteVide[2]{};
        NoeudDeclarationHote racineVide{};
        OrigineUniteDeclarationsHote originesVides[2]{};
        requete = {vides, 2, texteVide, 2, &racineVide, 1, originesVides, 2, {}};
        Exiger(assembler(&requete) == 0 && requete.Resultat.NombreNoeuds == 1
            && requete.Resultat.NombreOctetsSource == 2 && texteVide[0] == '\n' && texteVide[1] == '\n'
            && racineVide.Genre == 0 && originesVides[0].PremiereLigne == 1 && originesVides[1].PremiereLigne == 2,
            "assemblage d'unités vides avec pointeurs nuls incorrect");
        Exiger(AllocationsActives.empty() && !LiberationInvalide && NombreAllocations == NombreLiberations, "assemblage avec fuite mémoire");
        std::cout << "Assemblage préparé : " << valides.size() << " corpus bilingues valides, " << invalides.size()
                  << " refus sémantiques et " << erreursSyntaxiques.size() << " refus syntaxiques bilingues, origines et sorties transactionnelles vérifiées.\n";
    }

    /** <résumé>Compare les sélections au normaliseur C++, puis analyse les vrais AST normalisés Gs++.</résumé> **/
    void TesterNormalisationDeclarationsPreparees(
        AssembleurDeclarationsAutoHeberge normaliser,
        AnalyseurDeclarationsAutoHeberge source, AnalyseurDeclarationsAutoHeberge interface,
        AnalyseurSemantiqueUnitesAutoHeberge semantique)
    {
        struct Corpus
        {
            std::vector<std::pair<std::string, bool>> Unites;
            std::uint32_t ErreurNormalisation = 0;
            std::uint32_t ErreurSemantique = 0;
        };
        const std::vector<Corpus> valides{
            {{{"entier32 Lire(entier32 valeur);", true}, {"publique entier32 Lire(entier32 x) { retourner x; }", false}}},
            {{{"publique entier32 Lire() { retourner 42; }", false}, {"entier32 Lire();", true}}},
            {{{"externe entier32 Lire(); externe entier32 Lire();", false}, {"publique entier32 Lire() { retourner 42; }", false}, {"entier32 Lire();", true}}},
            {{{"entier32 Lire(entier32 valeur); entier32 Lire(entier64 valeur);", true}, {"publique entier32 Lire(entier64 x) { retourner 2; } publique entier32 Lire(entier32 x) { retourner x; }", false}}},
            {{{"espace A::B { entier32 Lire(); }", true}, {"espace A { espace B { publique entier32 Lire() { retourner 42; } } }", false}}},
            {{{"espace A { entier32 Lire(); } espace B { entier32 Lire(); }", true}, {"espace B { publique entier32 Lire() { retourner 2; } } espace A { publique entier32 Lire() { retourner 1; } }", false}}},
            {{{"vide Modifier(entier32& valeur, constante entier32& lecture);", true}, {"publique vide Modifier(entier32& x, constante entier32& y) { x = y; }", false}}},
            {{{"vide Lire(constante volatile entier32* valeur);", true}, {"publique vide Lire(volatile constante constante entier32* x) {}", false}}},
            {{{"structure P { entier32 X; }; P Creer(P valeur);", true}, {"publique P Creer(P p) { retourner p; }", false}}},
            {{{"espace N { structure P { entier32 X; }; } vide Lire(N :: P* valeur);", true}, {"publique vide Lire(N::P* p) {}", false}}},
            {{{"pointeur_fonction<entier32()> Creer(); entier32 Cible();", true}, {"publique entier32 Cible() { retourner 42; } publique pointeur_fonction<entier32()> Creer() { retourner Cible; }", false}}},
            {{{"vide Lire(pointeur_fonction<pointeur_fonction<entier32()>()> valeur);", true}, {"publique vide Lire(pointeur_fonction< pointeur_fonction< int32() > () > x) {}", false}}},
            {{{"vide Lire(pointeur_fonction<entier32&(entier32&)> valeur);", true}, {"publique vide Lire(pointeur_fonction<entier32&(entier32&)> x) {}", false}}},
            {{{"entier32 Globale;", true}, {"publique entier32 Globale = 42; publique entier32 Lire() { retourner Globale; }", false}, {"entier32 Globale;", true}}},
            {{{"publique entier32 Globale; publique vide Lire() {}", false}, {"entier32 Globale;", true}}},
            {{{"entier32 Valeurs[1_0];", true}, {"publique entier32 Valeurs[10] = {42}; publique entier32 Lire() { retourner Valeurs[0]; }", false}}},
            {{{"constante entier32 Globale;", true}, {"publique constante entier32 Globale = 42; publique entier32 Lire() { retourner Globale; }", false}}},
            {{{"espace N { structure P { entier32 X; }; } alias Vue = N :: P;", true}, {"alias Vue = N::P; publique entier32 Lire(Vue p) { retourner p.X; }", false}}},
            {{{"structure P { entier32 X; }; espace N { alias Vue = P; }", true}, {"alias N :: Vue = P; publique entier32 Lire(N::Vue p) { retourner p.X; }", false}}},
            {{{"structure P { entier32 X; }; entier32 opérateur+(P p, entier32 x);", true}, {"publique entier32 opérateur+(P p, entier32 x) { retourner p.X + x; }", false}}},
            {{{"\xEF\xBB\xBF\r\nespace Démo { entier32 Créer(); } // fin", true}, {"\xEF\xBB\xBF\r\nespace Démo { publique entier32 Créer() { retourner 42; } }", false}}},
            {{{"classe C { publique: entier32 Lire() { retourner 42; } }; externe entier32 F(C& c);", false}, {"publique entier32 F(C& c) { retourner c.Lire(); }", false}}},
        };
        const std::vector<Corpus> refus{
            {{{"entier32 Lire();", true}, {"publique entier64 Lire() { retourner 42; }", false}}, 6},
            {{{"publique entier32 Lire() { retourner 42; }", false}, {"entier64 Lire();", true}}, 6},
            {{{"publique entier32 Lire(entier32 x) { retourner x; }", false}, {"publique entier32 Lire(entier32 y) { retourner y; }", false}}, 7},
            {{{"entier32 Lire();", true}, {"publique entier32 Lire() { retourner 1; } publique entier32 Lire() { retourner 2; }", false}}, 7},
            {{{"entier32* Lire();", true}, {"publique constante entier32* Lire() { retourner 0; }", false}}, 6},
            {{{"pointeur_fonction<entier32()> Lire();", true}, {"publique pointeur_fonction<entier64()> Lire() { retourner 0; }", false}}, 6},
            {{{"entier32 Globale;", true}, {"entier64 Globale; publique vide F() {}", false}}, 8},
            {{{"entier32 Globale;", false}, {"entier32 Globale; publique vide F() {}", false}}, 9},
            {{{"entier32 Valeurs[2][3];", true}, {"entier32 Valeurs[3][2]; publique vide F() {}", false}}, 8},
            {{{"constante entier32 Globale;", true}, {"entier32 Globale; publique vide F() {}", false}}, 8},
            {{{"structure P {}; structure Q {}; alias Vue = P;", true}, {"alias Vue = Q; publique vide F() {}", false}}, 10},
            {{{"espace N { structure P {}; } alias Vue = N::P;", true}, {"espace N { alias Vue = P; } alias Vue = N::Vue; publique vide F() {}", false}}, 10},
            // Fonctions avant globales avant alias, indépendamment de leurs positions dans le texte.
            {{{"entier32 Globale; alias Vue = Inconnu; entier32 Lire();", true}, {"alias Vue = Autre; entier64 Globale; publique entier64 Lire() { retourner 0; }", false}}, 6},
            {{{"entier32 Globale; alias Vue = Inconnu;", true}, {"alias Vue = Autre; entier64 Globale; publique vide F() {}", false}}, 8},
            {{{"publique entier32 F() { retourner 1; } publique entier32 G() { retourner 2; }", false}, {"publique entier32 G() { retourner 3; } publique entier32 F() { retourner 4; }", false}}, 7},
            // Une unité syntaxiquement invalide est refusée avant la normalisation du programme.
            {{{"entier32 Lire();", true}, {"publique entier64 Lire() { retourner 1; }", false}, {"structure P { entier32 X;", true}}, 4},
        };
        const std::vector<Corpus> refusSemantiques{
            {{{"entier32 F(); entier32 G();", true}, {"publique entier32 G() { retourner AbsentG; } publique entier32 F() { retourner AbsentF; }", false}}, 0, 18},
            {{{"vide F(); vide G();", true}, {"publique vide G() { vide x; } publique vide F() { constante entier32 y; }", false}}, 0, 125},
            {{{"vide F(entier32 a, entier32 b, entier32 c, entier32 d, entier32 e);", true}, {"publique vide F(entier32 v, entier32 w, entier32 x, entier32 y, entier32 z) {}", false}}, 0, 106},
            {{{"structure P {}; alias Vue = P; vide F(Vue* p);", true}, {"publique vide F(P* p) {}", false}}, 0, 118},
            {{{"structure " + NomCollisionLiaisonA + " {}; structure " + NomCollisionLiaisonB + " {};", true},
              {"publique vide F(" + NomCollisionLiaisonA + "* p) {} publique vide F(" + NomCollisionLiaisonB + "* p) {}", false}}, 0, 119},
            {{{"structure " + NomCollisionCallbackA + " {}; structure " + NomCollisionCallbackB + " {};", true},
              {"publique vide F(pointeur_fonction<vide(" + NomCollisionCallbackA + "*)> p) {} publique vide F(pointeur_fonction<vide(" + NomCollisionCallbackB + "*)> p) {}", false}}, 0, 119},
            {{{"Inconnu F();", true}, {"publique Inconnu F() { retourner {}; }", false}}, 0, 100},
            {{{"espace N { structure P {}; } utilisant espace N; vide F();", true}, {"publique vide F() { P p; }", false}}, 0, 100},
        };
        std::size_t nombreRefusNormalisation = 0;
        auto cleFonction = [](const GsPP::Fonction& fonction)
        {
            std::string cle = "F:" + fonction.NomSourceComplet() + '(';
            for (const auto& parametre : fonction.Parametres) cle += parametre.Type.Afficher() + ';';
            return cle + ')';
        };
        auto tester = [&](const Corpus& corpus, bool anglais)
        {
            std::vector<std::string> textes;
            for (const auto& unite : corpus.Unites) textes.push_back(anglais ? TraduireCorpusConversions(unite.first) : unite.first);
            const auto textesAvant = textes;
            std::vector<UniteDeclarationsPrepareeHote> unites;
            std::vector<OrigineUniteDeclarationsHote> origines;
            std::vector<NoeudDeclarationHote> bruts;
            std::string texteBrut;
            using Position = std::pair<std::uint32_t, std::uint32_t>;
            std::map<Position, std::string> cles;
            GsPP::Programme programme;
            std::optional<std::tuple<std::string, std::uint32_t, std::uint32_t>> diagnostic;
            std::string messageBootstrap;
            std::uint32_t premiereLigne = 1;
            try
            {
                for (std::size_t index = 0; index < textes.size(); ++index)
                {
                    const auto& texte = textes[index];
                    const auto fichier = "unite-" + std::to_string(index);
                    unites.push_back({texte.data(), texte.size(), corpus.Unites[index].second ? 1U : 0U, 0});
                    const auto bom = texte.starts_with("\xEF\xBB\xBF") ? 3U : 0U;
                    const auto nombreLignes = 1U + static_cast<std::uint32_t>(std::count(texte.begin(), texte.end(), '\n'));
                    origines.push_back({texteBrut.size(), texte.size() - bom, premiereLigne, nombreLignes, bom, 0});
                    auto jetons = GsPP::Lexeur(texte, fichier).Analyser();
                    for (auto& jeton : jetons) jeton.Ligne += premiereLigne - 1;
                    auto partie = GsPP::AnalyseurSyntaxique(std::move(jetons), fichier, corpus.Unites[index].second).Analyser();
                    for (const auto& fonction : partie.Fonctions)
                        if (!fonction.EstMethode) cles[{static_cast<std::uint32_t>(fonction.Position.Ligne), static_cast<std::uint32_t>(fonction.Position.Colonne)}] = cleFonction(fonction);
                    for (const auto& globale : partie.VariablesGlobales)
                        cles[{static_cast<std::uint32_t>(globale.Position.Ligne), static_cast<std::uint32_t>(globale.Position.Colonne)}] = "G:" + globale.NomComplet();
                    for (const auto& alias : partie.Aliases)
                        cles[{static_cast<std::uint32_t>(alias.Position.Ligne), static_cast<std::uint32_t>(alias.Position.Colonne)}] = "A:" + alias.NomComplet();
                    auto ajouter = [](auto& destination, auto& origine)
                    {
                        destination.insert(destination.end(), std::make_move_iterator(origine.begin()), std::make_move_iterator(origine.end()));
                    };
                    ajouter(programme.Structures, partie.Structures); ajouter(programme.Enumerations, partie.Enumerations);
                    ajouter(programme.VariablesGlobales, partie.VariablesGlobales); ajouter(programme.Fonctions, partie.Fonctions);
                    ajouter(programme.Aliases, partie.Aliases); ajouter(programme.Utilisations, partie.Utilisations); ajouter(programme.EspacesNoms, partie.EspacesNoms);
                    auto noeuds = ComparerDeclarations(corpus.Unites[index].second ? interface : source, texte, fichier, corpus.Unites[index].second);
                    if (bruts.empty()) bruts.push_back(noeuds.front());
                    const auto base = bruts.size() - 1;
                    for (std::size_t n = 1; n < noeuds.size(); ++n)
                    {
                        auto noeud = noeuds[n];
                        if (noeud.Parent != 0) noeud.Parent += base;
                        noeud.Ligne += premiereLigne - 1;
                        if (noeud.TailleNom != 0) noeud.DebutNom = noeud.DebutNom - bom + texteBrut.size();
                        bruts.push_back(noeud);
                    }
                    texteBrut += texte.substr(bom) + '\n';
                    premiereLigne += nombreLignes;
                }
                GsPP::NormaliserDeclarations(programme);
            }
            catch (const GsPP::ErreurCompilation& erreur)
            {
                diagnostic = {erreur.Fichier(), static_cast<std::uint32_t>(erreur.Ligne()), static_cast<std::uint32_t>(erreur.Colonne())};
                messageBootstrap = erreur.what();
            }
            // Même en cas de refus syntaxique, fournir toutes les unités à l'entrée Gs++.
            unites.clear();
            for (std::size_t index = 0; index < textes.size(); ++index)
                unites.push_back({textes[index].data(), textes[index].size(), corpus.Unites[index].second ? 1U : 0U, 0});
            const auto unitesAvant = unites;
            NoeudDeclarationHote gardeNoeud; OrigineUniteDeclarationsHote gardeOrigine;
            std::memset(&gardeNoeud, 0xA5, sizeof(gardeNoeud)); std::memset(&gardeOrigine, 0xA5, sizeof(gardeOrigine));
            char gardeTexte = 'Z';
            auto noeudGarde = gardeNoeud; auto origineGarde = gardeOrigine;
            RequeteAssemblageDeclarationsHote requete{unites.data(), unites.size(), &gardeTexte, 1, &noeudGarde, 1, &origineGarde, 1, {}};
            const auto mesure = normaliser(&requete);
            auto localiser = [&](std::uint32_t ligne)
            {
                for (std::size_t index = 0; index < origines.size(); ++index)
                    if (ligne >= origines[index].PremiereLigne && ligne - origines[index].PremiereLigne < origines[index].NombreLignes)
                        return std::pair<std::uint64_t, std::uint32_t>{index, ligne - origines[index].PremiereLigne + 1};
                throw std::runtime_error("diagnostic bootstrap hors table d'origines");
            };
            Exiger(gardeTexte == 'Z' && std::memcmp(&noeudGarde, &gardeNoeud, sizeof(gardeNoeud)) == 0
                && std::memcmp(&origineGarde, &gardeOrigine, sizeof(gardeOrigine)) == 0, "sorties modifiées pendant la mesure normalisée");
            if (corpus.ErreurNormalisation != 0)
            {
                Exiger(diagnostic.has_value(), "le bootstrap devait refuser la normalisation");
                const auto [index, ligne] = localiser(std::get<1>(*diagnostic));
                Exiger(mesure == corpus.ErreurNormalisation && requete.Resultat.Erreur == mesure
                    && requete.Resultat.IndexUniteErreur == index && requete.Resultat.LigneErreur == ligne
                    && requete.Resultat.ColonneErreur == std::get<2>(*diagnostic)
                    && std::get<0>(*diagnostic) == "unite-" + std::to_string(index),
                    "diagnostic de normalisation différent du bootstrap : attendu=" + std::to_string(corpus.ErreurNormalisation)
                    + ", obtenu=" + std::to_string(mesure) + ", bootstrap=" + messageBootstrap);
                if (mesure != 4) ++nombreRefusNormalisation;
                return;
            }
            Exiger(!diagnostic && mesure == 1, "normalisation valide refusée : " + std::to_string(mesure) + " " + messageBootstrap);
            std::map<std::string, Position> selections;
            for (const auto& fonction : programme.Fonctions)
                if (!fonction.EstMethode) selections[cleFonction(fonction)] = {static_cast<std::uint32_t>(fonction.Position.Ligne), static_cast<std::uint32_t>(fonction.Position.Colonne)};
            for (const auto& globale : programme.VariablesGlobales)
                selections["G:" + globale.NomComplet()] = {static_cast<std::uint32_t>(globale.Position.Ligne), static_cast<std::uint32_t>(globale.Position.Colonne)};
            for (const auto& alias : programme.Aliases)
                selections["A:" + alias.NomComplet()] = {static_cast<std::uint32_t>(alias.Position.Ligne), static_cast<std::uint32_t>(alias.Position.Colonne)};
            std::map<Position, std::size_t> racines;
            std::vector<std::size_t> fins(bruts.size(), bruts.size());
            std::size_t precedent = 0;
            for (std::size_t index = 1; index < bruts.size(); ++index)
                if (bruts[index].Parent == 0)
                {
                    fins[precedent] = index; precedent = index;
                    racines[{bruts[index].Ligne, bruts[index].Colonne}] = index;
                }
            std::vector<NoeudDeclarationHote> attendus{bruts.front()};
            std::vector<std::string> dejaVus;
            for (std::size_t index = 1; index < bruts.size(); index = fins[index])
            {
                const Position position{bruts[index].Ligne, bruts[index].Colonne};
                auto choisi = index;
                if (const auto cle = cles.find(position); cle != cles.end())
                {
                    if (std::find(dejaVus.begin(), dejaVus.end(), cle->second) != dejaVus.end()) continue;
                    dejaVus.push_back(cle->second);
                    choisi = racines.at(selections.at(cle->second));
                }
                const auto destination = attendus.size();
                for (std::size_t n = choisi; n < fins[choisi]; ++n)
                {
                    auto noeud = bruts[n];
                    if (noeud.Parent != 0) noeud.Parent = noeud.Parent - choisi + destination;
                    attendus.push_back(noeud);
                }
            }
            Exiger(requete.Resultat.NombreNoeuds == attendus.size() && requete.Resultat.NombreOctetsSource == texteBrut.size()
                && requete.Resultat.NombreOrigines == origines.size(), "capacités normalisées différentes du bootstrap");
            std::vector<char> texteSortie(texteBrut.size() + 1, 'Z');
            std::vector<NoeudDeclarationHote> noeuds(attendus.size() + 1, gardeNoeud);
            std::vector<OrigineUniteDeclarationsHote> originesSortie(origines.size() + 1, gardeOrigine);
            requete.SourceAssemblee = texteSortie.data(); requete.CapaciteSource = texteBrut.size();
            requete.Noeuds = noeuds.data(); requete.CapaciteNoeuds = attendus.size();
            requete.Origines = originesSortie.data(); requete.CapaciteOrigines = origines.size();
            auto intact = [&]()
            {
                Exiger(std::all_of(texteSortie.begin(), texteSortie.end(), [](char c) { return c == 'Z'; }), "texte normalisé partiellement publié");
                for (const auto& noeud : noeuds) Exiger(std::memcmp(&noeud, &gardeNoeud, sizeof(noeud)) == 0, "AST normalisé partiellement publié");
                for (const auto& origine : originesSortie) Exiger(std::memcmp(&origine, &gardeOrigine, sizeof(origine)) == 0, "origines normalisées partiellement publiées");
            };
            for (auto* capacite : {&requete.CapaciteSource, &requete.CapaciteNoeuds, &requete.CapaciteOrigines})
            {
                --*capacite; Exiger(normaliser(&requete) == 1, "capacité partielle normalisée acceptée"); ++*capacite; intact();
            }
            bool succes = false;
            for (std::uint64_t budget = 0; budget < 192; ++budget)
            {
                LimiteAllocationsAssemblage = NombreAllocations + budget;
                const auto code = normaliser(&requete);
                LimiteAllocationsAssemblage.reset();
                Exiger(AllocationsActives.empty() && !LiberationInvalide && NombreAllocations == NombreLiberations, "arène de normalisation non libérée");
                if (code == 0) { succes = true; break; }
                Exiger(code == 3, "échec d'allocation du normaliseur non propagé"); intact();
            }
            Exiger(succes && normaliser(&requete) == 0, "normalisation complète impossible");
            Exiger(std::string(texteSortie.data(), texteBrut.size()) == texteBrut && texteSortie.back() == 'Z'
                && std::memcmp(noeuds.data(), attendus.data(), attendus.size() * sizeof(gardeNoeud)) == 0
                && std::memcmp(&noeuds.back(), &gardeNoeud, sizeof(gardeNoeud)) == 0
                && std::memcmp(originesSortie.data(), origines.data(), origines.size() * sizeof(gardeOrigine)) == 0
                && std::memcmp(&originesSortie.back(), &gardeOrigine, sizeof(gardeOrigine)) == 0, "normalisation différente des sélections bootstrap ou sortie non bornée");
            Exiger(textes == textesAvant && std::memcmp(unites.data(), unitesAvant.data(), unites.size() * sizeof(unites[0])) == 0, "entrées de normalisation modifiées");
            const auto astAvant = noeuds;
            const auto originesAvant = originesSortie;
            const auto texteAvant = texteSortie;
            diagnostic.reset();
            try { GsPP::AnalyseurSemantique().Analyser(programme); }
            catch (const GsPP::ErreurCompilation& erreur)
            {
                diagnostic = {erreur.Fichier(), static_cast<std::uint32_t>(erreur.Ligne()), static_cast<std::uint32_t>(erreur.Colonne())};
                messageBootstrap = erreur.what();
            }
            Exiger(diagnostic.has_value() == (corpus.ErreurSemantique != 0), "contrat sémantique après normalisation incorrect : " + messageBootstrap);
            RequeteAnalyseSemantiqueHote analyse{texteSortie.data(), texteBrut.size(), noeuds.data(), attendus.size(), nullptr, 0, nullptr, 0, {}};
            RequeteAnalyseSemantiqueUnitesHote analyseUnites{&analyse, originesSortie.data(), origines.size(), 0, 0, 0};
            const auto code = semantique(&analyseUnites);
            if (diagnostic)
            {
                const auto [index, ligne] = localiser(std::get<1>(*diagnostic));
                Exiger(code == corpus.ErreurSemantique && analyseUnites.IndexUniteErreur == index
                    && analyseUnites.LigneLocaleErreur == ligne && analyseUnites.ColonneLocaleErreur == std::get<2>(*diagnostic),
                    "priorité sémantique après normalisation différente : attendu=" + std::to_string(corpus.ErreurSemantique)
                    + ", obtenu=" + std::to_string(code) + ", bootstrap=" + messageBootstrap);
                ++NombreRefusSemantiquesDifferentiels;
            }
            else
            {
                Exiger(code == 4, "AST normalisé valide refusé : code=" + std::to_string(code));
                std::vector<SymboleSemantiqueHote> symboles(analyse.Resultat.NombreSymboles);
                std::vector<ResolutionSemantiqueHote> resolutions(analyse.Resultat.NombreResolutions);
                analyse.Symboles = symboles.data(); analyse.CapaciteSymboles = symboles.size();
                analyse.Resolutions = resolutions.data(); analyse.CapaciteResolutions = resolutions.size();
                Exiger(semantique(&analyseUnites) == 0 && analyseUnites.IndexUniteErreur == origines.size(), "analyse de l'AST normalisé échouée");
            }
            Exiger(std::memcmp(noeuds.data(), astAvant.data(), astAvant.size() * sizeof(gardeNoeud)) == 0
                && std::memcmp(originesSortie.data(), originesAvant.data(), originesAvant.size() * sizeof(gardeOrigine)) == 0
                && texteSortie == texteAvant, "AST, texte ou origines normalisés modifiés par la sémantique");
        };
        for (const auto& corpus : valides) for (bool anglais : {false, true}) tester(corpus, anglais);
        for (const auto& corpus : refus) for (bool anglais : {false, true}) tester(corpus, anglais);
        for (const auto& corpus : refusSemantiques) for (bool anglais : {false, true}) tester(corpus, anglais);
        Exiger(normaliser(nullptr) == 2, "requête normalisée nulle acceptée");
        RequeteAssemblageDeclarationsHote invalide{};
        Exiger(normaliser(&invalide) == 2, "normalisation sans unité acceptée");
        const std::string texte = "vide F();";
        UniteDeclarationsPrepareeHote unite{texte.data(), texte.size(), 1, 0};
        for (int champ = 0; champ < 3; ++champ)
        {
            invalide = {&unite, 1, nullptr, 0, nullptr, 0, nullptr, 0, {}};
            if (champ == 0) invalide.CapaciteSource = 1;
            if (champ == 1) invalide.CapaciteNoeuds = 1;
            if (champ == 2) invalide.CapaciteOrigines = 1;
            Exiger(normaliser(&invalide) == 2, "capacité sans tampon normalisé acceptée");
        }
        for (const auto uniteInvalide : std::vector<UniteDeclarationsPrepareeHote>{
                 {nullptr, 1, 0, 0}, {texte.data(), texte.size(), 2, 0}, {texte.data(), texte.size(), 1, 1},
                 {texte.data(), UINT64_MAX, 0, 0}})
        {
            invalide = {&uniteInvalide, 1, nullptr, 0, nullptr, 0, nullptr, 0, {}};
            Exiger(normaliser(&invalide) == (uniteInvalide.Taille == UINT64_MAX ? 5U : 2U), "unité normalisée invalide acceptée");
        }
        UniteDeclarationsPrepareeHote uniteVide{nullptr, 0, 1, 0};
        char texteVide{}; NoeudDeclarationHote racineVide{}; OrigineUniteDeclarationsHote origineVide{};
        invalide = {&uniteVide, 1, &texteVide, 1, &racineVide, 1, &origineVide, 1, {}};
        Exiger(normaliser(&invalide) == 0 && texteVide == '\n' && racineVide.Genre == 0
            && invalide.Resultat.NombreNoeuds == 1 && origineVide.NombreLignes == 1,
            "normalisation d'une unité vide incorrecte");
        Exiger(AllocationsActives.empty() && !LiberationInvalide && NombreAllocations == NombreLiberations, "normalisation avec fuite mémoire");
        std::cout << "Normalisation préparée : " << valides.size() << " corpus bilingues valides, " << nombreRefusNormalisation
                  << " refus différentiels de normalisation, 1 refus syntaxique bilingue et " << refusSemantiques.size()
                  << " refus sémantiques bilingues ; sélections, origines et sorties transactionnelles vérifiées.\n";
    }

    void TesterUtilisationsEspacesSemantiques(
        AnalyseurDeclarationsAutoHeberge syntaxe, AnalyseurSemantiqueAutoHeberge semantique)
    {
        const std::vector<std::string> valides{
            "espace A { publique entier32 F() { retourner 42; } } utilisant espace A; publique entier32 Principal() { retourner F(); }",
            "espace A { structure Point { entier32 X; }; énumération Etat { Actif = 1 }; publique entier32 X = 41; } utilisant espace A; publique entier32 Principal() { Point p = {X}; retourner p.X + convertir<entier32>(Etat::Actif); }",
            "espace A { publique entier32 F(entier32 x) { retourner x; } } espace B { publique entier32 F(entier64 x) { retourner 2; } } utilisant espace A; utilisant espace B; publique entier32 Principal() { retourner F(1) + F(convertir<entier64>(2)); }",
            "espace A { publique entier32 X = 1; } utilisant espace A; espace Local { publique entier32 X = 42; publique entier32 Principal() { retourner X; } }",
            "espace A { publique entier32 F() { retourner 42; } } espace B { utilisant espace A; } utilisant espace B; publique entier32 Principal() { retourner F(); }",
            "espace A {} espace B { utilisant espace A; } espace A { utilisant espace B; publique entier32 F() { retourner 42; } } utilisant espace B; publique entier32 Principal() { retourner F(); }",
            "espace A::Types { structure Point { entier32 X; }; } espace A { utilisant espace Types; publique entier32 Principal() { Point p = {42}; retourner p.X; } }",
            "espace A { structure Point { entier32 X; }; alias Position = Point; } utilisant espace A; alias P = Position; publique entier32 Principal() { P p = {42}; retourner p.X; }",
            "espace A { publique entier32 F() { retourner 1; } } espace B { publique entier32 F() { retourner 42; } } utilisant espace A; utilisant espace B; publique entier32 Principal() { retourner B::F(); }",
            "espace A { publique entier32 F() { retourner 1; } } espace B { publique entier32 F = 2; } utilisant espace A; utilisant espace B; publique entier32 Principal() { entier32 F = 42; retourner F; }",
            "espace A { publique entier32 F(entier32 x) { retourner x; } } utilisant espace A; publique entier32 F(entier64 x) { retourner 2; } publique entier32 Principal() { retourner F(42); }",
            "espace A::B { publique entier32 F() { retourner 42; } } utilisant espace A; publique entier32 Principal() { retourner B :: F(); }",
            "structure P { entier32 X; }; espace A { publique entier32 opérateur+(P p, entier32 x) { retourner p.X + x; } } espace B { publique entier32 opérateur+(P p, entier64 x) { retourner p.X; } } utilisant espace A; utilisant espace B; publique entier32 Principal() { P p = {40}; retourner p + 2; }",
            "structure P { entier32 X; }; espace A { publique booléen opérateur!(P p) { retourner p.X == 0; } } utilisant espace A; publique booléen Principal() { P p = {0}; retourner !p; }",
            "espace A { publique entier32 F(entier32 x) { retourner x; } } utilisant espace A; alias Appeler = F; publique entier32 Principal() { retourner Appeler(42); }",
            "espace A { publique entier32 F() { retourner 42; } } utilisant espace A; pointeur_fonction<entier32()> Rappel = F; publique entier32 Principal() { retourner Rappel(); }",
            "espace A { structure P { entier32 X; }; } utilisant espace A; publique P Creer() { retourner {42}; } publique entier32 Principal() { P p = Creer(); retourner p.X; }",
            "espace A { structure P { entier32 X; }; } utilisant espace A; publique entier32 Lire(P p) { retourner p.X; } publique entier32 Principal(pointeur_fonction<entier32(P)> rappel) { P p = {42}; retourner rappel(p); }",
            "espace A { classe Base { publique: entier32 X; }; } utilisant espace A; classe Derivee : publique Base {}; publique entier32 Principal() { Derivee p; retourner p.X; }",
            "espace A { structure P { entier32 X; }; } espace B { structure P { entier64 X; }; } utilisant espace A; utilisant espace B; publique entier32 Principal() { retourner 42; }",
            "espace Original { publique entier32 F(entier32 x) { retourner x; } } espace A { alias F = Original::F; } espace B { publique entier32 F(entier64 x) { retourner 2; } } utilisant espace A; utilisant espace B; publique entier32 Principal() { retourner F(42) + F(convertir<entier64>(2)); }",
            "espace A { publique entier32 F() { retourner 1; } } espace B { publique entier32 F = 2; } utilisant espace A; utilisant espace B; publique entier32 Principal(pointeur_fonction<entier32()> F) { retourner F(); }",
            "espace A { publique entier32 F() { retourner 42; } } utilisant espace A; utilisant espace A; publique entier32 Principal() { retourner F(); }",
            "espace Types { structure P { entier64 Mauvais; }; } espace A { espace Types { structure P { entier32 X; }; } utilisant espace Types; publique entier32 Principal() { P p = {42}; retourner p.X; } }"
        };
        for (std::size_t index = 0; index < valides.size(); ++index)
            for (const auto& texte : {valides[index], TraduireCorpusConversions(valides[index])})
            {
                const auto resultat = AnalyserSemantiqueValide(syntaxe, semantique, texte, "utilisation-valide-" + std::to_string(index));
                if (index == 2 || index == 20)
                {
                    std::vector<std::uint64_t> espacesChoisis;
                    for (const auto& resolution : resultat.Resolutions)
                        if (resultat.Noeuds[resolution.IndexNoeud].Genre == 24
                            && resultat.Noeuds[resolution.IndexNoeud].HachageNom == HacherTexte("F"))
                            espacesChoisis.push_back(resultat.Symboles[resolution.IndexSymbole].HachageEspace);
                    Exiger(espacesChoisis == std::vector<std::uint64_t>{HacherTexte(index == 2 ? "A" : "Original"), HacherTexte("B")},
                        "les appels n'ont pas sélectionné les deux espaces attendus");
                }
            }
        for (const auto& [source, code] : std::vector<std::pair<std::string, std::uint32_t>>{
            {"utilisant espace Absent; publique entier32 F() { retourner 0; }", 120},
            {"utilisant espace A; espace A {} publique entier32 F() { retourner 0; }", 120},
            {"espace A { structure P {}; } espace B { structure P {}; } utilisant espace A; utilisant espace B; publique entier32 F() { P p; retourner 0; }", 121},
            {"espace A { publique entier32 X = 1; } espace B { publique entier32 X = 2; } utilisant espace A; utilisant espace B; publique entier32 F() { retourner X; }", 121},
            {"espace A { publique entier32 F() { retourner 1; } } espace B { publique entier32 F = 2; } utilisant espace A; utilisant espace B; publique entier32 Principal() { retourner F(); }", 121},
            {"espace A { publique entier32 F(entier32 x) { retourner x; } } espace B { publique entier32 F(entier32 x) { retourner x; } } utilisant espace A; utilisant espace B; publique entier32 Principal() { retourner F(1); }", 22},
            {"espace A { publique entier32 F() { retourner 1; } } publique entier32 Principal() { retourner F(); } utilisant espace A;", 18},
            {"espace A { publique entier32 F() { retourner 1; } } espace Local { utilisant espace A; } publique entier32 Principal() { retourner F(); }", 18},
            {"espace A { publique entier32 X = 1; } utilisant espace A; publique entier32 X = 2; publique entier32 Principal() { retourner X; }", 121},
            {"espace A { structure P {}; } espace B { structure P {}; } utilisant espace A; utilisant espace B; alias C = P; publique vide F() {}", 121},
            {"espace A { publique entier32 F(entier32 x) { retourner x; } } espace B { publique entier32 F(entier64 x) { retourner 2; } } utilisant espace A; utilisant espace B; alias C = F; publique vide G() {}", 112},
            {"espace A { publique entier32 F(entier32 x) { retourner x; } } espace B { publique entier32 F(entier64 x) { retourner 2; } } utilisant espace A; utilisant espace B; publique vide G() { F; }", 19},
            {"structure P {}; espace A { publique entier32 opérateur+(P p, entier32 x) { retourner x; } } espace B { publique entier32 opérateur+(P p, entier32 x) { retourner x; } } utilisant espace A; utilisant espace B; publique entier32 G() { P p; retourner p + 1; }", 22},
            {"espace A { structure P {}; } espace B { structure P {}; } utilisant espace A; utilisant espace B; publique vide G() { convertir<P*>(0); }", 121}
        })
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
                ComparerErreurSemantique(syntaxe, semantique, texte, code, "utilisation-invalide");
        for (const auto& [source, code] : std::vector<std::pair<std::string, std::uint32_t>>{
            {"espace A {} utilisant A;", 30}, {"espace A {} utilisant espace A", 11},
            {"espace A {} utilisant espace ;", 5}})
            for (const auto& texte : {source, TraduireCorpusConversions(source)})
                ComparerErreurDeclarations(syntaxe, texte, code, "syntaxe-utilisation-invalide");
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
        const auto exportInterface = imageSyntaxe.ChercherExport(
            "GalacticShrine::GsPP::Autohebergement::AnalyserDeclarationsInterface");
        const auto aliasInterface = imageSyntaxe.ChercherExport(
            "GalacticShrine::GsPP::Autohebergement::AnalyzeInterfaceDeclarations");
        Exiger(exportInterface.has_value() && exportInterface == aliasInterface,
            "exports français et anglais de l'analyse d'interface absents ou différents");
        const auto interface = reinterpret_cast<AnalyseurDeclarationsAutoHeberge>(*exportInterface);

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
        TesterInterfacesEnMemoire(syntaxe, interface, semantique);
        const auto exportAssemblage = imageSyntaxe.ChercherExport(
            "GalacticShrine::GsPP::Autohebergement::AssemblerDeclarationsPreparees");
        const auto aliasAssemblage = imageSyntaxe.ChercherExport(
            "GalacticShrine::GsPP::Autohebergement::AssemblePreparedDeclarations");
        Exiger(exportAssemblage.has_value() && exportAssemblage == aliasAssemblage,
            "exports français et anglais de l'assemblage absents ou différents");
        const auto exportSemantiqueUnites = image.ChercherExport(
            "GalacticShrine::GsPP::Autohebergement::AnalyserSemantiqueUnites");
        const auto aliasSemantiqueUnites = image.ChercherExport(
            "GalacticShrine::GsPP::Autohebergement::AnalyzeUnitSemantics");
        Exiger(exportSemantiqueUnites.has_value() && exportSemantiqueUnites == aliasSemantiqueUnites,
            "exports français et anglais de la sémantique par unité absents ou différents");
        TesterAssemblageDeclarationsPreparees(reinterpret_cast<AssembleurDeclarationsAutoHeberge>(*exportAssemblage), syntaxe, interface,
            reinterpret_cast<AnalyseurSemantiqueUnitesAutoHeberge>(*exportSemantiqueUnites));
        const auto exportNormalisation = imageSyntaxe.ChercherExport(
            "GalacticShrine::GsPP::Autohebergement::AssemblerDeclarationsNormalisees");
        const auto aliasNormalisation = imageSyntaxe.ChercherExport(
            "GalacticShrine::GsPP::Autohebergement::AssembleNormalizedDeclarations");
        Exiger(exportNormalisation.has_value() && exportNormalisation == aliasNormalisation,
            "exports français et anglais de la normalisation absents ou différents");
        TesterNormalisationDeclarationsPreparees(reinterpret_cast<AssembleurDeclarationsAutoHeberge>(*exportNormalisation), syntaxe, interface,
            reinterpret_cast<AnalyseurSemantiqueUnitesAutoHeberge>(*exportSemantiqueUnites));
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
        TesterPrioritesInitialiseursLocauxSemantiques(syntaxe, semantique);
        TesterPrioritesInitialiseursGlobauxSemantiques(syntaxe, semantique);
        TesterDeclarationsLocalesContextuellesSemantiques(syntaxe, semantique);
        TesterPorteesLocalesBackend(syntaxe, semantique);
        TesterNomsDansEspacesParents(syntaxe, semantique);
        TesterNomsDansMethodesSemantiques(syntaxe, semantique);
        TesterOperateursDansMethodesSemantiques(syntaxe, semantique);
        TesterQualificationsConstructionsSemantiques(syntaxe, semantique);
        TesterCallbacksChampsContextuelsSemantiques(syntaxe, semantique);
        TesterRetoursReferencesCallbacks(syntaxe, semantique);
        TesterRetoursReferencesPointeursCallbacks(syntaxe, semantique);
        TesterReferencesCallbacksParametres(syntaxe, semantique);
        TesterArgumentsReferencesCallbacksImbriques(syntaxe, semantique);
        TesterReferencesStructuresPointeursCallbacksImbriques(syntaxe, semantique);
        TesterReferencesGroupesMixtesConstructions(syntaxe, semantique);
        TesterReferencesOperateursMixtesConstructions(syntaxe, semantique);
        TesterOperateursInitialiseursAgreges(syntaxe, semantique);
        TesterPrioritesPlansConstructeursSemantiques(syntaxe, semantique);
        TesterConstructionsLocalesContextuellesSemantiques(syntaxe, semantique);
        TesterChampsParDefautContextuelsSemantiques(syntaxe, semantique);
        TesterUtilisationsEspacesSemantiques(syntaxe, semantique);
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

        const std::array<std::string_view, 85> mots{{
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
            "publiqueX", "étoile", "utilisant", "using"
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
            << "Frontend.GsE unique, 85 classifications, lexeur différentiel, "
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
