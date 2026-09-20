"""Refresh the first-sector assets using the authoritative organic layout."""
from pathlib import Path
import runpy

runpy.run_path(str(Path(__file__).with_name("correct_first_sector.py")), run_name="__main__")
