import bpy, math, random, json
from pathlib import Path
from mathutils import Vector
root=Path(r'C:\UE5\SurviveThePlanet 5.8\ContentSource\Coal')
root.mkdir(parents=True,exist_ok=True)
scene=bpy.data.scenes.new('STP_Coal_Assets'); bpy.context.window.scene=scene
scene.unit_settings.system='METRIC'; scene.unit_settings.scale_length=1
collection=bpy.data.collections.new('Coal_Assets'); scene.collection.children.link(collection)
groups={}; rng=random.Random(487)
def mat(name,color,metal=0,rough=.6,emit=0):
    m=bpy.data.materials.new(name); m.diffuse_color=(*color,1); m.use_nodes=True
    p=m.node_tree.nodes.get('Principled BSDF'); p.inputs['Base Color'].default_value=(*color,1)
    p.inputs['Metallic'].default_value=metal; p.inputs['Roughness'].default_value=rough
    if emit: p.inputs['Emission Color'].default_value=(*color,1); p.inputs['Emission Strength'].default_value=emit
    return m
coal=mat('Coal_Charcoal',(.035,.045,.055),.15,.38)
coal_edge=mat('Coal_Fracture',(.105,.12,.135),.12,.48)
rock=mat('Coal_Sandstone',(.24,.17,.105))
steel=mat('Coal_Steel',(.23,.28,.31),.75,.38)
dark=mat('Coal_Chassis',(.075,.09,.105),.65,.5)
ivory=mat('Coal_Armor',(.63,.61,.54),.35,.48)
yellow=mat('Coal_SafetyYellow',(.94,.58,.035),.2,.45)
cyan=mat('Coal_StatusCyan',(.015,.55,.75),.1,.35,2)
def finish(o,name,m,g,bevel=0):
    o.name=name
    for c in list(o.users_collection): c.objects.unlink(o)
    collection.objects.link(o); o.data.materials.append(m); groups.setdefault(g,[]).append(o)
    if bevel:
        mod=o.modifiers.new('Edge chamfer','BEVEL'); mod.width=bevel; mod.segments=1
        o.modifiers.new('Weighted normals','WEIGHTED_NORMAL')
    return o
def box(name,loc,dim,m,g='Main',bevel=.025):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc); o=bpy.context.object; o.dimensions=dim
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    return finish(o,name,m,g,bevel)
def cyl(name,loc,r,depth,m,g='Main',rotation=(0,0,0),verts=16):
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts,radius=r,depth=depth,location=loc,rotation=rotation)
    return finish(bpy.context.object,name,m,g,.009)
