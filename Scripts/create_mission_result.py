"""Author result screen and connect the existing mission HUD in UE 5.8."""
import json
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
from editor_toolset.toolsets.object import ObjectTools
assert Path(unreal.Paths.project_dir()).resolve() == Path(r'C:\UE5\SurviveThePlanet 5.8')
assert not unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
umg=unreal.get_default_object(unreal.UMGToolSet)
assets=unreal.AssetToolsHelpers.get_asset_tools()
bp=unreal.load_asset('/Game/UI/Mission/WBP_MissionResult')
if not bp:
    factory=unreal.WidgetBlueprintFactory()
    factory.set_editor_property('parent_class',unreal.MissionResultWidget)
    bp=assets.create_asset('WBP_MissionResult','/Game/UI/Mission',unreal.WidgetBlueprint,factory)
assert bp
def props(obj,values):
    schema=json.loads(ObjectTools.list_properties(obj))
    keys={k.lower().replace('_',''):k for k in schema}
    resolved={keys[k.lower().replace('_','')]:v for k,v in values.items()}
    ObjectTools.get_properties(obj,list(resolved))
    assert ObjectTools.set_properties(obj,json.dumps(resolved)),resolved
def add(cls,name,parent=None,var=False):
    w=umg.call_method('AddWidget',(bp,cls,name,parent,-1)).widget
    assert w,name
    if var:umg.call_method('ToggleWidgetAsVariable',(bp,w,True))
    return w
def rgba(r,g,b,a=1):return dict(r=r,g=g,b=b,a=a)
gold=rgba(.52,.38,.16);cyan=rgba(0,.72,.9);white=rgba(.9,.95,.98);muted=rgba(.46,.62,.68)
def place(w,x,y,width,height,center=False):
    a=.5 if center else 0
    props(w.slot,{'layoutData':{'offsets':dict(left=x,top=y,right=width,bottom=height),'anchors':{'minimum':dict(x=a,y=a),'maximum':dict(x=a,y=a)},'alignment':dict(x=a,y=a)},'bAutoSize':False})
def border(name,parent,color,padding=0,var=False):
    w=add(unreal.Border,name,parent,var)
    props(w,{'brushColor':color,'padding':dict(left=padding,top=padding,right=padding,bottom=padding)})
    return w
def text(name,parent,label,x,y,width,height,size=18,color=white,var=False):
    w=add(unreal.TextBlock,name,parent,var)
    font=json.loads(ObjectTools.get_properties(w,['font']))['font'];font['size']=size
    props(w,{'font':font,'colorAndOpacity':{'specifiedColor':color},'visibility':'HitTestInvisible'})
    w.set_text(label)
    if isinstance(w.slot,unreal.CanvasPanelSlot):place(w,x,y,width,height)
    return w
for info in reversed(umg.call_method('GetWidgets',(bp,)).widgets):
    if info.widget:umg.call_method('RemoveWidget',(bp,info.widget))
canvas=add(unreal.CanvasPanel,'ResultCanvas')
scrim=border('Backdrop',canvas,rgba(0,.008,.015,.7))
props(scrim.slot,{'layoutData':{'offsets':dict(left=0,top=0,right=0,bottom=0),'anchors':{'minimum':dict(x=0,y=0),'maximum':dict(x=1,y=1)},'alignment':dict(x=0,y=0)}})
# Scale the fixed design to fit small viewports while preserving its aspect ratio.
scale=add(unreal.ScaleBox,'ResponsiveResult',canvas)
props(scale.slot,{'layoutData':{'offsets':dict(left=32,top=24,right=32,bottom=24),'anchors':{'minimum':dict(x=0,y=0),'maximum':dict(x=1,y=1)},'alignment':dict(x=0,y=0)}})
props(scale,{'stretch':'ScaleToFit'})
size=add(unreal.SizeBox,'DesignSize',scale)
size.set_width_override(720);size.set_height_override(760)
frame=border('Frame',size,gold,2)
surface=border('Surface',frame,rgba(.018,.035,.045,.98))
body=add(unreal.CanvasPanel,'ResultBody',surface)
accent=border('AccentBorder',body,cyan,2,True);place(accent,24,24,668,112)
header=border('HeaderSurface',accent,rgba(.022,.065,.085))
hc=add(unreal.CanvasPanel,'HeaderCanvas',header)
t=text('OutcomeText',hc,'MISSION COMPLETE',16,15,632,48,32,cyan,True);props(t,{'justification':'Center'})
t=text('ReasonText',hc,'Mission progression complete',16,69,632,28,17,white,True);props(t,{'justification':'Center'})
heading=border('RewardsBar',body,rgba(.055,.09,.105));place(heading,24,156,668,38)
text('RewardsHeading',heading,'REWARDS',0,0,0,0,18,white,True)
for name,x,w in [('CreditsCard',24,324),('ResourcesCard',364,328)]:
    card=border(name,body,gold,1);place(card,x,208,w,126)
    panel=border(name+'Surface',card,rgba(.028,.052,.065))
    cc=add(unreal.CanvasPanel,name+'Canvas',panel)
    if name=='CreditsCard':
        icon=add(unreal.Image,'CreditsIcon',cc)
        icon.set_brush_from_texture(unreal.load_asset('/Game/UI/Contracts/Icons/T_Credits'),False)
        place(icon,(w-54)/2,10,54,54)
        t=text('CreditsRewardText',cc,'+2,500  CREDITS',18,72,w-36,36,23,white,True);props(t,{'justification':'Center'})
    else:
        icon=add(unreal.Image,'SteelIcon',cc)
        icon.set_brush_from_texture(unreal.load_asset('/Game/UI/Icons/Resources/T_Steel'),False)
        place(icon,(w-54)/2,10,54,54)
        t=text('ResourceRewardText',cc,'+150  Steel',18,72,w-36,40,22,white,True);props(t,{'justification':'Center'})
