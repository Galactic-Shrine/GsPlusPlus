#include "GsPP/AnalyseurSyntaxique.hpp"
#include "GsPP/ErreurCompilation.hpp"

#include <algorithm>
#include <charconv>

namespace GsPP
{
    AnalyseurSyntaxique::AnalyseurSyntaxique(
        std::vector<Jeton> jetons,
        std::string fichier,
        bool estInterface)
        : _Jetons(std::move(jetons)),
          _Fichier(std::move(fichier)),
          _EstInterface(estInterface)
    {
    }

    PositionSource AnalyseurSyntaxique::Position(const Jeton& jeton) const
    {
        return {_Fichier, jeton.Ligne, jeton.Colonne};
    }

    const Jeton& AnalyseurSyntaxique::Courant() const { return _Jetons.at(_Position); }
    const Jeton& AnalyseurSyntaxique::Precedent() const { return _Jetons.at(_Position - 1); }
    bool AnalyseurSyntaxique::Est(GenreJeton genre) const { return Courant().Genre == genre; }

    bool AnalyseurSyntaxique::Accepter(GenreJeton genre)
    {
        if (!Est(genre)) return false;
        ++_Position;
        return true;
    }

    bool AnalyseurSyntaxique::AccepterUn(std::initializer_list<GenreJeton> genres)
    {
        for (const auto genre : genres) if (Accepter(genre)) return true;
        return false;
    }

    const Jeton& AnalyseurSyntaxique::Exiger(
        GenreJeton genre,
        const char* fr,
        const char* en)
    {
        if (!Est(genre))
            throw ErreurCompilation(fr, en, Courant().Ligne, Courant().Colonne, _Fichier);
        ++_Position;
        return Precedent();
    }

    void AnalyseurSyntaxique::ExigerFermetureType(
        const char* fr,
        const char* en)
    {
        if (Accepter(GenreJeton::Superieur)) return;
        if (Est(GenreJeton::DecalageDroite))
        {
            // Dans un type imbriqué, le premier caractère de « >> » ferme le
            // type intérieur et le second reste disponible pour le type parent.
            _Jetons[_Position].Genre = GenreJeton::Superieur;
            _Jetons[_Position].Texte = ">";
            return;
        }
        throw ErreurCompilation(
            fr, en, Courant().Ligne, Courant().Colonne, _Fichier);
    }

    Programme AnalyseurSyntaxique::Analyser()
    {
        Programme programme;
        AnalyserDeclarations(programme, "");
        Exiger(GenreJeton::Fin, "fin de fichier attendue", "expected end of file");
        return programme;
    }

    void AnalyseurSyntaxique::AnalyserDeclarations(
        Programme& programme,
        const std::string& espaceCourant)
    {
        while (!Est(GenreJeton::Fin) && !Est(GenreJeton::AccoladeFermante))
        {
            if (Accepter(GenreJeton::Espace))
                AnalyserEspace(programme, espaceCourant);
            else if (Accepter(GenreJeton::Classe))
                programme.Structures.push_back(AnalyserStructure(
                    programme, espaceCourant, false, true));
            else if (Accepter(GenreJeton::Structure))
                programme.Structures.push_back(AnalyserStructure(
                    programme, espaceCourant));
            else if (Accepter(GenreJeton::Union))
                programme.Structures.push_back(AnalyserStructure(
                    programme, espaceCourant, true));
            else if (Accepter(GenreJeton::Enumeration))
                programme.Enumerations.push_back(AnalyserEnumeration(espaceCourant));
            else if (Est(GenreJeton::Alias))
                programme.Aliases.push_back(AnalyserAlias(espaceCourant));
            else
                AnalyserFonctionOuGlobale(programme, espaceCourant);
        }
    }

    std::string AnalyseurSyntaxique::AnalyserNomQualifie()
    {
        std::string nom = Exiger(
            GenreJeton::Identifiant,
            "identifiant attendu",
            "expected identifier").Texte;
        while (Accepter(GenreJeton::DeuxPointsDouble))
        {
            nom += "::";
            nom += Exiger(
                GenreJeton::Identifiant,
                "nom attendu après '::'",
                "expected name after '::'").Texte;
        }
        return nom;
    }

    std::string AnalyseurSyntaxique::AnalyserNomFonction()
    {
        if (!Accepter(GenreJeton::Operateur))
            return Exiger(
                GenreJeton::Identifiant,
                "nom de fonction attendu",
                "expected function name").Texte;

        if (!AccepterUn({
                GenreJeton::Plus, GenreJeton::Moins, GenreJeton::Etoile,
                GenreJeton::BarreOblique, GenreJeton::Pourcentage,
                GenreJeton::EgalEgal, GenreJeton::Different,
                GenreJeton::Inferieur, GenreJeton::InferieurEgal,
                GenreJeton::Superieur, GenreJeton::SuperieurEgal,
                GenreJeton::Esperluette, GenreJeton::BarreVerticale,
                GenreJeton::Circonflexe, GenreJeton::DecalageGauche,
                GenreJeton::DecalageDroite, GenreJeton::PointExclamation,
                GenreJeton::Tilde}))
            throw ErreurCompilation(
                "opérateur surchargeable attendu",
                "expected overloadable operator",
                Courant().Ligne, Courant().Colonne, _Fichier);
        return "operator" + Precedent().Texte;
    }

