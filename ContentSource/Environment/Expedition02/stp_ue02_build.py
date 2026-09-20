import unreal as u, os, json, math, random, traceback, time
ROOT=r'C:\UE5\SurviveThePlanet 5.8'
SRC=ROOT+r'\ContentSource\Environment\Expedition02'
DST='/Game/Environment/Expedition02'
LOG=ROOT+r'\Saved\Expedition02'
os.makedirs(LOG,exist_ok=True)
ae=u.get_editor_subsystem(u.EditorActorSubsystem)
at=u.AssetToolsHelpers.get_asset_tools();me=u.MaterialEditingLibrary
report={'warnings':[],'steps':[]}
def checkpoint(step):
    report['steps'].append(step)
    with open(LOG+'/build_report.json','w') as f:json.dump(report,f,indent=2)
    u.log('STP_ENV02 '+step)
def asset(path):return u.EditorAssetLibrary.load_asset(path)
def save(obj):return u.EditorAssetLibrary.save_loaded_asset(obj,False)
def node(mat,cls,**props):
    ob=me.create_material_expression(mat,cls)
    for k,v in props.items():ob.set_editor_property(k,v)
    return ob
def conn(a,out,b,inp):
    ok=me.connect_material_expressions(a,out,b,inp)
    if not ok:raise RuntimeError('Material connection failed: '+a.get_name()+' '+out+' '+b.get_name()+' '+inp)
def prop(a,out,p):
    if not me.connect_material_property(a,out,p):raise RuntimeError('Material output failed')
def scalar(mat,name,value):return node(mat,u.MaterialExpressionScalarParameter,parameter_name=name,default_value=value)
def vector(mat,name,color):return node(mat,u.MaterialExpressionVectorParameter,parameter_name=name,default_value=u.LinearColor(*color,1))
def custom(mat,code,inputs,output=None):
    ob=node(mat,u.MaterialExpressionCustom,code=code,output_type=output or u.CustomMaterialOutputType.CMOT_FLOAT3)
    ci=[]
    for name in inputs:
        x=u.CustomInput();x.set_editor_property('input_name',name);ci.append(x)
    ob.set_editor_property('inputs',ci)
    for name,(source,pin) in inputs.items():conn(source,pin,ob,name)
    return ob
def newmat(name):
    path=DST+'/Materials/'+name
    mat=asset(path)
    if not mat:mat=at.create_asset(name,DST+'/Materials',u.Material,u.MaterialFactoryNew())
    else:me.delete_all_material_expressions(mat)
    mat.set_editor_property('two_sided',False)
    return mat
def finishmat(mat):
    me.layout_material_expressions(mat);me.recompile_material(mat);save(mat)
def importfile(path,name,kind):
    exists=asset(DST+'/Meshes/'+name if kind=='mesh' else DST+'/Textures/'+name)
    if exists:return exists
    task=u.AssetImportTask();task.filename=path;task.destination_path=DST+('/Meshes' if kind=='mesh' else '/Textures');task.destination_name=name;task.automated=True;task.replace_existing=False;task.save=True
    if kind=='mesh':
        opts=u.FbxImportUI();opts.import_mesh=True;opts.import_as_skeletal=False;opts.import_materials=True;opts.import_textures=False;opts.mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH;opts.automated_import_should_detect_type=False
        opts.static_mesh_import_data.combine_meshes=True;opts.static_mesh_import_data.generate_lightmap_u_vs=False;opts.static_mesh_import_data.auto_generate_collision=False
        opts.static_mesh_import_data.vertex_color_import_option=u.VertexColorImportOption.REPLACE
        task.options=opts
    at.import_asset_tasks([task])
    objs=task.get_objects()
    if not objs:raise RuntimeError('Import failed '+name)
    return next((o for o in objs if isinstance(o,u.StaticMesh if kind=='mesh' else u.Texture2D)),objs[0])
