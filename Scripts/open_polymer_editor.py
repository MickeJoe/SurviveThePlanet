import subprocess
from pathlib import Path
root=Path(r"C:\UE5\SurviveThePlanet 5.8")
subprocess.Popen([r"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe",str(root/"SurviveThePlanet.uproject"),"-EnablePlugins=PythonScriptPlugin","-nop4","-nosplash","-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=True"],cwd=root)
print("Started canonical editor with native MCP enabled")

