"""Re-author the six reference silhouettes in the canonical project."""
import json
import math
from pathlib import Path
import shutil
from datetime import datetime
import unreal

PROJECT = Path(unreal.Paths.project_dir()).resolve()
assert str(PROJECT).lower() == r"C:\UE5\SurviveThePlanet 5.8".lower(), PROJECT
ROOT = '/Game/WorldGeneration'
LEVEL = ROOT + '/SectorTemplates/ST_First/L_SectorTemplate_First'
TEMPLATE = ROOT + '/SectorTemplates/ST_First/DA_SectorTemplate_First'
# Each spine describes the broad rhythm of a future rock/vegetation group.
# Widths vary along the spine; endpoints taper. These are footprints, not meshes.
def ribbon(samples):
    left, right = [], []
    for i, (x, y, width) in enumerate(samples):
        before = samples[max(0, i - 1)]
        after = samples[min(len(samples) - 1, i + 1)]
        dx, dy = after[0] - before[0], after[1] - before[1]
        length = math.hypot(dx, dy)
        nx, ny = -dy / length, dx / length
        left.append((x + nx * width, y + ny * width))
        right.append((x - nx * width, y - ny * width))
    # Endpoints have zero width and occur only once in the closed polygon.
    return left + list(reversed(right[1:-1]))


FOOTPRINTS = [
    ('01_Tapered', ribbon([
        (-.75,-.43,0),(-.71,-.50,.055),(-.61,-.57,.105),
        (-.50,-.57,.085),(-.41,-.50,.11),(-.36,-.41,.075),
        (-.29,-.33,.085),(-.20,-.29,.05),(-.14,-.25,0)])),
    ('02_Curved', ribbon([
        (-.81,-.16,0),(-.76,-.11,.07),(-.69,-.02,.10),
        (-.66,.10,.065),(-.58,.21,.10),(-.48,.28,.075),
        (-.37,.29,.10),(-.28,.34,.05),(-.22,.39,0)])),
    ('03_Long', ribbon([
        (-.64,.56,0),(-.55,.63,.065),(-.44,.66,.105),
        (-.33,.65,.095),(-.22,.60,.06),(-.12,.55,.085),
        (-.02,.53,.07),(.06,.51,.045),(.13,.46,0)])),
    ('04_Slender', ribbon([
        (.32,.74,0),(.43,.66,.075),(.56,.56,.105),
        (.64,.44,.065),(.60,.30,.09),(.50,.18,.07),
        (.44,.07,.095),(.48,-.04,.065),(.59,-.12,.06),
        (.72,-.18,0)])),
    ('05_Forked', ribbon([
        (.73,-.43,0),(.67,-.51,.08),(.59,-.58,.105),
        (.48,-.57,.065),(.40,-.49,.10),(.32,-.42,.075),
        (.22,-.38,.09),(.14,-.34,.05),(.09,-.30,0)])),
    ('06_SmallBlob', ribbon([
        (-.27,-.79,0),(-.19,-.80,.06),(-.09,-.82,.10),
        (.01,-.81,.095),(.10,-.77,.065),(.18,-.74,.08),
        (.25,-.72,0)])),
]

# Leave a narrow margin so the wider lobes do not cross the sector perimeter.
FOOTPRINTS = [(name, [(x*.93, y*.93) for x,y in points]) for name,points in FOOTPRINTS]

def crosses(a,b,c,d):
    def side(p,q,r):
        return (q[0]-p[0])*(r[1]-p[1])-(q[1]-p[1])*(r[0]-p[0])
    return side(a,b,c)*side(a,b,d)<-1e-12 and side(c,d,a)*side(c,d,b)<-1e-12

for name,points in FOOTPRINTS:
    edges=list(zip(points,points[1:]+points[:1]))
    for i,(a,b) in enumerate(edges):
        for c,d in edges[i+1:]:
            assert not crosses(a,b,c,d), ('self intersection',name)
for i,(name,points) in enumerate(FOOTPRINTS):
    for other,others in FOOTPRINTS[i+1:]:
        for a,b in zip(points,points[1:]+points[:1]):
            for c,d in zip(others,others[1:]+others[:1]):
                assert not crosses(a,b,c,d), ('overlap',name,other)

levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/PlanetLevel')
grids=[a for a in actors.get_all_level_actors() if isinstance(a,unreal.HexSectorGrid)]
assert len(grids)==1
radius=grids[0].get_editor_property('exploration_sector_radius')
for name, points in FOOTPRINTS:
    assert 6 <= len(points) <= 20, (name, len(points))
    for x, y in points:
        assert abs(x) <= math.sqrt(3)/2 and abs(y)+abs(x)/math.sqrt(3) <= 1, name
