import bpy, math
from pathlib import Path
from mathutils import Vector
root=Path(r'C:\UE5\SurviveThePlanet 5.8');out=root/'ContentSource/PolymerPlant';out.mkdir(parents=True,exist_ok=True)
scene=bpy.data.scenes.new('STP_PolymerPlant_Refined');bpy.context.window.scene=scene
scene.unit_settings.system='METRIC';parts=[]
def mat(name,color,metal=.25,rough=.48,glow=0):
 m=bpy.data.materials.new(name);m.use_nodes=True;m.diffuse_color=(*color,1)
 ns=m.node_tree.nodes;links=m.node_tree.links;p=ns.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,1);p.inputs['Metallic'].default_value=metal;p.inputs['Roughness'].default_value=rough
 if glow:p.inputs['Emission Color'].default_value=(*color,1);p.inputs['Emission Strength'].default_value=glow
 else:
  noise=ns.new('ShaderNodeTexNoise');noise.inputs['Scale'].default_value=38;noise.inputs['Detail'].default_value=3
  ramp=ns.new('ShaderNodeValToRGB');ramp.color_ramp.elements[0].position=.28;ramp.color_ramp.elements[0].color=(*(v*.90 for v in color),1);ramp.color_ramp.elements[1].position=.72;ramp.color_ramp.elements[1].color=(*color,1)
  links.new(noise.outputs['Fac'],ramp.inputs[0]);links.new(ramp.outputs[0],p.inputs['Base Color'])
 return m
ivory=mat('Polymer_Ivory',(.94,.92,.87),.06);orange=mat('Polymer_Orange',(.88,.20,.025));dark=mat('Polymer_Graphite',(.07,.085,.10),.3);steel=mat('Polymer_Steel',(.53,.57,.58),.3);cyan=mat('Polymer_Cyan',(.008,.52,.7),.1,glow=2);poly=mat('Polymer_Product',(.83,.85,.77),0)
def finish(o,n,m,b=.02):
 o.name=n;o.data.materials.append(m);parts.append(o)
 if b:
  mod=o.modifiers.new('Rounded machining','BEVEL');mod.width=b;mod.segments=4;o.modifiers.new('Surface normals','WEIGHTED_NORMAL')
 return o
def box(n,p,d,m,b=.02):
 bpy.ops.mesh.primitive_cube_add(size=1,location=p);o=bpy.context.object;o.dimensions=d;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);return finish(o,n,m,b)
def cyl(n,p,r,h,m):
 bpy.ops.mesh.primitive_cylinder_add(vertices=48,radius=r,depth=h,location=p);o=finish(bpy.context.object,n,m,.015)
 for f in o.data.polygons:f.use_smooth=len(f.vertices)==4
 return o
def pipe(n,a,b,r,m):
 a,b=Vector(a),Vector(b);o=cyl(n,(a+b)/2,r,(b-a).length,m);o.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler();return o
def bent_pipe(n,pts,r,m):
 curve=bpy.data.curves.new(n,'CURVE');curve.dimensions='3D';curve.resolution_u=12;curve.bevel_depth=r;curve.bevel_resolution=3
 sp=curve.splines.new('BEZIER');sp.bezier_points.add(len(pts)-1)
 for p,co in zip(sp.bezier_points,pts):p.co=co;p.handle_left_type='AUTO';p.handle_right_type='AUTO'
 o=bpy.data.objects.new(n,curve);scene.collection.objects.link(o);bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o;bpy.ops.object.convert(target='MESH');return finish(bpy.context.object,n,m,0)
def sphere(n,p,r,scale,m):
 bpy.ops.mesh.primitive_uv_sphere_add(segments=48,ring_count=24,radius=r,location=p);o=bpy.context.object;o.scale=scale;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 for f in o.data.polygons:f.use_smooth=True
 return finish(o,n,m,0)
# A shallow foundation rests at z=0; separate feet and panels break up the silhouette.
box('Foundation',(0,0,.10),(4.48,3.5,.20),dark,.07);box('Inset orange skirt',(0,0,.19),(4.44,3.46,.08),orange)
box('Deck',(0,0,.25),(4.4,3.42,.08),steel,.045)
for x in [-2.02,2.02]:
 for y in [-1.49,1.49]:
  box('Landing foot',(x,y,.08),(.48,.47,.16),steel,.055);box('Foot cover',(x,y,.20),(.42,.4,.14),ivory,.045)
