import unreal,json
from pathlib import Path
project=Path(unreal.Paths.project_dir()).resolve()
taskdir=project/"Saved"
helpers=(project/'Scripts/create_building_flow_ui.py').read_text(encoding='utf-8-sig')
exec(helpers[:helpers.index('textures={}')])
textures={}
for p in (project/'ContentSource/UI/ObjectiveRewards').glob('*.png'):
    task=unreal.AssetImportTask();task.filename=str(p);task.destination_path='/Game/UI/Objectives';task.destination_name=p.stem;task.automated=True;task.replace_existing=True;task.save=True
    assets.import_asset_tasks([task]);tex=unreal.load_asset('/Game/UI/Objectives/'+p.stem)
    tex.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    tex.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    tex.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
    library.save_loaded_asset(tex);textures[p.stem]=tex
def widget_bp(name):
    bp=unreal.load_asset('/Game/UI/Objectives/'+name)
    if not bp:
        factory=unreal.WidgetBlueprintFactory();factory.set_editor_property('parent_class',unreal.UserWidget)
        bp=assets.create_asset(name,'/Game/UI/Objectives',unreal.WidgetBlueprint,factory)
    clear(bp);return bp
bp=widget_bp('WBP_ObjectiveRewardIcon')
unreal.BlueprintEditorLibrary.reparent_blueprint(bp,unreal.ObjectiveRewardIconWidget)
box=size(bp,'ChipSize',None,46,36)
props(box,{'visibility':'Visible'})
border=add(bp,unreal.Border,'RewardSurface',box)
props(border,{'brushColor':{'r':.024,'g':.048,'b':.06,'a':.94},'padding':{'left':0,'top':0,'right':0,'bottom':0},'visibility':'Visible'})
canvas=add(bp,unreal.CanvasPanel,'RewardCanvas',border)
props(canvas,{'visibility':'SelfHitTestInvisible'})
icon=image(bp,'RewardIcon',canvas,None,True);place(icon,2,3,28,28);props(icon,{'visibility':'HitTestInvisible'})
amount=text(bp,'AmountText',canvas,'+40',11,white,True);place(amount,19,17,26,18)
font=json.loads(ObjectTools.get_properties(amount,['font']))['font'];font['outlineSettings']['outlineSize']=1
props(amount,{'font':font,'justification':'Right','shadowOffset':{'x':1,'y':1},'shadowColorAndOpacity':{'r':0,'g':0,'b':0,'a':1}})
badge=text(bp,'BlueprintBadge',canvas,'BP',8,cyan,True);place(badge,29,0,17,13);props(badge,{'visibility':'Collapsed'})
unreal.BlueprintEditorLibrary.compile_blueprint(bp);assert library.save_loaded_asset(bp,False)
bp=widget_bp('WBP_ObjectiveRewardStrip')
box=size(bp,'StripWidth',None,302)
props(box,{'visibility':'SelfHitTestInvisible'})
wrap=add(bp,unreal.WrapBox,'RewardsWrap',box,True)
props(wrap,{'wrapSize':302,'bExplicitWrapSize':True,'innerSlotPadding':{'x':4,'y':3},'visibility':'SelfHitTestInvisible'})
unreal.BlueprintEditorLibrary.compile_blueprint(bp);assert library.save_loaded_asset(bp,False)
tracker=unreal.load_asset('/Game/UI/Objectives/WBP_ObjectiveTracker')
nodes={str(n.widget_name):n.widget for n in umg.call_method('GetWidgets',(tracker,)).widgets if n.widget}
for w in nodes.values():
    if not isinstance(w,(unreal.TextBlock,unreal.Image)):props(w,{'visibility':'SelfHitTestInvisible'})
    if isinstance(w,unreal.SizeBox) and w.get_editor_property('override_height_override'):
        w.set_editor_property('override_height_override',False)
unreal.BlueprintEditorLibrary.compile_blueprint(tracker);assert library.save_loaded_asset(tracker,False)
(taskdir/'objective_reward_authoring.json').write_text(json.dumps({'widgets':list(nodes),'new_assets':['WBP_ObjectiveRewardIcon','WBP_ObjectiveRewardStrip'],'chip_size':[46,36],'wrap_width':302,'icons':list(textures)},indent=2))
