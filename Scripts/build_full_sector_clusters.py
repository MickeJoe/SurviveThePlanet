"""Author dense, editable compositions for all six existing footprints.

This is an asset-authoring recipe, not a new runtime scattering system.
Runtime selection and rendering use the existing cluster variant pipeline.
The earlier POC assets are deliberately retained for comparison.
"""
import json
import math
import random
from pathlib import Path
import unreal

PROJECT=Path(unreal.Paths.project_dir()).resolve()
assert str(PROJECT).lower()==r'C:\UE5\SurviveThePlanet 5.8'.lower()
ROOT='/Game/WorldGeneration/ClusterVariants/FullSector'
SECTOR='/Game/WorldGeneration/SectorTemplates/ST_First/L_SectorTemplate_First'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assets=unreal.EditorAssetLibrary
template=assets.load_asset('/Game/WorldGeneration/SectorTemplates/ST_First/DA_SectorTemplate_First')
slots=template.get_editor_property('cluster_slots')
assert len(slots)==6 and template.get_editor_property('sector_radius')==4000

mesh_paths={
    'large':'/Game/Environment/RockKit_Step01/SM_Rock_Large_01',
    'ridge':'/Game/Environment/Expedition02/Meshes/SM_Cliff_Ridge_02',
    'crown':'/Game/Environment/Expedition02/Meshes/SM_Cliff_Crown_02',
    'boulder':'/Game/Environment/Expedition02/Meshes/SM_Boulder_02',
    'medium':'/Game/Environment/RockKit_Step01/SM_Rock_Medium_01',
    'wedge':'/Game/Environment/Expedition04/Meshes/SM_Rock04_Wedge',
    'scree':'/Game/Environment/Expedition02/Meshes/SM_Scree_02',
    'pebbles':'/Game/Environment/Expedition05/Meshes/SM_Pebbles05_Scatter',
    'shards':'/Game/Environment/Expedition05/Meshes/SM_Pebbles05_Shards',
    'rosette':'/Game/Environment/Expedition02/Meshes/SM_Alien_Rosette_02',
    'trumpets':'/Game/Environment/Expedition02/Meshes/SM_Alien_Trumpets_02',
    'coral':'/Game/Environment/Expedition02/Meshes/SM_Alien_Coral_02',
    'fungus':'/Game/Environment/Expedition06/Meshes/SM_Alien06_DiscFungus',
    'shrub':'/Game/Environment/Expedition06/Meshes/SM_Alien06_LowShrub',
}
meshes={key:assets.load_asset(path) for key,path in mesh_paths.items()}
assert all(isinstance(m,unreal.StaticMesh) for m in meshes.values())

def make_asset(folder,name,cls):
    existing=assets.load_asset(folder+'/'+name) if assets.does_asset_exist(folder+'/'+name) else None
    if existing: return existing
    factory=unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class',cls)
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,folder,cls,factory)

def lighting():
    ground=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,-12))
    ground.set_actor_label('Authoring_Ground')
    ground.set_editor_property('is_editor_only_actor',True)
    ground.static_mesh_component.set_static_mesh(assets.load_asset('/Engine/BasicShapes/Plane'))
    ground.static_mesh_component.set_material(0,assets.load_asset('/Game/Environment/Expedition02/Materials/M_Terrain_Expedition02'))
    ground.set_actor_scale3d(unreal.Vector(160,65,1))
    for name,yaw,power in [('Key',-35,5.0),('Fill',145,1.5)]:
        light=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,2500),unreal.Rotator(pitch=-48,yaw=yaw,roll=0))
        light.set_actor_label('Authoring_'+name)
        light.set_editor_property('is_editor_only_actor',True)
        light.light_component.set_editor_property('intensity',power)
        light.light_component.set_editor_property('forward_shading_priority',1 if name=='Key' else 0)

