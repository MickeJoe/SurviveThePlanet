import unreal,json
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
assert root==Path(r"C:\UE5\SurviveThePlanet 5.8")
src=root/"ContentSource/ConnectorFactory";dest="/Game/Units/Buildings/ConnectorFactory"
tools=unreal.AssetToolsHelpers.get_asset_tools();lib=unreal.EditorAssetLibrary
def imported(name,folder,texture=False):
 task=unreal.AssetImportTask();task.filename=str(src/(name+(".png" if texture else ".fbx")));task.destination_path=folder;task.destination_name=name;task.automated=True;task.save=True;task.replace_existing=True
 if not texture:
  opts=unreal.FbxImportUI();opts.import_mesh=True;opts.import_materials=False;opts.import_textures=False;opts.import_as_skeletal=False
  opts.static_mesh_import_data.combine_meshes=True;opts.static_mesh_import_data.auto_generate_collision=True;opts.static_mesh_import_data.generate_lightmap_u_vs=True;task.options=opts
 tools.import_asset_tasks([task]);asset=unreal.load_asset(folder+"/"+name);assert asset,name;return asset
mesh=imported("SM_ConnectorFactory",dest+"/Meshes")
color=imported("T_ConnectorFactory_BaseColor",dest+"/Textures",True)
ao=imported("T_ConnectorFactory_AO",dest+"/Textures",True);ao.set_editor_property("srgb",False);lib.save_loaded_asset(ao)
icon=imported("T_ConnectorFactory","/Game/UI/Icons/Buildings",True)
icon.set_editor_property("compression_settings",unreal.TextureCompressionSettings.TC_EDITOR_ICON);icon.set_editor_property("lod_group",unreal.TextureGroup.TEXTUREGROUP_UI);lib.save_loaded_asset(icon)
material=unreal.load_asset(dest+"/Materials/M_ConnectorSurface")
if not material:
 material=tools.create_asset("M_ConnectorSurface",dest+"/Materials",unreal.Material,unreal.MaterialFactoryNew())
 n=unreal.MaterialEditingLibrary.create_material_expression(material,unreal.MaterialExpressionTextureSample,-400,0);n.texture=color;unreal.MaterialEditingLibrary.connect_material_property(n,"RGB",unreal.MaterialProperty.MP_BASE_COLOR)
 n=unreal.MaterialEditingLibrary.create_material_expression(material,unreal.MaterialExpressionTextureSample,-400,200);n.texture=ao;n.set_editor_property("sampler_type",unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR);unreal.MaterialEditingLibrary.connect_material_property(n,"R",unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
 for v,p,y in [(.55,unreal.MaterialProperty.MP_ROUGHNESS,400),(.12,unreal.MaterialProperty.MP_METALLIC,500)]:
  n=unreal.MaterialEditingLibrary.create_material_expression(material,unreal.MaterialExpressionConstant,-400,y);n.r=v;unreal.MaterialEditingLibrary.connect_material_property(n,"",p)
 unreal.MaterialEditingLibrary.recompile_material(material);lib.save_loaded_asset(material)
cyan=unreal.load_asset("/Game/Units/Buildings/PolymerPlant/Materials/M_Polymer_Cyan");assert cyan
for i,slot in enumerate(mesh.get_editor_property("static_materials")):mesh.set_material(i,cyan if "Connector_Cyan" in str(slot.material_slot_name) else material)
lib.save_loaded_asset(mesh)
data=unreal.load_asset("/Game/Data/Buildings/DA_ConnectorFactory") or tools.create_asset("DA_ConnectorFactory","/Game/Data/Buildings",unreal.BuildingDataAsset,unreal.DataAssetFactory())
for key,value in {"building_mesh":mesh,"thumbnail":icon,"toolbar_icon":icon,"display_name":"Connector Factory","description":"Compact connector assembly workshop. Visual building; production recipe is not configured.","building_tag":"ConnectorFactory","blueprint_id":"ConnectorFactory","building_type":unreal.STPBuildingType.OTHER,"build_category":unreal.STPBuildCategory.INDUSTRY,"show_in_build_toolbar":False,"override_energy_settings":True,"energy_consumption_per_minute":0.0}.items():data.set_editor_property(key,value)
factory=unreal.BlueprintFactory();factory.set_editor_property("parent_class",unreal.BaseBuilding)
bp=unreal.load_asset("/Game/BluePrints/Buildings/ConnectorFactory/BP_ConnectorFactory") or tools.create_asset("BP_ConnectorFactory","/Game/BluePrints/Buildings/ConnectorFactory",unreal.Blueprint,factory)
cdo=unreal.get_default_object(bp.generated_class());cdo.set_editor_property("building_data",data);cdo.set_editor_property("construction_progress",1.0)
for c in cdo.get_components_by_class(unreal.StaticMeshComponent):
 if c.get_name()=="BuildingMesh":
  c.set_static_mesh(mesh);c.set_editor_property("relative_location",unreal.Vector(0,0,-130.5));c.set_editor_property("relative_rotation",unreal.Rotator(pitch=0,yaw=90,roll=0))
unreal.BlueprintEditorLibrary.compile_blueprint(bp);data.set_editor_property("building_class",bp.generated_class());lib.save_loaded_asset(data);lib.save_loaded_asset(bp)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
surface=next(a for a in actors.get_all_level_actors() if isinstance(a,unreal.PlanetSurfaceManager))
camp=next(a for a in actors.get_all_level_actors() if a.get_actor_label()=="BP_BaseModuleSpawnPoint")
footprint=cdo.get_grid_footprint();placement=None
for distance in [850,1100,1400]:
 for direction in [unreal.Vector(0,-1,0),unreal.Vector(-1,0,0),unreal.Vector(1,0,0),unreal.Vector(0,1,0)]:
  candidate=surface.get_building_placement_for_world_location(camp.get_actor_location()+direction*distance,footprint)
  if candidate.valid:placement=candidate;break
 if placement:break
assert placement,"No unobstructed starting site"
sp=next((a for a in actors.get_all_level_actors() if a.get_actor_label()=="Connector Factory - Starting Site"),None)
if not sp:sp=actors.spawn_actor_from_class(unreal.BaseModuleSpawnPoint,placement.world_location,placement.world_rotation)
sp.set_actor_label("Connector Factory - Starting Site")
for key,value in {"base_module_class":bp.generated_class(),"base_module_preview_mesh":mesh,"energy_coverage_radius":0.0,"spawned_actor_tag":"ConnectorFactory","spawn_on_begin_play":True,"spawn_options":[]}.items():sp.set_editor_property(key,value)
sp.set_actor_location_and_rotation(placement.world_location,placement.world_rotation,False,True)
polymer=unreal.load_asset("/Game/Units/Buildings/PolymerPlant/Meshes/SM_PolymerPlant");assert polymer
s=mesh.get_bounding_box().max-mesh.get_bounding_box().min;p=polymer.get_bounding_box().max-polymer.get_bounding_box().min
report={"passed":True,"blueprint":bp.get_path_name(),"mesh_size_cm":[s.x,s.y,s.z],"polymer_size_cm":[p.x,p.y,p.z],"spawn_point":sp.get_path_name(),"location":[placement.world_location.x,placement.world_location.y,placement.world_location.z],"map":"/Game/WorldGeneration/Maps/L_PlanetClusters","production_configured":False}
(root/"Saved/ConnectorImport.json").write_text(json.dumps(report,indent=2))
unreal.log("CONNECTOR_IMPORT_COMPLETE "+json.dumps(report))
