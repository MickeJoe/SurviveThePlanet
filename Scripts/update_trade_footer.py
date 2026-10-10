"""Update only existing trading WBP layout, preserving the edited trade catalog."""
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir()).resolve()
assert root==Path(r'C:\UE5\SurviveThePlanet 5.8')
script=(root/'Scripts/create_trade_ui.py').read_text()
exec(script[:script.index("credits=unreal.load_asset")])
credits=unreal.load_asset('/Game/UI/Contracts/Icons/T_Credits')
card=unreal.load_asset('/Game/UI/Trading/WBP_TradeItemCard')
nodes={str(n.widget_name):n.widget for n in umg.call_method('GetWidgets',(card,)).widgets if n.widget}
props(nodes['QuantityText'],{'justification':'Center'})
place(nodes['QuantityText'],31,80,39,22)
assert umg.call_method('CompileWidgetBlueprint',(card,))
unreal.EditorAssetLibrary.save_loaded_asset(card)
screen=unreal.load_asset('/Game/UI/Trading/WBP_TradeScreen')
nodes={str(n.widget_name):n.widget for n in umg.call_method('GetWidgets',(screen,)).widgets if n.widget}
c=nodes['TradeContents']
for name in ['BuyingTotalLabel','SellingTotalLabel','AfterLabel','AfterCoin']:
    if name in nodes: umg.call_method('RemoveWidget',(screen,nodes[name]))
if 'SummaryPanel' not in nodes:
    summary=border(screen,'SummaryPanel',c,24,886,1332,56,PANEL)
    props(summary.slot,{'zOrder':-1})
    border(screen,'SummaryAccent',c,24,883,1332,1,TEAL)
for label,name,value,x in [('BUYING','BuyingTotalText','0',42),('SELLING','SellingTotalText','0',372),('NET COST','NetCostText','0',702),('BALANCE AFTER TRADE','BalanceText','1,000',1032)]:
    if name+'Caption' not in nodes: text(screen,name+'Caption',c,label,x,889,304,20,12,MUTED)
    if name+'Coin' not in nodes: image(screen,name+'Coin',c,x,914,19,19,credits)
    if name not in nodes: text(screen,name,c,value,x+30,909,274,33,23,GOLD,True)
    else: place(nodes[name],x+30,909,274,33)
place(nodes['StatusText'],26,961,713,28)
place(nodes['ClearButton'],780,954,164,36)
place(nodes['ConfirmButton'],964,954,392,36)
assert umg.call_method('CompileWidgetBlueprint',(screen,))
assert unreal.EditorAssetLibrary.save_loaded_asset(screen)
unreal.log('TRADE_FOOTER_UPDATED: centered quantities and four-column summary')
