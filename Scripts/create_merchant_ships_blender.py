"""Four industrial VTOL merchants, authored through Blender's native MCP."""
import bpy, math, json, random
from pathlib import Path
from mathutils import Vector
ROOT=Path(r'C:\UE5\SurviveThePlanet 5.8\ContentSource\Trading\Ships')
ROOT.mkdir(parents=True,exist_ok=True)
scene=bpy.data.scenes.new('MerchantShipWorkshop');bpy.context.window.scene=scene
scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
scene.render.engine='CYCLES';scene.cycles.samples=16
scene.render.resolution_x=1600;scene.render.resolution_y=1100;scene.render.resolution_percentage=100
scene.world=bpy.data.worlds.new('MerchantWorkshopWorld');scene.world.use_nodes=True
scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.16,.20,.24,1)
scene.world.node_tree.nodes['Background'].inputs[1].default_value=.45
scene.view_settings.view_transform='AgX'
records=[]

def material(name,color,metal=.25,emission=0):
    m=bpy.data.materials.new(name);m.diffuse_color=(*color,1);m.use_nodes=True
    p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,1)
    p.inputs['Metallic'].default_value=metal;p.inputs['Roughness'].default_value=.58
    if emission:
        p.inputs['Emission Color'].default_value=(*color,1);p.inputs['Emission Strength'].default_value=emission
    else:
        n=m.node_tree.nodes;l=m.node_tree.links
        noise=n.new('ShaderNodeTexNoise');noise.inputs['Scale'].default_value=34;noise.inputs['Detail'].default_value=3
        ramp=n.new('ShaderNodeValToRGB');ramp.color_ramp.elements[0].position=.25;ramp.color_ramp.elements[0].color=(*(v*.50 for v in color),1)
        ramp.color_ramp.elements[1].position=.75;ramp.color_ramp.elements[1].color=(*color,1)
        l.new(noise.outputs['Fac'],ramp.inputs['Fac']);l.new(ramp.outputs[0],p.inputs['Base Color'])
        bump=n.new('ShaderNodeBump');bump.inputs['Strength'].default_value=.10;bump.inputs['Distance'].default_value=.003
        l.new(noise.outputs['Fac'],bump.inputs['Height']);l.new(bump.outputs[0],p.inputs['Normal'])
    return m

def meshpart(o,name,mat,group,bevel=.025):
    o.name=name;o.data.materials.append(mat);parts.setdefault(group,[]).append(o)
    if bevel:
        mod=o.modifiers.new('Industrial chamfer','BEVEL');mod.width=bevel;mod.segments=2
        mod=o.modifiers.new('Panel normals','WEIGHTED_NORMAL')
    return o
def box(name,loc,size,mat,group='Body',bevel=.025):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.dimensions=size
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    return meshpart(o,name,mat,group,bevel)
def cyl(name,loc,r,depth,mat,group='Body',rotation=(0,0,0),vertices=16):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices,radius=r,depth=depth,location=loc,rotation=rotation)
    return meshpart(bpy.context.object,name,mat,group,.012)
def join_group(group,pivot=(0,0,0)):
    bpy.ops.object.select_all(action='DESELECT')
    for o in parts[group]:
        o.select_set(True);bpy.context.view_layer.objects.active=o
        for mod in list(o.modifiers):bpy.ops.object.modifier_apply(modifier=mod.name)
    bpy.context.view_layer.objects.active=parts[group][0];bpy.ops.object.join();o=bpy.context.object
    o.name='SM_'+key+'_'+group;scene.cursor.location=pivot;bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    return o

