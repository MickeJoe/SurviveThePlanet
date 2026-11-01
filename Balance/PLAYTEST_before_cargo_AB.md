# Speltest: Mother Base och tidiga kontrakt

Datum: 2026-10-18. Aktivt projekt: C:/UE5/SurviveThePlanet 5.8.

Starta en ny run. Efter Uplink bygger du Cargo Bay. Mother Base tilldelar automatiskt ett av fem leveranskrav. Uppdraget ändras inte när panelen öppnas igen. Öppna den befintliga objective-panelen och använd DELIVER TO MOTHER BASE när hela mängden finns i lagret. Leveransen förbrukar varorna.

## Fem möjliga krav

Alla kräver leverans till Home Base, ger +2 Mission Progression och Remote Base-blueprint. Ingen separat deadline i första profilen; Mission Confidence ger tidspressen.

| Krav | Circuit Boards | Confidence | Extra blueprint |
|---|---:|---:|---|
| 180 Iron | 4 | +18 | Steelworks |
| 200 Stone | 4 | +18 | CopperSmelter |
| 30 Concrete | 5 | +22 | PolymerPlant |
| 24 Steel | 6 | +24 | ConnectorPlant |
| 24 CopperMetal | 5 | +22 | GroundwaterPump |

## Fem kontraktsvarianter med tre alternativ vardera

En variant slumpas per kontraktsfaktion och run. De tre korten är alternativa åtaganden; bara ett kontrakt kan vara aktivt åt gången. Två faktioner har två möjliga varufamiljer och den tredje har en. Alla fem familjer finns över flera runs, totalt 15 kontraktsalternativ.

Mängder nedan gäller per leverans. Standard: 3 leveranser, 8 speltimmars frist per leverans. Priority: 4 leveranser, 6 speltimmar. High Stakes: 5 leveranser, 4 speltimmar. Vid 1x är en speltimme en verklig minut. Tidig manuell leverans är tillåten och startar nästa frist.

| Vara | Standard | Priority | High Stakes | Faktion |
|---|---:|---:|---:|---|
| Iron | 35 | 55 | 75 | helix_industrial |
| Stone | 40 | 65 | 90 | xeno_research |
| Concrete | 6 | 10 | 14 | helix_industrial |
| Steel | 5 | 8 | 11 | nomad_shipwrights |
| CopperMetal | 6 | 9 | 12 | nomad_shipwrights |

Varje lyckad leverans ger credits och +8/+7/+7 Confidence. Fullföljt kontrakt ger extra credits, 1/2/3 Circuit Boards och +1/+2/+3 Mission Progression. En miss kostar 3/5/7 Confidence och 5/10/15 reputation. Resterande leveranser kan ge vanliga rewards, men slutbonus uteblir. Exakta credit-belopp finns i Config/ContractOffers.json och kontraktskorten.

## Resultat

8 100 slutkörningar: 5 objectives x 5 kontraktsfamiljer x 3 nivåer x 3 planeter x 3 spelstilar x 12 frön. Det är tolv frön per kombination, inte 8 100 unika kartor. Tre beslutsstrategier fördelas över fröna: objective först, kontraktsreserv först och ingen handel.

Lyckad körning betyder att både Mother Base-leveransen och ett helt kontrakt avslutas utan missade leveranser innan Confidence når noll. Det är inte en fullständig run till seger.

| Agent | Standard | Priority | High Stakes | Totalt |
|---|---:|---:|---:|---:|
| careful | 100.0% | 100.0% | 90.1% | 96.7% |
| expansion | 100.0% | 99.0% | 61.4% | 86.8% |
| inexperienced | 97.0% | 97.7% | 20.8% | 71.8% |

Mother Base-leveransen klarades i samtliga slutkörningar. Median efter Cargo Bay: järn 7,6 minuter, sten 8,2, betong 7,8, stål 10,1 och raffinerad koppar 9,3 vid 1x. Spelares tider kan avvika.

Ytterligare 4 050 stresstester med 20% lägre produktion: careful 78,1%, expansion 73,7%, inexperienced 17,4% lyckade körningar. Stresstestet ändrade inte spelets produktion.

Första förslaget hade för långa frister relativt Confidence-decay. Frister kortades från 10/8/6 till 8/6/4 speltimmar och leveranser fick större Confidence-reward. Standard fick extra återhämtningsmarginal. Importerade Circuit Boards ges som rewards i stället för att krävas som tidiga kontraktsvaror.

## Omfattning och begränsningar

