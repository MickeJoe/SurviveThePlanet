"""Author the trading templates and catalog in the canonical UE5.8 editor."""
import json
from pathlib import Path
import unreal
from editor_toolset.toolsets.object import ObjectTools

root=Path(unreal.Paths.project_dir()).resolve()
assert root==Path(r'C:\UE5\SurviveThePlanet 5.8')
assert not unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
umg=unreal.get_default_object(unreal.UMGToolSet)
assets=unreal.AssetToolsHelpers.get_asset_tools()
TEAL={'r':.04,'g':.60,'b':.70,'a':1}
GOLD={'r':1,'g':.64,'b':.08,'a':1}
WHITE={'r':.88,'g':.93,'b':.94,'a':1}
MUTED={'r':.43,'g':.57,'b':.61,'a':1}
DARK={'r':.014,'g':.029,'b':.039,'a':1}
PANEL={'r':.024,'g':.049,'b':.061,'a':1}

def props(obj,values):
    schema=json.loads(ObjectTools.list_properties(obj))
    keys={key.lower().replace('_',''):key for key in schema}
    resolved={keys[key.lower().replace('_','')]:value for key,value in values.items()}
    ObjectTools.get_properties(obj,list(resolved))
    assert ObjectTools.set_properties(obj,json.dumps(resolved)),(obj.get_name(),resolved)

def new_bp(name,parent):
    path='/Game/UI/Trading/'+name
    bp=unreal.find_object(None,path+'.'+name)
    if not bp and unreal.EditorAssetLibrary.does_asset_exist(path): bp=unreal.load_asset(path)
    if not bp:
        factory=unreal.WidgetBlueprintFactory()
        factory.set_editor_property('parent_class',parent)
        bp=assets.create_asset(name,'/Game/UI/Trading',unreal.WidgetBlueprint,factory)
    assert bp,name
    for node in reversed(umg.call_method('GetWidgets',(bp,)).widgets):
        if node.widget: umg.call_method('RemoveWidget',(bp,node.widget))
    return bp

def add(bp,cls,name,parent=None,variable=False):
    info=umg.call_method('AddWidget',(bp,cls,name,parent,-1))
    assert info.widget,name
    ObjectTools.list_properties(info.widget)
    if info.slot: ObjectTools.list_properties(info.slot)
    if variable: umg.call_method('ToggleWidgetAsVariable',(bp,info.widget,True))
    return info.widget

def place(widget,x,y,w,h):
    props(widget.slot,{'layoutData':{'offsets':{'left':x,'top':y,'right':w,'bottom':h},'anchors':{'minimum':{'x':0,'y':0},'maximum':{'x':0,'y':0}},'alignment':{'x':0,'y':0}},'bAutoSize':False})

def text(bp,name,parent,value,x,y,w,h,size=18,color=WHITE,variable=False):
    widget=add(bp,unreal.TextBlock,name,parent,variable)
    font=json.loads(ObjectTools.get_properties(widget,['font']))['font']
    font['size']=size
    props(widget,{'font':font,'colorAndOpacity':{'specifiedColor':color},'visibility':'HitTestInvisible'})
    widget.set_text(value)
    if isinstance(widget.slot,unreal.CanvasPanelSlot): place(widget,x,y,w,h)
    return widget

def border(bp,name,parent,x,y,w,h,color=PANEL):
    widget=add(bp,unreal.Border,name,parent)
    props(widget,{'brushColor':color,'padding':{'left':0,'top':0,'right':0,'bottom':0}})
    place(widget,x,y,w,h)
    return widget

def button(bp,name,parent,label,x,y,w,h,color=TEAL,variable=True):
    widget=add(bp,unreal.Button,name,parent,variable)
    props(widget,{'backgroundColor':color,'isFocusable':False})
    place(widget,x,y,w,h)
    label_widget=text(bp,name+'Label',widget,label,0,0,0,0,12 if h<=30 else 17,WHITE)
    props(label_widget.slot,{'horizontalAlignment':'HAlign_Center','verticalAlignment':'VAlign_Center','padding':{'left':0,'top':0,'right':0,'bottom':0}})
    return widget

def image(bp,name,parent,x,y,w,h,texture=None,variable=False):
    widget=add(bp,unreal.Image,name,parent,variable)
    place(widget,x,y,w,h)
    props(widget,{'visibility':'HitTestInvisible'})
    if texture: widget.set_brush_from_texture(texture,False)
    return widget

