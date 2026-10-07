"""Import the Blender-authored visual asset through the live UE 5.8 editor."""
import unreal, json
from pathlib import Path

ROOT = Path(unreal.Paths.project_dir()).resolve()
assert ROOT == Path(r'C:\UE5\SurviveThePlanet 5.8'), ROOT
SRC = ROOT / 'ContentSource' / 'RemoteBase'
DEST = '/Game/Units/Buildings/RemoteBase'
assets = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
meshes = {}
for part in ['Body', 'Antenna', 'Ring', 'Scan']:
    name = 'SM_RemoteBase_' + part
    path = DEST + '/Meshes/' + name
    mesh = unreal.load_asset(path) if lib.does_asset_exist(path) else None
    if mesh is None:
        task = unreal.AssetImportTask()
        task.filename = str(SRC / (name + '.fbx'))
        task.destination_path = DEST + '/Meshes'
        task.destination_name = name
        task.automated = True
        task.save = True
        options = unreal.FbxImportUI()
        options.import_mesh = True
        options.import_materials = False
        options.import_textures = False
        options.import_as_skeletal = False
        options.static_mesh_import_data.combine_meshes = True
        options.static_mesh_import_data.auto_generate_collision = part == 'Body'
        options.static_mesh_import_data.generate_lightmap_u_vs = True
        task.options = options
        assets.import_asset_tasks([task])
        mesh = unreal.load_asset(path)
    assert mesh, name
    meshes[part] = mesh

palette = {'Ivory': ((.73,.71,.65),.18,.5), 'Orange': ((.9,.19,.025),.18,.45), 'Graphite': ((.065,.085,.105),.6,.42), 'Steel': ((.32,.39,.42),.75,.32)}
materials = {}

def node(mat, cls, x, y):
    return mel.create_material_expression(mat, cls, x, y)

def constant(mat, value, x=0, y=0):
    n = node(mat, unreal.MaterialExpressionConstant, x, y)
    n.r = value
    return n

def color(mat, value, x=0, y=0):
    n = node(mat, unreal.MaterialExpressionConstant3Vector, x, y)
    n.constant = unreal.LinearColor(*value, 1)
    return n

def link(a, out, b, inp):
    assert mel.connect_material_expressions(a, out, b, inp), (a,b,inp)

def prop(n, name):
    assert mel.connect_material_property(n, '', name)

