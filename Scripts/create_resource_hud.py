"""Run in the canonical UE 5.8 editor's Python console after compiling C++."""
import unreal
import json
from pathlib import Path
from editor_toolset.toolsets.object import ObjectTools

root=Path(unreal.Paths.project_dir()).resolve()
assert root == Path(r'C:\UE5\SurviveThePlanet 5.8')
umg=unreal.get_default_object(unreal.UMGToolSet)
bp=unreal.load_asset('/Game/UI/WBP_ResourceDisplay')
definitions=unreal.ResourceCatalog.get_resource_definitions()
assert len(definitions)==46

# Import only textures used by the catalog. References in the WBP ensure cooking.
tasks=[]
for direction in ['Up','Down']:
    task=unreal.AssetImportTask()
    task.filename=str(root/'ContentSource/Resources'/f'T_ResourceChevron{direction}.png')
    task.destination_path='/Game/UI/Icons/Resources'
    task.destination_name=f'T_ResourceChevron{direction}'
    task.automated=True
    task.replace_existing=True
    task.save=True
    tasks.append(task)
for definition in definitions:
    key=str(definition.widget_prefix)
    png=root/'ContentSource/Resources'/f'T_{key}.png'
    if not png.exists() or unreal.EditorAssetLibrary.does_asset_exist('/Game/UI/Icons/Resources/T_'+key): continue
    task=unreal.AssetImportTask()
    task.filename=str(png)
    task.destination_path='/Game/UI/Icons/Resources'
    task.destination_name=f'T_{key}'
    task.automated=True
    task.replace_existing=False
    task.save=True
    tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for definition in definitions:
    texture=unreal.ResourceCatalog.get_resource_icon(definition.resource_type)
    assert texture,str(definition.widget_prefix)
    if '/Icons/Resources/' in texture.get_path_name():
        texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
        texture.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        unreal.EditorAssetLibrary.save_loaded_asset(texture)

nodes=list(umg.call_method('GetWidgets',(bp,)).widgets)
byname={str(n.widget_name):n for n in nodes if n.widget}
canvas=byname['ResourceDisplayRoot'].widget

def props(obj, values):
    schema=json.loads(ObjectTools.list_properties(obj))
    # Exact reflected names are discovered per widget/slot, including nested fields.
    keys={k.lower().replace('_',''):k for k in schema}
    resolved={keys[k.lower().replace('_','')]:v for k,v in values.items()}
    ObjectTools.get_properties(obj,list(resolved))
    assert ObjectTools.set_properties(obj,json.dumps(resolved)),(obj.get_name(),resolved)

for name in ['ResourceBarBorder','PolymerResourceCard','ConnectorResourceCard','ResourceBarScale']:
    if name in byname:
        assert umg.call_method('RemoveWidget',(bp,byname[name].widget)),name

def add(cls,name,parent):
    info=umg.call_method('AddWidget',(bp,cls,name,parent,-1))
    assert info.widget,name
    ObjectTools.list_properties(info.widget)
    if info.slot: ObjectTools.list_properties(info.slot)
    return info.widget

def text(name,label,parent,size=12,color=(.9,.93,.95,1)):
    widget=add(unreal.TextBlock,name,parent)
    widget.set_text(label)
    fontdata=json.loads(ObjectTools.get_properties(widget,['font']))['font']
    fontdata['size']=size
    fontdata['typefaceFontName']='Bold'
    props(widget,{'font':fontdata,
                  'colorAndOpacity':{'specifiedColor':dict(zip(['r','g','b','a'],color))},
                  'visibility':'HitTestInvisible'})
    return widget

def place(widget,x,y,w,h):
    widget.slot.set_position(unreal.Vector2D(x,y))
    widget.slot.set_size(unreal.Vector2D(w,h))

scale=add(unreal.ScaleBox,'ResourceBarScale',canvas)
props(scale.slot,{'layoutData':{'offsets':{'left':345,'top':20,'right':365,'bottom':128},
                              'anchors':{'minimum':{'x':0,'y':0},'maximum':{'x':1,'y':0}},
                              'alignment':{'x':0,'y':0}},'bAutoSize':False})
