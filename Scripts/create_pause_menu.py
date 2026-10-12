import unreal, json
from editor_toolset.toolsets.object import ObjectTools
assets = unreal.AssetToolsHelpers.get_asset_tools()
umg = unreal.get_default_object(unreal.UMGToolSet)
path = '/Game/UI/WBP_PauseMenu'
bp = unreal.load_asset(path)
if not bp:
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property('parent_class', unreal.PauseMenuWidget)
    bp = assets.create_asset('WBP_PauseMenu', '/Game/UI', unreal.WidgetBlueprint, factory)
assert bp
def props(obj, values):
    schema = json.loads(ObjectTools.list_properties(obj))
    keys = {k.lower().replace('_', ''): k for k in schema}
    data = {keys[k.lower().replace('_', '')]: v for k, v in values.items()}
    ObjectTools.get_properties(obj, list(data))
    assert ObjectTools.set_properties(obj, json.dumps(data)), data
def add(cls, name, parent=None, var=False):
    w = umg.call_method('AddWidget', (bp, cls, name, parent, -1)).widget
    assert w, name
    if var: umg.call_method('ToggleWidgetAsVariable', (bp, w, True))
    return w
def place(w, x, y, width, height, anchor=0, align=0):
    props(w.slot, {'layoutData': {'offsets': {'left': x, 'top': y, 'right': width, 'bottom': height}, 'anchors': {'minimum': {'x': anchor, 'y': anchor}, 'maximum': {'x': anchor, 'y': anchor}}, 'alignment': {'x': align, 'y': align}}, 'bAutoSize': False})
def rgba(r,g,b,a=1): return dict(r=r,g=g,b=b,a=a)
gold=rgba(.64,.46,.22); cyan=rgba(.0,.76,.9); white=rgba(.91,.95,.97)
def border(name, parent, color, padding=0):
    w=add(unreal.Border,name,parent)
    props(w,{'brushColor':color,'padding':dict(left=padding,top=padding,right=padding,bottom=padding)})
    return w
def text(name,parent,label,size,color=white):
    w=add(unreal.TextBlock,name,parent)
    font=json.loads(ObjectTools.get_properties(w,['font']))['font']
    font['size']=size
    props(w,{'font':font,'colorAndOpacity':{'specifiedColor':color},'visibility':'HitTestInvisible'})
    w.set_text(label)
    return w
for info in reversed(umg.call_method('GetWidgets',(bp,)).widgets):
    if info.widget: umg.call_method('RemoveWidget',(bp,info.widget))
canvas=add(unreal.CanvasPanel,'PauseCanvas')
scrim=border('Backdrop',canvas,rgba(0,0,0,.60))
props(scrim.slot,{'layoutData':{'offsets':dict(left=0,top=0,right=0,bottom=0),'anchors':{'minimum':dict(x=0,y=0),'maximum':dict(x=1,y=1)},'alignment':dict(x=0,y=0)}})
frame=border('MenuFrame',canvas,gold,2)
place(frame,0,0,520,438,.5,.5)
surface=border('MenuSurface',frame,rgba(.025,.048,.058,.99))
body=add(unreal.CanvasPanel,'MenuBody',surface)
t=text('PauseSymbol',body,'Ⅱ',32,cyan);place(t,24,18,40,48)
t=text('HeaderText',body,'GAME PAUSED',28);place(t,78,23,408,44)
t=text('SubtitleText',body,'Choose your next destination',16,rgba(.44,.62,.69));place(t,24,70,465,28)
def button(name,label,y,accent,fill,text_color=white):
    rim=border(name+'Outline',body,accent,2);place(rim,24,y,468,66)
    w=add(unreal.Button,name,rim,True)
    props(w,{'backgroundColor':fill})
    t=text(name+'Label',w,label,22,text_color)
    props(t,{'justification':'Center'})
button('ResumeButton','▶   RESUME GAME',116,cyan,rgba(.035,.20,.25))
button('PlanetSelectorButton','◎   PLANET SELECTOR',194,gold,rgba(.065,.09,.10))
button('QuitButton','➜   QUIT GAME',272,gold,rgba(.065,.09,.10),rgba(1,.34,.28))
line=border('Divider',body,rgba(.24,.32,.35));place(line,24,358,468,1)
t=text('ResumeHint',body,'ESC   Resume game',16,rgba(.56,.70,.77));place(t,32,382,440,28)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
assert unreal.EditorAssetLibrary.save_loaded_asset(bp)
print('PAUSE_MENU_ASSET_SAVED '+path)
