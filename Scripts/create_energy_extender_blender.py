import bpy, math
from mathutils import Vector
root = r"C:\UE5\SurviveThePlanet 5.8\Assets\Source\EnergyExtender"
scene = bpy.data.scenes.new("STP_EnergyExtender_Asset")
bpy.context.window.scene = scene
scene.unit_settings.system = "METRIC"
scene.unit_settings.scale_length = 1.0
collection = bpy.data.collections.new("EnergyExtender_Model")
scene.collection.children.link(collection)
parts=[]
def material(name,color,metal=0.0,rough=0.48,emission=0):
    m=bpy.data.materials.new(name); m.diffuse_color=(*color,1); m.use_nodes=True
    p=m.node_tree.nodes.get("Principled BSDF")
    p.inputs["Base Color"].default_value=(*color,1)
    p.inputs["Metallic"].default_value=metal; p.inputs["Roughness"].default_value=rough
    if emission:
        p.inputs["Emission Color"].default_value=(*color,1)
        p.inputs["Emission Strength"].default_value=emission
    return m
ivory=material("Extender_Ivory",(0.76,0.73,0.66),0.12)
orange=material("Extender_Orange",(0.95,0.22,0.025),0.08)
graphite=material("Extender_Graphite",(0.115,0.135,0.15),0.4)
steel=material("Extender_Steel",(0.38,0.43,0.45),0.65,0.34)
cyan=material("Extender_Cyan",(0.015,0.8,1.0),0.1,0.25,2.0)
def finish(obj,name,mat,bevel=0.02):
    obj.name=name
    for c in list(obj.users_collection): c.objects.unlink(obj)
    collection.objects.link(obj)
    obj.data.materials.append(mat)
    if bevel:
        mod=obj.modifiers.new("Manufactured chamfer","BEVEL"); mod.width=bevel; mod.segments=2
        mod.affect="EDGES"
        obj.modifiers.new("Weighted corner normals","WEIGHTED_NORMAL")
    parts.append(obj)
    return obj
def box(name,loc,dim,mat,bevel=0.025,rot=0):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc)
    o=bpy.context.object; o.dimensions=dim; o.rotation_euler.z=rot
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    return finish(o,name,mat,bevel)
def cylinder(name,loc,r,depth,mat,verts=32,bevel=0.015):
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts,radius=r,depth=depth,location=loc)
    return finish(bpy.context.object,name,mat,bevel)
def beam(name,a,b,width,mat):
    a,b=Vector(a),Vector(b)
    o=box(name,(a+b)/2,(width,width,(b-a).length),mat,0.025)
    o.rotation_euler=(b-a).to_track_quat("Z","Y").to_euler()
    return o
def arc(name,r1,r2,z,depth,start,end,mat,steps=7):
    verts=[]
    for h in [z-depth/2,z+depth/2]:
        for r in [r1,r2]:
            for j in range(steps+1):
                a=start+(end-start)*j/steps
                verts.append((r*math.cos(a),r*math.sin(a),h))
    n=steps+1; faces=[]
    for j in range(steps):
        faces += [(j,j+1,n+j+1,n+j),(2*n+j,3*n+j,3*n+j+1,2*n+j+1),
                  (j,2*n+j,2*n+j+1,j+1),(n+j,n+j+1,3*n+j+1,3*n+j)]
    faces += [(0,n,3*n,2*n),(n-1,3*n-1,4*n-1,2*n-1)]
    mesh=bpy.data.meshes.new(name); mesh.from_pydata(verts,[],faces); mesh.update()
    o=bpy.data.objects.new(name,mesh); collection.objects.link(o)
    return finish(o,name,mat,0.012)
box("Chassis_dark_underframe",(0,0,0.21),(2.32,2.32,0.36),graphite,0.1)
box("Chassis_orange_skirt",(0,0,0.38),(2.40,2.40,0.15),orange,0.07)
box("Chassis_ivory_armor",(0,0,0.57),(2.38,2.38,0.29),ivory,0.09)
box("Chassis_deck",(0,0,0.75),(2.22,2.22,0.09),steel,0.05)
for x in [-1.1,1.1]:
    for y in [-1.1,1.1]:
        box("Foot_sole",(x,y,0.065),(0.7,0.7,0.13),graphite,0.055)
        box("Foot_orange_band",(x,y,0.28),(0.56,0.56,0.15),orange,0.05)
        box("Foot_armored_cap",(x,y,0.48),(0.56,0.56,0.3),ivory,0.065)
        cylinder("Foot_mount_bolt",(x,y,0.645),0.055,0.025,steel,12,0.004)
for side in [-1,1]:
    box("Front_vent_recess",(0,side*1.203,0.57),(1.05,0.018,0.18),graphite,0.018)
    for j in range(6):
        box("Vent_louver",(0,side*1.22,0.502+j*0.027),(0.92,0.025,0.012),steel,0.003)
    box("Deck_status_light",(0,side*0.96,0.805),(0.64,0.045,0.02),cyan,0.008)
cylinder("Mast_core",(0,0,1.6),0.49,1.75,graphite)
cylinder("Lower_mast_armor",(0,0,1.2),0.57,0.77,ivory)
cylinder("Mast_safety_band",(0,0,1.6),0.575,0.18,orange)
cylinder("Upper_mast_armor",(0,0,2.04),0.555,0.69,ivory)
cylinder("Upper_mast_collar",(0,0,2.39),0.60,0.11,steel)
for angle in [45,135,225,315]:
    a=math.radians(angle)
    beam("Diagonal_mast_support",(0.83*math.cos(a),0.83*math.sin(a),0.77),(0.46*math.cos(a),0.46*math.sin(a),1.35),0.16,ivory)
