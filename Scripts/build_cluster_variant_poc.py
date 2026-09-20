"""Create three editable compositions for shape 03 and verify the C++ preview pipeline."""
import json
import math
from pathlib import Path
import unreal

PROJECT=Path(unreal.Paths.project_dir()).resolve()
assert str(PROJECT).lower()==r'C:\UE5\SurviveThePlanet 5.8'.lower()
ROOT='/Game/WorldGeneration'
VARIANTS=ROOT+'/ClusterVariants/Shape03'
AUTHOR_MAP=VARIANTS+'/L_ClusterVariants_Shape03'
SECTOR_MAP=ROOT+'/SectorTemplates/ST_First/L_SectorTemplate_First'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assets=unreal.EditorAssetLibrary
shape=assets.load_asset(ROOT+'/TerrainClusterShapes/DA_TerrainShape_03_Long')
assert shape
points=shape.get_editor_property('points')
# Recover the authored band's centerline from its paired perimeter samples.
spine=[points[0]]+[unreal.Vector2D((points[i].x+points[-i].x)/2,(points[i].y+points[-i].y)/2) for i in range(1,8)]+[points[8]]

def data_asset(name, cls):
    path=VARIANTS+'/'+name
    if assets.does_asset_exist(path): return assets.load_asset(path)
    factory=unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class',cls)
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,VARIANTS,cls,factory)

mesh_paths={
 'boulder':'/Game/Environment/Expedition02/Meshes/SM_Boulder_02',
 'ridge':'/Game/Environment/Expedition02/Meshes/SM_Cliff_Ridge_02',
 'scree':'/Game/Environment/Expedition02/Meshes/SM_Scree_02',
 'rosette':'/Game/Environment/Expedition02/Meshes/SM_Alien_Rosette_02',
 'trumpets':'/Game/Environment/Expedition02/Meshes/SM_Alien_Trumpets_02',
 'coral':'/Game/Environment/Expedition02/Meshes/SM_Alien_Coral_02',
 'fungus':'/Game/Environment/Expedition06/Meshes/SM_Alien06_DiscFungus',
}
meshes={key:assets.load_asset(path) for key,path in mesh_paths.items()}
assert all(meshes.values())
assets.make_directory(VARIANTS)
resume=assets.does_asset_exist(AUTHOR_MAP)
if not resume:
    assert levels.new_level(AUTHOR_MAP)

def place(root, key, sample, along, across, width, yaw, index):
    point=spine[sample]
    before,after=spine[max(0,sample-1)],spine[min(8,sample+1)]
    dx,dy=after.x-before.x,after.y-before.y
    length=math.hypot(dx,dy)
    dx,dy=dx/length,dy/length
    x=point.x+dx*along-dy*across
    y=point.y+dy*along+dx*across
    mesh=meshes[key]
    bounds=mesh.get_bounding_box()
    scale=width/max(bounds.max.x-bounds.min.x,bounds.max.y-bounds.min.y)
    origin=root.get_actor_location()
    actor=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(origin.x+x,origin.y+y,-bounds.min.z*scale),unreal.Rotator(pitch=0,yaw=yaw,roll=0))
    component=actor.static_mesh_component
    component.set_static_mesh(mesh)
    component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    actor.set_actor_scale3d(unreal.Vector(scale,scale,scale))
    actor.set_actor_label(f'{root.get_actor_label()}_{index:02}_{key}')
    actor.attach_to_actor(root,'',unreal.AttachmentRule.KEEP_WORLD,unreal.AttachmentRule.KEEP_WORLD,unreal.AttachmentRule.KEEP_WORLD,False)
    return actor

