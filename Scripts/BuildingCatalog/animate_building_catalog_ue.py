"""Create placeable visual Blueprints and their mechanical animation graphs."""
import unreal,json
from pathlib import Path
from editor_toolset.toolsets.actor import ActorTools
from editor_toolset.toolsets.blueprint import BlueprintTools
ROOT=Path(unreal.Paths.project_dir()).resolve();assert ROOT==Path(r'C:\UE5\SurviveThePlanet 5.8')
OUT=Path(r'C:\Users\qtxmj\Documents\Codex\2026-10-07\kan-du-skapa-alla-meshes-f-2\outputs')
DEST='/Game/Units/Buildings/BlueprintCatalog'
records=json.loads((OUT/'BuildingCatalog/authored_manifest.json').read_text())
lib=unreal.EditorAssetLibrary;sub=unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
report=[]
def components(bp):
 return [unreal.SubobjectDataBlueprintFunctionLibrary.get_associated_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)) for h in sub.k2_gather_subobject_data_for_blueprint(bp)]
for r in records[globals().get('BATCH_START',0):globals().get('BATCH_END',49)]:
 key=r['key'];path=DEST+'/'+key;bp=unreal.load_asset(path+'/BP_'+key);assert bp
 motion=next((o for o in components(bp) if isinstance(o,unreal.StaticMeshComponent) and o.get_name().startswith('Motion')),None)
 if not motion:motion=ActorTools.add_component(bp,unreal.StaticMeshComponent.static_class(),'Motion')
 for o in components(bp):
  if isinstance(o,unreal.StaticMeshComponent) and o!=motion and o.get_name().startswith('StaticMesh_GEN_VARIABLE'):ActorTools.remove_component(o)
 motion.set_editor_property('static_mesh',unreal.load_asset(path+'/Meshes/SM_'+key+'_Motion'))
 motion.set_editor_property('mobility',unreal.ComponentMobility.MOVABLE)
 # The source and UE FBX exporter axis mapping keep local axes consistent.
 x,y,z=[100*v for v in r['pivot_m']]
 y=-y
 motion.set_editor_property('relative_location',unreal.Vector(x,y,z))
 motion.set_editor_property('relative_scale3d',unreal.Vector(1,1,1));motion.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
 unreal.BlueprintEditorLibrary.compile_blueprint(bp)
 graph=BlueprintTools.get_graph(bp,'EventGraph')
 if r['motion']=='slide':
  code='''(event EventTick (DeltaSeconds)
    (bind t (Utilities|Time|GetGameTimeinSeconds))
    (bind offset (* (Math|Trig|Sin(Radians) (* t 1.1)) 100.0))
    (Transformation|SetRelativeLocation
      :self (Variables|Default|GetMotion)
      :NewLocation (Math|Vector|MakeVector :X (+ %f offset) :Y %f :Z %f)
      :bSweep false :bTeleport false))'''%(x,y,z)
 else:
  speed=160.0 if r['motion']=='fan' else (70.0 if r['motion']=='drill' else 25.0)
  code='''(event EventTick (DeltaSeconds)
    (Transformation|AddLocalRotation
      :self (Variables|Default|GetMotion)
      :DeltaRotation (Math|Rotator|MakeRotator :Roll 0.0 :Pitch 0.0 :Yaw (* DeltaSeconds %f))
      :bSweep false :bTeleport false))'''%speed
 BlueprintTools.write_graph_dsl(graph,code)
 unreal.BlueprintEditorLibrary.compile_blueprint(bp);assert lib.save_loaded_asset(bp)
 (OUT/'BuildingCatalog'/key/'Animation.dsl').write_text(code)
 report.append({'key':key,'blueprint':bp.get_path_name(),'motion_component':motion.get_name(),'pivot_cm':[x,y,z],'animation':r['motion'],'compiled':True})
 (OUT/'ue_animation_report.json').write_text(json.dumps(report,indent=2))
unreal.log('CATALOG_ANIMATIONS_READY '+str(len(report)))
