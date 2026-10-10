"""Author the first playable planet profiles and UMG system map. Run in UE5.8."""
import json, math
from pathlib import Path
import unreal
from editor_toolset.toolsets.object import ObjectTools
root=Path(unreal.Paths.project_dir()).resolve()
assert root==Path(r'C:\UE5\SurviveThePlanet 5.8')
assert not unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
assets=unreal.AssetToolsHelpers.get_asset_tools()
library=unreal.EditorAssetLibrary
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
umg=unreal.get_default_object(unreal.UMGToolSet)

def props(obj,values):
    schema=json.loads(ObjectTools.list_properties(obj))
    keys={key.lower().replace('_',''):key for key in schema}
    resolved={keys[key.lower().replace('_','')]:value for key,value in values.items()}
    ObjectTools.get_properties(obj,list(resolved))
    assert ObjectTools.set_properties(obj,json.dumps(resolved)),(obj.get_name(),resolved)

def add(bp,cls,name,parent=None,variable=False):
    info=umg.call_method('AddWidget',(bp,cls,name,parent,-1))
    assert info.widget,name
    ObjectTools.list_properties(info.widget)
    if info.slot:ObjectTools.list_properties(info.slot)
    if variable:umg.call_method('ToggleWidgetAsVariable',(bp,info.widget,True))
    return info.widget

def place(w,x,y,width,height):
    props(w.slot,{'layoutData':{'offsets':{'left':x,'top':y,'right':width,'bottom':height},'anchors':{'minimum':{'x':0,'y':0},'maximum':{'x':0,'y':0}},'alignment':{'x':0,'y':0}},'bAutoSize':False})

WHITE={'r':.85,'g':.95,'b':1,'a':1}
CYAN={'r':.03,'g':.7,'b':.85,'a':1}
def text(bp,name,parent,value,x,y,w,h,size=18,color=WHITE,variable=False):
    node=add(bp,unreal.TextBlock,name,parent,variable)
    font=json.loads(ObjectTools.get_properties(node,['font']))['font'];font['size']=size
    props(node,{'font':font,'colorAndOpacity':{'specifiedColor':color},'visibility':'HitTestInvisible'})
    node.set_text(value)
    if isinstance(node.slot,unreal.CanvasPanelSlot):place(node,x,y,w,h)
    return node

def panel(bp,name,parent,x,y,w,h,color):
    node=add(bp,unreal.Border,name,parent)
    props(node,{'brushColor':color,'padding':{'left':0,'top':0,'right':0,'bottom':0}})
    place(node,x,y,w,h);return node

def button(bp,name,parent,label,x,y,w,h,color):
    node=add(bp,unreal.Button,name,parent,True)
    props(node,{'backgroundColor':color,'isFocusable':True});place(node,x,y,w,h)
    caption=text(bp,name+'Label',node,label,0,0,0,0,18)
    props(caption.slot,{'horizontalAlignment':'HAlign_Center','verticalAlignment':'VAlign_Center'})
    return node

def image(bp,name,parent,x,y,w,h,texture):
    node=add(bp,unreal.Image,name,parent)
    props(node,{'visibility':'HitTestInvisible'});place(node,x,y,w,h)
    node.set_brush_from_texture(texture,False);return node