def compose(shape,variant_index,shape_index):
    points=[(p.x,p.y) for p in shape.get_editor_property('points')]
    # Existing ribbon footprints consist of two paired sides and tapered ends.
    half=len(points)//2
    spine=[points[0]]+[((points[i][0]+points[-i][0])/2,(points[i][1]+points[-i][1])/2) for i in range(1,half)]+[points[half]]
    widths=[0]+[math.dist(points[i],points[-i])/2 for i in range(1,half)]+[0]
    distance=[0]
    for a,b in zip(spine,spine[1:]): distance.append(distance[-1]+math.dist(a,b))
    length=distance[-1]
    rng=random.Random(91000+shape_index*101+variant_index*23)
    elements=[]

    def frame(t):
        d=max(0,min(1,t))*length
        i=next((i for i in range(len(distance)-1) if distance[i+1]>=d),len(distance)-2)
        alpha=(d-distance[i])/(distance[i+1]-distance[i])
        a,b=spine[i],spine[i+1]
        yaw=math.atan2(b[1]-a[1],b[0]-a[0])
        return (a[0]+(b[0]-a[0])*alpha,a[1]+(b[1]-a[1])*alpha,widths[i]*(1-alpha)+widths[i+1]*alpha,yaw)

    def inside(x,y):
        hit=False
        for a,b in zip(points,points[1:]+points[:1]):
            if (a[1]>y)!=(b[1]>y) and x<(b[0]-a[0])*(y-a[1])/(b[1]-a[1])+a[0]: hit=not hit
        return hit

    def add(key,t,side,size,yaw_offset=0,height=1):
        x,y,width,yaw=frame(t)
        x-=math.sin(yaw)*width*side
        y+=math.cos(yaw)*width*side
        rotation=yaw+math.radians(yaw_offset)
        mesh=meshes[key]
        bounds=mesh.get_bounding_box()
        scale=size/max(bounds.max.x-bounds.min.x,bounds.max.y-bounds.min.y)
        cx=(bounds.min.x+bounds.max.x)*.5
        cy=(bounds.min.y+bounds.max.y)*.5
        # Fit the rotated XY bounds inside the approximate footprint. This also
        # keeps large anchor rocks away from the open buildable corridors.
        for attempt in range(16):
            corners=[(x+((px-cx)*math.cos(rotation)-(py-cy)*math.sin(rotation))*scale,
                      y+((px-cx)*math.sin(rotation)+(py-cy)*math.cos(rotation))*scale)
                     for px in [bounds.min.x,bounds.max.x] for py in [bounds.min.y,bounds.max.y]]
            if all(inside(px,py) for px,py in corners): break
            scale*=.90
        else: return
        px=x-(cx*math.cos(rotation)-cy*math.sin(rotation))*scale
        py=y-(cx*math.sin(rotation)+cy*math.cos(rotation))*scale
        transform=unreal.Transform(location=unreal.Vector(px,py,-bounds.min.z*scale*height-3),
                                   rotation=unreal.Rotator(pitch=0,yaw=math.degrees(rotation),roll=0),
                                   scale=unreal.Vector(scale,scale,scale*height))
        elements.append((key,transform))

    # Uneven, overlapping rock groups form a continuous but irregular backbone.
    anchors=[.20,.48,.76] if shape_index!=6 else [.28,.66]
    for i,t in enumerate(anchors):
        key=['large','crown','ridge'][(i+shape_index+variant_index)%3]
        add(key,t,.10*(-1 if i%2 else 1),[720,640,540][variant_index],rng.uniform(-22,22),.86)
    n=max(7,round(length/220))
    for i in range(n):
        t=.06+.88*(i+.16*math.sin(i*2.4))/(n-1)
        side=.15*math.sin(i*2.1+shape_index)
        add(['boulder','medium','wedge'][(i+variant_index)%3],t,side,rng.uniform(270,440)*[1.10,1,.87][variant_index],rng.uniform(-85,85))
        add('boulder',t+.015,(-1 if i%2 else 1)*.48,rng.uniform(155,265),rng.uniform(0,360),.85)

    # Planted shoulders occupy both sides and connect the anchor groups. Their
    # species form patches, rather than choosing an unrelated mesh per point.
    count=max(14,round(length/105))
    palette=['trumpets','rosette','coral','rosette','shrub','fungus']
    for i in range(count):
        t=.045+.91*i/(count-1)
        for side in [-1,1]:
            species=palette[(i//3+shape_index+(1 if side>0 else 0)+variant_index)%len(palette)]
            offset=rng.uniform(.48,.78)*side
            add(species,t+rng.uniform(-.012,.012),offset,rng.uniform(140,205),rng.uniform(0,360))
            if i%3!=1 or variant_index==2:
                add('rosette' if species!='rosette' else 'shrub',t+.018,side*.38,rng.uniform(100,165),rng.uniform(0,360))
        if variant_index==2 and i%2==0:
            add('trumpets' if i%4 else 'coral',t,.12,rng.uniform(170,225),rng.uniform(0,360))

    # Fine debris ties rocks to the soil, including the tapered footprint ends.
    for i in range(max(18,round(length/85))):
        t=.025+.95*i/(max(18,round(length/85))-1)
        for side in [-1,1]:
            add('pebbles' if i%3 else 'scree',t,side*rng.uniform(.55,.83),rng.uniform(175,285),rng.uniform(0,360))
        if i%2==0: add('shards',t,.12,rng.uniform(150,240),rng.uniform(0,360))
    return elements

variants=[]
report={'shapes':{},'meshes':mesh_paths}
for shape_index,slot in enumerate(slots,1):
    shape=slot.get_editor_property('shape')
    folder=ROOT+f'/Shape{shape_index:02}'
    assets.make_directory(folder)
    level=folder+f'/L_ClusterVariants_{shape_index:02}_Dense'
    # Never discard a previously hand-edited authoring map on a recipe rerun.
    if assets.does_asset_exist(level):
        raise RuntimeError('Authoring map already exists; edit/bake it rather than rebuilding: '+level)
    assert levels.new_level(level)
    lighting()
    counts=[]
    for variant_index,label in enumerate(['A_Rock','B_Mixed','C_Vegetation']):
        variant=make_asset(folder,f'DA_Cluster{shape_index:02}_{label}_Dense',unreal.PlanetTerrainClusterVariant)
        variant.set_editor_property('variant_id',f'Cluster{shape_index:02}_{label}_Dense')
        root=actors.spawn_actor_from_class(unreal.PlanetTerrainClusterAuthoringActor,unreal.Vector((variant_index-1)*5000,0,0))
        root.set_actor_label(f'Author_{shape_index:02}_{label}_Dense')
        root.set_editor_property('shape',shape)
        root.set_editor_property('slot_id',label)
        root.set_editor_property('target_cluster_variant',variant)
        members=[]
        for index,(key,transform) in enumerate(compose(shape,variant_index,shape_index)):
            p=transform.translation+root.get_actor_location()
            actor=actors.spawn_actor_from_class(unreal.StaticMeshActor,p,transform.rotation.rotator())
            actor.set_actor_label(f'{label}_{index:03}_{key}')
            actor.static_mesh_component.set_static_mesh(meshes[key])
            actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
            actor.set_actor_scale3d(transform.scale3d)
            actor.attach_to_actor(root,'',unreal.AttachmentRule.KEEP_WORLD,unreal.AttachmentRule.KEEP_WORLD,unreal.AttachmentRule.KEEP_WORLD,False)
            members.append(actor)
        root.set_editor_property('mesh_actors',members)
        root.bake_cluster_variant()
        assert len(variant.get_editor_property('elements'))==len(members)>80
        assets.save_loaded_asset(variant)
        variants.append(variant)
        counts.append(len(members))
    unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(0,-4400,7200),unreal.Rotator(pitch=-58,yaw=90,roll=0))
    assert levels.save_current_level()
    report['shapes'][str(shape_index)]=counts
    unreal.log(f'DENSE_SHAPE_COMPLETE {shape_index}: {counts}')

library=make_asset(ROOT,'DA_ClusterVariantLibrary_FullSector',unreal.PlanetTerrainClusterLibrary)
library.set_editor_property('variants',variants)
assets.save_loaded_asset(library)
assert levels.load_level(SECTOR)
sector=next(a for a in actors.get_all_level_actors() if isinstance(a,unreal.PlanetSectorTemplateActor))
authored=[a.get_path_name() for a in actors.get_all_level_actors() if isinstance(a,unreal.PlanetTerrainClusterShapeActor)]
sector.set_editor_property('variant_library',library)
sector.set_editor_property('preview_seed',4)

def snapshot():
    generated=sector.get_editor_property('generated_preview')
    values=[]
    for component in generated.get_components_by_class(unreal.InstancedStaticMeshComponent):
        for i in range(component.get_instance_count()):
            t=component.get_instance_transform(i,False)
            p,q,s=t.translation,t.rotation,t.scale3d
            values.append((component.static_mesh.get_path_name(),(p.x,p.y,p.z,q.x,q.y,q.z,q.w,s.x,s.y,s.z)))
    return [str(v) for v in generated.get_editor_property('selected_variant_ids')],values

sector.generate_preview()
first=snapshot()
assert len(first[0])==6 and 'None' not in first[0]
assert not sector.get_editor_property('generated_preview').get_editor_property('diagnostics')
sector.generate_preview()
assert snapshot()==first
sector.clear_preview()
assert authored==[a.get_path_name() for a in actors.get_all_level_actors() if isinstance(a,unreal.PlanetTerrainClusterShapeActor)]
assert not any(isinstance(a,unreal.PlanetGeneratedSector) for a in actors.get_all_level_actors())
sector.generate_preview()
assert snapshot()==first
unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(700,-5200,6700),unreal.Rotator(pitch=-52,yaw=98,roll=0))
assert levels.save_current_level()
report.update({'selected':first[0],'instances':len(first[1]),'seed':4,'determinism':'passed','clear_preserves_shapes':'passed'})
(PROJECT/'Saved'/'FullSectorClusters.json').write_text(json.dumps(report,indent=2))
unreal.log('FULL_SECTOR_COMPLETE '+json.dumps(report))
