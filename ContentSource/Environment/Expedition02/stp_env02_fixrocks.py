import bpy, json, os
from mathutils import Vector, noise
OUT=r'C:\UE5\SurviveThePlanet 5.8\ContentSource\Environment\Expedition02'
sc=bpy.context.scene
rockmat=next(m for m in bpy.data.materials if m.name.startswith('M_Blender_FracturedBasalt') and '.001' in m.name)
for ob in list(sc.objects):
 if ob.type!='MESH' or not ob.name.startswith(('SM_Cliff','SM_Boulder','SM_Scree')):continue
 name=ob.name.split('.')[0];offset=ob.location.copy();ob.location=(0,0,0)
 ob.data.materials.clear();ob.data.materials.append(rockmat)
 for p in ob.data.polygons:p.material_index=0
 bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob
 # Break the uniform polygon edges with controlled medium scale chipping.
 sub=ob.modifiers.new('Fracture edge support','SUBSURF');sub.subdivision_type='SIMPLE';sub.levels=1
 bpy.ops.object.modifier_apply(modifier=sub.name)
 for v in ob.data.vertices:
  p=v.co.copy();amp=.032 if 'Scree' not in name else .009
  v.co += Vector((noise.noise(p*7.3),noise.noise(p*7.3+Vector((20,5,1))),noise.noise(p*7.3+Vector((3,24,5)))))*amp
 bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(angle_limit=1.2,island_margin=.009);bpy.ops.object.mode_set(mode='OBJECT')
 for typ,suffix in [('DIFFUSE','BaseColor'),('NORMAL','Normal')]:
  im=bpy.data.images.new('Fixed_'+name+'_'+suffix,width=2048,height=2048)
  if typ=='NORMAL':im.colorspace_settings.name='Non-Color'
  node=rockmat.node_tree.nodes.new('ShaderNodeTexImage');node.image=im;rockmat.node_tree.nodes.active=node
  bpy.ops.object.bake(type=typ,pass_filter={'COLOR'} if typ=='DIFFUSE' else {'DIRECT','INDIRECT','COLOR'})
  im.filepath_raw=os.path.join(OUT,'T_'+name+'_'+suffix+'.png');im.file_format='PNG';im.save()
 tri=ob.modifiers.new('Export triangles','TRIANGULATE');bpy.ops.object.modifier_apply(modifier=tri.name)
 bpy.ops.export_scene.fbx(filepath=os.path.join(OUT,name+'.fbx'),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',add_leaf_bones=False,bake_anim=False,mesh_smooth_type='FACE',path_mode='STRIP')
 ob.location=offset
sc.render.filepath=os.path.join(OUT,'Expedition02_Studio.png');bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT,'STP_Expedition02.blend'),copy=True);bpy.ops.render.render(write_still=True)
print('ROCKS_FIXED')
