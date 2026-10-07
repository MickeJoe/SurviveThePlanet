import bpy,bmesh,json
from pathlib import Path
from mathutils import Vector
root=Path(r"C:\UE5\SurviveThePlanet 5.8");folder=root/"ContentSource/ConnectorFactory"
bpy.ops.wm.open_mainfile(filepath=str(folder/"ConnectorFactory.blend"))
body=bpy.data.objects["SM_ConnectorFactory"]
assert not body.get("animated_parts_v1"),"Already extracted"
bm=bmesh.new();bm.from_mesh(body.data);bm.verts.ensure_lookup_table()
remaining=set(bm.verts);groups=[]
while remaining:
 seed=remaining.pop();group={seed};stack=[seed]
 while stack:
  vertex=stack.pop()
  for edge in vertex.link_edges:
   other=edge.other_vert(vertex)
   if other in remaining:
    remaining.remove(other);group.add(other);stack.append(other)
 groups.append(group)
selected={"FanA":[],"FanB":[],"Product":[]};remove=[]
for group in groups:
 lo=Vector(tuple(min(v.co[i] for v in group) for i in range(3)));hi=Vector(tuple(max(v.co[i] for v in group) for i in range(3)));center=(lo+hi)/2
 if lo.z>2.42 and hi.z<2.47 and abs(center.y-.38)<.5:
  selected["FanA" if center.x<-.335 else "FanB"].extend(group);remove.extend(group)
 elif lo.z>1.035 and hi.z<1.225 and -.97<center.y<-.65 and any(abs(center.x-x)<.2 for x in [-.55,.25,1.12]):
  if abs(center.x-.25)<.2:selected["Product"].extend(group)
  remove.extend(group)
origins={"FanA":Vector((-.82,.38,2.44)),"FanB":Vector((.15,.38,2.44)),"Product":Vector((.25,-.75,1.12))}
report={}
for name,verts in selected.items():
 assert verts,name
 copy=bm.copy();keep={v.index for v in verts}
 bmesh.ops.delete(copy,geom=[v for v in copy.verts if v.index not in keep],context='VERTS')
 for v in copy.verts:v.co-=origins[name]
 mesh=bpy.data.meshes.new("SM_Connector"+name);copy.to_mesh(mesh);copy.free()
 obj=bpy.data.objects.new(mesh.name,mesh);bpy.context.collection.objects.link(obj)
 for material in body.data.materials:mesh.materials.append(material)
 bpy.ops.object.select_all(action="DESELECT");obj.select_set(True);bpy.context.view_layer.objects.active=obj
 with bpy.context.temp_override(selected_objects=[obj],active_object=obj,object=obj):
  bpy.ops.export_scene.fbx(filepath=str(folder/(mesh.name+".fbx")),use_selection=True,apply_unit_scale=True,axis_forward="-Y",axis_up="Z",bake_anim=False,object_types={"MESH"})
 obj.location=origins[name];report[name]=len(mesh.vertices)
bmesh.ops.delete(bm,geom=list(set(remove)),context='VERTS');bm.to_mesh(body.data);bm.free()
body["animated_parts_v1"]=True
bpy.ops.object.select_all(action="DESELECT");body.select_set(True);bpy.context.view_layer.objects.active=body
with bpy.context.temp_override(selected_objects=[body],active_object=body,object=body):
 bpy.ops.export_scene.fbx(filepath=str(folder/"SM_ConnectorFactory.fbx"),use_selection=True,apply_unit_scale=True,axis_forward="-Y",axis_up="Z",bake_anim=False,object_types={"MESH"})
bpy.ops.wm.save_as_mainfile(filepath=str(folder/"ConnectorFactory.blend"))
result=report