def rock_material(name,textures):
    mat=newmat('M_'+name)
    color=node(mat,u.MaterialExpressionTextureSample,texture=textures['BaseColor'])
    normal=node(mat,u.MaterialExpressionTextureSample,texture=textures['Normal'],sampler_type=u.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    prop(color,'RGB',u.MaterialProperty.MP_BASE_COLOR);prop(normal,'RGB',u.MaterialProperty.MP_NORMAL)
    prop(scalar(mat,'Roughness',.93),'',u.MaterialProperty.MP_ROUGHNESS)
    finishmat(mat);return mat
def plant_material(name,color,opening=False):
    mat=newmat(name);mat.set_editor_property('two_sided',True)
    col=vector(mat,'BaseTint',color);local=node(mat,u.MaterialExpressionPreSkinnedPosition)
    vert=node(mat,u.MaterialExpressionVertexColor);clock=node(mat,u.MaterialExpressionTime)
    obj=node(mat,u.MaterialExpressionObjectPositionWS)
    strength=scalar(mat,'WindStrength',1.0)
    speed=scalar(mat,'WindSpeed',.75)
    code='''float h=saturate(W.r); float phase=dot(O.xy,float2(0.0017,0.0021));
float gust=0.6+0.4*sin(T*0.37+phase);
float sway=sin(T*Speed+phase)*Strength*gust;
return float3(sway*3.2*h, sin(T*Speed*0.81+phase+1.8)*Strength*1.7*h,0);'''
    ins={'W':(vert,''),'O':(obj,''),'T':(clock,''),'Strength':(strength,''),'Speed':(speed,'')}
    if opening:
        op=scalar(mat,'BloomOpen',.65);auto=scalar(mat,'AutoBloom',1);period=scalar(mat,'BloomPeriodSeconds',32)
        code='''float phase=dot(O.xy,float2(0.0017,0.0021));
float cycle=0.5-0.5*cos(T*6.283185/max(Period,1)+phase);
float angle=saturate(lerp(Open,cycle,Auto))*0.959931;
float3 q=float3(cos(angle)*P.x+sin(angle)*P.z,P.y,-sin(angle)*P.x+cos(angle)*P.z);
float h=saturate(W.r);float sway=sin(T*Speed+phase)*Strength*(0.7+0.3*sin(T*0.37+phase));
return q-P+float3(sway*2.4*h,sin(T*Speed*.81+phase)*1.5*Strength*h,0);'''
        ins.update({'P':(local,''),'Open':(op,''),'Auto':(auto,''),'Period':(period,'')})
    deform=custom(mat,code,ins)
    transform=node(mat,u.MaterialExpressionTransform,transform_source_type=u.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_LOCAL,transform_type=u.MaterialVectorCoordTransform.TRANSFORM_WORLD)
    conn(deform,'',transform,'');prop(transform,'',u.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    pigment_pos=node(mat,u.MaterialExpressionWorldPosition)
    tint=custom(mat,'float v=0.78+0.22*sin(P.z*0.13+P.x*0.08);return C.rgb*v;',{'P':(pigment_pos,''),'C':(col,'')})
    prop(tint,'',u.MaterialProperty.MP_BASE_COLOR);prop(scalar(mat,'Roughness',.78),'',u.MaterialProperty.MP_ROUGHNESS)
    finishmat(mat);return mat

# The environment clusters leave broad, interconnected clearings.
clusters=[(-1550,-1650,520),(700,-2350,500),(2620,-1130,530),(2800,1450,650),(500,2750,570),(-2000,2050,630),(-2680,600,480),(-700,900,330),(1500,850,300)]
def ground_material():
    mat=newmat('M_Terrain_Expedition02');wp=node(mat,u.MaterialExpressionWorldPosition)
    code='''struct Noise { float hash(float2 p){return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453);} float value(float2 p){float2 i=floor(p), f=frac(p);f=f*f*(3-2*f);return lerp(lerp(hash(i),hash(i+float2(1,0)),f.x),lerp(hash(i+float2(0,1)),hash(i+1),f.x),f.y);} }; Noise n;
float2 p=P.xy; float broad=n.value(p/850);float mid=n.value(p/180);float fine=n.value(p/3.5);float grit=n.value(p/1.2);
float rock=0;
'''
    for x,y,rad in clusters:code+=f'rock=max(rock,1-smoothstep({rad*.48:.3f},{rad*1.5:.3f},length(p-float2({x},{y}))+(mid-.5)*180));\n'
    code+='''float sand=smoothstep(.52,.8,broad)*(1-rock);
float3 dirt=float3(.205,.132,.075);float3 sandy=float3(.28,.19,.11);float3 gravel=float3(.13,.112,.087);
float3 color=lerp(dirt,sandy,sand);color=lerp(color,gravel,rock*.73);
color*=.86+mid*.21+fine*.12;
float2 cell=floor(p/12);float rnd=n.hash(cell);float2 jitter=float2(n.hash(cell+13.1),n.hash(cell+47.7));float2 f=frac(p/12)-(.2+.6*jitter);float pebble=(1-smoothstep(.06,.12+rnd*.13,length(f)))*step(.61,rnd)*rock;
color=lerp(color,float3(.25,.219,.17)*(.65+rnd*.5),pebble*.7);
return color*(.93+.09*grit);'''
    c=custom(mat,code,{'P':(wp,'')});prop(c,'',u.MaterialProperty.MP_BASE_COLOR);prop(scalar(mat,'Roughness',.96),'',u.MaterialProperty.MP_ROUGHNESS)
    mat.set_editor_property('tangent_space_normal',False)
    ground_normal=node(mat,u.MaterialExpressionConstant3Vector,constant=u.LinearColor(0,0,1,1))
    prop(ground_normal,'',u.MaterialProperty.MP_NORMAL)
    finishmat(mat);return mat
def spawn(mesh,label,x,y,scale=1,yaw=0,z=None,folder='Rocks',collision=True):
    if z is None:
        result=u.SystemLibrary.line_trace_single(u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world(),u.Vector(x,y,250),u.Vector(x,y,-250),u.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[],u.DrawDebugTrace.NONE,True)
        z=50
        if result[0]:z=result[1].to_tuple()[4].z if False else 50
    ob=ae.spawn_actor_from_object(mesh,u.Vector(x,y,z),u.Rotator(pitch=0,yaw=yaw,roll=0));ob.set_actor_label(label);ob.set_actor_scale3d(u.Vector(scale,scale,scale));ob.set_folder_path('Environment/Expedition02/'+folder);ob.tags=['STP_Expedition02']
    comp=ob.static_mesh_component
    comp.set_collision_enabled(u.CollisionEnabled.QUERY_AND_PHYSICS if collision else u.CollisionEnabled.NO_COLLISION)
    if not collision:comp.set_editor_property('cast_shadow',True)
    comp.set_editor_property('bounds_scale',1.7 if folder=='Vegetation' else 1)
    return ob
def clear_generated():
    for ob in ae.get_all_level_actors():
        if ob.actor_has_tag('STP_RockKit_Step01') or ob.actor_has_tag('STP_Expedition02'):
            ae.destroy_actor(ob)
def safe_xy(x,y,rad=170):
    if math.hypot(x-600,y-150)<850+rad:return False
    for sx,sy in [(-570,-1070),(2130,130),(630,1630),(1830,1330),(-870,130),(630,-1370),(1830,-1970),(1830,2230),(-1770,130)]:
        if math.hypot(x-sx,y-sy)<370+rad:return False
    return True
def main():
    assert u.Paths.get_project_file_path().replace('\\','/').lower().endswith('ue5/survivetheplanet 5.8/survivetheplanet.uproject')
    u.EditorLevelLibrary.load_level('/Game/PlanetLevel_RockPreview')
    meshes={};rockmats={};manifest=json.load(open(SRC+'/COMPLETE.json'))
    for row in manifest:
        if row['kind']=='ground':continue
        name=row['name'];mesh=importfile(SRC+'/'+name+'.fbx',name,'mesh');meshes[name]=mesh
        if row['kind']=='rock':
            tex={}
            for suffix in ['BaseColor','Normal']:
                tname='T_'+name+'_'+suffix;t=importfile(SRC+'/'+tname+'.png',tname,'texture')
                if suffix=='Normal':t.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_NORMALMAP);t.set_editor_property('srgb',False);t.set_editor_property('flip_green_channel',True)
                save(t);tex[suffix]=t
            mat=rock_material(name,tex);mesh.set_material(0,mat);rockmats[name]=mat
            u.get_editor_subsystem(u.StaticMeshEditorSubsystem).set_convex_decomposition_collisions(mesh,8,16,30000)
        save(mesh)
    checkpoint('Imported 10 meshes and 10 baked textures')
    palette={'M_Plant_Coral_Red':(.30,.022,.009),'M_Plant_Ember_Tips':(.49,.071,.014),'M_Plant_DarkStem':(.065,.041,.018),'M_Plant_Olive':(.125,.16,.025),'M_Plant_Gold':(.34,.19,.02),'M_Plant_Tube':(.37,.09,.012),'M_Plant_Inside':(.052,.016,.008),'M_Plant_Petal':(.32,.047,.012)}
    pmat={k:plant_material(k+'_Wind',v,k=='M_Plant_Petal') for k,v in palette.items()}
    for name,mesh in meshes.items():
        if 'Alien' not in name:continue
        for i,slot in enumerate(mesh.get_editor_property('static_materials')):
            sn=str(slot.material_slot_name)
            key=next((k for k in palette if k in sn),None)
            if key:mesh.set_material(i,pmat[key])
            else:report['warnings'].append('Unmatched plant slot '+name+' '+sn)
        save(mesh)
    checkpoint('Wind and opening materials created')
    terrain=ground_material()
    source=asset('/Game/Meshes/GroundChunk/GroundChunk1')
    copy=asset(DST+'/Meshes/SM_TerrainChunk_02') or u.EditorAssetLibrary.duplicate_asset(source.get_path_name(),DST+'/Meshes/SM_TerrainChunk_02')
    for i in range(len(copy.get_editor_property('static_materials'))):copy.set_material(i,terrain)
    save(copy)
    surface=next(a for a in ae.get_all_level_actors() if 'SurfaceManager' in a.get_name())
    surface.root_component.set_mobility(u.ComponentMobility.STATIC)
    surface.set_editor_property('build_in_construction_script',False)
    for component in list(surface.get_components_by_class(u.StaticMeshComponent)):component.destroy_component(surface)
    surface.set_editor_property('chunk_meshes',[copy])
    surface.set_editor_property('chunk_height_offset',-130.002014)
    surface.call_method('RebuildSurface')
    ground_center,ground_extent=surface.get_actor_bounds(False)
    assert abs(ground_center.z+ground_extent.z-56.55)<1
    clear_generated();rng=random.Random(73119);placed=[];large=[]
    names=['SM_Cliff_Crown_02','SM_Cliff_Ridge_02','SM_Cliff_Wall_02']
    # Main forms at clusters; two small inner clusters receive boulders only.
    for j,(cx,cy,rad) in enumerate(clusters):
        if j<7:
            yaw=[-35,12,100,145,170,40,75][j];scale=[1.35,1.4,1.32,1.45,1.4,1.5,1.25][j]
            ob=spawn(meshes[names[j%3]],'E02_Cliff_%02d'%j,cx,cy,scale,yaw,z=20)
            placed.append(ob);large.append(ob)
        for k in range(4 if j<7 else 2):
            a=rng.random()*math.tau;d=rng.uniform(rad*.4,rad*.9);x=cx+math.cos(a)*d;y=cy+math.sin(a)*d
            if safe_xy(x,y,115):
                ob=spawn(meshes['SM_Boulder_02'],'E02_Boulder_%02d_%02d'%(j,k),x,y,rng.uniform(.48,.85),rng.uniform(0,360),z=28);placed.append(ob);large.append(ob)
        for k in range(9):
            a=rng.random()*math.tau;d=rng.uniform(rad*.45,rad*1.2);x=cx+math.cos(a)*d;y=cy+math.sin(a)*d
            if safe_xy(x,y,45):placed.append(spawn(meshes['SM_Scree_02'],'E02_Scree_%02d_%02d'%(j,k),x,y,rng.uniform(.28,.72),rng.uniform(0,360),z=47,folder='Scree',collision=False))
        for k in range(17):
            a=rng.random()*math.tau;d=rng.uniform(rad*.55,rad*1.3);x=cx+math.cos(a)*d;y=cy+math.sin(a)*d
            if not safe_xy(x,y,35):continue
            kind=['SM_Alien_Rosette_02','SM_Alien_Coral_02','SM_Alien_Trumpets_02'][rng.choices([0,1,2],[6,4,1])[0]]
            placed.append(spawn(meshes[kind],'E02_Plant_%02d_%02d'%(j,k),x,y,rng.uniform(.44,.9),rng.uniform(0,360),z=48,folder='Vegetation',collision=False))
    # Two slow opening flowers per selected cluster. Each petal remains editable.
    blooms=[]
    for j,(x,y) in enumerate([(-1100,-2100),(-2450,1650),(2380,1860),(1040,2510),(-1040,1220),(2150,-920)]):
        if not safe_xy(x,y,40):continue
        base=spawn(meshes['SM_Alien_Bloom_Base_02'],'E02_Bloom_%02d_Base'%j,x,y,.85,j*27,z=48,folder='Vegetation',collision=False);blooms.append(base)
        for k in range(6):
            p=spawn(meshes['SM_Alien_Bloom_Petal_02'],'E02_Bloom_%02d_Petal_%d'%(j,k),x,y,.85,k*60+j*27,z=48+48*.85,folder='Vegetation',collision=False)
            p.attach_to_actor(base,'None',u.AttachmentRule.KEEP_WORLD,u.AttachmentRule.KEEP_WORLD,u.AttachmentRule.KEEP_WORLD,False);placed.append(p)
        placed.append(base)
    # Register only substantial rocks as occupied cells using the existing grid API.
    reserved=0
    for ob in large:
        center,extent=ob.get_actor_bounds(False)
        # Tight core, excluding decorative rubble at the edges.
        footprint=u.IntPoint(max(1,math.ceil(extent.x*1.2/100)),max(1,math.ceil(extent.y*1.2/100)))
        pp=surface.get_placement_for_world_location(center,footprint)
        if surface.reserve_cells(ob,pp.origin_cell,footprint):reserved+=1
    report['actors']=len(placed);report['large_rocks']=len(large);report['grid_reserved_rocks']=reserved;report['blooms']=len(blooms)
    report['assets']={k:v.get_path_name() for k,v in meshes.items()}
    # Camera preview stored in this map for easy repeatable comparison.
    cam=ae.spawn_actor_from_class(u.CameraActor,u.Vector(-2950,-3250,3550),u.Rotator(pitch=-37,yaw=45,roll=0));cam.set_actor_label('E02_OverviewCamera');cam.tags=['STP_Expedition02'];cam.set_folder_path('Environment/Expedition02/Review');cam.camera_component.set_field_of_view(57)
    cam.camera_component.set_editor_property('post_process_blend_weight',0)
    fill=ae.spawn_actor_from_class(u.DirectionalLight,u.Vector(0,0,1000),u.Rotator(pitch=-55,yaw=140,roll=0))
    fill.set_actor_label('E02_SoftFill');fill.tags=['STP_Expedition02'];fill.set_folder_path('Environment/Expedition02/Lighting')
    fill.light_component.set_intensity(1.6);fill.light_component.set_cast_shadows(False);fill.light_component.set_light_color(u.LinearColor(.78,.85,1,1))
    u.EditorLevelLibrary.set_level_viewport_camera_info(cam.get_actor_location(),cam.get_actor_rotation())
    for ob in ae.get_all_level_actors():
        if isinstance(ob,u.DirectionalLight):
            # Existing actor values otherwise preserved; soften severe black shadow contrast.
            pass
    u.EditorAssetLibrary.save_directory(DST,False,True)
    u.EditorLoadingAndSavingUtils.save_map(u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world(),'/Game/PlanetLevel_RockPreview')
    checkpoint('Scene composed and saved')
    report['motion']={'wind':'Vertex R root mask, WindStrength and WindSpeed material parameters','bloom':'Six hinged petal meshes per flower. AutoBloom=1 loop, set AutoBloom=0 and BloomOpen 0..1 for manual opening.','weather':'Preview wind; not yet driven by gameplay weather.'}
    checkpoint('COMPLETE')
try:main()
except Exception:
    report['error']=traceback.format_exc();checkpoint('FAILED');u.log_error(report['error'])
