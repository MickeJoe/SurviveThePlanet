import bpy
from pathlib import Path
scene=bpy.context.scene
for m in bpy.data.materials:
    if not m.name.startswith('Coal_') or not m.use_nodes:continue
    bsdf=m.node_tree.nodes.get('Principled BSDF')
    if not bsdf:continue
    if m.name.startswith('Coal_Armor'):color=(.40,.365,.29)
    elif m.name.startswith('Coal_Steel'):color=(.17,.195,.205)
    else:continue
    n=m.node_tree.nodes.new('ShaderNodeTexNoise');n.inputs['Scale'].default_value=38;n.inputs['Detail'].default_value=2
    ramp=m.node_tree.nodes.new('ShaderNodeValToRGB')
    ramp.color_ramp.elements[0].color=(*(v*.48 for v in color),1)
    ramp.color_ramp.elements[1].color=(*color,1)
    m.node_tree.links.new(n.outputs['Fac'],ramp.inputs[0]);m.node_tree.links.new(ramp.outputs[0],bsdf.inputs['Base Color'])
    bump=m.node_tree.nodes.new('ShaderNodeBump');bump.inputs['Strength'].default_value=.12;bump.inputs['Distance'].default_value=.009
    m.node_tree.links.new(n.outputs['Fac'],bump.inputs['Height']);m.node_tree.links.new(bump.outputs[0],bsdf.inputs['Normal'])
for o in scene.objects:
    if o.type=='MESH':o.hide_render=o.name.startswith(('SM_CoalDeposit','SM_CoalChip'))
scene.camera.data.ortho_scale=4.4
scene.render.filepath=r'C:\UE5\SurviveThePlanet 5.8\ContentSource\Coal\T_CoalMine.png'
bpy.ops.render.render(write_still=True)
bpy.ops.wm.save_as_mainfile(filepath=r'C:\UE5\SurviveThePlanet 5.8\ContentSource\Coal\Coal.blend',copy=True)
result={'weathered_materials':True}
