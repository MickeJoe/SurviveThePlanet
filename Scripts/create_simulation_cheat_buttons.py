import json
from pathlib import Path
import unreal
from editor_toolset.toolsets.object import ObjectTools
root=Path(unreal.Paths.project_dir()).resolve()
assert root==Path(r'C:\UE5\SurviveThePlanet 5.8')
helpers=(root/'Scripts/create_trade_ui.py').read_text(encoding='utf-8')
exec(helpers[:helpers.index('credits=unreal.load_asset')])
bp=unreal.load_asset('/Game/UI/Development/WBP_CheatMenu')
assert bp
nodes={str(n.widget_name):n.widget for n in umg.call_method('GetWidgets',(bp,)).widgets if n.widget}
column=next(w for w in nodes.values() if isinstance(w,unreal.VerticalBox))
for name,label in [('Speed10Button','SPEED x10'),('ConfidenceDecayButton','PAUSE CONFIDENCE DECAY')]:
    if name in nodes: continue
    widget=add(bp,unreal.Button,name,column,True)
    props(widget,{'backgroundColor':TEAL,'isFocusable':False})
    props(widget.slot,{'padding':{'left':0,'top':8,'right':0,'bottom':0}})
    caption=text(bp,name+'Label',widget,label,0,0,0,0,17,WHITE,name=='ConfidenceDecayButton')
    props(caption.slot,{'horizontalAlignment':'HAlign_Center','verticalAlignment':'VAlign_Center','padding':{'left':8,'top':8,'right':8,'bottom':8}})
assert umg.call_method('CompileWidgetBlueprint',(bp,))
assert unreal.EditorAssetLibrary.save_loaded_asset(bp)
unreal.log('SIMULATION_CHEAT_BUTTONS_AUTHORED')