    void AnalyseurSyntaxique::AnalyserEspace(
        Programme& programme,
        const std::string& espaceParent)
    {
        const std::string nom = AnalyserNomQualifie();
        const std::string complet = espaceParent.empty() ? nom : espaceParent + "::" + nom;
        Exiger(GenreJeton::AccoladeOuvrante, "'{' attendue", "expected '{'");
        AnalyserDeclarations(programme, complet);
        Exiger(GenreJeton::AccoladeFermante, "'}' attendue", "expected '}'");
        Accepter(GenreJeton::PointVirgule);
    }

    TypeGs AnalyseurSyntaxique::AnalyserType()
    {
        TypeGs type;
        bool continuer = true;
        while (continuer)
        {
            if (Accepter(GenreJeton::Constante)) type.EstConstante = true;
            else if (Accepter(GenreJeton::Volatile)) type.EstVolatile = true;
            else continuer = false;
        }
        if (Accepter(GenreJeton::Entier8)) type.Genre = GenreType::Entier8;
        else if (Accepter(GenreJeton::Entier16)) type.Genre = GenreType::Entier16;
        else if (Accepter(GenreJeton::Entier32)) type.Genre = GenreType::Entier32;
        else if (Accepter(GenreJeton::Entier64)) type.Genre = GenreType::Entier64;
        else if (Accepter(GenreJeton::Naturel8)) type.Genre = GenreType::Naturel8;
        else if (Accepter(GenreJeton::Naturel16)) type.Genre = GenreType::Naturel16;
        else if (Accepter(GenreJeton::Naturel32)) type.Genre = GenreType::Naturel32;
        else if (Accepter(GenreJeton::Naturel64)) type.Genre = GenreType::Naturel64;
        else if (Accepter(GenreJeton::Booleen)) type.Genre = GenreType::Booleen;
        else if (Accepter(GenreJeton::Octet)) type.Genre = GenreType::Octet;
        else if (Accepter(GenreJeton::Caractere)) type.Genre = GenreType::Caractere;
        else if (Accepter(GenreJeton::Vide)) type.Genre = GenreType::Vide;
        else if (Accepter(GenreJeton::PointeurFonction))
        {
            type.Genre = GenreType::PointeurFonction;
            Exiger(
                GenreJeton::Inferieur,
                "'<' attendu après pointeur_fonction",
                "expected '<' after function_pointer");
            type.RetourFonction = std::make_shared<TypeGs>(AnalyserType());
            Exiger(
                GenreJeton::ParentheseOuvrante,
                "'(' attendue dans la signature de fonction",
                "expected '(' in function signature");
            if (!Est(GenreJeton::ParentheseFermante))
            {
                do type.ParametresFonction.push_back(AnalyserType());
                while (Accepter(GenreJeton::Virgule));
            }
            Exiger(
                GenreJeton::ParentheseFermante,
                "')' attendue dans la signature de fonction",
                "expected ')' in function signature");
            ExigerFermetureType(
                "'>' attendu après la signature de fonction",
                "expected '>' after function signature");
        }
        else if (Est(GenreJeton::Identifiant))
        {
            type.Genre = GenreType::Structure;
            type.Nom = AnalyserNomQualifie();
        }
        else
        {
            throw ErreurCompilation(
                "type attendu",
                "expected type",
                Courant().Ligne,
                Courant().Colonne,
                _Fichier);
        }
        while (Accepter(GenreJeton::Etoile)) ++type.NiveauPointeur;
        if (Accepter(GenreJeton::Esperluette)) type.EstReference = true;
        return type;
    }

    void AnalyseurSyntaxique::AnalyserDimensionsTableau(TypeGs& type)
    {
        while (Accepter(GenreJeton::CrochetOuvrant))
        {
            const auto& taille = Exiger(
                GenreJeton::NombreEntier,
                "taille entière attendue dans le tableau",
                "integer array size expected");
            std::string texte = taille.Texte;
            texte.erase(std::remove(texte.begin(), texte.end(), '_'), texte.end());
            std::uint64_t valeur = 0;
            const auto resultat = std::from_chars(
                texte.data(), texte.data() + texte.size(), valeur);
            if (resultat.ec != std::errc{}
                || resultat.ptr != texte.data() + texte.size()
                || valeur == 0
                || valeur > 0x7FFFFFFFULL)
                throw ErreurCompilation(
                    "taille de tableau invalide",
                    "invalid array size",
                    taille.Ligne,
                    taille.Colonne,
                    _Fichier);
            Exiger(GenreJeton::CrochetFermant, "']' attendu", "expected ']'");
            type.DimensionsTableau.push_back(static_cast<std::uint32_t>(valeur));
        }
    }