profiles=[
    ('Nexaris','Temperate world. Mild winds, alternating clear skies and rain. Suitable for the first colony.',[1,10],[2,10],[25,100],.55,18,24,35,10,71237,unreal.LinearColor(.08,.5,.65,1)),
    ('Aridus','Arid world. Strong sunlight, gusty winds and rare rain. Water collection is unreliable.',[4,22],[.3,2],[75,100],.08,38,30,20,5,98117,unreal.LinearColor(.7,.28,.08,1)),
    ('Borealis','Cool maritime world. Frequent rain, strong winds and limited direct sunlight.',[7,25],[3,16],[10,65],.8,6,36,55,-12,42173,unreal.LinearColor(.3,.5,.75,1)),
]
original=library.load_asset('/Game/Data/Planet/DA_PlanetDefinition')
assert original
created=[]
for name,description,wind,rain,sun,chance,temp,day,lat,season,seed,color in profiles:
    path='/Game/Data/Planet/DA_'+name
    planet=library.load_asset(path) if library.does_asset_exist(path) else library.duplicate_asset(original.get_path_name(),path)
    assert planet
    planet.set_editor_property('display_name',name.upper())
    planet.set_editor_property('climate_description',description)
    planet.set_editor_property('planet_color',color)
    planet.set_editor_property('seed',seed)
    planet.set_editor_property('rain_probability',chance)
    planet.set_editor_property('mean_temperature_celsius',temp)
    planet.set_editor_property('day_length_hours',day)
    planet.set_editor_property('landing_latitude_degrees',lat)
    planet.set_editor_property('solar_declination_degrees',season)
    weather=planet.get_editor_property('weather')
    for key,values in [('wind_range',wind),('precipitation_range',rain),('sun_range',sun)]:
        r=weather.get_editor_property(key);r.set_editor_property('minimum',values[0]);r.set_editor_property('maximum',values[1]);weather.set_editor_property(key,r)
    weather.set_editor_property('sun_penalty_per_mm_of_rain',6)
    planet.set_editor_property('weather',weather)
    map_path='/Game/Planet/Maps/L_'+name
    library.save_loaded_asset(planet)
    if not library.does_asset_exist(map_path):
        world=unreal.EditorLoadingAndSavingUtils.new_map_from_template('/Game/WorldGeneration/Maps/L_PlanetClusters',False)
        assert world
        assert unreal.EditorLoadingAndSavingUtils.save_map(world,map_path)
    else:levels.load_level(map_path)
    planet.set_editor_property('landing_map',unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world())
    library.save_loaded_asset(planet)
    weather_actors=[a for a in actors.get_all_level_actors() if isinstance(a,unreal.PlanetWeatherManager)]
    assert len(weather_actors)==1
    weather_actors[0].set_editor_property('planet_definition',planet)
    weather_actors[0].set_actor_label(name+' - Climate')
    for a in actors.get_all_level_actors():
        if isinstance(a,unreal.SectorPopulation):a.set_editor_property('seed',seed)
    assert levels.save_current_level()
    created.append({'planet':path,'map':map_path,'seed':seed})

