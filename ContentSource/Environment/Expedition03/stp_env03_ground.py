import bpy,os,json
out=r'C:\UE5\SurviveThePlanet 5.8\ContentSource\Environment\Expedition03'
os.makedirs(out,exist_ok=True)
scene=bpy.data.scenes.new('STP_Terrain03');bpy.context.window.scene=scene
scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
mesh=bpy.data.meshes.new('TerrainFlat03')
mesh.from_pydata([(-10,-10,.5655),(10,-10,.5655),(10,10,.5655),(-10,10,.5655)],[],[(0,1,2,3)])
mesh.update();ob=bpy.data.objects.new('SM_TerrainFlat_03',mesh);scene.collection.objects.link(ob)
bpy.context.view_layer.objects.active=ob;ob.select_set(True)
uv=mesh.uv_layers.new(name='UVMap')
for i,p in enumerate([(0,0),(1,0),(1,1),(0,1)]):uv.data[i].uv=p
bpy.ops.export_scene.fbx(filepath=out+'/SM_TerrainFlat_03.fbx',use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False)
bpy.ops.wm.save_as_mainfile(filepath=out+'/STP_Terrain03.blend',copy=True)
print(json.dumps({'exported':out+'/SM_TerrainFlat_03.fbx','size_m':20,'surface_z_cm':56.55}))
