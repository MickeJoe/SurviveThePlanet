import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.SystemLibrary.execute_console_command(world,'Automation RunTests SurviveThePlanet.Camera.CheatMenuInput')
