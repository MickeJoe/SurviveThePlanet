import unreal,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
project=Path('C:/UE5/SurviveThePlanet 5.8')
taskroot=project/'Saved'
try:
    exec((project/'Scripts/create_building_flow_ui.py').read_text(encoding='utf-8-sig').split('textures={}')[0])
    bp=unreal.load_asset('/Game/UI/WBP_WorldHoverTooltip')
    if not bp:
        factory=unreal.WidgetBlueprintFactory();factory.set_editor_property('parent_class',unreal.WorldHoverTooltipWidget)
        bp=assets.create_asset('WBP_WorldHoverTooltip','/Game/UI',unreal.WidgetBlueprint,factory)
    clear(bp)
    border=add(bp,unreal.Border,'TooltipSurface')
    props(border,{'visibility':'HitTestInvisible','brushColor':{'r':.025,'g':.06,'b':.08,'a':.97},'padding':{'left':12,'top':7,'right':12,'bottom':7}})
    text(bp,'NameText',border,'Building',15,white,True)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    assert bp.get_editor_property('status')==unreal.BlueprintStatus.BS_UP_TO_DATE
    assert library.save_loaded_asset(bp,False)
    (taskroot/'hover_author.json').write_text(json.dumps(dict(passed=True)))
except:(taskroot/'hover_error.txt').write_text(traceback.format_exc())
