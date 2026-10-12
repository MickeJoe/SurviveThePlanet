import unreal,json,inspect
from pathlib import Path
from editor_toolset.toolsets.blueprint import BlueprintTools
report={'methods':[n for n in dir(BlueprintTools) if 'graph' in n or 'node' in n],'writer':BlueprintTools.write_graph_dsl.__doc__,'fixtures':[]}
for f in state['fixtures']:
 report['fixtures'].append({'mine':f['mine'].get_name(),'drill_now':str(f['drill'].get_editor_property('relative_rotation')),'drill_idle':f['idle_drill'],'fan_now':str(f['fan'].get_editor_property('relative_rotation')),'fan_idle':f['idle_fan'],'drone_count':f['mine'].get_assigned_drone_count(),'producing':f['mine'].is_producing_resource()})
Path('C:/UE5/SurviveThePlanet 5.8/Saved/MineralMineMotionDiagnostics.json').write_text(json.dumps(report,indent=2))
