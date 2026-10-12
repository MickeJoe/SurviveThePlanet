import unreal,json,time,traceback
from pathlib import Path
from editor_toolset.toolsets.object import ObjectTools
root=Path('C:/UE5/SurviveThePlanet 5.8')
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
umg=unreal.get_default_object(unreal.UMGToolSet)
bp=unreal.load_asset('/Game/UI/Development/WBP_CheatMenu')
nodes={str(n.widget_name):n.widget for n in umg.call_method('GetWidgets',(bp,)).widgets if n.widget}
column=next(w for w in nodes.values() if isinstance(w,unreal.VerticalBox))
if 'BlueprintSearchBox' not in nodes:
 combo=nodes.get('BlueprintComboBox');index=column.get_child_index(combo) if combo else -1
 info=umg.call_method('AddWidget',(bp,unreal.EditableTextBox,'BlueprintSearchBox',column,index));assert info.widget
 umg.call_method('ToggleWidgetAsVariable',(bp,info.widget,True))
 info.widget.set_hint_text('Search blueprints...')
assert umg.call_method('CompileWidgetBlueprint',(bp,));assert unreal.EditorAssetLibrary.save_loaded_asset(bp)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Planet/Maps/L_Borealis')
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
test_state={}
def tick(dt):
 world=editor.get_game_world()
 if not world:return
 try:
  pc=unreal.GameplayStatics.get_player_controller(world,0)
  library=unreal.get_default_object(unreal.load_class(None,'/Script/UMG.WidgetBlueprintLibrary'))
  if 'widget' not in test_state:
   w=library.call_method('Create',(world,bp.generated_class(),pc));w.add_to_viewport();test_state['widget']=w;test_state['since']=time.monotonic();return
  if time.monotonic()-test_state['since']<1:return
  w=test_state['widget']
  combo=w.get_editor_property('blueprint_combo_box');search=w.get_editor_property('blueprint_search_box');button=w.get_editor_property('grant_blueprint_button')
  options=lambda:[combo.get_option_at_index(i) for i in range(combo.get_option_count())]
  all_options=options();assert len(all_options)>10;assert all_options==sorted(all_options,key=str.casefold)
  def filter_text(value):
   search.set_text(value)
   w.call_method('FilterBlueprints',(unreal.Text(value),))
  filter_text('nuclear');matches=options();assert matches and all('nuclear' in n.lower() for n in matches)
  filter_text('NUCLEAR');assert options()==matches
  filter_text('xyz_no_blueprint');assert not options() and not button.get_is_enabled()
  filter_text('');assert options()==all_options and button.get_is_enabled()
  result={'passed':True,'total':len(all_options),'nuclear_matches':matches,'checks':['Alphabetical list','Case-insensitive live substring filter','No matches disables granting','Clearing filter restores list']}
  w.remove_from_parent()
 except Exception:result={'passed':False,'error':traceback.format_exc()}
 (root/'Saved/CheatFilterReview.json').write_text(json.dumps(result,indent=2));unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
