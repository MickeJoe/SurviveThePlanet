import bpy, math, json, random
from pathlib import Path
from mathutils import Vector
OUT=Path(r'C:\Users\qtxmj\Documents\Codex\2026-10-07\kan-du-skapa-alla-meshes-f-2\outputs')
roster=json.loads((OUT/'building_inventory.json').read_text())
folder=OUT/'BuildingCatalog'; folder.mkdir(exist_ok=True)
original=bpy.context.window.scene
palette={'Ivory':(.66,.64,.57,1),'Graphite':(.045,.059,.065,1),'Steel':(.27,.32,.34,1),'Orange':(.8,.17,.028,1),'Cyan':(.01,.65,.9,1),'Glass':(.025,.12,.16,1),'Heat':(1,.22,.018,1)}
materials={}
for name,col in palette.items():
 m=bpy.data.materials.new('Catalog_'+name);m.diffuse_color=col;m.use_nodes=True
 bs=m.node_tree.nodes.get('Principled BSDF');bs.inputs['Base Color'].default_value=col
 bs.inputs['Metallic'].default_value=.45 if name in ('Steel','Graphite') else .15
 bs.inputs['Roughness'].default_value=.62
 if name in ('Cyan','Heat'):
  bs.inputs['Emission Color'].default_value=col;bs.inputs['Emission Strength'].default_value=2.5
 else:
  noise=m.node_tree.nodes.new('ShaderNodeTexNoise');noise.inputs['Scale'].default_value=75
  bump=m.node_tree.nodes.new('ShaderNodeBump');bump.inputs['Strength'].default_value=.17;bump.inputs['Distance'].default_value=.012
  m.node_tree.links.new(noise.outputs['Fac'],bump.inputs['Height']);m.node_tree.links.new(bump.outputs['Normal'],bs.inputs['Normal'])
  grime=m.node_tree.nodes.new('ShaderNodeTexNoise');grime.inputs['Scale'].default_value=7;grime.inputs['Detail'].default_value=4
  tint=m.node_tree.nodes.new('ShaderNodeMixRGB');tint.blend_type='MIX';tint.inputs[1].default_value=tuple(c*.85 if j<3 else c for j,c in enumerate(col));tint.inputs[2].default_value=col
  m.node_tree.links.new(grime.outputs['Fac'],tint.inputs[0]);m.node_tree.links.new(tint.outputs[0],bs.inputs['Base Color'])
 materials[name]=m
parts=[]
def finish(o,name,mat):
 o.name=name;o.data.materials.append(materials[mat]);parts.append(o);return o
def box(name,p,s,mat='Ivory',bevel=.04):
 bpy.ops.mesh.primitive_cube_add(size=1,location=p);o=bpy.context.object;o.dimensions=s
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 if bevel:
  b=o.modifiers.new('Edge chamfer','BEVEL');b.width=bevel;b.segments=2
 return finish(o,name,mat)
def cyl(name,p,r,h,mat='Steel',axis='Z',verts=20):
 bpy.ops.mesh.primitive_cylinder_add(vertices=verts,radius=r,depth=h,location=p);o=bpy.context.object
 if axis=='X':o.rotation_euler.y=math.pi/2
 if axis=='Y':o.rotation_euler.x=math.pi/2
 return finish(o,name,mat)
def beam(a,b,r=.07,mat='Steel'):
 o=cyl('Pipe', (Vector(a)+Vector(b))/2,r,(Vector(b)-Vector(a)).length,mat)
 o.rotation_euler=(Vector(b)-Vector(a)).to_track_quat('Z','Y').to_euler();return o
def ring(p,r,mat='Steel'):
 bpy.ops.mesh.primitive_torus_add(major_segments=24,minor_segments=6,location=p,major_radius=r,minor_radius=.055)
 return finish(bpy.context.object,'Flanged collar',mat)
def tank(x,y,h=2.5,r=.52):
 cyl('Pressure vessel',(x,y,.3+h/2),r,h,'Ivory')
 for z in [.5,h*.5,h+.2]:ring((x,y,z),r,'Graphite')
 cyl('Inspection cap',(x,y,h+.33),r*.7,.12,'Steel')
 box('Tank stripe',(x,y-r-.015,h*.5),(.19,.035,h*.7),'Orange',.01)
 beam((x,y,h+.4),(x+.7,y,h+.4));beam((x+.7,y,h+.4),(x+.7,y,.6))
