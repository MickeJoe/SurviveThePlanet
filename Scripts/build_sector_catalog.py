"""Author six additional templates; preserve ST_First and its six footprints.

Run in the UE editor. New assets use the existing shape / variant / template
bake pipeline. Refuses to replace existing authoring maps on a second run.
"""
import math
import json
import sys
from pathlib import Path
import unreal

sys.dont_write_bytecode=True
sys.path.insert(0, str(Path(unreal.Paths.project_dir()) / 'Scripts'))
import build_full_sector_clusters as base

ROOT = '/Game/WorldGeneration'
RADIUS = 4000.0
NEW_SHAPES = {
    '07_Crescent': [(-1050,-180,0),(-780,100,200),(-400,290,250),(0,360,260),(400,290,230),(780,90,180),(1050,-180,0)],
    '08_Kidney': [(-850,-130,0),(-570,40,340),(-220,120,490),(180,50,440),(510,-90,290),(780,-120,0)],
    '09_Elbow': [(-850,-450,0),(-520,-410,250),(-130,-340,300),(190,-170,310),(340,160,260),(390,530,180),(410,850,0)],
    '10_Winding': [(-1150,-360,0),(-860,-180,190),(-580,100,240),(-220,260,260),(120,190,230),(450,-90,250),(790,-190,180),(1110,-80,0)],
    '11_Horseshoe': [(-700,650,0),(-760,250,190),(-680,-180,230),(-430,-510,250),(0,-620,280),(430,-510,250),(680,-180,230),(760,250,190),(700,650,0)],
    '12_Fan': [(-950,-170,0),(-660,-30,230),(-360,120,490),(-40,240,330),(240,240,620),(490,110,310),(760,-30,430),(1040,-260,0)],
}
# Shape number, center in sector-radius units, desired long-axis angle, length cm.
LAYOUTS = [
    ('01_OpenBase', True, [(1,-.52,-.32,48,2300),(2,-.54,.30,65,2250),(3,0,.69,0,2250),(4,.53,.30,-64,2450),(5,.52,-.33,-48,2200),(6,0,-.76,0,1550)]),
    ('02_BroadPassage', True, [(3,-.52,-.35,55,2200),(4,.52,.36,55,2450),(7,-.20,.67,10,2050),(9,.23,-.60,-25,1800),(6,-.62,.22,65,1400),(8,.60,-.17,65,1550)]),
    ('03_Islands', False, [(6,-.51,-.40,25,1600),(8,-.54,.28,60,1700),(10,.05,.64,5,2050),(12,.48,.15,75,1800),(6,.40,-.51,-30,1450),(8,-.05,-.15,20,1450)]),
    ('04_ShelteredPocket', False, [(11,-.24,.15,0,3000),(2,.47,.35,-65,2300),(9,.36,-.45,20,2000),(6,-.25,-.72,0,1500),(4,-.59,-.32,70,1700)]),
    ('05_Winding', False, [(10,-.08,.04,65,3100),(1,-.53,-.40,45,2000),(3,.14,.68,-10,2000),(7,.56,.20,-75,2000),(8,.41,-.46,-35,1800),(6,-.59,.34,65,1400)]),
    ('06_ThreeClearings', False, [(12,.03,.12,-25,2600),(5,-.52,-.37,48,2100),(7,.49,-.40,-38,2050),(9,.51,.32,-65,1950),(6,-.04,-.77,0,1400),(8,-.42,.49,35,1900)]),
]

def ribbon(samples):
    left, right = [], []
    for i, (x,y,width) in enumerate(samples):
        a,b = samples[max(0,i-1)], samples[min(len(samples)-1,i+1)]
        length = math.hypot(b[0]-a[0], b[1]-a[1])
        nx,ny = -(b[1]-a[1])/length, (b[0]-a[0])/length
        left.append((x+nx*width,y+ny*width))
        right.append((x-nx*width,y-ny*width))
    return left + list(reversed(right[1:-1]))

def inside(p, polygon):
    x,y=p
    hit=False
    for a,b in zip(polygon,polygon[1:]+polygon[:1]):
        if (a[1]>y)!=(b[1]>y) and x<(b[0]-a[0])*(y-a[1])/(b[1]-a[1])+a[0]: hit=not hit
    return hit

def overlap(a,b):
    def cross(p,q,r): return (q[0]-p[0])*(r[1]-p[1])-(q[1]-p[1])*(r[0]-p[0])
    if any(inside(p,b) for p in a) or any(inside(p,a) for p in b): return True
    return any(cross(p,q,r)*cross(p,q,s)<0 and cross(r,s,p)*cross(r,s,q)<0
               for p,q in zip(a,a[1:]+a[:1]) for r,s in zip(b,b[1:]+b[:1]))

