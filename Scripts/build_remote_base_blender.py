import bpy, math, json
from pathlib import Path
from mathutils import Vector

ROOT = Path(r'C:\UE5\SurviveThePlanet 5.8')
assert (ROOT / 'SurviveThePlanet.uproject').exists()
OUT = ROOT / 'ContentSource' / 'RemoteBase'
OUT.mkdir(parents=True, exist_ok=True)
scene = bpy.data.scenes.new('STP_RemoteBase')
bpy.context.window.scene = scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1.0
scene.render.fps = 30
scene.frame_start = 1
scene.frame_end = 901
groups = {k: [] for k in ['Body', 'Antenna', 'Ring', 'Scan']}
group = 'Body'

def material(name, color, metal, rough, emit=0):
    m = bpy.data.materials.new('Remote_' + name)
    m.diffuse_color = (*color, 1)
    m.use_nodes = True
    p = m.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value = (*color, 1)
    p.inputs['Metallic'].default_value = metal
    p.inputs['Roughness'].default_value = rough
    p.inputs['Emission Color'].default_value = (*color, 1)
    p.inputs['Emission Strength'].default_value = emit
    return m

ivory = material('Ivory', (.73, .71, .65), .18, .5)
orange = material('Orange', (.9, .19, .025), .18, .45)
dark = material('Graphite', (.065, .085, .105), .6, .42)
steel = material('Steel', (.32, .39, .42), .75, .32)
cyan = material('Cyan', (.008, .65, .95), .1, .28, 3)

def finish(o, name, mat, bevel=.025):
    o.name = 'RB_' + name
    o.data.materials.append(mat)
    if bevel:
        mod = o.modifiers.new('Edge chamfer', 'BEVEL')
        mod.width = bevel
        mod.segments = 2
        o.modifiers.new('Corner normals', 'WEIGHTED_NORMAL')
    groups[group].append(o)
    return o

def box(name, loc, size, mat, bevel=.025, angle=0):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
    o = bpy.context.object
    o.dimensions = size
    o.rotation_euler.z = angle
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return finish(o, name, mat, bevel)

def cyl(name, loc, r, depth, mat, verts=32, bevel=.015):
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts, radius=r, depth=depth, location=loc)
    return finish(bpy.context.object, name, mat, bevel)

def beam(name, a, b, width, mat):
    a, b = Vector(a), Vector(b)
    o = box(name, (a+b)/2, (width, width, (b-a).length), mat, .012)
    o.rotation_euler = (b-a).to_track_quat('Z', 'Y').to_euler()
    return o

def arc(name, r, width, z, start, end, mat, steps=16):
    verts = []
    for radius in [r-width/2, r+width/2]:
        for j in range(steps+1):
            a = start + (end-start)*j/steps
            verts.append((radius*math.cos(a), radius*math.sin(a), z))
    n = steps+1
    faces = [(j,j+1,n+j+1,n+j) for j in range(steps)]
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    o = bpy.data.objects.new(name, mesh)
    scene.collection.objects.link(o)
    return finish(o, name, mat, 0)

# Eight-sided pod and broad roof silhouette, with a clear entrance at -Y.
cyl('Foundation', (0,0,.16), 2.08, .32, dark, 8, .06)
cyl('Lower_safety_band', (0,0,.36), 2.02, .12, orange, 8, .035)
cyl('Pod_core', (0,0,.91), 1.83, 1.02, dark, 8, .05)
cyl('Upper_safety_band', (0,0,1.44), 1.99, .1, orange, 8, .025)
cyl('Roof_armor', (0,0,1.62), 2.08, .30, ivory, 8, .08)
cyl('Roof_inset', (0,0,1.80), 1.69, .10, steel, 8, .025)
cyl('Roof_command_hatch', (0,0,1.87), .68, .10, ivory, 8, .025)
for j in range(8):
    a = math.pi/8 + j*math.pi/4
    x, y = 1.83*math.cos(a), 1.83*math.sin(a)
    box('Armor_pillar', (x,y,.93), (.31,.34,1.1), ivory, .06, a)
    box('Pillar_boot', (x*1.07,y*1.07,.25), (.47,.46,.31), dark, .04, a)
    box('Status_strip', (x*1.095,y*1.095,1.08), (.028,.11,.36), cyan, .012, a)
    box('Roof_seam', (1.25*math.cos(a),1.25*math.sin(a),1.868), (.72,.055,.018), orange, .005, a)
    cyl('Roof_fastener', (1.88*math.cos(a),1.88*math.sin(a),1.81), .055, .032, steel, 8, .005)
