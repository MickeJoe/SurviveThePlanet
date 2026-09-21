# Sektormallar – konceptförslag

Skapade med inbyggda imagegen. Exakta prompar och korrigeringar finns i Prompts.json.
Detta är illustrationsförslag, inte exporter från Unreal, exakta polygonritningar
eller färdiga spelassets. Inga levels, shape-assets eller seed-inställningar ändrades.

## Tolv återanvändbara placeringsytor

01–06 illustrerar befintliga principer ungefärligt; de ersätter inte projektets faktiska former.
Nya förslag: 07 grund halvmåne, 08 bred oregelbunden ö, 09 rundad vinkel,
10 S-form, 11 djup hästsko och 12 asymmetrisk solfjäder.

## Sex nya sektormallar, sju totalt

Beslut vid implementation: behåll ST_First separat och lägg till samtliga sex
förslag nedan. Endast ST_First, 01 och 02 tillåts som startsektor. Se
`Docs/SectorTemplateCatalog.md` för den implementerade katalogen.

| Mall | Karaktär | Föreslagna form-ID:n |
| --- | --- | --- |
| 01 Öppen bas | Befintlig princip, stor central byggyta | 01, 02, 03, 04, 05, 06 |
| 02 Bred passage | Bred genomfart och två byggfickor | 03, 04, 07, 09, 06, 08 |
| 03 Öar och fickor | Uppdelade kluster och flera vägar | 06, 08, 10, 12, 06, 08 |
| 04 Skyddad ficka | Öppen hästsko kring en byggficka | 11, 02, 09, 06, 04 |
| 05 Slingrande stråk | S-formad rygg mellan byggytor | 10, 01, 03, 07, 08, 06 |
| 06 Tre byggfickor | Förgrenade kluster med sammanhängande passager | 12, 05, 07, 09, 06, 08 |

En form kan återanvändas flera gånger inom samma mall. Mall 04 föreslår fem
placeringar; de övriga sex. Tolv avser antalet unika former i biblioteket, inte
antalet placeringar per sektor. Bildernas små planvyer är schematiska och kan
avvika från bibliotekets exakta konturer; tabellen anger den avsedda kombinationen.

Vid eventuell implementation: behåll den faktiska sektorsradien 4000 cm och
använd projektets befintliga former 01–06 oförändrade som utgångspunkt.
Kontrollera byggbara ytor, sammanhängande drönarpassager, sektorgränser och
prestanda. Rotation/spegling är inte automatiskt tillgängliga i den nuvarande
generatorn och behöver separat validering om de ska användas för variation.
