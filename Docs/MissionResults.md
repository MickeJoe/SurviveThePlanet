# Mission results

The UMG layout is authored in `/Game/UI/Mission/WBP_MissionResult`, with behavior in `UMissionResultWidget`. It reuses the existing resource inventory and the player controller's planet selection flow.

`UMissionConfidenceSubsystem` owns mission progression and the immutable mission outcome. `SetMissionProgress` and `AddMissionProgress` clamp to the 0–10 range. Reaching 10 ends the mission successfully; confidence reaching exactly zero ends it in failure. `OnMissionProgressChanged` updates the existing mission HUD, and `OnMissionEnded` opens the results screen through the game mode. Further progress/confidence updates cannot change a completed outcome.

Objectives and supply agreements now grant their configured progression rewards on successful completion. The contract subsystem owns real inventory shipments, deadlines and completion; the UI displays its authoritative state. See `Docs/ProgressionRewards.md` for the initial balance, JSON configuration and agreement rules. Changing a progress bar's visual Percent alone does not change gameplay state.

The results show actual mission progress, completed buildings currently present, elapsed simulation time and final confidence. The game is paused and background HUD widgets are hidden. The existing camera input guard and Escape handling also respect mission completion. Play Again reloads the current map; Planet Selector reuses the existing controller navigation.

Reward amounts are editable on the WBP defaults. Initial values: victory 2,500 credits and 150 steel; defeat 450 credits and 30 steel. Rewards are applied once to the current colony's inventory. There is no new persistent profile or meta-progression system; changing maps starts the existing fresh colony flow.

Validation: UE 5.8 Development Editor build succeeded. `SurviveThePlanet.Mission.Outcome` passed. `Scripts/test_mission_results.py` passed the real PIE sequence: 9/10 stays active; 10/10 wins; rewards once; retry resets mission; confidence 0 loses; planet selection navigation works. Both screens were visually inspected. Runtime report is in `Saved/MissionResultsRuntime.json` and captures in `Saved/MissionResult_win.png` and `Saved/MissionResult_fail.png`.

Rebuild the authored assets with `Scripts/create_mission_result.py` inside Unreal Editor when necessary; this recreates that result WBP's widget tree and sets its initial reward defaults.

## Input after Play Again (2026-10-16)

The shared GameViewportClient survives OpenLevel, including the IgnoreInput flag set by the result screen's UI Only input mode. The gameplay player controller now explicitly initializes Game and UI input, flushes held keys, shows the cursor and focuses the game viewport in BeginPlay. This restores keyboard and mouse controls on the new map after Mission Failed or Mission Complete.

`SurviveThePlanet.Mission.RetryInputPIE` in `Tests/MissionOutcomeTest.cpp` starts its own gameplay PIE session on the currently open gameplay map. It triggers Mission Failed, clicks the real Play Again button, verifies that the shared viewport no longer ignores input, and sends E/right-mouse events through the viewport using the valid local input device. It checks that both reach the player and that the camera actually rotates. The test passed with zero errors. Standard PIE/input test dependencies are editor-only.