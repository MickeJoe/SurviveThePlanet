"""Add the credits row to the existing UMG HUD in the canonical UE 5.8 editor."""
import json
from pathlib import Path
import unreal
from editor_toolset.toolsets.object import ObjectTools

root = Path(unreal.Paths.project_dir()).resolve()
assert root == Path(r'C:\UE5\SurviveThePlanet 5.8')
assert not unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world(), 'Stop PIE before editing the HUD'
umg = unreal.get_default_object(unreal.UMGToolSet)
bp = unreal.load_asset('/Game/UI/WBP_ResourceDisplay')
nodes = {str(n.widget_name): n for n in umg.call_method('GetWidgets', (bp,)).widgets if n.widget}

def props(obj, values):
    schema = json.loads(ObjectTools.list_properties(obj))
    exact = {key.lower().replace('_', ''): key for key in schema}
    resolved = {exact[key.lower().replace('_', '')]: value for key, value in values.items()}
    ObjectTools.get_properties(obj, list(resolved))
    assert ObjectTools.set_properties(obj, json.dumps(resolved)), (obj.get_name(), resolved)

def add(cls, name):
    if name in nodes:
        return nodes[name].widget
    info = umg.call_method('AddWidget', (bp, cls, name, nodes['EnergyCardContents'].widget, -1))
    assert info.widget, name
    ObjectTools.list_properties(info.widget)
    ObjectTools.list_properties(info.slot)
    return info.widget

def place(widget, x, y, width, height):
    props(widget.slot, {'layoutData': {
        'offsets': {'left': x, 'top': y, 'right': width, 'bottom': height},
        'anchors': {'minimum': {'x': 0, 'y': 0}, 'maximum': {'x': 0, 'y': 0}},
        'alignment': {'x': 0, 'y': 0}}, 'bAutoSize': False})

place(nodes['EnergyIcon'].widget, 8, 3, 40, 40)
place(nodes['EnergyAmountText'].widget, 62, 0, 100, 28)
place(nodes['EnergyRateText'].widget, 62, 26, 100, 18)

divider = add(unreal.Border, 'CreditsDivider')
props(divider, {'brushColor': {'r': .08, 'g': .13, 'b': .15, 'a': 1}, 'visibility': 'HitTestInvisible'})
place(divider, 8, 48, 150, 1)

icon_texture = unreal.load_asset('/Game/UI/Contracts/Icons/T_Credits')
if not icon_texture:
    task = unreal.AssetImportTask()
    task.filename = str(root / 'ContentSource/UI/T_Credits.png')
    task.destination_path = '/Game/UI/Contracts/Icons'
    task.destination_name = 'T_Credits'
    task.automated = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    icon_texture = unreal.load_asset('/Game/UI/Contracts/Icons/T_Credits')
assert icon_texture
icon_texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON)
icon_texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_UI)
icon_texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
assert unreal.EditorAssetLibrary.save_loaded_asset(icon_texture)
icon = add(unreal.Image, 'CreditsIcon')
props(icon, {'visibility': 'HitTestInvisible'})
icon.set_brush_from_texture(icon_texture, False)
icon.set_tool_tip_text('Credits')
place(icon, 12, 53, 28, 28)

amount = add(unreal.TextBlock, 'CreditsAmountText')
ObjectTools.list_properties(nodes['EnergyAmountText'].widget)
font = json.loads(ObjectTools.get_properties(nodes['EnergyAmountText'].widget, ['font']))['font']
font['size'] = 22
props(amount, {'font': font,
              'colorAndOpacity': {'specifiedColor': {'r': 1, 'g': .63, 'b': .015, 'a': 1}},
              'visibility': 'HitTestInvisible'})
amount.set_text('1,000')
amount.set_tool_tip_text('Credits')
place(amount, 62, 51, 100, 30)
umg.call_method('ToggleWidgetAsVariable', (bp, amount, True))
assert umg.call_method('CompileWidgetBlueprint', (bp,))
assert unreal.EditorAssetLibrary.save_loaded_asset(bp)

planet = unreal.load_asset('/Game/Data/Planet/DA_PlanetDefinition')
assert planet
ObjectTools.list_properties(planet)
assert unreal.EditorAssetLibrary.save_loaded_asset(planet, False)
report = {'passed': True, 'starting_credits': planet.get_editor_property('starting_credits'),
          'widget': bp.get_path_name(), 'currency_suffix': '', 'icon': icon_texture.get_path_name()}
(root / 'Saved/CreditsHudBuild.json').write_text(json.dumps(report, indent=2))
unreal.log('CREDITS_HUD_BUILD ' + json.dumps(report))
