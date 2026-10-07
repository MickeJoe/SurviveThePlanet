"""PIE regression for category filtering, all inventory bindings, icons and collapse."""
import unreal,json
from pathlib import Path
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
assert world,'PIE must be running'
library=unreal.get_default_object(unreal.load_class(None,'/Script/UMG.WidgetBlueprintLibrary'))
display=library.call_method('GetAllWidgetsOfClass',(world,unreal.ResourceDisplayWidget,False))[0]
manager=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.ResourceManager)[0]
tree=display.get_editor_property('ResourceCardsGrid')
while tree.get_parent(): tree=tree.get_parent()
widgets={}
def walk(widget):
    widgets[widget.get_name()]=widget
    if isinstance(widget,unreal.PanelWidget):
        for index in range(widget.get_children_count()): walk(widget.get_child_at(index))
walk(tree)
definitions=unreal.ResourceCatalog.get_resource_definitions()
original={d.resource_type:manager.get_resource_amount(d.resource_type) for d in definitions}
original_category=display.get_selected_resource_category()
original_visible=display.are_resource_cards_visible()
checks=[]
try:
    assert len(definitions)==46
    for index,definition in enumerate(definitions):
        key=str(definition.widget_prefix)
        assert key+'ResourceCard' in widgets,key
        assert widgets[key+'Icon'].get_editor_property('brush').resource_object,key
        value=1000 if definition.resource_type==unreal.ResourceType.ENERGY else 1237+index
        manager.set_resource_amount(definition.resource_type,value)
        label=str(widgets[key+'AmountText'].get_text())
        assert int(''.join(c for c in label if c.isdigit()))==value,(key,label,value)
    checks.append('All 46 inventory amounts update through the real manager event; every item has a texture')
    for category,count in [(unreal.ResourceCategory.RAW_MATERIALS,9),(unreal.ResourceCategory.MATERIALS,11),(unreal.ResourceCategory.COMPONENTS,15),(unreal.ResourceCategory.ADVANCED_GOODS,10)]:
        display.select_resource_category(category)
        visible=[]
        slots=[]
        for definition in definitions:
            card=widgets[str(definition.widget_prefix)+'ResourceCard']
            if card.get_visibility()!=unreal.SlateVisibility.COLLAPSED:
                visible.append(definition)
                slot=card.slot
                if definition.resource_type!=unreal.ResourceType.ENERGY: slots.append((slot.get_editor_property('row'),slot.get_editor_property('column')))
                assert definition.category in [category,unreal.ResourceCategory.ENERGY]
        assert len(visible)==count+1,(category,len(visible))
        assert len(set(slots))==count,'Grid overlap'
        assert sum(row==0 for row,col in slots)==(count+1)//2
        assert sum(row==1 for row,col in slots)==count//2
        assert max(row for row,col in slots)<=1 and max(col for row,col in slots)<=7
    checks.append('Balanced rows 5+4, 6+5, 8+7 and 5+5; Category counts 9/11/15/10 plus energy, unique positions, at most two rows and eight columns')
    display.set_resource_cards_visible(False)
    assert not display.are_resource_cards_visible()
    assert widgets['ResourceCardsBody'].get_visibility()==unreal.SlateVisibility.COLLAPSED
    assert widgets['ResourceCollapseDownIcon'].get_visibility()!=unreal.SlateVisibility.COLLAPSED
    assert widgets['ResourceCollapseUpIcon'].get_visibility()==unreal.SlateVisibility.COLLAPSED
    assert not any(name.endswith('NameText') for name in widgets)
    assert 'ToggleResourceCardsText' not in widgets
    for name in ['RawMaterialsButton','MaterialsButton','ComponentsButton','AdvancedGoodsButton','ToggleResourceCardsButton']:
        assert widgets[name].get_visibility()!=unreal.SlateVisibility.COLLAPSED
    manager.set_resource_amount(unreal.ResourceType.XENOTECH_MODULES,3456)
    assert '3456'==''.join(c for c in str(widgets['XenotechModulesAmountText'].get_text()) if c.isdigit())
    display.select_resource_category(unreal.ResourceCategory.ADVANCED_GOODS)
    assert display.are_resource_cards_visible()
    assert widgets['ResourceCardsGrid'].get_visibility()!=unreal.SlateVisibility.COLLAPSED
    checks.append('Collapse leaves category controls available; inventory updates while hidden; category click reopens cards')
    report={'passed':True,'checks':checks,'resources':len(definitions),'categories':[9,11,15,10]}
finally:
    for resource,amount in original.items():manager.set_resource_amount(resource,amount)
    display.select_resource_category(original_category)
    display.set_resource_cards_visible(original_visible)
(Path(unreal.Paths.project_dir())/'Saved/ResourceHudRuntimeTest.json').write_text(json.dumps(report,indent=2))
unreal.log('RESOURCE_HUD_RUNTIME_TEST '+json.dumps(report))