for j in range(8):
    a = j*math.pi/4
    if j == 6:
        continue
    x,y = 1.70*math.cos(a),1.70*math.sin(a)
    box('Wall_panel', (x,y,.96), (.10,1.05,.62), ivory, .025, a)
    box('Wall_dark_window', (x*1.035,y*1.035,1.09), (.024,.74,.19), dark, .015, a)
    box('Wall_safety_stripe', (x*1.04,y*1.04,.70), (.026,.98,.065), orange, .009, a)
box('Entry_frame',(0,-1.81,.83),(.97,.16,1.04),steel,.035)
box('Entry_door',(0,-1.91,.84),(.70,.08,.86),dark,.025)
box('Door_center_panel',(0,-1.962,.86),(.52,.022,.68),orange,.018)
box('Entry_header',(0,-1.97,1.36),(.72,.03,.065),cyan,.008)
ramp = box('Entry_ramp',(0,-2.30,.20),(.98,1.12,.12),steel,.018)
ramp.rotation_euler.x = math.radians(17)
for i in range(7):
    y = -2.78+i*.145
    z = .055+(y+2.8)*.305
    box('Ramp_tread',(0,y,z+.052),(.86,.025,.028),dark,.004)
for x in [-.46,.46]:
    beam('Ramp_edge',(x,-2.81,.08),(x,-1.78,.40),.065,orange)
# Integrated right-side power cabinet.
box('Power_cabinet',(2.05,.15,.76),(.58,.80,1.20),dark,.07)
box('Power_armor',(2.10,.15,1.37),(.65,.87,.12),ivory,.04)
box('Power_light',(2.37,.15,.83),(.025,.16,.82),cyan,.009)
for j in range(5):
    box('Power_vent',(2.19,-.27,.57+j*.12),(.30,.025,.042),steel,.004)
beam('Power_conduit',(1.60,.55,.50),(2.08,.55,.50),.10,orange)
# Static antenna pedestal and braces.
cyl('Mast_pedestal',(0,0,2.0),.42,.24,dark,16)
cyl('Mast_collar',(0,0,2.14),.34,.10,orange,16)
for a in [0,math.pi/2,math.pi,3*math.pi/2]:
    beam('Mast_brace',(.56*math.cos(a),.56*math.sin(a),1.9),(.12*math.cos(a),.12*math.sin(a),2.47),.085,steel)

group = 'Antenna'
pivot = Vector((0,0,2.18))
cyl('Rotating_shaft',(0,0,3.04),.065,1.75,dark,16,.008)
for z in [2.65,3.22,3.72]:
    cyl('Mast_joint',(0,0,z),.12,.085,steel,16,.006)
beam('Antenna_backbone',(0,.14,2.36),(0,.14,3.91),.065,steel)
for z in [3.39,3.88]:
    box('Antenna_beacon',(0,-.015,z),(.075,.095,.33),cyan,.012)
    box('Beacon_backplate',(0,.045,z),(.14,.055,.38),dark,.008)
beam('Dish_bracket',(0,0,2.76),(.30,-.04,2.92),.055,steel)
# Concave low-poly dish, oriented toward the front-right.
center = Vector((.38,-.08,3.0))
normal = Vector((.65,-.55,.52)).normalized()
right = normal.cross(Vector((0,0,1))).normalized()
up = right.cross(normal).normalized()
verts = [tuple(center)]
for radius in [.14,.29,.40]:
    for j in range(32):
        a = j*2*math.pi/32
        v = center + right*(radius*math.cos(a))+up*(radius*math.sin(a))+normal*(radius*radius*.65)
        verts.append(tuple(v))
faces=[]
for j in range(32): faces.append((0,1+j,1+(j+1)%32))
for k in range(2):
    for j in range(32):
        a=1+k*32+j; b=1+k*32+(j+1)%32
        faces.append((a,a+32,b+32,b))
mesh=bpy.data.meshes.new('DishSurface'); mesh.from_pydata(verts,[],faces);mesh.update()
o=bpy.data.objects.new('Dish',mesh);scene.collection.objects.link(o);finish(o,'Dish',ivory,0)
mod=o.modifiers.new('Dish shell','SOLIDIFY');mod.thickness=.028
beam('Dish_feed',center,center+normal*.34,.032,steel)
for j in range(3):
    a=j*2*math.pi/3
    beam('Dish_feed_arm',center+right*(.36*math.cos(a))+up*(.36*math.sin(a))+normal*.09,center+normal*.30,.018,dark)

group='Ring'
for j in range(8):
    a=j*math.pi/4
    arc('Activation_ring',2.55,.035,.025,a+.065,a+math.pi/4-.065,cyan)
group='Scan'
arc('Expanding_scan',2.45,.025,.035,0,math.tau,cyan,96)

