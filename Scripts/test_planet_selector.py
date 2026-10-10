"""PIE checks for authored selection, all landing maps, climate and presentation."""
import json,time,traceback,shutil
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir()).resolve()
assert root==Path(r'C:\UE5\SurviveThePlanet 5.8')
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
save_backups={p:p.read_bytes() for p in (root/'Saved/SaveGames').glob('*.sav')}
library=unreal.EditorAssetLibrary
assets=unreal.AssetToolsHelpers.get_asset_tools()
if not library.does_asset_exist('/Game/Weather/M_WindMote'):
    mat=assets.create_asset('M_WindMote','/Game/Weather',unreal.Material,unreal.MaterialFactoryNew())
    mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('used_with_instanced_static_meshes',True)
    mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
    color=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector,0,0)
    color.set_editor_property('constant',unreal.LinearColor(.3,.4,.45,1))
    alpha=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant,0,150)
    alpha.set_editor_property('r',.18)
    unreal.MaterialEditingLibrary.connect_material_property(color,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(alpha,'',unreal.MaterialProperty.MP_OPACITY)
    unreal.MaterialEditingLibrary.recompile_material(mat);library.save_loaded_asset(mat)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
for material_path in ['/Game/Weather/M_RainStreak','/Game/Weather/M_WindMote']:
    assert unreal.load_asset(material_path).get_editor_property('used_with_instanced_static_meshes'),material_path
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
widgets=unreal.get_default_object(unreal.load_class(None,'/Script/UMG.WidgetBlueprintLibrary'))
levels.load_level('/Game/Planet/Maps/L_PlanetSelection')
profiles=[('Nexaris',71237),('Aridus',98117),('Borealis',42173)]
state={'stage':'selector','index':0,'since':time.monotonic(),'started':time.monotonic(),'checks':[]}

def report(passed,error=None):
    for p,data in save_backups.items():p.write_bytes(data)
    value={'passed':passed,'checks':state['checks'],'error':error}
    (root/'Saved/PlanetSelectorRuntime.json').write_text(json.dumps(value,indent=2))
    unreal.log('PLANET_RUNTIME '+json.dumps(value))

def get_widgets(world,cls):return widgets.call_method('GetAllWidgetsOfClass',(world,cls,False))
def capture(world,label):
    unreal.SystemLibrary.execute_console_command(world,'Shot SHOWUI')
    state['capture']=label
def preserve_capture():
    candidates=list((root/'Saved/Screenshots').rglob('*.png'))
    if candidates and state.get('capture'):
        shutil.copy2(max(candidates,key=lambda p:p.stat().st_mtime),root/'Saved'/('Planet_'+state.pop('capture')+'.png'))

def tick(delta):
    now=time.monotonic();world=editor.get_game_world()
    if now-state['started']>180:
        report(False,'Runtime checks timed out');unreal.unregister_slate_post_tick_callback(handle);return
    if now-state['since']<2:return
    try:
        stage=state['stage'];name,seed=profiles[state['index']]
        if stage=='selector':
            if not world:return
            selectors=get_widgets(world,unreal.PlanetSelectorWidget)
            if not selectors:return
            selector=selectors[0]
            for n,_ in profiles:
                selector.call_method('Select'+n)
                assert str(selector.get_editor_property('PlanetName').get_text())==n.upper()
                assert str(unreal.load_asset('/Game/Data/Planet/DA_'+n).get_editor_property('climate_description')) in str(selector.get_editor_property('PlanetDetails').get_text())
                assert selector.get_editor_property('LandButton').get_is_enabled()
            selector.call_method('Select'+name)
            state['checks'].append('Planet buttons update details and enable landing: '+name)
            if state['index']==0:capture(world,'Selector')
            state['selector']=selector;state['stage']='land'
        elif stage=='land':
            preserve_capture();state['selector'].call_method('Land');state['stage']='world'
        elif stage=='world':
            if not world or ('L_'+name) not in world.get_path_name():return
            managers=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.PlanetWeatherManager)
            assert len(managers)==1
            weather=managers[0];planet=weather.get_planet_definition()
            assert planet.get_editor_property('seed')==seed
            assert str(planet.get_editor_property('display_name'))==name.upper()
            population=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.SectorPopulation)[0]
            assert population.get_editor_property('seed')==seed
            resources=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.ResourceManager)[0]
            assert resources.get_credits()==planet.get_editor_property('starting_credits')
            day=planet.get_editor_property('day_length_hours')*60
            assert str(weather.get_day_phase(0))=='Night'
            assert str(weather.get_day_phase(day*.5))=='Day'
            assert {'Night','Dawn','Day','Dusk'}=={str(weather.get_day_phase(t)) for t in range(0,int(day),5)}
            weather.restart_simulation();a=weather.get_current_weather()
            weather.restart_simulation();b=weather.get_current_weather()
            assert (a.precipitation_percent,a.wind_percent,a.sun_percent)==(b.precipitation_percent,b.wind_percent,b.sun_percent)
            state['weather']=weather;state['world']=world
            hud=get_widgets(world,unreal.ResourceDisplayWidget)[0]
            assert unreal.GameplayStatics.get_global_time_dilation(world)<.001
            hud.call_method('HandleSpeed1Clicked')
            assert abs(unreal.GameplayStatics.get_global_time_dilation(world)-1)<.01
            weather.set_weather_immediately(unreal.PlanetWeatherState(precipitation_percent=10,wind_percent=20,sun_percent=100))
            assert weather.get_current_weather().sun_percent<=40.01
            weather.update_presentation(day*.5)
            state['checks'].append('Landed on '+name+' with matching definition, terrain seed, economy, deterministic weather and four solar phases')
            state['stage']='rain'
        elif stage=='rain':
            weather=state['weather'];world=state['world']
            components=weather.get_components_by_class(unreal.InstancedStaticMeshComponent)
            rain=next(c for c in components if c.get_instance_count()==600)
            wind=next(c for c in components if c.get_instance_count()==120)
            visible=sum(c.scale3d.z>0 for c in [rain.get_instance_transform(i,True) for i in range(600)])
            assert visible==450,visible
            assert wind.get_instance_transform(0,True).scale3d.x>0
            sun=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.DirectionalLight)[0].get_component_by_class(unreal.DirectionalLightComponent)
            weather.update_presentation(unreal.load_asset('/Game/Data/Planet/DA_'+name).get_editor_property('day_length_hours')*30)
            day_intensity=sun.get_editor_property('intensity');assert day_intensity>.1
            weather.update_presentation(0);assert sun.get_editor_property('intensity')==0
            state['checks'].append('Rain intensity drives 450 visible streaks; wind motes visible; daytime sunlight changes to zero direct light at night: '+name)
            if state['index']==0:
                get_widgets(world,unreal.ResourceDisplayWidget)[0].call_method('HandlePauseTimeClicked')
                weather.update_presentation(weather.get_planet_definition().get_editor_property('day_length_hours')*30)
                capture(world,'Rain')
            state['stage']='stop'
        elif stage=='stop':
            preserve_capture();unreal.EditorLevelLibrary.editor_end_play();state['stage']='next'
        elif stage=='next':
            if world:return
            state['index']+=1
            if state['index']>=len(profiles):
                report(True);unreal.unregister_slate_post_tick_callback(handle)
                levels.load_level('/Game/Planet/Maps/L_PlanetSelection');return
            levels.load_level('/Game/Planet/Maps/L_PlanetSelection');levels.editor_request_begin_play();state['stage']='selector'
        state['since']=now
    except Exception:
        report(False,traceback.format_exc());unreal.unregister_slate_post_tick_callback(handle)

handle=unreal.register_slate_post_tick_callback(tick)
levels.editor_request_begin_play()