    Structure AnalyseurSyntaxique::AnalyserStructure(
        Programme& programme,
        const std::string& espaceCourant,
        bool estUnion,
        bool estClasse)
    {
        Structure structure;
        structure.Espace = espaceCourant;
        structure.EstUnion = estUnion;
        structure.EstClasse = estClasse;
        structure.Position = Position(Courant());
        structure.Nom = Exiger(
            GenreJeton::Identifiant,
            "nom de structure attendu",
            "expected structure name").Texte;
        if (estClasse && Accepter(GenreJeton::DeuxPoints))
        {
            if (AccepterUn({GenreJeton::Publique, GenreJeton::Protegee,
                            GenreJeton::Privee}))
            {
                if (Precedent().Genre == GenreJeton::Protegee)
                    structure.VisibiliteHeritage = VisibiliteMembre::Protegee;
                else if (Precedent().Genre == GenreJeton::Privee)
                    structure.VisibiliteHeritage = VisibiliteMembre::Privee;
            }
            structure.ClasseBase = AnalyserNomQualifie();
        }
        Exiger(GenreJeton::AccoladeOuvrante, "'{' attendue", "expected '{'");
        auto visibilite = estClasse
            ? VisibiliteMembre::Privee
            : VisibiliteMembre::Publique;
        while (!Est(GenreJeton::AccoladeFermante))
        {
            if (AccepterUn({GenreJeton::Publique, GenreJeton::Privee,
                            GenreJeton::Protegee}))
            {
                if (Precedent().Genre == GenreJeton::Publique)
                    visibilite = VisibiliteMembre::Publique;
                else if (Precedent().Genre == GenreJeton::Protegee)
                    visibilite = VisibiliteMembre::Protegee;
                else visibilite = VisibiliteMembre::Privee;
                Exiger(
                    GenreJeton::DeuxPoints,
                    "':' attendu après la visibilité",
                    "expected ':' after visibility");
                continue;
            }
            if (estClasse
                && (Est(GenreJeton::Constructeur)
                    || Est(GenreJeton::Destructeur)
                    || Est(GenreJeton::Virtuel)
                    || Est(GenreJeton::Remplacer)))
            {
                AnalyserMembreClasse(programme, structure, visibilite);
                continue;
            }
            if (Est(GenreJeton::Alias))
            {
                AliasChamp alias;
                alias.Position = Position(Courant());
                Exiger(GenreJeton::Alias, "'alias' attendu", "expected 'alias'");
                alias.Nom = Exiger(
                    GenreJeton::Identifiant,
                    "nom d’alias de champ attendu",
                    "expected field alias name").Texte;
                Exiger(GenreJeton::Egal, "'=' attendu dans l’alias", "expected '=' in alias");
                alias.Cible = Exiger(
                    GenreJeton::Identifiant,
                    "champ cible attendu",
                    "expected target field").Texte;
                Exiger(GenreJeton::PointVirgule, "';' attendu après l’alias", "expected ';' after alias");
                structure.AliasesChamps.push_back(std::move(alias));
                continue;
            }
            if (estClasse)
            {
                // Une méthode commence comme un champ, mais le nom est suivi
                // d'une parenthèse. Le membre est analysé ici en une seule fois.
                const auto positionSauvee = _Position;
                (void)AnalyserType();
                if (Est(GenreJeton::Operateur))
                {
                    _Position = positionSauvee;
                    AnalyserMembreClasse(programme, structure, visibilite);
                    continue;
                }
                (void)Exiger(
                    GenreJeton::Identifiant,
                    "nom de membre attendu",
                    "expected member name");
                const bool estMethode = Est(GenreJeton::ParentheseOuvrante);
                _Position = positionSauvee;
                if (estMethode)
                {
                    AnalyserMembreClasse(programme, structure, visibilite);
                    continue;
                }
            }
            ChampStructure champ;
            champ.Position = Position(Courant());
            champ.Visibilite = visibilite;
            champ.Type = AnalyserType();
            champ.Nom = Exiger(
                GenreJeton::Identifiant,
                "nom de champ attendu",
                "expected field name").Texte;
            AnalyserDimensionsTableau(champ.Type);
            if (Accepter(GenreJeton::Egal))
            {
                if (!estClasse)
                    throw ErreurCompilation(
                        "un initialiseur de champ par défaut est réservé aux classes",
                        "a default field initializer is only allowed in classes",
                        champ.Position.Ligne,
                        champ.Position.Colonne,
                        champ.Position.Fichier);
                champ.InitialiseurParDefaut = AnalyserExpression();
            }
            Exiger(GenreJeton::PointVirgule, "';' attendu", "expected ';'");
            structure.Champs.push_back(std::move(champ));
        }
        Exiger(GenreJeton::AccoladeFermante, "'}' attendue", "expected '}'");
        Exiger(GenreJeton::PointVirgule, "';' attendu après la structure", "expected ';' after struct");
        return structure;
    }

    void AnalyseurSyntaxique::AnalyserMembreClasse(
        Programme& programme,
        Structure& structure,
        VisibiliteMembre visibilite)
    {
        const auto position = Position(Courant());
        const auto classe = structure.NomComplet();
        bool estVirtuelle = false;
        bool estRemplacement = false;
        while (Est(GenreJeton::Virtuel) || Est(GenreJeton::Remplacer))
        {
            if (Accepter(GenreJeton::Virtuel))
            {
                if (estVirtuelle)
                    throw ErreurCompilation(
                        "modificateur virtuel répété",
                        "duplicate virtual modifier",
                        position.Ligne, position.Colonne, position.Fichier);
                estVirtuelle = true;
            }
            else
            {
                (void)Accepter(GenreJeton::Remplacer);
                if (estRemplacement)
                    throw ErreurCompilation(
                        "modificateur remplacer répété",
                        "duplicate override modifier",
                        position.Ligne, position.Colonne, position.Fichier);
                estRemplacement = true;
                estVirtuelle = true;
            }
        }

        Fonction fonction;
        if (Accepter(GenreJeton::Constructeur))
        {
            if (estVirtuelle || estRemplacement)
                throw ErreurCompilation(
                    "un constructeur ne peut pas être virtuel ou remplacer une méthode",
                    "a constructor cannot be virtual or override a method",
                    position.Ligne, position.Colonne, position.Fichier);
            fonction = TerminerFonction(
                "$constructeur", TypeGs{GenreType::Vide},
                visibilite == VisibiliteMembre::Publique,
                false, position, classe, true);
            fonction.EstConstructeur = true;
        }
        else if (Accepter(GenreJeton::Destructeur))
        {
            fonction = TerminerFonction(
                "$destructeur", TypeGs{GenreType::Vide},
                visibilite == VisibiliteMembre::Publique,
                false, position, classe);
            fonction.EstDestructeur = true;
            fonction.EstVirtuelle = estVirtuelle;
            fonction.EstRemplacement = estRemplacement;
            if (!fonction.Parametres.empty())
                throw ErreurCompilation(
                    "un destructeur ne reçoit aucun paramètre explicite",
                    "a destructor takes no explicit parameter",
                    position.Ligne, position.Colonne, position.Fichier);
        }
        else
        {
            const auto typeRetour = AnalyserType();
            const auto nom = AnalyserNomFonction();
            fonction = TerminerFonction(
                nom, typeRetour,
                visibilite == VisibiliteMembre::Publique,
                false, position, classe);
            fonction.EstVirtuelle = estVirtuelle;
            fonction.EstRemplacement = estRemplacement;
            if (nom.starts_with("operator"))
            {
                fonction.EstOperateur = true;
                fonction.Operateur = nom.substr(8);
            }
        }

        fonction.NomSource = fonction.Nom;
        fonction.ClasseProprietaire = classe;
        fonction.Visibilite = visibilite;
        fonction.EstMethode = true;
        Parametre soi;
        soi.Nom = "soi";
        soi.Position = position;
        soi.Type = TypeGs{GenreType::Structure, classe};
        soi.Type.EstReference = true;
        fonction.Parametres.insert(fonction.Parametres.begin(), std::move(soi));
        programme.Fonctions.push_back(std::move(fonction));
    }

