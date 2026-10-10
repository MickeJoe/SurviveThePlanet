"""Register catalog meshes with the existing ownership, UMG and construction systems."""
import unreal, json, re
from pathlib import Path
from editor_toolset.toolsets.actor import ActorTools
from editor_toolset.toolsets.blueprint import BlueprintTools

def register_catalog():
    root=Path(unreal.Paths.project_dir()).resolve()
    assert root==Path(r'C:\UE5\SurviveThePlanet 5.8')
    source=root/'ContentSource/BlueprintCatalog'
    out=Path(r'C:\Users\qtxmj\Documents\Codex\2026-10-07\kan-du-skapa-alla-meshes-f-2\outputs')
    records=json.loads((source/'authored_manifest.json').read_text())
    lib=unreal.EditorAssetLibrary;tools=unreal.AssetToolsHelpers.get_asset_tools()
    dest='/Game/Units/Buildings/BlueprintCatalog'
    sub=unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    catalog=unreal.load_asset('/Game/Data/Buildings/DA_BuildingCatalog');assert catalog
    entries=list(catalog.get_editor_property('buildings'))
    cost_templates={
        'INDUSTRY':unreal.STPBuildTool.POLYMER_PLANT,
        'INFRASTRUCTURE':unreal.STPBuildTool.COMMUNICATION_MODULE,
        'LOGISTICS':unreal.STPBuildTool.CARGO_BAY,
        'ENERGY':unreal.STPBuildTool.GEOTHERMAL_PLANT,
    }
    report=[]
    def components(bp):
        return [unreal.SubobjectDataBlueprintFunctionLibrary.get_associated_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h))
            for h in sub.k2_gather_subobject_data_for_blueprint(bp)]
    for r in records[globals().get('BATCH_START',0):globals().get('BATCH_END',49)]:
        key=r['key'];folder=dest+'/'+key
        enum_name=re.sub(r'(?<!^)(?=[A-Z])','_',key).upper()
        tool=getattr(unreal.STPBuildTool,enum_name)
        category='INDUSTRY'
        if r['id'] in (7,62,63,64,65,69):category='LOGISTICS'
        elif r['id'] in (14,15,16,18,32,61,66,67):category='INFRASTRUCTURE'
        elif r['id']==56:category='ENERGY'
        # Existing category construction costs are provisional balance defaults.
        template=next((d for d in entries if d.get_editor_property('build_tool')==cost_templates[category]),None)
        assert template,(category,cost_templates[category])
        mesh=unreal.load_asset(folder+'/Meshes/SM_'+key+'_Body');assert mesh
        motion_mesh=unreal.load_asset(folder+'/Meshes/SM_'+key+'_Motion');assert motion_mesh
        icon=unreal.load_asset(folder+'/T_'+key)
        if not icon:
            task=unreal.AssetImportTask();task.filename=str(source/key/'preview.png')
            task.destination_path=folder;task.destination_name='T_'+key;task.automated=True;task.save=True
            tools.import_asset_tasks([task]);icon=unreal.load_asset(folder+'/T_'+key)
        assert icon
        icon.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        icon.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        icon.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
        data_path='/Game/Data/Buildings/BlueprintCatalog'
        data=unreal.load_asset(data_path+'/DA_'+key)
        if not data:
            factory=unreal.DataAssetFactory();factory.set_editor_property('data_asset_class',unreal.BuildingDataAsset)
            data=tools.create_asset('DA_'+key,data_path,unreal.BuildingDataAsset,factory)
        bp=unreal.load_asset(folder+'/BP_'+key+'Building')
        if not bp:
            factory=unreal.BlueprintFactory();factory.set_editor_property('parent_class',unreal.BaseBuilding)
            bp=tools.create_asset('BP_'+key+'Building',folder,unreal.Blueprint,factory)
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        for name,value in {
            'display_name':r['name'],'description':r['name']+'. Buildable catalog building.',
            'blueprint_id':key,'blueprint_initially_owned':False,'show_in_build_toolbar':True,
            'build_tool':tool,'build_category':getattr(unreal.STPBuildCategory,category),
            'building_type':unreal.STPBuildingType.OTHER,'building_tag':key,
            'building_mesh':mesh,'toolbar_icon':icon,'thumbnail':icon,'toolbar_sort_order':1000+r['id'],
            'construction_costs':list(template.get_editor_property('construction_costs')),
            'override_energy_settings':True,'energy_consumption_per_minute':0.0,
            'energy_production_per_minute':0.0,'energy_storage_capacity':0.0,
        }.items():data.set_editor_property(name,value)
        cdo=unreal.get_default_object(bp.generated_class())
        cdo.set_editor_property('building_data',data);cdo.set_editor_property('building_type',unreal.STPBuildingType.OTHER)
        cdo.set_editor_property('building_tag',key);cdo.set_editor_property('building_display_name',r['name'])
        cdo.set_editor_property('construction_progress',0.0)
        cdo.set_editor_property('ground_mesh_to_surface',True)
        body=next(o for o in components(bp) if isinstance(o,unreal.StaticMeshComponent) and o.get_name()=='BuildingMesh')
        body.set_static_mesh(mesh);body.set_editor_property('mobility',unreal.ComponentMobility.MOVABLE)
        motion=next((o for o in components(bp) if isinstance(o,unreal.StaticMeshComponent) and o.get_name().startswith('Motion')),None)
        if not motion:motion=ActorTools.add_component(bp,unreal.StaticMeshComponent.static_class(),'Motion')
        body=next(o for o in components(bp) if isinstance(o,unreal.StaticMeshComponent) and o.get_name()=='BuildingMesh')
        assert ActorTools.set_parent_component(motion,body)
        motion.set_static_mesh(motion_mesh);motion.set_editor_property('mobility',unreal.ComponentMobility.MOVABLE)
        motion.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        x,y,z=[100*v for v in r['pivot_m']];motion.set_editor_property('relative_location',unreal.Vector(x,-y,z))
        if r['id'] in (28,30,33,34,35,44,51,54,55,69):
            fx=next((o for o in components(bp) if isinstance(o,unreal.NiagaraComponent)),None)
            if not fx:fx=ActorTools.add_component(bp,unreal.NiagaraComponent.static_class(),'ProcessVapor')
            body=next(o for o in components(bp) if isinstance(o,unreal.StaticMeshComponent) and o.get_name()=='BuildingMesh')
            assert ActorTools.set_parent_component(fx,body)
            fx.set_asset(unreal.load_asset(dest+'/FX/NS_CatalogProcessVapor'))
            fx.set_editor_property('relative_location',unreal.Vector(-165,-80,300+25*(r['id']%3)))
            fx.set_editor_property('auto_activate',True)
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        graph=BlueprintTools.get_graph(bp,'EventGraph')
        BlueprintTools.write_graph_dsl(graph,(source/key/'Animation.dsl').read_text())
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        assert bp.get_editor_property('status')==unreal.BlueprintStatus.BS_UP_TO_DATE,str(bp.get_editor_property('status'))
        data.set_editor_property('building_class',bp.generated_class())
        # Keep existing entries and reject identity collisions instead of replacing unrelated buildings.
        other=next((d for d in entries if d.get_editor_property('build_tool')==tool and d!=data),None)
        assert not other,(key,other)
        if data not in entries:entries.append(data)
        for asset in (icon,data,bp):assert lib.save_loaded_asset(asset)
        report.append({'key':key,'tool':str(tool),'category':category,'definition':data.get_path_name(),
            'class':bp.generated_class().get_path_name(),'initially_owned':False,
            'footprint':str(unreal.get_default_object(bp.generated_class()).get_grid_footprint()),
            'cost_template':template.get_path_name()})
        (out/'ue_gameplay_registration.json').write_text(json.dumps(report,indent=2))
    catalog.set_editor_property('buildings',entries);assert lib.save_loaded_asset(catalog)
    unreal.log('CATALOG_GAMEPLAY_REGISTERED '+str(len(report)))

register_catalog()
