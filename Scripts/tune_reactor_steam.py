from pathlib import Path
import json
exec(Path('C:/UE5/SurviveThePlanet 5.8/Scripts/tune_uranium_dust.py').read_text().split("scalar('EmitterUpdateScript'")[0])
system={'refPath':'/Game/Uranium/NS_NuclearSteam.NS_NuclearSteam'}
expression('EmitterUpdateScript','SpawnRate','SpawnRate','18.0 + 10.0 * sin(Emitter.Age * 3.8)')
for name,v in [('Lifetime Min',2.0),('Lifetime Max',3.2),('Uniform Sprite Size Min',22),('Uniform Sprite Size Max',36)]:scalar('ParticleSpawnScript','InitializeParticle',name,v)
expression('ParticleUpdateScript','SolveForcesAndVelocity','Velocity','float3(User.Wind.x + sin(Particles.Age * 3.7 + Particles.MaterialRandom * 6.28) * 32.0, User.Wind.y + cos(Particles.Age * 2.9 + Particles.MaterialRandom * 6.28) * 26.0, 130.0 + sin(Particles.Age * 4.0 + Particles.MaterialRandom * 6.28) * 24.0)')
expression('ParticleUpdateScript','ScaleSpriteSize','Uniform Curve Scale','0.5 + Particles.NormalizedAge * 2.4')
request({'jsonrpc':'2.0','id':2,'method':'tools/call','params':{'name':'call_tool','arguments':{'toolset_name':'editor_toolset.toolsets.asset.AssetTools','tool_name':'save_assets','arguments':{'asset_paths':['/Game/Uranium/NS_NuclearSteam']}}}})
print('Nuclear steam motion and pulses saved')
