from pathlib import Path
root=Path('C:/UE5/SurviveThePlanet 5.8')
exec(compile((root/'Scripts/test_geothermal_runtime.py').read_text(),str(root/'Scripts/test_geothermal_runtime.py'),'exec'),globals())
