import bpy,math,json
from pathlib import Path
out=Path(r'C:\UE5\SurviveThePlanet 5.8\ContentSource\RemoteBase')
scene=bpy.data.scenes['STP_RemoteBase'];bpy.context.window.scene=scene
scene.frame_end=901
antenna=bpy.data.objects['SM_RemoteBase_Antenna'];antenna.animation_data_clear()
for frame,angle in [(1,0),(901,math.tau)]:
    antenna.rotation_euler.z=angle;antenna.keyframe_insert(data_path='rotation_euler',frame=frame)
for layer in antenna.animation_data.action.layers:
    for strip in layer.strips:
        for bag in strip.channelbags:
            for curve in bag.fcurves:
                for point in curve.keyframe_points:point.interpolation='LINEAR'
                curve.modifiers.new('CYCLES')
scan=bpy.data.objects['SM_RemoteBase_Scan'];scan.animation_data_clear()
for axis in [0,1]:
    scan.driver_add('scale',axis).driver.expression='1 + 0.65 * (((frame - 1) % 120) / 120)'
cyan=bpy.data.materials['Remote_Cyan']
cyan.node_tree.nodes.get('Principled BSDF').inputs['Emission Strength'].driver_add('default_value').driver.expression='2.3 + 0.7 * sin(2 * pi * (frame - 1) / 90)'
mat=bpy.data.materials.new('Remote_ScanPulse');mat.use_nodes=True
nodes=mat.node_tree.nodes;nodes.clear()
output=nodes.new('ShaderNodeOutputMaterial');transparent=nodes.new('ShaderNodeBsdfTransparent');emission=nodes.new('ShaderNodeEmission');mix=nodes.new('ShaderNodeMixShader')
emission.inputs['Color'].default_value=(.008,.65,.95,1);emission.inputs['Strength'].default_value=3
mix.inputs[0].driver_add('default_value').driver.expression='1 - (((frame - 1) % 120) / 120)'
mat.node_tree.links.new(transparent.outputs[0],mix.inputs[1]);mat.node_tree.links.new(emission.outputs[0],mix.inputs[2]);mat.node_tree.links.new(mix.outputs[0],output.inputs['Surface'])
scan.data.materials.clear();scan.data.materials.append(mat)
scene.frame_set(76)
evaluated=antenna.evaluated_get(bpy.context.evaluated_depsgraph_get())
report={'antenna_frame_76_yaw_degrees':math.degrees(evaluated.rotation_euler.z),'scan_frame_76_scale':list(scan.evaluated_get(bpy.context.evaluated_depsgraph_get()).scale),'frame_end':scene.frame_end,'fps':scene.render.fps}
assert abs(report['antenna_frame_76_yaw_degrees']-30)<.1,report
scene.frame_set(1)
bpy.ops.wm.save_as_mainfile(filepath=str(out/'RemoteBase.blend'),copy=True)
(out/'RemoteBase_animation_check.json').write_text(json.dumps(report,indent=2))
result=report
