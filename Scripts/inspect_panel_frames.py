import json
from pathlib import Path
import unreal
from editor_toolset.toolsets.object import ObjectTools
root=Path(unreal.Paths.project_dir()).resolve()
assert root==Path(r'C:\UE5\SurviveThePlanet 5.8')
umg=unreal.get_default_object(unreal.UMGToolSet)
report={}
for path in ['/Game/UI/WBP_WeatherTimeDisplay_V2','/Game/UI/Mission/WBP_MissionConfidencePanel','/Game/UI/Trading/WBP_VisitingMerchant']:
    bp=unreal.load_asset(path)
    if not bp: continue
    entries=[]
    for node in umg.call_method('GetWidgets',(bp,)).widgets:
        widget=node.widget
        if not widget: continue
        if isinstance(widget,unreal.Border):
            entry={'name':widget.get_name(),'brush':json.loads(ObjectTools.get_properties(widget,['background','brushColor','padding']))}
            if isinstance(widget.slot,unreal.CanvasPanelSlot):
                entry['slot']=json.loads(ObjectTools.get_properties(widget.slot,['layoutData']))
            entries.append(entry)
    report[path]=entries
(root/'Saved/PanelFrameInspection.json').write_text(json.dumps(report,indent=2))
unreal.log('PANEL_FRAMES_INSPECTED')
