"""Show all three editable POC compositions with authoring lighting."""
import runpy
from pathlib import Path
runpy.run_path(str(Path(__file__).with_name('show_cluster_variant_poc.py')),init_globals={'AUTHOR_VARIANTS':True})
