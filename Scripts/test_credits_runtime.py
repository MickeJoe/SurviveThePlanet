"""PIE integration checks for planet-configured credits and live UMG updates."""
import json
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir()).resolve()
assert root == Path(r'C:\UE5\SurviveThePlanet 5.8')
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
assert world, 'PIE must be running'
manager = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ResourceManager)[0]
weather = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.PlanetWeatherManager)[0]
planet = weather.get_planet_definition()
assert planet
expected = max(0, planet.get_editor_property('starting_credits'))
assert manager.get_credits() == expected, (manager.get_credits(), expected)
library = unreal.get_default_object(unreal.load_class(None, '/Script/UMG.WidgetBlueprintLibrary'))
display = library.call_method('GetAllWidgetsOfClass', (world, unreal.ResourceDisplayWidget, False))[0]
tree = display.get_editor_property('CreditsAmountText')
assert tree
while tree.get_parent():
    tree = tree.get_parent()
widgets = {}

def walk(widget):
    widgets[widget.get_name()] = widget
    if isinstance(widget, unreal.PanelWidget):
        for index in range(widget.get_children_count()):
            walk(widget.get_child_at(index))
walk(tree)

def check_balance(value):
    assert manager.get_credits() == value, (manager.get_credits(), value)
    label = str(widgets['CreditsAmountText'].get_text())
    assert not any(c.isalpha() for c in label), label
    assert int(''.join(c for c in label if c.isdigit())) == value, (label, value)

original_visible = display.are_resource_cards_visible()
original_category = display.get_selected_resource_category()
checks = []
try:
    check_balance(expected)
    checks.append('Starting balance comes from the active planet definition: ' + str(expected))
    assert widgets['CreditsIcon'].get_editor_property('brush').resource_object
    assert widgets['CreditsIcon'].get_parent() == widgets['EnergyIcon'].get_parent()
    assert widgets['CreditsAmountText'].slot.get_editor_property('layout_data').offsets.top > widgets['EnergyRateText'].slot.get_editor_property('layout_data').offsets.top
    manager.add_credits(250)
    check_balance(expected + 250)
    assert manager.try_spend_credits(125)
    check_balance(expected + 125)
    assert not manager.try_spend_credits(expected + 126)
    assert not manager.try_spend_credits(-1)
    check_balance(expected + 125)
    checks.append('Income, spending and rejected overspending update the real HUD immediately, with no CR suffix')
    manager.set_credits(-50)
    check_balance(0)
    manager.add_credits(-1)
    check_balance(0)
    manager.set_credits(2147483647)
    manager.add_credits(100)
    check_balance(2147483647)
    manager.add_credits(-2147483648)
    check_balance(0)
    checks.append('Negative balances and integer overflow are prevented')
    for category in [unreal.ResourceCategory.RAW_MATERIALS, unreal.ResourceCategory.MATERIALS,
                     unreal.ResourceCategory.COMPONENTS, unreal.ResourceCategory.ADVANCED_GOODS]:
        display.select_resource_category(category)
        assert widgets['CreditsAmountText'].get_visibility() != unreal.SlateVisibility.COLLAPSED
        assert widgets['EnergyResourceCard'].get_visibility() != unreal.SlateVisibility.COLLAPSED
    display.set_resource_cards_visible(False)
    manager.set_credits(1234)
    check_balance(1234)
    display.set_resource_cards_visible(True)
    check_balance(1234)
    checks.append('Credits survive category switching and continue updating when the resource bar is collapsed')
finally:
    manager.set_credits(expected)
    display.select_resource_category(original_category)
    display.set_resource_cards_visible(original_visible)

report = {'passed': True, 'checks': checks, 'starting_credits': expected}
(root / ('Saved/CreditsRuntimeTest-' + str(expected) + '.json')).write_text(json.dumps(report, indent=2))
unreal.log('CREDITS_RUNTIME_TEST ' + json.dumps(report))
