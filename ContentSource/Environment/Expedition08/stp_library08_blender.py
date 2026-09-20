import bpy,math,random,os,json
from mathutils import Vector
ROOT=r'C:\UE5\SurviveThePlanet 5.8';OUT=ROOT+r'\ContentSource\Environment\Expedition08';os.makedirs(OUT,exist_ok=True)
sc=bpy.data.scenes.new('STP_Library08');bpy.context.window.scene=sc
with bpy.data.libraries.load(ROOT+r'\ContentSource\Environment\Expedition04\STP_ModularRock04.blend',link=False) as (src,dst):
 dst.objects=[n for n in src.objects if n.startswith('SM_Rock04_')]
sources={}
for ob in dst.objects:
 if ob:
  for kind in ['Pillar','Slab','Wedge','Foot']:
   if kind in ob.name:sources[kind]=ob
materials={}
for name,c in {'Bark':(.095,.065,.038,1),'Bone':(.32,.25,.15,1),'Metal':(.08,.095,.10,1),'Rust':(.23,.10,.032,1),'Vent':(.10,.075,.045,1),'Crystal':(.48,.12,.015,1)}.items():
 m=bpy.data.materials.new('M_E08_'+name);m.diffuse_color=c;materials[name]=m
for kind in sources:
 m=bpy.data.materials.new('M_Reuse_'+kind);m.diffuse_color=(.20,.16,.115,1);materials[kind]=m
def rock(kind,loc,scale=(1,1,1),yaw=0):
 ob=bpy.data.objects.new('Module_'+kind,sources[kind].data.copy());sc.collection.objects.link(ob);ob.data.materials.clear();ob.data.materials.append(materials[kind]);ob.location=loc;ob.scale=scale;ob.rotation_euler[2]=math.radians(yaw);return ob
def tube(points,radii,material,sides=8):
 verts=[];faces=[]
 for j,p in enumerate(points):
  p=Vector(p);d=Vector(points[min(j+1,len(points)-1)])-Vector(points[max(0,j-1)]);d.normalize();ref=Vector((0,0,1)) if abs(d.z)<.9 else Vector((1,0,0));u=d.cross(ref).normalized();v=d.cross(u).normalized()
  for k in range(sides):verts.append(p+radii[j]*(u*math.cos(k*math.tau/sides)+v*math.sin(k*math.tau/sides)))
 for j in range(len(points)-1):
  for k in range(sides):faces.append((j*sides+k,j*sides+(k+1)%sides,(j+1)*sides+(k+1)%sides,(j+1)*sides+k))
 faces.extend([tuple(reversed(range(sides))),tuple(range((len(points)-1)*sides,len(points)*sides))]);me=bpy.data.meshes.new('OrganicTube');me.from_pydata(verts,[],faces);me.materials.append(materials[material]);ob=bpy.data.objects.new('OrganicTube',me);sc.collection.objects.link(ob);return ob
def ring(rad,width,height,material,seed=0):
 r=random.Random(seed);verts=[];faces=[];n=40
 for j in range(3):
  for i in range(n):
   a=i*math.tau/n;rr=[rad-width,rad,rad+width][j]*(1+.07*math.sin(a*5+seed));z=[0,height,0][j]+r.uniform(-.025,.025);verts.append((math.cos(a)*rr,math.sin(a)*rr,z))
 for j in range(2):
  for i in range(n):faces.append((j*n+i,j*n+(i+1)%n,(j+1)*n+(i+1)%n,(j+1)*n+i))
 me=bpy.data.meshes.new('Ring');me.from_pydata(verts,[],faces);me.materials.append(materials[material]);o=bpy.data.objects.new('Ring',me);sc.collection.objects.link(o);return o
manifest=[];assets=[]
def finish(name,parts,category,note=''):
 bpy.ops.object.select_all(action='DESELECT')
 for p in parts:p.select_set(True)
 bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=bpy.context.object;o.name=name;sc.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR');bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
 bpy.ops.export_scene.fbx(filepath=OUT+'/'+name+'.fbx',use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False)
 manifest.append({'name':name,'category':category,'polygons':len(o.data.polygons),'note':note});assets.append(o);o.select_set(False)
finish('SM_Cliff08_Corner',[rock('Slab',(-1,0,0),(1,1,1.1)),rock('Slab',(1,0,0),(.85,1,1.2)),rock('Slab',(1,1.4,0),(1,1,1),90),rock('Pillar',(1,2.1,0),(.65,.7,.7),20),rock('Wedge',(-.7,-.6,-.03),(.8,.8,.7))],'cliff','L-shaped corner assembled from Rock04 modules')
finish('SM_Cliff08_End',[rock('Pillar',(0,0,0),(.8,.85,.85),12),rock('Slab',(.55,-.2,0),(.65,.8,.7),55),rock('Wedge',(.1,-.7,-.03),(.8,.8,.7)),rock('Foot',(-.55,-.55,-.02))],'cliff','Tapered end cap')
finish('SM_Cliff08_Arch',[rock('Pillar',(-1.55,0,0),(.8,.9,.85),-8),rock('Pillar',(1.55,0,0),(.9,.9,.88),15),rock('Slab',(0,0,2.7),(1.7,1.1,.40),3),rock('Wedge',(-1.8,-.5,0),(.85,.8,.7)),rock('Wedge',(1.8,-.45,0),(.8,.8,.7))],'cliff','Open arch; keep simple collision per module or complex collision to preserve opening')
parts=[tube([(0,0,0),(.08,.02,.7),(-.05,.06,1.4),(.18,.09,2.0),(.12,.14,2.65)],[.20,.15,.11,.065,.015],'Bark')]
for j in range(7):
 a=j*2.4;z=.55+j*.23;parts.append(tube([(0,.03,z),(.35*math.cos(a),.35*math.sin(a),z+.25),(.75*math.cos(a),.75*math.sin(a),z+.6),(.86*math.cos(a+.2),.86*math.sin(a+.2),z+.83)],[.065,.045,.022,.004],'Bark'))
