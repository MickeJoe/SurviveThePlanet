import unreal,json
from pathlib import Path
from editor_toolset.toolsets.object import ObjectTools
root=Path(unreal.Paths.project_dir())
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
assets=unreal.AssetToolsHelpers.get_asset_tools();library=unreal.EditorAssetLibrary
umg=unreal.get_default_object(unreal.UMGToolSet)
def props(obj,values):
    schema=json.loads(ObjectTools.list_properties(obj));keys={k.lower().replace('_',''):k for k in schema}
    data={keys[k.lower().replace('_','')]:v for k,v in values.items()}
    ObjectTools.get_properties(obj,list(data));assert ObjectTools.set_properties(obj,json.dumps(data)),data
def add(bp,cls,name,parent=None,var=False):
    info=umg.call_method('AddWidget',(bp,cls,name,parent,-1));assert info.widget,name
    if var:umg.call_method('ToggleWidgetAsVariable',(bp,info.widget,True))
    return info.widget
def place(w,x,y,width,height):
    props(w.slot,{'layoutData':{'offsets':{'left':x,'top':y,'right':width,'bottom':height},'anchors':{'minimum':{'x':0,'y':0},'maximum':{'x':0,'y':0}},'alignment':{'x':0,'y':0}},'bAutoSize':False})
white={'r':.87,'g':.95,'b':1,'a':1};cyan={'r':.08,'g':.8,'b':1,'a':1};gold={'r':1,'g':.72,'b':.16,'a':1};green={'r':.28,'g':.93,'b':.48,'a':1}
def text(bp,name,parent,value,size=24,color=white,var=False):
    w=add(bp,unreal.TextBlock,name,parent,var);font=json.loads(ObjectTools.get_properties(w,['font']))['font'];font['size']=size
    props(w,{'font':font,'colorAndOpacity':{'specifiedColor':color},'visibility':'HitTestInvisible'})
    w.set_text(value);return w
def image(bp,name,parent,texture=None,var=False):
    w=add(bp,unreal.Image,name,parent,var)
    if texture:w.set_brush_from_texture(texture,False)
    return w
def size(bp,name,parent,width=None,height=None):
    w=add(bp,unreal.SizeBox,name,parent);values={}
    if width is not None:values.update(widthOverride=width,bOverride_WidthOverride=True)
    if height is not None:values.update(heightOverride=height,bOverride_HeightOverride=True)
    props(w,values);return w
def clear(bp):
    for n in reversed(umg.call_method('GetWidgets',(bp,)).widgets):
        if n.widget:umg.call_method('RemoveWidget',(bp,n.widget))
textures={}
for p in (root/'ContentSource/UI/BuildingFlow').glob('*.png'):
    task=unreal.AssetImportTask();task.filename=str(p);task.destination_path='/Game/UI/BuildingFlow';task.destination_name=p.stem;task.automated=True;task.replace_existing=True;task.save=True
    assets.import_asset_tasks([task]);tex=unreal.load_asset('/Game/UI/BuildingFlow/'+p.stem)
    tex.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    tex.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    library.save_loaded_asset(tex);textures[p.stem]=tex
card_path='/Game/UI/BuildingFlow/WBP_FlowResourceCard'
card=unreal.load_asset(card_path)
if not card:
    factory=unreal.WidgetBlueprintFactory();factory.set_editor_property('parent_class',unreal.UserWidget)
    card=assets.create_asset('WBP_FlowResourceCard','/Game/UI/BuildingFlow',unreal.WidgetBlueprint,factory)
clear(card)
box=size(card,'CardSize',None,104,126)
border=add(card,unreal.Border,'CardSurface',box)
props(border,{'brushColor':{'r':.04,'g':.095,'b':.13,'a':1},'padding':{'left':10,'top':10,'right':10,'bottom':8}})
vb=add(card,unreal.VerticalBox,'CardContent',border)
ic=size(card,'IconSize',vb,64,64);props(ic.slot,{'horizontalAlignment':'HAlign_Center'})
image(card,'ResourceIcon',ic,None,True)
amount=text(card,'AmountText',vb,'0',24,white,True)
props(amount,{'justification':'Center'});props(amount.slot,{'padding':{'left':0,'top':8,'right':0,'bottom':0}})
unreal.BlueprintEditorLibrary.compile_blueprint(card);library.save_loaded_asset(card)

