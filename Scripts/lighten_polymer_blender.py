import bpy
from pathlib import Path
out=Path(r'C:\UE5\SurviveThePlanet 5.8\ContentSource\PolymerPlant')
scene=next(s for s in bpy.data.scenes if s.name=='STP_PolymerPlant_Refined');bpy.context.window.scene=scene
body=next(o for o in scene.objects if o.name.startswith('SM_PolymerPlant'))
palette={'Polymer_Ivory':(.94,.92,.87),'Polymer_Steel':(.53,.57,.58),'Polymer_Orange':(.88,.20,.025),'Polymer_Graphite':(.07,.085,.10)}
for m in body.data.materials:
 key=next((k for k in palette if m.name.startswith(k)),None)
 if not key:continue
 color=palette[key];m.diffuse_color=(*color,1);p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Metallic'].default_value=.06 if key=='Polymer_Ivory' else .3;p.inputs['Roughness'].default_value=.60
 for node in m.node_tree.nodes:
  if node.type=='VALTORGB':
   node.color_ramp.elements[0].color=(*(v*.90 for v in color),1);node.color_ramp.elements[1].color=(*color,1)
bpy.ops.object.select_all(action='DESELECT');body.select_set(True);bpy.context.view_layer.objects.active=body
image=bpy.data.images.get('T_PolymerPlant_BaseColor');assert image
for m in body.data.materials:
 node=next(n for n in m.node_tree.nodes if n.type=='TEX_IMAGE' and n.image==image);m.node_tree.nodes.active=node
scene.render.bake.use_pass_direct=False;scene.render.bake.use_pass_indirect=False;scene.render.bake.use_pass_color=True
bpy.ops.object.bake(type='DIFFUSE');image.filepath_raw=str(out/'T_PolymerPlant_BaseColor.png');image.save()
bpy.ops.wm.save_as_mainfile(filepath=str(out/'PolymerPlant.blend'));scene.render.filepath=str(out/'T_PolymerPlant.png');bpy.ops.render.render(write_still=True)
result={'palette':'warm white painted panels, light gray roof, orange accents','texture':str(image.filepath_raw)}
