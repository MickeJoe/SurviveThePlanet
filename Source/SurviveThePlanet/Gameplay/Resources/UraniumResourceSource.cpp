#include "UraniumResourceSource.h"
#include "Gameplay/Buildings/IndustrialEffectsComponent.h"

AUraniumResourceSource::AUraniumResourceSource()
{
 ResourceType = EResourceType::UraniumOre;
 StartingAmount = 4000;
 bRemainsVisibleAfterDiscovery = true;
 Effects = CreateDefaultSubobject<UIndustrialEffectsComponent>(TEXT("UraniumEffects"));
 Effects->EffectKind = EIndustrialEffectKind::UraniumDeposit;
}