def hall(w=3.6,d=2.6,h=1.9):
 box('Dark structural core',(0,.35,.3+h/2),(w,d,h),'Graphite')
 for side in [-1,1]:
  for k in range(4):
   x=-w/2+(k+.5)*w/4
   box('Bolted facade panel',(x,.35+side*(d/2+.025),.3+h/2),(w/4-.055,.055,h-.15))
   for dx in [-w/8+.08,w/8-.08]:
    for z in [.5,h+.13]:cyl('Hex fastener',(x+dx,.35+side*(d/2+.063),z),.022,.018,'Steel','Y',6)
   box('Lower safety stripe',(x,.35+side*(d/2+.064),.58),(w/4-.07,.012,.08),'Orange',0)
 for x in [-w/2,w/2]:box('Corner reinforcement',(x,.35,.3+h/2),(.12,d+.12,h),'Steel')
 for k in range(4):box('Roof panel',(-w/2+(k+.5)*w/4,.35,h+.34),(w/4-.055,d-.12,.10))
 box('Roller door',(0,.35-d/2-.09,1.05),(.95,.08,1.45),'Graphite')
 for z in [.49,.67,.85,1.03,1.21,1.39,1.57]:box('Door slat',(0,.35-d/2-.14,z),(.89,.025,.08),'Steel',.005)
 box('Door light',(0,.35-d/2-.14,1.87),(1.02,.025,.045),'Cyan',.005)
 box('Service console',(w/2+.1,-.3,1.0),(.2,.5,.7),'Graphite')
 box('Console screen',(w/2+.215,-.3,1.15),(.02,.33,.21),'Cyan',.003)
 for z in [.83,.9,.97]:box('Console vents',(w/2+.22,-.3,z),(.015,.35,.025),'Steel',0)
def fan(p,r=.5):
 start=len(parts);cyl('Fan hub',p,r*.2,.12,'Steel')
 for a in range(0,360,60):
  rad=math.radians(a);o=box('Rotor blade',(p[0]+r*.5*math.cos(rad),p[1]+r*.5*math.sin(rad),p[2]),(r*.8,r*.18,.055),'Graphite',.01);o.rotation_euler.z=rad
 return parts[start:]
def join(objects,name,pivot=(0,0,0)):
 bpy.ops.object.select_all(action='DESELECT')
 for o in objects:o.select_set(True)
 bpy.context.view_layer.objects.active=objects[0]
 for o in objects:
  bpy.context.view_layer.objects.active=o
  for mod in list(o.modifiers):bpy.ops.object.modifier_apply(modifier=mod.name)
 bpy.context.view_layer.objects.active=objects[0];bpy.ops.object.join();o=bpy.context.object;o.name=name
 bpy.context.scene.cursor.location=pivot;bpy.ops.object.origin_set(type='ORIGIN_CURSOR');bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
 bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(angle_limit=1.15,island_margin=.025);bpy.ops.object.mode_set(mode='OBJECT')
 return o
def export(o,path):
 bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
 location=o.location.copy();o.location=(0,0,0)
 bpy.context.view_layer.update()
 bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH'},apply_unit_scale=True,global_scale=1,axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE')
 o.location=location
 bpy.context.view_layer.update()