def fitted_transform(shape, x, y, angle, length, previous, start):
    points=[(p.x,p.y) for p in shape.get_editor_property('points')]
    # Align the principal axis, keeping the actual authored contour unchanged.
    cx=sum(p[0] for p in points)/len(points); cy=sum(p[1] for p in points)/len(points)
    xx=sum((p[0]-cx)**2 for p in points); yy=sum((p[1]-cy)**2 for p in points)
    xy=sum((p[0]-cx)*(p[1]-cy) for p in points)
    principal=.5*math.atan2(2*xy,xx-yy)
    projected=[p[0]*math.cos(principal)+p[1]*math.sin(principal) for p in points]
    scale=length/(max(projected)-min(projected))
    yaw=math.radians(angle)-principal
    co,si=math.cos(yaw),math.sin(yaw)
    for attempt in range(16):
        polygon=[(x*RADIUS+((px-cx)*co-(py-cy)*si)*scale,
                  y*RADIUS+((px-cx)*si+(py-cy)*co)*scale) for px,py in points]
        in_hex=all(abs(px)<math.sqrt(3)*RADIUS/2-100 and abs(py)+abs(px)/math.sqrt(3)<RADIUS-100 for px,py in polygon)
        # HQ center remains free in both new starting templates.
        hq=[(-1050,-900),(1050,-900),(1050,900),(-1050,900)]
        if in_hex and not any(overlap(polygon,p) for p in previous) and not (start and overlap(polygon,hq)):
            assert attempt<12, 'Layout required excessive shrinking'
            return unreal.Transform(location=unreal.Vector(x*RADIUS-(cx*co-cy*si)*scale,y*RADIUS-(cx*si+cy*co)*scale,0),
                rotation=unreal.Rotator(pitch=0,yaw=math.degrees(yaw),roll=0),scale=unreal.Vector(scale,scale,scale)), polygon
        scale*=.95
    raise RuntimeError('Cannot fit authored shape without overlap')

