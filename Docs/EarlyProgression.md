# Tidig progression

Implementerat i `C:\UE5\SurviveThePlanet 5.8`.

Start-BP: Mining Machine, Solar Array, Wind Generator, Energy Extender, Crude Workshop och Communication Module. Övriga byggnader är låsta tills deras BP erhålls.

| Byggnad | Preliminär kostnad |
|---|---|
| Basic Mining Machine | 10 järn |
| Crude Workshop | 40 järn |
| Solar Array | 10 järn |
| Wind Generator | 15 järn |
| Communication Module | 20 järn + 10 betong |
| Cargo Bay | 5 betong + 5 Circuit Boards |

Startlager på Nexaris: 20 järn, 3 Circuit Boards, 100 Connectors och 1 000 energi. Betong börjar på 0. Övriga startvärden är oförändrade. Connectors används av det befintliga systemet för elnätsförlängning.

Vägen till länken: bygg en stengruva och Crude Workshop, ordna el och tilldela drönare. Välj betongreceptet och starta produktionen. 40 sten räcker till länkens 10 betong. Järngruvan möjliggör fortsatt expansion. Bygg först järngruvan och utvinn mer järn; verkstaden kostar 40 järn och kan inte betalas med startens 20. Startsektorn får alltid järn, sten och koppar. Gruvornas markfrigång kontrolleras av den befintliga placeringskontrollen. Crude Workshop kan fortfarande inte tillverka Circuit Boards.

ESTABLISH UPLINK ger vid färdigbyggd Communication Module:

- 5 Circuit Boards
- BP för Cargo Bay, Water Collector, Concrete Plant och Energy Storage
- 1 Explorer Drone
- +30 Mission Confidence
- +1 Mission Progression

De tre extra industri-BP:na gör de befintliga objectives som väljs efter länken genomförbara. Reward-listan framgår av uplink-objectivets beskrivning. Befintliga objective- och BP-system används; ingen separat upplåsningsmekanik har införts.

Ändrade filer: `Config/Objectives.json`, elva Building Data Assets och Nexaris-kartan. Reproducerbar konfiguration finns i `Scripts/configure_early_progression.py`; körtestet finns i `Scripts/test_early_progression.py`.

Verifierat i UE5.8:

- BlueprintToolbar och Mission.ProgressionRewards: godkända. Det senare använder en isolerad testvärld utan GameInstance och loggar därför fyra varningar för BP-rewards; den riktiga GameInstance-vägen kontrollerades separat i PIE.
- Ny PIE-session: exakt sex start-BP, noll startbetong och rätt Connectors-lager.
- Kostnadskontroll av öppningens gruvor, verkstad och solkraft.
- Uplink-händelsen delar ut exakt fem Circuit Boards och alla fyra BP i riktig GameInstance.
- Upprepad händelse delar inte ut rewards igen.
- Cargo Bays konfigurerade kostnad verifierad.
- Git diff --check godkänd.

Hela öppningen har inte spelats manuellt från gruvplacering till färdig Cargo Bay. Värdena är en första balanspassning; produktions- och drönartempo bör utvärderas under spel.
