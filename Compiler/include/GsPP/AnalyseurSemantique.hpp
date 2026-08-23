#pragma once

#include "GsPP/Ast.hpp"

#include <string>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace GsPP
{
    class AnalyseurSemantique final
    {
    public:
        void Analyser(Programme& programme);

        [[nodiscard]] static std::uint32_t TailleType(
            const TypeGs& type,
            const std::unordered_map<std::string, Structure*>& structures);
        [[nodiscard]] static std::uint32_t AlignementType(
            const TypeGs& type,
            const std::unordered_map<std::string, Structure*>& structures);

    private:
        void ResoudreType(TypeGs& type, const std::string& espace, const PositionSource& position);
        void ResoudreHeritage(Structure& structure);
        void ResoudreAlias(DeclarationAlias& alias);
        void CalculerEnumeration(Enumeration& enumeration);
        void CalculerStructure(Structure& structure);
        void PreparerFonction(Fonction& fonction);
        void IndexerFonctions(Programme& programme);
        void AnalyserFonction(Fonction& fonction);
        void AnalyserVariableGlobale(VariableGlobale& variable);
        void AnalyserInstruction(Instruction& instruction, Fonction& fonction);
        TypeGs AnalyserExpression(Expression& expression, Fonction& fonction);
        Fonction* ResoudreSurcharge(
            const std::string& nom,
            std::vector<Expression*>& arguments,
            Fonction& contexte,
            const PositionSource& position);
        void PreparerAppel(
            ExpressionAppel& appel,
            Fonction& cible,
            std::vector<Expression*>& arguments,
            Fonction& contexte);
        [[nodiscard]] std::string QualifierNomFonction(
            const std::string& nom,
            const Fonction& contexte) const;
        [[nodiscard]] std::string TrouverNomMethode(
            const std::string& classe,
            const std::string& nom) const;
        [[nodiscard]] const ChampStructure* TrouverChamp(
            const Structure& structure,
            const std::string& nom,
            const Structure*& proprietaire) const;
        [[nodiscard]] bool EstDeriveDe(
            const std::string& classe,
            const std::string& base) const;
        [[nodiscard]] bool ConversionHeritageAutorisee(
            const TypeGs& source,
            const TypeGs& destination,
            bool liaisonReference = false) const;
        [[nodiscard]] bool AccesMembreAutorise(
            VisibiliteMembre visibilite,
            const std::string& classe,
            const Fonction& contexte) const;
        [[nodiscard]] bool AccesDepuisClasseAutorise(
            VisibiliteMembre visibilite,
            const std::string& classe,
            const std::string& classeContexte) const;
        Fonction* ResoudreConstructeurClasse(
            const Structure& classe,
            std::vector<Expression*>& argumentsUtilisateur,
            Fonction& fonction,
            const PositionSource& position,
            const std::string& classeContexte,
            std::vector<bool>& argumentsParReference,
            bool estBase = false);
        void PlanifierConstructionImplicite(
            const Structure& classe,
            std::uint32_t decalage,
            const std::string& classeContexte,
            Fonction& fonction,
            const PositionSource& position,
            std::vector<EtapeConstructionClasse>& etapes);
        void PlanifierConstructionTypeObjet(
            const TypeGs& type,
            std::uint32_t decalage,
            const std::string& classeContexte,
            Fonction& fonction,
            const PositionSource& position,
            std::vector<EtapeConstructionClasse>& etapes);
        void PlanifierConstructionTableauAvecConstructeur(
            const TypeGs& type,
            std::uint32_t decalage,
            const std::string& symboleConstructeur,
            const std::string& classeRecepteur,
            std::vector<EtapeConstructionClasse>& etapes);
        void PlanifierDestructionClasse(
            const Structure& classe,
            std::uint32_t decalage,
            const std::string& classeContexte,
            const PositionSource& position,
            std::vector<ActionDestructionClasse>& actions);
        void PlanifierDestructionTypeObjet(
            const TypeGs& type,
            std::uint32_t decalage,
            const std::string& classeContexte,
            const PositionSource& position,
            std::vector<ActionDestructionClasse>& actions);
        TypeGs AnalyserInitialiseur(
            Expression& expression,
            const TypeGs& typeCible,
            Fonction& fonction);
        void EcrireInitialiseurGlobal(
            const Expression& expression,
            const TypeGs& typeCible,
            std::uint32_t decalage,
            VariableGlobale& variable);
        [[nodiscard]] bool AdapterConstante(Expression& expression, const TypeGs& cible);
        [[nodiscard]] bool EstExpressionConstante(const Expression& expression) const;
        [[nodiscard]] bool EstValeurGauche(const Expression& expression) const;
        [[nodiscard]] std::uint64_t EvaluerConstante(const Expression& expression) const;
        [[noreturn]] void Erreur(
            const PositionSource& position,
            std::string francais,
            std::string anglais) const;

        Programme* _Programme = nullptr;
        std::unordered_map<std::string, Structure*> _Structures;
        std::unordered_map<std::string, Enumeration*> _Enumerations;
        struct ValeurEnumeration
        {
            TypeGs Type;
            std::int64_t Valeur = 0;
        };
        std::unordered_map<std::string, ValeurEnumeration> _ValeursEnumerations;
        std::unordered_map<std::string, Fonction*> _Fonctions;
        std::unordered_map<std::string, std::vector<Fonction*>> _Surcharges;
        std::unordered_map<std::string, VariableGlobale*> _Globales;
        std::unordered_map<std::string, DeclarationAlias*> _Aliases;
        std::unordered_map<std::string, TypeGs> _Variables;
        std::unordered_set<std::string> _StructuresEnCours;
        std::unordered_set<std::string> _StructuresCalculees;
        std::unordered_set<std::string> _AliasesEnCours;
        std::unordered_set<std::string> _AliasesResolus;
    };
}
