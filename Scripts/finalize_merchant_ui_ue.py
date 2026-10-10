import unreal,json
from pathlib import Path
from editor_toolset.toolsets.object import ObjectTools
root=Path(unreal.Paths.project_dir()).resolve();assert root==Path(r'C:\UE5\SurviveThePlanet 5.8')
helpers=(root/'Scripts/create_trade_ui.py').read_text(encoding='utf-8');exec(helpers[:helpers.index('credits=unreal.load_asset')])
bp=unreal.load_asset('/Game/UI/Trading/WBP_VisitingMerchant')
nodes={str(n.widget_name):n.widget for n in umg.call_method('GetWidgets',(bp,)).widgets if n.widget}
umg.call_method('ToggleWidgetAsVariable',(bp,nodes['TradeButtonLabel'],True))
assert umg.call_method('CompileWidgetBlueprint',(bp,));assert unreal.EditorAssetLibrary.save_loaded_asset(bp)
planet=unreal.load_asset('/Game/Data/Planet/DA_PlanetDefinition');schedule=planet.get_editor_property('merchant_visits')
assert len(schedule.visits)==4 and schedule.repeat_cycle_days==8 and schedule.visits[0].arrival_after_hours==24
unreal.log('MERCHANT_UI_AND_AUTHORED_SCHEDULE_VERIFIED')
