import unreal,time,json,traceback,shutil
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();assert root==Path('C:/UE5/SurviveThePlanet 5.8')
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem);levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
levels.load_level('/Game/Planet/Maps/L_Aridus')
state={'stage':0,'since':time.monotonic(),'started':time.monotonic(),'checks':[]}
spawner=unreal.get_default_object(unreal.GameplayStatics)
def spawn(world,cls,loc):
 t=unreal.Transform(location=loc)
 a=spawner.call_method('BeginDeferredActorSpawnFromClass',(world,cls,t,unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
 return spawner.call_method('FinishSpawningActor',(a,t,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
def report(passed,error=None):
 (root/'Saved/GeothermalRuntime.json').write_text(json.dumps({'passed':passed,'checks':state['checks'],'error':error},indent=2))
 unreal.unregister_slate_post_tick_callback(handle)
 if passed:
  exec(compile((root/'Scripts/verify_geothermal_visuals.py').read_text(),str(root/'Scripts/verify_geothermal_visuals.py'),'exec'),globals())
def tick(dt):
 now=time.monotonic()
 if now-state['started']>120:report(False,'Timeout');return
 if now-state['since']<3:return
 try:
  world=editor.get_game_world()
  if not world:return
  if state['stage']==0:
   sites=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.GeothermalSite)
   assert sites,'Aridus must generate a starting geothermal site'
   surface=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.PlanetSurfaceManager)[0]
   grid=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.HexSectorGrid)[0]
   site=next(s for s in sites if grid.get_sector_at_world_location(s.get_actor_location())==grid.get_editor_property('starting_sector_id'))
   assert site.is_available()
   cls=unreal.load_class(None,'/Game/Geothermal/BP_GeothermalPlant.BP_GeothermalPlant_C')
   ghost=spawn(world,cls,unreal.Vector(9000,9000,0))
   footprint=ghost.get_grid_footprint();assert footprint==unreal.IntPoint(3,3),str(footprint)
   target=surface.get_building_placement_for_world_location(site.get_actor_location(),footprint)
   assert target.valid,'Starting site must have terrain and building clearance'
   assert surface.can_build_in_sector(ghost,target.world_location),'Preview accepts uncovered geothermal site'
   assert not surface.can_build_in_sector(ghost,target.world_location+unreal.Vector(500,0,0)),'Ordinary ground rejects geothermal plant'
   state['checks'].append('Aridus starting site; smaller 3x3 footprint; geothermal-only placement')
   other=spawn(world,unreal.BaseBuilding,unreal.Vector(9500,9500,0))
   assert not surface.can_build_in_sector(other,target.world_location),'Non-geothermal construction must preserve the resource'
   other.destroy_actor()
   plant=spawn(world,cls,target.world_location)
   assert site.get_occupant()==plant and not site.is_available(),'Plant claims geothermal site'
   assert surface.reserve_cells(plant,target.origin_cell,footprint),'Final reservation must accept claimed site'
   assert not surface.can_build_in_sector(ghost,target.world_location),'Occupied site blocks duplicate plant'
   plant.set_construction_progress(1)
   rotor=plant.get_editor_property('turbine_rotor');state['rotor_start']=float(rotor.get_editor_property('relative_rotation').roll)
   unreal.GameplayStatics.set_game_paused(world,False)
   unreal.GameplayStatics.set_global_time_dilation(world,1.0)
   state['site']=site;state['plant']=plant;state['ghost']=ghost;state['surface']=surface
   state['checks'].append('Final reservation, unique ownership and other-building protection')
   # Reveal more sectors to verify seeded non-start occurrence.
   population=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.SectorPopulation)[0]
   for sector in grid.get_editor_property('sectors'):population.reveal_sector(sector.id)
   assert len(unreal.GameplayStatics.get_all_actors_of_class(world,unreal.GeothermalSite))>1,'Aridus frequency must produce multiple resource sites'
   state['checks'].append('Planet frequency produces additional geothermal sites')
   state['stage']=1
  elif state['stage']==1:
   plant=state['plant'];site=state['site'];surface=state['surface']
   assert len(site.get_components_by_class(unreal.NiagaraComponent))==2,'Site Niagara exhausts created'
   assert len(plant.get_components_by_class(unreal.NiagaraComponent))==2,'Plant Niagara exhausts created'
   assert plant.is_operational(),'Completed plant uses the existing automatic power connection'
   rotation=float(plant.get_editor_property('turbine_rotor').get_editor_property('relative_rotation').roll)
   assert abs(rotation-state['rotor_start'])>.01,('Operational turbine rotates',state['rotor_start'],rotation,plant.is_actor_tick_enabled())
   state['checks'].append('Completed plant connects to power and turbine animation advances')
   # Operational power behavior is unchanged: rotor/steam stop while disconnected.
   state['checks'].append('Site/plant steam and animated heat presentation initialized')
   surface.release_cells(plant);plant.destroy_actor()
   assert site.is_available(),'Resource becomes reusable after plant removal'
   assert surface.can_build_in_sector(state['ghost'],site.get_actor_location()),'Reusable site accepts replacement plant'
   state['ghost'].destroy_actor()
   # Leave a working plant for visual review, without saving the test world.
   cls=unreal.load_class(None,'/Game/Geothermal/BP_GeothermalPlant.BP_GeothermalPlant_C')
   placement=surface.get_building_placement_for_world_location(site.get_actor_location(),unreal.IntPoint(3,3))
   plant=spawn(world,cls,placement.world_location);surface.reserve_cells(plant,placement.origin_cell,plant.get_grid_footprint());plant.set_construction_progress(1)
   pc=unreal.GameplayStatics.get_player_controller(world,0)
   pawn=unreal.GameplayStatics.get_player_pawn(world,0)
   if pawn:pawn.set_actor_location(site.get_actor_location()+unreal.Vector(-350,0,0),False,False)
   unreal.SystemLibrary.execute_console_command(world,'Shot SHOWUI')
   state['checks'].append('Resource is reusable after building removal')
   state['stage']=2
  else:
   screenshots=list((root/'Saved/Screenshots').rglob('*.png'))
   if screenshots:shutil.copy2(max(screenshots,key=lambda p:p.stat().st_mtime),root/'Saved/GeothermalPreview.png')
   report(True)
  state['since']=now
 except Exception:report(False,traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)

