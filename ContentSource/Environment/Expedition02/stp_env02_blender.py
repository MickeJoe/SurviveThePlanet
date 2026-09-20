import bpy, bmesh, math, random, json, os
from mathutils import Vector, noise
OUT=r'C:\UE5\SurviveThePlanet 5.8\ContentSource\Environment\Expedition02'
os.makedirs(OUT,exist_ok=True)
assert not os.path.exists(os.path.join(OUT,'COMPLETE.json')), 'Version already generated; inspect before overwriting.'
sc=bpy.data.scenes.new('STP_Expedition02');bpy.context.window.scene=sc
sc.unit_settings.system='METRIC';sc.unit_settings.scale_length=1
sc.render.engine='CYCLES';sc.cycles.samples=8;sc.cycles.use_denoising=True
sc.render.bake.margin=12
r=random.Random(41820);manifest=[];display=[]
def activate(ob):
    bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob
def material(name,color,rough=.84):
    m=bpy.data.materials.new(name);m.diffuse_color=(*color,1);m.use_nodes=True
    bs=m.node_tree.nodes.get('Principled BSDF');bs.inputs['Base Color'].default_value=(*color,1);bs.inputs['Roughness'].default_value=rough
    return m
rockmat=material('M_Blender_FracturedBasalt',(.16,.115,.078))
n=rockmat.node_tree.nodes;l=rockmat.node_tree.links;bs=n.get('Principled BSDF')
tc=n.new('ShaderNodeTexCoord');tex=n.new('ShaderNodeTexNoise');tex.inputs['Scale'].default_value=3.7;tex.inputs['Detail'].default_value=5;tex.inputs['Roughness'].default_value=.75;l.new(tc.outputs['Object'],tex.inputs['Vector'])
ramp=n.new('ShaderNodeValToRGB');ramp.color_ramp.elements[0].position=.19;ramp.color_ramp.elements[0].color=(.045,.035,.026,1);ramp.color_ramp.elements[1].position=.8;ramp.color_ramp.elements[1].color=(.29,.225,.15,1);l.new(tex.outputs['Fac'],ramp.inputs[0])
fine=n.new('ShaderNodeTexNoise');fine.inputs['Scale'].default_value=95;fine.inputs['Detail'].default_value=2;l.new(tc.outputs['Object'],fine.inputs['Vector'])
mix=n.new('ShaderNodeMixRGB');mix.blend_type='MULTIPLY';mix.inputs[0].default_value=.45;l.new(ramp.outputs[0],mix.inputs[1]);l.new(fine.outputs['Fac'],mix.inputs[2]);l.new(mix.outputs[0],bs.inputs['Base Color'])
bump=n.new('ShaderNodeBump');bump.inputs['Strength'].default_value=.48;bump.inputs['Distance'].default_value=.045;l.new(tex.outputs['Fac'],bump.inputs['Height'])
bump2=n.new('ShaderNodeBump');bump2.inputs['Strength'].default_value=.27;bump2.inputs['Distance'].default_value=.008;l.new(fine.outputs['Fac'],bump2.inputs['Height']);l.new(bump.outputs[0],bump2.inputs['Normal']);l.new(bump2.outputs[0],bs.inputs['Normal'])
def stone(center,scale,seed):
    rr=random.Random(seed)
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2,radius=1)
    ob=bpy.context.object
    # Unequal planes and sloping fractures, no stacked horizontal blocks.
    bm=bmesh.new();bm.from_mesh(ob.data)
    for v in bm.verts:
        v.co *= 1+rr.uniform(-.085,.085)
    for k in range(9):
        a=math.tau*k/9+rr.uniform(-.2,.2)
        normal=Vector((math.cos(a),math.sin(a),rr.uniform(-.5,.5))).normalized()
        ret=bmesh.ops.bisect_plane(bm,geom=list(bm.verts)+list(bm.edges)+list(bm.faces),dist=.0001,plane_co=normal*rr.uniform(.65,.89),plane_no=normal,clear_outer=True)
        edges=[e for e in ret['geom_cut'] if isinstance(e,bmesh.types.BMEdge) and e.is_boundary]
        if edges:bmesh.ops.holes_fill(bm,edges=edges,sides=0)
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(ob.data);bm.free()
    ob.scale=scale;ob.location=center;activate(ob);bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    bevel=ob.modifiers.new('Chipped edges','BEVEL');bevel.width=.018*max(scale);bevel.segments=2
    bpy.ops.object.modifier_apply(modifier=bevel.name)
    # Cut narrow diagonal surface seams into the larger blocks.
    if max(scale)>1:
        for k in range(2):
            bpy.ops.mesh.primitive_cube_add(size=1,location=(center[0],center[1]-scale[1]*.65,center[2]+rr.uniform(-.45,.4)*scale[2]))
            cutter=bpy.context.object;cutter.dimensions=(scale[0]*2.7,scale[1]*.65,.028*max(scale));cutter.rotation_euler=(rr.uniform(-.25,.25),rr.uniform(-.25,.25),rr.uniform(-.1,.1));activate(cutter);bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
            activate(ob);mod=ob.modifiers.new('Mineral fracture','BOOLEAN');mod.operation='DIFFERENCE';mod.solver='EXACT';mod.object=cutter
            bpy.ops.object.modifier_apply(modifier=mod.name);bpy.data.objects.remove(cutter,do_unlink=True)
    ob.data.materials.append(rockmat)
    return ob
