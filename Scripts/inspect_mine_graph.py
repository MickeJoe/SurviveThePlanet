import unreal,json
from pathlib import Path
from editor_toolset.toolsets.blueprint import BlueprintTools
bp=unreal.load_asset('/Game/Units/Buildings/BlueprintCatalog/SilicaExtractor/BP_SilicaExtractorBuilding');g=BlueprintTools.get_graph(bp,'EventGraph')
report={n:getattr(BlueprintTools,n).__doc__ for n in ['find_nodes','get_node_infos','delete_node']}
report['dsl']=str(BlueprintTools.read_graph_dsl(g))
Path('C:/UE5/SurviveThePlanet 5.8/Saved/MineralMineGraphDiagnostics.json').write_text(json.dumps(report,indent=2))
