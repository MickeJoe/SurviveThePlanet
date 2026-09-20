"""Bounded uncooked Editor -game smoke test of the configured default map."""
from pathlib import Path
import json
import subprocess
project=Path(__file__).resolve().parents[1]
assert str(project).lower()==r'C:\UE5\SurviveThePlanet 5.8'.lower()
log=project/'Saved'/'Logs'/'ClusterGameSmoke.log'
command=[r'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe',
         str(project/'SurviveThePlanet.uproject'),'-game','-nullrhi','-nosound','-unattended',
         '-benchmark','-seconds=10','-abslog='+str(log)]
process=subprocess.Popen(command,cwd=project)
try:
    code=process.wait(timeout=110)
except subprocess.TimeoutExpired:
    process.terminate()
    process.wait(timeout=15)
    raise RuntimeError('Standalone smoke test exceeded its timeout')
text=log.read_text(encoding='utf-8',errors='replace')
assert code==0,('Exit code',code)
assert '37 authored clusters across 37 sectors' in text,'Runtime generation not reached'
assert 'L_PlanetClusters' in text,'Default map not loaded'
assert 'Fatal error:' not in text
result={'result':'passed','mode':'UnrealEditor -game -nullrhi','exit_code':code,'generated_sectors':37}
(project/'Saved'/'ClusterGameStandalone.json').write_text(json.dumps(result,indent=2))
print(json.dumps(result))
