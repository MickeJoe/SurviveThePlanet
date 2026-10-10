"""PIE integration checks: real resources, currency, WBP baskets and deliveries."""
import json
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir()).resolve()
assert root==Path(r'C:\UE5\SurviveThePlanet 5.8')
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
assert world,'PIE must be running'
subsystems=unreal.get_default_object(unreal.load_class(None,'/Script/Engine.SubsystemBlueprintLibrary'))
trade=subsystems.call_method('GetWorldSubsystem',(world,unreal.TradeSubsystem))
manager=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.ResourceManager)[0]
pc=unreal.GameplayStatics.get_player_controller(world,0)
items=trade.get_items()
resources=[i for i in items if i.kind==unreal.TradeItemKind.RESOURCE]
checks=[]
trader='helix_industrial'
def line(item,n): return unreal.TradeLine(item_id=item.id,quantity=n)
def confirm(buy,sell):
    result=trade.confirm_trade(trader,buy,sell)
    return result[0] if isinstance(result,tuple) else result
def balance(): return manager.get_credits()
original_balance=balance()
original={str(i.id):manager.get_resource_amount(i.resource) for i in resources}
unreal.GameplayStatics.set_game_paused(world,True)
try:
    for i in resources[:8]: manager.set_resource_amount(i.resource,100)
    a,b,c,d=resources[:4]
    manager.set_credits(1000)
    buy=[line(a,3),line(b,4)]
    sell=[line(c,5),line(d,2)]
    q=trade.quote_trade(trader,buy,sell)
    expected=1000+5*c.sell_price+2*d.sell_price-3*a.buy_price-4*b.buy_price
    assert q.can_confirm and q.balance_after==expected
    assert balance()==1000 and manager.get_resource_amount(a.resource)==100
    assert confirm(buy,sell)
    assert balance()==expected
    assert [manager.get_resource_amount(i.resource) for i in [a,b,c,d]]==[103,104,95,98]
    assert trade.get_stock(trader,a.id)==a.starting_stock-3
    checks.append('One confirmed transaction buys and sells several different items; quotes do not mutate inventory')
    manager.set_credits(0)
    q=trade.quote_trade(trader,[line(a,1)],[line(c,10)])
    assert q.can_confirm and confirm([line(a,1)],[line(c,10)])
    checks.append('Sale revenue funds purchases in the same transaction')
    before=(balance(),manager.get_resource_amount(a.resource),trade.get_stock(trader,a.id))
    assert not confirm([line(a,999999)],[])
    assert not confirm([line(a,1)],[line(a,1)])
    assert not confirm([line(a,0)],[])
    assert not confirm([],[line(c,999999)])
    manager.set_credits(0)
    assert not confirm([line(a,1)],[])
    assert manager.get_resource_amount(a.resource)==before[1] and trade.get_stock(trader,a.id)==before[2]
    checks.append('Invalid quantities, overselling, insufficient funds, and conflicting lines are rejected without inventory changes')
    manager.set_credits(1000)
    assert confirm([line(a,1),line(a,2)],[])
    manager.set_credits(2147483647)
    assert not confirm([],[line(c,1)])
    checks.append('Duplicate lines are combined and credit overflow is rejected')

    manager.set_credits(1000)
    input_before=(pc.is_move_input_ignored(),pc.is_look_input_ignored())
    pc.open_trade_screen(trader)
    assert pc.is_move_input_ignored() and pc.is_look_input_ignored()
    library=unreal.get_default_object(unreal.load_class(None,'/Script/UMG.WidgetBlueprintLibrary'))
    screen=library.call_method('GetAllWidgetsOfClass',(world,unreal.TradeScreenWidget,False))[0]
    grid=screen.get_editor_property('TraderGrid')
    for index in range(4):
        grid.get_child_at(index).call_method('Increase')
    player_grid=screen.get_editor_property('PlayerGrid')
    for index in range(4,8):
        player_grid.get_child_at(index).call_method('Increase')
    assert screen.get_editor_property('BuyingList').get_children_count()==4
    assert screen.get_editor_property('SellingList').get_children_count()==4
    assert str(resources[0].display_name) in str(grid.get_child_at(0).get_editor_property('tool_tip_text'))
    assert screen.get_editor_property('ConfirmButton').get_is_enabled()
    assert screen.get_editor_property('CreditsText').get_text()
    screen.call_method('Clear')
    assert screen.get_editor_property('BuyingList').get_children_count()==0
    assert not screen.get_editor_property('ConfirmButton').get_is_enabled()
    assert balance()==1000
    for index in range(4): grid.get_child_at(index).call_method('Increase')
    screen.close_trade()
    assert balance()==1000
    assert (pc.is_move_input_ignored(),pc.is_look_input_ignored())==input_before
    checks.append('Authored WBP cards support four different basket items, clear and close discard unconfirmed selections')

    bp=next(i for i in items if i.kind==unreal.TradeItemKind.BLUEPRINT and trade.get_owned_count(i.id)==0)
    assert confirm([line(bp,1)],[])
    assert trade.get_owned_count(bp.id)==1 and trade.get_sellable_count(bp.id)==0
    assert not confirm([line(bp,1)],[])
    checks.append('Blueprint purchase grants the existing construction unlock exactly once')
    manager.set_credits(1000)
    drone=next(i for i in items if i.kind==unreal.TradeItemKind.DRONE)
    count=trade.get_owned_count(drone.id)
    existing=set(actor.get_name() for actor in unreal.GameplayStatics.get_all_actors_of_class(world,drone.drone_class))
    assert confirm([line(drone,2)],[]),'Drone delivery failed'
    assert trade.get_owned_count(drone.id)==count+2
    deliveries=[actor for actor in unreal.GameplayStatics.get_all_actors_of_class(world,drone.drone_class) if actor.get_name() not in existing]
    assert len(deliveries)==2
    positions=[actor.get_actor_location() for actor in deliveries]
    assert (positions[0]-positions[1]).length()>1
    assert trade.get_sellable_count(drone.id)>=2
    assert confirm([],[line(drone,1)])
    assert trade.get_owned_count(drone.id)==count+1
    checks.append('Purchased drones spawn beside the base with distinct reserved grid cells; idle drones can be sold')
    # Removing a base is confined to this disposable PIE world.
    for base in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.BaseBuilding):
        if base.get_building_type()==unreal.STPBuildingType.BASE_MODULE: base.destroy_actor()
    before=(balance(),manager.get_resource_amount(a.resource),trade.get_stock(trader,a.id),trade.get_stock(trader,drone.id))
    assert not confirm([line(a,1),line(drone,1)],[])
    assert (balance(),manager.get_resource_amount(a.resource),trade.get_stock(trader,a.id),trade.get_stock(trader,drone.id))==before
    checks.append('Failed drone delivery cancels the entire mixed basket without deductions or stock changes')
finally:
    manager.set_credits(original_balance)
    for i in resources: manager.set_resource_amount(i.resource,original[str(i.id)])
    unreal.GameplayStatics.set_game_paused(world,False)
report={'passed':True,'checks':checks}
(root/'Saved/TradeRuntimeTest.json').write_text(json.dumps(report,indent=2))
unreal.log('TRADE_RUNTIME_TEST '+json.dumps(report))
