import bpy,math,random,os,json
from mathutils import Vector
OUT=r'C:\UE5\SurviveThePlanet 5.8\ContentSource\Environment\Expedition06';os.makedirs(OUT,exist_ok=True)
sc=bpy.data.scenes.new('STP_Vegetation06');bpy.context.window.scene=sc
mats=[]
for name,color in [('Stem',(.07,.085,.028,1)),('Leaves',(.19,.23,.06,1)),('Cap',(.37,.11,.035,1)),('Gills',(.14,.085,.045,1))]:
 m=bpy.data.materials.new('M_Plant06_'+name);m.diffuse_color=color;mats.append(m)
def stem(a,b,r):
 direction=Vector(b)-Vector(a);bpy.ops.mesh.primitive_cone_add(vertices=7,radius1=r,radius2=r*.45,depth=direction.length,location=(Vector(a)+Vector(b))*.5);o=bpy.context.object;o.rotation_euler=direction.to_track_quat('Z','Y').to_euler();o.data.materials.append(mats[0]);return o
def leaf(base,tip,width):
 a=Vector(base);d=Vector(tip)-a;side=d.cross(Vector((0,0,1))).normalized()*width
 verts=[a,a+d*.32+side,a+d*.5+Vector((0,0,width*.45)),a+d*.32-side,a+d*.72+side*.6,a+d*.72-side*.6,a+d]
 me=bpy.data.meshes.new('Leaf');me.from_pydata(verts,[],[(0,1,2),(0,2,3),(1,4,2),(2,4,6),(2,6,5),(2,5,3)]);me.materials.append(mats[1]);o=bpy.data.objects.new('Leaf',me);sc.collection.objects.link(o);return o
def export(name,parts,height):
 bpy.ops.object.select_all(action='DESELECT')
 for o in parts:o.select_set(True)
 bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=bpy.context.object;o.name=name;sc.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR');bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
 attr=o.data.color_attributes.new(name='WindWeight',type='BYTE_COLOR',domain='CORNER')
 for i,loop in enumerate(o.data.loops):
  z=o.data.vertices[loop.vertex_index].co.z;attr.data[i].color=(max(0,min(1,z/height))**1.6,0,0,1)
 bpy.ops.export_scene.fbx(filepath=OUT+'/'+name+'.fbx',use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False)
 o.select_set(False);return {'name':name,'polygons':len(o.data.polygons),'wind':'vertex R root 0 tip 1'}
r=random.Random(608);parts=[]
for j in range(13):
 ang=j*2.4;reach=r.uniform(.26,.53);h=r.uniform(.24,.54);tip=Vector((math.cos(ang)*reach,math.sin(ang)*reach,h));parts.append(stem((0,0,0),tip,.018))
 for k in range(3):
  base=tip*(.35+k*.2);yaw=ang+(-1 if k%2 else 1)*.65;end=base+Vector((math.cos(yaw)*.23,math.sin(yaw)*.23,.11));parts.append(leaf(base,end,.065))
report=[export('SM_Alien06_LowShrub',parts,.65)]
parts=[]
for x,y,h,rad in [(-.28,.06,.42,.30),(.17,.18,.65,.39),(.3,-.24,.28,.23),(-.2,-.3,.22,.19)]:
 parts.append(stem((x,y,0),(x,y,h),.045))
 verts=[];faces=[];rings=[(.06,h-.075),(.62,h-.04),(1,h),(.82,h+.065),(.12,h+.095)];segments=32
 for rr,z in rings:
  for i in range(segments):
   a=i*math.tau/segments;w=1+.055*math.sin(a*5+x*17);verts.append((x+math.cos(a)*rad*rr*w,y+math.sin(a)*rad*rr*w,z+.013*math.sin(a*3)))
 for k in range(4):
  for i in range(segments):faces.append((k*segments+i,k*segments+(i+1)%segments,(k+1)*segments+(i+1)%segments,(k+1)*segments+i))
 faces.append(tuple(range(128,160)))
 me=bpy.data.meshes.new('DiscCap');me.from_pydata(verts,[],faces);me.materials.append(mats[2]);me.materials.append(mats[3])
 for p in me.polygons:p.material_index=1 if p.index<64 else 0;p.use_smooth=True
 o=bpy.data.objects.new('DiscCap',me);sc.collection.objects.link(o);parts.append(o)
report.append(export('SM_Alien06_DiscFungus',parts,.78))
bpy.ops.wm.save_as_mainfile(filepath=OUT+'/STP_Vegetation06.blend',copy=True);json.dump(report,open(OUT+'/manifest.json','w'),indent=2);print(report)
