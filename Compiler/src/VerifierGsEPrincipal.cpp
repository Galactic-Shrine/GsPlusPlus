#include "GsPP/VerificateurGsE.hpp"

#include <filesystem>
#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Utilisation : gseverifier <Application.GsE>\n";
        return 1;
    }
    const auto rapport = GsPP::VerificateurGsE().Verifier(std::filesystem::path(argv[1]));
    for (const auto& information : rapport.Informations)
        std::cout << information << '\n';
    for (const auto& erreur : rapport.Erreurs)
        std::cerr << "GSEV1001 : " << erreur << '\n';
    std::cout << (rapport.Valide ? "GsE valide.\n" : "GsE invalide.\n");
    return rapport.Valide ? 0 : 2;
}
