#include "GsPP/Lexeur.hpp"
#include "GsPP/ErreurCompilation.hpp"

#include <cctype>
#include <cstdint>
#include <unordered_map>

namespace GsPP
{
    namespace
    {
        const std::unordered_map<std::string, GenreJeton> MotsCles = {
            {"espace", GenreJeton::Espace},
            {"namespace", GenreJeton::Espace},
            {"classe", GenreJeton::Classe},
            {"class", GenreJeton::Classe},
            {"structure", GenreJeton::Structure},
            {"struct", GenreJeton::Structure},
            {"union", GenreJeton::Union},
            {"énumération", GenreJeton::Enumeration},
            {"enumeration", GenreJeton::Enumeration},
            {"enum", GenreJeton::Enumeration},
            {"alias", GenreJeton::Alias},
            {"externe", GenreJeton::Externe},
            {"extern", GenreJeton::Externe},
            {"publique", GenreJeton::Publique},
            {"public", GenreJeton::Publique},
            {"privée", GenreJeton::Privee},
            {"private", GenreJeton::Privee},
            {"protégée", GenreJeton::Protegee},
            {"protegee", GenreJeton::Protegee},
            {"protected", GenreJeton::Protegee},
            {"virtuel", GenreJeton::Virtuel},
            {"virtual", GenreJeton::Virtuel},
            {"remplacer", GenreJeton::Remplacer},
            {"override", GenreJeton::Remplacer},
            {"parent", GenreJeton::Parent},
            {"super", GenreJeton::Parent},
            {"constructeur", GenreJeton::Constructeur},
            {"constructor", GenreJeton::Constructeur},
            {"destructeur", GenreJeton::Destructeur},
            {"destructor", GenreJeton::Destructeur},
            {"opérateur", GenreJeton::Operateur},
            {"operateur", GenreJeton::Operateur},
            {"operator", GenreJeton::Operateur},
            {"soi", GenreJeton::Soi},
            {"this", GenreJeton::Soi},
            {"constante", GenreJeton::Constante},
            {"const", GenreJeton::Constante},
            {"volatile", GenreJeton::Volatile},
            {"entier8", GenreJeton::Entier8},
            {"int8", GenreJeton::Entier8},
            {"entier16", GenreJeton::Entier16},
            {"int16", GenreJeton::Entier16},
            {"entier32", GenreJeton::Entier32},
            {"int32", GenreJeton::Entier32},
            {"entier64", GenreJeton::Entier64},
            {"int64", GenreJeton::Entier64},
            {"naturel8", GenreJeton::Naturel8},
            {"uint8", GenreJeton::Naturel8},
            {"naturel16", GenreJeton::Naturel16},
            {"uint16", GenreJeton::Naturel16},
            {"naturel32", GenreJeton::Naturel32},
            {"uint32", GenreJeton::Naturel32},
            {"naturel64", GenreJeton::Naturel64},
            {"uint64", GenreJeton::Naturel64},
            {"booléen", GenreJeton::Booleen},
            {"booleen", GenreJeton::Booleen},
            {"bool", GenreJeton::Booleen},
            {"octet", GenreJeton::Octet},
            {"byte", GenreJeton::Octet},
            {"caractère", GenreJeton::Caractere},
            {"caractere", GenreJeton::Caractere},
            {"char", GenreJeton::Caractere},
            {"vide", GenreJeton::Vide},
            {"void", GenreJeton::Vide},
            {"pointeur_fonction", GenreJeton::PointeurFonction},
            {"function_pointer", GenreJeton::PointeurFonction},
            {"convertir", GenreJeton::Convertir},
            {"cast", GenreJeton::Convertir},
            {"retourner", GenreJeton::Retourner},
            {"return", GenreJeton::Retourner},
            {"si", GenreJeton::Si},
            {"if", GenreJeton::Si},
            {"sinon", GenreJeton::Sinon},
            {"else", GenreJeton::Sinon},
            {"tantque", GenreJeton::TantQue},
            {"while", GenreJeton::TantQue},
            {"vrai", GenreJeton::Vrai},
            {"true", GenreJeton::Vrai},
            {"faux", GenreJeton::Faux},
            {"false", GenreJeton::Faux}
        };

