import bpy,math,os,json
ROOT=r'C:\UE5\SurviveThePlanet 5.8';OUT=ROOT+r'\ContentSource\Environment\Expedition08'
exec(open(OUT+'/stp_library08_blender.py').read().split("finish('SM_Cliff08_Corner'")[0])
parts=[rock('Pillar',(-1.55,.1,-.1),(.95,1.05,.83),-18),rock('Pillar',(1.45,.2,-.12),(1.1,1,.69),33),rock('Slab',(-.70,.08,2.32),(.91,1,.30),-18),rock('Slab',(.80,.15,2.12),(.86,1,.33),17),rock('Wedge',(-1.95,-.1,-.03),(1.0,1,.9),15),rock('Wedge',(1.9,.1,-.03),(1.1,1,.85),70)]
parts[0].rotation_euler[1]=.08;parts[1].rotation_euler[1]=-.15
for i in range(12):
    side=-1 if i<6 else 1;a=(i%6)*1.1
    parts.append(rock('Foot',(side*(1.75+.42*math.cos(a)),.15+.7*math.sin(a),-.02),(.25+.05*(i%3),)*3,i*37))
finish('SM_Cliff08_Arch',parts,'cliff','Asymmetric overlapping slabs, uneven piers and rubble; modular Rock04 source geometry')
v=ring(.33,.21,.48,'Vent',8)
for p in v.data.polygons:p.flip()
finish('SM_Atmos08_Vent',[v]+[rock('Foot',(math.cos(i)*.47,math.sin(i)*.47,-.04),(.35,.35,.35),i*50) for i in range(6)],'atmosphere','Static vent with upward-facing rim; no smoke VFX')
v=ring(1.4,.48,.32,'Vent',12)
for p in v.data.polygons:p.flip()
finish('SM_Atmos08_CraterRim',[v],'atmosphere','Upward-facing rim; ground is not excavated')
old=json.load(open(OUT+'/manifest.json'));updates={r['name']:r for r in manifest}
json.dump([updates.get(r['name'],r) for r in old],open(OUT+'/manifest.json','w'),indent=2)
bpy.ops.wm.save_as_mainfile(filepath=OUT+'/STP_Library08_Refined.blend',copy=True)
print('Refined arch and corrected two rim meshes')