mat_path='/Game/Weather/M_RainStreak'
mat=library.load_asset(mat_path) if library.does_asset_exist(mat_path) else assets.create_asset('M_RainStreak','/Game/Weather',unreal.Material,unreal.MaterialFactoryNew())
mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property('used_with_instanced_static_meshes',True)
mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
color=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector,0,0)
color.set_editor_property('constant',unreal.LinearColor(.7,1.0,1.3,1))
rain_alpha=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant,0,150)
rain_alpha.set_editor_property('r',.35)
unreal.MaterialEditingLibrary.connect_material_property(rain_alpha,'',unreal.MaterialProperty.MP_OPACITY)
unreal.MaterialEditingLibrary.connect_material_property(color,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
unreal.MaterialEditingLibrary.recompile_material(mat);library.save_loaded_asset(mat)

if not library.does_asset_exist('/Game/Weather/M_WindMote'):
    wind_mat=assets.create_asset('M_WindMote','/Game/Weather',unreal.Material,unreal.MaterialFactoryNew())
    wind_mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
    wind_mat.set_editor_property('used_with_instanced_static_meshes',True)
    wind_mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
    wind_color=unreal.MaterialEditingLibrary.create_material_expression(wind_mat,unreal.MaterialExpressionConstant3Vector,0,0)
    wind_color.set_editor_property('constant',unreal.LinearColor(.3,.4,.45,1))
    wind_alpha=unreal.MaterialEditingLibrary.create_material_expression(wind_mat,unreal.MaterialExpressionConstant,0,150)
    wind_alpha.set_editor_property('r',.18)
    unreal.MaterialEditingLibrary.connect_material_property(wind_color,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(wind_alpha,'',unreal.MaterialProperty.MP_OPACITY)
    unreal.MaterialEditingLibrary.recompile_material(wind_mat);library.save_loaded_asset(wind_mat)

textures={}
for key in ['System','Nexaris','Aridus','Borealis']:
    task=unreal.AssetImportTask();task.filename=str(root/'ContentSource/Planet'/('T_'+key+'.png'))
    task.destination_path='/Game/UI/Planet/Art';task.destination_name='T_'+key;task.automated=True;task.save=True
    assets.import_asset_tasks([task])
    texture=library.load_asset('/Game/UI/Planet/Art/T_'+key);assert texture
    texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    library.save_loaded_asset(texture);textures[key]=texture

path='/Game/UI/Planet/WBP_PlanetSelector'
bp=library.load_asset(path) if library.does_asset_exist(path) else None
if not bp:
    factory=unreal.WidgetBlueprintFactory();factory.set_editor_property('parent_class',unreal.PlanetSelectorWidget)
    bp=assets.create_asset('WBP_PlanetSelector','/Game/UI/Planet',unreal.WidgetBlueprint,factory)
for node in reversed(umg.call_method('GetWidgets',(bp,)).widgets):
    if node.widget:umg.call_method('RemoveWidget',(bp,node.widget))
scale=add(bp,unreal.ScaleBox,'ScreenScale')
props(scale,{'stretch':'ScaleToFit','stretchDirection':'Both'})
size=add(bp,unreal.SizeBox,'DesignSize',scale)
props(size,{'widthOverride':1600,'heightOverride':900,'bOverride_WidthOverride':True,'bOverride_HeightOverride':True})
canvas=add(bp,unreal.CanvasPanel,'SystemCanvas',size)
image(bp,'SystemBackground',canvas,0,0,1600,900,textures['System'])
text(bp,'Title',canvas,'PLANETARY SYSTEM',55,36,800,60,36)
text(bp,'Subtitle',canvas,'VYRON SYSTEM  /  COLONISATION PROTOTYPE',58,95,800,40,17,CYAN)
text(bp,'Instructions',canvas,'Select a planet to inspect its climate',58,820,900,45,19,CYAN)
text(bp,'BaseTitle',canvas,'MOTHER BASE',480,430,300,45,23,CYAN)
for name,x,y in [('Nexaris',600,150),('Aridus',165,430),('Borealis',720,590)]:
    planet_button=button(bp,name+'Button',canvas,name.upper(),x-5,y,195,240,CYAN)
    props(planet_button.get_content().slot,{'verticalAlignment':'VAlign_Bottom','padding':{'left':0,'top':0,'right':0,'bottom':10}})
    image(bp,name+'Orb',canvas,x,y,185,185,textures[name])
details=panel(bp,'DetailsPanel',canvas,1100,110,440,740,{'r':.025,'g':.055,'b':.075,'a':.98})
dc=add(bp,unreal.CanvasPanel,'DetailsCanvas',details)
text(bp,'PlanetName',dc,'NEXARIS',25,22,390,50,32,CYAN,True)
info=text(bp,'PlanetDetails',dc,'',25,90,390,550,16,WHITE,True)
props(info,{'autoWrapText':True})
button(bp,'LandButton',dc,'LAND & START COLONY',25,670,390,48,CYAN)
text(bp,'PrototypeNotice',canvas,'FIRST EXPEDITION / THREE PLAYABLE WORLDS',1100,860,440,30,14)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
assert library.save_loaded_asset(bp)

selection_map='/Game/Planet/Maps/L_PlanetSelection'
if library.does_asset_exist(selection_map):levels.load_level(selection_map)
else:levels.new_level(selection_map)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode',unreal.PlanetSelectionGameMode)
assert levels.save_current_level()
(root/'Saved/PlanetSelectorBuild.json').write_text(json.dumps({'created':created,'selector':path,'map':selection_map},indent=2))
unreal.log('PLANET_SELECTOR_ASSETS_CREATED')
