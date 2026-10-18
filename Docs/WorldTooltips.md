# Namnverktygstips

Resurser, byggnader och drönare på kartan får ett UMG-baserat namnverktygstips efter 0,25 sekunders hovring. Det följer muspekaren och hålls inom bildens kanter. Widgeten blockerar inga klick. Byggnadsförhandsvisningar, öppna menyer och muspekaren över klickbart UI visar inget namn från kartan bakom dem.

Namn hämtas från befintlig Resource Catalog, Building Data och Drone Data. Byggmenyn och resursikonerna behåller sina befintliga verktygstips. Tilldelade drönarplatser och drönarval har kompletterats med namnverktygstips på knapparna.

Ändrade filer: PlayerController .h/.cpp, BuildingInfoWidget.cpp samt nya WorldHoverTooltipWidget .h/.cpp och WBP_WorldHoverTooltip. UMG-layouten finns i `/Game/UI/WBP_WorldHoverTooltip` och kan redigeras i Widget Blueprint-editorn.

UE5.8 Development Editor-build och git diff --check passerar. Körtest kontrollerar namnkoppling och widgetens visningsläge i en riktig PIE-värld. Fullständig visuell kontroll med fysisk muspekare är inte automatiserad.
