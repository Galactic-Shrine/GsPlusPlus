#pragma once

#include <string_view>

namespace GsPP
{
    enum class GenreIntrinseque
    {
        Aucun,
        ChargerAtomique32,
        ChargerAtomique64,
        StockerAtomique32,
        StockerAtomique64,
        EchangerAtomique32,
        EchangerAtomique64,
        AjouterAtomique32,
        AjouterAtomique64,
        ComparerEchanger32,
        ComparerEchanger64,
        BarriereMemoire,
        PauseProcesseur
    };

    [[nodiscard]] inline GenreIntrinseque IdentifierIntrinseque(
        std::string_view nom) noexcept
    {
        if (nom == "GalacticShrine::GsPP::Intrinseques::ChargerAtomique32")
            return GenreIntrinseque::ChargerAtomique32;
        if (nom == "GalacticShrine::GsPP::Intrinseques::ChargerAtomique64")
            return GenreIntrinseque::ChargerAtomique64;
        if (nom == "GalacticShrine::GsPP::Intrinseques::StockerAtomique32")
            return GenreIntrinseque::StockerAtomique32;
        if (nom == "GalacticShrine::GsPP::Intrinseques::StockerAtomique64")
            return GenreIntrinseque::StockerAtomique64;
        if (nom == "GalacticShrine::GsPP::Intrinseques::EchangerAtomique32")
            return GenreIntrinseque::EchangerAtomique32;
        if (nom == "GalacticShrine::GsPP::Intrinseques::EchangerAtomique64")
            return GenreIntrinseque::EchangerAtomique64;
        if (nom == "GalacticShrine::GsPP::Intrinseques::AjouterAtomique32")
            return GenreIntrinseque::AjouterAtomique32;
        if (nom == "GalacticShrine::GsPP::Intrinseques::AjouterAtomique64")
            return GenreIntrinseque::AjouterAtomique64;
        if (nom == "GalacticShrine::GsPP::Intrinseques::ComparerEchanger32")
            return GenreIntrinseque::ComparerEchanger32;
        if (nom == "GalacticShrine::GsPP::Intrinseques::ComparerEchanger64")
            return GenreIntrinseque::ComparerEchanger64;
        if (nom == "GalacticShrine::GsPP::Intrinseques::BarriereMemoire")
            return GenreIntrinseque::BarriereMemoire;
        if (nom == "GalacticShrine::GsPP::Intrinseques::PauseProcesseur")
            return GenreIntrinseque::PauseProcesseur;
        return GenreIntrinseque::Aucun;
    }
}
