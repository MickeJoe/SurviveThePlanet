import json,time
def capture(pose):
    return execute_tool('EditorToolset.EditorAppToolset.CaptureViewport',json.dumps({'captureTransform':pose,'annotations':None,'bShowUI':False}))['returnValue']['image']
def run():
    pose={'location':{'x':-1100,'y':-2450,'z':260},'rotation':{'pitch':-25,'yaw':90,'roll':0}}
    frames=[]
    for i in range(8):
        t=time.monotonic();im=capture(pose);frames.append({'time':t,'image':im});time.sleep(.5)
    return {'frames':frames}
