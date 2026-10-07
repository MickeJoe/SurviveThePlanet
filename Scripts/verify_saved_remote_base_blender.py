import bpy,math
scene=bpy.data.scenes['STP_RemoteBase']
window=list(bpy.context.window_manager.windows)[0]
assert window.scene==scene,window.scene.name
with bpy.context.temp_override(window=window):
    scene.frame_set(76)
    deps=bpy.context.evaluated_depsgraph_get()
    antenna=bpy.data.objects['SM_RemoteBase_Antenna'].evaluated_get(deps)
    angle=math.degrees(antenna.rotation_euler.z)
    assert abs(angle-30)<.1,angle
    scene.frame_set(1)
result={'standalone':True,'file':bpy.data.filepath,'active_scene':window.scene.name,'antenna_frame_76_yaw':angle,'mesh_count':sum(o.type=='MESH' for o in scene.objects),'objects':len(scene.objects)}
