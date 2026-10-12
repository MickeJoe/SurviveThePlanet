import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
lib=unreal.EditorAssetLibrary
if not lib.does_asset_exist('/Game/Uranium/NS_NuclearSteam'):
 assert lib.duplicate_asset('/Game/Geothermal/NS_GeothermalSteam','/Game/Uranium/NS_NuclearSteam')
lib.save_asset('/Game/Uranium/NS_NuclearSteam')
