#pragma once

#include "CoreMinimal.h"
#include "BuildToolTypes.generated.h"

UENUM(BlueprintType)
enum class ESTPBuildTool : uint8
{
	None UMETA(DisplayName = "None"),
	// Reserved for serialized assets; manual cable construction has been removed.
	EnergyCable UMETA(Hidden),
	EnergyModule UMETA(DisplayName = "Energy Module"),
	EnergyStorage UMETA(DisplayName = "Energy Storage"),
	MiningMachine UMETA(DisplayName = "Mining Machine"),
	WaterCollector UMETA(DisplayName = "Water Collector"),
	ConcretePlant UMETA(DisplayName = "Concrete Plant"),
	CommunicationModule UMETA(DisplayName = "Communication Module"),
	CargoBay UMETA(DisplayName = "Cargo Bay"),
	CommandHub UMETA(DisplayName = "Command Hub"),
	SolarArray UMETA(DisplayName = "Solar Array"),
	WindGenerator UMETA(DisplayName = "Wind Generator"),
	GeothermalPlant UMETA(DisplayName = "Geothermal Plant"),
	NuclearReactor UMETA(DisplayName = "Nuclear Reactor"),
	MiningStation UMETA(DisplayName = "Mining Station"),
	ResourceStorage UMETA(DisplayName = "Resource Storage"),
	DroneFactory UMETA(DisplayName = "Drone Factory"),
	CommunicationsTower UMETA(DisplayName = "Communications Tower"),
	Steelworks UMETA(DisplayName = "Steelworks"),
	EnergyExtender UMETA(DisplayName = "Energy Extender"),
	PolymerPlant UMETA(DisplayName = "Polymer Plant"),
	ConnectorPlant UMETA(DisplayName = "Connector Plant"),
	RemoteBase UMETA(DisplayName = "Remote Base"),

	// Append catalog tools to preserve the values stored in existing assets.
	CrudeWorkshop UMETA(DisplayName = "Crude Workshop"),
	LogisticsHub UMETA(DisplayName = "Logistics Hub"),
	SurveyBeacon UMETA(DisplayName = "Survey Beacon"),
	ExplorerDroneControlCenter UMETA(DisplayName = "Explorer Drone Control Center"),
	TradeBeacon UMETA(DisplayName = "Trade Beacon"),
	GroundwaterPump UMETA(DisplayName = "Groundwater Pump"),
	SilicaExtractor UMETA(DisplayName = "Silica Extractor"),
	RareMineralMine UMETA(DisplayName = "Rare Mineral Mine"),
	UraniumMine UMETA(DisplayName = "Uranium Mine"),
	AlienMaterialHarvester UMETA(DisplayName = "Alien Material Harvester"),
	CopperSmelter UMETA(DisplayName = "Copper Smelter"),
	Glassworks UMETA(DisplayName = "Glassworks"),
	WaterPurifier UMETA(DisplayName = "Water Purifier"),
	CeramicsKiln UMETA(DisplayName = "Ceramics Kiln"),
	RareMetalRefinery UMETA(DisplayName = "Rare Metal Refinery"),
	FuelProcessor UMETA(DisplayName = "Fuel Processor"),
	UraniumProcessor UMETA(DisplayName = "Uranium Processor"),
	AlienMaterialProcessor UMETA(DisplayName = "Alien Material Processor"),
	MachineShop UMETA(DisplayName = "Machine Shop"),
	StructuralFabricator UMETA(DisplayName = "Structural Fabricator"),
	CompositeWorks UMETA(DisplayName = "Composite Works"),
	BatteryFactory UMETA(DisplayName = "Battery Factory"),
	OpticsFactory UMETA(DisplayName = "Optics Factory"),
	CoolingSystemsPlant UMETA(DisplayName = "Cooling Systems Plant"),
	PressureSystemsPlant UMETA(DisplayName = "Pressure Systems Plant"),
	ElectronicsAssembly UMETA(DisplayName = "Electronics Assembly"),
	PowerSystemsFactory UMETA(DisplayName = "Power Systems Factory"),
	FuelCellPlant UMETA(DisplayName = "Fuel Cell Plant"),
	ToolFactory UMETA(DisplayName = "Tool Factory"),
	DronePartsWorkshop UMETA(DisplayName = "Drone Parts Workshop"),
	AdvancedAlloyFoundry UMETA(DisplayName = "Advanced Alloy Foundry"),
	ControlSystemsAssembly UMETA(DisplayName = "Control Systems Assembly"),
	NavigationSystemsPlant UMETA(DisplayName = "Navigation Systems Plant"),
	LifeSupportAssembly UMETA(DisplayName = "Life Support Assembly"),
	ReactorComponentsPlant UMETA(DisplayName = "Reactor Components Plant"),
	HighDensityPowerPlant UMETA(DisplayName = "High-Density Power Plant"),
	SurveyEquipmentPlant UMETA(DisplayName = "Survey Equipment Plant"),
	CommunicationSystemsPlant UMETA(DisplayName = "Communication Systems Plant"),
	PrecisionEngineeringPlant UMETA(DisplayName = "Precision Engineering Plant"),
	HabitatSystemsPlant UMETA(DisplayName = "Habitat Systems Plant"),
	XenotechLaboratory UMETA(DisplayName = "Xenotech Laboratory"),
	ShipRepairBay UMETA(DisplayName = "Ship Repair Bay"),
	RefuelingStation UMETA(DisplayName = "Refueling Station"),
	ShipUpgradeDock UMETA(DisplayName = "Ship Upgrade Dock"),
	DroneMaintenanceBay UMETA(DisplayName = "Drone Maintenance Bay"),
	CalibrationCenter UMETA(DisplayName = "Calibration Center"),
	ResearchLaboratory UMETA(DisplayName = "Research Laboratory"),
	CustomFabricationCenter UMETA(DisplayName = "Custom Fabrication Center"),
	DecontaminationBay UMETA(DisplayName = "Decontamination Bay")
};

UENUM(BlueprintType)
enum class ESTPBuildCategory : uint8
{
	Energy UMETA(DisplayName = "Energy Build"),
	Industry UMETA(DisplayName = "Industry"),
	Logistics UMETA(DisplayName = "Logistics"),
	Infrastructure UMETA(DisplayName = "Infrastructure")
};