        bool EstUtf8Valide(std::string_view texte)
        {
            std::size_t position = 0;
            while (position < texte.size())
            {
                const auto premier = static_cast<unsigned char>(texte[position]);
                std::size_t longueur = 0;
                std::uint32_t point = 0;

                if (premier <= 0x7F) { longueur = 1; point = premier; }
                else if ((premier & 0xE0) == 0xC0) { longueur = 2; point = premier & 0x1F; }
                else if ((premier & 0xF0) == 0xE0) { longueur = 3; point = premier & 0x0F; }
                else if ((premier & 0xF8) == 0xF0) { longueur = 4; point = premier & 0x07; }
                else return false;

                if (position + longueur > texte.size()) return false;
                for (std::size_t index = 1; index < longueur; ++index)
                {
                    const auto suite = static_cast<unsigned char>(texte[position + index]);
                    if ((suite & 0xC0) != 0x80) return false;
                    point = (point << 6) | (suite & 0x3F);
                }

                if ((longueur == 2 && point < 0x80)
                    || (longueur == 3 && point < 0x800)
                    || (longueur == 4 && point < 0x10000)
                    || point > 0x10FFFF
                    || (point >= 0xD800 && point <= 0xDFFF))
                    return false;

                position += longueur;
            }
            return true;
        }
    }

    GenreJeton ClassifierMotCle(std::string_view texte)
    {
        const auto trouve = MotsCles.find(std::string(texte));
        return trouve == MotsCles.end()
            ? GenreJeton::Identifiant
            : trouve->second;
    }

