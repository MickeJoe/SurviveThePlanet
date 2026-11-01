import unreal,time,json,traceback
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
library=unreal.get_default_object(unreal.load_class(None,'/Script/UMG.WidgetBlueprintLibrary'))
unreal.get_default_object(unreal.load_class(None,'/Script/PythonScriptPlugin.EditorPythonScriptingLibrary')).call_method('SetKeepPythonScriptAlive',(True,))
levels.editor_request_begin_play();phase=0;start=time.monotonic();checks=[]
def finish(error=None):
 unreal.unregister_slate_post_tick_callback(handle)
 (root/'Saved/ContractRefreshTest.json').write_text(json.dumps({'passed':error is None,'checks':checks,'error':error},indent=2))
 levels.editor_request_end_play();unreal.SystemLibrary.quit_editor()
def texts():
 return [x for x in unreal.ObjectIterator(unreal.TextBlock) if x.get_outer().get_outer()==panel]
def tick(delta):
 global phase,start,panel,contracts,resources,resource,tier,status,parent,button,initial_widgets,changes
 if time.monotonic()-start<3:return
 try:
  world=editor.get_game_world()
  if not world:return
  pc=unreal.GameplayStatics.get_player_controller(world,0)
  if not pc:return
  if phase==0:
   statics=unreal.get_default_object(unreal.GameplayStatics);t=unreal.Transform(location=unreal.Vector(0,0,-10000))
   cargo=statics.call_method('BeginDeferredActorSpawnFromClass',(world,unreal.CargoBay,t,unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
   cargo=statics.call_method('FinishSpawningActor',(cargo,t,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT));cargo.set_construction_progress(1)
   contracts=next(x for x in unreal.ObjectIterator(unreal.ContractOfferSubsystem) if x.get_outer()==world)
   offer=contracts.get_contract_offers_for_trader('helix_industrial')[0];tier=offer.tiers[0]
   resource=next(d.resource_type for d in unreal.ResourceCatalog.get_resource_definitions() if str(d.widget_prefix)==str(offer.item_id))
   resources=unreal.GameplayStatics.get_actor_of_class(world,unreal.ResourceManager);resources.set_resource_amount(resource,500)
   assert contracts.toggle_contract_selection(offer.id,tier.id)
   assert contracts.confirm_selected_contract()
   panel=library.call_method('GetAllWidgetsOfClass',(world,unreal.TraderPanelWidget,False))[0]
   phase=1;start=time.monotonic()
  elif phase==1:
   status=next(x for x in texts() if str(x.get_text())==f'0 / {tier.delivery_count} DELIVERIES')
   parent=status.get_parent()
   button=next(x for x in parent.get_all_children() if isinstance(x,unreal.Button))
   initial_widgets={x.get_path_name() for x in texts()};changes=0
   phase=2;start=time.monotonic()
  elif phase==2:
   assert status.get_parent()==parent and status in parent.get_all_children(),'Delivery row was rebuilt'
   assert button in parent.get_all_children(),'Delivery button was rebuilt'
   assert {x.get_path_name() for x in texts()}==initial_widgets,'Text widgets reallocated on timer/resource update'
   resources.add_resource(resource,1);changes+=1
   assert any(str(x.get_text())==f'{500+changes} / {tier.delivery_amount}' for x in texts()),'Inventory count did not update'
   if changes>=3:
    checks.extend(['Same delivery texts and button survive periodic refreshes','Repeated resource changes update values without rebuilding trader text'])
    assert contracts.deliver_selected_contract()
    phase=3
   start=time.monotonic()
  elif phase==3:
   assert status.get_parent()==parent and button in parent.get_all_children()
   assert str(status.get_text())==f'1 / {tier.delivery_count} DELIVERIES',str(status.get_text())
   checks.append('Shipment completion updates delivery count in the same widgets')
   unreal.SystemLibrary.execute_console_command(world,'shot showui',pc)
   phase=4;start=time.monotonic()
  else:finish()
 except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