for j in range(3):
    x=-0.25+j*0.16
    beam("Exposed_service_conduit",(x,-0.66,0.83),(x,-0.66,2.33),0.085,graphite)
    box("Conduit_clamp",(x,-0.66,1.17),(0.12,0.15,0.08),orange,0.009)
box("Control_panel_socket",(0.30,-0.535,1.98),(0.25,0.08,0.36),graphite,0.025)
for j in range(3):
    box("Control_panel_LED",(0.30,-0.582,1.90+j*0.08),(0.11,0.024,0.045),cyan,0.01)
cylinder("Crown_lower_hub",(0,0,2.62),0.67,0.36,steel)
cylinder("Crown_inner_emitter",(0,0,2.84),0.62,0.06,cyan)
cylinder("Crown_top_cap",(0,0,3.02),0.64,0.30,ivory)
cylinder("Crown_cap_disk",(0,0,3.185),0.48,0.04,steel,32,0.01)
for j in range(8):
    a0=2*math.pi*j/8; a1=2*math.pi*(j+1)/8
    arc("Crown_ivory_segment",0.78,1.16,2.9,0.34,a0+0.025,a1-0.025,ivory)
    arc("Crown_orange_join",0.785,1.165,2.90,0.36,a0-0.016,a0+0.016,orange,2)
    arc("Crown_inner_cyan_band",0.76,0.79,2.86,0.06,a0+0.045,a1-0.045,cyan)
for j in range(3):
    a=2*math.pi*j/3+math.pi/6; direction=Vector((math.cos(a),math.sin(a),0))
    beam("Crown_radial_arm",direction*0.53+Vector((0,0,2.64)),direction*1.0+Vector((0,0,2.64)),0.15,graphite)
    loc=direction*1.03+Vector((0,0,3.04))
    box("Emitter_armored_module",loc,(0.45,0.49,0.45),ivory,0.055,a)
    box("Emitter_dark_socket",direction*1.279+Vector((0,0,3.04)),(0.027,0.36,0.26),graphite,0.018,a)
    box("Emitter_cyan_lens",direction*1.30+Vector((0,0,3.04)),(0.03,0.27,0.16),cyan,0.019,a)
# Apply modifiers and merge into one game mesh with a ground-centred pivot.
bpy.ops.object.select_all(action="DESELECT")
for o in parts:
    bpy.context.view_layer.objects.active=o; o.select_set(True)
    for mod in list(o.modifiers): bpy.ops.object.modifier_apply(modifier=mod.name)
    o.select_set(False)
for o in parts: o.select_set(True)
bpy.context.view_layer.objects.active=parts[0]
bpy.ops.object.join(); asset=bpy.context.object; asset.name="SM_EnergyExtender"
scene.cursor.location=(0,0,0); bpy.ops.object.origin_set(type="ORIGIN_CURSOR")
bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
bpy.ops.object.mode_set(mode="EDIT"); bpy.ops.mesh.select_all(action="SELECT")
bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=0.015)
bpy.ops.object.mode_set(mode="OBJECT")
# FBX is only the model; studio setup is excluded.
bpy.ops.export_scene.fbx(filepath=root+"\\SM_EnergyExtender.fbx",use_selection=True,object_types={"MESH"},apply_unit_scale=True,axis_forward="-Y",axis_up="Z",add_leaf_bones=False,mesh_smooth_type="FACE")
# Studio render and icon.
cam_data=bpy.data.cameras.new("Extender_ConceptCamera"); cam=bpy.data.objects.new("Extender_ConceptCamera",cam_data); scene.collection.objects.link(cam)
cam.location=(6,-8,6); cam.rotation_euler=(Vector((0,0,1.55))-cam.location).to_track_quat("-Z","Y").to_euler()
cam.data.type="ORTHO"; cam.data.ortho_scale=5.8; scene.camera=cam
for name,loc,power,size in [("Key",(4,-5,7),900,5),("Fill",(-4,-2,5),650,5),("Rim",(2,4,6),800,4)]:
    d=bpy.data.lights.new("Extender_"+name,"AREA"); d.energy=power; d.shape="DISK"; d.size=size
    o=bpy.data.objects.new("Extender_"+name,d); scene.collection.objects.link(o); o.location=loc
    o.rotation_euler=(Vector((0,0,1.5))-o.location).to_track_quat("-Z","Y").to_euler()
scene.world=bpy.data.worlds.new("Extender_StudioWorld"); scene.world.use_nodes=True
scene.world.node_tree.nodes["Background"].inputs[0].default_value=(0.36,0.39,0.43,1)
scene.world.node_tree.nodes["Background"].inputs[1].default_value=0.6
scene.render.engine="CYCLES"; scene.cycles.samples=24
scene.render.resolution_x=768; scene.render.resolution_y=768; scene.render.resolution_percentage=100
scene.render.image_settings.file_format="PNG"; scene.render.film_transparent=True
scene.render.filepath=root+"\\T_EnergyExtender.png"
bpy.ops.wm.save_as_mainfile(filepath=root+"\\EnergyExtender.blend")
bpy.ops.render.render(write_still=True)
triangles=sum(len(p.vertices)-2 for p in asset.data.polygons)
result={"blend":root+"\\EnergyExtender.blend","fbx":root+"\\SM_EnergyExtender.fbx","icon":scene.render.filepath,"triangles":triangles,"dimensions_m":list(asset.dimensions),"objects_preserved":len(bpy.data.scenes)-1}
