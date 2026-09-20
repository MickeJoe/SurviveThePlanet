import json
def exists(path):
    return execute_tool('editor_toolset.toolsets.asset.AssetTools.exists',json.dumps({'path':path}))['returnValue']
def import_mesh(name):
    return execute_tool('editor_toolset.toolsets.static_mesh.StaticMeshTools.import_file',json.dumps({'folder_path':'/Game/Environment/Expedition08/Library','asset_name':name,'source_file':'C:/UE5/SurviveThePlanet 5.8/ContentSource/Environment/Expedition08/'+name+'.fbx','import_materials':True}))['returnValue']
def slots(mesh):
    return execute_tool('editor_toolset.toolsets.static_mesh.StaticMeshTools.get_material_slots',json.dumps({'mesh':mesh}))['returnValue']
def assign(mesh,slot,material):
    return execute_tool('editor_toolset.toolsets.static_mesh.StaticMeshTools.set_material',json.dumps({'mesh':mesh,'slot_name':slot,'material':{'refPath':material+'.'+material.split('/')[-1]}}))['returnValue']
def save(paths):
    return execute_tool('editor_toolset.toolsets.asset.AssetTools.save_assets',json.dumps({'asset_paths':paths}))['returnValue']
def run():
    names=['SM_Cliff08_Corner','SM_Cliff08_End','SM_Cliff08_Arch','SM_Organic08_DeadTree','SM_Organic08_DeadLog','SM_Organic08_Roots','SM_Organic08_RibBones','SM_Atmos08_Vent','SM_Atmos08_CraterRim','SM_Atmos08_WreckFrame','SM_Mineral08_CrystalCluster']
    result=[]
    for name in names:
        path='/Game/Environment/Expedition08/Library/'+name
        imported=[] if exists(path) else import_mesh(name)
        mesh={'refPath':path+'.'+name}
        ss=slots(mesh)
        for slot in ss:
            if 'Reuse_' in slot:
                kind=next(k for k in ['Pillar','Slab','Wedge','Foot'] if k in slot)
                assert assign(mesh,slot,'/Game/Environment/Expedition04/Materials/M_SM_Rock04_'+kind)
        assert save([path]+[a['refPath'] for a in imported])
        result.append({'name':name,'slots':ss})
    return {'imported':result}

