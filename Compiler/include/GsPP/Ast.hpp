#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace GsPP
{
    struct PositionSource
    {
        std::string Fichier;
        std::size_t Ligne = 1;
        std::size_t Colonne = 1;
    };

    enum class GenreType
    {
        Entier8,
        Entier16,
        Entier32,
        Entier64,
        Naturel8,
        Naturel16,
        Naturel32,
        Naturel64,
        Booleen,
        Octet,
        Caractere,
        Vide,
        Structure,
        Enumeration,
        PointeurFonction,
        Inconnu
    };

    struct TypeGs
    {
        TypeGs() = default;
        TypeGs(GenreType genre, std::string nom = {}, std::uint32_t niveauPointeur = 0)
            : Genre(genre), Nom(std::move(nom)), NiveauPointeur(niveauPointeur) {}

        GenreType Genre = GenreType::Inconnu;
        std::string Nom;
        std::uint32_t NiveauPointeur = 0;
        std::vector<std::uint32_t> DimensionsTableau;
        std::shared_ptr<TypeGs> RetourFonction;
        std::vector<TypeGs> ParametresFonction;
        bool EstConstante = false;
        bool EstVolatile = false;
        bool EstReference = false;

        [[nodiscard]] bool EstPointeur() const noexcept
        {
            return NiveauPointeur > 0 && DimensionsTableau.empty() && !EstReference;
        }
        [[nodiscard]] bool EstTableau() const noexcept
        {
            return !DimensionsTableau.empty();
        }
        [[nodiscard]] bool EstPointeurFonction() const noexcept
        {
            return Genre == GenreType::PointeurFonction
                && NiveauPointeur == 0
                && DimensionsTableau.empty()
                && !EstReference;
        }
        [[nodiscard]] bool EstAdresse() const noexcept
        {
            return EstPointeur() || EstPointeurFonction();
        }
        [[nodiscard]] bool EstEntier() const noexcept
        {
            if (NiveauPointeur != 0 || EstTableau() || EstReference) return false;
            return Genre == GenreType::Entier8
                || Genre == GenreType::Entier16
                || Genre == GenreType::Entier32
                || Genre == GenreType::Entier64
                || Genre == GenreType::Naturel8
                || Genre == GenreType::Naturel16
                || Genre == GenreType::Naturel32
                || Genre == GenreType::Naturel64
                || Genre == GenreType::Octet
                || Genre == GenreType::Caractere;
        }
        [[nodiscard]] bool EstEntierSigne() const noexcept
        {
            return EstEntier()
                && (Genre == GenreType::Entier8
                    || Genre == GenreType::Entier16
                    || Genre == GenreType::Entier32
                    || Genre == GenreType::Entier64
                    || Genre == GenreType::Caractere);
        }
        [[nodiscard]] bool EstEntierNonSigne() const noexcept
        {
            return EstEntier() && !EstEntierSigne();
        }
        [[nodiscard]] bool EstBooleen() const noexcept
        {
            return NiveauPointeur == 0 && !EstTableau() && !EstReference
                && Genre == GenreType::Booleen;
        }
        [[nodiscard]] bool EstEnumeration() const noexcept
        {
            return NiveauPointeur == 0 && !EstTableau() && !EstReference
                && Genre == GenreType::Enumeration;
        }
        [[nodiscard]] bool EstScalaire() const noexcept
        {
            return EstAdresse() || EstEntier() || EstBooleen() || EstEnumeration();
        }
        [[nodiscard]] bool EstVide() const noexcept
        {
            return NiveauPointeur == 0 && !EstTableau() && !EstReference
                && Genre == GenreType::Vide;
        }
        [[nodiscard]] bool EstStructure() const noexcept
        {
            return NiveauPointeur == 0 && !EstTableau() && !EstReference
                && Genre == GenreType::Structure;
        }
        [[nodiscard]] TypeGs TypeReference() const
        {
            TypeGs type = *this;
            type.EstReference = false;
            return type;
        }
        [[nodiscard]] TypeGs ElementTableau() const
        {
            TypeGs element = *this;
            if (!element.DimensionsTableau.empty())
                element.DimensionsTableau.erase(element.DimensionsTableau.begin());
            return element;
        }
        [[nodiscard]] std::string Afficher() const
        {
            std::string valeur;
            if (EstConstante) valeur += "constante ";
            if (EstVolatile) valeur += "volatile ";
            if (Genre == GenreType::Entier8) valeur += "entier8";
            else if (Genre == GenreType::Entier16) valeur += "entier16";
            else if (Genre == GenreType::Entier32) valeur += "entier32";
            else if (Genre == GenreType::Entier64) valeur += "entier64";
            else if (Genre == GenreType::Naturel8) valeur += "naturel8";
            else if (Genre == GenreType::Naturel16) valeur += "naturel16";
            else if (Genre == GenreType::Naturel32) valeur += "naturel32";
            else if (Genre == GenreType::Naturel64) valeur += "naturel64";
            else if (Genre == GenreType::Booleen) valeur += "booléen";
            else if (Genre == GenreType::Octet) valeur += "octet";
            else if (Genre == GenreType::Caractere) valeur += "caractère";
            else if (Genre == GenreType::Vide) valeur += "vide";
            else if (Genre == GenreType::Structure || Genre == GenreType::Enumeration) valeur += Nom;
            else if (Genre == GenreType::PointeurFonction)
            {
                valeur += "pointeur_fonction<";
                valeur += RetourFonction ? RetourFonction->Afficher() : "<inconnu>";
                valeur += '(';
                for (std::size_t index = 0; index < ParametresFonction.size(); ++index)
                {
                    if (index != 0) valeur += ", ";
                    valeur += ParametresFonction[index].Afficher();
                }
                valeur += ")>";
            }
            else valeur += "<inconnu>";
            valeur.append(NiveauPointeur, '*');
            if (EstReference) valeur += '&';
            for (const auto dimension : DimensionsTableau)
                valeur += '[' + std::to_string(dimension) + ']';
            return valeur;
        }
        [[nodiscard]] bool operator==(const TypeGs& autre) const noexcept
        {
            if (Genre != autre.Genre
                || NiveauPointeur != autre.NiveauPointeur
                || DimensionsTableau != autre.DimensionsTableau
                || EstConstante != autre.EstConstante
                || EstVolatile != autre.EstVolatile
                || EstReference != autre.EstReference
                || ((Genre == GenreType::Structure || Genre == GenreType::Enumeration)
                    && Nom != autre.Nom))
                return false;
            if (Genre == GenreType::PointeurFonction)
            {
                if (static_cast<bool>(RetourFonction)
                    != static_cast<bool>(autre.RetourFonction))
                    return false;
                if (RetourFonction && !(*RetourFonction == *autre.RetourFonction))
                    return false;
                if (ParametresFonction != autre.ParametresFonction) return false;
            }
            return true;
        }
    };

    enum class GenreExpression
    {
        Entier,
        Chaine,
        Variable,
        Unaire,
        Binaire,
        Affectation,
        Appel,
        Membre,
        Index,
        Conversion,
        Agregat
    };

    struct Expression
    {
        Expression(GenreExpression genre, PositionSource position)
            : Genre(genre), Position(std::move(position)) {}
        virtual ~Expression() = default;
        GenreExpression Genre;
        PositionSource Position;
        TypeGs TypeSemantique;
        bool EstValeurConstante = false;
    };

    struct ExpressionEntier final : Expression
    {
        ExpressionEntier(std::uint64_t valeur, PositionSource position, bool estBooleen = false)
            : Expression(GenreExpression::Entier, std::move(position)),
              Valeur(valeur), EstLitteralBooleen(estBooleen) {}
        std::uint64_t Valeur;
        bool EstLitteralBooleen = false;
    };

    struct ExpressionChaine final : Expression
    {
        ExpressionChaine(std::string valeur, PositionSource position)
            : Expression(GenreExpression::Chaine, std::move(position)),
              Valeur(std::move(valeur)) {}
        std::string Valeur;
    };

    struct ExpressionVariable final : Expression
    {
        ExpressionVariable(std::string nom, PositionSource position)
            : Expression(GenreExpression::Variable, std::move(position)), Nom(std::move(nom)) {}
        std::string Nom;
        bool EstGlobale = false;
        bool EstFonction = false;
        bool EstReference = false;
        bool EstBase = false;
        bool EstConstanteEnumeration = false;
        std::int64_t ValeurEnumeration = 0;
    };

    struct ExpressionUnaire final : Expression
    {
        ExpressionUnaire(
            std::string operateur,
            std::unique_ptr<Expression> operande,
            PositionSource position)
            : Expression(GenreExpression::Unaire, std::move(position)),
              Operateur(std::move(operateur)),
              Operande(std::move(operande)) {}
        std::string Operateur;
        std::unique_ptr<Expression> Operande;
        std::string NomSurcharge;
        bool OperandeParReference = false;
    };

    struct ExpressionBinaire final : Expression
    {
        ExpressionBinaire(
            std::string operateur,
            std::unique_ptr<Expression> gauche,
            std::unique_ptr<Expression> droite,
            PositionSource position)
            : Expression(GenreExpression::Binaire, std::move(position)),
              Operateur(std::move(operateur)),
              Gauche(std::move(gauche)),
              Droite(std::move(droite)) {}
        std::string Operateur;
        std::unique_ptr<Expression> Gauche;
        std::unique_ptr<Expression> Droite;
        std::string NomSurcharge;
        bool GaucheParReference = false;
        bool DroiteParReference = false;
    };

    struct ExpressionAffectation final : Expression
    {
        ExpressionAffectation(
            std::unique_ptr<Expression> cible,
            std::unique_ptr<Expression> valeur,
            PositionSource position)
            : Expression(GenreExpression::Affectation, std::move(position)),
              Cible(std::move(cible)),
              Valeur(std::move(valeur)) {}
        std::unique_ptr<Expression> Cible;
        std::unique_ptr<Expression> Valeur;
    };

    struct ExpressionAppel final : Expression
    {
        ExpressionAppel(
            std::unique_ptr<Expression> cible,
            std::vector<std::unique_ptr<Expression>> arguments,
            PositionSource position)
            : Expression(GenreExpression::Appel, std::move(position)),
              Cible(std::move(cible)),
              Arguments(std::move(arguments)) {}
        std::unique_ptr<Expression> Cible;
        std::vector<std::unique_ptr<Expression>> Arguments;
        std::string NomDirect;
        bool EstIndirect = false;
        bool EstIntrinseque = false;
        bool EstVirtuel = false;
        std::uint32_t IndexVirtuel = 0;
        std::uint32_t DecalageTableVirtuelle = 0;
        bool RetourneReference = false;
        bool ForcerAppelDirect = false;
        std::vector<bool> ArgumentsParReference;
    };

    struct ExpressionMembre final : Expression
    {
        ExpressionMembre(
            std::unique_ptr<Expression> objet,
            std::string membre,
            bool viaPointeur,
            PositionSource position)
            : Expression(GenreExpression::Membre, std::move(position)),
              Objet(std::move(objet)),
              Membre(std::move(membre)),
              ViaPointeur(viaPointeur) {}
        std::unique_ptr<Expression> Objet;
        std::string Membre;
        bool ViaPointeur;
        std::uint32_t DecalageMembre = 0;
    };

    struct ExpressionIndex final : Expression
    {
        ExpressionIndex(
            std::unique_ptr<Expression> objet,
            std::unique_ptr<Expression> indice,
            PositionSource position)
            : Expression(GenreExpression::Index, std::move(position)),
              Objet(std::move(objet)),
              Indice(std::move(indice)) {}
        std::unique_ptr<Expression> Objet;
        std::unique_ptr<Expression> Indice;
        std::uint32_t TailleElement = 0;
    };

    struct ExpressionConversion final : Expression
    {
        ExpressionConversion(
            TypeGs typeCible,
            std::unique_ptr<Expression> valeur,
            PositionSource position)
            : Expression(GenreExpression::Conversion, std::move(position)),
              TypeCible(std::move(typeCible)),
              Valeur(std::move(valeur)) {}
        TypeGs TypeCible;
        std::unique_ptr<Expression> Valeur;
    };

    struct ExpressionAgregat final : Expression
    {
        ExpressionAgregat(
            std::vector<std::unique_ptr<Expression>> elements,
            PositionSource position)
            : Expression(GenreExpression::Agregat, std::move(position)),
              Elements(std::move(elements)) {}
        std::vector<std::unique_ptr<Expression>> Elements;
    };

    enum class GenreInstruction
    {
        Bloc,
        Retour,
        Expression,
        Variable,
        Si,
        TantQue
    };

    struct Instruction
    {
        Instruction(GenreInstruction genre, PositionSource position)
            : Genre(genre), Position(std::move(position)) {}
        virtual ~Instruction() = default;
        GenreInstruction Genre;
        PositionSource Position;
    };

    struct InstructionBloc final : Instruction
    {
        explicit InstructionBloc(PositionSource position)
            : Instruction(GenreInstruction::Bloc, std::move(position)) {}
        std::vector<std::unique_ptr<Instruction>> Instructions;
    };

    struct InstructionRetour final : Instruction
    {
        InstructionRetour(std::unique_ptr<Expression> valeur, PositionSource position)
            : Instruction(GenreInstruction::Retour, std::move(position)), Valeur(std::move(valeur)) {}
        std::unique_ptr<Expression> Valeur;
    };

    struct InstructionExpression final : Instruction
    {
        InstructionExpression(std::unique_ptr<Expression> expression, PositionSource position)
            : Instruction(GenreInstruction::Expression, std::move(position)), Valeur(std::move(expression)) {}
        std::unique_ptr<Expression> Valeur;
    };

    struct EtapeConstructionClasse
    {
        std::uint32_t Decalage = 0;
        std::string SymboleConstructeur;
        std::string ClasseTableVirtuelle;
        std::string ClasseRecepteur;
    };

    struct ActionDestructionClasse
    {
        std::uint32_t Decalage = 0;
        std::string SymboleDestructeur;
        std::string ClasseRecepteur;
    };

    struct InstructionVariable final : Instruction
    {
        InstructionVariable(
            TypeGs type,
            std::string nom,
            std::unique_ptr<Expression> initialiseur,
            PositionSource position)
            : Instruction(GenreInstruction::Variable, std::move(position)),
              Type(std::move(type)),
              Nom(std::move(nom)),
              Initialiseur(std::move(initialiseur)) {}
        TypeGs Type;
        std::string Nom;
        std::unique_ptr<Expression> Initialiseur;
        std::vector<std::unique_ptr<Expression>> ArgumentsConstruction;
        std::vector<bool> ArgumentsConstructionParReference;
        bool ConstructionExplicite = false;
        std::vector<std::string> ClassesConstructeursBases;
        std::vector<std::string> SymbolesConstructeursBases;
        std::vector<EtapeConstructionClasse> EtapesConstructionImplicite;
        std::string SymboleConstructeur;
        std::string SymboleDestructeur;
        std::vector<std::string> SymbolesDestructeurs;
        std::vector<ActionDestructionClasse> ActionsDestruction;
        bool InitialiserTableVirtuelle = false;
    };

    struct InstructionSi final : Instruction
    {
        InstructionSi(
            std::unique_ptr<Expression> condition,
            std::unique_ptr<Instruction> alors,
            std::unique_ptr<Instruction> sinon,
            PositionSource position)
            : Instruction(GenreInstruction::Si, std::move(position)),
              Condition(std::move(condition)),
              Alors(std::move(alors)),
              Sinon(std::move(sinon)) {}
        std::unique_ptr<Expression> Condition;
        std::unique_ptr<Instruction> Alors;
        std::unique_ptr<Instruction> Sinon;
    };

    struct InstructionTantQue final : Instruction
    {
        InstructionTantQue(
            std::unique_ptr<Expression> condition,
            std::unique_ptr<Instruction> corps,
            PositionSource position)
            : Instruction(GenreInstruction::TantQue, std::move(position)),
              Condition(std::move(condition)),
              Corps(std::move(corps)) {}
        std::unique_ptr<Expression> Condition;
        std::unique_ptr<Instruction> Corps;
    };

    struct Parametre
    {
        TypeGs Type;
        std::string Nom;
        PositionSource Position;
    };

    enum class VisibiliteMembre
    {
        Publique,
        Protegee,
        Privee
    };

    struct ChampStructure
    {
        TypeGs Type;
        std::string Nom;
        PositionSource Position;
        std::uint32_t Decalage = 0;
        VisibiliteMembre Visibilite = VisibiliteMembre::Publique;
        std::unique_ptr<Expression> InitialiseurParDefaut;
    };

    struct AliasChamp
    {
        std::string Nom;
        std::string Cible;
        std::string CibleCanonique;
        PositionSource Position;
    };

    struct Structure
    {
        std::string Nom;
        std::string Espace;
        PositionSource Position;
        std::vector<ChampStructure> Champs;
        std::vector<AliasChamp> AliasesChamps;
        std::uint32_t Taille = 0;
        std::uint32_t Alignement = 1;
        bool EstUnion = false;
        bool EstClasse = false;
        std::string ClasseBase;
        std::string ClasseBaseCanonique;
        VisibiliteMembre VisibiliteHeritage = VisibiliteMembre::Publique;
        bool EstPolymorphe = false;
        std::uint32_t DecalageTableVirtuelle = 0;
        std::string SymboleTableVirtuelle;
        std::vector<std::string> ClesMethodesVirtuelles;
        std::vector<std::string> SymbolesTableVirtuelle;
        std::vector<std::string> MethodesVirtuellesAbi;

        [[nodiscard]] std::string NomComplet() const
        {
            return Espace.empty() ? Nom : Espace + "::" + Nom;
        }
    };

    struct Enumerateur
    {
        std::string Nom;
        PositionSource Position;
        std::unique_ptr<Expression> Initialiseur;
        std::int64_t Valeur = 0;
    };

    struct Enumeration
    {
        std::string Nom;
        std::string Espace;
        PositionSource Position;
        std::vector<Enumerateur> Valeurs;

        [[nodiscard]] std::string NomComplet() const
        {
            return Espace.empty() ? Nom : Espace + "::" + Nom;
        }
    };

    struct InitialiseurChampConstructeur
    {
        std::string Nom;
        std::string NomCanonique;
        PositionSource Position;
        std::vector<std::unique_ptr<Expression>> Arguments;
        TypeGs Type;
        std::uint32_t Decalage = 0;
        bool EstObjetClasse = false;
        bool EstImplicite = false;
        Expression* InitialiseurParDefaut = nullptr;
        std::string SymboleConstructeur;
        std::vector<bool> ArgumentsConstructeurParReference;
        std::vector<EtapeConstructionClasse> EtapesConstructionImplicite;
    };

    struct Fonction
    {
        std::string Nom;
        std::string Espace;
        PositionSource Position;
        TypeGs TypeRetour;
        bool EstPublique = false;
        bool EstExterne = false;
        std::string NomSource;
        std::string NomLien;
        std::string ClasseProprietaire;
        VisibiliteMembre Visibilite = VisibiliteMembre::Publique;
        bool EstMethode = false;
        bool EstConstructeur = false;
        bool EstDestructeur = false;
        bool EstOperateur = false;
        std::string Operateur;
        bool EstVirtuelle = false;
        bool EstRemplacement = false;
        std::uint32_t IndexVirtuel = 0;
        bool InitialiseurBaseExplicite = false;
        bool DelegueConstructeur = false;
        std::vector<std::unique_ptr<Expression>> ArgumentsConstructeurDelegue;
        std::vector<bool> ArgumentsConstructeurDelegueParReference;
        std::string SymboleConstructeurDelegue;
        std::vector<std::unique_ptr<Expression>> ArgumentsConstructeurBase;
        std::vector<bool> ArgumentsConstructeurBaseParReference;
        std::vector<std::string> ClassesBasesSansConstructeur;
        std::string SymboleConstructeurBase;
        std::vector<EtapeConstructionClasse> EtapesConstructionBaseImplicite;
        std::vector<InitialiseurChampConstructeur> InitialiseursChamps;
        std::vector<Parametre> Parametres;
        std::unique_ptr<InstructionBloc> Corps;

        [[nodiscard]] std::string NomComplet() const
        {
            const auto& nom = NomLien.empty() ? Nom : NomLien;
            return Espace.empty() ? nom : Espace + "::" + nom;
        }
        [[nodiscard]] std::string NomSourceComplet() const
        {
            const auto& nom = NomSource.empty() ? Nom : NomSource;
            return Espace.empty() ? nom : Espace + "::" + nom;
        }
    };

    struct VariableGlobale
    {
        struct RelocalisationInitialiseur
        {
            std::uint32_t Decalage = 0;
            std::string Symbole;
        };

        std::string Nom;
        std::string Espace;
        PositionSource Position;
        TypeGs Type;
        bool EstPublique = false;
        bool EstExterne = false;
        std::unique_ptr<Expression> Initialiseur;
        bool EstInitialisee = false;
        std::uint64_t ValeurInitiale = 0;
        std::string SymboleInitialiseur;
        std::vector<std::uint8_t> DonneesInitiales;
        std::vector<RelocalisationInitialiseur> RelocalisationsInitialiseur;

        [[nodiscard]] std::string NomComplet() const
        {
            return Espace.empty() ? Nom : Espace + "::" + Nom;
        }
    };

    enum class GenreCibleAlias
    {
        Inconnue,
        Structure,
        Fonction,
        VariableGlobale
    };

    struct DeclarationAlias
    {
        std::string Nom;
        std::string Cible;
        std::string Espace;
        PositionSource Position;
        std::string CibleCanonique;
        GenreCibleAlias GenreCible = GenreCibleAlias::Inconnue;

        [[nodiscard]] std::string NomComplet() const
        {
            return Espace.empty() ? Nom : Espace + "::" + Nom;
        }
    };

    struct Programme
    {
        std::vector<Structure> Structures;
        std::vector<Enumeration> Enumerations;
        std::vector<VariableGlobale> VariablesGlobales;
        std::vector<Fonction> Fonctions;
        std::vector<DeclarationAlias> Aliases;
    };
}