    Enumeration AnalyseurSyntaxique::AnalyserEnumeration(const std::string& espaceCourant)
    {
        Enumeration enumeration;
        enumeration.Espace = espaceCourant;
        enumeration.Position = Position(Courant());
        enumeration.Nom = Exiger(
            GenreJeton::Identifiant,
            "nom d’énumération attendu",
            "expected enum name").Texte;
        Exiger(GenreJeton::AccoladeOuvrante, "'{' attendue", "expected '{'");
        if (!Est(GenreJeton::AccoladeFermante))
        {
            do
            {
                Enumerateur valeur;
                valeur.Position = Position(Courant());
                valeur.Nom = Exiger(
                    GenreJeton::Identifiant,
                    "nom d’énumérateur attendu",
                    "expected enumerator name").Texte;
                if (Accepter(GenreJeton::Egal)) valeur.Initialiseur = AnalyserExpression();
                enumeration.Valeurs.push_back(std::move(valeur));
            }
            while (Accepter(GenreJeton::Virgule)
                   && !Est(GenreJeton::AccoladeFermante));
        }
        Exiger(GenreJeton::AccoladeFermante, "'}' attendue", "expected '}'");
        Exiger(GenreJeton::PointVirgule, "';' attendu après l’énumération", "expected ';' after enum");
        return enumeration;
    }

    DeclarationAlias AnalyseurSyntaxique::AnalyserAlias(const std::string& espaceCourant)
    {
        DeclarationAlias alias;
        alias.Espace = espaceCourant;
        alias.Position = Position(Courant());
        Exiger(GenreJeton::Alias, "'alias' attendu", "expected 'alias'");
        alias.Nom = AnalyserNomQualifie();
        Exiger(GenreJeton::Egal, "'=' attendu dans l’alias", "expected '=' in alias");
        alias.Cible = AnalyserNomQualifie();
        Exiger(GenreJeton::PointVirgule, "';' attendu après l’alias", "expected ';' after alias");
        return alias;
    }

    void AnalyseurSyntaxique::AnalyserFonctionOuGlobale(
        Programme& programme,
        const std::string& espaceCourant)
    {
        const auto position = Position(Courant());
        bool estPublique = false;
        bool estExterne = false;
        bool continuer = true;
        while (continuer)
        {
            if (Accepter(GenreJeton::Publique)) estPublique = true;
            else if (Accepter(GenreJeton::Privee)) estPublique = false;
            else if (Accepter(GenreJeton::Externe)) estExterne = true;
            else continuer = false;
        }

        auto type = AnalyserType();
        auto nom = AnalyserNomFonction();
        AnalyserDimensionsTableau(type);

        if (Est(GenreJeton::ParentheseOuvrante))
        {
            if (type.EstTableau())
                throw ErreurCompilation(
                    "une fonction ne peut pas retourner un tableau",
                    "a function cannot return an array",
                    position.Ligne, position.Colonne, position.Fichier);
            programme.Fonctions.push_back(TerminerFonction(
                std::move(nom),
                std::move(type),
                estPublique,
                estExterne,
                position,
                espaceCourant));
            return;
        }

        VariableGlobale variable;
        variable.Nom = std::move(nom);
        variable.Espace = espaceCourant;
        variable.Position = position;
        variable.Type = std::move(type);
        variable.EstPublique = _EstInterface ? false : estPublique;
        variable.EstExterne = estExterne || _EstInterface;
        if (Accepter(GenreJeton::Egal))
        {
            if (variable.EstExterne)
                throw ErreurCompilation(
                    "une variable globale externe ne peut pas avoir d’initialiseur",
                    "an external global variable cannot have an initializer",
                    position.Ligne, position.Colonne, position.Fichier);
            variable.Initialiseur = AnalyserExpression();
        }
        Exiger(GenreJeton::PointVirgule, "';' attendu", "expected ';'");
        programme.VariablesGlobales.push_back(std::move(variable));
    }

