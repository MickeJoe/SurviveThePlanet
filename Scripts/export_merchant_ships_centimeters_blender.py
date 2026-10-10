"""FBX vertex coordinates in UE centimetres, keeping each moving part's pivot."""
import bpy,json
from pathlib import Path
from mathutils import Matrix
root=Path(r'C:\UE5\SurviveThePlanet 5.8\ContentSource\Trading\Ships')
records=json.loads((root/'MerchantShips.json').read_text())
exports=[('SM_'+r['merchant']+'_'+part,root/r['merchant']) for r in records for part in ['Body','Gear','Ramp']]
exports.extend([(name,root) for name in ['SM_MerchantExhaust','SM_MerchantDust']])
scene=bpy.context.scene
for name,folder in exports:
    original=bpy.data.objects[name];o=original.copy();o.data=original.data.copy();scene.collection.objects.link(o)
    o.parent=None;o.animation_data_clear();o.matrix_world=Matrix.Identity(4);o.hide_render=False;o.hide_set(False)
    o.data.transform(Matrix.Scale(100,4))
    bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
    bpy.ops.export_scene.fbx(filepath=str(folder/(name+'.fbx')),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',bake_anim=False,apply_unit_scale=False,mesh_smooth_type='FACE')
    bpy.data.objects.remove(o,do_unlink=True)
result={'centimetre_exports':len(exports),'source_blends_preserved':True}
