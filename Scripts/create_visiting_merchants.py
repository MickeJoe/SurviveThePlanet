"""Author the compact Cargo Bay WBP, four merchants and planet visit schedule."""
import json
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir()).resolve()
assert root == Path(r'C:\UE5\SurviveThePlanet 5.8')
helpers = (root / 'Scripts/create_trade_ui.py').read_text(encoding='utf-8')
exec(helpers[:helpers.index('credits=unreal.load_asset')])

task = unreal.AssetImportTask()
task.filename = str(root / 'ContentSource/Trading/T_VisitingMerchantPortraits.png')
task.destination_path = '/Game/UI/Trading/Art'
task.destination_name = 'T_VisitingMerchantPortraits'
task.automated = True
task.save = True
if not unreal.EditorAssetLibrary.does_asset_exist('/Game/UI/Trading/Art/T_VisitingMerchantPortraits'):
    assets.import_asset_tasks([task])
portraits = unreal.load_asset('/Game/UI/Trading/Art/T_VisitingMerchantPortraits')
assert portraits
portraits.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON)
portraits.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_UI)
portraits.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
unreal.EditorAssetLibrary.save_loaded_asset(portraits)

catalog = unreal.load_asset('/Game/Data/Trading/DA_TradeCatalog')
assert catalog
items = list(catalog.get_editor_property('items'))
raw_ids = [str(i.id) for i in items if i.kind == unreal.TradeItemKind.RESOURCE and i.buy_price == 8]
material_ids = [str(i.id) for i in items if i.kind == unreal.TradeItemKind.RESOURCE and i.buy_price == 16]
component_ids = [str(i.id) for i in items if i.kind == unreal.TradeItemKind.RESOURCE and i.buy_price == 38]
blueprint_ids = [str(i.id) for i in items if i.kind == unreal.TradeItemKind.BLUEPRINT]
drone_ids = [str(i.id) for i in items if i.kind == unreal.TradeItemKind.DRONE]
chip_ids = [str(i.id) for i in items if i.resource == unreal.ResourceType.CONTROL_CHIP and i.kind == unreal.TradeItemKind.RESOURCE]
definitions = [
    ('OrionExchange', 'orion_exchange', 'Orion Exchange', 'General goods, building blueprints and colony equipment.', raw_ids + material_ids + blueprint_ids + chip_ids + drone_ids),
    ('FrontierSupplies', 'frontier_supplies', 'Frontier Supplies', 'Raw resources and construction materials from the frontier.', raw_ids + material_ids),
    ('NexusRobotics', 'nexus_robotics', 'Nexus Robotics', 'Control chips, components, blueprints and working drones.', component_ids + chip_ids + blueprint_ids + drone_ids),
    ('AtlasFoundry', 'atlas_foundry', 'Atlas Foundry', 'Industrial materials, components and construction technology.', material_ids + component_ids + blueprint_ids),
]
merchants = []
for index, (asset_name, merchant_id, name, description, offered) in enumerate(definitions):
    name_in_content = 'DA_' + asset_name
    asset_path = '/Game/Data/Trading/Merchants/' + name_in_content
    merchant = unreal.load_asset(asset_path)
    if not merchant:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property('data_asset_class', unreal.MerchantDefinition)
        merchant = assets.create_asset(name_in_content, '/Game/Data/Trading/Merchants', unreal.MerchantDefinition, factory)
    merchant.set_editor_property('id', merchant_id)
    merchant.set_editor_property('display_name', name)
    merchant.set_editor_property('description', description)
    merchant.set_editor_property('portrait', portraits)
    merchant.set_editor_property('portrait_uv_min', unreal.Vector2D((index % 2) * .5, (index // 2) * .5))
    merchant.set_editor_property('portrait_uv_max', unreal.Vector2D((index % 2 + 1) * .5, (index // 2 + 1) * .5))
    merchant.set_editor_property('offered_item_ids', list(dict.fromkeys(offered)))
    unreal.EditorAssetLibrary.save_loaded_asset(merchant)
    merchants.append(merchant)

planet = unreal.load_asset('/Game/Data/Planet/DA_PlanetDefinition')
assert planet
schedule = unreal.MerchantVisitSchedule()
schedule.set_editor_property('repeat_cycle_days', 8.0)
visits = []
for index, merchant in enumerate(merchants):
    visit = unreal.MerchantScheduledVisit()
    visit.set_editor_property('merchant', merchant)
    visit.set_editor_property('arrival_after_hours', 24.0 + index * 48.0)
    visit.set_editor_property('stay_hours', 8.0)
    visits.append(visit)
schedule.set_editor_property('visits', visits)
planet.set_editor_property('merchant_visits', schedule)
unreal.EditorAssetLibrary.save_loaded_asset(planet)

bp = new_bp('WBP_VisitingMerchant', unreal.VisitingMerchantWidget)
canvas = add(bp, unreal.CanvasPanel, 'MerchantRoot')
props(canvas, {'visibility': 'SelfHitTestInvisible'})
panel = add(bp, unreal.Border, 'MerchantPanel', canvas, True)
props(panel, {'brushColor': DARK, 'padding': {'left': 0, 'top': 0, 'right': 0, 'bottom': 0}, 'visibility': 'SelfHitTestInvisible'})
props(panel.slot, {'layoutData': {'offsets': {'left': -357, 'top': 378, 'right': 340, 'bottom': 158}, 'anchors': {'minimum': {'x': 1, 'y': 0}, 'maximum': {'x': 1, 'y': 0}}, 'alignment': {'x': 0, 'y': 0}}, 'bAutoSize': False})
contents = add(bp, unreal.CanvasPanel, 'MerchantContents', panel)
props(contents, {'visibility': 'SelfHitTestInvisible'})
border(bp, 'TopAccent', contents, 0, 0, 340, 2, GOLD)
text(bp, 'CargoBayHeading', contents, 'CARGO BAY', 46, 11, 264, 27, 17)
cargo_icon = unreal.load_asset('/Game/UI/Images/CargoBayBuildIcon')
image(bp, 'CargoBayIcon', contents, 12, 8, 27, 27, cargo_icon)
border(bp, 'HeaderLine', contents, 10, 44, 320, 1, MUTED)
image(bp, 'MerchantPortrait', contents, 12, 54, 72, 72, portraits, True)
text(bp, 'MerchantNameText', contents, 'Orion Exchange', 96, 56, 230, 27, 16, WHITE, True)
text(bp, 'ArrivalText', contents, '24:00', 96, 86, 222, 36, 25, GOLD, True)
button(bp, 'TradeButton', contents, 'TRADE', 96, 88, 222, 34, TEAL)
progress = add(bp, unreal.ProgressBar, 'ArrivalProgress', contents, True)
place(progress, 12, 140, 316, 6)
props(progress, {'fillColorAndOpacity': TEAL, 'percent': .0, 'visibility': 'HitTestInvisible'})
assert umg.call_method('CompileWidgetBlueprint', (bp,))
assert unreal.EditorAssetLibrary.save_loaded_asset(bp)

contracts = unreal.load_asset('/Game/UI/Trading/WBP_TraderPanel')
if contracts:
    for node in umg.call_method('GetWidgets', (contracts,)).widgets:
        if isinstance(node.widget, unreal.TextBlock) and str(node.widget.get_text()) == 'TRADERS':
            node.widget.set_text('CONTRACTS')
    assert umg.call_method('CompileWidgetBlueprint', (contracts,))
    unreal.EditorAssetLibrary.save_loaded_asset(contracts)
unreal.log('VISITING_MERCHANTS_AUTHORED: 4 merchants, 24h first delay, 48h intervals, 8h stays, 8-day cycle')

import runpy
runpy.run_path(str(root / 'Scripts/match_merchant_panel_frame.py'))
