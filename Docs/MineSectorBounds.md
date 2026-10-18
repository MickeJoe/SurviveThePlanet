# Gruvor inom sektorgränser

Resursgenereringen kontrollerar nu hela gruvans byggyta, inte enbart dess mittpunkt. Alla fyra hörn måste tillhöra samma sektor som resursen. Kontrollen använder planetytans rutstorlek och transform samt den konfigurerade gruvans footprint och placeringsförskjutning.

Ändringen finns i `Gameplay/World/SectorPopulation.cpp` och gäller även sökningen efter specialresurser. Befintliga krav på etablerade sektorer och skyddet mot blockerande byggnader behålls.

Regressionstestet `Scripts/test_mine_sector_bounds_ue.py` behåller grannsektorerna som Undiscovered och provar att skapa och ansluta gruvor på startresurserna. Det kontrollerar även att andra byggnader inte kan blockera gruvplatserna.

Ny generering kräver en ny runda. Redan genererade resurser flyttas inte under ett pågående spel.

Verifierat i UE5.8: Development Editor-build och git diff --check passerar. På Nexaris, Aridus och Borealis provades fem startresurser per planet med samtliga grannsektorer fortfarande Undiscovered. Alla gruvors hörn låg inom startsektorn och riktiga gruvaktörer kunde anslutas till resurserna. Skyddet mot blockerande byggnader passerade också.