box('Dark hall shell',(.58,0,1.06),(2.65,2.76,1.48),dark,.15)
box('Upper shell rim',(.58,0,1.69),(2.77,2.89,.17),ivory,.12)
box('Roof perimeter',(.55,0,1.82),(2.84,2.94,.17),ivory,.13)
box('Inset roof plate',(.55,0,1.90),(2.53,2.62,.06),steel,.11)
for y in [-1.40,1.40]:
 for x in [-.35,.48,1.31]:
  box('Inset wall panel',(x,y,1.07),(.77,.09,1.03),ivory,.07)
  box('Panel lower reinforcement',(x,y*1.035,.54),(.77,.07,.15),steel)
  box('Orange panel stripe',(x,y*1.045,.66),(.77,.035,.065),orange,.009)
  for dx in [-.27,.27]:
   for z in [.82,1.44]:sphere('Panel fastener',(x+dx,y*1.04,z),.023,(1,.45,1),steel)
for x in [-.73,1.88]:
 for y in [-1.36,1.36]:
  box('Rounded corner column',(x,y,1.05),(.20,.22,1.36),ivory,.08)
  box('Corner stripe',(x,y,.64),(.215,.235,.10),orange,.035)
  box('Corner shoe',(x,y,.38),(.27,.29,.18),steel,.04)
# Detailed front service panel and restrained illuminated status window.
box('Front inset frame',(1.94,0,1.10),(.10,1.99,1.08),steel,.07)
box('Front service door',(2.005,-.42,1.08),(.065,.81,.9),ivory,.04)
box('Door seam',(2.045,-.4,1.05),(.018,.50,.66),dark,.015)
box('Door face',(2.058,-.4,1.07),(.017,.44,.57),ivory,.016)
box('Status bezel',(2.08,-.43,1.24),(.055,.26,.30),steel)
box('Cyan process display',(2.112,-.43,1.24),(.015,.18,.19),cyan,.008)
for y in [-.49,-.39]:box('Display marks',(2.124,y,1.24),(.006,.018,.105),ivory,.003)
for z in [.87,.94,1.01,1.08,1.15,1.22]:box('Intake grille',(2.055,.55,z),(.035,.58,.025),dark,.004)
box('Output throat',(2.04,0,.75),(.09,.52,.29),dark,.045)
box('Output shelf',(2.19,0,.58),(.36,.80,.10),steel)
for y in [-.38,.38]:box('Output rail',(2.18,y,.70),(.39,.045,.12),orange,.014)
for x in [2.05,2.13,2.21,2.29]:pipe('Conveyor roller',(x,-.30,.65),(x,.30,.65),.031,dark)
# Twin squat pressure vessels with domed caps, collars and arched pipes.
for y in [-.67,.67]:
 cyl('Tank plinth',(-1.38,y,.35),.55,.12,steel)
 cyl('Lower tank bumper',(-1.38,y,.47),.535,.12,dark)
 cyl('Reactor cylinder',(-1.38,y,1.15),.515,1.25,ivory)
 sphere('Pressure dome',(-1.38,y,1.79),.515,(1,1,.55),ivory)
 cyl('Tank shoulder seam',(-1.38,y,1.80),.524,.04,steel)
 cyl('Tank orange band',(-1.38,y,.99),.526,.105,orange)
 cyl('Tank bottom seam',(-1.38,y,.60),.526,.05,steel)
 cyl('Valve socket',(-1.38,y,2.08),.12,.14,steel)
 bent_pipe('Arched process pipe',[(-1.38,y,2.15),(-1.36,y,2.37),(-1.1,y,2.42),(-.83,y,2.34),(-.72,y,2.03)],.072,steel)
 pipe('Orange coupling',(-1.38,y,2.16),(-1.38,y,2.22),.097,orange)
 bent_pipe('Lower process line',[(-1.87,y,.7),(-1.94,y,.53),(-1.79,y,.43),(-.8,y,.43)],.06,dark)
 for a in [math.pi*.2,math.pi*.8,math.pi*1.2,math.pi*1.8]:
  x=-1.38+.42*math.cos(a);yy=y+.42*math.sin(a);box('Tank brace',(x,yy,.45),(.09,.09,.30),steel,.015)
# Roof fan cages preserve existing animation pivots.
for y in [-.65,.65]:
 box('Cooling housing',(.55,y,1.99),(1.09,1.06,.18),ivory,.10)
 cyl('Fan throat',(.55,y,2.08),.435,.07,dark)
 bpy.ops.mesh.primitive_torus_add(major_segments=48,minor_segments=8,location=(.55,y,2.135),major_radius=.43,minor_radius=.024);finish(bpy.context.object,'Fan steel rim',steel,0)
 for j in range(8):
  a=j*math.tau/8;pipe('Cage spoke',(.55+.13*math.cos(a),y+.13*math.sin(a),2.22),(.55+.43*math.cos(a),y+.43*math.sin(a),2.15),.012,steel)
 for x in [.09,1.01]:
  for yy in [y-.44,y+.44]:sphere('Roof bolt',(x,yy,2.09),.027,(1,1,.5),steel)
