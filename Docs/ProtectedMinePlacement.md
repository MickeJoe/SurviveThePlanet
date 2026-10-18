# Skydd för gruvplatser

Varje aktiv resurskälla skyddar byggytan för sin konfigurerade gruva, inklusive den vanliga frigången till andra byggnader. Ytan räknas med gruvans verkliga footprint och dess placering relativt resursmodellen, även för modeller med förskjuten pivot.

Kontrollen ligger i `PlanetSurfaceManager::HasBuildingClearance`, som används både av förhandsvisningen och slutlig byggreservation. Det förhindrar att andra byggnader, även små elnätsförlängare, blockerar en framtida gruva. Den matchande gruvan på rätt plats undantas så att den kan ersätta sin resurskälla.

Skyddet reserverar inga extra navigeringsceller. Drönare kan fortfarande färdas över den fria ytan. Förbrukade och borttagna resurser skyddar inte längre en framtida gruvyta.

Ändrade källfiler: `Gameplay/Planet/PlanetSurfaceManager.h` och `.cpp`. Regressionstest finns i `Scripts/test_mine_placement_protection.py`.

Regeln påverkar nya placeringar. Redan byggda byggnader flyttas inte automatiskt.

Verifierat: UE5.8 Development Editor-build och git diff --check passerar. Körtest på Nexaris, Aridus och Borealis kontrollerade fem startresurser per planet: tomma celler i skyddszonen nekades både i förhandsvisningen och vid slutlig reservation, medan rätt gruva kunde skapas och kopplas till sin källa. Navigeringscellerna förblev tillgängliga. Testet etablerade sektorerna för att isolera geometriregeln från separata krav på utforskning och sektortillstånd.