credits=unreal.load_asset('/Game/UI/Contracts/Icons/T_Credits')
task=unreal.AssetImportTask()
task.filename=str(root/'ContentSource/Trading/T_HelixTradePortrait.png')
task.destination_path='/Game/UI/Trading/Art'; task.destination_name='T_HelixTradePortrait'
task.automated=True; task.save=True
if not unreal.load_asset('/Game/UI/Trading/Art/T_HelixTradePortrait'): assets.import_asset_tasks([task])
portrait=unreal.load_asset('/Game/UI/Trading/Art/T_HelixTradePortrait')
assert portrait
portrait.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
portrait.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
portrait.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
unreal.EditorAssetLibrary.save_loaded_asset(portrait)

# Compact item card. Names are exclusively in the C++ tooltip.
card=new_bp('WBP_TradeItemCard',unreal.TradeItemWidget)
sizebox=add(card,unreal.SizeBox,'CardSize')
props(sizebox,{'widthOverride':102,'heightOverride':104,'bOverride_WidthOverride':True,'bOverride_HeightOverride':True})
card_border=add(card,unreal.Border,'SelectionBorder',sizebox,True)
props(card_border,{'brushColor':PANEL,'padding':{'left':0,'top':0,'right':0,'bottom':0}})
canvas=add(card,unreal.CanvasPanel,'CardCanvas',card_border)
image(card,'ItemIcon',canvas,23,3,54,54,variable=True)
text(card,'StockText',canvas,'250',60,3,40,18,12,MUTED,True)
image(card,'CoinIcon',canvas,24,59,14,14,credits)
text(card,'PriceText',canvas,'10',43,57,55,20,15,GOLD,True)
button(card,'MinusButton',canvas,'−',5,80,25,22,PANEL)
quantity=text(card,'QuantityText',canvas,'0',33,80,33,22,15,WHITE,True)
props(quantity,{'justification':'Center'})
button(card,'PlusButton',canvas,'+',72,80,25,22,TEAL)
assert umg.call_method('CompileWidgetBlueprint',(card,))
unreal.EditorAssetLibrary.save_loaded_asset(card)

row=new_bp('WBP_TradeBasketRow',unreal.TradeItemWidget)
row_size=add(row,unreal.SizeBox,'RowSize')
props(row_size,{'heightOverride':44,'bOverride_HeightOverride':True})
row_border=add(row,unreal.Border,'SelectionBorder',row_size,True)
props(row_border,{'brushColor':PANEL,'padding':{'left':0,'top':0,'right':0,'bottom':0}})
canvas=add(row,unreal.CanvasPanel,'RowCanvas',row_border)
image(row,'ItemIcon',canvas,4,4,36,36,variable=True)
text(row,'ItemNameText',canvas,'Resource',48,10,272,26,16,WHITE,True)
stock=text(row,'StockText',canvas,'0',0,0,0,0,12,MUTED,True)
props(stock,{'visibility':'Collapsed'})
image(row,'CoinIcon',canvas,325,14,16,16,credits)
text(row,'PriceText',canvas,'10',348,10,64,25,16,GOLD,True)
button(row,'MinusButton',canvas,'−',430,8,28,28,PANEL)
text(row,'QuantityText',canvas,'0',466,10,58,28,16,WHITE,True)
button(row,'PlusButton',canvas,'+',526,8,28,28,TEAL)
button(row,'RemoveButton',canvas,'×',576,8,28,28,PANEL)
assert umg.call_method('CompileWidgetBlueprint',(row,))
unreal.EditorAssetLibrary.save_loaded_asset(row)

