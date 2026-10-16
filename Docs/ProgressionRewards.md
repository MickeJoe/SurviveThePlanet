# Mission progression rewards

Progression is a configurable completion reward, rather than a fixed amount for every objective or agreement. All rewards feed `UMissionConfidenceSubsystem::AddMissionProgress`; reaching the existing target of 10 opens Mission Complete.

## Initial balance

| Objective id | Progression |
| --- | ---: |
| build_first_mining_machine | +1 |
| deliver_iron_to_home_base | +2 |
| mission_build_communication | +1 |
| mission_deliver_copper | +2 |
| mission_deliver_water | +1 |
| mission_build_storage | +1 |
| mission_build_concrete_plant | +2 |
| mission_expand_mining | +2 |

| Supply agreement tier | Completion progression |
| --- | ---: |
| Standard | +1 |
| Priority | +2 |
| High Stakes | +3 |

Edit `Config/Objectives.json`: include `{ "type": "give_mission_progress", "amount": 2 }` in an objective's rewards. Omit the entry for no progression reward. Amounts are positive integers.

Edit `Config/ContractOffers.json`: include a `missionProgress` effect in a tier's `completionBonus`, using its existing progression icon reference. Each tier can have its own amount. Progression is not awarded when accepting the agreement or completing only part of it.

The reward is displayed in the active objective HUD, objective selection cards, agreement offer cards and active agreement tracker. The tracker follows the objective list's actual height, and agreements scroll when space is limited.

## Agreement execution

The existing agreement previously had an inventory-incompatible placeholder item id and a UI-only countdown. It now requests Circuit Boards (`ControlChip`), keeping the stable offer id `helix_advanced_components`. The offer title and icon match the real resource.

The subsystem owns delivery counts, deadlines, penalties, completion and reward settlement. The UMG tracker offers **DELIVER SHIPMENT** when the inventory can pay for a full shipment. A shipment is also attempted automatically at its deadline. One game hour is 60 simulation seconds, consistent with mission confidence and the game clock; world pause and simulation speed apply.

An agreement has its configured number of delivery slots. Every slot must succeed to earn the completion bonus. A missed slot applies its penalties; the remaining deliveries can still earn their individual rewards, but an agreement with any missed slot ends Failed and gets no completion progression. The UI distinguishes active, fulfilled and failed agreements.

The state is committed before broadcasting rewards; repeated delivery calls cannot reward a completed agreement again. Once finished, the player may explicitly accept a new agreement and pay for a new set of shipments. This makes the current set of objectives and agreements capable of filling the mission bar through repeated, paid supply contracts.

Existing credit, resource and confidence effects reuse existing inventories/subsystems. Reputation changes and trader cooldowns are tracked for the world session. The existing `rare_industrial_blueprints` pool grants an unowned Industry blueprint from the building catalog through the existing blueprint inventory, when one is available.

All other completion rewards settle before adding progression, so the final reward that fills the mission bar does not interrupt the same completion's payouts. Failed objectives and failed agreements do not receive completion progression. Mission end freezes further rewards/deliveries.

## Verification

UE 5.8 Development Editor build succeeded. `SurviveThePlanet.Mission.ProgressionRewards` and `SurviveThePlanet.Mission.Outcome` passed with no errors or warnings. The progression test covers actual objective events, partial completion, different reward amounts, agreement acceptance, insufficient inventory, shipment costs, completion-only progression, duplicate completion protection, payout order, pause, missed deadlines, failed agreements and a real reward filling the mission bar.

The gameplay PIE UI check verifies reward labels in objective HUD/selection and the WBP agreement popup, then uses the real delivery handler to complete a Standard agreement. Report: `Saved/ProgressionUIRuntime.json`. Visual captures: `Saved/Progression_*.png`.
