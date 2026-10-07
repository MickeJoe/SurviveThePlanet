"""Restyle the two existing factory assets through Blender's native MCP."""
import bpy, math, json, shutil
from pathlib import Path
from mathutils import Vector
ROOT=Path(r"C:\UE5\SurviveThePlanet 5.8")

def restyle(kind):
    assert kind in ("PolymerPlant","ConnectorFactory")
    folder=ROOT/"ContentSource"/kind
    backup=ROOT/"ContentSource"/"_StyleBackups"/"2026-10-04"/kind
    if not backup.exists():
        shutil.copytree(folder,backup,ignore=shutil.ignore_patterns("*.blend1","*.blend2"))
    source_scene="STP_PolymerPlant_Refined" if kind=="PolymerPlant" else "STP_ConnectorFactory"
    with bpy.data.libraries.load(str(backup/(kind+".blend")),link=False) as (source,target):
        target.scenes=[next(n for n in source.scenes if n.startswith(source_scene))]
    scene=target.scenes[0]
    bpy.context.window.scene=scene
    body=next(o for o in scene.objects if o.name.startswith("SM_"+kind))
    assert not body.get("basecamp_style_v1"),"Already restyled; restore backup before rerunning"
    original_min=Vector(tuple(min((body.matrix_world@v.co)[axis] for v in body.data.vertices) for axis in range(3)))
    original_max=Vector(tuple(max((body.matrix_world@v.co)[axis] for v in body.data.vertices) for axis in range(3)))
    prefix="Polymer_" if kind=="PolymerPlant" else "Connector_"
    find=lambda suffix:next(m for m in body.data.materials if m.name.startswith(prefix+suffix))
    ivory,orange,dark,steel=[find(s) for s in ("Ivory","Orange","Graphite","Steel")]
    additions=[]
    def box(name,pos,size,material,bevel=.015):
        bpy.ops.mesh.primitive_cube_add(size=1,location=pos)
        o=bpy.context.object;o.name="Basecamp_"+name;o.dimensions=size
        bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
        o.data.materials.append(material)
        if bevel:
            b=o.modifiers.new("Machined chamfer","BEVEL");b.width=bevel;b.segments=1
            o.modifiers.new("Panel normals","WEIGHTED_NORMAL")
        additions.append(o);return o
    def bolt(pos,axis="Z",radius=.018):
        bpy.ops.mesh.primitive_cylinder_add(vertices=6,radius=radius,depth=.014,location=pos)
        o=bpy.context.object;o.name="Basecamp_hex_fastener";o.data.materials.append(steel)
        if axis=="X":o.rotation_euler.y=math.pi/2
        elif axis=="Y":o.rotation_euler.x=math.pi/2
        additions.append(o)
    if kind=="PolymerPlant":
        for y in [-1.46,1.46]:
            for x in [-.35,.48,1.31]:
                box("panel_header",(x,y,1.57),(.68,.018,.052),steel,.008)
                for dx in [-.285,.285]:
                    bolt((x+dx,y,1.48),"Y")
                    bolt((x+dx,y,.80),"Y")
                box("panel_latch",(x+.26,y,1.14),(.06,.025,.12),steel,.008)
            for x in [-.73,1.86]:
                box("corner_gusset",(x,y,1.65),(.20,.027,.20),steel,.025)
                bolt((x,y*1.007,1.66),"Y")
        for x in [-.53,1.66]:
            box("roof_side_rail",(x,0,1.96),(.075,2.34,.052),ivory,.014)
            for y in [-1.02,0,1.02]:
                box("roof_rail_collar",(x,y,1.99),(.12,.12,.035),steel,.012)
                bolt((x,y,2.02))
        for y in [-1.16,1.16]:
            for x in [-.10,.58,1.26]:
                box("roof_panel",(x,y,1.963),(.62,.18,.035),ivory,.015)
                bolt((x+.21,y,1.99),radius=.015)
        for y in [-.67,.67]:
            for angle in range(0,360,60):
                a=math.radians(angle)
                bolt((-1.38+.48*math.cos(a),y+.48*math.sin(a),1.85),radius=.020)
            box("tank_reinforcement",(-1.89,y,1.14),(.026,.18,.53),steel,.009)
            box("tank_label_plate",(-1.91,y,1.26),(.012,.13,.16),ivory,.006)
            for z in [1.01,1.45]:bolt((-1.918,y,z),"X",.015)
        for z in [.86,.94,1.02,1.10]:
            box("service_vent_frame",(2.085,.55,z),(.032,.64,.035),steel,.004)
        for y in [-1.20,1.20]:
            for x in [-1.97,1.81]:
                box("foundation_lug",(x,y,.32),(.18,.18,.055),ivory,.020)
                bolt((x,y,.357))
    else:
        for x in [-1.28,-.43,.43,1.28]:
            box("back_panel_cap",(x,1.455,1.71),(.69,.019,.065),steel,.01)
            for dx in [-.285,.285]:
                for z in [.79,1.61]:bolt((x+dx,1.465,z),"Y")
            box("panel_latch",(x+.25,1.468,1.10),(.055,.025,.13),steel,.008)
        for x in [-1.785,1.785]:
            for y in [.17,.62,1.07]:
                box("side_panel",(x,y,1.13),(.028,.40,1.15),ivory,.018)
                for z in [.72,1.57]:bolt((x*1.012,y,z),"X")
            box("side_reinforcement",(x,.64,1.73),(.04,1.27,.075),steel,.01)
        for x in [-1.30,-.65,0,.65,1.30]:
            box("roof_service_panel",(x,-.55,2.151),(.60,.75,.047),ivory,.023)
            for dx in [-.22,.22]:bolt((x+dx,-.76,2.184),radius=.015)
        for x in [-1.68,1.68]:
            box("roof_rim_rail",(x,.06,2.17),(.074,1.95,.06),steel,.015)
            for y in [-.76,.0,.85]:
                box("rim_clamp",(x,y,2.20),(.13,.12,.04),ivory,.014)
                bolt((x,y,2.23))
        for x in [-1.72,1.72]:
            box("bay_corner_plate",(x,-1.007,1.76),(.19,.024,.22),steel,.018)
            bolt((x,-1.024,1.79),"Y")
            box("bay_access_panel",(x,-1.001,1.26),(.14,.024,.38),ivory,.014)
            for z in [1.13,1.39]:bolt((x,-1.020,z),"Y",.014)
        for x in [-1.35,0,1.35]:
            box("assembly_service_panel",(x,-1.118,.70),(.58,.025,.32),ivory,.020)
            for dx in [-.22,.22]:bolt((x+dx,-1.138,.70),"Y",.016)
        for y in [.03,.47]:
            for z in [.50,1.25]:bolt((1.851,y,z),"X",.014)
        for x in [-2.06,2.06]:
            for y in [-1.47,1.47]:
                box("foundation_clamp",(x,y,.32),(.18,.18,.055),ivory,.018)
                bolt((x,y,.357))
    # Preserve all runtime pivots and separately animated objects.
    bpy.ops.object.select_all(action="DESELECT")
    body.select_set(True)
    for o in additions:
        o.select_set(True);bpy.context.view_layer.objects.active=o
        for mod in list(o.modifiers):bpy.ops.object.modifier_apply(modifier=mod.name)
    bpy.context.view_layer.objects.active=body;bpy.ops.object.join()
    body.name="SM_"+kind
    colors={"Ivory":(.80,.78,.73),"Orange":(.80,.18,.028),"Graphite":(.055,.066,.076),"Steel":(.38,.41,.42),"Copper":(.67,.27,.075),"Product":(.83,.85,.77)}
    # Shared weathered paint: large dirt variation, cavity grime and restrained edge chips.
    for material in body.data.materials:
        suffix=next((s for s in colors if material.name.startswith(prefix+s)),None)
        if suffix is None:continue
        color=colors[suffix];material.diffuse_color=(*color,1);material.use_nodes=True
        nodes=material.node_tree.nodes;links=material.node_tree.links;nodes.clear()
        output=nodes.new("ShaderNodeOutputMaterial");p=nodes.new("ShaderNodeBsdfPrincipled");links.new(p.outputs["BSDF"],output.inputs["Surface"])
        p.inputs["Metallic"].default_value=.12 if suffix=="Ivory" else .45
        geom=nodes.new("ShaderNodeNewGeometry")
        broad=nodes.new("ShaderNodeTexNoise");broad.inputs["Scale"].default_value=4.5;broad.inputs["Detail"].default_value=3
        grime=nodes.new("ShaderNodeValToRGB");grime.color_ramp.elements[0].position=.26;grime.color_ramp.elements[0].color=(*(v*.67 for v in color),1);grime.color_ramp.elements[1].position=.74;grime.color_ramp.elements[1].color=(*color,1)
        links.new(broad.outputs["Fac"],grime.inputs["Fac"])
        cavity=nodes.new("ShaderNodeAmbientOcclusion");cavity.inputs["Distance"].default_value=.14;cavity.samples=8
        cavity_ramp=nodes.new("ShaderNodeValToRGB");cavity_ramp.color_ramp.elements[0].color=(.72,.67,.60,1);cavity_ramp.color_ramp.elements[1].color=(1,1,1,1);links.new(cavity.outputs["AO"],cavity_ramp.inputs["Fac"])
        dirt=nodes.new("ShaderNodeMixRGB");dirt.blend_type="MULTIPLY";dirt.inputs[0].default_value=.65;links.new(grime.outputs["Color"],dirt.inputs[1]);links.new(cavity_ramp.outputs["Color"],dirt.inputs[2])
        fine=nodes.new("ShaderNodeTexNoise");fine.inputs["Scale"].default_value=135;fine.inputs["Detail"].default_value=2
        edge=nodes.new("ShaderNodeValToRGB");edge.color_ramp.elements[0].position=.515;edge.color_ramp.elements[0].color=(0,0,0,1);edge.color_ramp.elements[1].position=.555;edge.color_ramp.elements[1].color=(1,1,1,1);links.new(geom.outputs["Pointiness"],edge.inputs["Fac"])
        chip=nodes.new("ShaderNodeMath");chip.operation="MULTIPLY";links.new(edge.outputs["Color"],chip.inputs[0]);links.new(fine.outputs["Fac"],chip.inputs[1])
        worn=nodes.new("ShaderNodeMixRGB");links.new(chip.outputs[0],worn.inputs[0]);links.new(dirt.outputs[0],worn.inputs[1]);worn.inputs[2].default_value=(.28,.29,.28,1) if suffix in ("Ivory","Orange") else (.46,.47,.44,1);links.new(worn.outputs[0],p.inputs["Base Color"])
        rough=nodes.new("ShaderNodeMapRange");rough.inputs["From Min"].default_value=0;rough.inputs["From Max"].default_value=1;rough.inputs["To Min"].default_value=.48;rough.inputs["To Max"].default_value=.76;links.new(fine.outputs["Fac"],rough.inputs["Value"]);links.new(rough.outputs[0],p.inputs["Roughness"])
        bump=nodes.new("ShaderNodeBump");bump.inputs["Strength"].default_value=.12;bump.inputs["Distance"].default_value=.004;links.new(fine.outputs["Fac"],bump.inputs["Height"]);links.new(bump.outputs["Normal"],p.inputs["Normal"])
    bpy.ops.object.mode_set(mode="EDIT");bpy.ops.mesh.select_all(action="SELECT");bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=.005);bpy.ops.object.mode_set(mode="OBJECT")
    scene.render.engine="CYCLES";scene.cycles.samples=8
    scene.render.bake.use_pass_direct=False;scene.render.bake.use_pass_indirect=False;scene.render.bake.use_pass_color=True;scene.render.bake.margin=8
    for suffix,bake_type,size in [("BaseColor","DIFFUSE",2048),("AO","AO",1024),("Roughness","ROUGHNESS",1024),("Normal","NORMAL",1024)]:
        name="T_"+kind+"_"+suffix
        previous=bpy.data.images.get(name)
        if previous:bpy.data.images.remove(previous)
        img=bpy.data.images.new(name,width=size,height=size,alpha=False)
        if suffix!="BaseColor":img.colorspace_settings.name="Non-Color"
        for material in body.data.materials:
            node=material.node_tree.nodes.new("ShaderNodeTexImage");node.image=img;material.node_tree.nodes.active=node
        bpy.ops.object.bake(type=bake_type)
        img.filepath_raw=str(folder/(name+".png"));img.file_format="PNG";img.save()
    current_min=Vector(tuple(min((body.matrix_world@v.co)[axis] for v in body.data.vertices) for axis in range(3)))
    current_max=Vector(tuple(max((body.matrix_world@v.co)[axis] for v in body.data.vertices) for axis in range(3)))
    assert (current_min-original_min).length<.002 and (current_max-original_max).length<.002,(original_min,original_max,current_min,current_max)
    bpy.ops.export_scene.fbx(filepath=str(folder/("SM_"+kind+".fbx")),use_selection=True,apply_unit_scale=True,axis_forward="-Y",axis_up="Z",bake_anim=False,object_types={"MESH"})
    body["basecamp_style_v1"]=True
    scene.frame_set(1);scene.cycles.samples=24;scene.render.filepath=str(folder/("T_"+kind+".png"))
    bpy.ops.wm.save_as_mainfile(filepath=str(folder/(kind+".blend")))
    bpy.ops.render.render(write_still=True)
    report={"building":kind,"size_m":list(current_max-current_min),"vertices":len(body.data.vertices),"added_parts":len(additions),"backup":str(backup)}
    (folder/"BasecampStyle.json").write_text(json.dumps(report,indent=2))
    return report

