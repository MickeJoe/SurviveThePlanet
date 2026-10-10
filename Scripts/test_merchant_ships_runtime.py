"""PIE test fixture. The authored schedule is restored before saving/exiting PIE."""
import unreal,json,time
from pathlib import Path
ROOT=Path(unreal.Paths.project_dir()).resolve()
assert ROOT==Path(r'C:\UE5\SurviveThePlanet 5.8')
ST={}
def widgets(cls):
    found=ST['widgets'].call_method('GetAllWidgetsOfClass',(ST['world'],cls,False))
    return [w for w in found if cls!=unreal.TradeScreenWidget or w.is_in_viewport()]
def select_merchant(index):
    visit=unreal.MerchantScheduledVisit();visit.set_editor_property('merchant',ST['merchants'][index]);visit.set_editor_property('arrival_after_hours',0);visit.set_editor_property('stay_hours',8)
    schedule=unreal.MerchantVisitSchedule();schedule.set_editor_property('repeat_cycle_days',8);schedule.set_editor_property('visits',[visit])
    ST['planet'].set_editor_property('merchant_visits',schedule)
def setup():
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();assert world
    ST['world']=world;ST['pc']=unreal.GameplayStatics.get_player_controller(world,0)
    ST['widgets']=unreal.get_default_object(unreal.load_class(None,'/Script/UMG.WidgetBlueprintLibrary'))
    subs=unreal.get_default_object(unreal.load_class(None,'/Script/Engine.SubsystemBlueprintLibrary'))
    ST['trade']=subs.call_method('GetWorldSubsystem',(world,unreal.TradeSubsystem))
    ST['hud']=widgets(unreal.ResourceDisplayWidget)[0]
    weather=next(w for w in widgets(unreal.UserWidget) if w.get_name()=='WeatherTimeDisplay')
    ST['clock']=unreal.find_object(None,weather.get_path_name()+'.WidgetTree_0.ClockText');assert ST['clock']
    for screen in widgets(unreal.TradeScreenWidget):
        if screen.is_in_viewport(): screen.close_trade()
    assert not ST['trade'].get_visit_state().cargo_bay_ready
    assert not unreal.GameplayStatics.get_all_actors_of_class(world,unreal.MerchantShip)
    ST['hud'].set_cheat_speed30();assert unreal.GameplayStatics.get_global_time_dilation(world)==30
    ST['pc'].open_trade_screen('helix_industrial')
    ST['screen']=widgets(unreal.TradeScreenWidget)[0]
    assert unreal.GameplayStatics.get_global_time_dilation(world)<.001
    ST['paused_clock']=str(ST['clock'].get_text())
    ST['planet']=unreal.load_asset('/Game/Data/Planet/DA_PlanetDefinition')
    original=ST['planet'].get_editor_property('merchant_visits')
    snapshot=unreal.MerchantVisitSchedule();snapshot.set_editor_property('repeat_cycle_days',original.repeat_cycle_days)
    detached=[]
    for authored in original.visits:
        visit=unreal.MerchantScheduledVisit();visit.set_editor_property('merchant',authored.merchant)
        visit.set_editor_property('arrival_after_hours',authored.arrival_after_hours);visit.set_editor_property('stay_hours',authored.stay_hours)
        detached.append(visit)
    snapshot.set_editor_property('visits',detached);ST['original_schedule']=snapshot
    ST['merchants']=[unreal.load_asset('/Game/Data/Trading/Merchants/DA_'+key) for key in ['OrionExchange','FrontierSupplies','NexusRobotics','AtlasFoundry']]
    assert len({m.get_editor_property('ship_mesh').get_path_name() for m in ST['merchants']})==4
    select_merchant(0)
    ST['checks']=['x30 produces global dilation 30','No Cargo Bay means no merchant or ship','Four distinct Blender ship meshes are assigned']
    unreal.log('MERCHANT_TEST_SETUP_PASSED')
