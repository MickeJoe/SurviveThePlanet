import unreal
from pathlib import Path
taskdir=Path(unreal.Paths.project_saved_dir())
umg=unreal.get_default_object(unreal.UMGToolSet)
for path in ['/Game/UI/WBP_DroneSelectionPopup','/Game/UI/WBP_DroneSelectionCard']:
    bp=unreal.load_asset(path)
    nodes={str(n.widget_name):n.widget for n in umg.call_method('GetWidgets',(bp,)).widgets if n.widget}
    if 'TitleText' in nodes:
        nodes['TitleText'].set_text(unreal.Text('SELECT DRONE TYPE'))
        nodes['TitleText'].set_tool_tip_text(unreal.Text('One card per drone type. The count on each card shows how many drones of that type are available.'))
    if 'DroneCountText' in nodes:
        nodes['DroneCountText'].set_tool_tip_text(unreal.Text('Available drones of this type. For example, x2 means two drones. Assigned drones and drones currently constructing are busy.'))
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    assert unreal.EditorAssetLibrary.save_loaded_asset(bp,False)
(taskdir/'drone_picker_clarified.txt').write_text('Saved SELECT DRONE TYPE title and count tooltip.')
