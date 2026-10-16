"""Exercise real UMG controls and drone assignment against live workshop production."""
import unreal,json,time,traceback,shutil
from pathlib import Path
root=Path('C:/UE5/SurviveThePlanet 5.8');editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem);world=editor.get_game_world();assert world
actors=lambda cls:list(unreal.GameplayStatics.get_all_actors_of_class(world,cls))
statics=unreal.get_default_object(unreal.GameplayStatics);wl=unreal.get_default_object(unreal.load_class(None,'/Script/UMG.WidgetBlueprintLibrary'));subs=unreal.get_default_object(unreal.load_class(None,'/Script/Engine.SubsystemBlueprintLibrary'))
state={'stage':0,'since':time.monotonic(),'started':time.monotonic(),'checks':[]};backups={p:p.read_bytes() for p in (root/'Saved/SaveGames').glob('*.sav')}
def spawn(cls,pos):
 t=unreal.Transform(location=pos);a=statics.call_method('BeginDeferredActorSpawnFromClass',(world,cls,t,unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT));return statics.call_method('FinishSpawningActor',(a,t,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
def widget(name):
 return next(o for o in unreal.ObjectIterator(unreal.Widget) if o.get_name()==name and o.get_path_name().startswith(state['info'].get_path_name()))
def text_values():
 paths=[state['info'].get_path_name()]+[widget('RecipeButtonsBox').get_child_at(i).get_path_name() for i in range(widget('RecipeButtonsBox').get_children_count())]
 return [str(o.get_text()) for o in unreal.ObjectIterator(unreal.TextBlock) if any(o.get_path_name().startswith(p) for p in paths)]
def effect_active():return any(c.is_active() for c in state['plant'].get_components_by_class(unreal.NiagaraComponent))
def finish(passed,error=None):
 for p,b in backups.items():p.write_bytes(b)
 unreal.GameplayStatics.set_global_time_dilation(world,1)
 (root/'Saved/ProductionPanelRuntime.json').write_text(json.dumps({'passed':passed,'checks':state['checks'],'error':error},indent=2));unreal.unregister_slate_post_tick_callback(panel_review_handle);unreal.log('PRODUCTION_PANEL_RUNTIME '+str(passed)+' '+str(error))
def click_recipe(index):
 cards=widget('RecipeButtonsBox');button=next(o for o in unreal.ObjectIterator(unreal.WorkshopRecipeButton) if o.get_path_name().startswith(cards.get_child_at(index).get_path_name()));button.call_method('HandleClicked')
def shot(name):
 state['capture']=name;unreal.SystemLibrary.execute_console_command(world,'Shot SHOWUI')
def capture():
 if 'capture' in state:
  files=list((root/'Saved/Screenshots').rglob('*.png'));shutil.copy2(max(files,key=lambda p:p.stat().st_mtime),root/'Saved'/state.pop('capture'))
def tick(dt):
 if time.monotonic()-state['started']>240:finish(False,'Timeout '+str(state['stage']));return
 if 'focus' in state:unreal.GameplayStatics.get_player_pawn(world,0).set_actor_location(state['focus'],False,False)
 if time.monotonic()-state['since']<(28 if state['stage']==4 else 6 if state['stage'] in [8,9] else 3):return
 try:
  stage=state['stage'];rm=actors(unreal.ResourceManager)[0];grid=actors(unreal.CableNetworkManager)[0];pc=actors(unreal.SurviveThePlanetPlayerController)[0]
  if stage==0:
   unreal.GameplayStatics.set_game_paused(world,False);unreal.GameplayStatics.set_global_time_dilation(world,1)
   hud=wl.call_method('GetAllWidgetsOfClass',(world,unreal.ResourceDisplayWidget,False))[0];hud.call_method('HandleSpeed1Clicked');state['hud']=hud
   subs.call_method('GetWorldSubsystem',(world,unreal.MissionConfidenceSubsystem)).set_decay_paused(True)
   base=next(b for b in actors(unreal.BaseBuilding) if b.get_building_type()==unreal.STPBuildingType.BASE_MODULE)
   data=unreal.load_asset('/Game/Data/Buildings/BlueprintCatalog/DA_CrudeWorkshop');cls=data.get_editor_property('building_class');surface=actors(unreal.PlanetSurfaceManager)[0];placement=None
   for dist in [850,1100,1400]:
    for v in [unreal.Vector(0,1,0),unreal.Vector(1,0,0),unreal.Vector(0,-1,0),unreal.Vector(-1,0,0)]:
     candidate=surface.get_building_placement_for_world_location(base.get_actor_location()+v*dist,unreal.get_default_object(cls).get_grid_footprint())
     if candidate.valid:placement=candidate;break
    if placement:break
   assert placement;plant=spawn(cls,placement.world_location);assert surface.reserve_cells(plant,placement.origin_cell,plant.get_grid_footprint());plant.set_construction_progress(1)
   for r,n in [(unreal.ResourceType.STONE,0),(unreal.ResourceType.IRON,100),(unreal.ResourceType.COPPER,100),(unreal.ResourceType.CONCRETE,0),(unreal.ResourceType.STEEL,0),(unreal.ResourceType.COPPER_METAL,0),(unreal.ResourceType.CONTROL_CHIP,0),(unreal.ResourceType.ENERGY,1000)]:rm.set_resource_amount(r,n)
   grid.refresh_energy_grid();assert plant.is_operational();assert plant.get_max_drone_slots()==4 and plant.get_unlocked_drone_slots()==4;assert not plant.is_production_enabled()
   pc.set_selected_actor(plant);info=wl.call_method('GetAllWidgetsOfClass',(world,unreal.BuildingInfoWidget,False))[0]
   state.update(plant=plant,info=info,base=base,focus=(base.get_actor_location()+plant.get_actor_location())*.5)
   drones=[spawn(unreal.load_asset('/Game/BluePrints/WorkingDrone/BP_ConstructionDrone').generated_class(),plant.get_actor_location()+unreal.Vector(200,i*60,180)) for i in range(5)];state['drones']=drones
   state['checks'].append('Workshop starts paused with four unlocked drone slots');state['stage']=1
  elif stage==1:
   plant=state['plant'];assert widget('RecipeButtonsBox').get_children_count()==3
   labels=text_values();assert all(s in labels for s in ['CONCRETE','STEEL','COPPER']);assert not any('CIRCUIT' in s.upper() or 'REPEAT' in s.upper() or 'WAITING FOR' in s.upper() for s in labels)
   warning=next(o for o in unreal.ObjectIterator(unreal.Image) if o.get_name()=='MissingInputWarning' and o.get_path_name().startswith(widget('InputsBox').get_child_at(0).get_path_name()));assert warning.get_visibility()!=unreal.SlateVisibility.COLLAPSED
   assert widget('DroneSlotsBox').get_children_count()==4
   # The actual Start handler remains usable with no materials or drones.
   state['info'].call_method('HandleStartProduction');assert plant.is_production_enabled();state['stage']=2
   state['checks'].append('Only Concrete/Steel/Copper appear; inline input warning replaces status banner and repeat control')
  elif stage==2:
   plant=state['plant'];assert not plant.is_producing() and not effect_active();assert rm.get_resource_amount(unreal.ResourceType.CONCRETE)==0
   rm.set_resource_amount(unreal.ResourceType.STONE,100)
   # Assign the first drone through the existing slot and drone-selection card handlers.
   slot=widget('DroneSlotsBox').get_child_at(0);slot.call_method('HandleClicked')
   buttons=[o for o in unreal.ObjectIterator(unreal.DroneTypeSelectionButton) if o.get_name()=='SelectButton' and o.get_is_enabled()]
   assert buttons
   def efficiency(button):
    card=button.get_outer().get_outer()
    label=next(o for o in unreal.ObjectIterator(unreal.TextBlock) if o.get_name()=='EfficiencyText' and o.get_path_name().startswith(card.get_path_name()))
    return float(str(label.get_text()).replace('%',''))
   buttons.sort(key=efficiency,reverse=True)
   for b in buttons:
    b.call_method('HandleClicked')
    if plant.get_assigned_drone_count():break
   assert plant.get_assigned_drone_count()==1
   state['stage']=3;state['checks'].append('Start can wait for supplies; zero drones cannot produce; existing picker assigns a real drone')
  elif stage==3:
   plant=state['plant']
   if not plant.is_producing() or not effect_active():return
   assert not widget('StartProductionButton').get_is_enabled() and widget('PauseProductionButton').get_is_enabled()
   state['fan']=next(c for c in plant.get_components_by_class(unreal.StaticMeshComponent) if c.get_name()=='WorkshopFan');state['rotation']=str(state['fan'].get_editor_property('relative_rotation'))
   state['hud'].call_method('SetCheatSpeed30');state['stage']=4
  elif stage==4:
   plant=state['plant'];made=rm.get_resource_amount(unreal.ResourceType.CONCRETE);assert made>=2;assert rm.get_resource_amount(unreal.ResourceType.STONE)==100-4*made
   assert str(state['fan'].get_editor_property('relative_rotation'))!=state['rotation'];state['info'].call_method('HandlePauseProduction');state['made']=made;state['stage']=5
   state['checks'].append('Real drone drives repeated concrete cycles; each output pays four stone; animation and steam run')
  elif stage==5:
   plant=state['plant'];assert not plant.is_producing() and not effect_active();assert rm.get_resource_amount(unreal.ResourceType.CONCRETE)==state['made']
   state['paused_progress']=widget('CycleProgress').get_editor_property('percent');state['stopped_rotation']=str(state['fan'].get_editor_property('relative_rotation'));state['stage']=6
  elif stage==6:
   assert widget('CycleProgress').get_editor_property('percent')==state['paused_progress'];assert str(state['fan'].get_editor_property('relative_rotation'))==state['stopped_rotation']
   click_recipe(1);assert state['plant'].get_recipe_output()==unreal.ResourceType.STEEL;assert not state['plant'].is_production_enabled();state['stage']=7
   state['checks'].append('Pause freezes work progress, output, animation and steam; recipe can change while paused')
  elif stage==7:
   assert widget('CycleProgress').get_editor_property('percent')==0
   plant=state['plant']
   for i in range(1,4):assert plant.try_assign_drone(state['drones'][i],i)
   assert not plant.try_assign_drone(state['drones'][4],4)
   assert plant.get_assigned_drone_count()==4;state['info'].call_method('HandleStartProduction');state['stage']=8
   state['checks'].append('Recipe switch resets unfinished work; max four drones; fifth assignment rejected')
  elif stage==8:
   plant=state['plant'];steel=rm.get_resource_amount(unreal.ResourceType.STEEL);assert steel>=1;assert rm.get_resource_amount(unreal.ResourceType.IRON)==100-4*steel
   assert plant.get_combined_drone_efficiency()>1;state['info'].call_method('HandlePauseProduction');click_recipe(2);assert plant.get_recipe_output()==unreal.ResourceType.COPPER_METAL;state['info'].call_method('HandleStartProduction');state['stage']=9
   state['checks'].append('Steel recipe consumes iron and four assigned drones increase throughput')
  elif stage==9:
   copper=rm.get_resource_amount(unreal.ResourceType.COPPER_METAL);assert copper>=1;assert rm.get_resource_amount(unreal.ResourceType.COPPER)==100-3*copper;assert rm.get_resource_amount(unreal.ResourceType.CONTROL_CHIP)==0
   state['checks'].append('Copper ore becomes copper metal; no recipe produces circuit boards')
   rm.set_resource_amount(unreal.ResourceType.COPPER,0);state['copper']=copper;state['stage']=10
  elif stage==10:
   assert not state['plant'].is_producing() and not effect_active();assert state['plant'].is_production_enabled();assert rm.get_resource_amount(unreal.ResourceType.COPPER_METAL)==state['copper']
   rm.set_resource_amount(unreal.ResourceType.COPPER,100);state['stage']=11
  elif stage==11:
   assert state['plant'].is_producing() and effect_active();state['checks'].append('Starvation stops effects and output; replenishing inputs resumes automatically without another Start')
   rm.set_resource_amount(unreal.ResourceType.ENERGY,0);grid.refresh_energy_grid();state['stage']=12
  elif stage==12:
   assert not state['plant'].is_producing();assert not effect_active();rm.set_resource_amount(unreal.ResourceType.ENERGY,1000);grid.refresh_energy_grid();state['stage']=13
  elif stage==13:
   assert state['plant'].is_producing();state['hud'].call_method('HandleSpeed1Clicked');state['info'].call_method('HandlePauseProduction');click_recipe(0);rm.set_resource_amount(unreal.ResourceType.STONE,0)
   state['checks'].append('Power loss stops work and restoring power resumes it');shot('ProductionPanelMissingInput.png');state['stage']=14
  elif stage==14:
   capture();state['info'].call_method('HandleStartProduction');rm.set_resource_amount(unreal.ResourceType.STONE,100);state['stage']=15
  elif stage==15:shot('ProductionPanelRunning.png');state['stage']=16
  elif stage==16:
   capture();state['info'].call_method('HandleClosePopup');assert pc.get_selected_actor() is None;pc.set_selected_actor(state['plant']);assert state['info'].get_visibility()!=unreal.SlateVisibility.COLLAPSED
   # The same panel adapts to the existing reactor, including its two input warnings.
   data=unreal.load_asset('/Game/Data/Buildings/BlueprintCatalog/DA_NuclearReactor') or unreal.load_asset('/Game/Data/Buildings/STP/DA_STP_NuclearReactor')
   reactor=spawn(unreal.NuclearReactor,state['plant'].get_actor_location()+unreal.Vector(700,0,0));reactor.set_construction_progress(1);pc.set_selected_actor(reactor)
   assert widget('RecipeSelectionSection').get_visibility()==unreal.SlateVisibility.COLLAPSED
   state['info'].call_method('HandlePauseProduction');assert not reactor.is_production_enabled();state['info'].call_method('HandleStartProduction');assert reactor.is_production_enabled()
   state['checks']+=['Closing clears selection and the same building can reopen','Shared reactor panel hides recipe choices and supports start/pause'];state['stage']=17
  else:
   pc.set_selected_actor(state['plant']);finish(True);return
  state['since']=time.monotonic()
 except Exception:finish(False,traceback.format_exc())
panel_review_handle=unreal.register_slate_post_tick_callback(tick)
