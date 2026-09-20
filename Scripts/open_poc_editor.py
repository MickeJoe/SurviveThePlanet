"""Launch the correct project without nested-shell quoting of Windows paths."""
from pathlib import Path
import subprocess

project=Path(__file__).resolve().parents[1]
assert str(project).lower()==r'C:\UE5\SurviveThePlanet 5.8'.lower()
process=subprocess.Popen([
    r'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe',
    str(project/'SurviveThePlanet.uproject'),
    '-EnablePlugins=PythonScriptPlugin',
    '/Game/WorldGeneration/SectorTemplates/ST_First/L_SectorTemplate_First',
    '-nop4','-nosplash',
],cwd=project)
print('Opened canonical POC editor PID',process.pid)