    Fonction AnalyseurSyntaxique::TerminerFonction(
        std::string nom,
        TypeGs typeRetour,
        bool estPublique,
        bool estExterne,
        PositionSource position,
        const std::string& espaceCourant,
        bool autoriserListeInitialisation)
    {
        Fonction fonction;
        fonction.Nom = std::move(nom);
        fonction.NomSource = fonction.Nom;
        fonction.Espace = espaceCourant;
        fonction.Position = std::move(position);
        fonction.EstPublique = _EstInterface ? false : estPublique;
        fonction.EstExterne = estExterne || _EstInterface;
        fonction.TypeRetour = std::move(typeRetour);

        Exiger(GenreJeton::ParentheseOuvrante, "'(' attendue", "expected '('");
        if (!Est(GenreJeton::ParentheseFermante))
        {
            do
            {
                Parametre parametre;
                parametre.Position = Position(Courant());
                parametre.Type = AnalyserType();
                parametre.Nom = Exiger(
                    GenreJeton::Identifiant,
                    "nom de paramètre attendu",
                    "expected parameter name").Texte;
                AnalyserDimensionsTableau(parametre.Type);
                fonction.Parametres.push_back(std::move(parametre));
            }
            while (Accepter(GenreJeton::Virgule));
        }
        Exiger(GenreJeton::ParentheseFermante, "')' attendue", "expected ')'");
        if (Accepter(GenreJeton::DeuxPoints))
        {
            if (!autoriserListeInitialisation)
                throw ErreurCompilation(
                    "une liste d’initialisation est réservée aux constructeurs",
                    "an initializer list is only allowed on constructors",
                    fonction.Position.Ligne,
                    fonction.Position.Colonne,
                    fonction.Position.Fichier);

            bool premier = true;
            do
            {
                if (Accepter(GenreJeton::Soi))
                {
                    if (!premier)
                        throw ErreurCompilation(
                            "soi doit être l’unique initialiseur d’un constructeur délégué",
                            "this must be the only initializer of a delegating constructor",
                            Position(Precedent()).Ligne,
                            Position(Precedent()).Colonne,
                            Position(Precedent()).Fichier);
                    Exiger(
                        GenreJeton::ParentheseOuvrante,
                        "'(' attendue après soi",
                        "expected '(' after this");
                    if (!Est(GenreJeton::ParentheseFermante))
                    {
                        do fonction.ArgumentsConstructeurDelegue.push_back(
                            AnalyserExpression());
                        while (Accepter(GenreJeton::Virgule));
                    }
                    Exiger(
                        GenreJeton::ParentheseFermante,
                        "')' attendue après l’initialiseur soi",
                        "expected ')' after this initializer");
                    fonction.DelegueConstructeur = true;
                    if (Est(GenreJeton::Virgule))
                        throw ErreurCompilation(
                            "un constructeur délégué ne peut pas initialiser directement parent ou un champ",
                            "a delegating constructor cannot directly initialize super or a field",
                            Position(Courant()).Ligne,
                            Position(Courant()).Colonne,
                            Position(Courant()).Fichier);
                }
                else if (Accepter(GenreJeton::Parent))
                {
                    if (!premier)
                        throw ErreurCompilation(
                            "parent doit être le premier initialiseur",
                            "super must be the first initializer",
                            Position(Precedent()).Ligne,
                            Position(Precedent()).Colonne,
                            Position(Precedent()).Fichier);
                    if (fonction.InitialiseurBaseExplicite)
                        throw ErreurCompilation(
                            "initialiseur parent déclaré plusieurs fois",
                            "super initializer declared more than once",
                            Position(Precedent()).Ligne,
                            Position(Precedent()).Colonne,
                            Position(Precedent()).Fichier);
                    Exiger(
                        GenreJeton::ParentheseOuvrante,
                        "'(' attendue après parent",
                        "expected '(' after super");
                    if (!Est(GenreJeton::ParentheseFermante))
                    {
                        do fonction.ArgumentsConstructeurBase.push_back(
                            AnalyserExpression());
                        while (Accepter(GenreJeton::Virgule));
                    }
                    Exiger(
                        GenreJeton::ParentheseFermante,
                        "')' attendue après l’initialiseur parent",
                        "expected ')' after super initializer");
                    fonction.InitialiseurBaseExplicite = true;
                }
                else
                {
                    InitialiseurChampConstructeur initialiseur;
                    initialiseur.Position = Position(Courant());
                    initialiseur.Nom = Exiger(
                        GenreJeton::Identifiant,
                        "nom de champ attendu dans la liste d’initialisation",
                        "expected field name in initializer list").Texte;
                    Exiger(
                        GenreJeton::ParentheseOuvrante,
                        "'(' attendue après le nom du champ",
                        "expected '(' after field name");
                    if (!Est(GenreJeton::ParentheseFermante))
                    {
                        do initialiseur.Arguments.push_back(
                            AnalyserExpression());
                        while (Accepter(GenreJeton::Virgule));
                    }
                    Exiger(
                        GenreJeton::ParentheseFermante,
                        "')' attendue après l’initialiseur de champ",
                        "expected ')' after field initializer");
                    fonction.InitialiseursChamps.push_back(
                        std::move(initialiseur));
                }
                premier = false;
            }
            while (Accepter(GenreJeton::Virgule));
        }
        if (fonction.EstExterne)
        {
            if (fonction.DelegueConstructeur
                || fonction.InitialiseurBaseExplicite
                || !fonction.InitialiseursChamps.empty())
                throw ErreurCompilation(
                    "une déclaration externe ne porte pas de liste d’initialisation",
                    "an external declaration cannot have an initializer list",
                    fonction.Position.Ligne,
                    fonction.Position.Colonne,
                    fonction.Position.Fichier);
            Exiger(GenreJeton::PointVirgule, "';' attendu après la déclaration externe", "expected ';' after extern declaration");
        }
        else
            fonction.Corps = AnalyserBloc();
        return fonction;
    }