joined={}
for key, objects in groups.items():
    bpy.ops.object.select_all(action='DESELECT')
    for o in objects:
        o.select_set(True)
        bpy.context.view_layer.objects.active=o
        for modifier in list(o.modifiers):
            bpy.ops.object.modifier_apply(modifier=modifier.name)
    bpy.context.view_layer.objects.active=objects[0]
    bpy.ops.object.join()
    o=bpy.context.object;o.name='SM_RemoteBase_'+key
    scene.cursor.location=pivot if key=='Antenna' else (0,0,0)
    bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=.025)
    bpy.ops.object.mode_set(mode='OBJECT')
    joined[key]=o
    # Export relative to its component pivot. UE places the antenna at 218cm.
    location=o.location.copy();o.location=(0,0,0)
    bpy.ops.export_scene.fbx(filepath=str(OUT/(o.name+'.fbx')),use_selection=True,object_types={'MESH'},apply_unit_scale=True,axis_forward='-Y',axis_up='Z',use_mesh_modifiers=True,bake_anim=False,add_leaf_bones=False)
    o.location=location

antenna=joined['Antenna']
antenna.rotation_euler.z=0;antenna.keyframe_insert(data_path='rotation_euler',frame=1)
antenna.rotation_euler.z=math.tau;antenna.keyframe_insert(data_path='rotation_euler',frame=901)
try:
    for layer in antenna.animation_data.action.layers:
        for strip in layer.strips:
            for bag in strip.channelbags:
                for curve in bag.fcurves:
                    for key in curve.keyframe_points:key.interpolation='LINEAR'
                    curve.modifiers.new('CYCLES')
except AttributeError: pass
scan=joined['Scan']
for axis in [0,1]:
    driver=scan.driver_add('scale',axis).driver
    driver.expression='1 + 0.65 * (((frame - 1) % 120) / 120)'
cyan.node_tree.nodes.get('Principled BSDF').inputs['Emission Strength'].driver_add('default_value').driver.expression='2.3 + 0.7 * sin(2 * pi * (frame - 1) / 90)'
scan_material=bpy.data.materials.new('Remote_ScanPulse');scan_material.use_nodes=True
nodes=scan_material.node_tree.nodes;nodes.clear()
output=nodes.new('ShaderNodeOutputMaterial');transparent=nodes.new('ShaderNodeBsdfTransparent');emission=nodes.new('ShaderNodeEmission');mix=nodes.new('ShaderNodeMixShader')
emission.inputs['Color'].default_value=(.008,.65,.95,1);emission.inputs['Strength'].default_value=3
mix.inputs[0].driver_add('default_value').driver.expression='1 - (((frame - 1) % 120) / 120)'
scan_material.node_tree.links.new(transparent.outputs[0],mix.inputs[1]);scan_material.node_tree.links.new(emission.outputs[0],mix.inputs[2]);scan_material.node_tree.links.new(mix.outputs[0],output.inputs['Surface'])
scan.data.materials.clear();scan.data.materials.append(scan_material)
scene.frame_set(1)
# Render a separate preview; no change to the existing Blender asset scene.
floor_mat=material('PreviewSand',(.18,.14,.09),0,.9)
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.04))
floor=bpy.context.object;floor.name='PreviewGround';floor.data.materials.append(floor_mat)
for loc,power,size in [((3,-4,8),1700,6),((-4,-2,5),900,5),((1,5,7),1900,4)]:
    data=bpy.data.lights.new('Preview_softbox','AREA');data.energy=power;data.shape='DISK';data.size=size
    light=bpy.data.objects.new(data.name,data);scene.collection.objects.link(light);light.location=loc
    light.rotation_euler=(Vector((0,0,1.3))-light.location).to_track_quat('-Z','Y').to_euler()
data=bpy.data.cameras.new('PreviewCamera');camera=bpy.data.objects.new('PreviewCamera',data);scene.collection.objects.link(camera)
camera.location=(7,-10,8);camera.rotation_euler=(Vector((0,0,1.6))-camera.location).to_track_quat('-Z','Y').to_euler()
data.type='ORTHO';data.ortho_scale=8.4;scene.camera=camera
scene.world=bpy.data.worlds.new('RemoteBasePreviewWorld');scene.world.use_nodes=True
scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.12,.15,.18,1)
scene.world.node_tree.nodes['Background'].inputs[1].default_value=.35
scene.render.engine='CYCLES';scene.cycles.samples=32
scene.render.resolution_x=1200;scene.render.resolution_y=1200;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG';scene.render.filepath=str(OUT/'RemoteBase_Preview.png')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'RemoteBase.blend'),copy=True)
bpy.ops.render.render(write_still=True)
report={'scene':scene.name,'source':str(OUT),'antenna_pivot_cm':[0,0,218],'meshes':{k:{'vertices':len(o.data.vertices),'triangles':sum(len(p.vertices)-2 for p in o.data.polygons),'materials':[m.name for m in o.data.materials]} for k,o in joined.items()}}
(OUT/'RemoteBase_manifest.json').write_text(json.dumps(report,indent=2))
result=report