    const char* NomGenreJeton(GenreJeton genre) noexcept
    {
        switch (genre)
        {
            case GenreJeton::Fin: return "fin";
            case GenreJeton::Identifiant: return "identifiant";
            case GenreJeton::NombreEntier: return "nombre-entier";
            case GenreJeton::ChaineCaracteres: return "chaîne-de-caractères";
            case GenreJeton::Espace: return "espace/namespace";
            case GenreJeton::Classe: return "classe/class";
            case GenreJeton::Structure: return "structure/struct";
            case GenreJeton::Union: return "union";
            case GenreJeton::Enumeration: return "énumération/enum";
            case GenreJeton::Alias: return "alias";
            case GenreJeton::Externe: return "externe/extern";
            case GenreJeton::Publique: return "publique/public";
            case GenreJeton::Privee: return "privée/private";
            case GenreJeton::Protegee: return "protégée/protected";
            case GenreJeton::Virtuel: return "virtuel/virtual";
            case GenreJeton::Remplacer: return "remplacer/override";
            case GenreJeton::Parent: return "parent/super";
            case GenreJeton::Constructeur: return "constructeur/constructor";
            case GenreJeton::Destructeur: return "destructeur/destructor";
            case GenreJeton::Operateur: return "opérateur/operator";
            case GenreJeton::Soi: return "soi/this";
            case GenreJeton::Constante: return "constante/const";
            case GenreJeton::Volatile: return "volatile";
            case GenreJeton::Entier8: return "entier8/int8";
            case GenreJeton::Entier16: return "entier16/int16";
            case GenreJeton::Entier32: return "entier32/int32";
            case GenreJeton::Entier64: return "entier64/int64";
            case GenreJeton::Naturel8: return "naturel8/uint8";
            case GenreJeton::Naturel16: return "naturel16/uint16";
            case GenreJeton::Naturel32: return "naturel32/uint32";
            case GenreJeton::Naturel64: return "naturel64/uint64";
            case GenreJeton::Booleen: return "booléen/bool";
            case GenreJeton::Octet: return "octet/byte";
            case GenreJeton::Caractere: return "caractère/char";
            case GenreJeton::Vide: return "vide/void";
            case GenreJeton::PointeurFonction: return "pointeur_fonction/function_pointer";
            case GenreJeton::Convertir: return "convertir/cast";
            case GenreJeton::Retourner: return "retourner/return";
            case GenreJeton::Si: return "si/if";
            case GenreJeton::Sinon: return "sinon/else";
            case GenreJeton::TantQue: return "tantque/while";
            case GenreJeton::Vrai: return "vrai/true";
            case GenreJeton::Faux: return "faux/false";
            case GenreJeton::ParentheseOuvrante: return "(";
            case GenreJeton::ParentheseFermante: return ")";
            case GenreJeton::AccoladeOuvrante: return "{";
            case GenreJeton::AccoladeFermante: return "}";
            case GenreJeton::CrochetOuvrant: return "[";
            case GenreJeton::CrochetFermant: return "]";
            case GenreJeton::PointVirgule: return ";";
            case GenreJeton::Virgule: return ",";
            case GenreJeton::Point: return ".";
            case GenreJeton::Fleche: return "->";
            case GenreJeton::DeuxPointsDouble: return "::";
            case GenreJeton::DeuxPoints: return ":";
            case GenreJeton::Egal: return "=";
            case GenreJeton::EgalEgal: return "==";
            case GenreJeton::Different: return "!=";
            case GenreJeton::Inferieur: return "<";
            case GenreJeton::InferieurEgal: return "<=";
            case GenreJeton::Superieur: return ">";
            case GenreJeton::SuperieurEgal: return ">=";
            case GenreJeton::PointExclamation: return "!";
            case GenreJeton::EtLogique: return "&&";
            case GenreJeton::OuLogique: return "||";
            case GenreJeton::Esperluette: return "&";
            case GenreJeton::BarreVerticale: return "|";
            case GenreJeton::Circonflexe: return "^";
            case GenreJeton::Tilde: return "~";
            case GenreJeton::DecalageGauche: return "<<";
            case GenreJeton::DecalageDroite: return ">>";
            case GenreJeton::Plus: return "+";
            case GenreJeton::Moins: return "-";
            case GenreJeton::Etoile: return "*";
            case GenreJeton::BarreOblique: return "/";
            case GenreJeton::Pourcentage: return "%";
        }
        return "inconnu";
    }

    Lexeur::Lexeur(std::string_view source, std::string fichier)
        : _Source(source), _Fichier(std::move(fichier))
    {
        if (!EstUtf8Valide(source))
            throw ErreurCompilation(
                "le fichier source n’est pas un texte UTF-8 valide",
                "the source file is not valid UTF-8 text",
                1,
                1,
                _Fichier);

        if (source.size() >= 3
            && static_cast<unsigned char>(source[0]) == 0xEF
            && static_cast<unsigned char>(source[1]) == 0xBB
            && static_cast<unsigned char>(source[2]) == 0xBF)
            _Position = 3;
    }

    bool Lexeur::EstFin() const noexcept
    {
        return _Position >= _Source.size();
    }

    char Lexeur::Courant() const noexcept
    {
        return EstFin() ? '\0' : _Source[_Position];
    }

    char Lexeur::Suivant() const noexcept
    {
        return _Position + 1 >= _Source.size() ? '\0' : _Source[_Position + 1];
    }

    char Lexeur::Avancer()
    {
        const char valeur = Courant();
        if (!EstFin())
        {
            ++_Position;
            if (valeur == '\n')
            {
                ++_Ligne;
                _Colonne = 1;
            }
            else
            {
                ++_Colonne;
            }
        }
        return valeur;
    }

    bool Lexeur::CommenceIdentifiant(unsigned char octet) const noexcept
    {
        return octet == '_' || octet >= 0x80 || std::isalpha(octet) != 0;
    }

    bool Lexeur::ContinueIdentifiant(unsigned char octet) const noexcept
    {
        return CommenceIdentifiant(octet) || std::isdigit(octet) != 0;
    }