- Modellen täcker landning, gruvor/verkstad, Uplink, Cargo Bay, ett Mother Base-krav och ett kontrakt. Den täcker denna tidiga spelsträcka, inte alla 69 byggnader och sena kedjor.
- Uppdrag och kontrakt läses från spelets JSON. Kostnader, drönarspecialisering, handelspriser, stock och besök kommer från en exporterad Unreal-snapshot. Native fallback-recept och energi kommer från aktuell C++-kod. Snapshot och vissa beräkningsregler måste uppdateras när spelets regler ändras.
- Faktiska lagerkostnader, betalade recept, en gruvdrönare, två byggdrönare samt Uplink-belöningen på en extra byggdrönare ingår. Arbetseffektivitet, restider och misstag är antaganden. Agenterna köper inte fler drönare och bygger inte ut sena kedjor.
- Modellsteg är fem simuleringssekunder. Regn använder planeternas sannolikhet, intensitet och övergångar. Expansionsagenten kan bygga regnsamlare och betongfabrik; verkstaden är reservväg vid torka.
- Elnätet har produktion, efterfrågan, drönarnas förbrukningsmultiplikator, lagring och stopp vid energibrist. Lokal nättopologi och placering antas fungera. Inga blackouts inträffade med modellens två planerade generatorer. Det bevisar därför inte att energisystemet är färdigbalanserat.
- Sol och vind ger fasta 35 respektive 50 energi/minut i nuvarande native beräkning. Modellen lägger inte till väderberoende elproduktion som saknas där. Blueprint-specifika runtime-ändringar kräver fortsatt verklig mätning.
- Handel har riktiga besökstider, sortiment, priser, begränsad stock och credit-reserv. Första handlaren kommer 24 speltimmar efter Cargo Bay. Flygning/dockning och fysiska navigationshinder simuleras inte.
- Kontrakt levereras vid deadline i modellen. Spelet tillåter tidig manuell leverans; bra lagerplanering kan ge snabbare resultat i verkligt spel.
- Rewards, missar och Confidence-decay ingår. Blueprint-rewards kan öppna fler produktionsmöjligheter än agenterna använder; detta är en konservativ tidig produktionsplan.
- Alla 15 varianter har kontrollerats: köp av samtliga leveransvaror plus försäljning av belönade Circuit Boards ger inte credit-vinst vid dagens priser. Det är inte en fullständig exploiteringsanalys.

## Kör igen

Från projektroten: python Scripts/simulate_early_balance.py --seeds 12 --label final

Stresstest: python Scripts/simulate_early_balance.py --seeds 6 --label slow_production --production-scale 0.8

Balance/final innehåller runs.csv, summary.json och worst_replay.json med seed och händelser. summary.json har SHA-256 för modell och konfiguration. RuntimeSnapshot.json innehåller exporterade värden. Balance/Baseline innehåller säkerhetskopior före denna uppgift, inte en automatisk återställning.

## Ditt speltest

Spela en ny Nexaris-run på 1x. Notera tilldelat krav, tid till Cargo Bay och tid till leverans. Prova Standard först och High Stakes i en ny run. Notera dödtid, resursbrist, användbarheten i blueprint-rewards och om ett misstag går att återhämta sig från.

Leveransknappens tillgänglighet uppdateras när objective-panelen öppnas. Stäng och öppna igen om lagret fyllts medan den varit öppen.

## Ändringar och verifiering

Config/Objectives.json, ContractOffers.json och Traders.json innehåller profilen. Befintliga ObjectiveSubsystem och ContractOfferSubsystem återanvänds för tilldelning, betalning och rewards. MissionChoiceWidget lägger till leveransåtgärden i den befintliga WBP-panelen. MissionOutcomeTest.cpp verifierar progression, deadlines, lagerförbrukning och att rewards inte dupliceras.

UE5.8 Development Editor-build lyckades. SurviveThePlanet.Mission.ProgressionRewards lyckades med 0 fel. Den tillfälliga testvärlden saknar GameInstance och ger därför blueprint-reward-varningar; testet verifierar inte blueprint-inventariet. Det separata PIE-testet i Scripts/test_balance_playtest_ue.py lyckades: verkligt lager förbrukades, Circuit Boards och blueprint-inventariet uppdaterades, dubbel reward blockerades och samtliga fem kontraktsfamiljer med tre nivåer laddades. Resultatet finns i Balance/PIEVerification.json. Testspelet stoppades efter kontrollen; speltestet startar i en ny, ren run.

## Speltestjustering: expansionsutrymme

Communication Link ger nu även 50 Connectors och Sub-Base-blueprinten (RemoteBase), så att en utpost kan öppna byggutrymme före Cargo Bay. Simulatorn läser Uplink-resursrewards från samma JSON. Tidigare simuleringsrapporter gäller profilen före denna justering och modellerar inte fysisk platsbrist.
