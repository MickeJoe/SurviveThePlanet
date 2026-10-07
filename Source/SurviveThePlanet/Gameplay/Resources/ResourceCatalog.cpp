#include "Gameplay/Resources/ResourceCatalog.h"
#include "Engine/Texture2D.h"

namespace
{
    FResourceDefinition Make(EResourceType Type, EResourceCategory Category, const TCHAR* Prefix,
        const TCHAR* Name, const TCHAR* ShortName, const TCHAR* IconPath, bool bImported)
    {
        FResourceDefinition Definition;
        Definition.ResourceType = Type;
        Definition.Category = Category;
        Definition.WidgetPrefix = FName(Prefix);
        Definition.DisplayName = FText::FromString(Name);
        Definition.ShortName = FText::FromString(ShortName);
        Definition.Icon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(IconPath));
        Definition.bImported = bImported;
        return Definition;
    }
}

const TArray<FResourceDefinition>& UResourceCatalog::GetDefinitions()
{
    static const TArray<FResourceDefinition> Definitions = {
        Make(EResourceType::Energy, EResourceCategory::Energy, TEXT("Energy"), TEXT("Energy"), TEXT("Energy"), TEXT("/Game/UI/EnergyResourceIcon.EnergyResourceIcon"), false),
        Make(EResourceType::Water, EResourceCategory::RawMaterials, TEXT("Water"), TEXT("Water"), TEXT("Water"), TEXT("/Game/UI/Icons/Resources/T_Water.T_Water"), false),
        Make(EResourceType::Stone, EResourceCategory::RawMaterials, TEXT("Stone"), TEXT("Stone"), TEXT("Stone"), TEXT("/Game/UI/StoneResourceIcon.StoneResourceIcon"), false),
        Make(EResourceType::Copper, EResourceCategory::RawMaterials, TEXT("Copper"), TEXT("Copper Ore"), TEXT("Copper Ore"), TEXT("/Game/UI/CopperResourceIcon.CopperResourceIcon"), false),
        Make(EResourceType::Iron, EResourceCategory::RawMaterials, TEXT("Iron"), TEXT("Iron Ore"), TEXT("Iron Ore"), TEXT("/Game/UI/IronResourceIcon.IronResourceIcon"), false),
        Make(EResourceType::Silica, EResourceCategory::RawMaterials, TEXT("Silica"), TEXT("Silica"), TEXT("Silica"), TEXT("/Game/UI/Icons/Resources/T_Silica.T_Silica"), false),
        Make(EResourceType::Coal, EResourceCategory::RawMaterials, TEXT("Coal"), TEXT("Carbon"), TEXT("Carbon"), TEXT("/Game/UI/Icons/T_Coal.T_Coal"), false),
        Make(EResourceType::RareMinerals, EResourceCategory::RawMaterials, TEXT("RareMinerals"), TEXT("Rare Minerals"), TEXT("Rare Minerals"), TEXT("/Game/UI/Icons/Resources/T_RareMinerals.T_RareMinerals"), false),
        Make(EResourceType::UraniumOre, EResourceCategory::RawMaterials, TEXT("UraniumOre"), TEXT("Uranium Ore"), TEXT("Uranium Ore"), TEXT("/Game/UI/Icons/Resources/T_UraniumOre.T_UraniumOre"), false),
        Make(EResourceType::AlienMaterial, EResourceCategory::RawMaterials, TEXT("AlienMaterial"), TEXT("Alien Material"), TEXT("Alien Material"), TEXT("/Game/UI/Icons/Resources/T_AlienMaterial.T_AlienMaterial"), false),
        Make(EResourceType::Concrete, EResourceCategory::Materials, TEXT("Concrete"), TEXT("Concrete"), TEXT("Concrete"), TEXT("/Game/UI/ConcreteResourceIcon.ConcreteResourceIcon"), false),
        Make(EResourceType::CopperMetal, EResourceCategory::Materials, TEXT("CopperMetal"), TEXT("Copper"), TEXT("Copper"), TEXT("/Game/UI/Icons/Resources/T_CopperMetal.T_CopperMetal"), false),
        Make(EResourceType::Steel, EResourceCategory::Materials, TEXT("Steel"), TEXT("Steel"), TEXT("Steel"), TEXT("/Game/UI/Icons/Resources/T_Steel.T_Steel"), false),
        Make(EResourceType::Glass, EResourceCategory::Materials, TEXT("Glass"), TEXT("Glass"), TEXT("Glass"), TEXT("/Game/UI/Icons/Resources/T_Glass.T_Glass"), false),
        Make(EResourceType::Polymer, EResourceCategory::Materials, TEXT("Polymer"), TEXT("Polymer"), TEXT("Polymer"), TEXT("/Game/UI/Icons/Resources/T_Polymer.T_Polymer"), false),
        Make(EResourceType::PurifiedWater, EResourceCategory::Materials, TEXT("PurifiedWater"), TEXT("Purified Water"), TEXT("Purified Water"), TEXT("/Game/UI/Icons/Resources/T_PurifiedWater.T_PurifiedWater"), false),
        Make(EResourceType::Ceramics, EResourceCategory::Materials, TEXT("Ceramics"), TEXT("Ceramics"), TEXT("Ceramics"), TEXT("/Game/UI/Icons/Resources/T_Ceramics.T_Ceramics"), false),
        Make(EResourceType::RareMetals, EResourceCategory::Materials, TEXT("RareMetals"), TEXT("Rare Metals"), TEXT("Rare Metals"), TEXT("/Game/UI/Icons/Resources/T_RareMetals.T_RareMetals"), false),
        Make(EResourceType::SyntheticFuel, EResourceCategory::Materials, TEXT("SyntheticFuel"), TEXT("Synthetic Fuel"), TEXT("Synth. Fuel"), TEXT("/Game/UI/Icons/Resources/T_SyntheticFuel.T_SyntheticFuel"), false),
        Make(EResourceType::FuelRods, EResourceCategory::Materials, TEXT("FuelRods"), TEXT("Fuel Rods"), TEXT("Fuel Rods"), TEXT("/Game/UI/Icons/Resources/T_FuelRods.T_FuelRods"), false),
        Make(EResourceType::XenoCompound, EResourceCategory::Materials, TEXT("XenoCompound"), TEXT("Xeno Compound"), TEXT("Xeno Compound"), TEXT("/Game/UI/Icons/Resources/T_XenoCompound.T_XenoCompound"), false),
        Make(EResourceType::Connector, EResourceCategory::Components, TEXT("Connector"), TEXT("Connectors"), TEXT("Connectors"), TEXT("/Game/UI/Icons/Resources/T_Connector.T_Connector"), false),
        Make(EResourceType::MachineParts, EResourceCategory::Components, TEXT("MachineParts"), TEXT("Machine Parts"), TEXT("Machine Parts"), TEXT("/Game/UI/Icons/Resources/T_MachineParts.T_MachineParts"), false),
        Make(EResourceType::StructuralComponents, EResourceCategory::Components, TEXT("StructuralComponents"), TEXT("Structural Components"), TEXT("Structural Parts"), TEXT("/Game/UI/Icons/Resources/T_StructuralComponents.T_StructuralComponents"), false),
        Make(EResourceType::CompositePanels, EResourceCategory::Components, TEXT("CompositePanels"), TEXT("Composite Panels"), TEXT("Composite Panels"), TEXT("/Game/UI/Icons/Resources/T_CompositePanels.T_CompositePanels"), false),
        Make(EResourceType::BatteryCells, EResourceCategory::Components, TEXT("BatteryCells"), TEXT("Battery Cells"), TEXT("Battery Cells"), TEXT("/Game/UI/Icons/Resources/T_BatteryCells.T_BatteryCells"), false),
        Make(EResourceType::OpticalUnits, EResourceCategory::Components, TEXT("OpticalUnits"), TEXT("Optical Units"), TEXT("Optical Units"), TEXT("/Game/UI/Icons/Resources/T_OpticalUnits.T_OpticalUnits"), false),
        Make(EResourceType::CoolingUnits, EResourceCategory::Components, TEXT("CoolingUnits"), TEXT("Cooling Units"), TEXT("Cooling Units"), TEXT("/Game/UI/Icons/Resources/T_CoolingUnits.T_CoolingUnits"), false),
        Make(EResourceType::PressureUnits, EResourceCategory::Components, TEXT("PressureUnits"), TEXT("Pressure Units"), TEXT("Pressure Units"), TEXT("/Game/UI/Icons/Resources/T_PressureUnits.T_PressureUnits"), false),
        Make(EResourceType::SensorUnits, EResourceCategory::Components, TEXT("SensorUnits"), TEXT("Sensor Units"), TEXT("Sensor Units"), TEXT("/Game/UI/Icons/Resources/T_SensorUnits.T_SensorUnits"), false),
        Make(EResourceType::PowerRegulators, EResourceCategory::Components, TEXT("PowerRegulators"), TEXT("Power Regulators"), TEXT("Power Regulators"), TEXT("/Game/UI/Icons/Resources/T_PowerRegulators.T_PowerRegulators"), false),
        Make(EResourceType::FuelCells, EResourceCategory::Components, TEXT("FuelCells"), TEXT("Fuel Cells"), TEXT("Fuel Cells"), TEXT("/Game/UI/Icons/Resources/T_FuelCells.T_FuelCells"), false),
        Make(EResourceType::IndustrialTools, EResourceCategory::Components, TEXT("IndustrialTools"), TEXT("Industrial Tools"), TEXT("Industrial Tools"), TEXT("/Game/UI/Icons/Resources/T_IndustrialTools.T_IndustrialTools"), false),
        Make(EResourceType::DroneSpareParts, EResourceCategory::Components, TEXT("DroneSpareParts"), TEXT("Drone Spare Parts"), TEXT("Drone Spares"), TEXT("/Game/UI/Icons/Resources/T_DroneSpareParts.T_DroneSpareParts"), false),
        Make(EResourceType::AdvancedAlloy, EResourceCategory::Components, TEXT("AdvancedAlloy"), TEXT("Advanced Alloy"), TEXT("Advanced Alloy"), TEXT("/Game/UI/Icons/Resources/T_AdvancedAlloy.T_AdvancedAlloy"), false),
        Make(EResourceType::ControlChip, EResourceCategory::Components, TEXT("ControlChip"), TEXT("Circuit Boards"), TEXT("Circuit Boards"), TEXT("/Game/UI/ControlChipResourceIcon.ControlChipResourceIcon"), true),
        Make(EResourceType::ControlUnits, EResourceCategory::AdvancedGoods, TEXT("ControlUnits"), TEXT("Control Units"), TEXT("Control Units"), TEXT("/Game/UI/Icons/Resources/T_ControlUnits.T_ControlUnits"), false),
        Make(EResourceType::NavigationCores, EResourceCategory::AdvancedGoods, TEXT("NavigationCores"), TEXT("Navigation Cores"), TEXT("Navigation Cores"), TEXT("/Game/UI/Icons/Resources/T_NavigationCores.T_NavigationCores"), false),
        Make(EResourceType::LifeSupportModules, EResourceCategory::AdvancedGoods, TEXT("LifeSupportModules"), TEXT("Life Support Modules"), TEXT("Life Support"), TEXT("/Game/UI/Icons/Resources/T_LifeSupportModules.T_LifeSupportModules"), false),
        Make(EResourceType::ReactorAssemblies, EResourceCategory::AdvancedGoods, TEXT("ReactorAssemblies"), TEXT("Reactor Assemblies"), TEXT("Reactor Modules"), TEXT("/Game/UI/Icons/Resources/T_ReactorAssemblies.T_ReactorAssemblies"), false),
        Make(EResourceType::PowerPacks, EResourceCategory::AdvancedGoods, TEXT("PowerPacks"), TEXT("Power Packs"), TEXT("Power Packs"), TEXT("/Game/UI/Icons/Resources/T_PowerPacks.T_PowerPacks"), false),
        Make(EResourceType::SurveyPackages, EResourceCategory::AdvancedGoods, TEXT("SurveyPackages"), TEXT("Survey Packages"), TEXT("Survey Packages"), TEXT("/Game/UI/Icons/Resources/T_SurveyPackages.T_SurveyPackages"), false),
        Make(EResourceType::CommunicationArrays, EResourceCategory::AdvancedGoods, TEXT("CommunicationArrays"), TEXT("Communication Arrays"), TEXT("Comms Arrays"), TEXT("/Game/UI/Icons/Resources/T_CommunicationArrays.T_CommunicationArrays"), false),
        Make(EResourceType::PrecisionComponents, EResourceCategory::AdvancedGoods, TEXT("PrecisionComponents"), TEXT("Precision Components"), TEXT("Precision Parts"), TEXT("/Game/UI/Icons/Resources/T_PrecisionComponents.T_PrecisionComponents"), false),
        Make(EResourceType::HabitatModules, EResourceCategory::AdvancedGoods, TEXT("HabitatModules"), TEXT("Habitat Modules"), TEXT("Habitat Modules"), TEXT("/Game/UI/Icons/Resources/T_HabitatModules.T_HabitatModules"), false),
        Make(EResourceType::XenotechModules, EResourceCategory::AdvancedGoods, TEXT("XenotechModules"), TEXT("Xenotech Modules"), TEXT("Xenotech Modules"), TEXT("/Game/UI/Icons/Resources/T_XenotechModules.T_XenotechModules"), false),
    };
    return Definitions;
}

TArray<FResourceDefinition> UResourceCatalog::GetResourceDefinitions()
{
    return GetDefinitions();
}

UTexture2D* UResourceCatalog::GetResourceIcon(EResourceType ResourceType)
{
    for (const FResourceDefinition& Definition : GetDefinitions())
    {
        if (Definition.ResourceType == ResourceType)
        {
            return Definition.Icon.LoadSynchronous();
        }
    }
    return nullptr;
}
