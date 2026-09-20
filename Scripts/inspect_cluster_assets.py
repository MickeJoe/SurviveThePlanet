import unreal
import json
from pathlib import Path
project=Path(unreal.Paths.project_dir()).resolve()
assert str(project).lower()==r'C:\UE5\SurviveThePlanet 5.8'.lower()
paths=unreal.EditorAssetLibrary.list_assets('/Game/Environment',recursive=True)
result=[]
for path in paths:
    if '/SM_' not in path: continue
    mesh=unreal.load_asset(path)
    if not isinstance(mesh,unreal.StaticMesh): continue
    b=mesh.get_bounding_box()
    result.append({'path':path,'min':[b.min.x,b.min.y,b.min.z],'max':[b.max.x,b.max.y,b.max.z]})
(project/'Saved'/'ClusterMeshInventory.json').write_text(json.dumps(result,indent=2))
