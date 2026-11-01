# Speltest: Cargo Bay, paket A och B

Aktiv profil 2026-10-18. Projekt: C:/UE5/SurviveThePlanet 5.8.

Starta en NY run. Uplink och Cargo Bay fungerar som tidigare. När Cargo Bay är färdig väljer Mother Base slumpmässigt ett paket för denna run. Paketet ändras inte när panelen öppnas igen.

| Paket | Tre blueprints | Tre separata leveransuppdrag |
|---|---|---|
| A | Glassworks, Polymer Plant, Water Purifier | 15 Glass, 15 Polymer, 20 Purified Water |
| B | Polymer Plant, Fuel Processor, Water Purifier | 15 Polymer, 12 Synthetic Fuel, 20 Purified Water |

Du får göra leveranserna i valfri ordning och får välja bort uppdrag. Oavklarade uppdrag ligger kvar utan egen deadline. Mission Confidence minskar fortfarande med 2,5 per speltimme och noll innebär förlust. Vid 1× motsvarar en speltimme en verklig minut.

Nya tidsstyrda uppdrag är uppskjutna enligt beslut. Systemet tillåter upp till 32 aktiva samtidigt. Överskjutande tillgängliga uppdrag väntar i stället för att radera äldre. Uppdragspanelen har rullning och visar fler än åtta kort.

## Försörjning och kostnader

Uplink ger dessutom Groundwater Pump och Silica Extractor som grundläggande försörjning; dessa ingår inte i Cargo Bays tre produktionsblueprints. Pumpen ger vatten med el och tilldelade byggdrönare även när det inte regnar. Silica Extractor byggs vid en Silica-fyndighet; befintlig världsgenerering garanterar en startfyndighet. Cargo Bay ger 40 Coal som en begränsad startreserv. A behöver 10 Coal och B behöver 34 Coal för sina tre ursprungliga leveranser. Ytterligare användning kräver egen försörjning eller handel.

| Byggnad | Byggkostnad | Recept | Grundcykel / el per minut |
|---|---|---|---|
| Glassworks | 15 Iron + 5 Concrete + 1 Circuit Board | 2 Silica → 1 Glass | 20 s / 12 |
| Fuel Processor | 15 Iron + 5 Concrete + 1 Circuit Board | 2 Coal + 1 Water → 1 Synthetic Fuel | 20 s / 12 |
| Water Purifier | 15 Iron + 5 Concrete + 1 Circuit Board | 2 Water → 1 Purified Water | 20 s / 12 |
| Groundwater Pump | 10 Iron + 5 Concrete | → 2 Water | 20 s / 10 |
| Polymer Plant | 15 Iron + 5 Concrete + 1 Circuit Board | 2 Coal + 1 Water → 3 Polymer | 20 s / 12 |
| Silica Extractor | 10 Iron | Bryter Silica | Befintliga gruvregler / 10 |

Den befintliga globala produktionsfaktorn är 3×. Grundcykeln 20 s blir 6,67 s vid 100 % total drönareffektivitet. Glassworks, Fuel Processor, Water Purifier och Groundwater Pump behöver tilldelade byggdrönare; återanvänd och flytta dina drönare mellan byggandet och maskinerna. Polymer Plant behåller sitt befintliga automatiska arbetssätt. El, byggstatus och produktionspaus gäller fortfarande.

Varje leverans kostar verkliga lagervaror och ger 2 Circuit Boards samt +2 Mission Progression. Confidence: Glass +24, Polymer +20, Purified Water +26, Synthetic Fuel +24. Uppdrag kan inte belöna samma leverans två gånger. Uplink + Cargo Bay + alla tre leveranser ger 8/10 progression; kontrakt kan bidra med återstående progression. Alla tre uppdrag är därför inte ett obligatoriskt villkor för seger.

## Simulering och verifiering

1 800 ekonomiska körningar: A/B, Nexaris/Aridus/Borealis, tre spelstilar, alla sex leveransordningar, överhoppade uppdrag, uteblivna leveranser, handelsstöd, tio minuters försening och halverad produktion. Modellen återanvänder den tidigare landningssimuleringen för resursutvinning, byggande, verkstad, el och Confidence. Inga gratis färdigvaror används i simuleringen.

- Normalfall: samtliga 1 080 körningar klarade sina tre uppdrag. Median efter Cargo Bay: A 15,92 minuter, B 13,58 minuter vid 1×.
- Uppdrag plus efterföljande kontrakt klarades i 91,9 % för A och 93,3 % för B. Svåraste kontraktet kan missas även när alla uppdrag är klara.
- Ett uppdrag överhoppat: de två prioriterade uppdragen klarades i samtliga 360 körningar. Det överhoppade låg kvar. Efterföljande kontrakt hade lägre marginal: 67,8/68,3 % klarades.
- Inga leveranser: alla 120 körningar förlorade genom Confidence. Alla tre uppdragen låg kvar.
- Belastningsfall: 50 % klarade alla tre uppdrag och 40 % även det efterföljande kontraktet. Detta är avsiktligt hårda kombinationer, inte den normala svårighetsgraden.

Resultat och alla körningar: cargo_packages/summary.json respektive runs.csv. Unreal-testresultat: CargoPackages/index.json och CargoProgression/index.json. Assetinställningar som faktiskt sparats: CargoPackageAssets.json. Recepten testas mot verkliga Blueprint-klasser och spelets gemensamma produktionskomponent. Paketens blueprintbelöningar verifieras i ett riktigt GameInstance-inventarium.

Modellen bevisar inte att spelet är roligt eller att alla byggnader får plats. Den modellerar inte verklig kollisionskontroll, kabelförläggning, drönarnas navigering eller synlighet. Elmodellen använder nuvarande författade produktionsvärden; vädervariation i faktisk elproduktion behöver speltestas. De gamla rapporterna final/ och slow_production/ beskriver tidigare fem uppdragsvarianter och gäller inte denna profil.

## Fokusera på detta i speltestet

1. Får du plats med pump, produktionsbyggnader och eventuell Sub-Base utan att fastna?
2. Är det tydligt vilka maskiner som behöver byggdrönare, och är det rimligt att flytta dem?
3. Kommer första leveransen i tid för att skapa hopp, utan att alla tre känns gratis?
4. Prova att lämna ett uppdrag ogjort och fortsätta med kontrakt. Det ska vara ett val med konsekvenser.
5. Jämför A och B över nya runs: känns Glassworks-kedjan för lång eller bränslekedjan för enkel?

De fem kontraktsfamiljerna med tre alternativ vardera är oförändrade. Tidigare detaljtabell finns i PLAYTEST_before_cargo_AB.md; aktuella belopp är Config/ContractOffers.json.

Kör ekonomimodellen igen: python Scripts/simulate_cargo_packages.py.
