import unreal as u,json,os
ROOT=r'C:\UE5\SurviveThePlanet 5.8';DST='/Game/Environment/Expedition08/Library'
for name in ['SM_Cliff08_Arch','SM_Atmos08_Vent','SM_Atmos08_CraterRim']:
    task=u.AssetImportTask();task.filename=ROOT+'/ContentSource/Environment/Expedition08/'+name+'.fbx';task.destination_path=DST;task.destination_name=name;task.replace_existing=True;task.automated=True;task.save=False;task.factory=u.FbxFactory()
    options=u.FbxImportUI();options.automated_import_should_detect_type=False;options.import_mesh=True;options.import_as_skeletal=False;options.mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH;options.import_materials=False;options.import_textures=False;options.static_mesh_import_data.combine_meshes=True;options.static_mesh_import_data.auto_generate_collision=False;options.static_mesh_import_data.normal_import_method=u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS
    task.options=options;u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task]);m=u.load_asset(DST+'/'+name);assert m
    for i,slot in enumerate(m.static_materials):
        label=str(slot.material_slot_name)
        if 'Reuse_' in label:
            kind=next(k for k in ['Pillar','Slab','Wedge','Foot'] if k in label);mat=u.load_asset('/Game/Environment/Expedition04/Materials/M_SM_Rock04_'+kind)
        else:mat=u.load_asset('/Game/Environment/Expedition08/Materials/M_Library08_Vent')
        assert mat;m.set_material(i,mat)
    if 'Cliff' in name:m.get_editor_property('body_setup').set_editor_property('collision_trace_flag',u.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    assert u.EditorAssetLibrary.save_loaded_asset(m,False)
exec(open(ROOT+'/ContentSource/Environment/Expedition08/stp08_finish_offscreen.py').read())
