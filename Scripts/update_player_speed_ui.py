import unreal,json
bp=unreal.load_asset('/Game/UI/WBP_WeatherTimeDisplay_V2')
umg=unreal.get_default_object(unreal.UMGToolSet)
nodes={str(n.widget_name):n.widget for n in umg.call_method('GetWidgets',(bp,)).widgets if n.widget}
labels={}
for name,label in [('Speed1Button','1x'),('Speed15Button','2x'),('Speed2Button','4x'),('Speed3Button','10x')]:
    button=nodes[name]
    child=button.get_child_at(0)
    assert isinstance(child,unreal.TextBlock),(name,child)
    child.set_text(unreal.Text(label))
    button.set_tool_tip_text(unreal.Text(label+' simulation speed'))
    labels[name]=str(child.get_text())
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
assert unreal.EditorAssetLibrary.save_loaded_asset(bp,False)
unreal.log(str(labels))