bar=border('StatsBar',body,rgba(.055,.09,.105));place(bar,24,358,668,38)
text('StatsHeading',bar,'MISSION STATS',0,0,0,0,18)
rows=[('Progress','MISSION PROGRESSION','10 / 10'),('Buildings','COMPLETED BUILDINGS','24'),('Time','SIMULATION TIME','00:42:18'),('Confidence','FINAL CONFIDENCE','78%')]
for i,(name,label,value) in enumerate(rows):
    y=408+i*47
    text(name+'Label',body,label,40,y,440,36,18)
    t=text(name+'Value',body,value,480,y,196,36,20,cyan,True);props(t,{'justification':'Right'})
    line=border(name+'Divider',body,rgba(.10,.16,.18));place(line,40,y+38,636,1)
text('RewardHint',body,'Rewards added to this colony inventory',32,610,652,30,15,muted)
def button(name,label,x,width,color):
    rim=border(name+'Outline',body,color,2);place(rim,x,662,width,66)
    b=add(unreal.Button,name,rim,True);props(b,{'backgroundColor':rgba(.035,.12,.15)})
    t=text(name+'Label',b,label,0,0,0,0,20);props(t,{'justification':'Center'})
button('PlanetSelectButton','PLANET SELECTOR',24,324,gold)
button('RetryButton','PLAY AGAIN',364,328,cyan)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
cdo=unreal.get_default_object(bp.generated_class())
props(cdo,{'VictoryResources':[{'resource':'Steel','cost':150}], 'DefeatResources':[{'resource':'Steel','cost':30}]})
assert unreal.EditorAssetLibrary.save_loaded_asset(bp)
# Existing panel's progression controls are renamed to the native optional bindings.
hud=unreal.load_asset('/Game/UI/Mission/WBP_MissionConfidencePanel')
nodes=umg.call_method('GetWidgets',(hud,)).widgets
print('MISSION_HUD_WIDGETS '+json.dumps([{'name':i.widget.get_name(),'class':i.widget.get_class().get_name(),'text':str(i.widget.get_text()) if isinstance(i.widget,unreal.TextBlock) else ''} for i in nodes if i.widget]))
progress=[i.widget for i in nodes if isinstance(i.widget,unreal.ProgressBar)]
assert len(progress)==1,progress
for w,new in [(progress[0],'MissionProgressBar')]+[(i.widget,'MissionProgressText') for i in nodes if isinstance(i.widget,unreal.TextBlock) and '/' in str(i.widget.get_text()) and 'h' not in str(i.widget.get_text()).lower()]:
    if w.get_name()!=new:umg.call_method('RenameWidget',(hud,w,new))
    umg.call_method('ToggleWidgetAsVariable',(hud,w,True))
unreal.BlueprintEditorLibrary.compile_blueprint(hud)
assert unreal.EditorAssetLibrary.save_loaded_asset(hud)
print('MISSION_RESULT_ASSETS_SAVED')
