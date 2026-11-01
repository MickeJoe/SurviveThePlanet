import unreal,json,time,traceback,math
from pathlib import Path
root=Path('C:/UE5/SurviveThePlanet 5.8')
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
started=time.monotonic()
def scan_tick(dt):
    try:
        world=editor.get_game_world()
        if not world or time.monotonic()-started<6:return
        grid=unreal.GameplayStatics.get_actor_of_class(world,unreal.HexSectorGrid)
        surface=unreal.GameplayStatics.get_actor_of_class(world,unreal.PlanetSurfaceManager)
        hq=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.BaseBuilding) if a.get_building_type()==unreal.STPBuildingType.BASE_MODULE)
        sid=grid.get_editor_property('starting_sector_id')
        sector=next(s for s in grid.get_editor_property('sectors') if s.id==sid)
        sources=[s for s in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.BaseResourceSource) if grid.get_sector_at_world_location(s.get_actor_location())==sid and s.get_mine_blueprint()]
        stone=next(s for s in sources if s.get_resource_type()==unreal.ResourceType.STONE)
        mine=unreal.get_default_object(stone.get_mine_blueprint());fp=mine.get_grid_footprint()
        clearance=surface.get_building_clearance_cells()
        target=hq.get_actor_location()+unreal.Vector(2100,-1100,0)
        others=[]
        for s in sources:
            if s==stone:continue
            m=unreal.get_default_object(s.get_mine_blueprint());f=m.get_grid_footprint()
            p=surface.get_placement_for_world_location(m.get_placement_transform_for_source(s).translation,f)
            others.append((s.get_actor_location(),p.origin_cell,f))
        counts={'terrain':0,'bounds':0,'separation':0,'valid':0};found=[]
        for dx in range(0,3401,100):
            for dy in range(-3000,2001,100):
                loc=hq.get_actor_location()+unreal.Vector(dx,dy,0);loc.z=stone.get_actor_location().z
                tr=mine.get_placement_transform_for_source_at_transform(stone,unreal.Transform(location=loc))
                p=surface.get_placement_for_world_location(tr.translation,fp)
                if not surface.has_terrain_clearance(p.origin_cell,fp,clearance+1):counts['terrain']+=1;continue
                radius=math.hypot(tr.translation.x-sector.world_center.x,tr.translation.y-sector.world_center.y)
                if radius<1500 or radius>grid.get_editor_property('exploration_sector_radius')-650:counts['bounds']+=1;continue
                step=surface.get_tile_spacing()
                if any(grid.get_sector_at_world_location(p.world_location+unreal.Vector(x*fp.x*step*.5,y*fp.y*step*.5,0))!=sid for x in [-1,1] for y in [-1,1]):counts['bounds']+=1;continue
                separated=True
                for other,origin,foot in others:
                    a=p.origin_cell;b=origin
                    if math.hypot(loc.x-other.x,loc.y-other.y)<1400 or not (a.x+fp.x-1+clearance<b.x or b.x+foot.x-1+clearance<a.x or a.y+fp.y-1+clearance<b.y or b.y+foot.y-1+clearance<a.y):separated=False;break
                if not separated:counts['separation']+=1;continue
                counts['valid']+=1
                found.append(dict(location=[loc.x,loc.y,loc.z],offset=[dx,dy],distance=math.hypot(loc.x-target.x,loc.y-target.y)))
        found.sort(key=lambda x:x['distance'])
        (root/'Saved/StonePlacementScan.json').write_text(json.dumps(dict(counts=counts,candidates=found[:30],hq=str(hq.get_actor_location()),sources=[dict(name=s.get_name(),location=str(s.get_actor_location())) for s in sources]),indent=2))
        unreal.unregister_slate_post_tick_callback(scan_handle)
        unreal.log('STONE SCAN COMPLETE')
    except:
        (root/'Saved/StonePlacementScan.json').write_text(json.dumps(dict(error=traceback.format_exc())))
        unreal.unregister_slate_post_tick_callback(scan_handle)
levels.load_level('/Game/Planet/Maps/L_Nexaris')
scan_handle=unreal.register_slate_post_tick_callback(scan_tick)
levels.editor_request_begin_play()