scale.set_stretch(unreal.Stretch.SCALE_TO_FIT)
scale.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
widthbox=add(unreal.SizeBox,'ResourceBarWidth',scale)
props(widthbox.slot,{'horizontalAlignment':'HAlign_Center','verticalAlignment':'VAlign_Top'})
widthbox.set_width_override(1200)
frame=add(unreal.Border,'ResourceBarFrame',widthbox)
frame.set_brush_color(unreal.LinearColor(.32,.29,.20,1))
frame.set_padding(unreal.Margin(1))
background=add(unreal.Border,'ResourceBarBackground',frame)
background.set_brush_color(unreal.LinearColor(.008,.014,.018,.96))
background.set_padding(unreal.Margin(3))
layout=add(unreal.VerticalBox,'ResourceBarLayout',background)
header=add(unreal.SizeBox,'ResourceCategoryHeaderSize',layout)
header.set_height_override(28)
tabs=add(unreal.HorizontalBox,'ResourceCategoryTabs',header)
for prefix,label in [('RawMaterials','RAW MATERIALS  9'),('Materials','MATERIALS  11'),('Components','COMPONENTS  15'),('AdvancedGoods','ADVANCED GOODS  10')]:
    button=add(unreal.Button,prefix+'Button',tabs)
    props(button.slot,{'size':{'value':1,'sizeRule':'Fill'},'padding':{'left':1,'top':0,'right':1,'bottom':2}})
    button.set_tool_tip_text('Show '+label.split('  ')[0].lower())
    button.set_background_color(unreal.LinearColor(.04,.07,.09,1))
    labelwidget=text(prefix+'Label',label,button,13)
    props(labelwidget,dict(justification='Center'))
togglebox=add(unreal.SizeBox,'ResourceCollapseSize',tabs)
togglebox.set_width_override(36)
toggle=add(unreal.Button,'ToggleResourceCardsButton',togglebox)
toggle.set_tool_tip_text('Hide or show resource cards')
toggle.set_background_color(unreal.LinearColor(.04,.07,.09,1))
style=json.loads(ObjectTools.get_properties(toggle,['widgetStyle']))['widgetStyle']
style['normalPadding']=style['pressedPadding']={'left':4,'top':1,'right':4,'bottom':1}
props(toggle,{'widgetStyle':style})
chevrons=add(unreal.Overlay,'ResourceCollapseIcons',toggle)
for direction in ['Up','Down']:
    icon=add(unreal.Image,'ResourceCollapse'+direction+'Icon',chevrons)
    icon.set_brush_from_texture(unreal.load_asset('/Game/UI/Icons/Resources/T_ResourceChevron'+direction),True)
    props(icon.slot,{'horizontalAlignment':'HAlign_Center','verticalAlignment':'VAlign_Center'})
    icon.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE if direction=='Up' else unreal.SlateVisibility.COLLAPSED)
body=add(unreal.HorizontalBox,'ResourceCardsBody',layout)
grid=add(unreal.UniformGridPanel,'ResourceCardsGrid',body)
props(grid.slot,{'size':{'value':1,'sizeRule':'Fill'}})
grid.set_slot_padding(unreal.Margin(2))
grid.set_min_desired_slot_width(124)
grid.set_min_desired_slot_height(44)
energybox=add(unreal.SizeBox,'ResourceEnergyContainer',body)
energybox.set_width_override(170)
props(energybox.slot,{'padding':{'left':4,'top':2,'right':2,'bottom':2}})

