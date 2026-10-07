import bpy
from pathlib import Path
scene=bpy.data.scenes['STP_RemoteBase']
bpy.context.window.scene=scene
hidden={o:o.hide_render for o in scene.objects}
original=(scene.render.resolution_x,scene.render.resolution_y,scene.render.film_transparent,scene.render.filepath,scene.camera.data.ortho_scale,scene.frame_current)
for o in scene.objects:
    if o.type=='MESH' and not o.name.startswith('SM_RemoteBase_'): o.hide_render=True
scene.render.resolution_x=512;scene.render.resolution_y=512
scene.render.film_transparent=True
scene.render.image_settings.file_format='PNG';scene.render.image_settings.color_mode='RGBA'
scene.camera.data.ortho_scale=7.5
scene.frame_set(1)
scene.render.filepath=r'C:\UE5\SurviveThePlanet 5.8\ContentSource\RemoteBase\T_RemoteBase.png'
bpy.ops.render.render(write_still=True)
for o,value in hidden.items(): o.hide_render=value
scene.render.resolution_x,scene.render.resolution_y,scene.render.film_transparent,scene.render.filepath,scene.camera.data.ortho_scale,frame=original
scene.frame_set(frame)
result={'icon':r'C:\UE5\SurviveThePlanet 5.8\ContentSource\RemoteBase\T_RemoteBase.png'}
