from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
root=Path(unreal.Paths.project_dir()).resolve()
assert root==Path(r'C:\UE5\SurviveThePlanet 5.8')
path='/Game/UI/Contracts/WBP_ContractOffer'
bp=unreal.load_asset(path)
if not bp:
    factory=unreal.WidgetBlueprintFactory()
    factory.set_editor_property('parent_class',unreal.ContractOfferWidget)
    bp=unreal.AssetToolsHelpers.get_asset_tools().create_asset('WBP_ContractOffer','/Game/UI/Contracts',unreal.WidgetBlueprint,factory)
assert bp
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
assert unreal.EditorAssetLibrary.save_loaded_asset(bp)
print('PROGRESSION_UI_WBP_SAVED')