def join(parts,name):
    bpy.ops.object.select_all(action='DESELECT')
    for o in parts:o.select_set(True)
    bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=parts[0];o.name=name
    sc.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    return o
def uv(ob):
    activate(ob);bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(angle_limit=1.15,island_margin=.018);bpy.ops.object.mode_set(mode='OBJECT')
def export(ob,name,kind='rock',bake=False):
    activate(ob);uv(ob)
    if bake:
        for typ,suffix in [('DIFFUSE','BaseColor'),('NORMAL','Normal')]:
            im=bpy.data.images.new('T_'+name+'_'+suffix,width=2048 if kind=='rock' else 1024,height=2048 if kind=='rock' else 1024)
            if typ=='NORMAL':im.colorspace_settings.name='Non-Color'
            for mat in set(m for m in ob.data.materials if m is not None):
                node=mat.node_tree.nodes.new('ShaderNodeTexImage');node.image=im;mat.node_tree.nodes.active=node
            bpy.ops.object.bake(type=typ,pass_filter={'COLOR'} if typ=='DIFFUSE' else {'DIRECT','INDIRECT','COLOR'})
            im.filepath_raw=os.path.join(OUT,im.name+'.png');im.file_format='PNG';im.save()
    tri=ob.modifiers.new('Triangles','TRIANGULATE');bpy.ops.object.modifier_apply(modifier=tri.name)
    bpy.ops.export_scene.fbx(filepath=os.path.join(OUT,name+'.fbx'),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',add_leaf_bones=False,bake_anim=False,mesh_smooth_type='FACE',path_mode='STRIP')
    manifest.append({'name':name,'kind':kind,'triangles':len(ob.data.polygons),'dimensions_m':list(ob.dimensions),'baked':bake})
    display.append(ob)
specs=[
 ('SM_Cliff_Crown_02',[(0,0,1.65,1.25,1.02,2.0),(-1.05,.25,1.0,.9,.85,1.3),(.95,.25,1.25,.8,.8,1.6),(.25,-.75,.55,.85,.65,.75),(-.85,-.55,.36,.62,.64,.5)]),
 ('SM_Cliff_Ridge_02',[(-2.0,.15,.6,.95,.85,.85),(-1.0,0,1.05,1.0,.95,1.4),(.2,.2,1.25,1.1,.9,1.6),(1.4,.18,.75,1.0,.8,1.0),(2.05,-.3,.4,.6,.6,.65)]),
 ('SM_Cliff_Wall_02',[(-2.1,0,1.3,.88,.75,1.75),(-.9,.1,1.65,.85,.9,2.0),(.3,.2,1.45,.85,.9,1.85),(1.4,.2,1.2,.8,.8,1.6),(2.1,0,.6,.65,.7,.9)]),
 ('SM_Boulder_02',[(0,0,.65,1.15,.8,.95),(.7,.2,.35,.6,.6,.55),(-.8,-.2,.25,.5,.4,.45)]),
 ('SM_Scree_02',[])]
for idx,(name,forms) in enumerate(specs):
    parts=[stone((x,y,z),(sx,sy,sz),2000+idx*100+j) for j,(x,y,z,sx,sy,sz) in enumerate(forms)]
    count=26 if idx==4 else 14
    for j in range(count):
        a=r.random()*math.tau;d=r.uniform(.6,2.8 if idx in [1,2] else 1.8);rad=r.uniform(.08,.26)
        parts.append(stone((math.cos(a)*d,math.sin(a)*d*.6,rad*.38),(rad*1.3,rad,rad*.8),idx*1000+j))
    ob=join(parts,name);export(ob,name,bake=True)
    print('COMPLETED_ROCK',name,flush=True)

# Plant vertex RGBA: red = wind bend weight, green = leaf-tip flutter,
# blue = normalized height, alpha = 1. Roots are exactly zero in RGB.
red=material('M_Plant_Coral_Red',(.28,.024,.014));tip=material('M_Plant_Ember_Tips',(.52,.075,.021));stem=material('M_Plant_DarkStem',(.062,.038,.021));green=material('M_Plant_Olive',(.15,.19,.035));gold=material('M_Plant_Gold',(.39,.22,.025));orange=material('M_Plant_Tube',(.39,.105,.015));inside=material('M_Plant_Inside',(.09,.021,.012));petal=material('M_Plant_Petal',(.34,.065,.018))
def tube(points,radii,mat,sides=7):
    verts=[];faces=[]
    for j,p in enumerate(points):
        tang=Vector(points[min(j+1,len(points)-1)])-Vector(points[max(j-1,0)])
        tang.normalize();axis=tang.cross(Vector((0,1,0))).normalized();other=tang.cross(axis).normalized()
        for k in range(sides):verts.append(Vector(p)+radii[j]*(axis*math.cos(k*math.tau/sides)+other*math.sin(k*math.tau/sides)))
    for j in range(len(points)-1):
        for k in range(sides):faces.append((j*sides+k,j*sides+(k+1)%sides,(j+1)*sides+(k+1)%sides,(j+1)*sides+k))
    faces.extend([tuple(reversed(range(sides))),tuple(range((len(points)-1)*sides,len(points)*sides))])
    me=bpy.data.meshes.new('OrganicTube');me.from_pydata(verts,[],faces);me.update();o=bpy.data.objects.new('OrganicTube',me);sc.collection.objects.link(o);me.materials.append(mat)
    for p in me.polygons:p.use_smooth=True
    return o
def wind(ob,height):
    ca=ob.data.color_attributes.new(name='Wind',type='BYTE_COLOR',domain='POINT')
    for v in ob.data.vertices:
        t=max(0,min(1,v.co.z/height));ca.data[v.index].color=(t*t,t**3,t,1)
    ob['Wind_R']='0=root locked; 1=tip';ob['Wind_G']='Tip flutter';ob['Wind_B']='Normalized height'
def leaf(angle,length,width,rise,mat):
    verts=[];faces=[];steps=12
    for j in range(steps+1):
        t=j/steps;rad=length*t;z=.07+rise*math.sin(t*math.pi*.7);w=width*math.sin(math.pi*t)**.8
        for s in [-1,0,1]:
            verts.append((math.cos(angle)*rad-math.sin(angle)*w*s,math.sin(angle)*rad+math.cos(angle)*w*s,z+(.05*math.sin(t*math.pi) if s==0 else 0)))
    for j in range(steps):
        for k in range(2):faces.append((3*j+k,3*j+k+1,3*(j+1)+k+1,3*(j+1)+k))
    me=bpy.data.meshes.new('Leaf');me.from_pydata(verts,[],faces);me.update();o=bpy.data.objects.new('Leaf',me);sc.collection.objects.link(o);me.materials.append(mat)
    activate(o);sol=o.modifiers.new('Leaf thickness','SOLIDIFY');sol.thickness=.009;bpy.ops.object.modifier_apply(modifier=sol.name)
    for p in me.polygons:p.use_smooth=True
    return o
parts=[]
for j in range(10):
    a=math.tau*j/10+r.uniform(-.3,.3);h=r.uniform(.65,1.45);reach=r.uniform(.45,.95)
    points=[(math.cos(a)*reach*t*t,math.sin(a)*reach*t*t,h*t) for t in [0,.2,.4,.6,.8,1]]
    parts.append(tube(points,[.055,.048,.037,.026,.017,.008],red))
    for k in [2,3,4]:
        base=Vector(points[k]);direction=Vector((math.cos(a+(-1 if k%2 else 1)),math.sin(a+(-1 if k%2 else 1)),.75))
        end=base+direction*r.uniform(.25,.45);mid=base.lerp(end,.5)
        parts.append(tube([base,mid,end],[.022,.013,.004],red))
        parts.append(tube([mid,mid+Vector((.12*math.cos(a+1),.12*math.sin(a+1),.15)),end+Vector((.06,.03,.1))],[.01,.006,.002],tip))
ob=join(parts,'SM_Alien_Coral_02');wind(ob,1.55);export(ob,ob.name,'plant')
parts=[leaf(math.tau*j/13+r.uniform(-.2,.2),r.uniform(.5,1.05),r.uniform(.08,.15),r.uniform(.3,.75),green if j%3 else gold) for j in range(13)]
ob=join(parts,'SM_Alien_Rosette_02');wind(ob,.9);export(ob,ob.name,'plant')
parts=[]
for j in range(6):
    a=math.tau*j/6;x=.32*math.cos(a);y=.32*math.sin(a);h=r.uniform(.65,1.65);rad=r.uniform(.09,.15)
    # Hollow trumpet rim, with inner surface and dark recessed cavity.
    verts=[];faces=[];sides=12
    for z,rr in [(0,rad*.65),(h*.4,rad*.8),(h*.8,rad),(h,rad*1.18),(h-.02,rad*.9),(h-.25,rad*.6)]:
        for k in range(sides):
            aa=math.tau*k/sides;verts.append((x+rr*math.cos(aa)+.12*(z/h)**2*math.cos(a),y+rr*math.sin(aa)+.12*(z/h)**2*math.sin(a),z))
    for q in range(5):
        for k in range(sides):faces.append((q*sides+k,q*sides+(k+1)%sides,(q+1)*sides+(k+1)%sides,(q+1)*sides+k))
    faces.append(tuple(range(5*sides,6*sides)))
    me=bpy.data.meshes.new('Trumpet');me.from_pydata(verts,[],faces);me.materials.append(orange);me.materials.append(inside);me.update();o=bpy.data.objects.new('Trumpet',me);sc.collection.objects.link(o)
    for p in me.polygons:p.material_index=1 if p.index>=4*sides else 0;p.use_smooth=True
    parts.append(o)
parts.extend(leaf(math.tau*j/8,.65,.09,.32,green) for j in range(8))
ob=join(parts,'SM_Alien_Trumpets_02');wind(ob,1.7);export(ob,ob.name,'plant')

# Bloom parts: six separate petal components can hinge around local origin.
# Closed angle is 0 deg; open angle is -55 deg about local Y, before azimuth.
parts=[tube([(0,0,0),(0,0,.25),(0,0,.48)],[.08,.06,.1],stem)]
parts.extend(leaf(math.tau*j/7,.48,.075,.2,green) for j in range(7))
ob=join(parts,'SM_Alien_Bloom_Base_02');wind(ob,.7);export(ob,ob.name,'plant')
verts=[];faces=[]
for j in range(13):
    t=j/12;width=.19*math.sin(math.pi*t)**.65
    for s in [-1,0,1]:verts.append((.05+.22*math.sin(math.pi*t*.9)+.04*(1-abs(s)),s*width,t*.85))
for j in range(12):
    for k in range(2):faces.append((j*3+k,j*3+k+1,(j+1)*3+k+1,(j+1)*3+k))
me=bpy.data.meshes.new('OpeningPetal');me.from_pydata(verts,[],faces);me.materials.append(petal);me.update();ob=bpy.data.objects.new('SM_Alien_Bloom_Petal_02',me);sc.collection.objects.link(ob)
activate(ob);sol=ob.modifiers.new('Petal shell','SOLIDIFY');sol.thickness=.014;bpy.ops.object.modifier_apply(modifier=sol.name)
wind(ob,.85);ob['Opening']='Hinge around local Y, 0 closed to +55 deg open; pivot at petal root';export(ob,ob.name,'petal')

# Bake a seamless ground color and normal tile. UE blends multiple tints at world scale.
ground=material('M_Blender_FineSoil',(.19,.12,.065));n=ground.node_tree.nodes;l=ground.node_tree.links;bs=n.get('Principled BSDF');tc=n.new('ShaderNodeTexCoord')
tex=n.new('ShaderNodeTexNoise');tex.inputs['Scale'].default_value=7;tex.inputs['Detail'].default_value=5;l.new(tc.outputs['Object'],tex.inputs['Vector'])
fine=n.new('ShaderNodeTexNoise');fine.inputs['Scale'].default_value=120;fine.inputs['Detail'].default_value=2;l.new(tc.outputs['Object'],fine.inputs['Vector'])
ramp=n.new('ShaderNodeValToRGB');ramp.color_ramp.elements[0].color=(.12,.075,.038,1);ramp.color_ramp.elements[1].color=(.30,.205,.11,1);l.new(tex.outputs[0],ramp.inputs[0]);l.new(ramp.outputs[0],bs.inputs['Base Color'])
bump=n.new('ShaderNodeBump');bump.inputs['Distance'].default_value=.008;bump.inputs['Strength'].default_value=.3;l.new(fine.outputs[0],bump.inputs['Height']);l.new(bump.outputs[0],bs.inputs['Normal'])
bpy.ops.mesh.primitive_plane_add(size=4);ob=bpy.context.object;ob.name='SM_Ground_Sample_02';ob.data.materials.append(ground);export(ob,ob.name,'ground',True)

# Studio arrangement for QA only; exported FBXs remain centered at origin.
for i,ob in enumerate(display):ob.location=((i%5)*5-10,(i//5)*5,0)
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.25));bpy.context.object.data.materials.append(material('Studio',(.055,.065,.074)))
world=bpy.data.worlds.new('STP_Studio02');world.use_nodes=True;world.node_tree.nodes['Background'].inputs[1].default_value=.5;sc.world=world
for loc,power,size,col in [((-8,-6,12),2600,9,(1,.79,.61)),((8,6,10),2000,7,(.68,.8,1))]:
    d=bpy.data.lights.new('Studio','AREA');d.energy=power;d.size=size;d.color=col;o=bpy.data.objects.new('Studio',d);sc.collection.objects.link(o);o.location=loc;o.rotation_euler=(Vector((0,3,1))-o.location).to_track_quat('-Z','Y').to_euler()
d=bpy.data.cameras.new('QA');cam=bpy.data.objects.new('QA',d);sc.collection.objects.link(cam);cam.location=(14,-25,23);cam.rotation_euler=(Vector((0,3,1))-cam.location).to_track_quat('-Z','Y').to_euler();d.type='ORTHO';d.ortho_scale=29;sc.camera=cam
sc.render.resolution_x=1800;sc.render.resolution_y=1100;sc.render.resolution_percentage=100;sc.cycles.samples=24
sc.render.filepath=os.path.join(OUT,'Expedition02_Studio.png');bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT,'STP_Expedition02.blend'),copy=True);bpy.ops.render.render(write_still=True)
with open(os.path.join(OUT,'COMPLETE.json'),'w') as f:json.dump(manifest,f,indent=2)
print('EXPEDITION02_COMPLETE',json.dumps(manifest))