    std::unique_ptr<InstructionBloc> AnalyseurSyntaxique::AnalyserBloc()
    {
        const auto position = Position(Courant());
        Exiger(GenreJeton::AccoladeOuvrante, "'{' attendue", "expected '{'");
        auto bloc = std::make_unique<InstructionBloc>(position);
        while (!Est(GenreJeton::AccoladeFermante) && !Est(GenreJeton::Fin))
            bloc->Instructions.push_back(AnalyserInstruction());
        Exiger(GenreJeton::AccoladeFermante, "'}' attendue", "expected '}'");
        return bloc;
    }

    bool AnalyseurSyntaxique::DebuteType() const
    {
        switch (Courant().Genre)
        {
            case GenreJeton::Constante:
            case GenreJeton::Volatile:
            case GenreJeton::Entier8:
            case GenreJeton::Entier16:
            case GenreJeton::Entier32:
            case GenreJeton::Entier64:
            case GenreJeton::Naturel8:
            case GenreJeton::Naturel16:
            case GenreJeton::Naturel32:
            case GenreJeton::Naturel64:
            case GenreJeton::Booleen:
            case GenreJeton::Octet:
            case GenreJeton::Caractere:
            case GenreJeton::Vide:
            case GenreJeton::PointeurFonction:
                return true;
            default:
                return false;
        }
    }

    bool AnalyseurSyntaxique::DebuteDeclarationVariable() const
    {
        if (DebuteType()) return true;
        if (!Est(GenreJeton::Identifiant)) return false;
        std::size_t position = _Position + 1;
        while (position + 1 < _Jetons.size()
               && _Jetons[position].Genre == GenreJeton::DeuxPointsDouble
               && _Jetons[position + 1].Genre == GenreJeton::Identifiant)
            position += 2;
        while (position < _Jetons.size() && _Jetons[position].Genre == GenreJeton::Etoile) ++position;
        if (position < _Jetons.size()
            && _Jetons[position].Genre == GenreJeton::Esperluette) ++position;
        return position < _Jetons.size() && _Jetons[position].Genre == GenreJeton::Identifiant;
    }

    std::unique_ptr<Instruction> AnalyseurSyntaxique::AnalyserInstruction()
    {
        const auto position = Position(Courant());
        if (Est(GenreJeton::AccoladeOuvrante)) return AnalyserBloc();
        if (Accepter(GenreJeton::Retourner))
        {
            std::unique_ptr<Expression> valeur;
            if (!Est(GenreJeton::PointVirgule)) valeur = AnalyserExpression();
            Exiger(GenreJeton::PointVirgule, "';' attendu", "expected ';'");
            return std::make_unique<InstructionRetour>(std::move(valeur), position);
        }
        if (Accepter(GenreJeton::Si))
        {
            Exiger(GenreJeton::ParentheseOuvrante, "'(' attendue après 'si'", "expected '(' after 'if'");
            auto condition = AnalyserExpression();
            Exiger(GenreJeton::ParentheseFermante, "')' attendue", "expected ')'");
            auto alors = AnalyserInstruction();
            std::unique_ptr<Instruction> sinon;
            if (Accepter(GenreJeton::Sinon)) sinon = AnalyserInstruction();
            return std::make_unique<InstructionSi>(
                std::move(condition), std::move(alors), std::move(sinon), position);
        }
        if (Accepter(GenreJeton::TantQue))
        {
            Exiger(GenreJeton::ParentheseOuvrante, "'(' attendue après 'tantque'", "expected '(' after 'while'");
            auto condition = AnalyserExpression();
            Exiger(GenreJeton::ParentheseFermante, "')' attendue", "expected ')'");
            return std::make_unique<InstructionTantQue>(
                std::move(condition), AnalyserInstruction(), position);
        }
        if (DebuteDeclarationVariable())
        {
            auto type = AnalyserType();
            const auto nom = Exiger(
                GenreJeton::Identifiant,
                "nom de variable attendu",
                "expected variable name").Texte;
            AnalyserDimensionsTableau(type);
            std::unique_ptr<Expression> initialiseur;
            std::vector<std::unique_ptr<Expression>> argumentsConstruction;
            bool constructionExplicite = false;
            if (Accepter(GenreJeton::Egal)) initialiseur = AnalyserExpression();
            else if (Accepter(GenreJeton::ParentheseOuvrante))
            {
                constructionExplicite = true;
                if (!Est(GenreJeton::ParentheseFermante))
                {
                    do argumentsConstruction.push_back(AnalyserExpression());
                    while (Accepter(GenreJeton::Virgule));
                }
                Exiger(GenreJeton::ParentheseFermante, "')' attendue", "expected ')'");
            }
            Exiger(GenreJeton::PointVirgule, "';' attendu", "expected ';'");
            auto variable = std::make_unique<InstructionVariable>(
                std::move(type), nom, std::move(initialiseur), position);
            variable->ArgumentsConstruction = std::move(argumentsConstruction);
            variable->ConstructionExplicite = constructionExplicite;
            return variable;
        }
        auto expression = AnalyserExpression();
        Exiger(GenreJeton::PointVirgule, "';' attendu", "expected ';'");
        return std::make_unique<InstructionExpression>(std::move(expression), position);
    }