def shard(name,loc,dim,g='Deposit',m=coal):
    n=7; vs=[]
    radii=[rng.uniform(.8,1.1) for i in range(n)]
    for z in [-.5,.5]:
        for i in range(n):
            a=2*math.pi*i/n; vs.append((math.cos(a)*dim[0]*.5*radii[i],math.sin(a)*dim[1]*.5*radii[i],z*dim[2]+rng.uniform(-.02,.02)))
    fs=[tuple(reversed(range(n))),tuple(range(n,2*n))]+[(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
    mesh=bpy.data.meshes.new(name);mesh.from_pydata(vs,[],fs);mesh.update()
    o=bpy.data.objects.new(name,mesh);collection.objects.link(o);o.location=loc
    finish(o,name,m,g);o.data.materials.append(coal_edge)
    for p in o.data.polygons: p.material_index=1 if p.index==1 else 0
    return o
# Three squat layered blocks, ~2.2 m wide, comparable to existing mineral nodes.
for j,(x,y,w,d,h) in enumerate([(-.48,-.12,1.05,.93,.54),(.5,.16,1.12,1.0,.68),(-.05,.63,.75,.7,.41)]):
    for k in range(4):
        shard('Coal seam',(x,y,.07+(k+.5)*h/4),(w*(1-k*.075),d*(1-k*.06),h/4*.86))
    shard('Host rock',(x,y,.05),(w*1.06,d*1.06,.12),m=rock)
for i in range(24):
    a=rng.uniform(0,math.tau);r=rng.uniform(.8,1.16)
    shard('Coal rubble',(math.cos(a)*r,math.sin(a)*r,.04),(rng.uniform(.05,.13),rng.uniform(.05,.12),.065))
# Armored industrial cutter, preserving the compact gameplay footprint.
def link(name,a,b,r,m,g='Main',verts=12):
    delta=Vector(b)-Vector(a)
    o=cyl(name,(Vector(a)+Vector(b))*.5,r,delta.length,m,g,verts=verts)
    o.rotation_euler=delta.to_track_quat('Z','Y').to_euler()
    return o
def plate(name,loc,dim):
    box(name+' dark gasket',loc,(dim[0]+.035,dim[1]+.035,dim[2]+.025),dark,bevel=.035)
    box(name,Vector(loc)+Vector((0,0,.022)),dim,ivory,bevel=.035)
box('Heavy skid chassis',(-.25,.13,.16),(1.98,1.82,.28),dark,bevel=.055)
box('Rear crusher motor',(-.25,.45,.63),(1.67,1.03,.8),dark,bevel=.07)
for x in [-1.01,.51]:
    box('Armored shoulder backing',(x,-.26,.55),(.38,1.37,.95),steel,bevel=.065)
    box('Shoulder outer plate',(x,-.24,.6),(.405,1.08,.69),ivory,bevel=.065)
    box('Shoulder top armor',(x,-.25,1.04),(.42,1.25,.12),ivory,bevel=.035)
    for y in [-.76,.29]:
        box('Raised corner reinforcement',(x,y,.61),(.455,.09,.81),dark,bevel=.022)
    box('Front bumper',(x,-.98,.125),(.53,.2,.19),steel)
    box('Front hazard inset',(x,-.954,.59),(.305,.03,.64),dark)
    box('Safety yellow',(x,-.976,.62),(.245,.02,.47),yellow,bevel=.004)
    for z in [.47,.65,.83]:
        o=box('Hazard slash',(x,-.990,z),(.27,.015,.075),dark,bevel=0);o.rotation_euler.y=-.62
    box('Cyan lamp surround',(x,-.99,.245),(.25,.07,.1),dark,bevel=.017)
    box('Cyan work light',(x,-1.029,.245),(.17,.012,.045),cyan,bevel=.008)
    for y in [-.78,.74]:
        box('Ground skid foot',(x,y,.035),(.55,.38,.07),steel)
        for sx in [-.16,.16]: cyl('Skid fastening',(x+sx,y,.081),.026,.035,dark,verts=8)
    # Side access panel with fasteners, inset vents and a yellow service marking.
    outside=x+(-.215 if x<0 else .215)
    box('Side maintenance gasket',(outside,-.22,.6),(.025,.69,.43),dark,bevel=.014)
    box('Side maintenance plate',(outside+(-.02 if x<0 else .02),-.22,.6),(.025,.61,.35),ivory,bevel=.012)
    for y in [-.45,.0]:
        for z in [.48,.72]:
            cyl('Side hex bolt',(outside+(-.045 if x<0 else .045),y,z),.023,.025,steel,rotation=(0,math.pi/2,0),verts=6)
    for j in range(5):box('Side louvre',(outside+(-.04 if x<0 else .04),.49,.5+j*.073),(.04,.22,.026),dark,bevel=.003)
    link('Hydraulic piston',(x,-.78,.89),(x,.18,1.08),.035,steel)
    link('Hydraulic sleeve',(x,-.3,.985),(x,.17,1.08),.065,dark)
    for j in range(8):cyl('Hydraulic collar',(x,-.33+j*.057,1.01+j*.01),.068,.012,steel,rotation=(math.pi/2,0,0))
# Split service roof: segmented panels, grille, lifting eyes, reinforcing trims.
plate('Rear roof',(-.25,.75,1.065),(1.65,.39,.085))
plate('Central service hatch',(-.25,.31,1.10),(1.40,.42,.10))
for x in [-.84,.34]:
    box('Roof reinforcement',(x,.38,1.18),(.055,.83,.045),steel,bevel=.01)
    for y in [.02,.75]:cyl('Roof hex fastener',(x,y,1.205),.026,.022,steel,verts=6)
for j in range(8):box('Roof cooling vent',(-.63+j*.11,.75,1.13),(.052,.19,.023),dark,bevel=.004)
for x in [-.7,.2]:
    box('Hatch hinge',(x,.52,1.18),(.13,.08,.055),dark)
    link('Lifting eye',(x,.2,1.19),(x,.32,1.19),.019,steel)
box('Forward crossbar',(-.25,-.12,.88),(1.22,.18,.19),steel)
plate('Front service cover',(-.25,-.20,1.02),(1.14,.27,.09))
for j in range(8):box('Front cooling fins',(-.77+j*.15,-.22,.81),(.072,.045,.115),dark,bevel=.004)
for x in [-.77,.27]:box('Upper cyan indicator',(x,-.35,.975),(.09,.02,.037),cyan,bevel=.005)
# Rear pipes and cable conduits, visible from the RTS camera.
for x in [-.8,.3]:
    link('Rear exhaust',(x,.82,.60),(x,.89,1.16),.071,steel)
    cyl('Exhaust cap',(x,.89,1.16),.09,.042,dark)
    for j in range(6):cyl('Exhaust heat ribs',(x,.86,.83+j*.045),.086,.012,dark)
for i in range(2):
    points=[(-.8+i*.14,-.65,.91),(-.66+i*.14,-.44,1.12),(-.5+i*.14,-.10,1.21),(-.34+i*.14,.05,1.15)]
    for a,b in zip(points,points[1:]):link('Curved hydraulic hose',a,b,.022,dark)
# Conveyor with exposed rollers, segmented belt, structural rails and drive hub.
box('Conveyor frame',(1.01,.20,.49),(1.20,.58,.14),steel)
box('Rubber belt',(1.01,.20,.565),(1.12,.46,.035),dark,bevel=.006)
for y in [-.105,.505]:
    box('Conveyor guard',(1.01,y,.61),(1.22,.045,.18),dark,bevel=.012)
    box('Worn rail cap',(1.01,y,.715),(1.22,.075,.033),ivory,bevel=.009)
    for x in [.5,.8,1.1,1.48]:cyl('Rail bolt',(x,y,.74),.018,.015,steel,verts=6)
for j in range(13):box('Belt rib',(.48+j*.09,.20,.599),(.022,.445,.020),steel,bevel=.004)
for x in [.49,1.51]:cyl('Conveyor drive roller',(x,.20,.52),.11,.69,steel,rotation=(math.pi/2,0,0))
cyl('Belt drive hub',(.49,-.21,.52),.14,.17,dark,rotation=(math.pi/2,0,0))
box('Discharge hazard lip',(1.63,.20,.57),(.07,.66,.12),yellow,bevel=.012)
for x in [.77,1.38]:
    link('Conveyor diagonal support',(x,.20,.08),(x+.14,.20,.48),.05,steel)
    box('Conveyor footing',(x,.20,.045),(.27,.29,.09),dark)
# Exposed coal bed stays under the cutter after the source actor is hidden.
for i in range(36):
    x=rng.uniform(-.96,.42);y=rng.uniform(-1.00,-.45)
    shard('Broken coal under cutter',(x,y,.06),(rng.uniform(.08,.23),rng.uniform(.08,.18),rng.uniform(.06,.13)),'Main')
for i in range(20):
    x=rng.uniform(-1.15,.69);y=rng.uniform(-1.01,.91)
    shard('Ground coal scatter',(x,y,.025),(rng.uniform(.04,.10),rng.uniform(.04,.11),.045),'Main')
# Paint wear exposes metallic edges; broad seams and chips read at game zoom.
for i in range(35):
    x=rng.uniform(-.83,.3);y=rng.uniform(.1,.85)
    box('Roof paint chip',(x,y,1.184),(rng.uniform(.012,.09),rng.uniform(.008,.025),.004),steel,bevel=0)
for x in [-1.01,.51]:
    for i in range(9):box('Shoulder paint chip',(x+rng.uniform(-.14,.14),rng.uniform(-.7,.19),1.108),(rng.uniform(.015,.065),.018,.004),steel,bevel=0)
# Asymmetric staggered cutter teeth make rotation readable instead of a smooth silhouette.
cyl('Cutter axle',(-.25,-.61,.35),.20,1.20,steel,'Cutter',(0,math.pi/2,0),24)
for j in range(7):
    x=-.79+j*.18
    cyl('Cutter drum segment',(x,-.61,.35),.255,.11,dark,'Cutter',(0,math.pi/2,0),20)
    cyl('Cutter raised ring',(x+.04,-.61,.35),.275,.029,steel,'Cutter',(0,math.pi/2,0),20)
    for k in range(5):
        a=k*math.tau/5+j*.45
        o=box('Heavy pick tooth',(x,-.61+math.sin(a)*.30,.35+math.cos(a)*.30),(.10,.15,.19),steel,'Cutter',.015);o.rotation_euler.x=-a
        o=box('Bright cutting edge',(x,-.61+math.sin(a)*.375,.35+math.cos(a)*.375),(.105,.058,.055),ivory,'Cutter',.008);o.rotation_euler.x=-a
for x in [-.84,.36]:
    cyl('Cutter safety ring',(x,-.61,.35),.285,.035,yellow,'Cutter',(0,math.pi/2,0),24)

for i in range(6):
    shard('Conveyed coal',(.57+i*.17,.2,.65),(.15,.16,.12),'Payload')
shard('Dust chip',(0,0,0),(.06,.05,.04),'Chip')
assets={}
def merge(g,name,pivot):
    bpy.ops.object.select_all(action='DESELECT')
    for o in groups[g]:
        bpy.context.view_layer.objects.active=o;o.select_set(True)
        for mod in list(o.modifiers): bpy.ops.object.modifier_apply(modifier=mod.name)
        o.select_set(False)
    for o in groups[g]:o.select_set(True)
    bpy.context.view_layer.objects.active=groups[g][0];bpy.ops.object.join();o=bpy.context.object;o.name=name
    scene.cursor.location=pivot;bpy.ops.object.origin_set(type='ORIGIN_CURSOR');bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(island_margin=.015);bpy.ops.object.mode_set(mode='OBJECT')
    saved=o.location.copy();o.location=(0,0,0)
    bpy.ops.export_scene.fbx(filepath=str(root/(name+'.fbx')),use_selection=True,object_types={'MESH'},apply_unit_scale=True,axis_forward='-Y',axis_up='Z',add_leaf_bones=False,mesh_smooth_type='FACE',bake_anim=False)
    o.location=saved;assets[g]=o
    return o
merge('Deposit','SM_CoalDeposit',(0,0,0));merge('Main','SM_CoalMine',(0,0,0))
cutter=merge('Cutter','SM_CoalCutter',(-.25,-.61,.35))
merge('Payload','SM_CoalPayload',(0,0,0));merge('Chip','SM_CoalChip',(0,0,0))
# Native Blender action previews continuous cutter rotation; UE animates this pivot during production.
cutter.rotation_euler=(0,0,0);cutter.keyframe_insert(data_path='rotation_euler',frame=1)
cutter.rotation_euler.x=math.tau;cutter.keyframe_insert(data_path='rotation_euler',frame=61)
if cutter.animation_data and cutter.animation_data.action:
    for layer in cutter.animation_data.action.layers:
        for strip in layer.strips:
            for bag in strip.channelbags:
                for fc in bag.fcurves:
                    for key in fc.keyframe_points:key.interpolation='LINEAR'
                    fc.modifiers.new('CYCLES')
scene.frame_start=1;scene.frame_end=60;scene.render.fps=30;scene.frame_set(1)
camdata=bpy.data.cameras.new('Coal camera');cam=bpy.data.objects.new('Coal camera',camdata);scene.collection.objects.link(cam)
cam.location=(4,-6,4);cam.rotation_euler=(Vector((.1,0,.5))-cam.location).to_track_quat('-Z','Y').to_euler();camdata.type='ORTHO';camdata.ortho_scale=4.4;scene.camera=cam
for name,loc,power in [('Key',(3,-4,6),750),('Fill',(-4,-1,4),450),('Rim',(1,4,5),650)]:
    d=bpy.data.lights.new('Coal '+name,'AREA');d.energy=power;d.size=4
    o=bpy.data.objects.new(d.name,d);scene.collection.objects.link(o);o.location=loc;o.rotation_euler=(Vector((0,0,.4))-o.location).to_track_quat('-Z','Y').to_euler()
scene.world=bpy.data.worlds.new('Coal studio');scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs[1].default_value=.45
scene.render.engine='CYCLES';scene.cycles.samples=32;scene.render.resolution_x=640;scene.render.resolution_y=640;scene.render.resolution_percentage=100;scene.render.film_transparent=True;scene.render.image_settings.file_format='PNG'
assets['Deposit'].hide_render=True;assets['Chip'].hide_render=True
scene.render.filepath=str(root/'T_CoalMine.png');bpy.ops.render.render(write_still=True)
for g,o in assets.items():o.hide_render=g!='Deposit'
camdata.ortho_scale=3;scene.render.filepath=str(root/'T_Coal.png');bpy.ops.render.render(write_still=True)
for g,o in assets.items():o.hide_render=g in ['Deposit','Chip']
bpy.ops.wm.save_as_mainfile(filepath=str(root/'Coal.blend'),copy=True)
result={'assets':{g:{'triangles':sum(len(p.vertices)-2 for p in o.data.polygons),'dimensions_m':list(o.dimensions)} for g,o in assets.items()},'blend':str(root/'Coal.blend')}
(root/'manifest.json').write_text(json.dumps(result,indent=2))
