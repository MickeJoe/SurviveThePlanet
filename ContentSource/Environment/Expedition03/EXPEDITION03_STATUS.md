# Expedition03 – fortsättning

Projekt: C:/UE5/SurviveThePlanet 5.8/SurviveThePlanet.uproject
Arbetskarta: /Game/PlanetLevel_RockPreview

## Omfattning

Förbättrade klippor, markmaterial och alienvegetation samt stöd för vind och öppnande blommor. Minimiavstånd mellan byggnader ingår INTE i uppgiften. WBP/UI och huvudkartan är inte ändrade av miljöarbetet.

## Senast sparat

- Expedition02:s fem stenmeshes och fem växtdelar är importerade och placerade: 308 miljöaktörer, en översiktskamera och ett fyllnadsljus.
- Ny Blenderkälla STP_Terrain03.blend och FBX SM_TerrainFlat_03: plan markdel på 20 x 20 meter, yta på 56,55 cm.
- /Game/Environment/Expedition03/Materials/M_Terrain_03 blandar jord, sand och grus med oregelbundna övergångar och diskret normaldetalj.
- Previewkartan använder 80 markdelar enligt projektets nuvarande cirkulära världsform. Äldre anteckningar som anger 100 markdelar är felaktiga för denna version.
- Automatisk ombyggnad i konstruktionen är avstängd på previewkartans SurfaceManager. Tidigare dubblerade komponenter rensades och korrekt höjd verifierades efter återöppning.
- 18 ytterligare lediga gridceller under överlappande klippor/boulders reserverades, utöver tidigare reservationer. Ingen regel om avstånd mellan byggnader är införd.
- Säkerhetskopia före Expedition03: /Game/Environment/Expedition03/PlanetLevel_Before03.

## Rörelsestöd

Växtmaterial använder vertexfärg R för att hålla roten stilla och låta toppen vaja. WindStrength och WindSpeed är justerbara. Sex blommor består av bas och sex separata kronblad; AutoBloom=1 demonstrerar öppning, AutoBloom=0 och BloomOpen 0–1 ger manuell styrning. Detta stöd är byggt och visuellt kontrollerat. Vinden är ännu inte kopplad till spelets väderdata.

## Verifiering och nästa steg

Spelkontroll 2026-09-11: 9 av 9 resursområden hade en nåbar angöringspunkt enligt spelets gridvägsökning. 80 markdelar och korrekt markhöjd verifierades efter återöppning. Ljusprioriteten korrigerades och kartan sparades. Saved/Expedition03/gameplay_verified.json innehåller resultatet; Gameplay03.png visar verklig PIE med WBP. Testet gäller vägar från en startpunkt nära basen med 1x1 footprint; det ersätter inte ett helt manuellt drönartest eller test av alla byggnadsstorlekar.

stp_ue03_verify.py korrigerar ljusprioritet, sparar previewkartan och väntar därefter på PIE för att kontrollera vägar och ta Shot showui. Starta PIE efter att skriptet laddats. Skriptet lämnar UE öppet.

Miljön är ett första konstnärligt prov och når inte referensbildens slutkvalitet. Prioritera naturligare klipptytor, mer detaljrik men lugn mark, och tätare lokala växtgrupper. Återanvänd befintliga resurser innan fler modeller genereras. Kör inte om hela Expedition02-byggaren på en manuellt redigerad previewkarta: den ersätter egna aktörer och äldre markmaterial. Expedition03-byggaren ändrar marken och kompletterar gridreservationer, men återskapar inte växterna.

