import sys
from pathlib import Path
client=Path(r'C:\UE5\SurviveThePlanet 5.8\Scripts\blender_native_request.py').read_text(encoding='utf-8-sig')
exec(client.replace('timeout=120','timeout=600'))
