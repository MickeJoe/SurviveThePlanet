import bpy, math, os, json
from mathutils import Vector
root=r"C:\UE5\SurviveThePlanet 5.8\Assets\Source\PowerConnection"
os.makedirs(root,exist_ok=True)
scene=bpy.data.scenes.new("STP_PowerConnection_Assets")
bpy.context.window.scene=scene
scene.unit_settings.system="METRIC"
scene.unit_settings.scale_length=1
parts=[]
def mat(name,color,metal,rough,emission=0):
 m=bpy.data.materials.new(name); m.diffuse_color=(*color,1); m.use_nodes=True
 p=m.node_tree.nodes.get("Principled BSDF"); p.inputs["Base Color"].default_value=(*color,1)
 p.inputs["Metallic"].default_value=metal; p.inputs["Roughness"].default_value=rough
 p.inputs["Emission Color"].default_value=(*color,1); p.inputs["Emission Strength"].default_value=emission
 return m
ivory=mat("Power_Ivory",(0.82,0.79,0.72),0.15,0.48)
orange=mat("Power_Orange",(0.95,0.22,0.025),0.12,0.45)
dark=mat("Power_Graphite",(0.075,0.095,0.11),0.2,0.56)
steel=mat("Power_Steel",(0.38,0.43,0.45),0.65,0.34)
cyan=mat("Power_Cyan",(0.015,0.8,1),0.1,0.25,1.5)
def finish(o,name,m,bevel=0.015):
 o.name=name; o.data.materials.append(m)
 if bevel:
  mod=o.modifiers.new("Edge chamfer","BEVEL"); mod.width=bevel; mod.segments=2
  o.modifiers.new("Weighted normals","WEIGHTED_NORMAL")
 parts.append(o); return o
def box(name,loc,size,m,bevel=0.015):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc)
 o=bpy.context.object; o.dimensions=size
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 return finish(o,name,m,bevel)
def cyl(name,loc,r,h,m):
 bpy.ops.mesh.primitive_cylinder_add(vertices=16,radius=r,depth=h,location=loc)
 return finish(bpy.context.object,name,m,0.008)
assets=[]
def export(name):
 global parts
 bpy.ops.object.select_all(action="DESELECT")
 for o in parts:
  o.select_set(True); bpy.context.view_layer.objects.active=o
  for mod in list(o.modifiers): bpy.ops.object.modifier_apply(modifier=mod.name)
  o.select_set(False)
 for o in parts:o.select_set(True)
 bpy.context.view_layer.objects.active=parts[0]; bpy.ops.object.join()
 o=bpy.context.object; o.name=name
 scene.cursor.location=(0,0,0); bpy.ops.object.origin_set(type="ORIGIN_CURSOR")
 bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
 bpy.ops.object.mode_set(mode="EDIT"); bpy.ops.mesh.select_all(action="SELECT")
 bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=0.015)
 bpy.ops.object.mode_set(mode="OBJECT")
 bpy.ops.export_scene.fbx(filepath=os.path.join(root,name+".fbx"),use_selection=True,object_types={"MESH"},apply_unit_scale=True,axis_forward="-Y",axis_up="Z",mesh_smooth_type="FACE",add_leaf_bones=False)
 assets.append({"name":name,"dimensions_m":list(o.dimensions),"triangles":sum(len(p.vertices)-2 for p in o.data.polygons)})
 parts=[]; return o
box("Foundation",(0,0,0.07),(0.74,0.74,0.14),dark,0.04)
box("Foot_armor",(0,0,0.19),(0.64,0.64,0.17),ivory,0.04)
box("Foot_orange",(0,0,0.29),(0.52,0.52,0.06),orange)
box("Mast",(0,0,1.65),(0.18,0.20,2.7),ivory,0.025)
for z in [0.53,1.57,2.63]:
 box("Safety_band",(0,0,z),(0.195,0.215,0.15),orange)
