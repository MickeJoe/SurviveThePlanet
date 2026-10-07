import bpy, math, json
from pathlib import Path
from mathutils import Vector
root=Path(r'C:\UE5\SurviveThePlanet 5.8')
helper=(root/'Scripts/create_polymer_plant_blender.py').read_text().split('# A shallow foundation')[0]
exec(helper.replace('ContentSource/PolymerPlant','ContentSource/ConnectorFactory').replace('STP_PolymerPlant_Refined','STP_ConnectorFactory').replace('Polymer_','Connector_'))
copper=mat('Connector_Copper',(.72,.30,.09),.72)
box('Foundation',(0,0,.10),(4.7,3.5,.20),dark,.06)
box('Orange skirt',(0,0,.20),(4.66,3.46,.07),orange)
box('Deck',(0,0,.25),(4.6,3.4,.08),steel,.04)
box('Workshop',(0,.65,1.15),(3.5,1.48,1.72),dark,.1)
for x in [-1.28,-.43,.43,1.28]:
 box('Rear panel',(x,1.41,1.15),(.79,.07,1.34),ivory,.05)
 box('Rear stripe',(x,1.455,.64),(.79,.03,.08),orange,.008)
for x in [-1.72,1.72]:
 box('Side panel',(x,.65,1.14),(.1,1.43,1.38),ivory,.06)
 box('Side stripe',(x,.65,.62),(.12,1.43,.10),orange,.01)
 box('Bay column',(x,-.85,1.10),(.28,.28,1.65),ivory,.07)
 box('Column collar',(x,-.85,.65),(.30,.30,.13),orange,.03)
 box('Column shoe',(x,-.85,.39),(.36,.34,.22),steel,.04)
box('Canopy',(0,-.06,1.98),(3.78,2.33,.28),ivory,.10)
box('Canopy fascia',(0,-1.235,1.96),(3.53,.035,.07),orange,.008)
box('Roof inset',(0,.06,2.14),(3.32,1.95,.06),steel,.05)
box('Cooling enclosure',(-.35,.38,2.27),(1.96,1.24,.23),ivory,.09)
for x in [-.82,.15]:
 cyl('Fan housing',(x,.38,2.41),.43,.10,dark)
 cyl('Fan hub',(x,.38,2.46),.09,.08,steel)
 for j in range(8):
  a=j*math.tau/8;o=box('Fan blade',(x+.22*math.cos(a),.38+.22*math.sin(a),2.44),(.30,.09,.025),steel,.008);o.rotation_euler.z=a+.30
 for dx in [-.29,0,.29]:pipe('Fan guard',(x+dx,.02,2.48),(x+dx,.74,2.48),.012,dark)
box('Assembly bed',(0,-.72,.69),(2.85,.76,.55),steel,.055)
box('Assembly belt',(0,-.74,.98),(2.77,.62,.07),dark,.02)
for i in range(14):
 x=-1.3+i*.2;pipe('Belt roller',(x,-1.04,1.025),(x,-.43,1.025),.025,steel)
for y in [-1.08,-.39]:box('Assembly rail',(0,y,1.08),(2.85,.035,.075),orange,.008)
for x in [-.90,.80]:
 cyl('Robot pedestal',(x,-.02,.68),.21,.73,dark)
 cyl('Robot collar',(x,-.02,1.04),.23,.09,steel)
 a=(x,-.02,1.12);b=(x-.17,-.05,1.58);c=(x+.20,-.68,1.40);d=(x+.17,-.73,1.14)
 for p,q in [(a,b),(b,c),(c,d)]:
  pipe('Arm core',p,q,.082,dark)
  mid=(Vector(p)+Vector(q))/2;o=box('Arm casing',mid,(.17,.18,(Vector(q)-Vector(p)).length*.75),ivory,.04);o.rotation_euler=(Vector(q)-Vector(p)).to_track_quat('Z','Y').to_euler()
 for p in [a,b,c]:
  pipe('Joint',(p[0]-.115,p[1],p[2]),(p[0]+.115,p[1],p[2]),.12,steel)
  pipe('Joint light',(p[0]-.12,p[1],p[2]),(p[0]-.13,p[1],p[2]),.067,cyan)
 for dx in [-.06,.06]:box('Gripper',(d[0]+dx,d[1],d[2]-.06),(.035,.1,.12),dark,.008)