def build(i):
 global parts
 key=i['key'];n=i['id'];rng=random.Random(n)
 scene=bpy.data.scenes.new('Catalog_'+key);bpy.context.window.scene=scene;scene.unit_settings.system='METRIC';scene.render.engine='CYCLES';scene.cycles.samples=24
 scene.frame_end=120;parts=[];moving=[];pivot=(0,0,0);kind='fan'
 w=4.8+(n%3)*.4;d=4.1+(n%2)*.4
 if n in (62,64):w,d=8,6
 box('Foundation',(0,0,.13),(w,d,.26),'Graphite',.1)
 for x in [-w/2+.15,w/2-.15]:
  for y in [-d/2+.15,d/2-.15]:
   box('Anchor foot',(x,y,.29),(.42,.42,.16));cyl('Foundation bolt',(x,y,.4),.04,.03,'Steel',verts=6)
 for x in [-w/2+.05,w/2-.05]:box('Foundation safety edge',(x,0,.3),(.05,d-.5,.045),'Orange',0)
 if n in (14,16):
  hall(2.2,1.8,1.1);cyl('Beacon mast',(0,.45,2.6),.22,2.6,'Ivory')
  for z in [1.6,2,2.4,2.8,3.2]:ring((0,.45,z),.25,'Orange')
  pivot=(0,.45,3.9);start=len(parts)
  for a in range(0,360,120):
   rad=math.radians(a);beam(pivot,(math.cos(rad)*1.1,.45+math.sin(rad)*1.1,4.15),.08)
   box('Signal face',(math.cos(rad)*1.1,.45+math.sin(rad)*1.1,4.15),(.36,.36,.6),'Cyan')
  moving=parts[start:];kind='beacon'
 elif n in (7,15):
  hall(3.6,2.4,1.7)
  for x in [-1.3,0,1.3]:
   box('Drone landing pad',(x,-1.45,.48),(1.0,.9,.16),'Steel');ring((x,-1.45,.59),.34,'Cyan')
  for x in [-1.4,1.4]:cyl('Roof antenna',(x,.8,2.9),.045,1.8,'Steel')
  pivot=(0,.5,2.18);moving=fan(pivot,.6)
 elif n in (18,32,35,44,45,48,54,63,69):
  hall(2.4,2.3,1.6)
  for x,y in [(-1.8,.8),(-1.8,-.65),(1.8,.8)]:tank(x,y,2.0+.2*(n%4),.45)
  for x in [-.7,.7]:beam((x,-1.6,.6),(x,-1.6,1.6),.11,'Orange');beam((x,-1.6,1.6),(x,-.7,1.6),.11)
  if n==69:
   for x in [-1.25,1.25]:box('Wash gantry',(x,-1.55,1.6),(.12,.14,2.6),'Steel')
   box('Wash spray arch',(0,-1.55,2.95),(2.65,.18,.15),'Ivory')
  pivot=(0,.6,2.0);moving=fan(pivot,.52)
 elif n in (22,24,25,26):
  hall(2.1,1.8,1.4)
  for x in [-1.45,1.45]:
   beam((x,-1.45,.4),(x,-1.45,3.3),.14,'Ivory');beam((x,-1.45,1),(0,-1.45,2.8),.075)
  box('Drill gantry',(0,-1.45,3.3),(3.2,.45,.35),'Steel')
  cyl('Drill casing',(0,-1.45,2.6),.38,.8,'Ivory');pivot=(0,-1.45,1.7);start=len(parts)
  cyl('Drill spindle',pivot,.18,1.9,'Steel')
  for z in [.95,1.2,1.45,1.7,1.95,2.2]:ring((0,-1.45,z),.27,'Orange' if n!=26 else 'Cyan')
  moving=parts[start:];kind='drill'
  for x in [-1.8,1.8]:box('Ore hopper',(x,.8,1.2),(.7,1,.9),'Graphite')
 elif n in (28,30,33,34,36,37,51,55):
  hall(3.3,2.3,1.65)
  for x in [-1.65,1.65]:
   tank(x,.8,2.5+.25*(n%3),.4)
   cyl('Exhaust crown',(x,.8,3.0+.25*(n%3)),.5,.16,'Graphite')
  box('Furnace intake',(0,-1.1,1.15),(1.8,.4,.8),'Steel')
  box('Furnace glow',(0,-1.32,1.1),(1.4,.025,.4),'Heat' if n not in (36,37) else 'Cyan')
  for x in [-.6,-.3,0,.3,.6]:box('Heat grille',(x,-1.35,1.1),(.055,.04,.53),'Graphite',.005)
  pivot=(0,.4,2.08);moving=fan(pivot,.58)
 elif n in (62,64,65,68):
  hall(3.8,1.8,1.9)
  for x in [-2.7,2.7]:
   box('Dock crane tower',(x,-1.1,2.0),(.3,.4,3.5),'Ivory');beam((x,-1.1,.5),(x,-1.1,3.5),.055,'Orange')
  box('Dock cross girder',(0,-1.1,3.9),(5.7,.5,.4),'Steel')
  for y in [-2.4,-1.5,-.6]:box('Dock track',(0,y,.34),(5.4,.075,.08),'Orange',.01)
  pivot=(0,-1.1,3.75);start=len(parts);box('Crane trolley',pivot,(.7,.8,.3),'Orange');beam((0,-1.1,3.6),(0,-1.1,2.9),.035);cyl('Tool clamp',(0,-1.1,2.8),.18,.25,'Steel');moving=parts[start:];kind='slide'
 elif n in (43,46,52,53,57,58,61,66,67):
  hall(3.3,2.6,2.0)
  for x in [-1.4,1.4]:box('Dark observation window',(x,-1.005,1.55),(.55,.03,.5),'Glass')
  tank(-1.8,.8,2.1,.35)
  pivot=(.7,.55,2.5);start=len(parts);cyl('Sensor pedestal',pivot,.3,.5,'Steel')
  bpy.ops.mesh.primitive_uv_sphere_add(segments=20,ring_count=10,radius=.7,location=(.7,.55,2.95));o=finish(bpy.context.object,'Radome','Ivory');o.scale=(1,1,.6)
  for a in range(0,360,90):
   rad=math.radians(a);box('Sensor aperture',(.7+.65*math.cos(rad),.55+.65*math.sin(rad),2.95),(.16,.16,.14),'Cyan')
  moving=parts[start:];kind='beacon'
 else:
  hall(3.4+.2*(n%3),2.5,1.8+.12*(n%3))
  for x in [-1.5,1.5]:
   box('Workshop extension',(x,-1.3,1.0),(.8,1.3,1.3),'Ivory')
   for y in [-1.7,-1.45,-1.2]:box('Vent louvre',(x,y,1.69),(.65,.08,.06),'Steel',.008)
  if n in (40,41,60):
   for z in [.55,.7,.85,1.0]:box('Stored fabrication panel',(-1.1,-1.65,z),(.7,.75,.1),'Steel')
  if n in (42,47,56):
   for x in [-.8,0,.8]:tank(x,1.5,1.8,.25)
  pivot=(0,.6,2.24+.12*(n%3));moving=fan(pivot,.56)
 # Function-specific equipment gives each catalog entry its own readable skyline.
 if n==5:
  box('Workshop awning',(0,-1.75,2.1),(2.2,1.2,.12),'Steel')
  for x in [-1,1]:beam((x,-2.1,.4),(x,-2.1,2.1),.05)
 if n==7:
  for x in [-1.6,1.6]:box('Cargo crate',(x,1.55,.8),(.75,.65,.9),'Steel')
 if n==15:
  cyl('Drone relay disk',(.8,.8,3),.65,.12,'Ivory',axis='Y');cyl('Relay feed',(.8,.65,3),.12,.25,'Cyan',axis='Y')
 if n==16:box('Trader signal billboard',(0,.45,3),(1.2,.15,.5),'Graphite');box('Commercial light bar',(0,.35,3),(1,.03,.1),'Orange')
 if n==18:
  cyl('Pump flywheel',(1.4,-1.5,.9),.42,.28,'Steel',axis='X');box('Pump motor',(1.4,-.9,.7),(.65,.7,.6),'Ivory')
 if n==22:tank(1.7,1,2.3,.4)
 if n in (24,25,26):
  for k in range(5):
   cyl('Ore sorting sample',(1.75,.35+k*.2,1.75),.08,.25+(k%3)*.09,'Cyan' if n==26 else ('Orange' if n==25 else 'Steel'),verts=5)
 if n==28:box('Copper casting trough',(0,-1.8,.55),(1.6,.4,.24),'Orange')
 if n==30:
  for x in [-.75,-.25,.25,.75]:box('Glass sheet rack',(x,1.65,1.2),(.08,.55,1.7),'Glass',.01)
 if n==32:
  for x in [-.7,0,.7]:cyl('Filter cartridge',(x,1.65,1.25),.22,1.7,'Steel')
 if n==33:
  for x in [-.8,0,.8]:cyl('Kiln chamber roof',(x,.2,2.35),.33,.5,'Ivory')
 if n==34:tank(0,1.5,2.5,.32)
 if n==35:
  for z in [2.5,2.8,3.1]:ring((-1.8,.8,z),.48,'Orange')
 if n==36:
  for x in [-.7,-.35,0,.35,.7]:cyl('Fuel rod cradle',(x,1.5,1.1),.065,1.4,'Orange')
 if n in (37,61,67):
  for k in range(4):
   a=k*math.pi/2;beam((.7*math.cos(a),.7*math.sin(a),2.4),(.45*math.cos(a),.45*math.sin(a),3.5),.055,'Steel')
  cyl('Contained xeno core',(0,0,2.9),.26,1.0,'Cyan',verts=6)
 if n==39:cyl('Lathe spindle',(-.9,-1.7,1.1),.25,.65,'Steel',axis='X')
 if n==40:
  for x in [-.9,.9]:beam((x,1.4,.5),(x,1.4,3.0),.12,'Ivory')
  beam((-.9,1.4,3),(.9,1.4,3),.12)
 if n==41:
  for x in [-.8,-.4,0,.4,.8]:box('Composite press fins',(x,.3,2.65),(.1,1.7,.75),'Steel')
 if n in (42,47,56):
  for x in [-.7,0,.7]:
   box('Power cell tower',(x,.3,2.9),(.45,.5,.8+(n%3)*.2),'Ivory')
   box('Cell monitoring bar',(x,.03,2.9),(.25,.03,.07),'Cyan')
 if n==43:ring((.7,.55,3.0),.74,'Orange')
 if n==44:
  for k in range(8):box('Cooling radiator',(1.9,-1.2+k*.13,1.0),(.4,.055,1.3),'Steel',.005)
 if n==45:
  for x in [-.5,.5]:cyl('Pressure manifold',(x,1.55,2.1),.19,1.0,'Orange',axis='X')
 if n==46:
  for x in [-1,0,1]:box('Assembly inspection enclosure',(x,1.45,2.1),(.7,.4,.5),'Glass')
 if n==48:
  for x in [-.6,0,.6]:cyl('Fuel cell stack',(x,1.5,2.2),.2,.9,'Graphite');ring((x,1.5,2.4),.21,'Cyan')
 if n==49:
  for x in [-.8,0,.8]:box('Tool die press',(x,.3,2.7),(.35,.4,.8),'Orange')
 if n==50:
  for x in [-.65,.65]:box('Drone wing jig',(x,-1.5,1.65),(.8,.3,.08),'Steel')
 if n==51:cyl('Foundry crucible',(0,-1.7,.85),.65,.8,'Graphite');ring((0,-1.7,1.3),.65,'Heat')
 if n==52:
  for x in [-1.3,1.3]:box('Control cabinet',(x,1.5,2.65),(.45,.4,1.25),'Ivory')
 if n==53:
  for z in [2.6,2.85,3.1]:ring((-.7,.7,z),.48,'Steel')
 if n==54:box('Life support air duct',(0,1.6,2.4),(1.7,.5,.5),'Ivory')
 if n==55:box('Reactor lifting cage',(0,1.5,2.9),(1.4,.7,1.5),'Graphite');cyl('Reactor coil',(0,1.5,3.0),.45,1.4,'Steel')
 if n==57:
  for x in [-1.2,1.2]:cyl('Survey instrument',(x,.9,3.3),.13,1.5,'Orange')
 if n==58:
  for x in [-1,1]:cyl('Communications dish',(x,.65,3.25),.55,.10,'Ivory','Y')
 if n==59:
  for x in [-.7,0,.7]:box('Precision machining hood',(x,.4,2.75),(.55,.8,.65),'Ivory')
 if n==60:
  for x in [-.7,.7]:box('Habitat module jig',(x,.2,2.9),(1.1,1.3,.9),'Ivory');box('Habitat window',(x,-.46,2.9),(.75,.035,.25),'Glass')
 if n==62:
  for x in [-2,2]:box('Repair gantry toolbox',(x,-2,.8),(.6,.5,.8),'Orange')
 if n==63:beam((1.7,-1.5,.5),(1.7,-1.5,2.6),.09,'Orange');beam((1.7,-1.5,2.6),(.7,-1.5,2.6),.09)
 if n==64:
  for x in [-2,2]:box('Upgrade power gantry',(x,-2.3,1.6),(.3,.5,2.6),'Graphite');box('Upgrade gantry light',(x,-2.57,1.6),(.04,.02,1.8),'Cyan')
 if n==65:
  for x in [-.8,.8]:ring((x,-1.7,.4),.46,'Cyan')
 if n==66:
  for z in [2.6,3,3.4]:cyl('Calibration target',(1.6,.7,z),.18,.1,'Cyan','Y')
 if n==68:box('Fabrication printer enclosure',(0,-1.75,1.2),(1.5,.8,1.6),'Graphite');box('Printer glass',(0,-2.17,1.2),(1.3,.03,1.2),'Glass')
 # Common service pipes, access steps, weathering scars and asset identifier.
 for z in [.35,.43,.51]:box('Access step',(0,-d/2+.25,z),(1.15,.34,.08),'Steel',.02)
 for k in range(24):
  x=rng.uniform(-1.4,1.4);y=.35-1.3-.065;z=rng.uniform(.6,1.7)
  box('Paint scuff',(x,y,z),(rng.uniform(.025,.12),.006,rng.uniform(.006,.014)),'Graphite',0)
 box('ID plaque',(.95,-1.05,1.65),(.6,.065,.25),'Graphite',.01)
 for k in range(3):box('Status indicator',(.75+k*.12,-1.09,1.66),(.05,.01,.05),'Cyan',0)
 fixed=[o for o in parts if o not in moving]
 body=join(fixed,'SM_'+key+'_Body');rotor=join(moving,'SM_'+key+'_Motion',pivot)
 rotor.rotation_euler=(0,0,0);rotor.keyframe_insert(data_path='rotation_euler',frame=1)
 if kind=='slide':
  rotor.keyframe_insert(data_path='location',frame=1);rotor.location.x+=1.3;rotor.keyframe_insert(data_path='location',frame=60);rotor.location.x-=1.3;rotor.keyframe_insert(data_path='location',frame=120)
 else:rotor.rotation_euler.z=2*math.pi;rotor.keyframe_insert(data_path='rotation_euler',frame=120)
 scene.frame_set(1)
 out=folder/key;out.mkdir(exist_ok=True)
 export(body,out/(body.name+'.fbx'));export(rotor,out/(rotor.name+'.fbx'))
 # Renderable authored source with animation, meter scale and separate motion pivot.
 scene.world=bpy.data.worlds.new('World_'+key);scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.13,.16,.19,1)
 bpy.ops.object.camera_add(location=(9,-12,10));camera=bpy.context.object;camera.rotation_euler=(Vector((0,0,1.5))-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.type='ORTHO';camera.data.ortho_scale=11 if n in (62,64) else 8;scene.camera=camera
 bpy.ops.object.light_add(type='AREA',location=(1,-3,8));bpy.context.object.data.energy=1500;bpy.context.object.data.shape='DISK';bpy.context.object.data.size=7
 bpy.ops.object.light_add(type='AREA',location=(-5,4,5));bpy.context.object.data.energy=900;bpy.context.object.data.size=5
 scene.render.resolution_x=600;scene.render.resolution_y=600;scene.render.resolution_percentage=100;scene.render.image_settings.file_format='PNG';scene.render.film_transparent=True
 scene.render.filepath=str(out/'preview.png')
 bpy.data.libraries.write(str(out/(key+'.blend')),{scene},fake_user=True)
 bpy.ops.render.render(write_still=True)
 record={**i,'status':'authored','revision':3,'motion':kind,'pivot_m':list(pivot),'triangles':sum(len(p.vertices)-2 for o in [body,rotor] for p in o.data.polygons),'source':str(out/(key+'.blend'))}
 (out/'manifest.json').write_text(json.dumps(record,indent=2))
 return record
records=[]
for i in roster:
 if i['status']=='missing':
  if (folder/i['key']/'manifest.json').exists() and json.loads((folder/i['key']/'manifest.json').read_text()).get('revision')==3:records.append(json.loads((folder/i['key']/'manifest.json').read_text()));continue
  records.append(build(i));(folder/'authored_manifest.json').write_text(json.dumps(records,indent=2))
bpy.context.window.scene=original
result={'authored':len(records),'folder':str(folder),'triangles':{r['key']:r['triangles'] for r in records}}
