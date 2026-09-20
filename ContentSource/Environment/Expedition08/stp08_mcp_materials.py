import json
def call(group,method,args):
    return execute_tool('editor_toolset.toolsets.'+group+'.'+method,json.dumps(args))
def node(m,kind,values):
    n=call('material.MaterialTools','add_expression',{'material_or_function':m,'expression_class':{'refPath':'/Script/Engine.MaterialExpression'+kind}})['returnValue']
    assert call('object.ObjectTools','set_properties',{'instance':n,'values':json.dumps(values)})['returnValue']
    return n
def link(a,b,pin):
    call('material.MaterialTools','connect_expressions',{'from_expression':a,'from_output_name':'','to_expression':b,'to_input_name':pin})
def output(n,prop):
    call('material.MaterialTools','connect_to_output',{'expression':n,'output_name':'','material_property':'MP_'+prop})
def save(paths):
    return call('asset.AssetTools','save_assets',{'asset_paths':paths})['returnValue']
def run():
    completed=[]
    try:
        palette={'Bark':[.095,.065,.038],'Bone':[.30,.235,.15],'Metal':[.07,.085,.095],'Rust':[.24,.085,.025],'Vent':[.10,.075,.045],'Crystal':[.43,.095,.008]}
        for kind,c in palette.items():
            name='M_Library08_'+kind
            path='/Game/Environment/Expedition08/Materials/'+name
            if call('asset.AssetTools','exists',{'path':path})['returnValue']:
                completed.append(path)
                continue
            m=call('material.MaterialTools','create_material',{'folder_path':'/Game/Environment/Expedition08/Materials','asset_name':name})['returnValue']
            noise=node(m,'Noise',{'Scale':.065,'Quality':1,'Levels':2,'OutputMin':0.0,'OutputMax':1.0})
            lo=node(m,'Constant3Vector',{'Constant':{'R':c[0]*.65,'G':c[1]*.65,'B':c[2]*.65,'A':1}})
            hi=node(m,'Constant3Vector',{'Constant':{'R':c[0]*1.2,'G':c[1]*1.2,'B':c[2]*1.2,'A':1}})
            mix=node(m,'LinearInterpolate',{'ConstAlpha':.5})
            link(lo,mix,'A');link(hi,mix,'B');link(noise,mix,'Alpha');output(mix,'BaseColor')
            output(node(m,'Constant',{'R':.68 if kind=='Metal' else .92}),'Roughness')
            if kind=='Metal':output(node(m,'Constant',{'R':.65}),'Metallic')
            if kind=='Crystal':output(node(m,'Constant3Vector',{'Constant':{'R':.075,'G':.011,'B':.0005,'A':1}}),'EmissiveColor')
            call('material.MaterialTools','recompile',{'material_or_function':m})
            assert save([path])
            completed.append(path)
        names=['SM_Organic08_DeadTree','SM_Organic08_DeadLog','SM_Organic08_Roots','SM_Organic08_RibBones','SM_Atmos08_Vent','SM_Atmos08_CraterRim','SM_Atmos08_WreckFrame','SM_Mineral08_CrystalCluster']
        for name in names:
            path='/Game/Environment/Expedition08/Library/'+name
            mesh={'refPath':path+'.'+name}
            for slot in call('static_mesh.StaticMeshTools','get_material_slots',{'mesh':mesh})['returnValue']:
                if 'Reuse_' in slot:continue
                kind=next((k for k in palette if k in slot),'Vent');mat='M_Library08_'+kind
                assert call('static_mesh.StaticMeshTools','set_material',{'mesh':mesh,'slot_name':slot,'material':{'refPath':'/Game/Environment/Expedition08/Materials/'+mat+'.'+mat}})['returnValue']
            assert save([path])
        return {'materials':completed,'assigned':len(names)}
    except Exception as e:
        return {'completed':completed,'error':str(e)}
