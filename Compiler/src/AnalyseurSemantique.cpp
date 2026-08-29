#include "GsPP/AnalyseurSemantique.hpp"
#include "GsPP/ErreurCompilation.hpp"
#include "GsPP/Intrinseques.hpp"

#include <algorithm>
#include <climits>
#include <functional>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace GsPP
{
    namespace
    {
        std::uint32_t Aligner(std::uint32_t valeur, std::uint32_t alignement)
        {
            return (valeur + alignement - 1) & ~(alignement - 1);
        }

        TypeGs SansQualificateurs(TypeGs type)
        {
            type.EstConstante = false;
            type.EstVolatile = false;
            return type;
        }

        TypeGs SansReference(TypeGs type)
        {
            type.EstReference = false;
            return type;
        }

        std::string SuffixeSurcharge(const Fonction& fonction)
        {
            std::string signature;
            for (const auto& parametre : fonction.Parametres)
            {
                signature += parametre.Type.Afficher();
                signature.push_back(';');
            }
            std::uint64_t empreinte = 1469598103934665603ULL;
            for (const auto octet : signature)
            {
                empreinte ^= static_cast<unsigned char>(octet);
                empreinte *= 1099511628211ULL;
            }
            std::ostringstream sortie;
            sortie << '$' << std::hex << std::uppercase << std::setw(16)
                   << std::setfill('0') << empreinte;
            return sortie.str();
        }

        bool TypesValeurEgaux(const TypeGs& gauche, const TypeGs& droite)
        {
            if (gauche.EstAdresse() || droite.EstAdresse()) return gauche == droite;
            return SansQualificateurs(gauche) == SansQualificateurs(droite);
        }

        TypeGs CreerPointeurFonction(const Fonction& fonction)
        {
            TypeGs type{GenreType::PointeurFonction};
            type.RetourFonction = std::make_shared<TypeGs>(fonction.TypeRetour);
            type.ParametresFonction.reserve(fonction.Parametres.size());
            for (const auto& parametre : fonction.Parametres)
                type.ParametresFonction.push_back(parametre.Type);
            return type;
        }

        std::uint32_t LargeurType(const TypeGs& type)
        {
            switch (type.Genre)
            {
                case GenreType::Entier8:
                case GenreType::Naturel8:
                case GenreType::Booleen:
                case GenreType::Octet:
                case GenreType::Caractere:
                    return 8;
                case GenreType::Entier16:
                case GenreType::Naturel16:
                    return 16;
                case GenreType::Entier32:
                case GenreType::Naturel32:
                case GenreType::Enumeration:
                    return 32;
                case GenreType::Entier64:
                case GenreType::Naturel64:
                    return 64;
                default:
                    return 0;
            }
        }

        bool EstTypeSigne(const TypeGs& type)
        {
            return type.EstEntierSigne() || type.EstEnumeration();
        }

        std::uint64_t NormaliserBits(std::uint64_t valeur, const TypeGs& type)
        {
            const auto largeur = LargeurType(type);
            if (largeur == 0 || largeur == 64) return valeur;
            return valeur & ((std::uint64_t{1} << largeur) - 1);
        }

        std::int64_t InterpreterSigne(std::uint64_t valeur, const TypeGs& type)
        {
            const auto largeur = LargeurType(type);
            valeur = NormaliserBits(valeur, type);
            if (largeur == 64)
            {
                if (valeur <= static_cast<std::uint64_t>(INT64_MAX))
                    return static_cast<std::int64_t>(valeur);
                return -1 - static_cast<std::int64_t>(
                    std::numeric_limits<std::uint64_t>::max() - valeur);
            }
            const auto signe = std::uint64_t{1} << (largeur - 1);
            if ((valeur & signe) == 0) return static_cast<std::int64_t>(valeur);
            const auto masque = (std::uint64_t{1} << largeur) - 1;
            const auto magnitude = ((~valeur) & masque) + 1;
            return -static_cast<std::int64_t>(magnitude);
        }

        bool ValeurDansType(
            std::uint64_t valeur,
            const TypeGs& source,
            const TypeGs& destination)
        {
            if (destination.EstAdresse()
                || destination.EstTableau()
                || destination.EstStructure()
                || LargeurType(destination) == 0)
                return false;

            if (destination.EstBooleen())
            {
                if (EstTypeSigne(source))
                {
                    const auto signee = InterpreterSigne(valeur, source);
                    return signee == 0 || signee == 1;
                }
                const auto nonSignee = NormaliserBits(valeur, source);
                return nonSignee == 0 || nonSignee == 1;
            }

            const auto largeurDestination = LargeurType(destination);
            const bool destinationSignee = EstTypeSigne(destination);
            if (EstTypeSigne(source))
            {
                const auto signee = InterpreterSigne(valeur, source);
                if (!destinationSignee)
                {
                    if (signee < 0) return false;
                    const auto maximum = largeurDestination == 64
                        ? std::numeric_limits<std::uint64_t>::max()
                        : (std::uint64_t{1} << largeurDestination) - 1;
                    return static_cast<std::uint64_t>(signee) <= maximum;
                }
                if (largeurDestination == 64) return true;
                const auto maximum = static_cast<std::int64_t>(
                    (std::uint64_t{1} << (largeurDestination - 1)) - 1);
                const auto minimum = -maximum - 1;
                return signee >= minimum && signee <= maximum;
            }

            const auto nonSignee = NormaliserBits(valeur, source);
            if (destinationSignee)
            {
                const auto maximum = largeurDestination == 64
                    ? static_cast<std::uint64_t>(INT64_MAX)
                    : (std::uint64_t{1} << (largeurDestination - 1)) - 1;
                return nonSignee <= maximum;
            }
            const auto maximum = largeurDestination == 64
                ? std::numeric_limits<std::uint64_t>::max()
                : (std::uint64_t{1} << largeurDestination) - 1;
            return nonSignee <= maximum;
        }
    }

    [[noreturn]] void AnalyseurSemantique::Erreur(
        const PositionSource& position,
        std::string francais,
        std::string anglais) const
    {
        throw ErreurCompilation(
            std::move(francais),
            std::move(anglais),
            position.Ligne,
            position.Colonne,
            position.Fichier);
    }

    std::uint32_t AnalyseurSemantique::TailleType(
        const TypeGs& type,
        const std::unordered_map<std::string, Structure*>& structures)
    {
        if (type.EstTableau())
        {
            const auto element = type.ElementTableau();
            const auto tailleElement = TailleType(element, structures);
            const auto total = static_cast<std::uint64_t>(type.DimensionsTableau.front())
                * tailleElement;
            if (total > std::numeric_limits<std::uint32_t>::max())
                throw std::runtime_error("taille de tableau supérieure à 4 Gio");
            return static_cast<std::uint32_t>(total);
        }
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

    std::uint32_t AnalyseurSemantique::AlignementType(
        const TypeGs& type,
        const std::unordered_map<std::string, Structure*>& structures)
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

    void AnalyseurSemantique::ResoudreType(
        TypeGs& type,
        const std::string& espace,
        const PositionSource& position)
    {
        if (type.EstReference && type.EstTableau())
            Erreur(position,
                   "une référence de tableau n’est pas prise en charge",
                   "array references are unsupported");
        if (type.Genre == GenreType::PointeurFonction)
        {
            if (!type.RetourFonction)
                Erreur(
                    position,
                    "signature de pointeur de fonction incomplète",
                    "incomplete function pointer signature");
            ResoudreType(*type.RetourFonction, espace, position);
            for (auto& parametre : type.ParametresFonction)
                ResoudreType(parametre, espace, position);
            if (type.ParametresFonction.size() > 4)
                Erreur(
                    position,
                    "un pointeur de fonction accepte au maximum quatre paramètres",
                    "a function pointer accepts at most four parameters");
            if (type.RetourFonction->EstTableau())
                Erreur(
                    position,
                    "type de retour de pointeur de fonction non pris en charge",
                    "unsupported function pointer return type");
            for (const auto& parametre : type.ParametresFonction)
                if (parametre.EstVide()
                    || parametre.EstTableau())
                    Erreur(
                        position,
                        "type de paramètre de pointeur de fonction non pris en charge",
                        "unsupported function pointer parameter type");
            if (type.RetourFonction->EstStructure()
                && type.ParametresFonction.size() > 3)
                Erreur(
                    position,
                    "un callback retournant une structure accepte au maximum trois paramètres",
                    "a callback returning a struct accepts at most three parameters");
            return;
        }
        if (type.Genre != GenreType::Structure) return;
        if (const auto trouve = _Structures.find(type.Nom); trouve != _Structures.end())
        {
            type.Nom = trouve->second->NomComplet();
            return;
        }
        if (const auto trouve = _Enumerations.find(type.Nom); trouve != _Enumerations.end())
        {
            type.Genre = GenreType::Enumeration;
            type.Nom = trouve->second->NomComplet();
            return;
        }
        if (!espace.empty())
        {
            const auto local = espace + "::" + type.Nom;
            if (const auto trouve = _Structures.find(local); trouve != _Structures.end())
            {
                type.Nom = trouve->second->NomComplet();
                return;
            }
            if (const auto trouve = _Enumerations.find(local); trouve != _Enumerations.end())
            {
                type.Genre = GenreType::Enumeration;
                type.Nom = trouve->second->NomComplet();
                return;
            }
        }
        Erreur(
            position,
            "type de structure ou d’énumération introuvable : " + type.Nom,
            "unknown struct or enum type: " + type.Nom);
    }

    void AnalyseurSemantique::ResoudreHeritage(Structure& structure)
    {
        if (structure.ClasseBase.empty()) return;
        if (!structure.EstClasse)
            Erreur(
                structure.Position,
                "seule une classe peut déclarer une classe de base",
                "only a class can declare a base class");
        if (structure.VisibiliteHeritage != VisibiliteMembre::Publique)
            Erreur(
                structure.Position,
                "Gs++ 0.22 prend en charge uniquement l’héritage public",
                "Gs++ 0.22 supports public inheritance only");

        TypeGs typeBase{GenreType::Structure, structure.ClasseBase};
        ResoudreType(typeBase, structure.Espace, structure.Position);
        const auto trouve = _Structures.find(typeBase.Nom);
        if (trouve == _Structures.end() || !trouve->second->EstClasse)
            Erreur(
                structure.Position,
                "la base d’une classe doit être une classe : "
                    + structure.ClasseBase,
                "a class base must be a class: " + structure.ClasseBase);
        if (typeBase.Nom == structure.NomComplet())
            Erreur(
                structure.Position,
                "une classe ne peut pas hériter d’elle-même",
                "a class cannot inherit from itself");
        structure.ClasseBaseCanonique = typeBase.Nom;
    }

    std::string AnalyseurSemantique::QualifierNomFonction(
        const std::string& nom,
        const Fonction& contexte) const
    {
        if (_Surcharges.contains(nom)) return nom;
        auto espace = contexte.Espace;
        while (!espace.empty())
        {
            const auto candidat = espace + "::" + nom;
            if (_Surcharges.contains(candidat)) return candidat;
            const auto separateur = espace.rfind("::");
            if (separateur == std::string::npos) break;
            espace.resize(separateur);
        }
        return nom;
    }

    bool AnalyseurSemantique::EstDeriveDe(
        const std::string& classe,
        const std::string& base) const
    {
        if (classe.empty() || base.empty()) return false;
        std::string courant = classe;
        std::unordered_set<std::string> visites;
        while (visites.insert(courant).second)
        {
            if (courant == base) return true;
            const auto trouve = _Structures.find(courant);
            if (trouve == _Structures.end()
                || trouve->second->ClasseBaseCanonique.empty())
                return false;
            courant = trouve->second->ClasseBaseCanonique;
        }
        return false;
    }

    bool AnalyseurSemantique::ConversionHeritageAutorisee(
        const TypeGs& source,
        const TypeGs& destination,
        bool liaisonReference) const
    {
        if (source.Genre != GenreType::Structure
            || destination.Genre != GenreType::Structure
            || source.EstTableau() || destination.EstTableau()
            || source.NiveauPointeur != destination.NiveauPointeur
            || source.NiveauPointeur > 1
            || (source.NiveauPointeur == 0 && !liaisonReference)
            || source.EstReference || destination.EstReference)
            return false;
        if (source.EstConstante && !destination.EstConstante) return false;
        return source.Nom != destination.Nom
            && EstDeriveDe(source.Nom, destination.Nom);
    }

    std::string AnalyseurSemantique::TrouverNomMethode(
        const std::string& classe,
        const std::string& nom) const
    {
        std::string courant = classe;
        std::unordered_set<std::string> visites;
        while (visites.insert(courant).second)
        {
            const auto candidat = courant + "::" + nom;
            if (_Surcharges.contains(candidat)) return candidat;
            const auto trouve = _Structures.find(courant);
            if (trouve == _Structures.end()
                || trouve->second->ClasseBaseCanonique.empty())
                break;
            courant = trouve->second->ClasseBaseCanonique;
        }
        return {};
    }

    const ChampStructure* AnalyseurSemantique::TrouverChamp(
        const Structure& structure,
        const std::string& nom,
        const Structure*& proprietaire) const
    {
        const Structure* courante = &structure;
        std::unordered_set<std::string> visites;
        while (courante
               && visites.insert(courante->NomComplet()).second)
        {
            auto nomChamp = nom;
            auto trouve = std::find_if(
                courante->Champs.begin(), courante->Champs.end(),
                [&](const ChampStructure& champ)
                { return champ.Nom == nomChamp; });
            if (trouve == courante->Champs.end())
            {
                const auto alias = std::find_if(
                    courante->AliasesChamps.begin(),
                    courante->AliasesChamps.end(),
                    [&](const AliasChamp& valeur)
                    { return valeur.Nom == nomChamp; });
                if (alias != courante->AliasesChamps.end())
                {
                    nomChamp = alias->CibleCanonique;
                    trouve = std::find_if(
                        courante->Champs.begin(), courante->Champs.end(),
                        [&](const ChampStructure& champ)
                        { return champ.Nom == nomChamp; });
                }
            }
            if (trouve != courante->Champs.end())
            {
                proprietaire = courante;
                return &*trouve;
            }
            if (courante->ClasseBaseCanonique.empty()) break;
            courante = _Structures.at(courante->ClasseBaseCanonique);
        }
        proprietaire = nullptr;
        return nullptr;
    }

    bool AnalyseurSemantique::AccesMembreAutorise(
        VisibiliteMembre visibilite,
        const std::string& classe,
        const Fonction& contexte) const
    {
        return visibilite == VisibiliteMembre::Publique
            || contexte.ClasseProprietaire == classe
            || (visibilite == VisibiliteMembre::Protegee
                && EstDeriveDe(contexte.ClasseProprietaire, classe));
    }

    bool AnalyseurSemantique::AccesDepuisClasseAutorise(
        VisibiliteMembre visibilite,
        const std::string& classe,
        const std::string& classeContexte) const
    {
        return visibilite == VisibiliteMembre::Publique
            || classeContexte == classe
            || (visibilite == VisibiliteMembre::Protegee
                && EstDeriveDe(classeContexte, classe));
    }

    Fonction* AnalyseurSemantique::ResoudreConstructeurClasse(
        const Structure& classe,
        std::vector<Expression*>& argumentsUtilisateur,
        Fonction& fonction,
        const PositionSource& position,
        const std::string& classeContexte,
        std::vector<bool>& argumentsParReference,
        bool estBase)
    {
        static constexpr const char* nomRecepteur =
            "@recepteur_construction_implicite";
        _Variables.emplace(
            nomRecepteur,
            TypeGs{GenreType::Structure, classe.NomComplet()});
        ExpressionVariable recepteur(nomRecepteur, position);
        std::vector<Expression*> arguments{&recepteur};
        arguments.insert(
            arguments.end(),
            argumentsUtilisateur.begin(),
            argumentsUtilisateur.end());
        auto* constructeur = ResoudreSurcharge(
            classe.NomComplet() + "::$constructeur",
            arguments,
            fonction,
            position);
        _Variables.erase(nomRecepteur);

        if (!AccesDepuisClasseAutorise(
                constructeur->Visibilite,
                constructeur->ClasseProprietaire,
                classeContexte))
            Erreur(
                position,
                estBase
                    ? "constructeur de base inaccessible : "
                        + classe.NomComplet()
                    : "constructeur inaccessible : "
                        + classe.NomComplet(),
                estBase
                    ? "base constructor is inaccessible: "
                        + classe.NomComplet()
                    : "constructor is inaccessible: "
                        + classe.NomComplet());

        argumentsParReference.clear();
        for (const auto& parametre : constructeur->Parametres)
            argumentsParReference.push_back(parametre.Type.EstReference);
        for (std::size_t index = 1;
             index < constructeur->Parametres.size();
             ++index)
            if (!constructeur->Parametres[index].Type.EstReference)
                (void)AnalyserInitialiseur(
                    *arguments[index],
                    constructeur->Parametres[index].Type,
                    fonction);
        return constructeur;
    }

    void AnalyseurSemantique::PlanifierConstructionImplicite(
        const Structure& classe,
        std::uint32_t decalage,
        const std::string& classeContexte,
        Fonction& fonction,
        const PositionSource& position,
        std::vector<EtapeConstructionClasse>& etapes)
    {
        const auto nomConstructeur =
            classe.NomComplet() + "::$constructeur";
        if (_Surcharges.contains(nomConstructeur))
        {
            std::vector<Expression*> sansArguments;
            std::vector<bool> argumentsParReference;
            auto* constructeur = ResoudreConstructeurClasse(
                classe,
                sansArguments,
                fonction,
                position,
                classeContexte,
                argumentsParReference,
                EstDeriveDe(classeContexte, classe.NomComplet()));
            etapes.push_back({
                decalage,
                constructeur->NomComplet(),
                {},
                classe.NomComplet()
            });
            return;
        }

        if (!classe.ClasseBaseCanonique.empty())
            PlanifierConstructionImplicite(
                *_Structures.at(classe.ClasseBaseCanonique),
                decalage,
                classe.NomComplet(),
                fonction,
                position,
                etapes);

        if (classe.EstPolymorphe)
            etapes.push_back({decalage, {}, classe.NomComplet(), {}});

        for (const auto& champ : classe.Champs)
            PlanifierConstructionTypeObjet(
                champ.Type,
                decalage + champ.Decalage,
                classe.NomComplet(),
                fonction,
                champ.Position,
                etapes);
    }

    void AnalyseurSemantique::PlanifierConstructionTypeObjet(
        const TypeGs& type,
        std::uint32_t decalage,
        const std::string& classeContexte,
        Fonction& fonction,
        const PositionSource& position,
        std::vector<EtapeConstructionClasse>& etapes)
    {
        if (type.EstTableau())
        {
            const auto element = type.ElementTableau();
            const auto tailleElement = TailleType(element, _Structures);
            for (std::uint32_t index = 0;
                 index < type.DimensionsTableau.front();
                 ++index)
                PlanifierConstructionTypeObjet(
                    element,
                    decalage + index * tailleElement,
                    classeContexte,
                    fonction,
                    position,
                    etapes);
            return;
        }
        if (type.EstStructure() && _Structures.at(type.Nom)->EstClasse)
            PlanifierConstructionImplicite(
                *_Structures.at(type.Nom),
                decalage,
                classeContexte,
                fonction,
                position,
                etapes);
    }

    void AnalyseurSemantique::PlanifierConstructionTableauAvecConstructeur(
        const TypeGs& type,
        std::uint32_t decalage,
        const std::string& symboleConstructeur,
        const std::string& classeRecepteur,
        std::vector<EtapeConstructionClasse>& etapes)
    {
        if (type.EstTableau())
        {
            const auto element = type.ElementTableau();
            const auto tailleElement = TailleType(element, _Structures);
            for (std::uint32_t index = 0;
                 index < type.DimensionsTableau.front();
                 ++index)
                PlanifierConstructionTableauAvecConstructeur(
                    element,
                    decalage + index * tailleElement,
                    symboleConstructeur,
                    classeRecepteur,
                    etapes);
            return;
        }
        etapes.push_back({
            decalage,
            symboleConstructeur,
            {},
            classeRecepteur
        });
    }

    void AnalyseurSemantique::PlanifierDestructionClasse(
        const Structure& classe,
        std::uint32_t decalage,
        const std::string& classeContexte,
        const PositionSource& position,
        std::vector<ActionDestructionClasse>& actions)
    {
        const auto nomDestructeur =
            classe.NomComplet() + "::$destructeur";
        if (_Surcharges.contains(nomDestructeur))
        {
            auto* destructeur = _Surcharges.at(nomDestructeur).front();
            if (!AccesDepuisClasseAutorise(
                    destructeur->Visibilite,
                    destructeur->ClasseProprietaire,
                    classeContexte))
            {
                const bool estBase = EstDeriveDe(
                    classeContexte,
                    classe.NomComplet());
                Erreur(
                    position,
                    estBase
                        ? "destructeur de base inaccessible : "
                            + classe.NomComplet()
                        : "destructeur inaccessible : "
                            + classe.NomComplet(),
                    estBase
                        ? "base destructor is inaccessible: "
                            + classe.NomComplet()
                        : "destructor is inaccessible: "
                            + classe.NomComplet());
            }
            actions.push_back({
                decalage,
                destructeur->NomComplet(),
                classe.NomComplet()
            });
        }

        for (auto champ = classe.Champs.rbegin();
             champ != classe.Champs.rend();
             ++champ)
            PlanifierDestructionTypeObjet(
                champ->Type,
                decalage + champ->Decalage,
                classe.NomComplet(),
                champ->Position,
                actions);

        if (!classe.ClasseBaseCanonique.empty())
            PlanifierDestructionClasse(
                *_Structures.at(classe.ClasseBaseCanonique),
                decalage,
                classe.NomComplet(),
                position,
                actions);
    }

    void AnalyseurSemantique::PlanifierDestructionTypeObjet(
        const TypeGs& type,
        std::uint32_t decalage,
        const std::string& classeContexte,
        const PositionSource& position,
        std::vector<ActionDestructionClasse>& actions)
    {
        if (type.EstTableau())
        {
            const auto element = type.ElementTableau();
            const auto tailleElement = TailleType(element, _Structures);
            for (std::uint32_t index = type.DimensionsTableau.front();
                 index > 0;
                 --index)
                PlanifierDestructionTypeObjet(
                    element,
                    decalage + (index - 1) * tailleElement,
                    classeContexte,
                    position,
                    actions);
            return;
        }
        if (type.EstStructure() && _Structures.at(type.Nom)->EstClasse)
            PlanifierDestructionClasse(
                *_Structures.at(type.Nom),
                decalage,
                classeContexte,
                position,
                actions);
    }

    Fonction* AnalyseurSemantique::ResoudreSurcharge(
        const std::string& nom,
        std::vector<Expression*>& arguments,
        Fonction& contexte,
        const PositionSource& position)
    {
        const auto qualifie = QualifierNomFonction(nom, contexte);
        const auto groupe = _Surcharges.find(qualifie);
        if (groupe == _Surcharges.end())
            Erreur(position,
                   "fonction introuvable : " + nom,
                   "unknown function: " + nom);

        Fonction* meilleure = nullptr;
        unsigned meilleurScore = std::numeric_limits<unsigned>::max();
        bool ambigu = false;
        for (auto* candidate : groupe->second)
        {
            if (candidate->Parametres.size() != arguments.size()) continue;
            unsigned score = 0;
            bool compatible = true;
            for (std::size_t index = 0; index < arguments.size(); ++index)
            {
                auto& expression = *arguments[index];
                const auto& parametre = candidate->Parametres[index].Type;
                if (expression.Genre == GenreExpression::Agregat)
                {
                    if (parametre.EstReference)
                    {
                        compatible = false;
                        break;
                    }
                    score += 2;
                    continue;
                }
                const auto source = AnalyserExpression(expression, contexte);
                if (parametre.EstReference)
                {
                    const auto destination = SansReference(parametre);
                    if (!EstValeurGauche(expression)
                        || (!TypesValeurEgaux(source, destination)
                            && !ConversionHeritageAutorisee(
                                source, destination, true))
                        || (!destination.EstConstante
                            && expression.EstValeurConstante))
                    {
                        compatible = false;
                        break;
                    }
                    if (!TypesValeurEgaux(source, destination)) ++score;
                    continue;
                }
                if (TypesValeurEgaux(source, parametre)) continue;
                if (ConversionHeritageAutorisee(source, parametre))
                {
                    ++score;
                    continue;
                }
                if (EstExpressionConstante(expression)
                    && source.EstEntier()
                    && parametre.EstEntier()
                    && ValeurDansType(
                        EvaluerConstante(expression), source, parametre))
                {
                    ++score;
                    continue;
                }
                compatible = false;
                break;
            }
            if (!compatible) continue;
            if (score < meilleurScore)
            {
                meilleure = candidate;
                meilleurScore = score;
                ambigu = false;
            }
            else if (score == meilleurScore) ambigu = true;
        }
        if (!meilleure)
            Erreur(position,
                   "aucune surcharge compatible pour " + nom,
                   "no matching overload for " + nom);
        if (ambigu)
            Erreur(position,
                   "appel de surcharge ambigu pour " + nom,
                   "ambiguous overload call for " + nom);
        return meilleure;
    }

    void AnalyseurSemantique::PreparerAppel(
        ExpressionAppel& appel,
        Fonction& cible,
        std::vector<Expression*>& arguments,
        Fonction& contexte)
    {
        appel.NomDirect = cible.NomComplet();
        appel.EstVirtuel = cible.EstVirtuelle
            && !appel.ForcerAppelDirect;
        appel.IndexVirtuel = cible.IndexVirtuel;
        if (cible.EstVirtuelle)
            appel.DecalageTableVirtuelle = _Structures.at(
                cible.ClasseProprietaire)->DecalageTableVirtuelle;
        appel.ArgumentsParReference.clear();
        appel.ArgumentsParReference.reserve(arguments.size());
        for (std::size_t index = 0; index < arguments.size(); ++index)
        {
            const auto& typeParametre = cible.Parametres[index].Type;
            appel.ArgumentsParReference.push_back(typeParametre.EstReference);
            if (!typeParametre.EstReference)
                (void)AnalyserInitialiseur(
                    *arguments[index], typeParametre, contexte);
        }
        auto* variable = dynamic_cast<ExpressionVariable*>(appel.Cible.get());
        if (variable)
        {
            variable->Nom = cible.NomComplet();
            variable->EstFonction = true;
            variable->TypeSemantique = CreerPointeurFonction(cible);
        }
        appel.RetourneReference = cible.TypeRetour.EstReference;
        appel.TypeSemantique = cible.TypeRetour.EstReference
            ? SansReference(cible.TypeRetour)
            : cible.TypeRetour;
    }

    void AnalyseurSemantique::IndexerFonctions(Programme& programme)
    {
        _Fonctions.clear();
        _Surcharges.clear();
        for (auto& fonction : programme.Fonctions)
            _Surcharges[fonction.NomSourceComplet()].push_back(&fonction);

        for (auto& [nom, groupe] : _Surcharges)
        {
            for (std::size_t gauche = 0; gauche < groupe.size(); ++gauche)
                for (std::size_t droite = gauche + 1; droite < groupe.size(); ++droite)
                {
                    if (groupe[gauche]->Parametres.size()
                        != groupe[droite]->Parametres.size()) continue;
                    bool identiques = true;
                    for (std::size_t index = 0;
                         index < groupe[gauche]->Parametres.size(); ++index)
                        identiques = identiques
                            && groupe[gauche]->Parametres[index].Type
                                == groupe[droite]->Parametres[index].Type;
                    if (identiques)
                        Erreur(
                            groupe[droite]->Position,
                            "surcharge déclarée plusieurs fois : " + nom,
                            "overload declared more than once: " + nom);
                }
            if (groupe.size() > 1)
                for (auto* fonction : groupe)
                    fonction->NomLien = fonction->Nom + SuffixeSurcharge(*fonction);
        }

        for (auto& structure : programme.Structures)
        {
            structure.ClesMethodesVirtuelles.clear();
            structure.SymbolesTableVirtuelle.clear();
            structure.MethodesVirtuellesAbi.clear();
        }
        for (auto& fonction : programme.Fonctions)
        {
            if (!_Fonctions.emplace(fonction.NomComplet(), &fonction).second)
                Erreur(fonction.Position,
                       "collision de symbole de fonction : " + fonction.NomComplet(),
                       "function symbol collision: " + fonction.NomComplet());
        }

        auto cleVirtuelle = [](const Fonction& fonction)
        {
            std::string cle = fonction.NomSource + '(';
            for (std::size_t index = 1;
                 index < fonction.Parametres.size(); ++index)
            {
                if (index != 1) cle.push_back(',');
                cle += fonction.Parametres[index].Type.Afficher();
            }
            cle += ")->" + fonction.TypeRetour.Afficher();
            return cle;
        };

        std::unordered_set<std::string> tablesConstruites;
        std::unordered_set<std::string> tablesEnCours;
        std::function<void(Structure&)> construireTable;
        construireTable = [&](Structure& structure)
        {
            const auto nomClasse = structure.NomComplet();
            if (tablesConstruites.contains(nomClasse)) return;
            if (!tablesEnCours.insert(nomClasse).second)
                Erreur(
                    structure.Position,
                    "cycle d’héritage détecté pour " + nomClasse,
                    "inheritance cycle detected for " + nomClasse);

            if (!structure.ClasseBaseCanonique.empty())
            {
                auto& base = *_Structures.at(structure.ClasseBaseCanonique);
                construireTable(base);
                structure.ClesMethodesVirtuelles =
                    base.ClesMethodesVirtuelles;
                structure.SymbolesTableVirtuelle =
                    base.SymbolesTableVirtuelle;
                structure.MethodesVirtuellesAbi =
                    base.MethodesVirtuellesAbi;
            }

            for (auto& fonction : programme.Fonctions)
            {
                if (!fonction.EstMethode
                    || fonction.EstConstructeur
                    || fonction.ClasseProprietaire != nomClasse)
                    continue;
                const auto cle = cleVirtuelle(fonction);
                const auto trouve = std::find(
                    structure.ClesMethodesVirtuelles.begin(),
                    structure.ClesMethodesVirtuelles.end(), cle);
                const bool existe =
                    trouve != structure.ClesMethodesVirtuelles.end();

                if (fonction.EstRemplacement)
                {
                    if (!existe)
                        Erreur(
                            fonction.Position,
                            "aucune méthode virtuelle héritée compatible à remplacer : "
                                + fonction.NomSource,
                            "no matching inherited virtual method to override: "
                                + fonction.NomSource);
                    const auto index = static_cast<std::size_t>(
                        std::distance(
                            structure.ClesMethodesVirtuelles.begin(), trouve));
                    fonction.EstVirtuelle = true;
                    fonction.IndexVirtuel = static_cast<std::uint32_t>(index);
                    structure.SymbolesTableVirtuelle[index] =
                        fonction.NomComplet();
                    structure.MethodesVirtuellesAbi[index] =
                        std::to_string(index) + ':' + cle + '=' + nomClasse;
                    continue;
                }

                if (existe)
                    Erreur(
                        fonction.Position,
                        "une méthode virtuelle héritée doit utiliser remplacer/override : "
                            + fonction.NomSource,
                        "an inherited virtual method must use override: "
                            + fonction.NomSource);
                if (!fonction.EstVirtuelle) continue;

                fonction.IndexVirtuel = static_cast<std::uint32_t>(
                    structure.ClesMethodesVirtuelles.size());
                structure.ClesMethodesVirtuelles.push_back(cle);
                structure.SymbolesTableVirtuelle.push_back(
                    fonction.NomComplet());
                structure.MethodesVirtuellesAbi.push_back(
                    std::to_string(fonction.IndexVirtuel) + ':' + cle
                        + '=' + nomClasse);
            }

            if (!structure.ClesMethodesVirtuelles.empty())
                structure.EstPolymorphe = true;
            tablesEnCours.erase(nomClasse);
            tablesConstruites.insert(nomClasse);
        };
        for (auto& structure : programme.Structures)
            if (structure.EstClasse) construireTable(structure);
    }

    void AnalyseurSemantique::ResoudreAlias(DeclarationAlias& alias)
    {
        const auto nomAlias = alias.NomComplet();
        if (_AliasesResolus.contains(nomAlias)) return;
        if (!_AliasesEnCours.insert(nomAlias).second)
            Erreur(
                alias.Position,
                "cycle d’alias détecté pour " + nomAlias,
                "alias cycle detected for " + nomAlias);

        std::vector<std::string> candidats{alias.Cible};
        if (!alias.Espace.empty())
            candidats.push_back(alias.Espace + "::" + alias.Cible);

        bool trouve = false;
        for (const auto& candidat : candidats)
        {
            const bool estStructure = _Structures.contains(candidat);
            const bool estFonction = _Surcharges.contains(candidat);
            const bool estGlobale = _Globales.contains(candidat);
            const bool estAlias = _Aliases.contains(candidat);
            const auto nombre = static_cast<int>(estStructure)
                + static_cast<int>(estFonction)
                + static_cast<int>(estGlobale)
                + static_cast<int>(estAlias);
            if (nombre == 0) continue;
            if (nombre > 1)
                Erreur(
                    alias.Position,
                    "cible d’alias ambiguë : " + alias.Cible,
                    "ambiguous alias target: " + alias.Cible);

            if (estAlias)
            {
                auto& cible = *_Aliases.at(candidat);
                ResoudreAlias(cible);
                alias.GenreCible = cible.GenreCible;
                alias.CibleCanonique = cible.CibleCanonique;
            }
            else if (estStructure)
            {
                alias.GenreCible = GenreCibleAlias::Structure;
                alias.CibleCanonique = _Structures.at(candidat)->NomComplet();
            }
            else if (estFonction)
            {
                if (_Surcharges.at(candidat).size() != 1)
                    Erreur(
                        alias.Position,
                        "un alias de fonction surchargée est ambigu : " + alias.Cible,
                        "an overloaded function alias is ambiguous: " + alias.Cible);
                alias.GenreCible = GenreCibleAlias::Fonction;
                alias.CibleCanonique =
                    _Surcharges.at(candidat).front()->NomComplet();
            }
            else
            {
                alias.GenreCible = GenreCibleAlias::VariableGlobale;
                alias.CibleCanonique = _Globales.at(candidat)->NomComplet();
            }
            trouve = true;
            break;
        }

        if (!trouve)
            Erreur(
                alias.Position,
                "cible d’alias introuvable : " + alias.Cible,
                "unknown alias target: " + alias.Cible);

        _AliasesEnCours.erase(nomAlias);
        _AliasesResolus.insert(nomAlias);
    }

    void AnalyseurSemantique::CalculerEnumeration(Enumeration& enumeration)
    {
        const auto nomComplet = enumeration.NomComplet();
        if (enumeration.Valeurs.empty())
            Erreur(
                enumeration.Position,
                "une énumération doit contenir au moins une valeur",
                "an enum must contain at least one value");

        std::unordered_set<std::string> noms;
        std::int64_t prochaine = 0;
        for (std::size_t index = 0; index < enumeration.Valeurs.size(); ++index)
        {
            auto& valeur = enumeration.Valeurs[index];
            if (!noms.insert(valeur.Nom).second)
                Erreur(
                    valeur.Position,
                    "énumérateur déclaré plusieurs fois : " + valeur.Nom,
                    "enumerator declared more than once: " + valeur.Nom);

            if (valeur.Initialiseur)
            {
                Fonction contexte;
                contexte.Espace = enumeration.Espace;
                const auto type = AnalyserExpression(*valeur.Initialiseur, contexte);
                if (!type.EstEntier())
                    Erreur(
                        valeur.Position,
                        "la valeur d’énumération doit être une constante entière",
                        "enum value must be an integer constant");
                if (!EstExpressionConstante(*valeur.Initialiseur))
                    Erreur(
                        valeur.Position,
                        "la valeur d’énumération doit être constante",
                        "enum value must be constant");
                const auto bits = EvaluerConstante(*valeur.Initialiseur);
                if (EstTypeSigne(type))
                    valeur.Valeur = InterpreterSigne(bits, type);
                else
                {
                    const auto nonSignee = NormaliserBits(bits, type);
                    if (nonSignee > static_cast<std::uint64_t>(INT32_MAX))
                        Erreur(
                            valeur.Position,
                            "valeur d’énumération hors de la plage entier32",
                            "enum value is outside int32 range");
                    valeur.Valeur = static_cast<std::int64_t>(nonSignee);
                }
            }
            else valeur.Valeur = prochaine;

            if (valeur.Valeur < INT32_MIN || valeur.Valeur > INT32_MAX)
                Erreur(
                    valeur.Position,
                    "valeur d’énumération hors de la plage entier32",
                    "enum value is outside int32 range");

            const auto nomValeur = nomComplet + "::" + valeur.Nom;
            _ValeursEnumerations.emplace(
                nomValeur,
                ValeurEnumeration{{GenreType::Enumeration, nomComplet, 0}, valeur.Valeur});

            if (index + 1 < enumeration.Valeurs.size())
            {
                if (valeur.Valeur == INT32_MAX)
                    Erreur(
                        valeur.Position,
                        "débordement de la valeur d’énumération suivante",
                        "next enum value would overflow");
                prochaine = valeur.Valeur + 1;
            }
        }
    }

    void AnalyseurSemantique::CalculerStructure(Structure& structure)
    {
        const auto nom = structure.NomComplet();
        if (_StructuresCalculees.contains(nom)) return;
        if (!_StructuresEnCours.insert(nom).second)
            Erreur(
                structure.Position,
                "cycle de structures par valeur détecté pour " + nom,
                "by-value structure cycle detected for " + nom);

        const Structure* classeBase = nullptr;
        if (!structure.ClasseBaseCanonique.empty())
        {
            classeBase = _Structures.at(structure.ClasseBaseCanonique);
            CalculerStructure(*_Structures.at(structure.ClasseBaseCanonique));
            structure.EstPolymorphe = structure.EstPolymorphe
                || classeBase->EstPolymorphe;
        }

        std::uint32_t position = classeBase ? classeBase->Taille : 0U;
        std::uint32_t alignement = classeBase ? classeBase->Alignement : 1U;
        if (structure.EstPolymorphe)
        {
            structure.SymboleTableVirtuelle =
                "@GsVTable::" + structure.NomComplet();
            if (classeBase && classeBase->EstPolymorphe)
                structure.DecalageTableVirtuelle =
                    classeBase->DecalageTableVirtuelle;
            else
            {
                position = Aligner(position, 8);
                structure.DecalageTableVirtuelle = position;
                position += 8;
                alignement = std::max<std::uint32_t>(alignement, 8);
            }
        }
        std::unordered_set<std::string> champs;
        std::unordered_map<std::string, ChampStructure*> champsParNom;
        for (auto& champ : structure.Champs)
        {
            if (!champs.insert(champ.Nom).second)
                Erreur(
                    champ.Position,
                    "champ déclaré plusieurs fois : " + champ.Nom,
                    "field declared more than once: " + champ.Nom);
            ResoudreType(champ.Type, structure.Espace, champ.Position);
            if (champ.Type.EstReference)
                Erreur(
                    champ.Position,
                    "un champ référence n’est pas pris en charge ; utilisez un pointeur",
                    "reference fields are unsupported; use a pointer");
            if (champ.Type.Genre == GenreType::Vide && champ.Type.NiveauPointeur == 0)
                Erreur(champ.Position, "un champ ne peut pas être 'vide'", "a field cannot be 'void'");
            if (champ.Type.Genre == GenreType::Structure
                && champ.Type.NiveauPointeur == 0)
                CalculerStructure(*_Structures.at(champ.Type.Nom));

            const auto alignementChamp = AlignementType(champ.Type, _Structures);
            const auto tailleChamp = TailleType(champ.Type, _Structures);
            if (structure.EstUnion)
            {
                champ.Decalage = 0;
                position = std::max(position, tailleChamp);
            }
            else
            {
                position = Aligner(position, alignementChamp);
                champ.Decalage = position;
                position += tailleChamp;
            }
            alignement = std::max(alignement, alignementChamp);
            champsParNom.emplace(champ.Nom, &champ);
        }

        std::unordered_map<std::string, AliasChamp*> aliasesChamps;
        for (auto& alias : structure.AliasesChamps)
        {
            if (champsParNom.contains(alias.Nom))
                Erreur(
                    alias.Position,
                    "alias de champ en conflit avec un champ : " + alias.Nom,
                    "field alias conflicts with a field: " + alias.Nom);
            if (!aliasesChamps.emplace(alias.Nom, &alias).second)
                Erreur(
                    alias.Position,
                    "alias de champ déclaré plusieurs fois : " + alias.Nom,
                    "field alias declared more than once: " + alias.Nom);
        }

        std::unordered_set<std::string> aliasesChampsEnCours;
        std::function<ChampStructure*(AliasChamp&)> resoudreAliasChamp;
        resoudreAliasChamp = [&](AliasChamp& alias) -> ChampStructure*
        {
            if (!alias.CibleCanonique.empty())
                return champsParNom.at(alias.CibleCanonique);
            if (!aliasesChampsEnCours.insert(alias.Nom).second)
                Erreur(
                    alias.Position,
                    "cycle d’alias de champ détecté pour " + alias.Nom,
                    "field alias cycle detected for " + alias.Nom);

            ChampStructure* cible = nullptr;
            if (const auto champ = champsParNom.find(alias.Cible); champ != champsParNom.end())
                cible = champ->second;
            else if (const auto autreAlias = aliasesChamps.find(alias.Cible);
                     autreAlias != aliasesChamps.end())
                cible = resoudreAliasChamp(*autreAlias->second);
            else
                Erreur(
                    alias.Position,
                    "champ cible d’alias introuvable : " + alias.Cible,
                    "unknown field alias target: " + alias.Cible);

            alias.CibleCanonique = cible->Nom;
            aliasesChampsEnCours.erase(alias.Nom);
            return cible;
        };
        for (auto& alias : structure.AliasesChamps) (void)resoudreAliasChamp(alias);

        structure.Alignement = alignement;
        structure.Taille = Aligner(std::max<std::uint32_t>(position, 1), alignement);
        _StructuresEnCours.erase(nom);
        _StructuresCalculees.insert(nom);
    }

    bool AnalyseurSemantique::EstValeurGauche(const Expression& expression) const
    {
        if (expression.Genre == GenreExpression::Variable)
        {
            const auto& variable = static_cast<const ExpressionVariable&>(expression);
            return !variable.EstConstanteEnumeration && !variable.EstFonction;
        }
        if (expression.Genre == GenreExpression::Membre
            || expression.Genre == GenreExpression::Index)
            return true;
        if (expression.Genre == GenreExpression::Appel)
            return static_cast<const ExpressionAppel&>(expression).RetourneReference;
        if (expression.Genre == GenreExpression::Unaire)
            return static_cast<const ExpressionUnaire&>(expression).Operateur == "*";
        return false;
    }

    bool AnalyseurSemantique::EstExpressionConstante(const Expression& expression) const
    {
        if (expression.Genre == GenreExpression::Entier) return true;
        if (expression.Genre == GenreExpression::Chaine) return false;
        if (expression.Genre == GenreExpression::Variable)
            return static_cast<const ExpressionVariable&>(expression).EstConstanteEnumeration;
        if (expression.Genre == GenreExpression::Unaire)
        {
            const auto& unaire = static_cast<const ExpressionUnaire&>(expression);
            return (unaire.Operateur == "+"
                    || unaire.Operateur == "-"
                    || unaire.Operateur == "!"
                    || unaire.Operateur == "~")
                && EstExpressionConstante(*unaire.Operande);
        }
        if (expression.Genre == GenreExpression::Binaire)
        {
            const auto& binaire = static_cast<const ExpressionBinaire&>(expression);
            return EstExpressionConstante(*binaire.Gauche)
                && EstExpressionConstante(*binaire.Droite);
        }
        if (expression.Genre == GenreExpression::Conversion)
            return EstExpressionConstante(
                *static_cast<const ExpressionConversion&>(expression).Valeur);
        return false;
    }

    bool AnalyseurSemantique::AdapterConstante(
        Expression& expression,
        const TypeGs& cible)
    {
        if (!EstExpressionConstante(expression)) return false;
        const auto source = SansQualificateurs(expression.TypeSemantique);
        const auto destination = SansQualificateurs(cible);
        if (!(source.EstEntier() && destination.EstEntier())) return false;
        const auto valeur = EvaluerConstante(expression);
        if (!ValeurDansType(valeur, source, destination))
            Erreur(
                expression.Position,
                "constante hors de la plage de " + destination.Afficher(),
                "constant is outside the range of " + destination.Afficher());
        expression.TypeSemantique = destination;
        return true;
    }

    TypeGs AnalyseurSemantique::AnalyserExpression(Expression& expression, Fonction& fonction)
    {
        switch (expression.Genre)
        {
            case GenreExpression::Entier:
            {
                const auto& entier = static_cast<ExpressionEntier&>(expression);
                if (entier.EstLitteralBooleen)
                    expression.TypeSemantique = {GenreType::Booleen, {}, 0};
                else if (entier.Valeur <= static_cast<std::uint64_t>(INT32_MAX))
                    expression.TypeSemantique = {GenreType::Entier32, {}, 0};
                else if (entier.Valeur <= static_cast<std::uint64_t>(INT64_MAX))
                    expression.TypeSemantique = {GenreType::Entier64, {}, 0};
                else expression.TypeSemantique = {GenreType::Naturel64, {}, 0};
                break;
            }
            case GenreExpression::Chaine:
                expression.TypeSemantique = {GenreType::Caractere, {}, 1};
                expression.TypeSemantique.EstConstante = true;
                expression.EstValeurConstante = true;
                break;
            case GenreExpression::Variable:
            {
                auto& variable = static_cast<ExpressionVariable&>(expression);
                if (variable.EstBase)
                {
                    if (!fonction.EstMethode
                        || fonction.ClasseProprietaire.empty())
                        Erreur(
                            expression.Position,
                            "parent est uniquement disponible dans une méthode de classe",
                            "super is only available inside a class method");
                    const auto* classe = _Structures.at(
                        fonction.ClasseProprietaire);
                    if (classe->ClasseBaseCanonique.empty())
                        Erreur(
                            expression.Position,
                            "cette classe ne possède aucune base",
                            "this class has no base");
                    variable.Nom = "soi";
                    variable.EstReference = true;
                    expression.TypeSemantique = TypeGs{
                        GenreType::Structure,
                        classe->ClasseBaseCanonique};
                    break;
                }
                if (variable.EstFonction
                    && variable.TypeSemantique.EstPointeurFonction())
                    break;
                const auto& nom = variable.Nom;
                const auto trouve = _Variables.find(nom);
                if (trouve != _Variables.end())
                {
                    variable.EstReference = trouve->second.EstReference;
                    expression.TypeSemantique = variable.EstReference
                        ? SansReference(trouve->second)
                        : trouve->second;
                    expression.EstValeurConstante =
                        expression.TypeSemantique.EstConstante
                        && !expression.TypeSemantique.EstAdresse()
                        && !expression.TypeSemantique.EstTableau();
                }
                else
                {
                    std::string valeurEnum = nom;
                    if (!_ValeursEnumerations.contains(valeurEnum) && !fonction.Espace.empty())
                    {
                        const auto locale = fonction.Espace + "::" + valeurEnum;
                        if (_ValeursEnumerations.contains(locale)) valeurEnum = locale;
                    }
                    if (_ValeursEnumerations.contains(valeurEnum))
                    {
                        const auto& information = _ValeursEnumerations.at(valeurEnum);
                        variable.Nom = valeurEnum;
                        variable.EstConstanteEnumeration = true;
                        variable.ValeurEnumeration = information.Valeur;
                        expression.TypeSemantique = information.Type;
                        break;
                    }
                    std::string cible = nom;
                    if (!_Globales.contains(cible) && !fonction.Espace.empty())
                    {
                        const auto locale = fonction.Espace + "::" + cible;
                        if (_Globales.contains(locale)) cible = locale;
                    }
                    if (_Globales.contains(cible))
                    {
                        variable.Nom = _Globales.at(cible)->NomComplet();
                        variable.EstGlobale = true;
                        expression.TypeSemantique = _Globales.at(cible)->Type;
                        expression.EstValeurConstante = expression.TypeSemantique.EstConstante
                            && !expression.TypeSemantique.EstAdresse()
                            && !expression.TypeSemantique.EstTableau();
                        break;
                    }

                    const auto cibleFonction = QualifierNomFonction(nom, fonction);
                    if (!_Surcharges.contains(cibleFonction))
                        Erreur(
                            expression.Position,
                            "variable ou fonction introuvable : " + nom,
                            "unknown variable or function: " + nom);
                    if (_Surcharges.at(cibleFonction).size() != 1)
                        Erreur(
                            expression.Position,
                            "l’adresse d’une fonction surchargée exige un contexte non ambigu : " + nom,
                            "taking an overloaded function address requires an unambiguous context: " + nom);
                    const auto* cibleTrouvee = _Surcharges.at(cibleFonction).front();
                    variable.Nom = cibleTrouvee->NomComplet();
                    variable.EstFonction = true;
                    expression.TypeSemantique = CreerPointeurFonction(*cibleTrouvee);
                }
                break;
            }
            case GenreExpression::Unaire:
            {
                auto& unaire = static_cast<ExpressionUnaire&>(expression);
                auto type = AnalyserExpression(*unaire.Operande, fonction);
                if (type.EstStructure()
                    && unaire.Operateur != "&"
                    && unaire.Operateur != "*")
                {
                    auto nom = TrouverNomMethode(
                        type.Nom, "operator" + unaire.Operateur);
                    if (nom.empty())
                    {
                        const auto libre = QualifierNomFonction(
                            "operator" + unaire.Operateur, fonction);
                        if (_Surcharges.contains(libre)) nom = libre;
                    }
                    if (nom.empty())
                        Erreur(
                            expression.Position,
                            "aucun opérateur surchargé pour cet objet",
                            "no overloaded operator for this object");
                    std::vector<Expression*> arguments{unaire.Operande.get()};
                    auto* surcharge = ResoudreSurcharge(
                        nom, arguments, fonction, expression.Position);
                    if (surcharge->EstMethode
                        && !AccesMembreAutorise(
                            surcharge->Visibilite,
                            surcharge->ClasseProprietaire,
                            fonction))
                        Erreur(expression.Position,
                               "opérateur membre inaccessible",
                               "member operator is inaccessible");
                    unaire.NomSurcharge = surcharge->NomComplet();
                    unaire.OperandeParReference =
                        surcharge->Parametres[0].Type.EstReference;
                    expression.TypeSemantique = surcharge->TypeRetour.EstReference
                        ? SansReference(surcharge->TypeRetour)
                        : surcharge->TypeRetour;
                    break;
                }
                if (unaire.Operateur == "&")
                {
                    if (unaire.Operande->Genre == GenreExpression::Variable
                        && static_cast<const ExpressionVariable&>(*unaire.Operande).EstFonction)
                    {
                        expression.TypeSemantique = type;
                        break;
                    }
                    if (!EstValeurGauche(*unaire.Operande))
                        Erreur(expression.Position, "'&' exige une valeur adressable", "'&' requires an addressable value");
                    if (type.EstTableau())
                        Erreur(expression.Position,
                               "les pointeurs vers tableaux complets ne sont pas encore pris en charge",
                               "pointers to complete arrays are not supported yet");
                    ++type.NiveauPointeur;
                    expression.TypeSemantique = type;
                }
                else if (unaire.Operateur == "*")
                {
                    if (type.EstPointeurFonction())
                    {
                        expression.TypeSemantique = type;
                        break;
                    }
                    if (!type.EstPointeur())
                        Erreur(expression.Position, "'*' exige un pointeur", "'*' requires a pointer");
                    --type.NiveauPointeur;
                    expression.TypeSemantique = type;
                    expression.EstValeurConstante = type.EstConstante
                        && !type.EstAdresse();
                }
                else
                {
                    if (unaire.Operateur == "!")
                    {
                        if (!type.EstScalaire())
                            Erreur(expression.Position, "'!' exige une valeur scalaire", "'!' requires a scalar value");
                        expression.TypeSemantique = {GenreType::Booleen, {}, 0};
                    }
                    else
                    {
                        if (!type.EstEntier())
                            Erreur(expression.Position, "opérateur unaire réservé aux entiers", "unary operator requires an integer");
                        if (unaire.Operateur == "-"
                            && type.Genre == GenreType::Naturel64
                            && EstExpressionConstante(*unaire.Operande)
                            && EvaluerConstante(*unaire.Operande) == (std::uint64_t{1} << 63))
                            expression.TypeSemantique = {GenreType::Entier64, {}, 0};
                        else expression.TypeSemantique = SansQualificateurs(type);
                    }
                }
                break;
            }
            case GenreExpression::Membre:
            {
                auto& membre = static_cast<ExpressionMembre&>(expression);
                auto typeObjet = AnalyserExpression(*membre.Objet, fonction);
                if (membre.ViaPointeur)
                {
                    if (!typeObjet.EstPointeur() || typeObjet.NiveauPointeur != 1
                        || typeObjet.Genre != GenreType::Structure)
                        Erreur(expression.Position, "'->' exige un pointeur vers structure", "'->' requires a pointer to struct");
                    --typeObjet.NiveauPointeur;
                }
                else if (!typeObjet.EstStructure())
                {
                    Erreur(expression.Position, "'.' exige une structure", "'.' requires a struct");
                }
                const auto* structure = _Structures.at(typeObjet.Nom);
                const Structure* proprietaire = nullptr;
                const auto* trouve = TrouverChamp(
                    *structure, membre.Membre, proprietaire);
                if (!trouve)
                    Erreur(expression.Position, "membre introuvable : " + membre.Membre, "unknown member: " + membre.Membre);
                if (!AccesMembreAutorise(
                        trouve->Visibilite,
                        proprietaire->NomComplet(), fonction))
                    Erreur(
                        expression.Position,
                        "accès interdit au membre " + membre.Membre,
                        "member access is not allowed: " + membre.Membre);
                membre.Membre = trouve->Nom;
                membre.DecalageMembre = trouve->Decalage;
                expression.TypeSemantique = trouve->Type;
                if (!expression.TypeSemantique.EstAdresse())
                {
                    if (typeObjet.EstConstante) expression.TypeSemantique.EstConstante = true;
                    if (typeObjet.EstVolatile) expression.TypeSemantique.EstVolatile = true;
                }
                expression.EstValeurConstante = membre.Objet->EstValeurConstante
                    || (typeObjet.EstConstante && typeObjet.EstStructure())
                    || (trouve->Type.EstConstante && !trouve->Type.EstAdresse());
                break;
            }
            case GenreExpression::Index:
            {
                auto& index = static_cast<ExpressionIndex&>(expression);
                auto typeObjet = AnalyserExpression(*index.Objet, fonction);
                const auto typeIndice = AnalyserExpression(*index.Indice, fonction);
                if (!(typeObjet.EstPointeur() || typeObjet.EstTableau()))
                    Erreur(expression.Position, "l’indexation exige un pointeur ou un tableau", "indexing requires a pointer or array");
                if (!typeIndice.EstEntier())
                    Erreur(index.Indice->Position, "l’indice doit être entier", "index must be an integer");
                if (typeObjet.EstTableau()) typeObjet = typeObjet.ElementTableau();
                else --typeObjet.NiveauPointeur;
                if (typeObjet.EstVide())
                    Erreur(expression.Position, "l’indexation de vide* est interdite", "indexing void* is forbidden");
                index.TailleElement = TailleType(typeObjet, _Structures);
                expression.TypeSemantique = typeObjet;
                expression.EstValeurConstante = index.Objet->EstValeurConstante
                    || (typeObjet.EstConstante && !typeObjet.EstAdresse());
                break;
            }
            case GenreExpression::Affectation:
            {
                auto& affectation = static_cast<ExpressionAffectation&>(expression);
                const auto cible = AnalyserExpression(*affectation.Cible, fonction);
                if (!EstValeurGauche(*affectation.Cible))
                    Erreur(expression.Position, "cible d’affectation non modifiable", "assignment target is not modifiable");
                if (affectation.Cible->EstValeurConstante)
                    Erreur(expression.Position, "affectation d’une valeur constante interdite", "cannot assign to a const value");
                if (cible.EstTableau())
                    Erreur(expression.Position, "copie de tableau non prise en charge", "array assignment is unsupported");
                (void)AnalyserInitialiseur(
                    *affectation.Valeur, cible, fonction);
                expression.TypeSemantique = SansQualificateurs(cible);
                break;
            }
            case GenreExpression::Binaire:
            {
                auto& binaire = static_cast<ExpressionBinaire&>(expression);
                auto gauche = AnalyserExpression(*binaire.Gauche, fonction);
                auto droite = AnalyserExpression(*binaire.Droite, fonction);
                if (gauche.EstStructure() || droite.EstStructure())
                {
                    std::string nom;
                    if (gauche.EstStructure())
                    {
                        const auto membre = TrouverNomMethode(
                            gauche.Nom, "operator" + binaire.Operateur);
                        if (!membre.empty()) nom = membre;
                    }
                    if (nom.empty())
                    {
                        const auto libre = QualifierNomFonction(
                            "operator" + binaire.Operateur, fonction);
                        if (_Surcharges.contains(libre)) nom = libre;
                    }
                    if (nom.empty())
                        Erreur(expression.Position,
                               "aucun opérateur surchargé pour ces objets",
                               "no overloaded operator for these objects");
                    std::vector<Expression*> arguments{
                        binaire.Gauche.get(), binaire.Droite.get()};
                    auto* surcharge = ResoudreSurcharge(
                        nom, arguments, fonction, expression.Position);
                    if (surcharge->EstMethode
                        && !AccesMembreAutorise(
                            surcharge->Visibilite,
                            surcharge->ClasseProprietaire,
                            fonction))
                        Erreur(expression.Position,
                               "opérateur membre inaccessible",
                               "member operator is inaccessible");
                    binaire.NomSurcharge = surcharge->NomComplet();
                    binaire.GaucheParReference =
                        surcharge->Parametres[0].Type.EstReference;
                    binaire.DroiteParReference =
                        surcharge->Parametres[1].Type.EstReference;
                    expression.TypeSemantique = surcharge->TypeRetour.EstReference
                        ? SansReference(surcharge->TypeRetour)
                        : surcharge->TypeRetour;
                    break;
                }
                const bool comparaison = binaire.Operateur == "==" || binaire.Operateur == "!="
                    || binaire.Operateur == "<" || binaire.Operateur == "<="
                    || binaire.Operateur == ">" || binaire.Operateur == ">=";
                const bool decalage = binaire.Operateur == "<<"
                    || binaire.Operateur == ">>";
                const bool logiqueBits = binaire.Operateur == "&"
                    || binaire.Operateur == "|"
                    || binaire.Operateur == "^";
                const bool logique = binaire.Operateur == "&&"
                    || binaire.Operateur == "||";
                if (logique)
                {
                    if (!gauche.EstScalaire() || !droite.EstScalaire())
                        Erreur(
                            expression.Position,
                            "un opérateur logique exige deux valeurs scalaires",
                            "a logical operator requires two scalar values");
                    expression.TypeSemantique = {GenreType::Booleen, {}, 0};
                    break;
                }
                if (!decalage && !TypesValeurEgaux(gauche, droite))
                {
                    if (AdapterConstante(*binaire.Gauche, droite))
                        gauche = binaire.Gauche->TypeSemantique;
                    else if (AdapterConstante(*binaire.Droite, gauche))
                        droite = binaire.Droite->TypeSemantique;
                }
                if (!decalage && !TypesValeurEgaux(gauche, droite))
                    Erreur(expression.Position, "opérandes de types différents", "operands have different types");
                if (decalage && !(gauche.EstEntier() && droite.EstEntier()))
                    Erreur(
                        expression.Position,
                        "un décalage binaire exige deux entiers",
                        "a bit shift requires two integers");
                if (!comparaison && !gauche.EstEntier())
                    Erreur(expression.Position, "calcul arithmétique réservé aux entiers", "arithmetic requires integers");
                if (logiqueBits && !droite.EstEntier())
                    Erreur(
                        expression.Position,
                        "opération binaire réservée aux entiers",
                        "bitwise operation requires integers");
                if (comparaison && !gauche.EstScalaire())
                    Erreur(expression.Position, "comparaison de ce type non prise en charge", "comparison of this type is unsupported");
                if ((binaire.Operateur != "==" && binaire.Operateur != "!=") && gauche.EstAdresse())
                    Erreur(expression.Position, "seules == et != sont permises sur les pointeurs", "only == and != are allowed for pointers");
                if ((binaire.Operateur != "==" && binaire.Operateur != "!=")
                    && gauche.EstBooleen())
                    Erreur(expression.Position, "seules == et != sont permises sur les booléens", "only == and != are allowed for bool");
                expression.TypeSemantique = comparaison
                    ? TypeGs{GenreType::Booleen, {}, 0}
                    : SansQualificateurs(gauche);
                break;
            }
            case GenreExpression::Appel:
            {
                auto& appel = static_cast<ExpressionAppel&>(expression);
                if (appel.Cible->Genre == GenreExpression::Membre)
                {
                    auto& membre = static_cast<ExpressionMembre&>(*appel.Cible);
                    const bool appelBase =
                        membre.Objet->Genre == GenreExpression::Variable
                        && static_cast<const ExpressionVariable&>(
                            *membre.Objet).EstBase;
                    auto typeObjet = AnalyserExpression(*membre.Objet, fonction);
                    if (membre.ViaPointeur)
                    {
                        if (!typeObjet.EstPointeur()
                            || typeObjet.NiveauPointeur != 1
                            || typeObjet.Genre != GenreType::Structure)
                            Erreur(membre.Position,
                                   "'->' exige un pointeur vers classe",
                                   "'->' requires a pointer to class");
                        --typeObjet.NiveauPointeur;
                    }
                    else
                    {
                        if (!typeObjet.EstStructure())
                            Erreur(membre.Position,
                                   "'.' exige une classe",
                                   "'.' requires a class");
                    }
                    const auto nomMethode = TrouverNomMethode(
                        typeObjet.Nom, membre.Membre);
                    if (!nomMethode.empty())
                    {
                        std::unique_ptr<Expression> recepteur;
                        if (membre.ViaPointeur)
                            recepteur = std::make_unique<ExpressionUnaire>(
                                "*", std::move(membre.Objet), membre.Position);
                        else recepteur = std::move(membre.Objet);
                        appel.Arguments.insert(
                            appel.Arguments.begin(), std::move(recepteur));
                        appel.ForcerAppelDirect = appelBase;
                        appel.Cible = std::make_unique<ExpressionVariable>(
                            nomMethode, membre.Position);
                    }
                }

                if (appel.Cible->Genre == GenreExpression::Variable)
                {
                    auto& variable =
                        static_cast<ExpressionVariable&>(*appel.Cible);
                    const bool estVariableLocale = _Variables.contains(variable.Nom);
                    const bool estGlobale = _Globales.contains(variable.Nom);
                    const auto nom = QualifierNomFonction(variable.Nom, fonction);
                    if (!estVariableLocale && !estGlobale
                        && _Surcharges.contains(nom))
                    {
                        std::vector<Expression*> arguments;
                        arguments.reserve(appel.Arguments.size());
                        for (auto& argument : appel.Arguments)
                            arguments.push_back(argument.get());
                        auto* surcharge = ResoudreSurcharge(
                            nom, arguments, fonction, expression.Position);
                        if (surcharge->EstMethode
                            && !AccesMembreAutorise(
                                surcharge->Visibilite,
                                surcharge->ClasseProprietaire,
                                fonction))
                            Erreur(expression.Position,
                                   "méthode inaccessible : " + surcharge->NomSource,
                                   "method is inaccessible: " + surcharge->NomSource);
                        PreparerAppel(appel, *surcharge, arguments, fonction);
                    }
                }
                const auto typeCible = AnalyserExpression(*appel.Cible, fonction);
                if (!typeCible.EstPointeurFonction() || !typeCible.RetourFonction)
                    Erreur(
                        appel.Cible->Position,
                        "la cible d’appel n’est pas un pointeur de fonction",
                        "call target is not a function pointer");
                if (appel.Cible->Genre == GenreExpression::Variable
                    && static_cast<const ExpressionVariable&>(*appel.Cible).EstFonction)
                {
                    appel.NomDirect =
                        static_cast<const ExpressionVariable&>(*appel.Cible).Nom;
                    const auto* declaration = _Fonctions.at(appel.NomDirect);
                    const auto intrinseque = IdentifierIntrinseque(appel.NomDirect);
                    if (declaration->EstExterne
                        && intrinseque != GenreIntrinseque::Aucun)
                    {
                        const auto estType = [](const TypeGs& type, GenreType genre,
                                                std::uint32_t pointeurs = 0)
                        {
                            return type.Genre == genre
                                && type.NiveauPointeur == pointeurs
                                && type.DimensionsTableau.empty();
                        };
                        const auto est32 = [&](const TypeGs& type, std::uint32_t pointeurs = 0)
                        { return estType(type, GenreType::Naturel32, pointeurs); };
                        const auto est64 = [&](const TypeGs& type, std::uint32_t pointeurs = 0)
                        { return estType(type, GenreType::Naturel64, pointeurs); };
                        const auto estVide = [&](const TypeGs& type)
                        { return estType(type, GenreType::Vide); };
                        const auto& parametres = declaration->Parametres;
                        bool valide = false;
                        switch (intrinseque)
                        {
                            case GenreIntrinseque::ChargerAtomique32:
                                valide = est32(declaration->TypeRetour)
                                    && parametres.size() == 1
                                    && est32(parametres[0].Type, 1);
                                break;
                            case GenreIntrinseque::ChargerAtomique64:
                                valide = est64(declaration->TypeRetour)
                                    && parametres.size() == 1
                                    && est64(parametres[0].Type, 1);
                                break;
                            case GenreIntrinseque::StockerAtomique32:
                                valide = estVide(declaration->TypeRetour)
                                    && parametres.size() == 2
                                    && est32(parametres[0].Type, 1)
                                    && est32(parametres[1].Type);
                                break;
                            case GenreIntrinseque::StockerAtomique64:
                                valide = estVide(declaration->TypeRetour)
                                    && parametres.size() == 2
                                    && est64(parametres[0].Type, 1)
                                    && est64(parametres[1].Type);
                                break;
                            case GenreIntrinseque::EchangerAtomique32:
                            case GenreIntrinseque::AjouterAtomique32:
                                valide = est32(declaration->TypeRetour)
                                    && parametres.size() == 2
                                    && est32(parametres[0].Type, 1)
                                    && est32(parametres[1].Type);
                                break;
                            case GenreIntrinseque::EchangerAtomique64:
                            case GenreIntrinseque::AjouterAtomique64:
                                valide = est64(declaration->TypeRetour)
                                    && parametres.size() == 2
                                    && est64(parametres[0].Type, 1)
                                    && est64(parametres[1].Type);
                                break;
                            case GenreIntrinseque::ComparerEchanger32:
                                valide = est32(declaration->TypeRetour)
                                    && parametres.size() == 3
                                    && est32(parametres[0].Type, 1)
                                    && est32(parametres[1].Type)
                                    && est32(parametres[2].Type);
                                break;
                            case GenreIntrinseque::ComparerEchanger64:
                                valide = est64(declaration->TypeRetour)
                                    && parametres.size() == 3
                                    && est64(parametres[0].Type, 1)
                                    && est64(parametres[1].Type)
                                    && est64(parametres[2].Type);
                                break;
                            case GenreIntrinseque::BarriereMemoire:
                            case GenreIntrinseque::PauseProcesseur:
                                valide = estVide(declaration->TypeRetour)
                                    && parametres.empty();
                                break;
                            case GenreIntrinseque::Aucun:
                                break;
                        }
                        if (!valide)
                            Erreur(
                                declaration->Position,
                                "prototype invalide pour l’intrinsèque "
                                    + appel.NomDirect,
                                "invalid prototype for intrinsic "
                                    + appel.NomDirect);
                        appel.EstIntrinseque = true;
                    }
                }
                else appel.EstIndirect = true;
                if (appel.Arguments.size() != typeCible.ParametresFonction.size())
                    Erreur(expression.Position, "nombre d’arguments incorrect", "incorrect argument count");
                appel.ArgumentsParReference.clear();
                for (std::size_t index = 0; index < appel.Arguments.size(); ++index)
                {
                    const auto& parametre = typeCible.ParametresFonction[index];
                    appel.ArgumentsParReference.push_back(parametre.EstReference);
                    (void)AnalyserInitialiseur(
                        *appel.Arguments[index], parametre, fonction);
                }
                appel.RetourneReference = typeCible.RetourFonction->EstReference;
                expression.TypeSemantique = appel.RetourneReference
                    ? SansReference(*typeCible.RetourFonction)
                    : *typeCible.RetourFonction;
                break;
            }
            case GenreExpression::Conversion:
            {
                auto& conversion = static_cast<ExpressionConversion&>(expression);
                ResoudreType(conversion.TypeCible, fonction.Espace, expression.Position);
                if (conversion.TypeCible.EstVide()
                    || conversion.TypeCible.EstStructure()
                    || conversion.TypeCible.EstTableau())
                    Erreur(expression.Position,
                           "type cible de conversion non scalaire",
                           "cast target is not scalar");

                const auto source = AnalyserExpression(*conversion.Valeur, fonction);
                if (!source.EstScalaire())
                    Erreur(expression.Position,
                           "source de conversion non scalaire",
                           "cast source is not scalar");
                if (source.EstAdresse() != conversion.TypeCible.EstAdresse())
                    Erreur(expression.Position,
                           "une conversion entre pointeur et entier est interdite",
                           "casts between pointers and integers are forbidden");

                if ((source.EstPointeurFonction()
                        || conversion.TypeCible.EstPointeurFonction())
                    && !TypesValeurEgaux(source, conversion.TypeCible))
                    Erreur(
                        expression.Position,
                        "conversion entre signatures de fonction incompatibles",
                        "cast between incompatible function signatures");

                if (!conversion.TypeCible.EstAdresse()
                    && EstExpressionConstante(*conversion.Valeur))
                {
                    const auto valeur = EvaluerConstante(*conversion.Valeur);
                    if (!conversion.TypeCible.EstBooleen()
                        && !ValeurDansType(valeur, source, conversion.TypeCible))
                        Erreur(expression.Position,
                               "conversion constante hors de la plage de "
                                   + conversion.TypeCible.Afficher(),
                               "constant cast is outside the range of "
                                   + conversion.TypeCible.Afficher());
                }
                // Une conversion explicite de pointeur doit conserver les
                // qualificatifs de sa cible. Les supprimer ici transformait
                // notamment `convertir<constante T*>(T*)` en `T*`, puis
                // faisait échouer l'initialisation ou le retour qui suivait.
                expression.TypeSemantique = conversion.TypeCible;
                expression.TypeSemantique.EstReference = false;
                break;
            }
            case GenreExpression::Agregat:
                Erreur(
                    expression.Position,
                    "un initialiseur agrégé exige un type de destination",
                    "an aggregate initializer requires a destination type");
        }
        return expression.TypeSemantique;
    }

    TypeGs AnalyseurSemantique::AnalyserInitialiseur(
        Expression& expression,
        const TypeGs& typeCible,
        Fonction& fonction)
    {
        if (typeCible.EstReference)
        {
            if (expression.Genre == GenreExpression::Agregat)
                Erreur(expression.Position,
                       "une référence ne peut pas être liée à un agrégat temporaire",
                       "a reference cannot bind to a temporary aggregate");
            const auto type = AnalyserExpression(expression, fonction);
            const auto destination = SansReference(typeCible);
            if (!EstValeurGauche(expression)
                || (!TypesValeurEgaux(type, destination)
                    && !ConversionHeritageAutorisee(
                        type, destination, true))
                || (!destination.EstConstante
                    && expression.EstValeurConstante))
                Erreur(
                    expression.Position,
                    "liaison de référence incompatible : " + type.Afficher()
                        + " vers " + typeCible.Afficher(),
                    "incompatible reference binding: " + type.Afficher()
                        + " to " + typeCible.Afficher());
            return destination;
        }
        if (expression.Genre != GenreExpression::Agregat)
        {
            auto type = AnalyserExpression(expression, fonction);
            if (!TypesValeurEgaux(type, typeCible)
                && AdapterConstante(expression, typeCible))
                type = expression.TypeSemantique;
            if (!TypesValeurEgaux(type, typeCible)
                && !ConversionHeritageAutorisee(type, typeCible))
                Erreur(
                    expression.Position,
                    "type d’initialisation incompatible : " + type.Afficher()
                        + " vers " + typeCible.Afficher(),
                    "incompatible initializer type: " + type.Afficher()
                        + " to " + typeCible.Afficher());
            return type;
        }

        auto& agregat = static_cast<ExpressionAgregat&>(expression);
        expression.TypeSemantique = SansQualificateurs(typeCible);
        if (typeCible.EstTableau())
        {
            const auto capacite = typeCible.DimensionsTableau.front();
            if (agregat.Elements.size() > capacite)
                Erreur(
                    expression.Position,
                    "trop d’éléments dans l’initialiseur de tableau",
                    "too many elements in array initializer");
            const auto element = typeCible.ElementTableau();
            for (auto& valeur : agregat.Elements)
                (void)AnalyserInitialiseur(*valeur, element, fonction);
            return expression.TypeSemantique;
        }

        if (typeCible.EstStructure())
        {
            const auto* structure = _Structures.at(typeCible.Nom);
            if (structure->EstClasse)
                Erreur(
                    expression.Position,
                    "une classe doit être initialisée par un constructeur",
                    "a class must be initialized by a constructor");
            const auto capacite = structure->EstUnion
                ? std::min<std::size_t>(1, structure->Champs.size())
                : structure->Champs.size();
            if (agregat.Elements.size() > capacite)
                Erreur(
                    expression.Position,
                    "trop d’éléments dans l’initialiseur de structure",
                    "too many elements in struct initializer");
            for (std::size_t index = 0; index < agregat.Elements.size(); ++index)
                (void)AnalyserInitialiseur(
                    *agregat.Elements[index],
                    structure->Champs[index].Type,
                    fonction);
            return expression.TypeSemantique;
        }

        if (agregat.Elements.size() > 1)
            Erreur(
                expression.Position,
                "un initialiseur scalaire agrégé accepte au maximum une valeur",
                "a scalar aggregate initializer accepts at most one value");
        if (!agregat.Elements.empty())
            (void)AnalyserInitialiseur(
                *agregat.Elements.front(), typeCible, fonction);
        return expression.TypeSemantique;
    }

    void AnalyseurSemantique::AnalyserInstruction(Instruction& instruction, Fonction& fonction)
    {
        switch (instruction.Genre)
        {
            case GenreInstruction::Bloc:
            {
                const auto variablesExterieures = _Variables;
                for (auto& enfant : static_cast<InstructionBloc&>(instruction).Instructions)
                    AnalyserInstruction(*enfant, fonction);
                _Variables = variablesExterieures;
                return;
            }
            case GenreInstruction::Variable:
            {
                auto& variable = static_cast<InstructionVariable&>(instruction);
                ResoudreType(variable.Type, fonction.Espace, variable.Position);
                auto typeElementObjet = variable.Type;
                while (typeElementObjet.EstTableau())
                    typeElementObjet = typeElementObjet.ElementTableau();
                const bool estObjetClasse = typeElementObjet.Genre
                        == GenreType::Structure
                    && typeElementObjet.NiveauPointeur == 0
                    && !typeElementObjet.EstReference
                    && _Structures.at(typeElementObjet.Nom)->EstClasse;
                if (variable.Type.Genre == GenreType::Vide
                    && variable.Type.NiveauPointeur == 0)
                    Erreur(variable.Position, "une variable ne peut pas être 'vide'", "a variable cannot be 'void'");
                if (_Variables.contains(variable.Nom))
                    Erreur(variable.Position, "nom déjà déclaré : " + variable.Nom, "name already declared: " + variable.Nom);
                _Variables.emplace(variable.Nom, variable.Type);
                if (variable.Type.EstReference && !variable.Initialiseur)
                    Erreur(variable.Position,
                           "une référence doit être initialisée",
                           "a reference must be initialized");
                if (variable.Type.EstConstante
                    && !variable.Type.EstAdresse()
                    && !variable.Type.EstReference
                    && !variable.Initialiseur)
                    Erreur(variable.Position,
                           "une variable constante doit être initialisée",
                           "a const variable must be initialized");
                if (variable.Initialiseur)
                {
                    if (variable.Type.EstTableau() && estObjetClasse)
                        Erreur(
                            variable.Position,
                            "un tableau d’objets classes est construit par défaut élément par élément et n’accepte pas encore d’initialiseur agrégé",
                            "an array of class objects is default-constructed element by element and does not support aggregate initializers yet");
                    if (variable.Type.Genre == GenreType::Structure
                        && variable.Type.NiveauPointeur == 0
                        && !variable.Type.EstReference
                        && _Structures.at(variable.Type.Nom)->EstClasse)
                        Erreur(variable.Position,
                               "utilisez la syntaxe de constructeur Classe objet(arguments)",
                               "use constructor syntax Class object(arguments)");
                    if (variable.Type.EstTableau()
                        && variable.Initialiseur->Genre != GenreExpression::Agregat)
                        Erreur(
                            variable.Position,
                            "un tableau exige un initialiseur agrégé",
                            "an array requires an aggregate initializer");
                    (void)AnalyserInitialiseur(
                        *variable.Initialiseur, variable.Type, fonction);
                }
                if (!variable.ArgumentsConstruction.empty()
                    || variable.ConstructionExplicite)
                {
                    if (!estObjetClasse)
                        Erreur(variable.Position,
                               "la syntaxe de construction exige un type classe",
                               "constructor syntax requires a class type");
                }
                if (variable.Type.EstTableau() && estObjetClasse)
                {
                    variable.InitialiserTableVirtuelle = false;
                    variable.ClassesConstructeursBases.clear();
                    variable.SymbolesConstructeursBases.clear();
                    variable.EtapesConstructionImplicite.clear();
                    variable.SymboleConstructeur.clear();
                    variable.ArgumentsConstructionParReference.clear();
                    if (variable.ArgumentsConstruction.empty())
                        PlanifierConstructionTypeObjet(
                            variable.Type,
                            0,
                            fonction.ClasseProprietaire,
                            fonction,
                            variable.Position,
                            variable.EtapesConstructionImplicite);
                    else
                    {
                        auto* classeElement = _Structures.at(
                            typeElementObjet.Nom);
                        const auto nomConstructeur =
                            classeElement->NomComplet() + "::$constructeur";
                        if (!_Surcharges.contains(nomConstructeur))
                            Erreur(
                                variable.Position,
                                "aucun constructeur déclaré pour les éléments du tableau",
                                "no constructor is declared for the array elements");
                        std::vector<Expression*> arguments;
                        for (auto& argument : variable.ArgumentsConstruction)
                            arguments.push_back(argument.get());
                        auto* constructeur = ResoudreConstructeurClasse(
                            *classeElement,
                            arguments,
                            fonction,
                            variable.Position,
                            fonction.ClasseProprietaire,
                            variable.ArgumentsConstructionParReference);
                        PlanifierConstructionTableauAvecConstructeur(
                            variable.Type,
                            0,
                            constructeur->NomComplet(),
                            classeElement->NomComplet(),
                            variable.EtapesConstructionImplicite);
                    }

                    variable.ActionsDestruction.clear();
                    PlanifierDestructionTypeObjet(
                        variable.Type,
                        0,
                        fonction.ClasseProprietaire,
                        variable.Position,
                        variable.ActionsDestruction);
                    variable.SymbolesDestructeurs.clear();
                    for (const auto& action : variable.ActionsDestruction)
                        variable.SymbolesDestructeurs.push_back(
                            action.SymboleDestructeur);
                    variable.SymboleDestructeur.clear();
                }
                else if (variable.Type.EstStructure()
                    && _Structures.at(variable.Type.Nom)->EstClasse)
                {
                    const auto* classe = _Structures.at(variable.Type.Nom);
                    variable.InitialiserTableVirtuelle = classe->EstPolymorphe;

                    const auto nomConstructeur =
                        variable.Type.Nom + "::$constructeur";
                    const bool constructeurDeclare =
                        _Surcharges.contains(nomConstructeur);

                    variable.ClassesConstructeursBases.clear();
                    variable.SymbolesConstructeursBases.clear();
                    variable.EtapesConstructionImplicite.clear();
                    if (!constructeurDeclare)
                        PlanifierConstructionImplicite(
                            *classe,
                            0,
                            fonction.ClasseProprietaire,
                            fonction,
                            variable.Position,
                            variable.EtapesConstructionImplicite);

                    if (constructeurDeclare)
                    {
                        std::vector<Expression*> arguments;
                        for (auto& argument : variable.ArgumentsConstruction)
                            arguments.push_back(argument.get());
                        auto* constructeur = ResoudreConstructeurClasse(
                            *classe,
                            arguments,
                            fonction,
                            variable.Position,
                            fonction.ClasseProprietaire,
                            variable.ArgumentsConstructionParReference);
                        variable.SymboleConstructeur =
                            constructeur->NomComplet();
                    }
                    else if (variable.ConstructionExplicite
                        || !variable.ArgumentsConstruction.empty())
                        Erreur(variable.Position,
                               "aucun constructeur déclaré pour cette classe",
                               "no constructor is declared for this class");

                    variable.ActionsDestruction.clear();
                    PlanifierDestructionClasse(
                        *classe,
                        0,
                        fonction.ClasseProprietaire,
                        variable.Position,
                        variable.ActionsDestruction);
                    variable.SymbolesDestructeurs.clear();
                    for (const auto& action : variable.ActionsDestruction)
                        variable.SymbolesDestructeurs.push_back(
                            action.SymboleDestructeur);
                    variable.SymboleDestructeur.clear();
                    const auto nomDestructeurPropre =
                        classe->NomComplet() + "::$destructeur";
                    if (_Surcharges.contains(nomDestructeurPropre))
                        variable.SymboleDestructeur = _Surcharges.at(
                            nomDestructeurPropre).front()->NomComplet();
                }
                return;
            }
            case GenreInstruction::Expression:
                AnalyserExpression(*static_cast<InstructionExpression&>(instruction).Valeur, fonction);
                return;
            case GenreInstruction::Retour:
            {
                auto& retour = static_cast<InstructionRetour&>(instruction);
                if (!retour.Valeur)
                {
                    if (!fonction.TypeRetour.EstVide())
                        Erreur(retour.Position, "valeur de retour attendue", "return value expected");
                }
                else
                {
                    (void)AnalyserInitialiseur(
                        *retour.Valeur, fonction.TypeRetour, fonction);
                }
                return;
            }
            case GenreInstruction::Si:
            {
                auto& valeur = static_cast<InstructionSi&>(instruction);
                const auto type = AnalyserExpression(*valeur.Condition, fonction);
                if (!type.EstScalaire())
                    Erreur(valeur.Condition->Position, "condition non scalaire", "condition is not scalar");
                const auto variablesExterieures = _Variables;
                AnalyserInstruction(*valeur.Alors, fonction);
                _Variables = variablesExterieures;
                if (valeur.Sinon)
                {
                    AnalyserInstruction(*valeur.Sinon, fonction);
                    _Variables = variablesExterieures;
                }
                return;
            }
            case GenreInstruction::TantQue:
            {
                auto& valeur = static_cast<InstructionTantQue&>(instruction);
                const auto type = AnalyserExpression(*valeur.Condition, fonction);
                if (!type.EstScalaire())
                    Erreur(valeur.Condition->Position, "condition non scalaire", "condition is not scalar");
                const auto variablesExterieures = _Variables;
                AnalyserInstruction(*valeur.Corps, fonction);
                _Variables = variablesExterieures;
                return;
            }
        }
    }

    void AnalyseurSemantique::PreparerFonction(Fonction& fonction)
    {
        ResoudreType(fonction.TypeRetour, fonction.Espace, fonction.Position);
        if (fonction.TypeRetour.EstReference)
            Erreur(fonction.Position,
                   "les retours par référence ne sont pas encore pris en charge ; utilisez un pointeur",
                   "reference returns are not supported yet; use a pointer");
        for (auto& parametre : fonction.Parametres)
        {
            ResoudreType(parametre.Type, fonction.Espace, parametre.Position);
            if (parametre.Type.EstVide()
                || parametre.Type.EstTableau())
                Erreur(parametre.Position, "type de paramètre non pris en charge", "unsupported parameter type");
            if (parametre.Type.EstReference
                && parametre.Type.Genre == GenreType::Vide
                && parametre.Type.NiveauPointeur == 0)
                Erreur(parametre.Position,
                       "un paramètre ne peut pas être vide&",
                       "a parameter cannot be void&");
        }
        if ((fonction.EstConstructeur || fonction.EstDestructeur)
            && !fonction.TypeRetour.EstVide())
            Erreur(fonction.Position,
                   "constructeur ou destructeur avec retour invalide",
                   "constructor or destructor has an invalid return type");
        if (fonction.EstOperateur)
        {
            const auto explicites = fonction.Parametres.size()
                - (fonction.EstMethode ? 1U : 0U);
            const bool unaire = fonction.Operateur == "!"
                || fonction.Operateur == "~";
            const auto attendus = unaire
                ? (fonction.EstMethode ? 0U : 1U)
                : (fonction.EstMethode ? 1U : 2U);
            if (explicites != attendus)
                Erreur(fonction.Position,
                       "arité invalide pour l’opérateur " + fonction.Operateur,
                       "invalid arity for operator " + fonction.Operateur);
        }
        if (fonction.TypeRetour.EstStructure()
            && fonction.Parametres.size() > 3)
            Erreur(
                fonction.Position,
                "une fonction retournant une structure accepte au maximum trois paramètres",
                "a function returning a struct accepts at most three parameters");
        if (!fonction.TypeRetour.EstStructure()
            && fonction.Parametres.size() > 4)
            Erreur(
                fonction.Position,
                "une fonction accepte au maximum quatre paramètres",
                "a function accepts at most four parameters");
    }

    void AnalyseurSemantique::AnalyserFonction(Fonction& fonction)
    {
        _Variables.clear();
        for (const auto& parametre : fonction.Parametres)
        {
            if (!_Variables.emplace(parametre.Nom, parametre.Type).second)
                Erreur(parametre.Position, "paramètre déclaré plusieurs fois", "parameter declared more than once");
        }
        if (fonction.EstExterne)
        {
            if (fonction.EstPublique)
                Erreur(fonction.Position,
                       "une fonction importée ne peut pas être exportée",
                       "an imported function cannot be exported");
            return;
        }
        if (fonction.EstConstructeur)
        {
            fonction.SymboleConstructeurDelegue.clear();
            fonction.ArgumentsConstructeurDelegueParReference.clear();
            fonction.SymboleConstructeurBase.clear();
            fonction.ArgumentsConstructeurBaseParReference.clear();
            fonction.ClassesBasesSansConstructeur.clear();
            fonction.EtapesConstructionBaseImplicite.clear();
            auto* classe = _Structures.at(fonction.ClasseProprietaire);
            if (fonction.DelegueConstructeur)
            {
                std::vector<Expression*> arguments;
                for (auto& argument : fonction.ArgumentsConstructeurDelegue)
                    arguments.push_back(argument.get());
                auto* constructeur = ResoudreConstructeurClasse(
                    *classe,
                    arguments,
                    fonction,
                    fonction.Position,
                    fonction.ClasseProprietaire,
                    fonction.ArgumentsConstructeurDelegueParReference);
                if (constructeur == &fonction)
                    Erreur(
                        fonction.Position,
                        "un constructeur ne peut pas déléguer directement vers lui-même",
                        "a constructor cannot delegate directly to itself");
                fonction.SymboleConstructeurDelegue =
                    constructeur->NomComplet();
                AnalyserInstruction(*fonction.Corps, fonction);
                return;
            }
            if (classe->ClasseBaseCanonique.empty())
            {
                if (fonction.InitialiseurBaseExplicite)
                    Erreur(
                        fonction.Position,
                        "un constructeur sans classe de base ne peut pas utiliser : parent(...)",
                        "a constructor without a base class cannot use : super(...)");
            }
            else
            {
                const Structure* baseDirecte = _Structures.at(
                    classe->ClasseBaseCanonique);
                const auto nomConstructeurBase =
                    baseDirecte->NomComplet() + "::$constructeur";
                if (fonction.InitialiseurBaseExplicite
                    && !fonction.ArgumentsConstructeurBase.empty()
                    && !_Surcharges.contains(nomConstructeurBase))
                    Erreur(
                        fonction.Position,
                        "la base directe ne déclare aucun constructeur acceptant des arguments : "
                            + baseDirecte->NomComplet(),
                        "the direct base declares no constructor accepting arguments: "
                            + baseDirecte->NomComplet());

                if (_Surcharges.contains(nomConstructeurBase))
                {
                    std::vector<Expression*> arguments;
                    if (fonction.InitialiseurBaseExplicite)
                        for (auto& argument :
                             fonction.ArgumentsConstructeurBase)
                            arguments.push_back(argument.get());
                    auto* constructeur = ResoudreConstructeurClasse(
                        *baseDirecte,
                        arguments,
                        fonction,
                        fonction.Position,
                        fonction.ClasseProprietaire,
                        fonction.ArgumentsConstructeurBaseParReference,
                        true);
                    fonction.SymboleConstructeurBase =
                        constructeur->NomComplet();
                }
                else
                    PlanifierConstructionImplicite(
                        *baseDirecte,
                        0,
                        fonction.ClasseProprietaire,
                        fonction,
                        fonction.Position,
                        fonction.EtapesConstructionBaseImplicite);
            }

            std::unordered_set<std::string> champsInitialises;
            std::size_t dernierIndex = 0;
            bool premierChamp = true;
            for (auto& initialiseur : fonction.InitialiseursChamps)
            {
                auto trouve = std::find_if(
                    classe->Champs.begin(), classe->Champs.end(),
                    [&](const ChampStructure& champ)
                    { return champ.Nom == initialiseur.Nom; });
                if (trouve == classe->Champs.end())
                {
                    const auto alias = std::find_if(
                        classe->AliasesChamps.begin(),
                        classe->AliasesChamps.end(),
                        [&](const AliasChamp& valeur)
                        { return valeur.Nom == initialiseur.Nom; });
                    if (alias != classe->AliasesChamps.end())
                        trouve = std::find_if(
                            classe->Champs.begin(), classe->Champs.end(),
                            [&](const ChampStructure& champ)
                            { return champ.Nom == alias->CibleCanonique; });
                }
                if (trouve == classe->Champs.end())
                {
                    const Structure* proprietaire = nullptr;
                    const auto* champHerite = TrouverChamp(
                        *classe, initialiseur.Nom, proprietaire);
                    if (champHerite && proprietaire != classe)
                        Erreur(
                            initialiseur.Position,
                            "un constructeur initialise uniquement les champs déclarés directement dans sa classe : "
                                + initialiseur.Nom,
                            "a constructor only initializes fields declared directly in its class: "
                                + initialiseur.Nom);
                    Erreur(
                        initialiseur.Position,
                        "champ introuvable dans la liste d’initialisation : "
                            + initialiseur.Nom,
                        "unknown field in initializer list: "
                            + initialiseur.Nom);
                }

                const auto index = static_cast<std::size_t>(
                    std::distance(classe->Champs.begin(), trouve));
                if (!champsInitialises.insert(trouve->Nom).second)
                    Erreur(
                        initialiseur.Position,
                        "champ initialisé plusieurs fois : " + trouve->Nom,
                        "field initialized more than once: " + trouve->Nom);
                if (!premierChamp && index <= dernierIndex)
                    Erreur(
                        initialiseur.Position,
                        "les champs doivent être initialisés dans leur ordre de déclaration : "
                            + trouve->Nom,
                        "fields must be initialized in declaration order: "
                            + trouve->Nom);
                premierChamp = false;
                dernierIndex = index;

                auto typeElement = trouve->Type;
                while (typeElement.EstTableau())
                    typeElement = typeElement.ElementTableau();
                const bool contientClasse = typeElement.Genre
                        == GenreType::Structure
                    && typeElement.NiveauPointeur == 0
                    && _Structures.at(typeElement.Nom)->EstClasse;
                initialiseur.NomCanonique = trouve->Nom;
                initialiseur.Type = trouve->Type;
                initialiseur.Decalage = trouve->Decalage;
                initialiseur.EstObjetClasse = contientClasse;
                initialiseur.SymboleConstructeur.clear();
                initialiseur.ArgumentsConstructeurParReference.clear();
                initialiseur.EtapesConstructionImplicite.clear();

                if (initialiseur.EstObjetClasse)
                {
                    const auto* classeChamp = _Structures.at(typeElement.Nom);
                    if (trouve->Type.EstTableau())
                    {
                        if (initialiseur.Arguments.empty())
                            PlanifierConstructionTypeObjet(
                                trouve->Type,
                                0,
                                fonction.ClasseProprietaire,
                                fonction,
                                initialiseur.Position,
                                initialiseur.EtapesConstructionImplicite);
                        else
                        {
                            const auto nomConstructeurChamp =
                                classeChamp->NomComplet() + "::$constructeur";
                            if (!_Surcharges.contains(nomConstructeurChamp))
                                Erreur(
                                    initialiseur.Position,
                                    "aucun constructeur déclaré pour les éléments du champ tableau : "
                                        + trouve->Nom,
                                    "no constructor is declared for the array field elements: "
                                        + trouve->Nom);
                            std::vector<Expression*> arguments;
                            for (auto& argument : initialiseur.Arguments)
                                arguments.push_back(argument.get());
                            auto* constructeur = ResoudreConstructeurClasse(
                                *classeChamp,
                                arguments,
                                fonction,
                                initialiseur.Position,
                                fonction.ClasseProprietaire,
                                initialiseur
                                    .ArgumentsConstructeurParReference);
                            PlanifierConstructionTableauAvecConstructeur(
                                trouve->Type,
                                0,
                                constructeur->NomComplet(),
                                classeChamp->NomComplet(),
                                initialiseur.EtapesConstructionImplicite);
                        }
                        continue;
                    }
                    const auto nomConstructeurChamp =
                        classeChamp->NomComplet() + "::$constructeur";
                    if (_Surcharges.contains(nomConstructeurChamp))
                    {
                        std::vector<Expression*> arguments;
                        for (auto& argument : initialiseur.Arguments)
                            arguments.push_back(argument.get());
                        auto* constructeur = ResoudreConstructeurClasse(
                            *classeChamp,
                            arguments,
                            fonction,
                            initialiseur.Position,
                            fonction.ClasseProprietaire,
                            initialiseur
                                .ArgumentsConstructeurParReference);
                        initialiseur.SymboleConstructeur =
                            constructeur->NomComplet();
                    }
                    else
                    {
                        if (!initialiseur.Arguments.empty())
                            Erreur(
                                initialiseur.Position,
                                "aucun constructeur déclaré pour le champ objet classe : "
                                    + trouve->Nom,
                                "no constructor is declared for class object field: "
                                    + trouve->Nom);
                        PlanifierConstructionImplicite(
                            *classeChamp,
                            0,
                            fonction.ClasseProprietaire,
                            fonction,
                            initialiseur.Position,
                            initialiseur.EtapesConstructionImplicite);
                    }
                }
                else
                {
                    if (initialiseur.Arguments.size() != 1)
                        Erreur(
                            initialiseur.Position,
                            "un initialiseur de champ exige exactement une expression : "
                                + trouve->Nom,
                            "a field initializer requires exactly one expression: "
                                + trouve->Nom);
                    if (trouve->Type.EstTableau()
                        && initialiseur.Arguments.front()->Genre
                            != GenreExpression::Agregat)
                        Erreur(
                            initialiseur.Position,
                            "un champ tableau exige un initialiseur agrégé : "
                                + trouve->Nom,
                            "an array field requires an aggregate initializer: "
                                + trouve->Nom);
                    (void)AnalyserInitialiseur(
                        *initialiseur.Arguments.front(),
                        trouve->Type,
                        fonction);
                }
            }

            for (const auto& champ : classe->Champs)
            {
                auto typeElement = champ.Type;
                while (typeElement.EstTableau())
                    typeElement = typeElement.ElementTableau();
                const bool contientClasse = typeElement.Genre
                        == GenreType::Structure
                    && typeElement.NiveauPointeur == 0
                    && _Structures.at(typeElement.Nom)->EstClasse;
                if (champsInitialises.contains(champ.Nom))
                    continue;

                if (champ.InitialiseurParDefaut)
                {
                    if (contientClasse)
                        Erreur(
                            champ.Position,
                            "un champ objet classe utilise un constructeur, pas un initialiseur par défaut avec '=' : "
                                + champ.Nom,
                            "a class object field uses a constructor, not a default '=' initializer: "
                                + champ.Nom);
                    InitialiseurChampConstructeur initialiseur;
                    initialiseur.Nom = champ.Nom;
                    initialiseur.NomCanonique = champ.Nom;
                    initialiseur.Position = champ.Position;
                    initialiseur.Type = champ.Type;
                    initialiseur.Decalage = champ.Decalage;
                    initialiseur.EstImplicite = true;
                    initialiseur.InitialiseurParDefaut =
                        champ.InitialiseurParDefaut.get();
                    (void)AnalyserInitialiseur(
                        *champ.InitialiseurParDefaut,
                        champ.Type,
                        fonction);
                    fonction.InitialiseursChamps.push_back(
                        std::move(initialiseur));
                    continue;
                }
                if (!contientClasse)
                {
                    if (champ.Type.EstConstante
                        && !champ.Type.EstAdresse())
                        Erreur(
                            champ.Position,
                            "un champ constant doit être initialisé explicitement ou posséder une valeur par défaut : "
                                + champ.Nom,
                            "a const field must be explicitly initialized or have a default value: "
                                + champ.Nom);
                    continue;
                }

                InitialiseurChampConstructeur initialiseur;
                initialiseur.Nom = champ.Nom;
                initialiseur.NomCanonique = champ.Nom;
                initialiseur.Position = champ.Position;
                initialiseur.Type = champ.Type;
                initialiseur.Decalage = champ.Decalage;
                initialiseur.EstObjetClasse = true;
                initialiseur.EstImplicite = true;
                const auto* classeChamp = _Structures.at(typeElement.Nom);
                if (champ.Type.EstTableau())
                {
                    PlanifierConstructionTypeObjet(
                        champ.Type,
                        0,
                        fonction.ClasseProprietaire,
                        fonction,
                        champ.Position,
                        initialiseur.EtapesConstructionImplicite);
                    fonction.InitialiseursChamps.push_back(
                        std::move(initialiseur));
                    continue;
                }
                const auto nomConstructeurChamp =
                    classeChamp->NomComplet() + "::$constructeur";
                if (_Surcharges.contains(nomConstructeurChamp))
                {
                    std::vector<Expression*> sansArguments;
                    auto* constructeur = ResoudreConstructeurClasse(
                        *classeChamp,
                        sansArguments,
                        fonction,
                        champ.Position,
                        fonction.ClasseProprietaire,
                        initialiseur.ArgumentsConstructeurParReference);
                    initialiseur.SymboleConstructeur =
                        constructeur->NomComplet();
                }
                else
                    PlanifierConstructionImplicite(
                        *classeChamp,
                        0,
                        fonction.ClasseProprietaire,
                        fonction,
                        champ.Position,
                        initialiseur.EtapesConstructionImplicite);
                fonction.InitialiseursChamps.push_back(
                    std::move(initialiseur));
            }

            std::sort(
                fonction.InitialiseursChamps.begin(),
                fonction.InitialiseursChamps.end(),
                [&](const InitialiseurChampConstructeur& gauche,
                    const InitialiseurChampConstructeur& droite)
                {
                    const auto indexChamp = [&](const std::string& nom)
                    {
                        return std::distance(
                            classe->Champs.begin(),
                            std::find_if(
                                classe->Champs.begin(),
                                classe->Champs.end(),
                                [&](const ChampStructure& champ)
                                { return champ.Nom == nom; }));
                    };
                    return indexChamp(gauche.NomCanonique)
                        < indexChamp(droite.NomCanonique);
                });
        }
        AnalyserInstruction(*fonction.Corps, fonction);
    }

    std::uint64_t AnalyseurSemantique::EvaluerConstante(const Expression& expression) const
    {
        if (expression.Genre == GenreExpression::Entier)
            return static_cast<const ExpressionEntier&>(expression).Valeur;
        if (expression.Genre == GenreExpression::Chaine)
            Erreur(
                expression.Position,
                "une chaîne n’est pas une constante entière",
                "a string is not an integer constant");
        if (expression.Genre == GenreExpression::Variable)
        {
            const auto& variable = static_cast<const ExpressionVariable&>(expression);
            if (variable.EstConstanteEnumeration)
                return static_cast<std::uint64_t>(variable.ValeurEnumeration);
        }
        if (expression.Genre == GenreExpression::Unaire)
        {
            const auto& unaire = static_cast<const ExpressionUnaire&>(expression);
            const auto valeur = EvaluerConstante(*unaire.Operande);
            if (unaire.Operateur == "+")
                return NormaliserBits(valeur, expression.TypeSemantique);
            if (unaire.Operateur == "-")
                return NormaliserBits(std::uint64_t{0} - valeur, expression.TypeSemantique);
            if (unaire.Operateur == "!") return valeur == 0 ? 1U : 0U;
            if (unaire.Operateur == "~")
                return NormaliserBits(~valeur, expression.TypeSemantique);
        }
        if (expression.Genre == GenreExpression::Binaire)
        {
            const auto& binaire = static_cast<const ExpressionBinaire&>(expression);
            const auto gauche = EvaluerConstante(*binaire.Gauche);
            if (binaire.Operateur == "&&")
                return gauche != 0
                    && EvaluerConstante(*binaire.Droite) != 0;
            if (binaire.Operateur == "||")
                return gauche != 0
                    || EvaluerConstante(*binaire.Droite) != 0;
            const auto droite = EvaluerConstante(*binaire.Droite);
            if (binaire.Operateur == "+")
                return NormaliserBits(gauche + droite, expression.TypeSemantique);
            if (binaire.Operateur == "-")
                return NormaliserBits(gauche - droite, expression.TypeSemantique);
            if (binaire.Operateur == "*")
                return NormaliserBits(gauche * droite, expression.TypeSemantique);
            if (binaire.Operateur == "&")
                return NormaliserBits(gauche & droite, expression.TypeSemantique);
            if (binaire.Operateur == "|")
                return NormaliserBits(gauche | droite, expression.TypeSemantique);
            if (binaire.Operateur == "^")
                return NormaliserBits(gauche ^ droite, expression.TypeSemantique);
            if (binaire.Operateur == "<<" || binaire.Operateur == ">>")
            {
                const auto largeur = LargeurType(binaire.Gauche->TypeSemantique);
                const auto distance = droite & (largeur - 1);
                if (binaire.Operateur == "<<")
                    return NormaliserBits(
                        NormaliserBits(gauche, binaire.Gauche->TypeSemantique)
                            << distance,
                        expression.TypeSemantique);
                if (EstTypeSigne(binaire.Gauche->TypeSemantique))
                    return NormaliserBits(
                        static_cast<std::uint64_t>(
                            InterpreterSigne(
                                gauche, binaire.Gauche->TypeSemantique)
                            >> distance),
                        expression.TypeSemantique);
                return NormaliserBits(
                    NormaliserBits(gauche, binaire.Gauche->TypeSemantique)
                        >> distance,
                    expression.TypeSemantique);
            }
            if (binaire.Operateur == "/" || binaire.Operateur == "%")
            {
                const auto& typeOperande = binaire.Gauche->TypeSemantique;
                if (NormaliserBits(droite, typeOperande) == 0)
                    Erreur(expression.Position,
                           "division constante par zéro",
                           "constant division by zero");
                std::uint64_t resultat = 0;
                if (EstTypeSigne(typeOperande))
                {
                    const auto gaucheSignee = InterpreterSigne(gauche, typeOperande);
                    const auto droiteSignee = InterpreterSigne(droite, typeOperande);
                    if (gaucheSignee == INT64_MIN && droiteSignee == -1)
                        resultat = binaire.Operateur == "/"
                            ? static_cast<std::uint64_t>(INT64_MIN)
                            : 0;
                    else if (binaire.Operateur == "/")
                        resultat = static_cast<std::uint64_t>(gaucheSignee / droiteSignee);
                    else resultat = static_cast<std::uint64_t>(gaucheSignee % droiteSignee);
                }
                else
                {
                    const auto gaucheNonSignee = NormaliserBits(gauche, typeOperande);
                    const auto droiteNonSignee = NormaliserBits(droite, typeOperande);
                    resultat = binaire.Operateur == "/"
                        ? gaucheNonSignee / droiteNonSignee
                        : gaucheNonSignee % droiteNonSignee;
                }
                return NormaliserBits(resultat, expression.TypeSemantique);
            }
            if (binaire.Operateur == "==" || binaire.Operateur == "!=")
            {
                const auto gaucheNormalisee = NormaliserBits(
                    gauche, binaire.Gauche->TypeSemantique);
                const auto droiteNormalisee = NormaliserBits(
                    droite, binaire.Droite->TypeSemantique);
                return binaire.Operateur == "=="
                    ? gaucheNormalisee == droiteNormalisee
                    : gaucheNormalisee != droiteNormalisee;
            }
            if (EstTypeSigne(binaire.Gauche->TypeSemantique))
            {
                const auto gaucheSignee = InterpreterSigne(
                    gauche, binaire.Gauche->TypeSemantique);
                const auto droiteSignee = InterpreterSigne(
                    droite, binaire.Droite->TypeSemantique);
                if (binaire.Operateur == "<") return gaucheSignee < droiteSignee;
                if (binaire.Operateur == "<=") return gaucheSignee <= droiteSignee;
                if (binaire.Operateur == ">") return gaucheSignee > droiteSignee;
                if (binaire.Operateur == ">=") return gaucheSignee >= droiteSignee;
            }
            else
            {
                const auto gaucheNonSignee = NormaliserBits(
                    gauche, binaire.Gauche->TypeSemantique);
                const auto droiteNonSignee = NormaliserBits(
                    droite, binaire.Droite->TypeSemantique);
                if (binaire.Operateur == "<") return gaucheNonSignee < droiteNonSignee;
                if (binaire.Operateur == "<=") return gaucheNonSignee <= droiteNonSignee;
                if (binaire.Operateur == ">") return gaucheNonSignee > droiteNonSignee;
                if (binaire.Operateur == ">=") return gaucheNonSignee >= droiteNonSignee;
            }
        }
        if (expression.Genre == GenreExpression::Conversion)
        {
            const auto& conversion = static_cast<const ExpressionConversion&>(expression);
            const auto valeur = EvaluerConstante(*conversion.Valeur);
            const auto& source = conversion.Valeur->TypeSemantique;
            const auto valeurEtendue = EstTypeSigne(source)
                ? static_cast<std::uint64_t>(InterpreterSigne(valeur, source))
                : NormaliserBits(valeur, source);
            return conversion.TypeCible.EstBooleen()
                ? (valeurEtendue != 0 ? 1U : 0U)
                : NormaliserBits(valeurEtendue, conversion.TypeCible);
        }
        Erreur(expression.Position,
               "l’initialiseur global doit être une expression constante",
               "global initializer must be a constant expression");
    }

    void AnalyseurSemantique::EcrireInitialiseurGlobal(
        const Expression& expression,
        const TypeGs& typeCible,
        std::uint32_t decalage,
        VariableGlobale& variable)
    {
        if (expression.Genre == GenreExpression::Agregat)
        {
            const auto& agregat = static_cast<const ExpressionAgregat&>(expression);
            if (typeCible.EstTableau())
            {
                const auto typeElement = typeCible.ElementTableau();
                const auto tailleElement = TailleType(typeElement, _Structures);
                for (std::size_t index = 0; index < agregat.Elements.size(); ++index)
                    EcrireInitialiseurGlobal(
                        *agregat.Elements[index],
                        typeElement,
                        decalage + static_cast<std::uint32_t>(index) * tailleElement,
                        variable);
                return;
            }
            if (typeCible.EstStructure())
            {
                const auto* structure = _Structures.at(typeCible.Nom);
                for (std::size_t index = 0; index < agregat.Elements.size(); ++index)
                    EcrireInitialiseurGlobal(
                        *agregat.Elements[index],
                        structure->Champs[index].Type,
                        decalage + structure->Champs[index].Decalage,
                        variable);
                return;
            }
            if (!agregat.Elements.empty())
                EcrireInitialiseurGlobal(
                    *agregat.Elements.front(), typeCible, decalage, variable);
            return;
        }

        if (typeCible.EstStructure() || typeCible.EstTableau())
            Erreur(
                expression.Position,
                "une globale agrégée doit être initialisée par une liste constante",
                "an aggregate global must be initialized by a constant list");

        if (typeCible.EstPointeurFonction())
        {
            const Expression* cible = &expression;
            if (cible->Genre == GenreExpression::Unaire
                && static_cast<const ExpressionUnaire&>(*cible).Operateur == "&")
                cible = static_cast<const ExpressionUnaire&>(*cible).Operande.get();
            if (cible->Genre != GenreExpression::Variable
                || !static_cast<const ExpressionVariable&>(*cible).EstFonction)
                Erreur(
                    expression.Position,
                    "un pointeur de fonction global doit référencer une fonction",
                    "a global function pointer must reference a function");
            variable.RelocalisationsInitialiseur.push_back({
                decalage,
                static_cast<const ExpressionVariable&>(*cible).Nom
            });
            return;
        }

        if (typeCible.EstAdresse())
            Erreur(
                expression.Position,
                "initialisation globale de pointeur de données non prise en charge",
                "global data-pointer initialization is unsupported");
        if (!EstExpressionConstante(expression))
            Erreur(
                expression.Position,
                "l’initialiseur global doit être constant",
                "global initializer must be constant");
        auto valeur = EvaluerConstante(expression);
        if (!ValeurDansType(valeur, expression.TypeSemantique, typeCible))
            Erreur(
                expression.Position,
                "valeur globale hors de la plage du type",
                "global value is outside the type range");
        valeur = NormaliserBits(valeur, typeCible);
        const auto taille = TailleType(typeCible, _Structures);
        if (decalage > variable.DonneesInitiales.size()
            || taille > variable.DonneesInitiales.size() - decalage)
            throw std::logic_error("initialiseur global hors stockage");
        for (std::uint32_t index = 0; index < taille; ++index)
            variable.DonneesInitiales[decalage + index] =
                static_cast<std::uint8_t>(valeur >> (index * 8));
    }

    void AnalyseurSemantique::AnalyserVariableGlobale(VariableGlobale& variable)
    {
        ResoudreType(variable.Type, variable.Espace, variable.Position);
        auto typeElement = variable.Type;
        while (typeElement.EstTableau())
            typeElement = typeElement.ElementTableau();
        if (typeElement.EstStructure()
            && typeElement.NiveauPointeur == 0
            && _Structures.at(typeElement.Nom)->EstClasse)
            Erreur(
                variable.Position,
                "les objets classes globaux ne sont pas pris en charge : utilisez une durée de vie locale ou un stockage initialisé explicitement",
                "global class objects are unsupported: use local lifetime or explicitly initialized storage");
        if (variable.Type.EstReference)
            Erreur(variable.Position,
                   "les références globales ne sont pas prises en charge",
                   "global references are unsupported");
        if (variable.Type.Genre == GenreType::Vide
            && variable.Type.NiveauPointeur == 0)
            Erreur(variable.Position, "une variable globale ne peut pas être 'vide'", "a global variable cannot be 'void'");
        if (variable.EstExterne)
        {
            if (variable.EstPublique)
                Erreur(variable.Position,
                       "une variable globale importée ne peut pas être exportée",
                       "an imported global variable cannot be exported");
            if (variable.Initialiseur)
                Erreur(variable.Position,
                       "une variable globale externe ne peut pas être initialisée",
                       "an external global variable cannot be initialized");
            return;
        }
        if (variable.Type.EstConstante
            && !variable.Type.EstAdresse()
            && !variable.Initialiseur)
            Erreur(variable.Position,
                   "une globale constante doit être initialisée",
                   "a const global must be initialized");
        if (!variable.Initialiseur) return;
        Fonction contexte;
        contexte.Espace = variable.Espace;
        if (variable.Type.EstTableau()
            && variable.Initialiseur->Genre != GenreExpression::Agregat)
            Erreur(
                variable.Position,
                "un tableau global exige un initialiseur agrégé",
                "a global array requires an aggregate initializer");
        (void)AnalyserInitialiseur(
            *variable.Initialiseur, variable.Type, contexte);
        variable.ValeurInitiale = 0;
        variable.SymboleInitialiseur.clear();
        variable.RelocalisationsInitialiseur.clear();
        variable.DonneesInitiales.assign(
            TailleType(variable.Type, _Structures), 0);
        EcrireInitialiseurGlobal(
            *variable.Initialiseur, variable.Type, 0, variable);
        if (variable.DonneesInitiales.size() <= sizeof(variable.ValeurInitiale))
            for (std::size_t index = 0;
                 index < variable.DonneesInitiales.size();
                 ++index)
                variable.ValeurInitiale |=
                    static_cast<std::uint64_t>(variable.DonneesInitiales[index])
                    << (index * 8);
        if (variable.RelocalisationsInitialiseur.size() == 1
            && variable.RelocalisationsInitialiseur.front().Decalage == 0)
            variable.SymboleInitialiseur =
                variable.RelocalisationsInitialiseur.front().Symbole;
        variable.EstInitialisee = true;
    }

    void AnalyseurSemantique::Analyser(Programme& programme)
    {
        _Programme = &programme;
        _Structures.clear();
        _Enumerations.clear();
        _ValeursEnumerations.clear();
        _Fonctions.clear();
        _Surcharges.clear();
        _Globales.clear();
        _Aliases.clear();
        _StructuresCalculees.clear();
        _StructuresEnCours.clear();
        _AliasesResolus.clear();
        _AliasesEnCours.clear();
        _Variables.clear();

        for (auto& enumeration : programme.Enumerations)
            if (!_Enumerations.emplace(enumeration.NomComplet(), &enumeration).second)
                Erreur(enumeration.Position,
                       "énumération déclarée plusieurs fois",
                       "enum declared more than once");
        for (auto& structure : programme.Structures)
        {
            if (_Enumerations.contains(structure.NomComplet()))
                Erreur(structure.Position,
                       "nom déjà utilisé par une énumération",
                       "name is already used by an enum");
            if (!_Structures.emplace(structure.NomComplet(), &structure).second)
                Erreur(structure.Position, "structure déclarée plusieurs fois", "struct declared more than once");
        }
        for (auto& fonction : programme.Fonctions)
        {
            fonction.NomSource = fonction.NomSource.empty()
                ? fonction.Nom : fonction.NomSource;
            if (_Structures.contains(fonction.NomSourceComplet())
                || _Enumerations.contains(fonction.NomSourceComplet()))
                Erreur(fonction.Position, "nom déjà utilisé par un type", "name is already used by a type");
            _Surcharges[fonction.NomSourceComplet()].push_back(&fonction);
            if (fonction.EstVirtuelle)
            {
                const auto classe = _Structures.find(
                    fonction.ClasseProprietaire);
                if (classe == _Structures.end())
                    Erreur(fonction.Position,
                           "classe propriétaire introuvable pour la méthode virtuelle",
                           "owning class not found for virtual method");
                classe->second->EstPolymorphe = true;
            }
        }
        for (auto& variable : programme.VariablesGlobales)
        {
            const auto nom = variable.NomComplet();
            if (!_Globales.emplace(nom, &variable).second)
                Erreur(variable.Position, "variable globale déclarée plusieurs fois", "global variable declared more than once");
            if (_Surcharges.contains(nom))
                Erreur(variable.Position, "nom déjà utilisé par une fonction", "name is already used by a function");
            if (_Structures.contains(nom) || _Enumerations.contains(nom))
                Erreur(variable.Position, "nom déjà utilisé par un type", "name is already used by a type");
        }
        for (auto& alias : programme.Aliases)
        {
            const auto nom = alias.NomComplet();
            if (_Structures.contains(nom)
                || _Enumerations.contains(nom)
                || _Surcharges.contains(nom)
                || _Globales.contains(nom))
                Erreur(
                    alias.Position,
                    "alias en conflit avec une déclaration : " + nom,
                    "alias conflicts with a declaration: " + nom);
            if (!_Aliases.emplace(nom, &alias).second)
                Erreur(
                    alias.Position,
                    "alias déclaré plusieurs fois : " + nom,
                    "alias declared more than once: " + nom);
        }
        for (auto& enumeration : programme.Enumerations) CalculerEnumeration(enumeration);
        for (auto& alias : programme.Aliases) ResoudreAlias(alias);
        for (auto& alias : programme.Aliases)
        {
            const auto nom = alias.NomComplet();
            switch (alias.GenreCible)
            {
                case GenreCibleAlias::Structure:
                    _Structures.emplace(nom, _Structures.at(alias.CibleCanonique));
                    break;
                case GenreCibleAlias::Fonction:
                {
                    const auto cible = _Surcharges.find(alias.CibleCanonique);
                    if (cible == _Surcharges.end())
                        Erreur(alias.Position,
                               "fonction cible d’alias introuvable",
                               "aliased function target not found");
                    _Surcharges.emplace(nom, cible->second);
                    break;
                }
                case GenreCibleAlias::VariableGlobale:
                    _Globales.emplace(nom, _Globales.at(alias.CibleCanonique));
                    break;
                case GenreCibleAlias::Inconnue:
                    Erreur(alias.Position, "alias non résolu", "unresolved alias");
            }
        }
        for (auto& structure : programme.Structures)
            ResoudreHeritage(structure);
        if (programme.Fonctions.empty())
            throw ErreurCompilation(
                "le programme ne contient aucune fonction",
                "the program contains no function",
                1, 1);

        for (auto& structure : programme.Structures) CalculerStructure(structure);
        for (auto& fonction : programme.Fonctions) PreparerFonction(fonction);
        IndexerFonctions(programme);
        for (auto& structure : programme.Structures)
        {
            bool possedeInitialiseurParDefaut = false;
            for (const auto& champ : structure.Champs)
            {
                if (!champ.InitialiseurParDefaut) continue;
                possedeInitialiseurParDefaut = true;
                auto typeElement = champ.Type;
                while (typeElement.EstTableau())
                    typeElement = typeElement.ElementTableau();
                if (typeElement.EstStructure()
                    && typeElement.NiveauPointeur == 0
                    && _Structures.at(typeElement.Nom)->EstClasse)
                    Erreur(
                        champ.Position,
                        "un champ objet classe utilise un constructeur, pas un initialiseur par défaut avec '=' : "
                            + champ.Nom,
                        "a class object field uses a constructor, not a default '=' initializer: "
                            + champ.Nom);
            }
            if (!possedeInitialiseurParDefaut) continue;
            const bool constructeurDefini = std::any_of(
                programme.Fonctions.begin(),
                programme.Fonctions.end(),
                [&](const Fonction& fonction)
                {
                    return fonction.EstConstructeur
                        && !fonction.EstExterne
                        && fonction.ClasseProprietaire
                            == structure.NomComplet();
                });
            if (!constructeurDefini)
                Erreur(
                    structure.Position,
                    "une classe possédant une valeur de champ par défaut doit déclarer un constructeur",
                    "a class with a default field value must declare a constructor");
        }
        for (auto& alias : programme.Aliases)
        {
            if (alias.GenreCible != GenreCibleAlias::Fonction) continue;
            const auto cible = std::find_if(
                programme.Fonctions.begin(), programme.Fonctions.end(),
                [&](const Fonction& fonction)
                {
                    return fonction.NomSourceComplet() == alias.CibleCanonique;
                });
            if (cible == programme.Fonctions.end())
                Erreur(alias.Position,
                       "fonction cible d’alias introuvable après surcharge",
                       "aliased function target missing after overload indexing");
            alias.CibleCanonique = cible->NomComplet();
            _Surcharges.emplace(
                alias.NomComplet(), std::vector<Fonction*>{&*cible});
        }
        for (auto& variable : programme.VariablesGlobales) AnalyserVariableGlobale(variable);
        for (auto& fonction : programme.Fonctions) AnalyserFonction(fonction);
        for (const auto& fonction : programme.Fonctions)
        {
            if (!fonction.DelegueConstructeur || fonction.EstExterne)
                continue;
            std::unordered_set<std::string> visites;
            const Fonction* courant = &fonction;
            while (courant->DelegueConstructeur)
            {
                if (!visites.insert(courant->NomComplet()).second)
                    Erreur(
                        fonction.Position,
                        "cycle de délégation entre constructeurs détecté",
                        "constructor delegation cycle detected");
                const auto cible = _Fonctions.find(
                    courant->SymboleConstructeurDelegue);
                if (cible == _Fonctions.end())
                    throw std::logic_error(
                        "constructeur délégué indexé introuvable");
                courant = cible->second;
            }
        }
    }
}