screen=new_bp('WBP_TradeScreen',unreal.TradeScreenWidget)
root_canvas=add(screen,unreal.CanvasPanel,'TradeRoot')
shade=border(screen,'Backdrop',root_canvas,0,0,1,1,{'r':0,'g':0,'b':0,'a':.78})
props(shade.slot,{'layoutData':{'offsets':{'left':0,'top':0,'right':0,'bottom':0},'anchors':{'minimum':{'x':0,'y':0},'maximum':{'x':1,'y':1}},'alignment':{'x':0,'y':0}}})
scale=add(screen,unreal.ScaleBox,'ResponsiveTradeScale',root_canvas)
props(scale,{'stretch':'ScaleToFit','stretchDirection':'DownOnly'})
props(scale.slot,{'layoutData':{'offsets':{'left':18,'top':18,'right':18,'bottom':18},'anchors':{'minimum':{'x':0,'y':0},'maximum':{'x':1,'y':1}},'alignment':{'x':0,'y':0}}})
window_size=add(screen,unreal.SizeBox,'TradeWindowSize',scale)
props(window_size,{'widthOverride':1380,'heightOverride':1000,'bOverride_WidthOverride':True,'bOverride_HeightOverride':True})
window=add(screen,unreal.Border,'TradeWindow',window_size)
props(window,{'brushColor':DARK,'padding':{'left':0,'top':0,'right':0,'bottom':0}})
c=add(screen,unreal.CanvasPanel,'TradeContents',window)
border(screen,'TopAccent',c,0,0,1380,3,TEAL)
image(screen,'TraderPortrait',c,24,20,100,100,portrait,True)
text(screen,'Eyebrow',c,'SUPPLY TRADER',148,20,450,24,14,TEAL)
text(screen,'TraderNameText',c,'HELIX INDUSTRIAL',148,44,720,43,30,WHITE,True)
desc=text(screen,'DescriptionText',c,'Supplies for a growing colony.',148,89,850,34,15,MUTED,True)
props(desc,{'autoWrapText':True})
text(screen,'BalanceLabel',c,'YOUR CREDITS',1080,29,240,24,14,MUTED)
image(screen,'CreditsIcon',c,1080,58,28,28,credits)
text(screen,'CreditsText',c,'1,000',1120,54,206,42,27,GOLD,True)
button(screen,'CloseButton',c,'×',1332,18,30,30,PANEL)
border(screen,'HeaderDivider',c,24,130,1332,1,TEAL)
text(screen,'TraderStockHeading',c,'TRADER STOCK',26,144,430,30,22,WHITE)
text(screen,'InventoryHeading',c,'YOUR INVENTORY',710,144,430,30,22,WHITE)
text(screen,'StockHint',c,'Hover for item details',398,151,266,24,13,MUTED)
text(screen,'InventoryHint',c,'Scroll for more items',1095,151,266,24,13,MUTED)
for name,x in [('TraderGrid',24),('PlayerGrid',708)]:
    panel=border(screen,name+'Panel',c,x,182,648,448,PANEL)
    scroll=add(screen,unreal.ScrollBox,name+'Scroll',panel)
    props(scroll,{'scrollbarThickness':{'x':5,'y':5}})
    grid=add(screen,unreal.UniformGridPanel,name,scroll,True)
    props(grid,{'slotPadding':{'left':2,'top':2,'right':2,'bottom':2},'minDesiredSlotWidth':106,'minDesiredSlotHeight':108})
text(screen,'BuyingHeading',c,'BUYING',26,645,320,30,22,TEAL)
text(screen,'SellingHeading',c,'SELLING',710,645,320,30,22,TEAL)
for name,x in [('BuyingList',24),('SellingList',708)]:
    panel=border(screen,name+'Panel',c,x,682,648,196,PANEL)
    scroll=add(screen,unreal.ScrollBox,name+'Scroll',panel)
    props(scroll,{'scrollbarThickness':{'x':5,'y':5}})
    column=add(screen,unreal.VerticalBox,name,scroll,True)
    props(column.slot,{'padding':{'left':8,'top':2,'right':8,'bottom':2}})
border(screen,'SummaryPanel',c,24,886,1332,56,PANEL)
border(screen,'SummaryAccent',c,24,883,1332,1,TEAL)
for label,name,value,x in [('BUYING','BuyingTotalText','0',42),('SELLING','SellingTotalText','0',372),('NET COST','NetCostText','0',702),('BALANCE AFTER TRADE','BalanceText','1,000',1032)]:
    text(screen,name+'Caption',c,label,x,889,304,20,12,MUTED)
    image(screen,name+'Coin',c,x,914,19,19,credits)
    text(screen,name,c,value,x+30,909,274,33,23,GOLD,True)
text(screen,'StatusText',c,'Choose items to buy or sell.',26,961,713,28,14,MUTED,True)
button(screen,'ClearButton',c,'CLEAR',780,954,164,36,PANEL)
button(screen,'ConfirmButton',c,'CONFIRM TRADE',964,954,392,36,TEAL)
assert umg.call_method('CompileWidgetBlueprint',(screen,))
default=unreal.get_default_object(screen.generated_class())
props(default,{'itemCardClass':card.generated_class().get_path_name(),'basketRowClass':row.generated_class().get_path_name()})
assert umg.call_method('CompileWidgetBlueprint',(screen,))
assert unreal.EditorAssetLibrary.save_loaded_asset(screen)

# Add an authored WBP button to the existing I menu.
cheat=unreal.load_asset('/Game/UI/Development/WBP_CheatMenu')
nodes={str(n.widget_name):n.widget for n in umg.call_method('GetWidgets',(cheat,)).widgets if n.widget}
if 'OpenTradeButton' not in nodes:
    column=next(w for w in nodes.values() if isinstance(w,unreal.VerticalBox))
    b=add(cheat,unreal.Button,'OpenTradeButton',column,True)
    props(b,{'backgroundColor':TEAL,'isFocusable':False})
    props(b.slot,{'padding':{'left':0,'top':10,'right':0,'bottom':0}})
    text(cheat,'OpenTradeButtonLabel',b,'OPEN TRADE',0,0,0,0,19,WHITE)
