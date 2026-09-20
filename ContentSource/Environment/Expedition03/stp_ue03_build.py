import unreal as u,os,json,math,time,traceback
exec(open(r'C:\UE5\SurviveThePlanet 5.8\ContentSource\Environment\Expedition02\stp_ue02_build.py').read().split('try:main()')[0])
DST='/Game/Environment/Expedition03'
LOG=ROOT+r'\Saved\Expedition03';os.makedirs(LOG,exist_ok=True)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert world.get_name()=='PlanetLevel_RockPreview'
u.EditorAssetLibrary.duplicate_asset('/Game/PlanetLevel_RockPreview','/Game/Environment/Expedition03/PlanetLevel_Before03') if not u.EditorAssetLibrary.does_asset_exist('/Game/Environment/Expedition03/PlanetLevel_Before03') else None
mesh=importfile(ROOT+r'\ContentSource\Environment\Expedition03\SM_TerrainFlat_03.fbx','SM_TerrainFlat_03','mesh')
mat=newmat('M_Terrain_03')
wp=node(mat,u.MaterialExpressionWorldPosition)
noise='''struct N { float h(float2 p){float3 q=frac(float3(p.xyx)*.1031);q+=dot(q,q.yzx+33.33);return frac((q.x+q.y)*q.z);} float v(float2 p){float2 i=floor(p),f=frac(p);f=f*f*(3-2*f);return lerp(lerp(h(i),h(i+float2(1,0)),f.x),lerp(h(i+float2(0,1)),h(i+1),f.x),f.y);} };N n;float2 p=P.xy;'''
code=noise+'''float broad=n.v(p/1100),mid=n.v(p/140),fine=n.v(p/4);float rock=0;'''
for x,y,r in clusters:code+=f'rock=max(rock,1-smoothstep({r*.5},{r*1.65},length(p-float2({x},{y}))+(mid-.5)*240));'
code+='''float sand=smoothstep(.5,.85,broad)*(1-rock);float3 c=lerp(float3(.19,.135,.083),float3(.255,.205,.14),sand);c=lerp(c,float3(.14,.121,.095),rock*.78);c*=.94+.09*mid+.08*fine;
float2 cell=floor(p/13);float rnd=n.h(cell);float2 j=float2(n.h(cell+11.1),n.h(cell+29.2));float d=length(frac(p/13)-(.2+j*.6));float peb=(1-smoothstep(.04,.09+rnd*.11,d))*step(.52,rnd)*rock;
return lerp(c,float3(.25,.225,.185)*(.8+rnd*.3),peb*.65);'''
c=custom(mat,code,{'P':(wp,'')});prop(c,'',u.MaterialProperty.MP_BASE_COLOR)
mat.set_editor_property('tangent_space_normal',False)
normal=custom(mat,noise+'float h=n.v(p/5);float dx=n.v((p+float2(.8,0))/5)-h;float dy=n.v((p+float2(0,.8))/5)-h;return normalize(float3(-dx*.7,-dy*.7,1));',{'P':(wp,'')})
prop(normal,'',u.MaterialProperty.MP_NORMAL);prop(scalar(mat,'Roughness',.94),'',u.MaterialProperty.MP_ROUGHNESS);finishmat(mat)
mesh.set_material(0,mat)
mesh.get_editor_property('body_setup').set_editor_property('collision_trace_flag',u.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
save(mesh)
surface=next(a for a in ae.get_all_level_actors() if 'SurfaceManager' in a.get_name())
surface.set_editor_property('build_in_construction_script',False)
for component in list(surface.get_components_by_class(u.StaticMeshComponent)):component.destroy_component(surface)
surface.set_editor_property('chunk_meshes',[mesh]);surface.set_editor_property('chunk_height_offset',-surface.get_actor_location().z)
surface.call_method('RebuildSurface')
cc=surface.get_components_by_class(u.StaticMeshComponent)
assert len(cc)==80,str(len(cc))
reserved_cells=0
rocks=[a for a in ae.get_all_level_actors() if a.get_actor_label().startswith(('E02_Cliff_','E02_Boulder_'))]
for a in rocks:
    center,extent=a.get_actor_bounds(False)
    fp=u.IntPoint(x=max(1,math.ceil(extent.x*1.2/100)),y=max(1,math.ceil(extent.y*1.2/100)))
    placement=surface.get_placement_for_world_location(center,fp)
    for y in range(fp.y):
        for x in range(fp.x):
            cell=u.STPGridCell(x=placement.origin_cell.x+x,y=placement.origin_cell.y+y)
            if surface.reserve_cells(a,cell,u.IntPoint(x=1,y=1)):reserved_cells+=1
u.EditorLoadingAndSavingUtils.save_map(world,'/Game/PlanetLevel_RockPreview')
with open(LOG+'/build.json','w') as f:json.dump({'ground_chunks':len(cc),'new_reserved_cells':reserved_cells,'rock_count':len(rocks),'material':mat.get_path_name(),'status':'saved'},f,indent=2)
cam=next(a for a in ae.get_all_level_actors() if a.get_actor_label()=='E02_OverviewCamera')
u.get_editor_subsystem(u.UnrealEditorSubsystem).set_level_viewport_camera_info(cam.get_actor_location(),cam.get_actor_rotation())
start=time.monotonic()
def capture_tick(dt):
    if time.monotonic()-start>15:
        u.AutomationLibrary.take_high_res_screenshot(1600,1000,LOG+'/Overview03.png',camera=cam,delay=2)
        u.unregister_slate_post_tick_callback(handle)
handle=u.register_slate_post_tick_callback(capture_tick)
u.EditorPythonScripting.set_keep_python_script_alive(True)