def new_mat(name):
    path = DEST + '/Materials/' + name
    m = unreal.load_asset(path) if lib.does_asset_exist(path) else assets.create_asset(name, DEST+'/Materials', unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(m)
    return m

for key,(rgb,metal,rough) in palette.items():
    m = new_mat('M_RemoteBase_'+key)
    prop(color(m,rgb,-300,0), unreal.MaterialProperty.MP_BASE_COLOR)
    prop(constant(m,metal,-300,160), unreal.MaterialProperty.MP_METALLIC)
    prop(constant(m,rough,-300,240), unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(m)
    lib.save_loaded_asset(m)
    materials['Remote_'+key]=m

# Cyan beacon pulse entirely on the GPU, without Blueprint Tick.
m = new_mat('M_RemoteBase_CyanPulse')
rgb = color(m,(.008,.65,.95),-800,0)
prop(rgb,unreal.MaterialProperty.MP_BASE_COLOR)
t = node(m,unreal.MaterialExpressionTime,-800,160)
sine = node(m,unreal.MaterialExpressionSine,-600,160); sine.set_editor_property('period',3.0)
link(t,'',sine,'')
scale = node(m,unreal.MaterialExpressionMultiply,-400,160);scale.set_editor_property('const_b',.7);link(sine,'',scale,'A')
bias = node(m,unreal.MaterialExpressionAdd,-200,160);bias.set_editor_property('const_b',2.3);link(scale,'',bias,'A')
emit = node(m,unreal.MaterialExpressionMultiply,0,0);link(rgb,'',emit,'A');link(bias,'',emit,'B')
prop(emit,unreal.MaterialProperty.MP_EMISSIVE_COLOR)
prop(constant(m,.35,0,260),unreal.MaterialProperty.MP_ROUGHNESS)
mel.recompile_material(m);lib.save_loaded_asset(m);materials['Remote_Cyan']=m

# Thin two-sided segmented perimeter glow.
ring = new_mat('M_RemoteBase_Perimeter')
ring.set_editor_property('two_sided',True)
ring.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
prop(color(ring,(.018,2.0,3.0),-300,0),unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(ring);lib.save_loaded_asset(ring)

# A four-second expanding, fading annulus. World-space displacement is relative
# to the component center, so every placed Blueprint carries its own effect.
scan = new_mat('M_RemoteBase_ScanPulse')
scan.set_editor_property('two_sided',True)
scan.set_editor_property('blend_mode',unreal.BlendMode.BLEND_ADDITIVE)
scan.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
prop(color(scan,(.008,1.8,2.8),-400,-200),unreal.MaterialProperty.MP_EMISSIVE_COLOR)
t=node(scan,unreal.MaterialExpressionTime,-1000,100)
speed=node(scan,unreal.MaterialExpressionMultiply,-800,100);speed.set_editor_property('const_b',.25);link(t,'',speed,'A')
phase=node(scan,unreal.MaterialExpressionFrac,-600,100);link(speed,'',phase,'')
fade=node(scan,unreal.MaterialExpressionOneMinus,-400,100);link(phase,'',fade,'')
prop(fade,unreal.MaterialProperty.MP_OPACITY)
world=node(scan,unreal.MaterialExpressionWorldPosition,-1000,400)
origin=node(scan,unreal.MaterialExpressionObjectPositionWS,-1000,560)
relative=node(scan,unreal.MaterialExpressionSubtract,-800,400);link(world,'',relative,'A');link(origin,'',relative,'B')
growth=node(scan,unreal.MaterialExpressionMultiply,-400,320);growth.set_editor_property('const_b',.65);link(phase,'',growth,'A')
offset=node(scan,unreal.MaterialExpressionMultiply,-180,400);link(relative,'',offset,'A');link(growth,'',offset,'B')
prop(offset,unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
mel.recompile_material(scan);lib.save_loaded_asset(scan)

for part,mesh in meshes.items():
    for i,slot in enumerate(mesh.get_editor_property('static_materials')):
        key=str(slot.material_slot_name)
        assert key in materials, (part,key)
        mesh.set_material(i,ring if part=='Ring' else scan if part=='Scan' else materials[key])
    if part=='Scan':
        mesh.set_editor_property('positive_bounds_extension',unreal.Vector(180,180,5))
        mesh.set_editor_property('negative_bounds_extension',unreal.Vector(180,180,5))
    lib.save_loaded_asset(mesh)

bp_path=DEST+'/BP_RemoteBase'
bp=unreal.load_asset(bp_path) if lib.does_asset_exist(bp_path) else None
if bp is None:
    factory=unreal.BlueprintFactory();factory.set_editor_property('parent_class',unreal.Actor)
    bp=assets.create_asset('BP_RemoteBase',DEST,unreal.Blueprint,factory)
    subsystem=unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles=subsystem.k2_gather_subobject_data_for_blueprint(bp)
    parent=handles[0]
    def component(cls,name,parent_handle):
        params=unreal.AddNewSubobjectParams(parent_handle=parent_handle,new_class=cls,blueprint_context=bp)
        handle,reason=subsystem.add_new_subobject(params)
        assert not str(reason), str(reason)
        assert subsystem.rename_subobject(handle,name)
        data=unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        obj=unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
        assert obj,name
        return handle,obj
    root_handle,root_component=component(unreal.SceneComponent,'RemoteBaseRoot',parent)
    for part,mesh in meshes.items():
        handle,obj=component(unreal.StaticMeshComponent,part,root_handle)
        obj.set_static_mesh(mesh)
        obj.set_editor_property('mobility',unreal.ComponentMobility.MOVABLE)
        if part=='Antenna':obj.set_editor_property('relative_location',unreal.Vector(0,0,218))
        if part!='Body':obj.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        if part in ['Ring','Scan']:
            obj.set_editor_property('cast_shadow',False)
            obj.set_editor_property('receives_decals',False)
    handle,movement=component(unreal.RotatingMovementComponent,'AntennaRotation',parent)
    movement.set_editor_property('rotation_rate',unreal.Rotator(pitch=0,yaw=12,roll=0))
    movement.set_editor_property('rotation_in_local_space',True)
    movement.set_editor_property('auto_register_updated_component',False)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
lib.save_loaded_asset(bp)
report={'imported':True,'blueprint':bp.get_path_name(),'mesh_bounds':{k:str(v.get_bounding_box()) for k,v in meshes.items()},'materials':[m.get_path_name() for m in materials.values()]+[ring.get_path_name(),scan.get_path_name()],'animation_pending':'Connect AntennaRotation to Antenna in BeginPlay'}
(ROOT/'Saved'/'RemoteBase_import.json').write_text(json.dumps(report,indent=2))
unreal.log('REMOTE_BASE_IMPORTED '+json.dumps(report))
