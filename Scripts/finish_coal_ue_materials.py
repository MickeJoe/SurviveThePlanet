import unreal,json
dest='/Game/Units/Buildings/Coal/Materials/'
edit=unreal.MaterialEditingLibrary
for key,color in [('Armor',(.55,.50,.40)),('Steel',(.23,.28,.31)),('Chassis',(.075,.09,.105))]:
    m=unreal.load_asset(dest+'M_Coal_'+key);edit.delete_all_material_expressions(m)
    noise=edit.create_material_expression(m,unreal.MaterialExpressionNoise,-600,0)
    noise.set_editor_property('scale',.035);noise.set_editor_property('levels',2)
    noise.set_editor_property('output_min',0);noise.set_editor_property('output_max',1)
    a=edit.create_material_expression(m,unreal.MaterialExpressionConstant3Vector,-400,100);a.constant=unreal.LinearColor(*(v*.82 for v in color),1)
    b=edit.create_material_expression(m,unreal.MaterialExpressionConstant3Vector,-400,200);b.constant=unreal.LinearColor(*color,1)
    mix=edit.create_material_expression(m,unreal.MaterialExpressionLinearInterpolate,-200,0)
    assert edit.connect_material_expressions(a,'',mix,'A')
    assert edit.connect_material_expressions(b,'',mix,'B')
    assert edit.connect_material_expressions(noise,'',mix,'Alpha')
    assert edit.connect_material_property(mix,'',unreal.MaterialProperty.MP_BASE_COLOR)
    for value,prop,y in [(.35 if key=='Armor' else .65,unreal.MaterialProperty.MP_METALLIC,350),(.6,unreal.MaterialProperty.MP_ROUGHNESS,450)]:
        c=edit.create_material_expression(m,unreal.MaterialExpressionConstant,-200,y);c.r=value;assert edit.connect_material_property(c,'',prop)
    edit.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m)
material=unreal.load_asset(dest+'M_CoalDust');edit.delete_all_material_expressions(material)
color=edit.create_material_expression(material,unreal.MaterialExpressionConstant3Vector,-400,0);color.constant=unreal.LinearColor(.27,.19,.105,1)
assert edit.connect_material_property(color,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
fresnel=edit.create_material_expression(material,unreal.MaterialExpressionFresnel,-600,200)
opacity=edit.create_material_expression(material,unreal.MaterialExpressionLinearInterpolate,-200,200)
a=edit.create_material_expression(material,unreal.MaterialExpressionConstant,-400,300);a.r=.15
b=edit.create_material_expression(material,unreal.MaterialExpressionConstant,-400,400);b.r=0
assert edit.connect_material_expressions(a,'',opacity,'A')
assert edit.connect_material_expressions(b,'',opacity,'B')
assert edit.connect_material_expressions(fresnel,'',opacity,'Alpha')
assert edit.connect_material_property(opacity,'',unreal.MaterialProperty.MP_OPACITY)
edit.recompile_material(material);unreal.EditorAssetLibrary.save_loaded_asset(material)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
if world:
    for mine in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.CoalMiningMachine):
        for c in mine.get_components_by_class(unreal.StaticMeshComponent):
            if c.get_name()=='CuttingDust':c.set_material(0,material)
unreal.log('COAL_MATERIALS_VALIDATED')
