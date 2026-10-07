import unreal,time,json,traceback
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
unreal.get_default_object(unreal.load_class(None,'/Script/PythonScriptPlugin.EditorPythonScriptingLibrary')).call_method('SetKeepPythonScriptAlive',(True,))
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
started=time.monotonic();phase=0;checks=[]
def totals(costs):return {str(c.resource):c.cost for c in costs}
def finish(error=None):
 unreal.unregister_slate_post_tick_callback(handle)
 if 'world' in globals():unreal.GameplayStatics.set_game_paused(world,False)
 report={'passed':error is None,'checks':checks,'error':error}
 (root/'Saved/BuildCostFlickerTest.json').write_text(json.dumps(report,indent=2))
 unreal.log('BUILD_COST_FLICKER_TEST '+json.dumps(report))
def tick(delta):
 global world,controller,widget,row,preview,before,children,started,phase
 if time.monotonic()-started<2:return
 try:
  world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
  if not world:
   if time.monotonic()-started>40:raise AssertionError('No PIE world')
   return
  if phase==0:
   controller=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.SurviveThePlanetPlayerController)[0]
   widget=next(w for w in unreal.ObjectIterator(unreal.BuildCostWidget) if w.get_owning_player()==controller and w.get_parent())
   controller.set_active_build_tool(unreal.STPBuildTool.ENERGY_EXTENDER)
   phase=1;started=time.monotonic()
  elif phase==1:
   row=next(r for r in unreal.ObjectIterator(unreal.HorizontalBox) if r.get_name()=='CostRow' and r.get_outer().get_outer()==widget)
   children=[row.get_child_at(i) for i in range(row.get_children_count())]
   preview=controller.get_active_placement_preview();assert preview
   before=totals(controller.get_build_costs(unreal.STPBuildTool.ENERGY_EXTENDER,True))
   assert any('CONNECTOR' in key for key in before),before
   unreal.GameplayStatics.set_game_paused(world,True)
   position=preview.get_actor_location();position.x+=10000
   preview.set_actor_location(position,False,False)
   phase=2;started=time.monotonic()
  elif phase==2:
   after=controller.get_build_costs(unreal.STPBuildTool.ENERGY_EXTENDER,True)
   assert totals(after)!=before,'Moving extender did not change connector cost'
   assert children==[row.get_child_at(i) for i in range(row.get_children_count())],'Cost widgets recreated during movement'
   for i,cost in enumerate(after):
    assert str(row.get_child_at(i*2+1).get_text()).replace(',','').replace(' ','')==str(cost.cost)
   size=widget.get_desired_size();assert size.y<100
   checks.extend(['Moving extender changes total cost','Existing icons and number widgets preserved','Displayed numbers match new total','Popup remains compact'])
   controller.set_active_build_tool(unreal.STPBuildTool.NONE)
   finish()
 except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
