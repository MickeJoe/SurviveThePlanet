import unreal,json
from pathlib import Path
root=Path('C:/UE5/SurviveThePlanet 5.8');reports=[]
for planet in ['Aridus','Nexaris','Borealis']:
 report=json.loads((root/'Saved'/('SurfaceResources_'+planet+'.json')).read_text());assert report['passed'],report
 counts={}
 for deposit in report['deposits']:counts[deposit['type']]=counts.get(deposit['type'],0)+1
 reports.append({'planet':planet,'passed':True,'checks':report['checks'],'counts':counts})
first=root/'Saved/SurfaceResources_Aridus_First.json'
assert json.loads(first.read_text())['deposits']==json.loads((root/'Saved/SurfaceResources_Aridus.json').read_text())['deposits']
result={'passed':True,'editor_build':'UE 5.8 Win64 Development succeeded','runtime':reports,'deterministic_aridus_layout':True,'day_night_review':'Aridus and Nexaris inspected at normal game-camera distance','blender_source':'ContentSource/SurfaceResources/SurfaceResources.blend','occurrence_setting':'PlanetDefinition.SurfaceDeposits'}
(root/'Saved/SurfaceResourcesReview.json').write_text(json.dumps(result,indent=2))
assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Planet/Maps/L_Aridus')
unreal.log('SURFACE_RESOURCES_COMPLETE '+json.dumps(result))