backup=PROJECT/'Saved'/'AuthoringBackups'/datetime.now().strftime('%Y%m%d_%H%M%S')
shutil.copytree(PROJECT/'Content'/'WorldGeneration',backup)
assert levels.load_level(LEVEL)
existing=actors.get_all_level_actors()
sectors=[a for a in existing if isinstance(a,unreal.PlanetSectorTemplateActor)]
assert len(sectors)==1
sector=sectors[0]
sector.set_actor_location(unreal.Vector(0,0,0),False,False)
sector.set_actor_rotation(unreal.Rotator(pitch=0,yaw=0,roll=0),False)
sector.set_actor_scale3d(unreal.Vector(1,1,1))
sector.set_editor_property('sector_radius',radius)
template=unreal.EditorAssetLibrary.load_asset(TEMPLATE)
template.set_editor_property('sector_radius',radius)
template.set_editor_property('sector_size',unreal.Vector2D(math.sqrt(3)*radius,2*radius))
slots=[]
for index,(name,perimeter) in enumerate(FOOTPRINTS,1):
    cx=sum(p[0] for p in perimeter)/len(perimeter)
    cy=sum(p[1] for p in perimeter)/len(perimeter)
    shape=unreal.EditorAssetLibrary.load_asset(ROOT+'/TerrainClusterShapes/DA_TerrainShape_'+name)
    shape.set_editor_property('points',[unreal.Vector2D((x-cx)*radius,(y-cy)*radius) for x,y in perimeter])
    unreal.EditorAssetLibrary.save_loaded_asset(shape)
    matches=[a for a in existing if isinstance(a,unreal.PlanetTerrainClusterShapeActor) and a.get_actor_label()==f'Cluster_{index:02}']
    assert len(matches)==1
    actor=matches[0]
    actor.set_actor_rotation(unreal.Rotator(pitch=0,yaw=0,roll=0),False)
    actor.set_actor_scale3d(unreal.Vector(1,1,1))
    actor.set_actor_location(unreal.Vector(cx*radius,cy*radius,0),False,False)
    actor.set_editor_property('shape',shape)
    actor.set_editor_property('owner_sector',sector)
    actor.set_editor_property('slot_id',str(index))
    slot=unreal.PlanetSectorClusterSlot()
    slot.set_editor_property('shape',shape)
    slot.set_editor_property('slot_id',str(index))
    slot.set_editor_property('transform',actor.get_actor_transform())
    slots.append(slot)
template.set_editor_property('cluster_slots',slots)
unreal.EditorAssetLibrary.save_loaded_asset(template)
unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(0,0,radius*2.5),unreal.Rotator(pitch=-90,yaw=-90,roll=0))
assert levels.save_current_level()
# Reload saved actors and export their actual world-space vertices for visual QA.
assert levels.load_level('/Game/PlanetLevel')
assert levels.load_level(LEVEL)
report={'radius':radius,'boundary':[],'shapes':[],'backup':str(backup)}
for i in range(6):
    angle=math.radians(30+i*60)
    report['boundary'].append([radius*math.cos(angle),radius*math.sin(angle)])
for actor in actors.get_all_level_actors():
    if not isinstance(actor,unreal.PlanetTerrainClusterShapeActor):
        continue
    points=actor.get_editor_property('shape').get_editor_property('points')
    transform=actor.get_actor_transform()
    world=[unreal.MathLibrary.transform_location(transform,unreal.Vector(p.x,p.y,0)) for p in points]
    for p in world:
        assert abs(p.z)<.01,(actor.get_actor_label(),'nonplanar',p)
        assert abs(p.x)<=math.sqrt(3)*radius/2+.1,(actor.get_actor_label(),p)
        assert abs(p.y)+abs(p.x)/math.sqrt(3)<=radius+.1,(actor.get_actor_label(),p)
    report['shapes'].append({'name':actor.get_actor_label(),'points':[[p.x,p.y] for p in world]})
assert len(report['shapes'])==6
assert len(unreal.EditorAssetLibrary.load_asset(TEMPLATE).get_editor_property('cluster_slots'))==6
(PROJECT/'Saved'/'FirstSectorVerification.json').write_text(json.dumps(report,indent=2))
unreal.log('FIRST_SECTOR_VERIFIED: six saved planar shapes inside PlanetLevel hex.')