box("Service_panel",(0,-0.115,0.93),(0.15,0.055,0.3),dark)
box("Status_LED",(0,-0.148,1.01),(0.08,0.012,0.055),cyan,0.003)
box("Crossarm",(0,0,3.02),(0.22,1.16,0.13),ivory,0.025)
for y in [-0.46,0.46]:
 cyl("Insulator",(0,y,3.16),0.075,0.18,dark)
 for z in [3.10,3.15,3.20]:cyl("Insulator_rib",(0,y,z),0.095,0.027,steel)
 cyl("Cable_clamp",(0,y,3.27),0.047,0.07,steel)
 box("Clamp_indicator",(0.068,y,3.25),(0.024,0.075,0.032),cyan,0.003)
pole=export("SM_PowerPole")
# One straight 1m span along Unreal spline forward X; sockets at y +/-46cm.
for y in [-0.46,0.46]:
 o=cyl("Insulated_cable",(0.5,y,0),0.025,1,dark)
 o.rotation_euler.y=math.pi/2
# Tiny tracer on only one cable, separately addressable cyan material slot.
box("Cable_tracer",(0.5,0.46,0.026),(1,0.009,0.006),cyan,0)
cable=export("SM_PowerCableSpan")
box("Terminal_foot",(0,0,0.04),(0.36,0.38,0.08),steel)
box("Terminal_housing",(0,0,0.19),(0.28,0.32,0.25),ivory,0.03)
box("Terminal_band",(0,-0.17,0.20),(0.22,0.025,0.08),orange)
for y in [-0.115,0.115]:
 cyl("Terminal_insulator",(0,y,0.40),0.045,0.18,dark)
 cyl("Terminal_clamp",(0,y,0.51),0.04,0.04,steel)
terminal=export("SM_PowerCableTerminal")
# Spread assets in studio scene AFTER exports; original exports keep ground/connection origins.
cable.location=(2.5,0,0.5); terminal.location=(-1.6,0,0)
cam_data=bpy.data.cameras.new("PowerAssets_Camera"); cam=bpy.data.objects.new("PowerAssets_Camera",cam_data); scene.collection.objects.link(cam)
cam.location=(6,-8,5); cam.rotation_euler=(Vector((0.4,0,1.35))-cam.location).to_track_quat("-Z","Y").to_euler()
cam.data.type="ORTHO"; cam.data.ortho_scale=6.1; scene.camera=cam
for name,loc,power in [("Key",(4,-5,7),1100),("Fill",(-4,-2,5),800),("Rim",(2,4,6),900)]:
 d=bpy.data.lights.new(name,"AREA"); d.energy=power; d.size=5
 o=bpy.data.objects.new(name,d); scene.collection.objects.link(o); o.location=loc
 o.rotation_euler=(Vector((0,0,1.5))-o.location).to_track_quat("-Z","Y").to_euler()
scene.world=bpy.data.worlds.new("PowerAssets_Studio"); scene.world.use_nodes=True
scene.world.node_tree.nodes["Background"].inputs[0].default_value=(0.3,0.34,0.4,1)
scene.world.node_tree.nodes["Background"].inputs[1].default_value=0.6
scene.render.engine="CYCLES"; scene.cycles.samples=24
scene.render.resolution_x=1024; scene.render.resolution_y=1024; scene.render.resolution_percentage=100
scene.render.film_transparent=True; scene.render.image_settings.file_format="PNG"
scene.render.filepath=os.path.join(root,"PowerConnection_Preview.png")
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(root,"PowerConnection.blend"))
bpy.ops.render.render(write_still=True)
metadata={"assets":assets,"pole_cable_attachment_cm":[[0,-46,327],[0,46,327]],"cable_forward_axis":"X","cable_span_length_cm":100,"recommended_pole_spacing_cm":600}
with open(os.path.join(root,"AssetSpecifications.json"),"w",encoding="utf-8") as f:json.dump(metadata,f,indent=2)
result=metadata
