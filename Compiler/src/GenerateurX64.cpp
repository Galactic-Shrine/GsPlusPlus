#include "GsPP/GenerateurX64.hpp"
#include "GsPP/Intrinseques.hpp"

#include <algorithm>
#include <climits>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace GsPP
{
    namespace
    {
        void Ajouter(std::vector<std::uint8_t>& code, std::initializer_list<std::uint8_t> octets)
        {
            code.insert(code.end(), octets.begin(), octets.end());
        }

        std::uint32_t Aligner(std::uint32_t valeur, std::uint32_t alignement)
        {
            return (valeur + alignement - 1) & ~(alignement - 1);
        }

        std::uint32_t TailleType(
            const TypeGs& type,
            const std::unordered_map<std::string, const Structure*>& structures)
        {
            if (type.EstTableau())
                return type.DimensionsTableau.front()
                    * TailleType(type.ElementTableau(), structures);
            if (type.EstAdresse() || type.EstReference) return 8;
            if (type.Genre == GenreType::Entier8
                || type.Genre == GenreType::Naturel8
                || type.Genre == GenreType::Booleen
                || type.Genre == GenreType::Octet
                || type.Genre == GenreType::Caractere)
                return 1;
            if (type.Genre == GenreType::Entier16 || type.Genre == GenreType::Naturel16)
                return 2;
            if (type.Genre == GenreType::Entier32
                || type.Genre == GenreType::Naturel32
                || type.Genre == GenreType::Enumeration)
                return 4;
            if (type.Genre == GenreType::Entier64 || type.Genre == GenreType::Naturel64)
                return 8;
            if (type.Genre == GenreType::Structure) return structures.at(type.Nom)->Taille;
            return 0;
        }

        std::uint32_t AlignementType(
            const TypeGs& type,
            const std::unordered_map<std::string, const Structure*>& structures)
        {
            if (type.EstTableau()) return AlignementType(type.ElementTableau(), structures);
            if (type.EstAdresse() || type.EstReference) return 8;
            if (type.Genre == GenreType::Entier8
                || type.Genre == GenreType::Naturel8
                || type.Genre == GenreType::Booleen
                || type.Genre == GenreType::Octet
                || type.Genre == GenreType::Caractere)
                return 1;
            if (type.Genre == GenreType::Entier16 || type.Genre == GenreType::Naturel16)
                return 2;
            if (type.Genre == GenreType::Entier32
                || type.Genre == GenreType::Naturel32
                || type.Genre == GenreType::Enumeration)
                return 4;
            if (type.Genre == GenreType::Entier64 || type.Genre == GenreType::Naturel64)
                return 8;
            if (type.Genre == GenreType::Structure) return structures.at(type.Nom)->Alignement;
            return 1;
        }

        const GenerateurX64::ContexteFonction::EmplacementVariable& TrouverVariable(
            const GenerateurX64::ContexteFonction& contexte,
            const std::string& nom)
        {
            const auto trouve = contexte.Variables.find(nom);
            if (trouve == contexte.Variables.end())
                throw std::runtime_error("variable inconnue dans le backend : " + nom);
            return trouve->second;
        }

        bool EstOperationNonSignee(const TypeGs& type)
        {
            return type.EstAdresse()
                || type.EstEntierNonSigne()
                || type.EstBooleen();
        }

        void GenererIntrinseque(
            std::string_view nom,
            std::vector<std::uint8_t>& code)
        {
            switch (IdentifierIntrinseque(nom))
            {
                case GenreIntrinseque::ChargerAtomique32:
                    Ajouter(code, {0x8B, 0x01});                       // mov eax, [rcx]
                    return;
                case GenreIntrinseque::ChargerAtomique64:
                    Ajouter(code, {0x48, 0x8B, 0x01});                 // mov rax, [rcx]
                    return;
                case GenreIntrinseque::StockerAtomique32:
                    Ajouter(code, {0x89, 0x11, 0x31, 0xC0});           // mov [rcx], edx
                    return;
                case GenreIntrinseque::StockerAtomique64:
                    Ajouter(code, {0x48, 0x89, 0x11, 0x31, 0xC0});     // mov [rcx], rdx
                    return;
                case GenreIntrinseque::EchangerAtomique32:
                    Ajouter(code, {0x89, 0xD0, 0x87, 0x01});           // xchg [rcx], eax
                    return;
                case GenreIntrinseque::EchangerAtomique64:
                    Ajouter(code, {0x48, 0x89, 0xD0, 0x48, 0x87, 0x01});
                    return;
                case GenreIntrinseque::AjouterAtomique32:
                    Ajouter(code, {0x89, 0xD0, 0xF0, 0x0F, 0xC1, 0x01});
                    return;                                            // lock xadd [rcx], eax
                case GenreIntrinseque::AjouterAtomique64:
                    Ajouter(code, {0x48, 0x89, 0xD0,
                                   0xF0, 0x48, 0x0F, 0xC1, 0x01});
                    return;
                case GenreIntrinseque::ComparerEchanger32:
                    Ajouter(code, {0x89, 0xD0,
                                   0xF0, 0x44, 0x0F, 0xB1, 0x01});    // cmpxchg [rcx], r8d
                    return;
                case GenreIntrinseque::ComparerEchanger64:
                    Ajouter(code, {0x48, 0x89, 0xD0,
                                   0xF0, 0x4C, 0x0F, 0xB1, 0x01});    // cmpxchg [rcx], r8
                    return;
                case GenreIntrinseque::BarriereMemoire:
                    Ajouter(code, {0x0F, 0xAE, 0xF0, 0x31, 0xC0});     // mfence
                    return;
                case GenreIntrinseque::PauseProcesseur:
                    Ajouter(code, {0xF3, 0x90, 0x31, 0xC0});           // pause
                    return;
                case GenreIntrinseque::Aucun:
                    throw std::runtime_error("intrinsèque x86-64 inconnue");
            }
        }

        std::string SignatureType(
            const TypeGs& type,
            const std::unordered_map<std::string, const Structure*>& structures,
            std::unordered_set<std::string>& structuresEnCours)
        {
            std::ostringstream sortie;
            if (type.EstConstante) sortie << 'C';
            if (type.EstVolatile) sortie << 'V';
            if (type.EstReference) sortie << 'R';
            sortie << 'T' << static_cast<unsigned>(type.Genre);
            if (!type.Nom.empty()) sortie << '<' << type.Nom << '>';
            sortie << 'P' << type.NiveauPointeur;
            for (const auto dimension : type.DimensionsTableau)
                sortie << 'A' << dimension;

            if (type.Genre == GenreType::PointeurFonction)
            {
                sortie << "F(";
                for (std::size_t index = 0;
                     index < type.ParametresFonction.size(); ++index)
                {
                    if (index != 0) sortie << ',';
                    sortie << SignatureType(
                        type.ParametresFonction[index], structures, structuresEnCours);
                }
                sortie << ")->";
                if (type.RetourFonction)
                    sortie << SignatureType(
                        *type.RetourFonction, structures, structuresEnCours);
                else sortie << "<incomplete>";
            }

            if (type.Genre == GenreType::Structure)
            {
                const auto trouve = structures.find(type.Nom);
                if (trouve != structures.end())
                {
                    const auto* structure = trouve->second;
                    sortie << "L" << structure->Taille << ':' << structure->Alignement
                           << ':';
                    if (structure->EstUnion) sortie << 'U';
                    else if (structure->EstClasse)
                        sortie << 'C' << (structure->EstPolymorphe ? 'V' : 'N');
                    else sortie << 'S';
                    if (structuresEnCours.insert(type.Nom).second)
                    {
                        sortie << '{';
                        if (structure->EstClasse
                            && !structure->ClasseBaseCanonique.empty())
                        {
                            TypeGs base{
                                GenreType::Structure,
                                structure->ClasseBaseCanonique};
                            sortie << "B=" << SignatureType(
                                base, structures, structuresEnCours) << ';';
                        }
                        for (const auto& champ : structure->Champs)
                            sortie << champ.Nom << '@' << champ.Decalage << '='
                                   << SignatureType(champ.Type, structures, structuresEnCours)
                                   << ';';
                        if (structure->EstPolymorphe)
                        {
                            sortie << "V@"
                                   << structure->DecalageTableVirtuelle
                                   << '[';
                            for (const auto& methode
                                 : structure->MethodesVirtuellesAbi)
                                sortie << methode << ';';
                            sortie << ']';
                        }
                        sortie << '}';
                        structuresEnCours.erase(type.Nom);
                    }
                    else sortie << "{R}";
                }
            }
            return sortie.str();
        }

        std::string SignatureType(
            const TypeGs& type,
            const std::unordered_map<std::string, const Structure*>& structures)
        {
            std::unordered_set<std::string> structuresEnCours;
            return SignatureType(type, structures, structuresEnCours);
        }

        std::string SignatureFonction(
            const Fonction& fonction,
            const std::unordered_map<std::string, const Structure*>& structures)
        {
            std::ostringstream sortie;
            sortie << "GsAbi:x64-ms-v1:F(";
            for (std::size_t index = 0; index < fonction.Parametres.size(); ++index)
            {
                if (index != 0) sortie << ',';
                sortie << SignatureType(fonction.Parametres[index].Type, structures);
            }
            sortie << ")->" << SignatureType(fonction.TypeRetour, structures);
            return sortie.str();
        }

        std::string SignatureObjet(
            const VariableGlobale& variable,
            const std::unordered_map<std::string, const Structure*>& structures)
        {
            return "GsAbi:x64-ms-v1:O(" + SignatureType(variable.Type, structures) + ')';
        }
    }

    void GenerateurX64::Ajouter64(std::vector<std::uint8_t>& code, std::uint64_t valeur)
    {
        for (int index = 0; index < 8; ++index)
            code.push_back(static_cast<std::uint8_t>((valeur >> (index * 8)) & 0xFF));
    }

    void GenerateurX64::Ajouter32(std::vector<std::uint8_t>& code, std::int32_t valeur)
    {
        const auto nonSigne = static_cast<std::uint32_t>(valeur);
        for (int index = 0; index < 4; ++index)
            code.push_back(static_cast<std::uint8_t>((nonSigne >> (index * 8)) & 0xFF));
    }

    void GenerateurX64::CorrigerRelatif32(
        std::vector<std::uint8_t>& code,
        std::size_t position,
        std::size_t cible)
    {
        const auto relatif = static_cast<std::int64_t>(cible)
            - static_cast<std::int64_t>(position + 4);
        if (relatif < INT32_MIN || relatif > INT32_MAX)
            throw std::runtime_error("saut relatif x86-64 hors portée");
        const auto valeur = static_cast<std::uint32_t>(static_cast<std::int32_t>(relatif));
        for (int index = 0; index < 4; ++index)
            code[position + index] = static_cast<std::uint8_t>((valeur >> (index * 8)) & 0xFF);
    }

    void GenerateurX64::RecenserVariables(
        const Instruction& instruction,
        std::vector<const InstructionVariable*>& variables)
    {
        switch (instruction.Genre)
        {
            case GenreInstruction::Variable:
                variables.push_back(&static_cast<const InstructionVariable&>(instruction));
                return;
            case GenreInstruction::Bloc:
                for (const auto& enfant : static_cast<const InstructionBloc&>(instruction).Instructions)
                    RecenserVariables(*enfant, variables);
                return;
            case GenreInstruction::Si:
            {
                const auto& valeur = static_cast<const InstructionSi&>(instruction);
                RecenserVariables(*valeur.Alors, variables);
                if (valeur.Sinon) RecenserVariables(*valeur.Sinon, variables);
                return;
            }
            case GenreInstruction::TantQue:
                RecenserVariables(*static_cast<const InstructionTantQue&>(instruction).Corps, variables);
                return;
            default:
                return;
        }
    }

    void GenerateurX64::RecenserTemporairesExpression(
        const Expression& expression,
        std::vector<const Expression*>& expressions)
    {
        if ((expression.Genre == GenreExpression::Appel
                && expression.TypeSemantique.EstStructure())
            || (expression.Genre == GenreExpression::Unaire
                && expression.TypeSemantique.EstStructure()
                && !static_cast<const ExpressionUnaire&>(expression)
                    .NomSurcharge.empty())
            || (expression.Genre == GenreExpression::Binaire
                && expression.TypeSemantique.EstStructure()
                && !static_cast<const ExpressionBinaire&>(expression)
                    .NomSurcharge.empty())
            || (expression.Genre == GenreExpression::Agregat
                && (expression.TypeSemantique.EstStructure()
                    || expression.TypeSemantique.EstTableau())))
            expressions.push_back(&expression);

        switch (expression.Genre)
        {
            case GenreExpression::Entier:
            case GenreExpression::Chaine:
            case GenreExpression::Variable:
                return;
            case GenreExpression::Unaire:
                RecenserTemporairesExpression(
                    *static_cast<const ExpressionUnaire&>(expression).Operande,
                    expressions);
                return;
            case GenreExpression::Binaire:
            {
                const auto& valeur = static_cast<const ExpressionBinaire&>(expression);
                RecenserTemporairesExpression(*valeur.Gauche, expressions);
                RecenserTemporairesExpression(*valeur.Droite, expressions);
                return;
            }
            case GenreExpression::Affectation:
            {
                const auto& valeur = static_cast<const ExpressionAffectation&>(expression);
                RecenserTemporairesExpression(*valeur.Cible, expressions);
                RecenserTemporairesExpression(*valeur.Valeur, expressions);
                return;
            }
            case GenreExpression::Appel:
            {
                const auto& valeur = static_cast<const ExpressionAppel&>(expression);
                RecenserTemporairesExpression(*valeur.Cible, expressions);
                for (const auto& argument : valeur.Arguments)
                    RecenserTemporairesExpression(*argument, expressions);
                return;
            }
            case GenreExpression::Membre:
                RecenserTemporairesExpression(
                    *static_cast<const ExpressionMembre&>(expression).Objet,
                    expressions);
                return;
            case GenreExpression::Index:
            {
                const auto& valeur = static_cast<const ExpressionIndex&>(expression);
                RecenserTemporairesExpression(*valeur.Objet, expressions);
                RecenserTemporairesExpression(*valeur.Indice, expressions);
                return;
            }
            case GenreExpression::Conversion:
                RecenserTemporairesExpression(
                    *static_cast<const ExpressionConversion&>(expression).Valeur,
                    expressions);
                return;
            case GenreExpression::Agregat:
                for (const auto& element
                     : static_cast<const ExpressionAgregat&>(expression).Elements)
                    RecenserTemporairesExpression(*element, expressions);
                return;
        }
    }

    void GenerateurX64::RecenserChainesExpression(
        const Expression& expression,
        std::vector<const ExpressionChaine*>& chaines)
    {
        switch (expression.Genre)
        {
            case GenreExpression::Entier:
            case GenreExpression::Variable:
                return;
            case GenreExpression::Chaine:
                chaines.push_back(
                    &static_cast<const ExpressionChaine&>(expression));
                return;
            case GenreExpression::Unaire:
                RecenserChainesExpression(
                    *static_cast<const ExpressionUnaire&>(expression).Operande,
                    chaines);
                return;
            case GenreExpression::Binaire:
            {
                const auto& valeur =
                    static_cast<const ExpressionBinaire&>(expression);
                RecenserChainesExpression(*valeur.Gauche, chaines);
                RecenserChainesExpression(*valeur.Droite, chaines);
                return;
            }
            case GenreExpression::Affectation:
            {
                const auto& valeur =
                    static_cast<const ExpressionAffectation&>(expression);
                RecenserChainesExpression(*valeur.Cible, chaines);
                RecenserChainesExpression(*valeur.Valeur, chaines);
                return;
            }
            case GenreExpression::Appel:
            {
                const auto& valeur =
                    static_cast<const ExpressionAppel&>(expression);
                RecenserChainesExpression(*valeur.Cible, chaines);
                for (const auto& argument : valeur.Arguments)
                    RecenserChainesExpression(*argument, chaines);
                return;
            }
            case GenreExpression::Membre:
                RecenserChainesExpression(
                    *static_cast<const ExpressionMembre&>(expression).Objet,
                    chaines);
                return;
            case GenreExpression::Index:
            {
                const auto& valeur =
                    static_cast<const ExpressionIndex&>(expression);
                RecenserChainesExpression(*valeur.Objet, chaines);
                RecenserChainesExpression(*valeur.Indice, chaines);
                return;
            }
            case GenreExpression::Conversion:
                RecenserChainesExpression(
                    *static_cast<const ExpressionConversion&>(expression).Valeur,
                    chaines);
                return;
            case GenreExpression::Agregat:
                for (const auto& element
                     : static_cast<const ExpressionAgregat&>(expression).Elements)
                    RecenserChainesExpression(*element, chaines);
                return;
        }
    }

    void GenerateurX64::RecenserChaines(
        const Instruction& instruction,
        std::vector<const ExpressionChaine*>& chaines)
    {
        switch (instruction.Genre)
        {
            case GenreInstruction::Bloc:
                for (const auto& enfant
                     : static_cast<const InstructionBloc&>(instruction).Instructions)
                    RecenserChaines(*enfant, chaines);
                return;
            case GenreInstruction::Retour:
            {
                const auto& valeur =
                    static_cast<const InstructionRetour&>(instruction);
                if (valeur.Valeur)
                    RecenserChainesExpression(*valeur.Valeur, chaines);
                return;
            }
            case GenreInstruction::Expression:
                RecenserChainesExpression(
                    *static_cast<const InstructionExpression&>(instruction).Valeur,
                    chaines);
                return;
            case GenreInstruction::Variable:
            {
                const auto& valeur =
                    static_cast<const InstructionVariable&>(instruction);
                if (valeur.Initialiseur)
                    RecenserChainesExpression(*valeur.Initialiseur, chaines);
                for (const auto& argument : valeur.ArgumentsConstruction)
                    RecenserChainesExpression(*argument, chaines);
                return;
            }
            case GenreInstruction::Si:
            {
                const auto& valeur =
                    static_cast<const InstructionSi&>(instruction);
                RecenserChainesExpression(*valeur.Condition, chaines);
                RecenserChaines(*valeur.Alors, chaines);
                if (valeur.Sinon) RecenserChaines(*valeur.Sinon, chaines);
                return;
            }
            case GenreInstruction::TantQue:
            {
                const auto& valeur =
                    static_cast<const InstructionTantQue&>(instruction);
                RecenserChainesExpression(*valeur.Condition, chaines);
                RecenserChaines(*valeur.Corps, chaines);
                return;
            }
        }
    }

    void GenerateurX64::RecenserTemporaires(
        const Instruction& instruction,
        std::vector<const Expression*>& expressions)
    {
        switch (instruction.Genre)
        {
            case GenreInstruction::Bloc:
                for (const auto& enfant
                     : static_cast<const InstructionBloc&>(instruction).Instructions)
                    RecenserTemporaires(*enfant, expressions);
                return;
            case GenreInstruction::Retour:
            {
                const auto& valeur = static_cast<const InstructionRetour&>(instruction);
                if (valeur.Valeur)
                    RecenserTemporairesExpression(*valeur.Valeur, expressions);
                return;
            }
            case GenreInstruction::Expression:
                RecenserTemporairesExpression(
                    *static_cast<const InstructionExpression&>(instruction).Valeur,
                    expressions);
                return;
            case GenreInstruction::Variable:
            {
                const auto& valeur = static_cast<const InstructionVariable&>(instruction);
                if (valeur.Initialiseur)
                    RecenserTemporairesExpression(*valeur.Initialiseur, expressions);
                for (const auto& argument : valeur.ArgumentsConstruction)
                    RecenserTemporairesExpression(*argument, expressions);
                return;
            }
            case GenreInstruction::Si:
            {
                const auto& valeur = static_cast<const InstructionSi&>(instruction);
                RecenserTemporairesExpression(*valeur.Condition, expressions);
                RecenserTemporaires(*valeur.Alors, expressions);
                if (valeur.Sinon) RecenserTemporaires(*valeur.Sinon, expressions);
                return;
            }
            case GenreInstruction::TantQue:
            {
                const auto& valeur = static_cast<const InstructionTantQue&>(instruction);
                RecenserTemporairesExpression(*valeur.Condition, expressions);
                RecenserTemporaires(*valeur.Corps, expressions);
                return;
            }
        }
    }

    void GenerateurX64::GenererEpilogue(std::vector<std::uint8_t>& code)
    {
        Ajouter(code, {0x48, 0x89, 0xEC, 0x5D, 0xC3});
    }

    void GenerateurX64::CopierMemoire(
        std::uint32_t taille,
        std::vector<std::uint8_t>& code)
    {
        std::uint32_t position = 0;
        while (taille - position >= 8)
        {
            Ajouter(code, {0x4C, 0x8B, 0x91});
            Ajouter32(code, static_cast<std::int32_t>(position));
            Ajouter(code, {0x4C, 0x89, 0x90});
            Ajouter32(code, static_cast<std::int32_t>(position));
            position += 8;
        }
        if (taille - position >= 4)
        {
            Ajouter(code, {0x44, 0x8B, 0x91});
            Ajouter32(code, static_cast<std::int32_t>(position));
            Ajouter(code, {0x44, 0x89, 0x90});
            Ajouter32(code, static_cast<std::int32_t>(position));
            position += 4;
        }
        if (taille - position >= 2)
        {
            Ajouter(code, {0x66, 0x44, 0x8B, 0x91});
            Ajouter32(code, static_cast<std::int32_t>(position));
            Ajouter(code, {0x66, 0x44, 0x89, 0x90});
            Ajouter32(code, static_cast<std::int32_t>(position));
            position += 2;
        }
        if (position < taille)
        {
            Ajouter(code, {0x44, 0x8A, 0x91});
            Ajouter32(code, static_cast<std::int32_t>(position));
            Ajouter(code, {0x44, 0x88, 0x90});
            Ajouter32(code, static_cast<std::int32_t>(position));
        }
    }

    void GenerateurX64::InitialiserMemoireZero(
        std::uint32_t taille,
        std::vector<std::uint8_t>& code)
    {
        Ajouter(code, {0x45, 0x31, 0xD2}); // xor r10d, r10d
        std::uint32_t position = 0;
        while (taille - position >= 8)
        {
            Ajouter(code, {0x4C, 0x89, 0x90});
            Ajouter32(code, static_cast<std::int32_t>(position));
            position += 8;
        }
        if (taille - position >= 4)
        {
            Ajouter(code, {0x44, 0x89, 0x90});
            Ajouter32(code, static_cast<std::int32_t>(position));
            position += 4;
        }
        if (taille - position >= 2)
        {
            Ajouter(code, {0x66, 0x44, 0x89, 0x90});
            Ajouter32(code, static_cast<std::int32_t>(position));
            position += 2;
        }
        if (position < taille)
        {
            Ajouter(code, {0x44, 0x88, 0x90});
            Ajouter32(code, static_cast<std::int32_t>(position));
        }
    }

    void GenerateurX64::ChargerDepuisAdresse(
        const TypeGs& type,
        std::vector<std::uint8_t>& code)
    {
        if (type.EstStructure() || type.EstTableau()) return;
        if (type.EstAdresse()) Ajouter(code, {0x48, 0x8B, 0x00}); // mov rax, [rax]
        else if (type.Genre == GenreType::Entier8
                 || type.Genre == GenreType::Caractere)
            Ajouter(code, {0x48, 0x0F, 0xBE, 0x00});               // movsx rax, byte [rax]
        else if (type.Genre == GenreType::Naturel8
                 || type.Genre == GenreType::Booleen
                 || type.Genre == GenreType::Octet)
            Ajouter(code, {0x0F, 0xB6, 0x00});                     // movzx eax, byte [rax]
        else if (type.Genre == GenreType::Entier16)
            Ajouter(code, {0x48, 0x0F, 0xBF, 0x00});               // movsx rax, word [rax]
        else if (type.Genre == GenreType::Naturel16)
            Ajouter(code, {0x0F, 0xB7, 0x00});                     // movzx eax, word [rax]
        else if (type.Genre == GenreType::Entier32
                 || type.Genre == GenreType::Enumeration)
            Ajouter(code, {0x48, 0x63, 0x00});                     // movsxd rax, dword [rax]
        else if (type.Genre == GenreType::Naturel32)
            Ajouter(code, {0x8B, 0x00});                           // mov eax, dword [rax]
        else Ajouter(code, {0x48, 0x8B, 0x00});                    // mov rax, qword [rax]
    }

    void GenerateurX64::StockerVersAdresse(
        const TypeGs& type,
        std::vector<std::uint8_t>& code)
    {
        if (type.EstAdresse() || type.EstReference
            || type.Genre == GenreType::Entier64
            || type.Genre == GenreType::Naturel64)
            Ajouter(code, {0x48, 0x89, 0x08});                     // mov [rax], rcx
        else if (type.Genre == GenreType::Entier8
                 || type.Genre == GenreType::Naturel8
                 || type.Genre == GenreType::Booleen
                 || type.Genre == GenreType::Octet
                 || type.Genre == GenreType::Caractere)
            Ajouter(code, {0x88, 0x08});                           // mov [rax], cl
        else if (type.Genre == GenreType::Entier16
                 || type.Genre == GenreType::Naturel16)
            Ajouter(code, {0x66, 0x89, 0x08});                     // mov [rax], cx
        else Ajouter(code, {0x89, 0x08});                          // mov [rax], ecx
    }

    void GenerateurX64::NormaliserValeur(
        const TypeGs& type,
        std::vector<std::uint8_t>& code)
    {
        if (type.EstAdresse()
            || type.EstStructure()
            || type.EstTableau()
            || type.EstVide()
            || type.Genre == GenreType::Entier64
            || type.Genre == GenreType::Naturel64)
            return;
        if (type.EstBooleen())
        {
            Ajouter(code, {0x48, 0x85, 0xC0, 0x0F, 0x95, 0xC0, 0x0F, 0xB6, 0xC0});
            return;
        }
        if (type.Genre == GenreType::Entier8 || type.Genre == GenreType::Caractere)
            Ajouter(code, {0x48, 0x0F, 0xBE, 0xC0});               // movsx rax, al
        else if (type.Genre == GenreType::Naturel8 || type.Genre == GenreType::Octet)
            Ajouter(code, {0x0F, 0xB6, 0xC0});                     // movzx eax, al
        else if (type.Genre == GenreType::Entier16)
            Ajouter(code, {0x48, 0x0F, 0xBF, 0xC0});               // movsx rax, ax
        else if (type.Genre == GenreType::Naturel16)
            Ajouter(code, {0x0F, 0xB7, 0xC0});                     // movzx eax, ax
        else if (type.Genre == GenreType::Entier32
                 || type.Genre == GenreType::Enumeration)
            Ajouter(code, {0x48, 0x63, 0xC0});                     // movsxd rax, eax
        else if (type.Genre == GenreType::Naturel32)
            Ajouter(code, {0x89, 0xC0});                           // zéro-étendre eax
    }

    void GenerateurX64::GenererAdresse(
        const Expression& expression,
        ContexteFonction& contexte)
    {
        auto& code = contexte.Machine->Texte;
        if (expression.Genre == GenreExpression::Variable)
        {
            const auto& variable = static_cast<const ExpressionVariable&>(expression);
            if (variable.EstFonction)
            {
                Ajouter(code, {0x48, 0x8D, 0x05}); // lea rax, [rip+rel32]
                const auto position = static_cast<std::uint32_t>(code.size());
                Ajouter32(code, 0);
                contexte.Machine->Relocalisations.push_back({
                    position,
                    variable.Nom,
                    SectionMachine::Texte,
                    TypeRelocalisationMachine::Relatif32
                });
                return;
            }
            if (variable.EstGlobale)
            {
                Ajouter(code, {0x48, 0x8D, 0x05}); // lea rax, [rip+rel32]
                const auto position = static_cast<std::uint32_t>(code.size());
                Ajouter32(code, 0);
                contexte.Machine->Relocalisations.push_back({
                    position,
                    variable.Nom,
                    SectionMachine::Texte,
                    TypeRelocalisationMachine::Relatif32
                });
                return;
            }
            const auto& emplacement = TrouverVariable(contexte, variable.Nom);
            if (variable.EstReference || emplacement.Type.EstReference)
            {
                Ajouter(code, {0x48, 0x8B, 0x85}); // mov rax, [rbp+disp32]
                Ajouter32(code, emplacement.Decalage);
                return;
            }
            Ajouter(code, {0x48, 0x8D, 0x85}); // lea rax, [rbp+disp32]
            Ajouter32(code, emplacement.Decalage);
            return;
        }
        if (expression.Genre == GenreExpression::Membre)
        {
            const auto& membre = static_cast<const ExpressionMembre&>(expression);
            if (membre.ViaPointeur
                || membre.Objet->Genre == GenreExpression::Appel
                || membre.Objet->Genre == GenreExpression::Agregat
                || membre.Objet->Genre == GenreExpression::Affectation)
                GenererExpression(*membre.Objet, contexte);
            else GenererAdresse(*membre.Objet, contexte);
            if (membre.DecalageMembre != 0)
            {
                Ajouter(code, {0x48, 0x05}); // add rax, imm32
                Ajouter32(code, static_cast<std::int32_t>(membre.DecalageMembre));
            }
            return;
        }
        if (expression.Genre == GenreExpression::Index)
        {
            const auto& index = static_cast<const ExpressionIndex&>(expression);
            if (index.Objet->TypeSemantique.EstTableau())
                GenererAdresse(*index.Objet, contexte);
            else GenererExpression(*index.Objet, contexte);
            Ajouter(code, {0x50});
            contexte.ProfondeurPile += 8;
            GenererExpression(*index.Indice, contexte);
            if (index.TailleElement != 1)
            {
                Ajouter(code, {0x48, 0x69, 0xC0}); // imul rax, rax, imm32
                Ajouter32(code, static_cast<std::int32_t>(index.TailleElement));
            }
            Ajouter(code, {0x59, 0x48, 0x01, 0xC8}); // pop rcx; add rax, rcx
            contexte.ProfondeurPile -= 8;
            return;
        }
        if (expression.Genre == GenreExpression::Unaire)
        {
            const auto& unaire = static_cast<const ExpressionUnaire&>(expression);
            if (unaire.Operateur == "*")
            {
                GenererExpression(*unaire.Operande, contexte);
                return;
            }
        }
        throw std::runtime_error("expression non adressable dans le backend");
    }

    void GenerateurX64::GenererInitialiseurVersAdresse(
        const Expression& expression,
        const TypeGs& type,
        ContexteFonction& contexte)
    {
        auto& code = contexte.Machine->Texte;
        if (expression.Genre == GenreExpression::Agregat)
        {
            const auto& agregat = static_cast<const ExpressionAgregat&>(expression);
            const auto taille = TailleType(type, *contexte.Structures);
            InitialiserMemoireZero(taille, code);
            Ajouter(code, {0x50});
            contexte.ProfondeurPile += 8;

            if (type.EstTableau())
            {
                const auto typeElement = type.ElementTableau();
                const auto tailleElement = TailleType(
                    typeElement, *contexte.Structures);
                for (std::size_t index = 0; index < agregat.Elements.size(); ++index)
                {
                    Ajouter(code, {0x48, 0x8B, 0x04, 0x24});
                    const auto decalage = static_cast<std::uint64_t>(index)
                        * tailleElement;
                    if (decalage != 0)
                    {
                        Ajouter(code, {0x48, 0x05});
                        Ajouter32(code, static_cast<std::int32_t>(decalage));
                    }
                    GenererInitialiseurVersAdresse(
                        *agregat.Elements[index], typeElement, contexte);
                }
            }
            else if (type.EstStructure())
            {
                const auto* structure = contexte.Structures->at(type.Nom);
                for (std::size_t index = 0; index < agregat.Elements.size(); ++index)
                {
                    Ajouter(code, {0x48, 0x8B, 0x04, 0x24});
                    const auto decalage = structure->Champs[index].Decalage;
                    if (decalage != 0)
                    {
                        Ajouter(code, {0x48, 0x05});
                        Ajouter32(code, static_cast<std::int32_t>(decalage));
                    }
                    GenererInitialiseurVersAdresse(
                        *agregat.Elements[index],
                        structure->Champs[index].Type,
                        contexte);
                }
            }
            else if (!agregat.Elements.empty())
            {
                Ajouter(code, {0x48, 0x8B, 0x04, 0x24});
                GenererInitialiseurVersAdresse(
                    *agregat.Elements.front(), type, contexte);
            }

            Ajouter(code, {0x58});
            contexte.ProfondeurPile -= 8;
            return;
        }

        Ajouter(code, {0x50});
        contexte.ProfondeurPile += 8;
        GenererExpression(expression, contexte);
        Ajouter(code, {0x48, 0x89, 0xC1, 0x58}); // source/valeur -> rcx, destination -> rax
        contexte.ProfondeurPile -= 8;
        if (type.EstStructure() || type.EstTableau())
            CopierMemoire(TailleType(type, *contexte.Structures), code);
        else StockerVersAdresse(type, code);
    }

    void GenerateurX64::GenererAppel(
        const std::string& symbole,
        const Expression* cibleIndirecte,
        const std::vector<const Expression*>& arguments,
        const std::vector<bool>& argumentsParReference,
        const TypeGs& typeRetour,
        const Expression* cleTemporaire,
        bool estIntrinseque,
        bool estVirtuel,
        std::uint32_t indexVirtuel,
        ContexteFonction& contexte)
    {
        auto& code = contexte.Machine->Texte;
        const bool retourStructure = typeRetour.EstStructure();
        const auto maximumArguments = retourStructure ? 3U : 4U;
        if (arguments.size() > maximumArguments)
            throw std::runtime_error(
                retourStructure
                    ? "un retour de structure accepte au maximum trois arguments"
                    : "le prototype accepte au maximum quatre arguments");

        if (cibleIndirecte)
        {
            GenererExpression(*cibleIndirecte, contexte);
            Ajouter(code, {0x50});
            contexte.ProfondeurPile += 8;
        }
        for (std::size_t index = 0; index < arguments.size(); ++index)
        {
            if (index < argumentsParReference.size()
                && argumentsParReference[index])
                GenererAdresse(*arguments[index], contexte);
            else GenererExpression(*arguments[index], contexte);
            Ajouter(code, {0x50});
            contexte.ProfondeurPile += 8;
        }
        for (std::size_t index = arguments.size(); index > 0; --index)
        {
            const auto registre = index - 1 + (retourStructure ? 1U : 0U);
            switch (registre)
            {
                case 0: Ajouter(code, {0x59}); break;
                case 1: Ajouter(code, {0x5A}); break;
                case 2: Ajouter(code, {0x41, 0x58}); break;
                case 3: Ajouter(code, {0x41, 0x59}); break;
                default: throw std::runtime_error("registre d’argument invalide");
            }
            contexte.ProfondeurPile -= 8;
        }
        if (estIntrinseque)
        {
            GenererIntrinseque(symbole, code);
            NormaliserValeur(typeRetour, code);
            return;
        }
        if (cibleIndirecte)
        {
            Ajouter(code, {0x41, 0x5B}); // pop r11
            contexte.ProfondeurPile -= 8;
        }

        std::int32_t temporaireRetour = 0;
        if (retourStructure)
        {
            const auto trouve = contexte.TemporairesValeurs.find(cleTemporaire);
            if (trouve == contexte.TemporairesValeurs.end())
                throw std::runtime_error(
                    "zone temporaire de retour de structure introuvable");
            temporaireRetour = trouve->second;
            Ajouter(code, {0x48, 0x8D, 0x8D}); // lea rcx, [rbp+disp32]
            Ajouter32(code, temporaireRetour);
        }
        if (estVirtuel)
        {
            if (arguments.empty())
                throw std::runtime_error("appel virtuel sans receveur");
            const auto& typeReceveur = arguments.front()->TypeSemantique;
            if (!typeReceveur.EstStructure())
                throw std::runtime_error(
                    "receveur virtuel sans type classe");
            const auto decalageTable = contexte.Structures->at(
                typeReceveur.Nom)->DecalageTableVirtuelle;
            Ajouter(code, retourStructure
                ? std::initializer_list<std::uint8_t>{0x4C, 0x8B, 0x9A}
                : std::initializer_list<std::uint8_t>{0x4C, 0x8B, 0x99});
            Ajouter32(code, static_cast<std::int32_t>(decalageTable));
            Ajouter(code, {0x4D, 0x8B, 0x9B}); // mov r11, [r11+disp32]
            Ajouter32(code, static_cast<std::int32_t>(indexVirtuel * 8U));
        }

        const std::uint8_t reserve = contexte.ProfondeurPile % 16 == 0 ? 32 : 40;
        Ajouter(code, {0x48, 0x83, 0xEC, reserve});
        if (cibleIndirecte || estVirtuel)
            Ajouter(code, {0x41, 0xFF, 0xD3}); // call r11
        else
        {
            Ajouter(code, {0xE8});
            const auto position = static_cast<std::uint32_t>(code.size());
            Ajouter32(code, 0);
            contexte.Machine->Relocalisations.push_back({
                position,
                symbole,
                SectionMachine::Texte,
                TypeRelocalisationMachine::Relatif32
            });
        }
        Ajouter(code, {0x48, 0x83, 0xC4, reserve});
        if (retourStructure)
        {
            Ajouter(code, {0x48, 0x8D, 0x85});
            Ajouter32(code, temporaireRetour);
        }
        else NormaliserValeur(typeRetour, code);
    }

    void GenerateurX64::GenererDestruction(
        const ContexteFonction::DestructionLocale& destruction,
        ContexteFonction& contexte)
    {
        const std::vector<bool> references{true};
        for (const auto& action : destruction.Actions)
        {
            auto objet = std::make_unique<ExpressionVariable>(
                destruction.NomVariable,
                contexte.FonctionActive->Position);
            objet->TypeSemantique = destruction.Type;
            auto recepteur = std::make_unique<ExpressionMembre>(
                std::move(objet),
                std::string{},
                false,
                contexte.FonctionActive->Position);
            recepteur->DecalageMembre = action.Decalage;
            recepteur->TypeSemantique = TypeGs{
                GenreType::Structure,
                action.ClasseRecepteur};
            const std::vector<const Expression*> arguments{
                recepteur.get()};
            GenererAppel(
                action.SymboleDestructeur,
                nullptr,
                arguments,
                references,
                TypeGs{GenreType::Vide},
                nullptr,
                false,
                false,
                0,
                contexte);
        }
    }

    void GenerateurX64::GenererExpression(
        const Expression& expression,
        ContexteFonction& contexte)
    {
        auto& code = contexte.Machine->Texte;
        switch (expression.Genre)
        {
            case GenreExpression::Entier:
            {
                const auto& entier = static_cast<const ExpressionEntier&>(expression);
                Ajouter(code, {0x48, 0xB8});
                Ajouter64(code, static_cast<std::uint64_t>(entier.Valeur));
                return;
            }
            case GenreExpression::Chaine:
            {
                const auto trouve = contexte.Chaines->find(&expression);
                if (trouve == contexte.Chaines->end())
                    throw std::runtime_error(
                        "symbole de chaîne introuvable dans le backend");
                Ajouter(code, {0x48, 0x8D, 0x05});
                const auto position = static_cast<std::uint32_t>(code.size());
                Ajouter32(code, 0);
                contexte.Machine->Relocalisations.push_back({
                    position,
                    trouve->second,
                    SectionMachine::Texte,
                    TypeRelocalisationMachine::Relatif32
                });
                return;
            }
            case GenreExpression::Variable:
                if (static_cast<const ExpressionVariable&>(expression).EstFonction)
                {
                    GenererAdresse(expression, contexte);
                    return;
                }
                if (static_cast<const ExpressionVariable&>(expression).EstConstanteEnumeration)
                {
                    Ajouter(code, {0x48, 0xB8});
                    Ajouter64(code, static_cast<std::uint64_t>(
                        static_cast<const ExpressionVariable&>(expression).ValeurEnumeration));
                    return;
                }
                [[fallthrough]];
            case GenreExpression::Membre:
            case GenreExpression::Index:
                GenererAdresse(expression, contexte);
                ChargerDepuisAdresse(expression.TypeSemantique, code);
                return;
            case GenreExpression::Affectation:
            {
                const auto& affectation = static_cast<const ExpressionAffectation&>(expression);
                GenererExpression(*affectation.Valeur, contexte);
                Ajouter(code, {0x50});
                contexte.ProfondeurPile += 8;
                GenererAdresse(*affectation.Cible, contexte);
                Ajouter(code, {0x59});
                contexte.ProfondeurPile -= 8;
                if (affectation.Cible->TypeSemantique.EstStructure())
                {
                    CopierMemoire(
                        TailleType(
                            affectation.Cible->TypeSemantique,
                            *contexte.Structures),
                        code);
                    return;
                }
                StockerVersAdresse(affectation.Cible->TypeSemantique, code);
                Ajouter(code, {0x48, 0x89, 0xC8}); // résultat = valeur affectée
                NormaliserValeur(expression.TypeSemantique, code);
                return;
            }
            case GenreExpression::Unaire:
            {
                const auto& unaire = static_cast<const ExpressionUnaire&>(expression);
                if (!unaire.NomSurcharge.empty())
                {
                    const std::vector<const Expression*> arguments{
                        unaire.Operande.get()};
                    const std::vector<bool> references{
                        unaire.OperandeParReference};
                    GenererAppel(
                        unaire.NomSurcharge, nullptr, arguments, references,
                        expression.TypeSemantique, &expression,
                        false, false, 0, contexte);
                    return;
                }
                if (unaire.Operateur == "&")
                {
                    GenererAdresse(*unaire.Operande, contexte);
                    return;
                }
                if (unaire.Operateur == "*")
                {
                    GenererExpression(*unaire.Operande, contexte);
                    if (unaire.Operande->TypeSemantique.EstPointeurFonction())
                        return;
                    ChargerDepuisAdresse(expression.TypeSemantique, code);
                    return;
                }
                GenererExpression(*unaire.Operande, contexte);
                if (unaire.Operateur == "-") Ajouter(code, {0x48, 0xF7, 0xD8});
                else if (unaire.Operateur == "~") Ajouter(code, {0x48, 0xF7, 0xD0});
                else if (unaire.Operateur == "!")
                {
                    Ajouter(code, {0x48, 0x85, 0xC0, 0x0F, 0x94, 0xC0, 0x48, 0x0F, 0xB6, 0xC0});
                }
                NormaliserValeur(expression.TypeSemantique, code);
                return;
            }
            case GenreExpression::Binaire:
            {
                const auto& binaire = static_cast<const ExpressionBinaire&>(expression);
                if (!binaire.NomSurcharge.empty())
                {
                    const std::vector<const Expression*> arguments{
                        binaire.Gauche.get(), binaire.Droite.get()};
                    const std::vector<bool> references{
                        binaire.GaucheParReference,
                        binaire.DroiteParReference};
                    GenererAppel(
                        binaire.NomSurcharge, nullptr, arguments, references,
                        expression.TypeSemantique, &expression,
                        false, false, 0, contexte);
                    return;
                }
                if (binaire.Operateur == "&&" || binaire.Operateur == "||")
                {
                    GenererExpression(*binaire.Gauche, contexte);
                    Ajouter(code, {0x48, 0x85, 0xC0});
                    Ajouter(code, {
                        0x0F,
                        static_cast<std::uint8_t>(
                            binaire.Operateur == "&&" ? 0x84 : 0x85)
                    });
                    const auto sautCourt = code.size();
                    Ajouter32(code, 0);
                    GenererExpression(*binaire.Droite, contexte);
                    Ajouter(code, {
                        0x48, 0x85, 0xC0,
                        0x0F, 0x95, 0xC0,
                        0x0F, 0xB6, 0xC0,
                        0xE9
                    });
                    const auto sautFin = code.size();
                    Ajouter32(code, 0);
                    CorrigerRelatif32(code, sautCourt, code.size());
                    if (binaire.Operateur == "&&")
                        Ajouter(code, {0x31, 0xC0});
                    else
                        Ajouter(code, {0xB8, 0x01, 0x00, 0x00, 0x00});
                    CorrigerRelatif32(code, sautFin, code.size());
                    return;
                }
                GenererExpression(*binaire.Gauche, contexte);
                Ajouter(code, {0x50});
                contexte.ProfondeurPile += 8;
                GenererExpression(*binaire.Droite, contexte);
                Ajouter(code, {0x48, 0x89, 0xC1, 0x58});
                contexte.ProfondeurPile -= 8;
                if (binaire.Operateur == "+") Ajouter(code, {0x48, 0x01, 0xC8});
                else if (binaire.Operateur == "-") Ajouter(code, {0x48, 0x29, 0xC8});
                else if (binaire.Operateur == "*") Ajouter(code, {0x48, 0x0F, 0xAF, 0xC1});
                else if (binaire.Operateur == "&") Ajouter(code, {0x48, 0x21, 0xC8});
                else if (binaire.Operateur == "|") Ajouter(code, {0x48, 0x09, 0xC8});
                else if (binaire.Operateur == "^") Ajouter(code, {0x48, 0x31, 0xC8});
                else if (binaire.Operateur == "<<") Ajouter(code, {0x48, 0xD3, 0xE0});
                else if (binaire.Operateur == ">>")
                    Ajouter(code, EstOperationNonSignee(binaire.Gauche->TypeSemantique)
                        ? std::initializer_list<std::uint8_t>{0x48, 0xD3, 0xE8}
                        : std::initializer_list<std::uint8_t>{0x48, 0xD3, 0xF8});
                else if (binaire.Operateur == "/" || binaire.Operateur == "%")
                {
                    if (EstOperationNonSignee(binaire.Gauche->TypeSemantique))
                        Ajouter(code, {0x31, 0xD2, 0x48, 0xF7, 0xF1});
                    else Ajouter(code, {0x48, 0x99, 0x48, 0xF7, 0xF9});
                    if (binaire.Operateur == "%") Ajouter(code, {0x48, 0x89, 0xD0});
                }
                else
                {
                    Ajouter(code, {0x48, 0x39, 0xC8});
                    if (binaire.Operateur == "==") Ajouter(code, {0x0F, 0x94, 0xC0});
                    else if (binaire.Operateur == "!=") Ajouter(code, {0x0F, 0x95, 0xC0});
                    else if (binaire.Operateur == "<")
                        Ajouter(code, {0x0F,
                            static_cast<std::uint8_t>(
                                EstOperationNonSignee(binaire.Gauche->TypeSemantique)
                                    ? 0x92 : 0x9C),
                            0xC0});
                    else if (binaire.Operateur == "<=")
                        Ajouter(code, {0x0F,
                            static_cast<std::uint8_t>(
                                EstOperationNonSignee(binaire.Gauche->TypeSemantique)
                                    ? 0x96 : 0x9E),
                            0xC0});
                    else if (binaire.Operateur == ">")
                        Ajouter(code, {0x0F,
                            static_cast<std::uint8_t>(
                                EstOperationNonSignee(binaire.Gauche->TypeSemantique)
                                    ? 0x97 : 0x9F),
                            0xC0});
                    else if (binaire.Operateur == ">=")
                        Ajouter(code, {0x0F,
                            static_cast<std::uint8_t>(
                                EstOperationNonSignee(binaire.Gauche->TypeSemantique)
                                    ? 0x93 : 0x9D),
                            0xC0});
                    else throw std::runtime_error("opérateur binaire non générable");
                    Ajouter(code, {0x48, 0x0F, 0xB6, 0xC0});
                }
                NormaliserValeur(expression.TypeSemantique, code);
                return;
            }
            case GenreExpression::Appel:
            {
                const auto& appel = static_cast<const ExpressionAppel&>(expression);
                std::vector<const Expression*> arguments;
                arguments.reserve(appel.Arguments.size());
                for (const auto& argument : appel.Arguments)
                    arguments.push_back(argument.get());
                GenererAppel(
                    appel.NomDirect,
                    appel.EstIndirect ? appel.Cible.get() : nullptr,
                    arguments,
                    appel.ArgumentsParReference,
                    expression.TypeSemantique,
                    &expression,
                    appel.EstIntrinseque,
                    appel.EstVirtuel,
                    appel.IndexVirtuel,
                    contexte);
                return;
            }
            case GenreExpression::Conversion:
            {
                const auto& conversion = static_cast<const ExpressionConversion&>(expression);
                GenererExpression(*conversion.Valeur, contexte);
                NormaliserValeur(conversion.TypeCible, code);
                return;
            }
            case GenreExpression::Agregat:
            {
                const auto& agregat = static_cast<const ExpressionAgregat&>(expression);
                if (expression.TypeSemantique.EstStructure()
                    || expression.TypeSemantique.EstTableau())
                {
                    const auto trouve = contexte.TemporairesValeurs.find(&expression);
                    if (trouve == contexte.TemporairesValeurs.end())
                        throw std::runtime_error(
                            "zone temporaire d’initialiseur agrégé introuvable");
                    Ajouter(code, {0x48, 0x8D, 0x85}); // lea rax, [rbp+disp32]
                    Ajouter32(code, trouve->second);
                    GenererInitialiseurVersAdresse(
                        expression, expression.TypeSemantique, contexte);
                    return;
                }
                if (agregat.Elements.empty()) Ajouter(code, {0x31, 0xC0});
                else GenererExpression(*agregat.Elements.front(), contexte);
                NormaliserValeur(expression.TypeSemantique, code);
                return;
            }
        }
    }

    void GenerateurX64::GenererInstruction(const Instruction& instruction, ContexteFonction& contexte)
    {
        auto& code = contexte.Machine->Texte;
        switch (instruction.Genre)
        {
            case GenreInstruction::Bloc:
            {
                const auto profondeur = contexte.DestructionsActives.size();
                for (const auto& enfant : static_cast<const InstructionBloc&>(instruction).Instructions)
                    GenererInstruction(*enfant, contexte);
                for (std::size_t index = contexte.DestructionsActives.size();
                     index > profondeur; --index)
                    GenererDestruction(
                        contexte.DestructionsActives[index - 1], contexte);
                contexte.DestructionsActives.resize(profondeur);
                return;
            }
            case GenreInstruction::Retour:
            {
                const auto& retour = static_cast<const InstructionRetour&>(instruction);
                if (contexte.FonctionActive->TypeRetour.EstStructure())
                {
                    Ajouter(code, {0x48, 0x8B, 0x85}); // mov rax, [rbp+disp32]
                    Ajouter32(code, contexte.DecalageRetourStructure);
                    if (retour.Valeur)
                        GenererInitialiseurVersAdresse(
                            *retour.Valeur,
                            contexte.FonctionActive->TypeRetour,
                            contexte);
                    else InitialiserMemoireZero(
                        TailleType(
                            contexte.FonctionActive->TypeRetour,
                            *contexte.Structures),
                        code);
                    if (!contexte.DestructionsActives.empty())
                    {
                        for (auto destruction = contexte.DestructionsActives.rbegin();
                             destruction != contexte.DestructionsActives.rend();
                             ++destruction)
                            GenererDestruction(*destruction, contexte);
                        Ajouter(code, {0x48, 0x8B, 0x85});
                        Ajouter32(code, contexte.DecalageRetourStructure);
                    }
                }
                else if (retour.Valeur)
                {
                    GenererExpression(*retour.Valeur, contexte);
                    NormaliserValeur(retour.Valeur->TypeSemantique, code);
                    if (!contexte.DestructionsActives.empty())
                    {
                        Ajouter(code, {0x50});
                        contexte.ProfondeurPile += 8;
                        for (auto destruction = contexte.DestructionsActives.rbegin();
                             destruction != contexte.DestructionsActives.rend();
                             ++destruction)
                            GenererDestruction(*destruction, contexte);
                        Ajouter(code, {0x58});
                        contexte.ProfondeurPile -= 8;
                    }
                }
                else
                {
                    for (auto destruction = contexte.DestructionsActives.rbegin();
                         destruction != contexte.DestructionsActives.rend();
                         ++destruction)
                        GenererDestruction(*destruction, contexte);
                    Ajouter(code, {0x31, 0xC0});
                }
                GenererEpilogue(code);
                return;
            }
            case GenreInstruction::Expression:
                GenererExpression(*static_cast<const InstructionExpression&>(instruction).Valeur, contexte);
                return;
            case GenreInstruction::Variable:
            {
                const auto& variable = static_cast<const InstructionVariable&>(instruction);
                ExpressionVariable cible(variable.Nom, variable.Position);
                cible.TypeSemantique = variable.Type;
                if (variable.Type.EstReference)
                {
                    GenererAdresse(*variable.Initialiseur, contexte);
                    Ajouter(code, {0x50});
                    contexte.ProfondeurPile += 8;
                    const auto& emplacement = TrouverVariable(contexte, variable.Nom);
                    Ajouter(code, {0x48, 0x8D, 0x85});
                    Ajouter32(code, emplacement.Decalage);
                    Ajouter(code, {0x59});
                    contexte.ProfondeurPile -= 8;
                    StockerVersAdresse(variable.Type, code);
                    return;
                }
                auto typeElementObjet = variable.Type;
                while (typeElementObjet.EstTableau())
                    typeElementObjet = typeElementObjet.ElementTableau();
                const bool estClasse = typeElementObjet.EstStructure()
                    && contexte.Structures->at(typeElementObjet.Nom)->EstClasse;
                if (estClasse)
                {
                    GenererAdresse(cible, contexte);
                    InitialiserMemoireZero(
                        TailleType(variable.Type, *contexte.Structures), code);
                    auto initialiserTable = [&](const Structure& classe,
                                                std::uint32_t decalageObjet)
                    {
                        if (!classe.EstPolymorphe) return;
                        GenererAdresse(cible, contexte);
                        if (decalageObjet != 0)
                        {
                            Ajouter(code, {0x48, 0x05});
                            Ajouter32(
                                code,
                                static_cast<std::int32_t>(decalageObjet));
                        }
                        Ajouter(code, {0x50, 0x48, 0x8D, 0x0D});
                        const auto position =
                            static_cast<std::uint32_t>(code.size());
                        Ajouter32(code, 0);
                        contexte.Machine->Relocalisations.push_back({
                            position,
                            classe.SymboleTableVirtuelle,
                            SectionMachine::Texte,
                            TypeRelocalisationMachine::Relatif32
                        });
                        Ajouter(code, {0x58});
                        if (classe.DecalageTableVirtuelle != 0)
                        {
                            Ajouter(code, {0x48, 0x05});
                            Ajouter32(
                                code,
                                static_cast<std::int32_t>(
                                    classe.DecalageTableVirtuelle));
                        }
                        Ajouter(code, {0x48, 0x89, 0x08});
                    };
                    auto creerRecepteur = [&](std::uint32_t decalage,
                                              const std::string& classe)
                    {
                        auto objet = std::make_unique<ExpressionVariable>(
                            variable.Nom,
                            variable.Position);
                        objet->TypeSemantique = variable.Type;
                        auto recepteur = std::make_unique<ExpressionMembre>(
                            std::move(objet),
                            std::string{},
                            false,
                            variable.Position);
                        recepteur->DecalageMembre = decalage;
                        recepteur->TypeSemantique = TypeGs{
                            GenreType::Structure,
                            classe};
                        return recepteur;
                    };
                    if (variable.SymboleConstructeur.empty())
                        for (const auto& etape :
                             variable.EtapesConstructionImplicite)
                        {
                            if (!etape.SymboleConstructeur.empty())
                            {
                                auto recepteur = creerRecepteur(
                                    etape.Decalage,
                                    etape.ClasseRecepteur.empty()
                                        ? variable.Type.Nom
                                        : etape.ClasseRecepteur);
                                std::vector<const Expression*> arguments{
                                    recepteur.get()};
                                for (const auto& argument :
                                     variable.ArgumentsConstruction)
                                    arguments.push_back(argument.get());
                                const std::vector<bool> references =
                                    variable.ArgumentsConstruction.empty()
                                    ? std::vector<bool>{true}
                                    : variable
                                        .ArgumentsConstructionParReference;
                                GenererAppel(
                                    etape.SymboleConstructeur,
                                    nullptr,
                                    arguments,
                                    references,
                                    TypeGs{GenreType::Vide},
                                    nullptr,
                                    false,
                                    false,
                                    0,
                                    contexte);
                            }
                            else if (!etape.ClasseTableVirtuelle.empty())
                                initialiserTable(
                                    *contexte.Structures->at(
                                        etape.ClasseTableVirtuelle),
                                    etape.Decalage);
                        }
                    if (!variable.SymboleConstructeur.empty())
                    {
                        std::vector<const Expression*> arguments{&cible};
                        for (const auto& argument : variable.ArgumentsConstruction)
                            arguments.push_back(argument.get());
                        GenererAppel(
                            variable.SymboleConstructeur,
                            nullptr,
                            arguments,
                            variable.ArgumentsConstructionParReference,
                            TypeGs{GenreType::Vide},
                            nullptr,
                            false,
                            false,
                            0,
                            contexte);
                    }
                }
                else if (!variable.Initialiseur) return;
                else if (variable.Type.EstStructure()
                    || variable.Type.EstTableau()
                    || variable.Initialiseur->Genre == GenreExpression::Agregat)
                {
                    GenererAdresse(cible, contexte);
                    GenererInitialiseurVersAdresse(
                        *variable.Initialiseur, variable.Type, contexte);
                }
                else
                {
                    GenererExpression(*variable.Initialiseur, contexte);
                    Ajouter(code, {0x50});
                    contexte.ProfondeurPile += 8;
                    GenererAdresse(cible, contexte);
                    Ajouter(code, {0x59});
                    contexte.ProfondeurPile -= 8;
                    StockerVersAdresse(variable.Type, code);
                }
                if (!variable.ActionsDestruction.empty())
                    contexte.DestructionsActives.push_back({
                        variable.Nom,
                        variable.ActionsDestruction,
                        variable.Type
                    });
                return;
            }
            case GenreInstruction::Si:
            {
                const auto& valeur = static_cast<const InstructionSi&>(instruction);
                GenererExpression(*valeur.Condition, contexte);
                Ajouter(code, {0x48, 0x85, 0xC0, 0x0F, 0x84});
                const auto sautSinon = code.size();
                Ajouter32(code, 0);
                const auto profondeurAlors =
                    contexte.DestructionsActives.size();
                GenererInstruction(*valeur.Alors, contexte);
                for (std::size_t index = contexte.DestructionsActives.size();
                     index > profondeurAlors; --index)
                    GenererDestruction(
                        contexte.DestructionsActives[index - 1], contexte);
                contexte.DestructionsActives.resize(profondeurAlors);
                if (valeur.Sinon)
                {
                    Ajouter(code, {0xE9});
                    const auto sautFin = code.size();
                    Ajouter32(code, 0);
                    CorrigerRelatif32(code, sautSinon, code.size());
                    const auto profondeurSinon =
                        contexte.DestructionsActives.size();
                    GenererInstruction(*valeur.Sinon, contexte);
                    for (std::size_t index = contexte.DestructionsActives.size();
                         index > profondeurSinon; --index)
                        GenererDestruction(
                            contexte.DestructionsActives[index - 1], contexte);
                    contexte.DestructionsActives.resize(profondeurSinon);
                    CorrigerRelatif32(code, sautFin, code.size());
                }
                else CorrigerRelatif32(code, sautSinon, code.size());
                return;
            }
            case GenreInstruction::TantQue:
            {
                const auto& valeur = static_cast<const InstructionTantQue&>(instruction);
                const auto debut = code.size();
                GenererExpression(*valeur.Condition, contexte);
                Ajouter(code, {0x48, 0x85, 0xC0, 0x0F, 0x84});
                const auto sautFin = code.size();
                Ajouter32(code, 0);
                const auto profondeurCorps =
                    contexte.DestructionsActives.size();
                GenererInstruction(*valeur.Corps, contexte);
                for (std::size_t index = contexte.DestructionsActives.size();
                     index > profondeurCorps; --index)
                    GenererDestruction(
                        contexte.DestructionsActives[index - 1], contexte);
                contexte.DestructionsActives.resize(profondeurCorps);
                Ajouter(code, {0xE9});
                const auto sautDebut = code.size();
                Ajouter32(code, 0);
                CorrigerRelatif32(code, sautDebut, debut);
                CorrigerRelatif32(code, sautFin, code.size());
                return;
            }
        }
    }

    CodeMachine GenerateurX64::Generer(const Programme& programme) const
    {
        CodeMachine resultat;
        std::unordered_set<std::string> fonctions;
        std::unordered_map<std::string, const Structure*> structures;
        for (const auto& fonction : programme.Fonctions) fonctions.insert(fonction.NomComplet());
        for (const auto& structure : programme.Structures) structures.emplace(structure.NomComplet(), &structure);

        for (const auto& structure : programme.Structures)
        {
            if (!structure.EstPolymorphe) continue;
            const auto decalage = Aligner(
                static_cast<std::uint32_t>(resultat.Donnees.size()), 8);
            resultat.Donnees.resize(decalage, 0);
            for (const auto& symboleMethode
                 : structure.SymbolesTableVirtuelle)
            {
                const auto position =
                    static_cast<std::uint32_t>(resultat.Donnees.size());
                resultat.Donnees.resize(resultat.Donnees.size() + 8, 0);
                resultat.Relocalisations.push_back({
                    position,
                    symboleMethode,
                    SectionMachine::Donnees,
                    TypeRelocalisationMachine::Adresse64
                });
            }
            resultat.Symboles.push_back({
                structure.SymboleTableVirtuelle,
                decalage,
                static_cast<std::uint32_t>(
                    structure.SymbolesTableVirtuelle.size() * 8U),
                false,
                SectionMachine::Donnees,
                true,
                GenreSymboleMachine::Objet,
                "GsAbi:x64-ms-v1:O(vtable<" + structure.NomComplet() + ">)",
                structure.Position
            });
        }

        for (const auto& variable : programme.VariablesGlobales)
        {
            if (variable.EstExterne)
            {
                resultat.Symboles.push_back({
                    variable.NomComplet(), 0, 0, false,
                    SectionMachine::Indefinie, false,
                    GenreSymboleMachine::Objet,
                    SignatureObjet(variable, structures),
                    variable.Position
                });
                continue;
            }
            const auto taille = TailleType(variable.Type, structures);
            const auto alignement = AlignementType(variable.Type, structures);
            SectionMachine section;
            std::uint32_t decalage;
            if (variable.EstInitialisee)
            {
                section = SectionMachine::Donnees;
                const auto alignee = Aligner(static_cast<std::uint32_t>(resultat.Donnees.size()), alignement);
                resultat.Donnees.resize(alignee, 0);
                decalage = alignee;
                if (variable.DonneesInitiales.size() == taille)
                {
                    resultat.Donnees.insert(
                        resultat.Donnees.end(),
                        variable.DonneesInitiales.begin(),
                        variable.DonneesInitiales.end());
                    for (const auto& relocalisation
                         : variable.RelocalisationsInitialiseur)
                        resultat.Relocalisations.push_back({
                            decalage + relocalisation.Decalage,
                            relocalisation.Symbole,
                            SectionMachine::Donnees,
                            TypeRelocalisationMachine::Adresse64
                        });
                }
                else
                {
                    const auto valeur = static_cast<std::uint64_t>(
                        variable.ValeurInitiale);
                    for (std::uint32_t index = 0; index < taille; ++index)
                        resultat.Donnees.push_back(static_cast<std::uint8_t>(
                            (valeur >> (index * 8)) & 0xFF));
                    if (!variable.SymboleInitialiseur.empty())
                    {
                        for (std::uint32_t index = 0; index < taille; ++index)
                            resultat.Donnees[decalage + index] = 0;
                        resultat.Relocalisations.push_back({
                            decalage,
                            variable.SymboleInitialiseur,
                            SectionMachine::Donnees,
                            TypeRelocalisationMachine::Adresse64
                        });
                    }
                }
            }
            else
            {
                section = SectionMachine::Zero;
                resultat.TailleZero = Aligner(resultat.TailleZero, alignement);
                decalage = resultat.TailleZero;
                resultat.TailleZero += taille;
            }
            resultat.Symboles.push_back({
                variable.NomComplet(), decalage, taille,
                variable.EstPublique, section, true,
                GenreSymboleMachine::Objet,
                SignatureObjet(variable, structures),
                variable.Position
            });
        }

        std::vector<const ExpressionChaine*> chainesRecensees;
        for (const auto& fonction : programme.Fonctions)
            if (!fonction.EstExterne)
            {
                RecenserChaines(*fonction.Corps, chainesRecensees);
                for (const auto& argument :
                     fonction.ArgumentsConstructeurBase)
                    RecenserChainesExpression(
                        *argument, chainesRecensees);
                for (const auto& argument :
                     fonction.ArgumentsConstructeurDelegue)
                    RecenserChainesExpression(
                        *argument, chainesRecensees);
                for (const auto& initialiseur :
                     fonction.InitialiseursChamps)
                {
                    for (const auto& argument : initialiseur.Arguments)
                        RecenserChainesExpression(
                            *argument, chainesRecensees);
                    if (initialiseur.InitialiseurParDefaut)
                        RecenserChainesExpression(
                            *initialiseur.InitialiseurParDefaut,
                            chainesRecensees);
                }
            }
        std::unordered_map<const Expression*, std::string> chaines;
        std::unordered_map<std::string, std::string> valeursChaines;
        for (const auto* chaine : chainesRecensees)
        {
            const auto existante = valeursChaines.find(chaine->Valeur);
            if (existante != valeursChaines.end())
            {
                chaines.emplace(chaine, existante->second);
                continue;
            }
            const auto nom = "@GsChaine"
                + std::to_string(valeursChaines.size());
            const auto decalage =
                static_cast<std::uint32_t>(resultat.Donnees.size());
            resultat.Donnees.insert(
                resultat.Donnees.end(),
                chaine->Valeur.begin(),
                chaine->Valeur.end());
            resultat.Donnees.push_back(0);
            resultat.Symboles.push_back({
                nom,
                decalage,
                static_cast<std::uint32_t>(chaine->Valeur.size() + 1),
                false,
                SectionMachine::Donnees,
                true,
                GenreSymboleMachine::Objet,
                "GsAbi:x64-ms-v1:O(litteral-utf8)",
                chaine->Position
            });
            valeursChaines.emplace(chaine->Valeur, nom);
            chaines.emplace(chaine, nom);
        }

        for (const auto& fonction : programme.Fonctions)
        {
            if (fonction.EstExterne)
            {
                resultat.Symboles.push_back({
                    fonction.NomComplet(), 0, 0, false,
                    SectionMachine::Indefinie, false,
                    GenreSymboleMachine::Fonction,
                    SignatureFonction(fonction, structures),
                    fonction.Position
                });
                continue;
            }
            const auto indexSymboleFonction = resultat.Symboles.size();
            resultat.Symboles.push_back({
                fonction.NomComplet(),
                static_cast<std::uint32_t>(resultat.Texte.size()),
                0,
                fonction.EstPublique,
                SectionMachine::Texte,
                true,
                GenreSymboleMachine::Fonction,
                SignatureFonction(fonction, structures),
                fonction.Position
            });
            if (fonction.Parametres.size() > 4)
                throw std::runtime_error("le prototype accepte au maximum quatre paramètres");

            ContexteFonction contexte;
            contexte.Machine = &resultat;
            contexte.Espace = fonction.Espace;
            contexte.Fonctions = &fonctions;
            contexte.Structures = &structures;
            contexte.Chaines = &chaines;
            contexte.FonctionActive = &fonction;

            std::vector<const InstructionVariable*> locales;
            RecenserVariables(*fonction.Corps, locales);
            std::uint32_t curseur = 0;
            if (fonction.TypeRetour.EstStructure())
            {
                curseur = Aligner(curseur, 8);
                curseur += 8;
                contexte.DecalageRetourStructure =
                    -static_cast<std::int32_t>(curseur);
            }
            auto allouer = [&](const std::string& nom, const TypeGs& type)
            {
                if (contexte.Variables.contains(nom))
                    throw std::runtime_error("variable ou paramètre déclaré plusieurs fois : " + nom);
                const auto alignement = AlignementType(type, structures);
                curseur = Aligner(curseur, alignement);
                curseur += TailleType(type, structures);
                contexte.Variables.emplace(
                    nom,
                    ContexteFonction::EmplacementVariable{-static_cast<std::int32_t>(curseur), type});
            };
            for (const auto& parametre : fonction.Parametres) allouer(parametre.Nom, parametre.Type);
            for (const auto* locale : locales) allouer(locale->Nom, locale->Type);
            std::vector<const Expression*> temporaires;
            RecenserTemporaires(*fonction.Corps, temporaires);
            for (const auto& argument :
                 fonction.ArgumentsConstructeurBase)
                RecenserTemporairesExpression(*argument, temporaires);
            for (const auto& argument :
                 fonction.ArgumentsConstructeurDelegue)
                RecenserTemporairesExpression(*argument, temporaires);
            for (const auto& initialiseur : fonction.InitialiseursChamps)
            {
                for (const auto& argument : initialiseur.Arguments)
                    RecenserTemporairesExpression(*argument, temporaires);
                if (initialiseur.InitialiseurParDefaut)
                    RecenserTemporairesExpression(
                        *initialiseur.InitialiseurParDefaut,
                        temporaires);
            }
            for (const auto* temporaire : temporaires)
            {
                const auto& type = temporaire->TypeSemantique;
                const auto alignement = AlignementType(type, structures);
                curseur = Aligner(curseur, alignement);
                curseur += TailleType(type, structures);
                contexte.TemporairesValeurs.emplace(
                    temporaire, -static_cast<std::int32_t>(curseur));
            }

            Ajouter(resultat.Texte, {0x55, 0x48, 0x89, 0xE5});
            const auto taillePile = Aligner(curseur, 16);
            if (taillePile > 0)
            {
                Ajouter(resultat.Texte, {0x48, 0x81, 0xEC});
                Ajouter32(resultat.Texte, static_cast<std::int32_t>(taillePile));
            }

            if (fonction.TypeRetour.EstStructure())
            {
                Ajouter(resultat.Texte, {0x48, 0x89, 0x8D}); // mov [rbp+disp32], rcx
                Ajouter32(resultat.Texte, contexte.DecalageRetourStructure);
            }

            for (std::size_t index = 0; index < fonction.Parametres.size(); ++index)
            {
                const auto& parametre = fonction.Parametres[index];
                const auto decalage = contexte.Variables.at(parametre.Nom).Decalage;
                const auto tailleParametre = TailleType(parametre.Type, structures);
                const auto registre = index
                    + (fonction.TypeRetour.EstStructure() ? 1U : 0U);
                if (parametre.Type.EstStructure())
                {
                    switch (registre)
                    {
                        case 0: break; // rcx contient déjà l’adresse source
                        case 1: Ajouter(resultat.Texte, {0x48, 0x89, 0xD1}); break;
                        case 2: Ajouter(resultat.Texte, {0x4C, 0x89, 0xC1}); break;
                        case 3: Ajouter(resultat.Texte, {0x4C, 0x89, 0xC9}); break;
                        default:
                            throw std::runtime_error(
                                "registre de paramètre de structure invalide");
                    }
                    Ajouter(resultat.Texte, {0x48, 0x8D, 0x85});
                    Ajouter32(resultat.Texte, decalage);
                    CopierMemoire(tailleParametre, resultat.Texte);
                    continue;
                }
                if (tailleParametre == 8)
                {
                    switch (registre)
                    {
                        case 0: Ajouter(resultat.Texte, {0x48, 0x89, 0x8D}); break;
                        case 1: Ajouter(resultat.Texte, {0x48, 0x89, 0x95}); break;
                        case 2: Ajouter(resultat.Texte, {0x4C, 0x89, 0x85}); break;
                        case 3: Ajouter(resultat.Texte, {0x4C, 0x89, 0x8D}); break;
                    }
                }
                else if (tailleParametre == 4)
                {
                    switch (registre)
                    {
                        case 0: Ajouter(resultat.Texte, {0x89, 0x8D}); break;
                        case 1: Ajouter(resultat.Texte, {0x89, 0x95}); break;
                        case 2: Ajouter(resultat.Texte, {0x44, 0x89, 0x85}); break;
                        case 3: Ajouter(resultat.Texte, {0x44, 0x89, 0x8D}); break;
                    }
                }
                else if (tailleParametre == 2)
                {
                    switch (registre)
                    {
                        case 0: Ajouter(resultat.Texte, {0x66, 0x89, 0x8D}); break;
                        case 1: Ajouter(resultat.Texte, {0x66, 0x89, 0x95}); break;
                        case 2: Ajouter(resultat.Texte, {0x66, 0x44, 0x89, 0x85}); break;
                        case 3: Ajouter(resultat.Texte, {0x66, 0x44, 0x89, 0x8D}); break;
                    }
                }
                else
                {
                    switch (registre)
                    {
                        case 0: Ajouter(resultat.Texte, {0x88, 0x8D}); break;
                        case 1: Ajouter(resultat.Texte, {0x88, 0x95}); break;
                        case 2: Ajouter(resultat.Texte, {0x44, 0x88, 0x85}); break;
                        case 3: Ajouter(resultat.Texte, {0x44, 0x88, 0x8D}); break;
                    }
                }
                Ajouter32(resultat.Texte, decalage);
            }

            if (fonction.EstConstructeur)
            {
                ExpressionVariable recepteur("soi", fonction.Position);
                recepteur.TypeSemantique = TypeGs{
                    GenreType::Structure,
                    fonction.ClasseProprietaire};
                recepteur.EstReference = true;
                auto initialiserTable = [&](const Structure& classe,
                                            std::uint32_t decalageObjet)
                {
                    if (!classe.EstPolymorphe) return;
                    GenererAdresse(recepteur, contexte);
                    if (decalageObjet != 0)
                    {
                        Ajouter(resultat.Texte, {0x48, 0x05});
                        Ajouter32(
                            resultat.Texte,
                            static_cast<std::int32_t>(decalageObjet));
                    }
                    Ajouter(resultat.Texte, {0x50, 0x48, 0x8D, 0x0D});
                    const auto position = static_cast<std::uint32_t>(
                        resultat.Texte.size());
                    Ajouter32(resultat.Texte, 0);
                    resultat.Relocalisations.push_back({
                        position,
                        classe.SymboleTableVirtuelle,
                        SectionMachine::Texte,
                        TypeRelocalisationMachine::Relatif32
                    });
                    Ajouter(resultat.Texte, {0x58});
                    if (classe.DecalageTableVirtuelle != 0)
                    {
                        Ajouter(resultat.Texte, {0x48, 0x05});
                        Ajouter32(
                            resultat.Texte,
                            static_cast<std::int32_t>(
                                classe.DecalageTableVirtuelle));
                    }
                    Ajouter(resultat.Texte, {0x48, 0x89, 0x08});
                };
                auto creerRecepteur = [&](std::uint32_t decalage,
                                          const std::string& classe)
                {
                    auto objet = std::make_unique<ExpressionVariable>(
                        "soi",
                        fonction.Position);
                    objet->TypeSemantique = TypeGs{
                        GenreType::Structure,
                        fonction.ClasseProprietaire};
                    objet->EstReference = true;
                    auto cible = std::make_unique<ExpressionMembre>(
                        std::move(objet),
                        std::string{},
                        false,
                        fonction.Position);
                    cible->DecalageMembre = decalage;
                    cible->TypeSemantique = TypeGs{
                        GenreType::Structure,
                        classe};
                    return cible;
                };

                if (!fonction.SymboleConstructeurDelegue.empty())
                {
                    std::vector<const Expression*> arguments{&recepteur};
                    for (const auto& argument :
                         fonction.ArgumentsConstructeurDelegue)
                        arguments.push_back(argument.get());
                    GenererAppel(
                        fonction.SymboleConstructeurDelegue,
                        nullptr,
                        arguments,
                        fonction
                            .ArgumentsConstructeurDelegueParReference,
                        TypeGs{GenreType::Vide},
                        nullptr,
                        false,
                        false,
                        0,
                        contexte);
                }
                if (fonction.SymboleConstructeurDelegue.empty())
                {
                if (!fonction.SymboleConstructeurBase.empty())
                {
                    std::vector<const Expression*> arguments{&recepteur};
                    for (const auto& argument :
                         fonction.ArgumentsConstructeurBase)
                        arguments.push_back(argument.get());
                    GenererAppel(
                        fonction.SymboleConstructeurBase,
                        nullptr,
                        arguments,
                        fonction.ArgumentsConstructeurBaseParReference,
                        TypeGs{GenreType::Vide},
                        nullptr,
                        false,
                        false,
                        0,
                        contexte);
                }
                for (const auto& nomBase :
                     fonction.ClassesBasesSansConstructeur)
                    initialiserTable(*structures.at(nomBase), 0);
                for (const auto& etape :
                     fonction.EtapesConstructionBaseImplicite)
                {
                    if (!etape.SymboleConstructeur.empty())
                    {
                        auto cible = creerRecepteur(
                            etape.Decalage,
                            etape.ClasseRecepteur.empty()
                                ? fonction.ClasseProprietaire
                                : etape.ClasseRecepteur);
                        const std::vector<const Expression*> arguments{
                            cible.get()};
                        const std::vector<bool> references{true};
                        GenererAppel(
                            etape.SymboleConstructeur,
                            nullptr,
                            arguments,
                            references,
                            TypeGs{GenreType::Vide},
                            nullptr,
                            false,
                            false,
                            0,
                            contexte);
                    }
                    else if (!etape.ClasseTableVirtuelle.empty())
                        initialiserTable(
                            *structures.at(etape.ClasseTableVirtuelle),
                            etape.Decalage);
                }
                initialiserTable(*structures.at(
                    fonction.ClasseProprietaire), 0);
                for (const auto& initialiseur :
                     fonction.InitialiseursChamps)
                {
                    if (initialiseur.EstObjetClasse)
                    {
                        if (!initialiseur.SymboleConstructeur.empty())
                        {
                            auto cible = creerRecepteur(
                                initialiseur.Decalage,
                                initialiseur.Type.Nom);
                            std::vector<const Expression*> arguments{
                                cible.get()};
                            for (const auto& argument :
                                 initialiseur.Arguments)
                                arguments.push_back(argument.get());
                            GenererAppel(
                                initialiseur.SymboleConstructeur,
                                nullptr,
                                arguments,
                                initialiseur
                                    .ArgumentsConstructeurParReference,
                                TypeGs{GenreType::Vide},
                                nullptr,
                                false,
                                false,
                                0,
                                contexte);
                        }
                        else
                            for (const auto& etape :
                                 initialiseur.EtapesConstructionImplicite)
                            {
                                const auto decalage =
                                    initialiseur.Decalage
                                    + etape.Decalage;
                                if (!etape.SymboleConstructeur.empty())
                                {
                                    auto cible = creerRecepteur(
                                        decalage,
                                        etape.ClasseRecepteur.empty()
                                            ? initialiseur.Type.Nom
                                            : etape.ClasseRecepteur);
                                    std::vector<const Expression*>
                                        arguments{cible.get()};
                                    for (const auto& argument :
                                         initialiseur.Arguments)
                                        arguments.push_back(argument.get());
                                    const std::vector<bool> references =
                                        initialiseur.Arguments.empty()
                                        ? std::vector<bool>{true}
                                        : initialiseur
                                            .ArgumentsConstructeurParReference;
                                    GenererAppel(
                                        etape.SymboleConstructeur,
                                        nullptr,
                                        arguments,
                                        references,
                                        TypeGs{GenreType::Vide},
                                        nullptr,
                                        false,
                                        false,
                                        0,
                                        contexte);
                                }
                                else if (!etape.ClasseTableVirtuelle.empty())
                                    initialiserTable(
                                        *structures.at(
                                            etape.ClasseTableVirtuelle),
                                        decalage);
                            }
                        continue;
                    }

                    GenererAdresse(recepteur, contexte);
                    if (initialiseur.Decalage != 0)
                    {
                        Ajouter(resultat.Texte, {0x48, 0x05});
                        Ajouter32(
                            resultat.Texte,
                            static_cast<std::int32_t>(
                                initialiseur.Decalage));
                    }
                    const auto* expressionInitialisation =
                        initialiseur.InitialiseurParDefaut
                        ? initialiseur.InitialiseurParDefaut
                        : initialiseur.Arguments.front().get();
                    GenererInitialiseurVersAdresse(
                        *expressionInitialisation,
                        initialiseur.Type,
                        contexte);
                }
                }
            }

            GenererInstruction(*fonction.Corps, contexte);
            if (fonction.TypeRetour.EstStructure())
            {
                Ajouter(resultat.Texte, {0x48, 0x8B, 0x85});
                Ajouter32(resultat.Texte, contexte.DecalageRetourStructure);
                InitialiserMemoireZero(
                    TailleType(fonction.TypeRetour, structures), resultat.Texte);
            }
            else Ajouter(resultat.Texte, {0x31, 0xC0});
            GenererEpilogue(resultat.Texte);
            resultat.Symboles[indexSymboleFonction].Taille =
                static_cast<std::uint32_t>(resultat.Texte.size())
                - resultat.Symboles[indexSymboleFonction].Decalage;
        }

        for (const auto& alias : programme.Aliases)
        {
            if (alias.GenreCible == GenreCibleAlias::Structure) continue;
            const auto cible = std::find_if(
                resultat.Symboles.begin(), resultat.Symboles.end(),
                [&](const SymboleMachine& symbole)
                {
                    return symbole.Nom == alias.CibleCanonique;
                });
            if (cible == resultat.Symboles.end())
                throw std::runtime_error(
                    "symbole machine cible d’alias introuvable : " + alias.CibleCanonique);

            // Un alias d’import est résolu vers le nom canonique pendant
            // l’analyse. Il ne doit pas créer un second import obligatoire.
            if (!cible->EstDefini) continue;
            auto symboleAlias = *cible;
            symboleAlias.Nom = alias.NomComplet();
            resultat.Symboles.push_back(std::move(symboleAlias));
        }

        // Une interface peut déclarer tout un module sans transformer chaque
        // prototype inutilisé en import obligatoire. Seuls les symboles qui
        // possèdent réellement une relocalisation restent dans l’objet.
        std::unordered_set<std::string> symbolesRelocalises;
        for (const auto& relocalisation : resultat.Relocalisations)
            symbolesRelocalises.insert(relocalisation.Symbole);
        resultat.Symboles.erase(
            std::remove_if(
                resultat.Symboles.begin(), resultat.Symboles.end(),
                [&](const SymboleMachine& symbole)
                {
                    return !symbole.EstDefini
                        && !symbolesRelocalises.contains(symbole.Nom);
                }),
            resultat.Symboles.end());
        return resultat;
    }
}