def main():
    assets,actors,levels=base.assets,base.actors,base.levels
    resume='--templates-only' in sys.argv
    # Preflight before any asset mutation; never silently replace manual edits.
    for name in NEW_SHAPES:
        assert resume or not assets.does_asset_exist(base.ROOT+'/Shape'+name[:2]+f'/L_ClusterVariants_{name[:2]}_Dense'), 'New authoring map already exists'
    for name,_,_ in LAYOUTS:
        assert resume or not assets.does_asset_exist(ROOT+'/SectorTemplates/ST_'+name+'/L_SectorTemplate_'+name)
    shapes={i:slot.get_editor_property('shape') for i,slot in enumerate(base.slots,1)}
    library=assets.load_asset(base.ROOT+'/DA_ClusterVariantLibrary_FullSector')
    variants=list(library.get_editor_property('variants'))
    assert len(variants)==(36 if resume else 18)
    report={'shapes':{},'templates':[]}
    for name,samples in NEW_SHAPES.items():
        number=int(name[:2])
        if resume:
            shapes[number]=assets.load_asset(ROOT+'/TerrainClusterShapes/DA_TerrainShape_'+name)
            continue
        shape=base.make_asset(ROOT+'/TerrainClusterShapes','DA_TerrainShape_'+name,unreal.PlanetTerrainClusterShape)
        shape.set_editor_property('shape_id',name)
        shape.set_editor_property('points',[unreal.Vector2D(x,y) for x,y in ribbon(samples)])
        assets.save_loaded_asset(shape)
        shapes[number]=shape
        folder=base.ROOT+f'/Shape{number:02}'
        assert levels.new_level(folder+f'/L_ClusterVariants_{number:02}_Dense')
        base.lighting()
        counts=[]
        for index,label in enumerate(['A_Rock','B_Mixed','C_Vegetation']):
            variant=base.make_asset(folder,f'DA_Cluster{number:02}_{label}_Dense',unreal.PlanetTerrainClusterVariant)
            variant.set_editor_property('variant_id',f'Cluster{number:02}_{label}_Dense')
            root=actors.spawn_actor_from_class(unreal.PlanetTerrainClusterAuthoringActor,unreal.Vector((index-1)*5000,0,0))
            root.set_actor_label(f'Author_{number:02}_{label}')
            root.set_editor_property('shape',shape)
            root.set_editor_property('target_cluster_variant',variant)
            members=[]
            for n,(key,t) in enumerate(base.compose(shape,index,number)):
                actor=actors.spawn_actor_from_class(unreal.StaticMeshActor,t.translation+root.get_actor_location(),t.rotation.rotator())
                actor.set_actor_label(f'{label}_{n:03}_{key}')
                actor.static_mesh_component.set_static_mesh(base.meshes[key])
                actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
                actor.set_actor_scale3d(t.scale3d)
                actor.attach_to_actor(root,'',unreal.AttachmentRule.KEEP_WORLD,unreal.AttachmentRule.KEEP_WORLD,unreal.AttachmentRule.KEEP_WORLD,False)
                members.append(actor)
            root.set_editor_property('mesh_actors',members)
            root.bake_cluster_variant()
            assert len(variant.get_editor_property('elements'))==len(members)>80
            assets.save_loaded_asset(variant)
            variants.append(variant)
            counts.append(len(members))
        assert levels.save_current_level()
        report['shapes'][name]=counts
        unreal.log('CATALOG_SHAPE_COMPLETE '+name)
    library.set_editor_property('variants',variants)
    assets.save_loaded_asset(library)
    first=base.template
    first.set_editor_property('can_be_starting_sector',True)
    assets.save_loaded_asset(first)
    catalog=[first]
    for name,start,placements in LAYOUTS:
        folder=ROOT+'/SectorTemplates/ST_'+name
        asset=base.make_asset(folder,'DA_SectorTemplate_'+name,unreal.PlanetSectorTemplate)
        asset.set_editor_property('template_id',name)
        asset.set_editor_property('can_be_starting_sector',start)
        level_path=folder+'/L_SectorTemplate_'+name
        assert levels.load_level(level_path) if assets.does_asset_exist(level_path) else levels.new_level(level_path)
        assert not any(isinstance(a,unreal.PlanetSectorTemplateActor) for a in actors.get_all_level_actors()), 'Refusing to overwrite an authored template map'
        base.lighting()
        ground=next(a for a in actors.get_all_level_actors() if a.get_actor_label()=='Authoring_Ground')
        ground.set_actor_scale3d(unreal.Vector(85,85,1))
        sector=actors.spawn_actor_from_class(unreal.PlanetSectorTemplateActor,unreal.Vector())
        sector.set_actor_label('SectorTemplate_'+name)
        sector.set_editor_property('target_template',asset)
        sector.set_editor_property('sector_radius',RADIUS)
        sector.set_editor_property('variant_library',library)
        previous=[]
        for index,(number,x,y,angle,length) in enumerate(placements,1):
            t,polygon=fitted_transform(shapes[number],x,y,angle,length,previous,start)
            previous.append(polygon)
            actor=actors.spawn_actor_from_class(unreal.PlanetTerrainClusterShapeActor,t.translation,t.rotation.rotator())
            actor.set_actor_scale3d(t.scale3d)
            actor.set_actor_label(f'Cluster_{index:02}_Shape{number:02}')
            actor.set_editor_property('shape',shapes[number])
            actor.set_editor_property('owner_sector',sector)
            actor.set_editor_property('slot_id',f'{index:02}')
        sector.bake_sector_template()
        assert len(asset.get_editor_property('cluster_slots'))==len(placements)
        assets.save_loaded_asset(asset)
        sector.generate_preview()
        preview=sector.get_editor_property('generated_preview')
        assert len(preview.get_editor_property('selected_variant_ids'))==len(placements)
        assert not preview.get_editor_property('diagnostics')
        unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(0,-5200,7200),unreal.Rotator(pitch=-54,yaw=90,roll=0))
        assert levels.save_current_level()
        catalog.append(asset)
        report['templates'].append({'name':name,'start':start,'polygons':previous})
        unreal.log('CATALOG_TEMPLATE_COMPLETE '+name)
    assert levels.load_level('/Game/WorldGeneration/Maps/L_PlanetClusters')
    population=next(a for a in actors.get_all_level_actors() if isinstance(a,unreal.SectorPopulation))
    population.set_editor_property('authored_sector_templates',catalog)
    population.set_editor_property('cluster_variant_library',library)
    grid=population.get_editor_property('grid')
    assert grid.get_editor_property('exploration_sector_radius')==RADIUS
    assert len(grid.get_editor_property('sectors'))==37
    assert len(catalog)==7 and sum(t.get_editor_property('can_be_starting_sector') for t in catalog)==3
    assert levels.save_current_level()
    (base.PROJECT/'Saved'/'SectorCatalog.json').write_text(json.dumps(report,indent=2))
    unreal.log('SECTOR_CATALOG_COMPLETE: 7 templates, 12 shapes, 36 variants, 3 eligible starts')

if __name__=='__main__':
    main()
