"""Rebake all six new maps with the real editor API and verify saved data."""
import sys
from pathlib import Path
import unreal

sys.dont_write_bytecode=True
sys.path.insert(0,str(Path(unreal.Paths.project_dir())/'Scripts'))
import build_sector_catalog as recipe
assets,actors,levels=recipe.base.assets,recipe.base.actors,recipe.base.levels

def signature(asset):
    result=[]
    for slot in asset.get_editor_property('cluster_slots'):
        t=slot.get_editor_property('transform')
        p,q,s=t.translation,t.rotation,t.scale3d
        result.append((str(slot.get_editor_property('slot_id')),slot.get_editor_property('shape').get_path_name(),
                       tuple(round(v,3) for v in (p.x,p.y,p.z,q.x,q.y,q.z,q.w,s.x,s.y,s.z))))
    return result

for name,start,placements in recipe.LAYOUTS:
    path=recipe.ROOT+'/SectorTemplates/ST_'+name
    assert levels.load_level(path+'/L_SectorTemplate_'+name)
    sector=next(a for a in actors.get_all_level_actors() if isinstance(a,unreal.PlanetSectorTemplateActor))
    asset=sector.get_editor_property('target_template')
    original=signature(asset)
    sector.bake_sector_template()
    assert signature(asset)==original,(name,'Bake altered existing slot data/order')
    assert asset.get_editor_property('can_be_starting_sector')==start
    assert len(original)==len(placements)
    assert asset.get_editor_property('sector_radius')==4000
    sector.generate_preview()
    preview=sector.get_editor_property('generated_preview')
    assert not preview.get_editor_property('diagnostics')
    assert len(preview.get_editor_property('selected_variant_ids'))==len(placements)
    assert assets.save_loaded_asset(asset)
    assert levels.save_current_level()
    unreal.log('CATALOG_BAKE_VERIFIED '+name)
assert levels.load_level(recipe.ROOT+'/SectorTemplates/ST_04_ShelteredPocket/L_SectorTemplate_04_ShelteredPocket')
unreal.log('CATALOG_AUTHORING_VERIFIED')