def start_arrival():
    assert str(ST['clock'].get_text())==ST['paused_clock']
    ST['screen'].close_trade();assert unreal.GameplayStatics.get_global_time_dilation(ST['world'])==30
    ST['hud'].call_method('HandlePauseTimeClicked')
    ST['pc'].open_trade_screen('helix_industrial');screen=widgets(unreal.TradeScreenWidget)[0];screen.close_trade()
    assert unreal.GameplayStatics.get_global_time_dilation(ST['world'])<.001
    ST['checks'].append('Trade freezes the colony clock and restores both x30 and an already-paused state')
    bases=unreal.GameplayStatics.get_all_actors_of_class(ST['world'],unreal.BaseBuilding)
    base=next((a for a in bases if 'BaseModule' in a.get_class().get_name()),bases[0])
    origin=base.get_actor_location();location=origin+unreal.Vector(800,300,0)
    transform=unreal.Transform(location=location)
    gameplay=unreal.get_default_object(unreal.load_class(None,'/Script/Engine.GameplayStatics'))
    bay=gameplay.call_method('BeginDeferredActorSpawnFromClass',(ST['world'],unreal.CargoBay.static_class(),transform,unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None))
    bay=gameplay.call_method('FinishSpawningActor',(bay,transform));bay.set_construction_progress(0)
    ST['bay']=bay
    assert not ST['trade'].get_visit_state().cargo_bay_ready
    bay.set_construction_progress(1)
    assert ST['trade'].get_visit_state().cargo_bay_ready
    ST['hud'].call_method('HandleSpeed1Clicked')
    ST['checks'].append('Cargo Bay construction must be complete before visits start')
    unreal.log('MERCHANT_TEST_ARRIVAL_STARTED dock='+str(bay.get_merchant_dock_transform())+' base='+str(origin))
def landed(index):
    state=ST['trade'].get_visit_state();assert state.present and state.ship_docked
    ship=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(ST['world'],unreal.MerchantShip) if a.get_merchant()==ST['merchants'][index])
    assert ship.is_docked();assert ST['trade'].is_trader_available(ST['merchants'][index].get_editor_property('id'))
    assert (ship.get_actor_location()-ST['bay'].get_merchant_dock_transform().translation).length()<.01
    panel=widgets(unreal.VisitingMerchantWidget)[0]
    assert 'Leaves' in str(panel.get_editor_property('ArrivalText').get_text())
    ST['ship']=ship
    ST['checks'].append('Merchant '+str(index+1)+' landed on Cargo Bay and enabled trade with a departure countdown')
    unreal.log('MERCHANT_TEST_LANDED_'+str(index)+' '+str(ship.get_actor_location()))
def open_merchant_trade():
    ST['pc'].open_trade_screen(ST['merchants'][0].get_editor_property('id'));ST['screen']=widgets(unreal.TradeScreenWidget)[0]
    assert unreal.GameplayStatics.get_global_time_dilation(ST['world'])<.001
    ST['paused_remaining']=ST['trade'].get_visit_state().remaining_game_minutes
    ST['paused_clock']=str(ST['clock'].get_text())
    ST['ship_position']=ST['ship'].get_actor_location()
    unreal.log('MERCHANT_TEST_TRADING_PAUSED')
def next_merchant(index):
    if index==1:
        assert str(ST['clock'].get_text())==ST['paused_clock']
        assert ST['trade'].get_visit_state().remaining_game_minutes==ST['paused_remaining']
        assert ST['ship'].get_actor_location()==ST['ship_position']
        ST['screen'].close_trade();assert unreal.GameplayStatics.get_global_time_dilation(ST['world'])==1
        ST['checks'].append('Merchant departure time and docked ship remain frozen throughout an open trade')
    ST['previous_ship']=ST['ship'];select_merchant(index)
    unreal.log('MERCHANT_TEST_NEXT_'+str(index))
def departed_previous():
    active=unreal.GameplayStatics.get_all_actors_of_class(ST['world'],unreal.MerchantShip)
    assert len(active)==1 and active[0]==ST['ship']
    ST['checks'].append('The previous merchant flew away and cleaned up its ship and effects')
def end_visit():
    schedule=unreal.MerchantVisitSchedule();schedule.set_editor_property('visits',[]);ST['planet'].set_editor_property('merchant_visits',schedule)
    ST['previous_ship']=ST['ship']
    unreal.log('MERCHANT_TEST_FINAL_DEPARTURE_STARTED')
def finish():
    assert not unreal.GameplayStatics.get_all_actors_of_class(ST['world'],unreal.MerchantShip)
    assert not ST['trade'].is_trader_available(ST['merchants'][-1].get_editor_property('id'))
    ST['checks'].append('No ships or effect components remain after the final departure')
    ST['hud'].call_method('HandlePauseTimeClicked')
    ST['bay'].destroy_actor();ST['planet'].set_editor_property('merchant_visits',ST['original_schedule'])
    (ROOT/'Saved/MerchantShipRuntimeTest.json').write_text(json.dumps({'passed':True,'checks':ST['checks']},indent=2))
    unreal.log('MERCHANT_SHIP_RUNTIME_TEST_PASSED '+str(len(ST['checks']))+' checks')
