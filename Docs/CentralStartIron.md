# Central startjärnkälla

Startsektorns järn placeras nära den befintliga basen. Sökningen börjar cirka 900 cm från basen och stannar senast vid 1600 cm, med extra marginal till basens faktiska elräckvidd. Järnet placeras inte utanför den räckvidden som reservlösning.

Hela gruvytan kontrolleras både vid rutnätets snappade position och vid den verkliga modelljusterade position som byggförhandsvisningen använder. Befintliga kontroller av terräng, andra byggnader och avstånd mellan gruvor behålls. Sten och koppar använder fortsatt sina övriga placeringar i sektorn.

Den tidigare tomma diagnosen när sektorregeln nekar en gruva har ersatts med en förklaring i placeringsloggen.

Specialresurser och geotermiska platser planeras också mot övriga resursers fulla framtida gruvytor. Det tidigare avståndstestet mellan mittpunkter kunde lämna överlappande gruvytor, vilket blockerade järngruvan på Aridus.

Ändrade filer: `Gameplay/World/SectorPopulation.h/.cpp`, PlayerController .h/.cpp och nya `Tests/CentralIronPlacementTest.cpp`.

Testerna `SurviveThePlanet.Start.CentralIron.Nexaris`, `.Aridus` och `.Borealis` använder riktig musspårning, gruvförhandsvisning och controllerns byggklick. De kontrollerar startens 20 järn, kostnaden 10 järn, fortsatt låsta grannsektorer och att den färdiga gruvan ansluts till basens el utan extender.

Slutresultat: alla tre controller-tester passerar, utan att tillföra resurser eller bygga en extender. UE5.8 Development Editor-build och git diff --check passerar.