variants=[]
counts={}
for variant_index,label in enumerate(['A_Rock','B_Mixed','C_Vegetation']):
    variant=data_asset('DA_Cluster03_'+label,unreal.PlanetTerrainClusterVariant)
    if resume:
        assert variant.get_editor_property('compatible_shape')==shape
        variants.append(variant)
        counts[label]=len(variant.get_editor_property('elements'))
        continue
    variant.set_editor_property('variant_id','Cluster03_'+label)
    root=actors.spawn_actor_from_class(unreal.PlanetTerrainClusterAuthoringActor,unreal.Vector((variant_index-1)*3700,0,0))
    root.set_actor_label('Author_'+label)
    root.set_editor_property('shape',shape)
    root.set_editor_property('slot_id',label)
    root.set_editor_property('target_cluster_variant',variant)
    # Hand-composed hierarchy: three, two or one rock anchors, offset satellites,
    # planted shoulders and deliberate gaps at sample 3/5, not a uniform scatter.
    anchors=[[2,4,6],[2,6],[4]][variant_index]
    specs=[('boulder',s,0,0,[470,390,320][variant_index],17+s*37) for s in anchors]
    medium_samples=[[1,2,3,4,5,6,7],[1,2,5,6,7],[2,6]][variant_index]
    specs += [('boulder',s,80 if s%2 else -110,65 if s%2 else -75,180+(s%3)*35,51+s*43) for s in medium_samples]
    detail_samples=[1,2,4,6,7]
    for s in detail_samples:
        specs.append(('scree',s,-70,115 if s%2 else -110,140,83+s*29))
    plant_count=[4,20,34][variant_index]
    groups=[(2,-125),(4,100),(6,-110),(7,40)] if variant_index<2 else [(1,15),(2,-100),(3,50),(5,-80),(6,110),(7,-20)]
    for j in range(plant_count):
        s,shoulder=groups[j%len(groups)]
        layer=j//len(groups)
        key=['rosette','trumpets','coral','fungus'][j%4]
        specs.append((key,s,(layer-2)*65+(j%3)*17,shoulder+(layer%2)*42,100+(j%4)*22,13+j*47))
    members=[place(root,*spec,i) for i,spec in enumerate(specs)]
    root.set_editor_property('mesh_actors',members)
    root.bake_cluster_variant()
    assert len(variant.get_editor_property('elements'))==len(specs)
    assert variant.get_editor_property('compatible_shape')==shape
    assets.save_loaded_asset(variant)
    variants.append(variant)
    counts[label]=len(specs)
if not resume:
    assert levels.save_current_level()
library=data_asset('DA_ClusterVariantLibrary_POC',unreal.PlanetTerrainClusterLibrary)
library.set_editor_property('variants',variants)
assets.save_loaded_asset(library)

assert levels.load_level(SECTOR_MAP)
sector=next(a for a in actors.get_all_level_actors() if isinstance(a,unreal.PlanetSectorTemplateActor))
authored=[a.get_path_name() for a in actors.get_all_level_actors() if isinstance(a,unreal.PlanetTerrainClusterShapeActor)]
sector.set_editor_property('variant_library',library)

def snapshot():
    generated=sector.get_editor_property('generated_preview')
    groups=generated.get_components_by_class(unreal.InstancedStaticMeshComponent)
    def values(t):
        p,q,s=t.translation,t.rotation,t.scale3d
        return (p.x,p.y,p.z,q.x,q.y,q.z,q.w,s.x,s.y,s.z)
    return [str(x) for x in generated.get_editor_property('selected_variant_ids')], sorted((g.static_mesh.get_path_name(),[values(g.get_instance_transform(i,False)) for i in range(g.get_instance_count())]) for g in groups)

sector.set_editor_property('preview_seed',12345)
sector.generate_preview()
first=snapshot()
sector.generate_preview()
assert snapshot()==first,'same seed changed instances'
seen={}
for seed in range(30):
    sector.set_editor_property('preview_seed',seed)
    sector.generate_preview()
    selected,instances=snapshot()
    chosen=next(x for x in selected if x!='None')
    seen.setdefault(chosen,seed)
    if len(seen)==3: break
assert len(seen)==3,seen
sector.clear_preview()
assert not [a for a in actors.get_all_level_actors() if isinstance(a,unreal.PlanetGeneratedSector)]
assert authored==[a.get_path_name() for a in actors.get_all_level_actors() if isinstance(a,unreal.PlanetTerrainClusterShapeActor)]
seed_mixed=seen['Cluster03_B_Mixed']
sector.set_editor_property('preview_seed',seed_mixed)
sector.generate_preview()
assert len(sector.get_editor_property('generated_preview').get_editor_property('diagnostics'))==5
unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(1000,-4500,7000),unreal.Rotator(pitch=-55,yaw=100,roll=0))
assert levels.save_current_level()
report={'elements':counts,'seeds':seen,'active_seed':seed_mixed,'determinism':'passed','clear_preserves_authored_actors':'passed','meshes':mesh_paths}
(PROJECT/'Saved'/'ClusterVariantPOC.json').write_text(json.dumps(report,indent=2))
unreal.log('CLUSTER_POC_PASSED '+json.dumps(report))