    std::unique_ptr<Expression> AnalyseurSyntaxique::AnalyserExpression() { return AnalyserAffectation(); }

    std::unique_ptr<Expression> AnalyseurSyntaxique::AnalyserAffectation()
    {
        auto expression = AnalyserOuLogique();
        if (Accepter(GenreJeton::Egal))
        {
            const auto position = expression->Position;
            return std::make_unique<ExpressionAffectation>(
                std::move(expression), AnalyserAffectation(), position);
        }
        return expression;
    }

    std::unique_ptr<Expression> AnalyseurSyntaxique::AnalyserOuLogique()
    {
        auto expression = AnalyserEtLogique();
        while (Accepter(GenreJeton::OuLogique))
        {
            const auto position = Position(Precedent());
            expression = std::make_unique<ExpressionBinaire>(
                "||", std::move(expression), AnalyserEtLogique(), position);
        }
        return expression;
    }

    std::unique_ptr<Expression> AnalyseurSyntaxique::AnalyserEtLogique()
    {
        auto expression = AnalyserOuBinaire();
        while (Accepter(GenreJeton::EtLogique))
        {
            const auto position = Position(Precedent());
            expression = std::make_unique<ExpressionBinaire>(
                "&&", std::move(expression), AnalyserOuBinaire(), position);
        }
        return expression;
    }

    std::unique_ptr<Expression> AnalyseurSyntaxique::AnalyserOuBinaire()
    {
        auto expression = AnalyserXorBinaire();
        while (Accepter(GenreJeton::BarreVerticale))
        {
            const auto position = Position(Precedent());
            expression = std::make_unique<ExpressionBinaire>(
                "|", std::move(expression), AnalyserXorBinaire(), position);
        }
        return expression;
    }

    std::unique_ptr<Expression> AnalyseurSyntaxique::AnalyserXorBinaire()
    {
        auto expression = AnalyserEtBinaire();
        while (Accepter(GenreJeton::Circonflexe))
        {
            const auto position = Position(Precedent());
            expression = std::make_unique<ExpressionBinaire>(
                "^", std::move(expression), AnalyserEtBinaire(), position);
        }
        return expression;
    }

    std::unique_ptr<Expression> AnalyseurSyntaxique::AnalyserEtBinaire()
    {
        auto expression = AnalyserEgalite();
        while (Accepter(GenreJeton::Esperluette))
        {
            const auto position = Position(Precedent());
            expression = std::make_unique<ExpressionBinaire>(
                "&", std::move(expression), AnalyserEgalite(), position);
        }
        return expression;
    }

    std::unique_ptr<Expression> AnalyseurSyntaxique::AnalyserEgalite()
    {
        auto expression = AnalyserComparaison();
        while (AccepterUn({GenreJeton::EgalEgal, GenreJeton::Different}))
        {
            const auto position = Position(Precedent());
            const std::string operateur = Precedent().Genre == GenreJeton::EgalEgal ? "==" : "!=";
            expression = std::make_unique<ExpressionBinaire>(
                operateur, std::move(expression), AnalyserComparaison(), position);
        }
        return expression;
    }

    std::unique_ptr<Expression> AnalyseurSyntaxique::AnalyserComparaison()
    {
        auto expression = AnalyserDecalage();
        while (AccepterUn({GenreJeton::Inferieur, GenreJeton::InferieurEgal, GenreJeton::Superieur, GenreJeton::SuperieurEgal}))
        {
            const auto position = Position(Precedent());
            std::string operateur;
            switch (Precedent().Genre)
            {
                case GenreJeton::Inferieur: operateur = "<"; break;
                case GenreJeton::InferieurEgal: operateur = "<="; break;
                case GenreJeton::Superieur: operateur = ">"; break;
                default: operateur = ">="; break;
            }
            expression = std::make_unique<ExpressionBinaire>(
                operateur, std::move(expression), AnalyserDecalage(), position);
        }
        return expression;
    }

    std::unique_ptr<Expression> AnalyseurSyntaxique::AnalyserDecalage()
    {
        auto expression = AnalyserAddition();
        while (AccepterUn({GenreJeton::DecalageGauche, GenreJeton::DecalageDroite}))
        {
            const auto position = Position(Precedent());
            const std::string operateur =
                Precedent().Genre == GenreJeton::DecalageGauche ? "<<" : ">>";
            expression = std::make_unique<ExpressionBinaire>(
                operateur, std::move(expression), AnalyserAddition(), position);
        }
        return expression;
    }

    std::unique_ptr<Expression> AnalyseurSyntaxique::AnalyserAddition()
    {
        auto expression = AnalyserMultiplication();
        while (AccepterUn({GenreJeton::Plus, GenreJeton::Moins}))
        {
            const auto position = Position(Precedent());
            const std::string operateur = Precedent().Genre == GenreJeton::Plus ? "+" : "-";
            expression = std::make_unique<ExpressionBinaire>(
                operateur, std::move(expression), AnalyserMultiplication(), position);
        }
        return expression;
    }

    std::unique_ptr<Expression> AnalyseurSyntaxique::AnalyserMultiplication()
    {
        auto expression = AnalyserUnaire();
        while (AccepterUn({GenreJeton::Etoile, GenreJeton::BarreOblique, GenreJeton::Pourcentage}))
        {
            const auto position = Position(Precedent());
            std::string operateur = "*";
            if (Precedent().Genre == GenreJeton::BarreOblique) operateur = "/";
            if (Precedent().Genre == GenreJeton::Pourcentage) operateur = "%";
            expression = std::make_unique<ExpressionBinaire>(
                operateur, std::move(expression), AnalyserUnaire(), position);
        }
        return expression;
    }