    void Lexeur::IgnorerSeparations()
    {
        while (!EstFin())
        {
            if (std::isspace(static_cast<unsigned char>(Courant())) != 0)
            {
                Avancer();
                continue;
            }

            if (Courant() == '/' && Suivant() == '/')
            {
                while (!EstFin() && Courant() != '\n') Avancer();
                continue;
            }

            if (Courant() == '/' && Suivant() == '*')
            {
                const auto ligne = _Ligne;
                const auto colonne = _Colonne;
                Avancer();
                Avancer();
                while (!EstFin() && !(Courant() == '*' && Suivant() == '/')) Avancer();
                if (EstFin())
                {
                    throw ErreurCompilation(
                        "commentaire de bloc non terminé",
                        "unterminated block comment",
                        ligne,
                        colonne,
                        _Fichier);
                }
                Avancer();
                Avancer();
                continue;
            }

            break;
        }
    }

    Jeton Lexeur::LireNombre()
    {
        const auto debut = _Position;
        const auto ligne = _Ligne;
        const auto colonne = _Colonne;
        while (!EstFin() && (std::isdigit(static_cast<unsigned char>(Courant())) != 0 || Courant() == '_'))
        {
            Avancer();
        }
        return {GenreJeton::NombreEntier, std::string(_Source.substr(debut, _Position - debut)), ligne, colonne};
    }

    Jeton Lexeur::LireIdentifiant()
    {
        const auto debut = _Position;
        const auto ligne = _Ligne;
        const auto colonne = _Colonne;
        while (!EstFin() && ContinueIdentifiant(static_cast<unsigned char>(Courant()))) Avancer();

        std::string texte(_Source.substr(debut, _Position - debut));
        return {
            ClassifierMotCle(texte),
            std::move(texte),
            ligne,
            colonne
        };
    }

    Jeton Lexeur::LireChaine()
    {
        const auto ligne = _Ligne;
        const auto colonne = _Colonne;
        Avancer();
        std::string texte;
        while (!EstFin())
        {
            const char valeur = Avancer();
            if (valeur == '"')
                return {
                    GenreJeton::ChaineCaracteres,
                    std::move(texte),
                    ligne,
                    colonne
                };
            if (valeur == '\n' || valeur == '\r')
                throw ErreurCompilation(
                    "chaîne de caractères non terminée",
                    "unterminated string literal",
                    ligne,
                    colonne,
                    _Fichier);
            if (valeur != '\\')
            {
                texte.push_back(valeur);
                continue;
            }
            if (EstFin())
                throw ErreurCompilation(
                    "séquence d’échappement non terminée",
                    "unterminated escape sequence",
                    ligne,
                    colonne,
                    _Fichier);
            const char echappement = Avancer();
            switch (echappement)
            {
                case '\\': texte.push_back('\\'); break;
                case '"': texte.push_back('"'); break;
                case 'n': texte.push_back('\n'); break;
                case 'r': texte.push_back('\r'); break;
                case 't': texte.push_back('\t'); break;
                case '0': texte.push_back('\0'); break;
                default:
                    throw ErreurCompilation(
                        std::string("séquence d’échappement inconnue : \\")
                            + echappement,
                        std::string("unknown escape sequence: \\")
                            + echappement,
                        _Ligne,
                        _Colonne - 1,
                        _Fichier);
            }
        }
        throw ErreurCompilation(
            "chaîne de caractères non terminée",
            "unterminated string literal",
            ligne,
            colonne,
            _Fichier);
    }

