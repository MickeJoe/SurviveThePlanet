# Expedition02 – miljöprov

Projekt: C:/UE5/SurviveThePlanet 5.8/SurviveThePlanet.uproject
Karta: /Game/PlanetLevel_RockPreview

## Sparat

- Fem nya stenmeshes: Crown, Ridge, Wall, Boulder och Scree. Texturer med bakad basfärg och tangentnormaler. Ersätter de tidigare blockiga prototyperna i previewkartan.
- Alien Coral, Rosette, Trumpets, Bloom Base och separat Bloom Petal.
- Material och importerade resurser: /Game/Environment/Expedition02.
- 308 placerade miljöaktörer, plus E02_OverviewCamera. Sju större klippgrupper och två mindre grupper runt öppna byggytor.
- Ny kopia av markchunk med ett världsförankrat material för jord, sand och grus. Ursprunglig GroundChunk1 är bevarad.
- Previewkartan är sparad. Föregående karta finns som PlanetLevel_RockPreview_before02.umap i denna källmapp.

## Rörelse

Växternas vertexfärg R maskerar vind från fast rot till rörlig topp. Materialparametrarna WindStrength och WindSpeed styr en mjuk vindrörelse. Detta är demonstrationsvind och är ännu inte kopplat till spelets vädersystem.

Sex blommor använder vardera sex separata kronblad med pivot vid fästet. Materialet M_Plant_Petal_Wind öppnar dem med AutoBloom=1, BloomPeriodSeconds=32. AutoBloom=0 låter BloomOpen (0–1) bestämma öppningen. Rörelsen görs i materialet; inget skelett krävs. Blenderkällan behåller separata delar för fortsatt arbete.

## Spel och återstående kontroll

Större stenar har konvex kollision. 29 av 39 större stenaktörer fick en hel reservation i befintligt bygg-/navigationsgrid; överlappande reservationer kan inte läggas till och fullständig navigationskontroll återstår. Småsten och växter är dekorativa utan kollision. Minimiavstånd mellan byggnader är inte implementerat.

Markfelet är rättat i previewkartan: 2 000 gamla, dubblerade statiska markkomponenter togs bort. Ett nytt lager med 100 chunks skapades, med ythöjd 56,55 cm. Automatisk konstruktion av marken är avstängd på just denna kartinstans, eftersom dess transient-array inte återfinner tidigare sparade instanskomponenter. Nativekoden är inte ändrad. Ändra inte detta tillbaka utan att först hantera gamla komponenter. E02_SoftFill ger ett svagt fyllnadsljus.

UE-rotationer måste alltid skapas med namngivna argument (pitch/yaw/roll); positionsargument gav fel orientering. Byggskriptet är korrigerat.

WBP/UI och huvudkartan PlanetLevel är inte ändrade av detta miljöprov. Materialimport och scensparning rapporterar COMPLETE; visuella kontrollbilder och eventuella begränsningar finns i Saved/Expedition02. COMPLETE betyder att genereringsskriptet är klart, inte att konstnärlig eller spelmässig slutgranskning är färdig.

## Fortsättning

Blenderkälla: STP_Expedition02.blend. Byggskript: stp_ue02_build.py. Kör endast mot ovanstående UE 5.8-projekt. Skriptet ersätter egna genererade aktörer med taggarna STP_Expedition02 och STP_RockKit_Step01 och skriver om egna material. Manuell redigering av dessa aktörer/material bör säkerhetskopieras före omkörning.

Kontrollbilderna visar nu växtgrupper och stenras ovan mark. Två bilder med samma detaljkamera visar förändrade kronblad och grenar. Bilderna är editorrenderingar; basbyggnaden och WBP skapas vid spelstart och visas därför inte där. Miljön är fortfarande ett första konstnärligt prov: markens övergångar har synlig regelbundenhet och belysningen behöver fortsatt bedömning från spelkameran. Prioritera markövergångar, drönarpassager och läsbarhet. Därefter kan vind kopplas till vädersystemet och variation/prestanda finjusteras.
