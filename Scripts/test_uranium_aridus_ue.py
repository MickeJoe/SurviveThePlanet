import unreal,time,json,traceback
from pathlib import Path
root=Path('C:/UE5/SurviveThePlanet 5.8');unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Planet/Maps/L_Aridus')
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
def tick(dt):
 try:
  world=editor.get_game_world()
  if not world:return
  pops=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.SectorPopulation)
  if not pops or not pops[0].resources.deposits:return
  pop=pops[0];weather=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.PlanetWeatherManager)[0]
  assert 'Aridus' in weather.get_planet_definition().get_name()
  assert all(d.resource_type!=unreal.ResourceType.URANIUM_ORE for d in pop.resources.deposits)
  assert not unreal.GameplayStatics.get_all_actors_of_class(world,unreal.UraniumResourceSource)
  assert unreal.GameplayStatics.get_all_actors_of_class(world,unreal.GeothermalSite),'Aridus start geothermal site regression'
  report={'passed':True,'checks':['Aridus has no uranium deposits or uranium actors','Aridus geothermal start site still spawns']}
 except Exception:report={'passed':False,'error':traceback.format_exc()}
 (root/'Saved/UraniumAridusReview.json').write_text(json.dumps(report,indent=2));unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