box('Output module',(1.48,.32,.88),(.70,1.20,1.14),ivory,.06)
box('Output opening',(1.842,.25,.88),(.025,.72,.42),dark,.035)
box('Output light',(1.87,.25,1.16),(.03,.58,.035),cyan,.008)
box('Output conveyor',(2.08,.25,.56),(.49,.83,.18),steel,.035)
box('Output belt',(2.08,.25,.67),(.50,.67,.04),dark,.01)
for y in [-.15,.65]:box('Output rail',(2.08,y,.75),(.50,.04,.12),orange,.01)
box('Feed tray',(-1.10,-1.47,.46),(1.05,.43,.32),steel,.04)
for x in [-1.4,-1.1,-.8]:box('Copper stock',(x,-1.47,.65),(.20,.26,.12),copper,.014)
for x,y,z in [(-.55,-.75,1.12),(.25,-.75,1.12),(1.12,-.75,1.12),(2.03,.05,.76),(2.13,.42,.76)]:
 box('Connector housing',(x,y,z),(.23,.24,.15),dark,.015)
 box('Connector lid',(x,y,z+.08),(.245,.25,.025),steel,.008)
 for dx in [-.065,0,.065]:
  for dz in [-.035,.035]:pipe('Connector pin',(x+dx,y-.13,z+dz),(x+dx,y-.18,z+dz),.012,copper)
for x in [-1.88,1.94]:
 box('Status tower',(x,.94,1.65),(.23,.26,.46),dark,.04)
 box('Status light',(x,.795,1.75),(.16,.025,.065),cyan,.009)
 pipe('Antenna',(x,.94,1.9),(x,.94,2.26),.02,steel)
for y in [.74,1.04]:bent_pipe('Insulated cable',[(.86,y,2.21),(1.2,y,2.2),(1.52,y,1.94),(1.58,y,1.55)],.045,dark)
bpy.ops.object.select_all(action='DESELECT')
for o in parts:
 o.select_set(True);bpy.context.view_layer.objects.active=o
 for mod in list(o.modifiers):bpy.ops.object.modifier_apply(modifier=mod.name)
bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();body=bpy.context.object;body.name='SM_ConnectorFactory'
scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=.008);bpy.ops.object.mode_set(mode='OBJECT')
scene.render.engine='CYCLES';scene.cycles.samples=16
for kind,bake_type in [('BaseColor','DIFFUSE'),('AO','AO')]:
 img=bpy.data.images.new('T_ConnectorFactory_'+kind,width=1024,height=1024,alpha=False)
 for m in body.data.materials:
  node=m.node_tree.nodes.new('ShaderNodeTexImage');node.image=img;m.node_tree.nodes.active=node
 scene.render.bake.use_pass_direct=False;scene.render.bake.use_pass_indirect=False;scene.render.bake.use_pass_color=True;scene.render.bake.margin=8
 bpy.ops.object.bake(type=bake_type);img.filepath_raw=str(out/('T_ConnectorFactory_'+kind+'.png'));img.file_format='PNG';img.save()
bpy.ops.export_scene.fbx(filepath=str(out/'SM_ConnectorFactory.fbx'),use_selection=True,apply_unit_scale=True,axis_forward='-Y',axis_up='Z',bake_anim=False,object_types={'MESH'})
bpy.ops.object.camera_add(location=(7,-9,7));cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,1))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=6.5;scene.camera=cam
for loc,power,size in [((1,-4,8),1200,6),((-4,2,5),850,5)]:
 bpy.ops.object.light_add(type='AREA',location=loc);o=bpy.context.object;o.data.energy=power;o.data.size=size;o.rotation_euler=(Vector((0,0,1))-o.location).to_track_quat('-Z','Y').to_euler()
scene.world=bpy.data.worlds.new('Connector studio');scene.world.color=(.18,.18,.18)
scene.render.resolution_x=1024;scene.render.resolution_y=1024;scene.render.resolution_percentage=100;scene.render.image_settings.file_format='PNG';scene.render.film_transparent=True;scene.render.filepath=str(out/'T_ConnectorFactory.png')
bpy.ops.wm.save_as_mainfile(filepath=str(out/'ConnectorFactory.blend'));bpy.ops.render.render(write_still=True)
result={'blend':str(out/'ConnectorFactory.blend'),'preview':str(out/'T_ConnectorFactory.png'),'vertices':len(body.data.vertices),'dimensions':list(body.dimensions)}

