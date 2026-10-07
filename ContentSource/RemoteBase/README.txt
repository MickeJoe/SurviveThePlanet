Remote Base — Blender och UE 5.8

Projekt: C:\UE5\SurviveThePlanet 5.8\SurviveThePlanet.uproject
Blender-källa: C:\UE5\SurviveThePlanet 5.8\ContentSource\RemoteBase\RemoteBase.blend
FBX-källor: C:\UE5\SurviveThePlanet 5.8\ContentSource\RemoteBase\SM_RemoteBase_*.fbx
UE Blueprint: /Game/Units/Buildings/RemoteBase/BP_RemoteBase
Jämförelsescen: /Game/Units/Buildings/RemoteBase/Preview/L_RemoteBasePreview

Modellen består av Body, Antenna, Ring och Scan. Totalt 14 980 trianglar.
Basmodulens footprint inklusive ramp och kraftenhet är cirka 4,52 × 4,95 meter.
Höjd inklusive antenn: cirka 4,07 meter. Antennpivot: Z = 218 cm i Blueprinten.
Kroppen har genererad enkel kollision; antenn och VFX saknar kollision.
UV0 genererat i Blender, lightmap-UV genererat vid UE-import.

Animation och effekter:
- Antennrotation: 12 grader/sekund, ett varv på 30 sekunder.
- Cyan statusljus: mjuk puls med 3 sekunders period.
- Segmenterad cyan markring.
- Scanring som expanderar från ungefär 2,45 till 4,04 meters radie och tonar ut,
  i en loop på 4 sekunder. Detta är en visuell prototyp, utan gameplay-trigger.
- Blender-källan innehåller antennens keyframes samt ljus- och scan-drivers.
- FBX innehåller separata statiska meshdelar. I UE gör RotatingMovementComponent
  antennrotationen och materialens Time-noder driver ljuspuls och scanring.

Verifiering:
- Blender: frame 76 vid 30 fps ger 30 graders antennrotation och scan-skala 1,40625.
- Blueprint kompilerar med warnings_as_errors=True.
- UE Play-In-Editor: rätt komponent roterar och uppmätt rotation över 40,67 sekunder
  stämmer med 12 grader/sekund, fel mindre än 0,000001 grader.
- Proportioner, material och synlig scanring granskade i en UE-skärmbild.
- Jämförelsescenen återanvänder projektets befintliga BaseModule-mesh.

Omfattning:
Detta levererar modellen och en fristående, animerad visual-Blueprint.
Sektorlåsning, byggkostnad och placering via byggmenyn är inte implementerade här.
Projektets vanliga gameplay-karta, katalog och C++-kod har inte ändrats.

Authoring-skript finns i projektets Scripts/ och BeginPlay-DSL i ContentSource/RemoteBase/.
Blender-operationer gjordes via Blenders inbyggda MCP på 9876; UE-operationer via UE MCP.
Build-skriptet är avsett för en separat authoring-session; befintliga exporter ska
bara ersättas när en avsiktlig modelluppdatering görs.
