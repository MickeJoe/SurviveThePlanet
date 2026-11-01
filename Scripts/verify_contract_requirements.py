import unreal,time,json,traceback
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
unreal.get_default_object(unreal.load_class(None,'/Script/PythonScriptPlugin.EditorPythonScriptingLibrary')).call_method('SetKeepPythonScriptAlive',(True,))
levels.editor_request_begin_play();start=time.monotonic();phase=0;checks=[]
def finish(error=None):
 unreal.unregister_slate_post_tick_callback(handle)
 (root/'Saved/ContractRequirementsPreview.json').write_text(json.dumps({'passed':error is None,'checks':checks,'error':error},indent=2))
 levels.editor_request_end_play();unreal.SystemLibrary.quit_editor()
def tick(delta):
 global phase,start,widget
 if time.monotonic()-start<4:return
 try:
  world=editor.get_game_world()
  if not world:return
  pc=unreal.GameplayStatics.get_player_controller(world,0)
  if not pc:return
  if phase==0:
   statics=unreal.get_default_object(unreal.GameplayStatics)
   t=unreal.Transform(location=unreal.Vector(0,0,-10000))
   cargo=statics.call_method('BeginDeferredActorSpawnFromClass',(world,unreal.CargoBay,t,unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
   cargo=statics.call_method('FinishSpawningActor',(cargo,t,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT));cargo.set_construction_progress(1)
   contracts=next(x for x in unreal.ObjectIterator(unreal.ContractOfferSubsystem) if x.get_outer()==world)
   offers=contracts.get_contract_offers_for_trader('helix_industrial')
   assert offers
   resource_name=next(str(d.display_name) for d in unreal.ResourceCatalog.get_resource_definitions() if str(d.widget_prefix)==str(offers[0].item_id))
   library=unreal.get_default_object(unreal.load_class(None,'/Script/UMG.WidgetBlueprintLibrary'))
   cls=unreal.load_class(None,'/Game/UI/Contracts/WBP_ContractOffer.WBP_ContractOffer_C')
   widget=library.call_method('Create',(world,cls,pc));widget.add_to_viewport(2000)
   assert widget.open_for_trader('helix_industrial')
   texts=[str(x.get_text()) for x in unreal.ObjectIterator(unreal.TextBlock) if x.get_outer().get_outer()==widget]
   assert not any('Reputation' in t for t in texts)
   assert all(f'{tier.delivery_amount} {resource_name}' in texts for tier in offers[0].tiers),texts
   assert any('Circuit Boards' in t for t in texts),texts
   checks.extend(['All tiers show resource name and exact delivery amount','Legacy reputation effects absent','Resource rewards use player-facing names'])
   unreal.GameplayStatics.set_game_paused(world,True)
   phase=1;start=time.monotonic()
  elif phase==1:
   unreal.SystemLibrary.execute_console_command(world,'shot showui',pc)
   phase=2;start=time.monotonic()
  else:finish()
 except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