    std::unique_ptr<Expression> AnalyseurSyntaxique::AnalyserUnaire()
    {
        if (AccepterUn({GenreJeton::Plus, GenreJeton::Moins, GenreJeton::PointExclamation,
                        GenreJeton::Tilde,
                        GenreJeton::Esperluette, GenreJeton::Etoile}))
        {
            const auto position = Position(Precedent());
            const auto operateur = Precedent().Texte;
            return std::make_unique<ExpressionUnaire>(
                operateur, AnalyserUnaire(), position);
        }
        return AnalyserPostfixe();
    }

    std::unique_ptr<Expression> AnalyseurSyntaxique::AnalyserPostfixe()
    {
        auto expression = AnalyserPrimaire();
        while (true)
        {
            if (Accepter(GenreJeton::ParentheseOuvrante))
            {
                const auto position = expression->Position;
                std::vector<std::unique_ptr<Expression>> arguments;
                if (!Est(GenreJeton::ParentheseFermante))
                {
                    do arguments.push_back(AnalyserExpression());
                    while (Accepter(GenreJeton::Virgule));
                }
                Exiger(GenreJeton::ParentheseFermante, "')' attendue", "expected ')'");
                expression = std::make_unique<ExpressionAppel>(
                    std::move(expression), std::move(arguments), position);
            }
            else if (Accepter(GenreJeton::CrochetOuvrant))
            {
                const auto position = Position(Precedent());
                auto indice = AnalyserExpression();
                Exiger(GenreJeton::CrochetFermant, "']' attendu", "expected ']'");
                expression = std::make_unique<ExpressionIndex>(
                    std::move(expression), std::move(indice), position);
            }
            else if (AccepterUn({GenreJeton::Point, GenreJeton::Fleche}))
            {
                const bool viaPointeur = Precedent().Genre == GenreJeton::Fleche;
                const auto position = Position(Precedent());
                auto membre = Exiger(
                    GenreJeton::Identifiant,
                    "nom de membre attendu",
                    "expected member name").Texte;
                expression = std::make_unique<ExpressionMembre>(
                    std::move(expression), std::move(membre), viaPointeur, position);
            }
            else break;
        }
        return expression;
    }

    std::unique_ptr<Expression> AnalyseurSyntaxique::AnalyserPrimaire()
    {
        const auto position = Position(Courant());
        if (Accepter(GenreJeton::AccoladeOuvrante))
        {
            std::vector<std::unique_ptr<Expression>> elements;
            if (!Est(GenreJeton::AccoladeFermante))
            {
                do
                {
                    elements.push_back(AnalyserExpression());
                }
                while (Accepter(GenreJeton::Virgule)
                       && !Est(GenreJeton::AccoladeFermante));
            }
            Exiger(
                GenreJeton::AccoladeFermante,
                "'}' attendue après l’initialiseur agrégé",
                "expected '}' after aggregate initializer");
            return std::make_unique<ExpressionAgregat>(
                std::move(elements), position);
        }
        if (Accepter(GenreJeton::NombreEntier))
        {
            std::string texte = Precedent().Texte;
            texte.erase(std::remove(texte.begin(), texte.end(), '_'), texte.end());
            std::uint64_t valeur = 0;
            const auto resultat = std::from_chars(texte.data(), texte.data() + texte.size(), valeur);
            if (resultat.ec != std::errc{} || resultat.ptr != texte.data() + texte.size())
                throw ErreurCompilation(
                    "nombre entier invalide ou trop grand",
                    "invalid or oversized integer",
                    position.Ligne, position.Colonne, position.Fichier);
            return std::make_unique<ExpressionEntier>(valeur, position);
        }
        if (Accepter(GenreJeton::ChaineCaracteres))
            return std::make_unique<ExpressionChaine>(
                Precedent().Texte, position);
        if (Accepter(GenreJeton::Vrai)) return std::make_unique<ExpressionEntier>(1, position, true);
        if (Accepter(GenreJeton::Faux)) return std::make_unique<ExpressionEntier>(0, position, true);
        if (Accepter(GenreJeton::Convertir))
        {
            Exiger(GenreJeton::Inferieur, "'<' attendu après convertir", "expected '<' after cast");
            auto type = AnalyserType();
            ExigerFermetureType(
                "'>' attendu après le type", "expected '>' after cast type");
            Exiger(GenreJeton::ParentheseOuvrante, "'(' attendue", "expected '('");
            auto valeur = AnalyserExpression();
            Exiger(GenreJeton::ParentheseFermante, "')' attendue", "expected ')'");
            return std::make_unique<ExpressionConversion>(
                std::move(type), std::move(valeur), position);
        }
        if (Accepter(GenreJeton::Soi))
            return std::make_unique<ExpressionVariable>("soi", position);
        if (Accepter(GenreJeton::Parent))
        {
            auto expression = std::make_unique<ExpressionVariable>(
                "soi", position);
            expression->EstBase = true;
            return expression;
        }
        if (Est(GenreJeton::Identifiant))
            return std::make_unique<ExpressionVariable>(AnalyserNomQualifie(), position);
        if (Accepter(GenreJeton::ParentheseOuvrante))
        {
            auto expression = AnalyserExpression();
            Exiger(GenreJeton::ParentheseFermante, "')' attendue", "expected ')'");
            return expression;
        }
        throw ErreurCompilation(
            "expression attendue",
            "expected expression",
            Courant().Ligne, Courant().Colonne, _Fichier);
    }
}
