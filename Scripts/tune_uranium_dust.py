from pathlib import Path
import json
p=Path(__file__).parent
exec(Path('C:/UE5/SurviveThePlanet 5.8/Scripts/unreal_mcp_request.py').read_text().split("\nrequest({'jsonrpc'")[0])
request({'jsonrpc':'2.0','id':0,'method':'initialize','params':{'protocolVersion':'2024-11-05','capabilities':{},'clientInfo':{'name':'Codex','version':'1'}}})
request({'jsonrpc':'2.0','method':'notifications/initialized'})
def call(name,args):
 r=request({'jsonrpc':'2.0','id':1,'method':'tools/call','params':{'name':'call_tool','arguments':{'toolset_name':'NiagaraToolsets.NiagaraToolset_System','tool_name':name,'arguments':args}}})
 if r.get('result',{}).get('isError'):raise RuntimeError(r)
 data=r['result']['content'][0]['text'];value=json.loads(data) if isinstance(data,str) else data
 return value.get('returnValue',value)
system={'refPath':'/Game/Uranium/NS_UraniumDust.NS_UraniumDust'}
def ref(emitter,script='',module='',inputs=None):return dict(system=system,emitterName=emitter,scriptName=script,moduleName=module,inputNameStack=inputs or [],rendererIndex=-1)
def value(script,module,name,struct,data):
 return call('SetStackInputData',{'stackInputRef':ref('Steam',script,module,[name]),'inputData':{'struct':{'refPath':struct},'value':data}})
def scalar(script,module,name,data):return value(script,module,name,'/Script/Niagara.NiagaraFloat',{'value':data})
def expression(script,module,name,data):return value(script,module,name,'/Script/NiagaraEditor.NiagaraExt_StackInputData_HlslExpression',{'hlslExpression':data})
scalar('EmitterUpdateScript','SpawnRate','SpawnRate',4)
for name,data in [('Lifetime Min',1.8),('Lifetime Max',2.7),('Uniform Sprite Size Min',8),('Uniform Sprite Size Max',12)]:scalar('ParticleSpawnScript','InitializeParticle',name,data)
scalar('ParticleSpawnScript','ShapeLocation','Sphere Radius',48)
scalar('ParticleSpawnScript','AddVelocity','Velocity Speed',8)
expression('ParticleUpdateScript','SolveForcesAndVelocity','Velocity','float3(User.Wind.x * 0.2, User.Wind.y * 0.2, 8.0)')
request({'jsonrpc':'2.0','id':2,'method':'tools/call','params':{'name':'call_tool','arguments':{'toolset_name':'editor_toolset.toolsets.asset.AssetTools','tool_name':'save_assets','arguments':{'asset_paths':['/Game/Uranium/NS_UraniumDust']}}}})
print('Uranium dust tuned and saved')