assert umg.call_method('CompileWidgetBlueprint',(cheat,))
unreal.EditorAssetLibrary.save_loaded_asset(cheat)

# A data asset owns prices and starting stock; existing inventories own the items.
catalog=unreal.load_asset('/Game/Data/Trading/DA_TradeCatalog')
if not catalog:
    factory=unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class',unreal.TradeCatalog)
    catalog=assets.create_asset('DA_TradeCatalog','/Game/Data/Trading',unreal.TradeCatalog,factory)
items=[]
definitions=unreal.ResourceCatalog.get_resource_definitions()
for resource in definitions:
    if resource.category==unreal.ResourceCategory.ENERGY: continue
    category=resource.category
    price={unreal.ResourceCategory.RAW_MATERIALS:8,unreal.ResourceCategory.MATERIALS:16,unreal.ResourceCategory.COMPONENTS:38,unreal.ResourceCategory.ADVANCED_GOODS:80}[category]
    stock={unreal.ResourceCategory.RAW_MATERIALS:150,unreal.ResourceCategory.MATERIALS:80,unreal.ResourceCategory.COMPONENTS:15,unreal.ResourceCategory.ADVANCED_GOODS:0}[category]
    item=unreal.TradeItemDefinition()
    item.set_editor_property('id','resource_'+str(resource.widget_prefix))
    item.set_editor_property('kind',unreal.TradeItemKind.RESOURCE)
    item.set_editor_property('resource',resource.resource_type)
    item.set_editor_property('display_name',resource.display_name)
    item.set_editor_property('description','Colony resource. Price shown per unit.')
    item.set_editor_property('icon',unreal.ResourceCatalog.get_resource_icon(resource.resource_type))
    item.set_editor_property('starting_stock',stock); item.set_editor_property('buy_price',price); item.set_editor_property('sell_price',max(1,price//2))
    items.append(item)
blueprints=[]
for path in unreal.EditorAssetLibrary.list_assets('/Game/Data/Buildings/BlueprintCatalog',recursive=True,include_folder=False):
    data=unreal.load_asset(path)
    if isinstance(data,unreal.BuildingDataAsset) and not data.get_editor_property('blueprint_initially_owned'):
        blueprints.append(data)
for data in blueprints[:8]:
    item=unreal.TradeItemDefinition()
    item.set_editor_property('id','blueprint_'+str(data.get_editor_property('blueprint_id')))
    item.set_editor_property('kind',unreal.TradeItemKind.BLUEPRINT); item.set_editor_property('build_tool',data.get_editor_property('build_tool'))
    item.set_editor_property('display_name',str(data.get_editor_property('display_name'))+' blueprint')
    item.set_editor_property('description','Unlocks this building in your construction toolbar.')
    item.set_editor_property('icon',data.get_editor_property('toolbar_icon') or data.get_editor_property('thumbnail'))
    item.set_editor_property('starting_stock',1); item.set_editor_property('buy_price',250); item.set_editor_property('sell_price',0)
    items.append(item)
for key,label,path,icon,price in [
    ('drone_construction','Construction drone','/Game/BluePrints/WorkingDrone/BP_ConstructionDrone','/Game/UI/Thumbsnails/T_ConstructionDroneThumbnail',300),
    ('drone_mining','Mining drone','/Game/BluePrints/MiningDrone/BP_MiningDrone','/Game/UI/Thumbsnails/T_ConstructionDroneThumbnail',350)]:
    drone_class=unreal.load_class(None,path+'.'+path.rsplit('/',1)[-1]+'_C')
    assert drone_class,path
    item=unreal.TradeItemDefinition()
    item.set_editor_property('id',key); item.set_editor_property('display_name',label); item.set_editor_property('description','Delivered beside your base. Only idle drones can be sold.')
    item.set_editor_property('kind',unreal.TradeItemKind.DRONE); item.set_editor_property('drone_class',drone_class)
    item.set_editor_property('icon',unreal.get_default_object(drone_class).get_drone_thumbnail() or unreal.load_asset(icon))
    item.set_editor_property('starting_stock',3); item.set_editor_property('buy_price',price); item.set_editor_property('sell_price',price//2)
    items.append(item)
assert len(set(str(item.id) for item in items))==len(items)
assert all(item.icon for item in items)
catalog.set_editor_property('items',items)
catalog.set_editor_property('merchant_portrait',portrait)
unreal.EditorAssetLibrary.save_loaded_asset(catalog)
report={'passed':True,'items':len(items),'blueprints':len(blueprints[:8]),'basket_visible_rows':4,'assets':[screen.get_path_name(),card.get_path_name(),row.get_path_name(),catalog.get_path_name()]}
(root/'Saved/TradeUiBuild.json').write_text(json.dumps(report,indent=2))
unreal.log('TRADE_UI_BUILD '+json.dumps(report))