bp=unreal.load_asset('/Game/UI/WBP_BuildingPopup');assert bp
clear(bp)
canvas=add(bp,unreal.CanvasPanel,'PopupCanvas')
props(canvas,{'visibility':'SelfHitTestInvisible'})
width=size(bp,'PopupWidth',canvas,680)
props(width.slot,{'layoutData':{'offsets':{'left':-24,'top':65,'right':680,'bottom':0},'anchors':{'minimum':{'x':1,'y':.5},'maximum':{'x':1,'y':.5}},'alignment':{'x':1,'y':.5}},'bAutoSize':True})
surface=add(bp,unreal.Border,'PopupSurface',width)
props(surface,{'brushColor':{'r':.025,'g':.06,'b':.08,'a':.98},'padding':{'left':20,'top':18,'right':20,'bottom':18}})
body=add(bp,unreal.VerticalBox,'PopupBody',surface)
header=size(bp,'HeaderSize',body,None,112);hc=add(bp,unreal.CanvasPanel,'HeaderCanvas',header)
im=image(bp,'BuildingImage',hc,None,True);place(im,0,0,118,104)
name=text(bp,'BuildingNameText',hc,'Building',27,white,True);place(name,136,20,420,60);props(name,{'autoWrapText':True})
status=image(bp,'PowerStatusImage',hc,textures['T_PowerConnected'],True);place(status,578,20,58,58)
flow_size=size(bp,'FlowSize',body,None,146);flow=add(bp,unreal.CanvasPanel,'FlowCanvas',flow_size)
inputs=add(bp,unreal.HorizontalBox,'InputsBox',flow,True);place(inputs,0,10,240,126)
arr=image(bp,'InputArrow',flow,textures['T_FlowArrow']);place(arr,242,58,26,26)
clock=image(bp,'CycleIcon',flow,textures['T_CycleClock']);place(clock,290,8,60,60)
ct=text(bp,'CycleText',flow,'20 s',24,cyan,True);place(ct,270,76,105,36);props(ct,{'justification':'Center','visibility':'Visible'})
progress=add(bp,unreal.ProgressBar,'CycleProgress',flow,True);place(progress,275,118,95,7);props(progress,{'fillColorAndOpacity':cyan})
arr=image(bp,'OutputArrow',flow,textures['T_FlowArrow']);place(arr,391,58,26,26)
outputs=add(bp,unreal.HorizontalBox,'OutputsBox',flow,True);place(outputs,440,10,170,126)
footer=size(bp,'EnergySize',body,None,64);fc=add(bp,unreal.CanvasPanel,'EnergyCanvas',footer)
line=add(bp,unreal.Border,'Divider',fc);place(line,0,5,640,1);props(line,{'brushColor':{'r':.1,'g':.34,'b':.42,'a':1},'padding':{'left':0,'top':0,'right':0,'bottom':0}})
energy=unreal.ResourceCatalog.get_resource_icon(unreal.ResourceType.ENERGY)
icon=image(bp,'ConsumptionIcon',fc,energy,True);place(icon,95,23,28,30);icon.set_tool_tip_text('Energy consumption')
t=text(bp,'ConsumptionText',fc,'-12.0 /min',24,gold,True);place(t,135,21,190,38);props(t,{'visibility':'Visible'})
icon=image(bp,'GenerationIcon',fc,energy,True);place(icon,335,23,28,30);icon.set_tool_tip_text('Energy production')
t=text(bp,'GenerationText',fc,'+0.0 /min',24,green,True);place(t,375,21,210,38);props(t,{'visibility':'Visible'})
# Existing bindings and drone interaction remain available in the new card.
for name in ['BuildingDescriptionText','PowerStatusText']:
    hidden=text(bp,name,body,'',1,white,True);props(hidden,{'visibility':'Collapsed'})
drones=add(bp,unreal.Border,'DroneAssignmentSection',body,True)
props(drones,{'brushColor':{'r':.025,'g':.06,'b':.08,'a':1},'padding':{'left':0,'top':12,'right':0,'bottom':0}})
add(bp,unreal.HorizontalBox,'DroneSlotsBox',drones,True)
unreal.BlueprintEditorLibrary.compile_blueprint(bp);library.save_loaded_asset(bp)
(root/'Saved/BuildingFlowAuthor.json').write_text(json.dumps({'passed':True}))
