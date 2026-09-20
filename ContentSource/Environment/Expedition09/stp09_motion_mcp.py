import json
def call(group,method,args):
    return execute_tool('editor_toolset.toolsets.'+group+'.'+method,json.dumps(args))
def props(obj,keys):
    return json.loads(call('object.ObjectTools','get_properties',{'instance':obj,'properties':keys})['returnValue'])
def setprops(obj,values):
    if not call('object.ObjectTools','set_properties',{'instance':obj,'values':json.dumps(values)})['returnValue']:
        raise RuntimeError('Property update failed: '+str(values))
def run():
    paths=['/Game/Environment/Expedition02/Materials/M_Plant_'+n+'_Wind' for n in ['Coral_Red','DarkStem','Ember_Tips','Gold','Inside','Olive','Petal','Tube']]
    paths+=['/Game/Environment/Expedition06/Materials/M_Plant06_'+n for n in ['Cap','Coral','Ember','Gills','Gold','Inside','Leaves','Olive','Stem','Tube']]
    done=[];changes=[]
    try:
        for path in paths:
            m={'refPath':path+'.'+path.split('/')[-1]}
            nodes=call('material.MaterialTools','get_expressions',{'material_or_function':m})['returnValue']
            for n in nodes:
                p=n['refPath']
                if 'MaterialExpressionTime_' in p:
                    old=props(n,['bIgnorePause']);setprops(n,{'bIgnorePause':True});changes.append({'node':p,'before':old,'after':{'bIgnorePause':True}})
                elif 'ScalarParameter' in p:
                    old=props(n,['ParameterName','DefaultValue']);name=old.get('ParameterName')
                    values={'WindStrength':3.0,'WindSpeed':1.2,'BloomPeriodSeconds':12.0,'AutoBloom':1.0}
                    if name in values:
                        setprops(n,{'DefaultValue':values[name]});changes.append({'node':p,'before':old,'after':values[name]})
            call('material.MaterialTools','recompile',{'material_or_function':m})
            if not call('asset.AssetTools','save_assets',{'asset_paths':[path]})['returnValue']:raise RuntimeError('Could not save '+path)
            done.append(path)
        return {'complete':True,'materials':done,'changes':changes,'clock':'real time, continues through pause and time dilation','cycle_seconds':12}
    except Exception as e:
        return {'complete':False,'materials':done,'changes':changes,'error':str(e)}