for y in [-1.18,1.18]:
 for x in [-.25,.15,.55,.95,1.35]:box('Roof seam',(x,y,1.952),(.26,.025,.009),dark,.003)
box('Roof orange line',(.55,-1.29,1.953),(2.05,.045,.012),orange,.006)
bodyparts=list(parts)
# Apply chamfers before joining; atlas UVs carry the baked finish into UE.
def join(objects,name):
 bpy.ops.object.select_all(action='DESELECT')
 for o in objects:
  o.select_set(True);bpy.context.view_layer.objects.active=o
  for mod in list(o.modifiers):bpy.ops.object.modifier_apply(modifier=mod.name)
 bpy.context.view_layer.objects.active=objects[0];bpy.ops.object.join();o=bpy.context.object;o.name=name;scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR');return o
body=join(bodyparts,'SM_PolymerPlant')
bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=.008);bpy.ops.object.mode_set(mode='OBJECT')
scene.render.engine='CYCLES';scene.cycles.samples=12
for kind,size,bake_type in [('BaseColor',2048,'DIFFUSE'),('AO',1024,'AO')]:
 img=bpy.data.images.new('T_PolymerPlant_'+kind,width=size,height=size,alpha=False)
 for m in body.data.materials:
  node=m.node_tree.nodes.new('ShaderNodeTexImage');node.image=img;m.node_tree.nodes.active=node
 scene.render.bake.use_pass_direct=False;scene.render.bake.use_pass_indirect=False;scene.render.bake.use_pass_color=True;scene.render.bake.margin=8
 bpy.ops.object.bake(type=bake_type);img.filepath_raw=str(out/('T_PolymerPlant_'+kind+'.png'));img.file_format='PNG';img.save()
# Export retains material slot names for the Unreal assignment script.
def export(o):
 bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
 bpy.ops.export_scene.fbx(filepath=str(out/(o.name.split('.')[0]+'.fbx')),use_selection=True,apply_unit_scale=True,axis_forward='-Y',axis_up='Z',bake_anim=False,object_types={'MESH'})
export(body)
parts=[];cyl('Rotor hub',(0,0,0),.105,.085,steel)
for j in range(7):
 a=j*math.tau/7;o=box('Fan blade',(.22*math.cos(a),.22*math.sin(a),-.02),(.36,.11,.027),dark,.015);o.rotation_euler.z=a+.30
fan=join(parts,'SM_PolymerFan');export(fan);parts=[]
product=box('Extruded polymer block',(0,0,0),(.25,.28,.20),poly,.03);product=join([product],'SM_PolymerBlock');export(product)
fan.location=(.55,-.65,2.16);second=fan.copy();second.data=fan.data;scene.collection.objects.link(second);second.location=(.55,.65,2.16)
for o in [fan,second]:
 o.rotation_euler.z=0;o.keyframe_insert(data_path='rotation_euler',frame=1);o.rotation_euler.z=math.tau;o.keyframe_insert(data_path='rotation_euler',frame=49)
 for curve in o.animation_data.action.layers[0].strips[0].channelbags[0].fcurves:
  for k in curve.keyframe_points:k.interpolation='LINEAR'
  curve.modifiers.new('CYCLES')
product.location=(2.18,0,.75);scene.frame_end=48;scene.frame_set(1)
bpy.ops.object.camera_add(location=(7,-9,7));cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,1))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=6.4;scene.camera=cam
for loc,power,size in [((1,-4,8),1200,6),((-4,2,5),850,5)]:
 bpy.ops.object.light_add(type='AREA',location=loc);o=bpy.context.object;o.data.energy=power;o.data.size=size;o.rotation_euler=(Vector((0,0,1))-o.location).to_track_quat('-Z','Y').to_euler()
scene.world=bpy.data.worlds.new('Polymer soft studio');scene.world.color=(.18,.18,.18)
scene.cycles.samples=32;scene.render.resolution_x=1024;scene.render.resolution_y=1024;scene.render.resolution_percentage=100;scene.render.image_settings.file_format='PNG';scene.render.film_transparent=True;scene.render.filepath=str(out/'T_PolymerPlant.png')
bpy.ops.wm.save_as_mainfile(filepath=str(out/'PolymerPlant.blend'));bpy.ops.render.render(write_still=True)
result={'blend':str(out/'PolymerPlant.blend'),'preview':str(out/'T_PolymerPlant.png'),'body_vertices':len(body.data.vertices),'ground_z':min((body.matrix_world@v.co).z for v in body.data.vertices),'footprint':[4.7,3.5]}
