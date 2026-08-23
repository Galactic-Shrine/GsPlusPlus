#pragma once

#include "GsPP/Ast.hpp"
#include "GsPP/Jeton.hpp"

#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

namespace GsPP
{
    class AnalyseurSyntaxique final
    {
    public:
        explicit AnalyseurSyntaxique(
            std::vector<Jeton> jetons,
            std::string fichier = {},
            bool estInterface = false);

        [[nodiscard]] Programme Analyser();

    private:
        void AnalyserDeclarations(Programme& programme, const std::string& espaceCourant);
        void AnalyserEspace(Programme& programme, const std::string& espaceParent);
        [[nodiscard]] Structure AnalyserStructure(
            Programme& programme,
            const std::string& espaceCourant,
            bool estUnion = false,
            bool estClasse = false);
        void AnalyserMembreClasse(
            Programme& programme,
            Structure& structure,
            VisibiliteMembre visibilite);
        [[nodiscard]] Enumeration AnalyserEnumeration(const std::string& espaceCourant);
        [[nodiscard]] DeclarationAlias AnalyserAlias(const std::string& espaceCourant);
        void AnalyserFonctionOuGlobale(Programme& programme, const std::string& espaceCourant);
        [[nodiscard]] Fonction TerminerFonction(
            std::string nom,
            TypeGs typeRetour,
            bool estPublique,
            bool estExterne,
            PositionSource position,
            const std::string& espaceCourant,
            bool autoriserListeInitialisation = false);
        [[nodiscard]] TypeGs AnalyserType();
        void AnalyserDimensionsTableau(TypeGs& type);
        [[nodiscard]] std::unique_ptr<InstructionBloc> AnalyserBloc();
        [[nodiscard]] std::unique_ptr<Instruction> AnalyserInstruction();
        [[nodiscard]] std::unique_ptr<Expression> AnalyserExpression();
        [[nodiscard]] std::unique_ptr<Expression> AnalyserAffectation();
        [[nodiscard]] std::unique_ptr<Expression> AnalyserOuLogique();
        [[nodiscard]] std::unique_ptr<Expression> AnalyserEtLogique();
        [[nodiscard]] std::unique_ptr<Expression> AnalyserOuBinaire();
        [[nodiscard]] std::unique_ptr<Expression> AnalyserXorBinaire();
        [[nodiscard]] std::unique_ptr<Expression> AnalyserEtBinaire();
        [[nodiscard]] std::unique_ptr<Expression> AnalyserEgalite();
        [[nodiscard]] std::unique_ptr<Expression> AnalyserComparaison();
        [[nodiscard]] std::unique_ptr<Expression> AnalyserDecalage();
        [[nodiscard]] std::unique_ptr<Expression> AnalyserAddition();
        [[nodiscard]] std::unique_ptr<Expression> AnalyserMultiplication();
        [[nodiscard]] std::unique_ptr<Expression> AnalyserUnaire();
        [[nodiscard]] std::unique_ptr<Expression> AnalyserPostfixe();
        [[nodiscard]] std::unique_ptr<Expression> AnalyserPrimaire();
        [[nodiscard]] std::string AnalyserNomQualifie();
        [[nodiscard]] std::string AnalyserNomFonction();
        [[nodiscard]] bool DebuteDeclarationVariable() const;
        [[nodiscard]] bool DebuteType() const;
        [[nodiscard]] PositionSource Position(const Jeton& jeton) const;

        [[nodiscard]] const Jeton& Courant() const;
        [[nodiscard]] const Jeton& Precedent() const;
        [[nodiscard]] bool Est(GenreJeton genre) const;
        bool Accepter(GenreJeton genre);
        bool AccepterUn(std::initializer_list<GenreJeton> genres);
        const Jeton& Exiger(GenreJeton genre, const char* fr, const char* en);
        void ExigerFermetureType(const char* fr, const char* en);

        std::vector<Jeton> _Jetons;
        std::size_t _Position = 0;
        std::string _Fichier;
        bool _EstInterface = false;
    };
}