    Jeton Lexeur::LireSymbole()
    {
        const auto ligne = _Ligne;
        const auto colonne = _Colonne;
        const char valeur = Avancer();

        switch (valeur)
        {
            case '(': return {GenreJeton::ParentheseOuvrante, "(", ligne, colonne};
            case ')': return {GenreJeton::ParentheseFermante, ")", ligne, colonne};
            case '{': return {GenreJeton::AccoladeOuvrante, "{", ligne, colonne};
            case '}': return {GenreJeton::AccoladeFermante, "}", ligne, colonne};
            case '[': return {GenreJeton::CrochetOuvrant, "[", ligne, colonne};
            case ']': return {GenreJeton::CrochetFermant, "]", ligne, colonne};
            case ';': return {GenreJeton::PointVirgule, ";", ligne, colonne};
            case ',': return {GenreJeton::Virgule, ",", ligne, colonne};
            case '.': return {GenreJeton::Point, ".", ligne, colonne};
            case '+': return {GenreJeton::Plus, "+", ligne, colonne};
            case '-':
                if (Courant() == '>')
                {
                    Avancer();
                    return {GenreJeton::Fleche, "->", ligne, colonne};
                }
                return {GenreJeton::Moins, "-", ligne, colonne};
            case '*': return {GenreJeton::Etoile, "*", ligne, colonne};
            case '/': return {GenreJeton::BarreOblique, "/", ligne, colonne};
            case '%': return {GenreJeton::Pourcentage, "%", ligne, colonne};
            case '&':
                if (Courant() == '&')
                {
                    Avancer();
                    return {GenreJeton::EtLogique, "&&", ligne, colonne};
                }
                return {GenreJeton::Esperluette, "&", ligne, colonne};
            case '|':
                if (Courant() == '|')
                {
                    Avancer();
                    return {GenreJeton::OuLogique, "||", ligne, colonne};
                }
                return {GenreJeton::BarreVerticale, "|", ligne, colonne};
            case '^': return {GenreJeton::Circonflexe, "^", ligne, colonne};
            case '~': return {GenreJeton::Tilde, "~", ligne, colonne};
            case '=':
                if (Courant() == '=')
                {
                    Avancer();
                    return {GenreJeton::EgalEgal, "==", ligne, colonne};
                }
                return {GenreJeton::Egal, "=", ligne, colonne};
            case '!':
                if (Courant() == '=')
                {
                    Avancer();
                    return {GenreJeton::Different, "!=", ligne, colonne};
                }
                return {GenreJeton::PointExclamation, "!", ligne, colonne};
            case '<':
                if (Courant() == '<')
                {
                    Avancer();
                    return {GenreJeton::DecalageGauche, "<<", ligne, colonne};
                }
                if (Courant() == '=')
                {
                    Avancer();
                    return {GenreJeton::InferieurEgal, "<=", ligne, colonne};
                }
                return {GenreJeton::Inferieur, "<", ligne, colonne};
            case '>':
                if (Courant() == '>')
                {
                    Avancer();
                    return {GenreJeton::DecalageDroite, ">>", ligne, colonne};
                }
                if (Courant() == '=')
                {
                    Avancer();
                    return {GenreJeton::SuperieurEgal, ">=", ligne, colonne};
                }
                return {GenreJeton::Superieur, ">", ligne, colonne};
            case ':':
                if (Courant() == ':')
                {
                    Avancer();
                    return {GenreJeton::DeuxPointsDouble, "::", ligne, colonne};
                }
                return {GenreJeton::DeuxPoints, ":", ligne, colonne};
        }

        throw ErreurCompilation(
            std::string("caractère inattendu : '") + valeur + "'",
            std::string("unexpected character: '") + valeur + "'",
            ligne,
            colonne,
            _Fichier);
    }

    std::vector<Jeton> Lexeur::Analyser()
    {
        std::vector<Jeton> jetons;
        while (true)
        {
            IgnorerSeparations();
            if (EstFin())
            {
                jetons.push_back({GenreJeton::Fin, "", _Ligne, _Colonne});
                return jetons;
            }

            const auto octet = static_cast<unsigned char>(Courant());
            if (std::isdigit(octet) != 0)
                jetons.push_back(LireNombre());
            else if (CommenceIdentifiant(octet))
                jetons.push_back(LireIdentifiant());
            else if (Courant() == '"')
                jetons.push_back(LireChaine());
            else
                jetons.push_back(LireSymbole());
        }
    }
}
