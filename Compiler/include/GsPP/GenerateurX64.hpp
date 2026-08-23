#pragma once

#include "GsPP/Ast.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace GsPP
{
    enum class SectionMachine : std::uint8_t
    {
        Texte,
        Donnees,
        Zero,
        Indefinie
    };

    enum class TypeRelocalisationMachine : std::uint8_t
    {
        Relatif32,
        Adresse64
    };

    enum class GenreSymboleMachine : std::uint8_t
    {
        Fonction,
        Objet
    };

    struct SymboleMachine
    {
        std::string Nom;
        std::uint32_t Decalage;
        std::uint32_t Taille;
        bool EstPublic;
        SectionMachine Section;
        bool EstDefini;
        GenreSymboleMachine Genre = GenreSymboleMachine::Fonction;
        std::string SignatureAbi;
        PositionSource Position;
    };

    struct CodeMachine
    {
        std::vector<std::uint8_t> Texte;
        std::vector<std::uint8_t> Donnees;
        std::uint32_t TailleZero = 0;
        std::vector<SymboleMachine> Symboles;
        struct Relocalisation
        {
            std::uint32_t Decalage;
            std::string Symbole;
            SectionMachine Section;
            TypeRelocalisationMachine Type;
        };
        std::vector<Relocalisation> Relocalisations;
    };

    class GenerateurX64 final
    {
    public:
        [[nodiscard]] CodeMachine Generer(const Programme& programme) const;

        // Contexte exposé aux composants internes du backend.
        struct ContexteFonction
        {
            struct EmplacementVariable
            {
                std::int32_t Decalage;
                TypeGs Type;
            };
            CodeMachine* Machine = nullptr;
            std::unordered_map<std::string, EmplacementVariable> Variables;
            const std::unordered_set<std::string>* Fonctions = nullptr;
            const std::unordered_map<std::string, const Structure*>* Structures = nullptr;
            const std::unordered_map<const Expression*, std::string>* Chaines = nullptr;
            const Fonction* FonctionActive = nullptr;
            std::unordered_map<const Expression*, std::int32_t> TemporairesValeurs;
            struct DestructionLocale
            {
                std::string NomVariable;
                std::vector<ActionDestructionClasse> Actions;
                TypeGs Type;
            };
            std::vector<DestructionLocale> DestructionsActives;
            std::int32_t DecalageRetourStructure = 0;
            std::string Espace;
            std::size_t ProfondeurPile = 0;
        };

    private:
        static void RecenserVariables(
            const Instruction& instruction,
            std::vector<const InstructionVariable*>& variables);
        static void RecenserTemporaires(
            const Instruction& instruction,
            std::vector<const Expression*>& expressions);
        static void RecenserTemporairesExpression(
            const Expression& expression,
            std::vector<const Expression*>& expressions);
        static void RecenserChaines(
            const Instruction& instruction,
            std::vector<const ExpressionChaine*>& chaines);
        static void RecenserChainesExpression(
            const Expression& expression,
            std::vector<const ExpressionChaine*>& chaines);
        static void GenererInstruction(const Instruction& instruction, ContexteFonction& contexte);
        static void GenererExpression(const Expression& expression, ContexteFonction& contexte);
        static void GenererAppel(
            const std::string& symbole,
            const Expression* cibleIndirecte,
            const std::vector<const Expression*>& arguments,
            const std::vector<bool>& argumentsParReference,
            const TypeGs& typeRetour,
            const Expression* cleTemporaire,
            bool estIntrinseque,
            bool estVirtuel,
            std::uint32_t indexVirtuel,
            ContexteFonction& contexte);
        static void GenererDestruction(
            const ContexteFonction::DestructionLocale& destruction,
            ContexteFonction& contexte);
        static void GenererInitialiseurVersAdresse(
            const Expression& expression,
            const TypeGs& type,
            ContexteFonction& contexte);
        static void GenererAdresse(const Expression& expression, ContexteFonction& contexte);
        static void CopierMemoire(
            std::uint32_t taille,
            std::vector<std::uint8_t>& code);
        static void InitialiserMemoireZero(
            std::uint32_t taille,
            std::vector<std::uint8_t>& code);
        static void ChargerDepuisAdresse(const TypeGs& type, std::vector<std::uint8_t>& code);
        static void StockerVersAdresse(const TypeGs& type, std::vector<std::uint8_t>& code);
        static void NormaliserValeur(const TypeGs& type, std::vector<std::uint8_t>& code);
        static void GenererEpilogue(std::vector<std::uint8_t>& code);
        static void Ajouter64(std::vector<std::uint8_t>& code, std::uint64_t valeur);
        static void Ajouter32(std::vector<std::uint8_t>& code, std::int32_t valeur);
        static void CorrigerRelatif32(
            std::vector<std::uint8_t>& code,
            std::size_t position,
            std::size_t cible);
    };
}