configs=[]
indices={}
for definition in definitions:
    key=str(definition.widget_prefix)
    category=definition.category
    energy=definition.resource_type==unreal.ResourceType.ENERGY
    index=indices.get(category,0)
    indices[category]=index+1
    # Fixed-size cells preserve icon/value alignment; hidden categories collapse.
    card=add(unreal.SizeBox,key+'ResourceCard',energybox if energy else grid)
    card.set_height_override(84 if energy else 40)
    card.set_min_desired_width(120 if not energy else 160)
    if not energy:
        count=sum(1 for d in definitions if d.category==category)
        columns=(count+1)//2
        props(card.slot,{'row':index//columns,'column':index%columns,'horizontalAlignment':'HAlign_Fill','verticalAlignment':'VAlign_Fill'})
    card.set_visibility(unreal.SlateVisibility.VISIBLE if energy or category==unreal.ResourceCategory.RAW_MATERIALS else unreal.SlateVisibility.COLLAPSED)
    tooltip=str(definition.display_name)+(' (imported: Mother Base, rewards, exploration or traders)' if definition.imported else '')
    card.set_tool_tip_text(tooltip)
    edge=add(unreal.Border,key+'CardEdge',card)
    props(edge.slot,{'horizontalAlignment':'HAlign_Fill','verticalAlignment':'VAlign_Fill'})
    edge.set_brush_color(unreal.LinearColor(.08,.13,.15,1))
    edge.set_padding(unreal.Margin(1))
    fill=add(unreal.Border,key+'CardFill',edge)
    fill.set_brush_color(unreal.LinearColor(.012,.023,.029,.98))
    fill.set_padding(unreal.Margin(0))
    contents=add(unreal.CanvasPanel,key+'CardContents',fill)
    props(contents.slot,{'horizontalAlignment':'HAlign_Fill','verticalAlignment':'VAlign_Fill'})
    icon=add(unreal.Image,key+'Icon',contents)
    icon.set_brush_from_texture(unreal.ResourceCatalog.get_resource_icon(definition.resource_type),False)
    place(icon,8 if energy else 2,18 if energy else 0,48 if energy else 40,48 if energy else 40)
    amount=text(key+'AmountText','1,000' if energy else '0',contents,22 if energy else 18)
    rate=text(key+'RateText','+0.0/min',contents,12 if energy else 11,(.68,.9,.28,1))
    for widget,y,height in [(amount,16 if energy else 0,28 if energy else 23),(rate,48 if energy else 23,20 if energy else 17)]:
        props(widget.slot,{'layoutData':{'offsets':{'left':62 if energy else 46,'top':y,'right':4,'bottom':height},
            'anchors':{'minimum':{'x':0,'y':0},'maximum':{'x':1,'y':0}},'alignment':{'x':0,'y':0}}})
    config=unreal.ResourceDisplayConfig()
    config.set_editor_property('resource_type',definition.resource_type)
    config.set_editor_property('tooltip',tooltip)
    config.set_editor_property('icon_texture',unreal.ResourceCatalog.get_resource_icon(definition.resource_type))
    configs.append(config)

# Preserve the existing C++ BindWidget variables and expose the new controls.
for node in umg.call_method('GetWidgets',(bp,)).widgets:
    name=str(node.widget_name)
    if name.endswith(('Icon','AmountText','RateText','Button')) or name in ['ResourceCardsGrid','ResourceCardsBody']:
        umg.call_method('ToggleWidgetAsVariable',(bp,node.widget,True))
assert umg.call_method('CompileWidgetBlueprint',(bp,))
default=unreal.get_default_object(bp.generated_class())
default.set_editor_property('resources',configs)
assert unreal.EditorAssetLibrary.save_loaded_asset(bp)
report={'passed':True,'resource_count':len(definitions),'category_counts':{str(k):v for k,v in indices.items()},'resource_textures':39,'widget':bp.get_path_name()}
(root/'Saved/ResourceHudBuild.json').write_text(json.dumps(report,indent=2))
unreal.log('RESOURCE_HUD_BUILD '+json.dumps(report))

# Keep objective progress inside the left sidebar, clear of the resource grid.
objectives=unreal.load_asset('/Game/UI/Objectives/WBP_ObjectiveTracker')
tracker=next(n.widget for n in umg.call_method('GetWidgets',(objectives,)).widgets if str(n.widget_name)=='TrackerSize')
props(tracker,{'widthOverride':310,'bOverride_WidthOverride':True})
assert umg.call_method('CompileWidgetBlueprint',(objectives,))
assert unreal.EditorAssetLibrary.save_loaded_asset(objectives)
