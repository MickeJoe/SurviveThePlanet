"""Regression checks for six-slot coverage, deterministic instances and reload."""
import json
from pathlib import Path
import unreal

project=Path(unreal.Paths.project_dir()).resolve()
assert str(project).lower()==r'C:\UE5\SurviveThePlanet 5.8'.lower()
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level='/Game/WorldGeneration/SectorTemplates/ST_First/L_SectorTemplate_First'
levels.load_level(level)
def root():
    return next(a for a in actors.get_all_level_actors() if isinstance(a,unreal.PlanetSectorTemplateActor))
sector=root()
def snapshot():
    generated=root().get_editor_property('generated_preview')
    assert generated and not generated.get_editor_property('diagnostics')
    selected=[str(v) for v in generated.get_editor_property('selected_variant_ids')]
    assert len(selected)==6 and 'None' not in selected
    groups=generated.get_components_by_class(unreal.InstancedStaticMeshComponent)
    data=[]
    for c in groups:
        for i in range(c.get_instance_count()):
            t=c.get_instance_transform(i,False)
            p,q,s=t.translation,t.rotation,t.scale3d
            data.append((c.static_mesh.get_path_name(),(p.x,p.y,p.z,q.x,q.y,q.z,q.w,s.x,s.y,s.z)))
    assert len([a for a in actors.get_all_level_actors() if isinstance(a,unreal.PlanetGeneratedSector)])==1
    return selected,sorted(data)

before_shapes=[a.get_path_name() for a in actors.get_all_level_actors() if isinstance(a,unreal.PlanetTerrainClusterShapeActor)]
tests={}
selections=[]
for seed in [0,3,4,10,12345]:
    sector.set_editor_property('preview_seed',seed)
    sector.generate_preview()
    first=snapshot()
    sector.generate_preview()
    assert snapshot()==first
    tests[str(seed)]={'selected':first[0],'instances':len(first[1])}
    selections.append(tuple(first[0]))
assert len(set(selections))>1
sector.clear_preview()
assert not any(isinstance(a,unreal.PlanetGeneratedSector) for a in actors.get_all_level_actors())
assert before_shapes==[a.get_path_name() for a in actors.get_all_level_actors() if isinstance(a,unreal.PlanetTerrainClusterShapeActor)]
sector.set_editor_property('preview_seed',4)
sector.generate_preview()
expected=snapshot()
assert levels.save_current_level()
levels.load_level('/Game/WorldGeneration/ClusterVariants/FullSector/Shape03/L_ClusterVariants_03_Dense')
levels.load_level(level)
assert snapshot()==expected,'Reload changed preview instances'
assert root().get_editor_property('sector_radius')==4000
unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(700,-5200,6700),unreal.Rotator(pitch=-52,yaw=98,roll=0))
levels.save_current_level()
(project/'Saved'/'FullSectorValidation.json').write_text(json.dumps({'seeds':tests,'reload':'passed','clear_preserves_six_shapes':'passed','radius_cm':4000},indent=2))
unreal.log('FULL_SECTOR_VALIDATION_PASSED')
