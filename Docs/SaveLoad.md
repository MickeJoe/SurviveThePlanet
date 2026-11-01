# Colony save/load

Esc opens WBP_PauseMenu. Save Game creates an independent slot every time. The editable display name defaults to local date and time (YYYY-MM-DD HH:MM:SS); blank names use that default. Duplicate names never overwrite a slot. The list is newest first. Load Selected Game reloads the original map and returns to the paused Esc menu.

Files use Unreal SaveGame serialization in Saved/SaveGames/Colony_<GUID>.sav. Display names are never used as filesystem paths. The format is versioned; missing maps/classes, invalid IDs and unsupported versions are rejected before travel.

UColonySaveSubsystem stores explicitly marked SaveGame properties. Each field has its own named payload. Actor/component references use snapshot IDs instead of PIE package paths. Nested struct members use the normal persistent archive. Transient visual components are recreated rather than saved.

Snapshots include buildings and construction progress, production settings and cycle progress, workers and assignments/jobs/routes, ongoing exploration, deposits and depletion, sector discovery, inventories/credits/energy, objective and contract state, blueprint ownership, merchant stock/visit timing, weather state/random sequence, clock/speed/pause, and camera position/zoom. Terrain is regenerated from the saved map; sector population is revealed before matching saved actors. Occupancy, power and bound UI are rebuilt after restoration. Rewards are never replayed.

Verification: SurviveThePlanet.SaveLoad.RoundTrip tests serialization, changed-world restoration, ongoing exploration, worker references, inventory/depletion, duplicate reward prevention, invalid slot handling and multiple same-name slots. Scripts/test_save_load_pie.py exercises the authored Save/Load buttons, actual map travel, clock, restored inventory/progression and paused-menu return, and writes Saved/SaveLoadPIEReport.json. Test-created slots are removed by the tests.
