import bpy
from pathlib import Path
scene=bpy.context.scene
palette={'Coal_Armor':(.76,.73,.66),'Coal_Steel':(.38,.43,.45),'Coal_Chassis':(.115,.135,.15),'Coal_SafetyYellow':(.95,.22,.025),'Coal_StatusCyan':(.015,.8,1.0)}
for material in bpy.data.materials:
    key=next((k for k in palette if material.name.startswith(k)),None)
    if not key or not material.use_nodes:continue
    color=palette[key];material.diffuse_color=(*color,1)
    bsdf=material.node_tree.nodes.get('Principled BSDF')
    # Match the shared faction palette while retaining the modeled paint wear.
    for link in list(bsdf.inputs['Base Color'].links):material.node_tree.links.remove(link)
    bsdf.inputs['Base Color'].default_value=(*color,1)
    bsdf.inputs['Metallic'].default_value=.12 if key=='Coal_Armor' else .4
    if key=='Coal_StatusCyan':bsdf.inputs['Emission Color'].default_value=(*color,1)
root=bpy.data.objects.new('CoalMine_VisualScale_125',None);scene.collection.objects.link(root)
root['ue_building_mesh_scale']=1.25
for obj in scene.objects:
    if obj.type=='MESH' and obj.name.startswith(('SM_CoalMine','SM_CoalCutter','SM_CoalPayload')):
        obj.parent=root
root.scale=(1.25,1.25,1.25)
scene.frame_set(1)
for obj in scene.objects:
    if obj.type=='MESH':obj.hide_render=obj.name.startswith(('SM_CoalDeposit','SM_CoalChip'))
scene.camera.data.ortho_scale=4.4
scene.render.filepath=r'C:\UE5\SurviveThePlanet 5.8\ContentSource\Coal\T_CoalMine.png'
bpy.ops.render.render(write_still=True)
bpy.ops.wm.save_as_mainfile(filepath=r'C:\UE5\SurviveThePlanet 5.8\ContentSource\Coal\Coal.blend',copy=True)
result={'building_visual_scale':1.25,'palette':'Energy Extender ivory/orange/graphite/steel/cyan'}
