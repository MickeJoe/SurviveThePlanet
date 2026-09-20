"""Launch the canonical project using its configured startup map."""
from pathlib import Path
import subprocess
project=Path(__file__).resolve().parents[1]
assert str(project).lower()==r'C:\UE5\SurviveThePlanet 5.8'.lower()
process=subprocess.Popen([
    r'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe',
    str(project/'SurviveThePlanet.uproject'),
    '-EnablePlugins=PythonScriptPlugin','-nop4','-nosplash',
],cwd=project)
print('Opened canonical editor with configured default map, PID',process.pid)