finish('SM_Organic08_DeadTree',parts,'organic')
finish('SM_Organic08_DeadLog',[tube([(-1.3,0,.15),(-.7,.04,.18),(0,0,.14),(.65,.08,.18),(1.25,.17,.30)],[.09,.18,.19,.14,.08],'Bark'),tube([(-.2,0,.22),(-.35,.35,.34),(-.7,.65,.50)],[.07,.035,.004],'Bark')],'organic')
parts=[]
for j in range(7):
 a=j*math.tau/7;parts.append(tube([(0,0,.1),(.22*math.cos(a),.22*math.sin(a),.30),(.55*math.cos(a+.2),.55*math.sin(a+.2),.18),(math.cos(a)*.95,math.sin(a)*.95,.035)],[.07,.055,.035,.008],'Bark'))
finish('SM_Organic08_Roots',parts,'organic')
parts=[tube([(-1.25,0,.14),(0,0,.18),(1.25,0,.12)],[.07,.11,.06],'Bone')]
for j in range(5):
 for side in [-1,1]:
  pts=[((j-2)*.46,side*math.sin(k/8*math.pi*.8)*.68,.15+math.sin(k/8*math.pi)*.95) for k in range(9)];parts.append(tube(pts,[.055*(1-k/11) for k in range(9)],'Bone'))
finish('SM_Organic08_RibBones',parts,'organic')
finish('SM_Atmos08_Vent',[ring(.33,.21,.48,'Vent',8)]+[rock('Foot',(math.cos(i)*.47,math.sin(i)*.47,-.04),(.35,.35,.35),i*50) for i in range(6)],'atmosphere','Static vent base with open centre; VFX attachment location centre z=50cm, no smoke effect included')
finish('SM_Atmos08_CraterRim',[ring(1.4,.48,.32,'Vent',12)],'atmosphere','Raised impact rim; does not excavate underlying ground')
parts=[]
for j in range(4):
 pts=[((j-1.5)*.48,math.cos(k/16*math.tau)*.73,.68+math.sin(k/16*math.tau)*.73) for k in range(14)];parts.append(tube(pts,[.052]*14,'Rust'))
for a in [0,.8,2.8,3.8]:parts.append(tube([(-.9,math.cos(a)*.73,.68+math.sin(a)*.73),(.9,math.cos(a)*.73,.68+math.sin(a)*.73)],[.045,.045],'Metal'))
finish('SM_Atmos08_WreckFrame',parts,'atmosphere','Damaged cylindrical machinery frame')
parts=[]
for j in range(7):
 r=random.Random(j+801);a=j*2.4;d=r.uniform(.05,.4);h=r.uniform(.4,1.25);bpy.ops.mesh.primitive_cone_add(vertices=5,radius1=.16,radius2=0,depth=h,location=(math.cos(a)*d,math.sin(a)*d,h/2));ob=bpy.context.object;ob.data.materials.append(materials['Crystal']);ob.rotation_euler=(r.uniform(-.25,.25),r.uniform(-.25,.25),a);parts.append(ob)
finish('SM_Mineral08_CrystalCluster',parts,'atmosphere','Decorative crystals; no resource gameplay assigned')
for j,o in enumerate(assets):o.location=((j%4)*6,(j//4)*6,0)
sc.render.engine='CYCLES';sc.cycles.samples=24;sc.render.resolution_x=1600;sc.render.resolution_y=1100;sc.render.resolution_percentage=100
sc.world=bpy.data.worlds.new('LibraryWorld');sc.world.use_nodes=True;sc.world.node_tree.nodes['Background'].inputs[0].default_value=(.12,.14,.18,1);sc.world.node_tree.nodes['Background'].inputs[1].default_value=.65
bpy.ops.mesh.primitive_plane_add(size=150,location=(0,0,-.07));floor=bpy.context.object;floor.data.materials.append(materials['Metal'])
ld=bpy.data.lights.new('LibraryKey','AREA');lo=bpy.data.objects.new('LibraryKey',ld);sc.collection.objects.link(lo);lo.location=(1,-6,18);ld.energy=5000;ld.size=12;lo.rotation_euler=(Vector((8,6,0))-lo.location).to_track_quat('-Z','Y').to_euler()
ca=bpy.data.cameras.new('LibraryCamera');co=bpy.data.objects.new('LibraryCamera',ca);sc.collection.objects.link(co);co.location=(24,-28,30);co.rotation_euler=(Vector((9,5,.6))-co.location).to_track_quat('-Z','Y').to_euler();ca.type='ORTHO';ca.ortho_scale=30;sc.camera=co
sc.render.filepath=OUT+'/Library08_Studio.png';bpy.ops.wm.save_as_mainfile(filepath=OUT+'/STP_Library08.blend',copy=True);bpy.ops.render.render(write_still=True);json.dump(manifest,open(OUT+'/manifest.json','w'),indent=2);print('Exported '+str(len(manifest))+' library assets')
