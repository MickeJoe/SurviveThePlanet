import unreal as u,os
root=r'C:\UE5\SurviveThePlanet 5.8\ContentSource\Environment\Expedition04'
dst='/Game/Environment/Expedition04'
tasks=[]
for name in ['SM_Rock04_Pillar','SM_Rock04_Slab','SM_Rock04_Wedge','SM_Rock04_Foot']:
 for filename,folder in [(name+'.fbx','Meshes'),('T_'+name+'_BaseColor.png','Textures'),('T_'+name+'_Normal.png','Textures')]:
  t=u.AssetImportTask();t.filename=root+'/'+filename;t.destination_path=dst+'/'+folder;t.destination_name=os.path.splitext(filename)[0];t.automated=True;t.replace_existing=True;t.replace_existing_settings=True;t.save=True
  if folder=='Meshes':
   opts=u.FbxImportUI();opts.import_mesh=True;opts.import_as_skeletal=False;opts.import_materials=False;opts.import_textures=False;opts.mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH;opts.automated_import_should_detect_type=False
   opts.static_mesh_import_data.combine_meshes=True;opts.static_mesh_import_data.generate_lightmap_u_vs=False;opts.static_mesh_import_data.auto_generate_collision=False;opts.static_mesh_import_data.normal_import_method=u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS;t.options=opts
  tasks.append(t)
u.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
exec(open(r'C:\UE5\SurviveThePlanet 5.8\ContentSource\Environment\Expedition04\stp_ue04.py',encoding='utf-8-sig').read())