specs=[('OrionExchange',(.055,.38,.46),'courier'),('FrontierSupplies',(.83,.20,.045),'hauler'),('NexusRobotics',(.27,.17,.42),'robotics'),('AtlasFoundry',(.78,.49,.07),'industrial')]
for key,accent,kind in specs:
    parts={};folder=ROOT/key;folder.mkdir(exist_ok=True)
    ivory=material(key+'_Ivory',(.79,.77,.69));dark=material(key+'_Graphite',(.055,.065,.074),.55)
    steel=material(key+'_Steel',(.29,.32,.33),.7);paint=material(key+'_Faction',accent)
    glass=material(key+'_Canopy',(.018,.085,.10),.65)
    cyan=material(key+'_Cyan',(.015,.52,.74),.15,2.0);heat=material(key+'_Heat',(1,.23,.045),.2,1.3)
    # Nose is +X, symmetrical vertical-lift nacelles; deployed feet touch Z=0.
    box('Armored keel',(0,0,.54),(3.25,1.30,.37),dark,bevel=.09)
    box('Cargo fuselage',(-.23,0,.88),(2.55,1.39,.63),ivory,bevel=.14)
    box('Roof spine',(-.40,0,1.24),(1.70,.89,.15),ivory,bevel=.06)
    nose=box('Sloping cockpit',(1.30,0,.91),(.78,1.16,.52),ivory,bevel=.13);nose.rotation_euler.y=.12
    box('Front panoramic canopy',(1.705,0,1.015),(.034,.92,.24),glass,bevel=.014)
    for y in [-.59,.59]:
        box('Side pilot canopy',(1.33,y,1.05),(.51,.026,.23),glass,bevel=.025)
        box('Faction stripe',(-.12,y*1.21,.99),(2.18,.033,.105),paint,bevel=.004)
        for x in [-1.12,-.48,.16]:
            box('Inset service gasket',(x,y*1.20,.80),(.48,.035,.36),dark)
            box('Removable armor panel',(x,y*1.25,.82),(.43,.028,.29),ivory,.0 if False else 'Body',.014)
            for z in [.72,.92]:cyl('Panel bolt',(x+.16,y*1.285,z),.018,.015,steel,rotation=(math.pi/2,0,0),vertices=8)
    anchors=[(-.94,-1.10,.49),(-.94,1.10,.49),(.91,-1.04,.49),(.91,1.04,.49)]
    for x,y,z in anchors:
        box('Outrigger beam',(x,y*.60,.65),(.38,.82,.21),dark)
        box('VTOL nacelle',(x,y,.70),(.68,.62,.58),ivory,bevel=.075)
        cyl('Upper turbine shroud',(x,y,1.01),.26,.10,dark)
        cyl('Fan recessed disc',(x,y,1.055),.20,.027,steel)
        for a in range(8):
            angle=a*math.tau/8
            blade=box('Radial turbine vane',(x+math.cos(angle)*.125,y+math.sin(angle)*.125,1.072),(.17,.045,.018),dark,bevel=.004);blade.rotation_euler.z=angle+.33
        cyl('Turbine hub',(x,y,1.091),.058,.048,steel)
        cyl('Downward exhaust',(x,y,.44),.23,.17,dark)
        cyl('Thruster aperture',(x,y,.348),.17,.018,cyan)
        box('Nacelle identifier',(x,y+( .315 if y>0 else -.315),.75),(.32,.014,.17),paint,bevel=.008)
        box('Extendible gear piston',(x,y,.28),(.07,.07,.50),steel,'Gear',.009)
        box('Gear damper',(x,y,.33),(.13,.14,.27),dark,'Gear',.014)
        box('Landing skid',(x,y,.045),(.50,.24,.09),dark,'Gear',.025)
        box('Skid warning stripe',(x,y,.093),(.33,.18,.012),paint,'Gear',.002)
    for y in [-.36,.36]:
        cyl('Rear engine casing',(-1.55,y,.89),.19,.29,dark,rotation=(0,math.pi/2,0))
        cyl('Rear thruster',(-1.71,y,.89),.12,.03,cyan,rotation=(0,math.pi/2,0))
    # Working rear loading door, pivot at the lower edge, driven in Unreal.
    ramp_pivot=(-1.65,0,.43)
    box('Rear loading hatch',(-1.67,0,.75),(.075,.90,.64),ivory,'Ramp',.02)
    for y in [-.32,.32]:box('Hatch reinforcement',(-1.717,y,.75),(.023,.06,.56),dark,'Ramp',.009)
    box('Ramp caution marking',(-1.72,0,.49),(.016,.80,.07),paint,'Ramp',.002)
    if kind=='courier':
        for y in [-.79,.79]:
            o=box('Courier stabilizer',(-1.18,y,1.13),(.75,.075,.43),paint,bevel=.025);o.rotation_euler.y=-.27
        box('Exchange sensor mast',(-.60,0,1.48),(.07,.07,.45),steel)
        cyl('Exchange comms dish',(-.60,0,1.70),.22,.055,ivory,rotation=(0,.40,0))
    elif kind=='hauler':
        for x in [-.90,-.18,.54]:
            box('Frontier cargo crate',(x,0,1.40),(.64,1.02,.42),paint,bevel=.05)
            for y in [-.37,.37]:box('External cargo strap',(x,y,1.63),(.66,.06,.025),steel,bevel=.004)
        for y in [-.78,.78]:box('Cargo protective rail',(-.30,y,1.22),(2.18,.075,.14),dark)
    elif kind=='robotics':
        for y in [-.79,.79]:
            box('Robotics equipment pod',(-.48,y,.93),(1.73,.25,.43),paint,bevel=.06)
            for x in [-1.06,-.58,-.10,.28]:box('Equipment status emitter',(x,y*1.18,.97),(.12,.018,.035),cyan,bevel=.004)
        box('Autonomy core',(-.34,0,1.42),(1.35,.62,.33),dark,bevel=.10)
        cyl('Lidar crown',(-.40,0,1.66),.25,.12,ivory)
        for x in [-.95,.26]:box('Relay antenna',(x,.19,1.68),(.038,.035,.55),steel)
    else:
        box('Foundry armored roof',(-.33,0,1.34),(2.34,1.23,.26),paint,bevel=.06)
        for x in [-1.17,-.54,.10,.64]:box('Roof reinforcement',(x,0,1.50),(.08,1.30,.075),dark)
        for y in [-.78,.78]:
            box('Heat exchanger',(-.45,y,.93),(1.70,.23,.40),steel,bevel=.04)
            for x in [-1.12+i*.16 for i in range(9)]:box('Radiator fin',(x,y*1.10,.98),(.04,.18,.29),dark,bevel=.002)
        for y in [-.49,.49]:cyl('Industrial beacon',(-1.1,y,1.65),.065,.13,heat)
    # Small lights and colored top ID bars keep the silhouette legible at RTS scale.
    for y in [-.45,.45]:box('Navigation lamp',(1.695,y,.83),(.033,.08,.06),cyan,bevel=.006)
    for x in [-.88,-.70,-.52]:box('Merchant ID roof bar',(x,0,1.335),(.08,.54,.025),paint,bevel=.002)
    body=join_group('Body');gear=join_group('Gear');ramp=join_group('Ramp',ramp_pivot)
    objects=[body,gear,ramp]
    bpy.ops.object.select_all(action='DESELECT')
    for o in objects:o.select_set(True)
    bpy.context.view_layer.objects.active=body
    bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=.007);bpy.ops.object.mode_set(mode='OBJECT')
    img=bpy.data.images.new('T_'+key+'_BaseColor',width=1024,height=1024,alpha=False)
    for o in objects:
        for m in o.data.materials:
            node=m.node_tree.nodes.new('ShaderNodeTexImage');node.image=img;m.node_tree.nodes.active=node
    scene.render.bake.use_pass_direct=False;scene.render.bake.use_pass_indirect=False;scene.render.bake.use_pass_color=True;scene.render.bake.margin=6
    bpy.ops.object.bake(type='DIFFUSE')
    img.filepath_raw=str(folder/(img.name+'.png'));img.file_format='PNG';img.save()
    for o,part in zip(objects,['Body','Gear','Ramp']):
        bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
        bpy.ops.export_scene.fbx(filepath=str(folder/('SM_'+key+'_'+part+'.fbx')),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',bake_anim=False,apply_unit_scale=True)
    bpy.ops.object.empty_add(type='PLAIN_AXES');rig=bpy.context.object;rig.name=key+'_FlightRig'
    for o in objects:o.parent=rig
    for frame,location,pitch in [(1,(-10,0,7),-.10),(45,(-3,0,2),-.05),(100,(0,0,0),0),(210,(0,0,0),0),(265,(0,0,2),.04),(330,(12,0,8),.12)]:
        rig.location=location;rig.rotation_euler=(0,pitch,0);rig.keyframe_insert('location',frame=frame);rig.keyframe_insert('rotation_euler',frame=frame)
    for frame,extension in [(1,.28),(45,.28),(85,1),(210,1),(250,.28),(330,.28)]:
        gear.scale.z=extension;gear.keyframe_insert('scale',frame=frame)
    for frame,angle in [(1,0),(100,0),(120,-1.15),(190,-1.15),(210,0),(330,0)]:
        ramp.rotation_euler.y=angle;ramp.keyframe_insert('rotation_euler',frame=frame)
    scene.frame_start=1;scene.frame_end=330;scene.render.fps=30;scene.frame_set(150)
    # Save each source with a real preview animation, preserving separate runtime parts.
    bpy.ops.wm.save_as_mainfile(filepath=str(folder/(key+'.blend')))
    record={'merchant':key,'thrusters_cm':[[100*v for v in anchor] for anchor in anchors],'ramp_pivot_cm':[v*100 for v in ramp_pivot],'vertices':sum(len(o.data.vertices) for o in objects)}
    records.append(record)
    for o in objects:o.hide_render=True;o.hide_set(True)
    rig.hide_set(True)

# Shared plume/radial dust assets are meshes too, authored here rather than using primitives in the game.
parts={};key='MerchantFX';white=material('MerchantFX_White',(1,1,1))
bpy.ops.mesh.primitive_cone_add(vertices=16,radius1=.32,radius2=.09,depth=1.0,location=(0,0,-.50))
plume=meshpart(bpy.context.object,'SM_MerchantExhaust',white,'Plume',0)
scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
bpy.ops.object.select_all(action='DESELECT');plume.select_set(True)
bpy.ops.export_scene.fbx(filepath=str(ROOT/'SM_MerchantExhaust.fbx'),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',bake_anim=False)
plume.hide_render=True
bpy.ops.mesh.primitive_plane_add(size=2,location=(0,0,.02));dust=bpy.context.object;dust.name='SM_MerchantDust'
bpy.ops.object.select_all(action='DESELECT');dust.select_set(True)
bpy.ops.export_scene.fbx(filepath=str(ROOT/'SM_MerchantDust.fbx'),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',bake_anim=False)
dust.hide_render=True
size=256;image=bpy.data.images.new('T_MerchantFXFalloff',width=size,height=size,alpha=True)
pixels=[]
for y in range(size):
    for x in range(size):
        r=math.hypot((x+.5)/size*2-1,(y+.5)/size*2-1)
        a=max(0,1-r*r)**2
        pixels.extend((1,1,1,a))
image.pixels=pixels;image.filepath_raw=str(ROOT/'T_MerchantFXFalloff.png');image.file_format='PNG';image.save()
(ROOT/'MerchantShips.json').write_text(json.dumps(records,indent=2))

# Contact sheet: all four ships in their landed pose with open rear hatches.
for index,record in enumerate(records):
    rig=bpy.data.objects[record['merchant']+'_FlightRig'];rig.hide_set(False);rig.animation_data_clear()
    rig.location=((index%2)*5.1-2.55,(index//2)*4.1-2.05,0);rig.rotation_euler=(0,0,.07)
    for o in rig.children:o.hide_render=False;o.hide_set(False)
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.035));ground=bpy.context.object;ground.data.materials.append(material('WorkshopGround',(.075,.090,.10),.1))
for loc,power,size in [((2,-6,10),1600,8),((-6,0,7),900,7),((2,7,8),1400,6)]:
    bpy.ops.object.light_add(type='AREA',location=loc);light=bpy.context.object;light.data.energy=power;light.data.shape='DISK';light.data.size=size;light.rotation_euler=(-Vector(loc)).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(11,-15,15));cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,.5))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=12.7;scene.camera=cam
scene.render.filepath=str(ROOT/'MerchantShipsPreview.png');bpy.ops.render.render(write_still=True)
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'MerchantShipsWorkshop.blend'))
exec(Path(r'C:\UE5\SurviveThePlanet 5.8\Scripts\export_merchant_ships_centimeters_blender.py').read_text())
result={'ships':records,'preview':str(ROOT/'MerchantShipsPreview.png'),'source':str(ROOT)}
