import json,time
def find(name):
    return execute_tool('editor_toolset.toolsets.scene.SceneTools.find_actors',json.dumps({'name':name,'tag':'','collision_channels':[]}))['returnValue']
def transform(a):
    return execute_tool('editor_toolset.toolsets.actor.ActorTools.get_actor_transform',json.dumps({'actor':a}))['returnValue']
def capture(pose):
    return execute_tool('EditorToolset.EditorAppToolset.CaptureViewport',json.dumps({'captureTransform':pose,'annotations':None,'bShowUI':False}))['returnValue']['image']
def run():
    p=transform(find('E06_Vegetation_0_00')[0])['location']
    pose={'location':{'x':p['x'],'y':p['y']-300,'z':p['z']+180},'rotation':{'pitch':-25,'yaw':90,'roll':0}}
    frames=[]
    for i in range(8):
        t=time.monotonic();im=capture(pose);frames.append({'time':t,'image':im});time.sleep(.5)
    return {'frames':frames,'target':p}
