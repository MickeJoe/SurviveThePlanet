"""Summarize Unreal CSV captures without loading editor assets."""
import csv
import json
import statistics
import sys
from pathlib import Path

project = Path(__file__).resolve().parents[1]
paths = [argument for argument in sys.argv[1:] if not argument.startswith('--')]
path = Path(paths[0]) if paths else max((project/'Saved'/'Profiling'/'CSV').glob('*.csv'), key=lambda p:p.stat().st_mtime)
with path.open(newline='', encoding='utf-8-sig') as stream:
    rows = list(csv.DictReader(stream))
if '--traversal' in sys.argv:
    # CSV starts mid-frame, after the test's initial transform-hash verification.
    # Exclude only that setup frame; retain all traversal/streaming/GC frames.
    rows = rows[1:]
result = {}
for key in rows[0]:
    if key and any(part in key for part in ('FrameTime', 'GPUTime', 'GameThreadTime', 'RenderThreadTime', 'GPUMem/',
                                           'RHI/DrawCalls', 'RHI/Primitives', 'GPUSceneInstanceCount', 'TextureStreaming/')):
        values = []
        for row in rows:
            try:
                values.append(float(row[key]))
            except (ValueError, TypeError):
                pass
        if values:
            result[key] = {'median': statistics.median(values), 'mean': statistics.mean(values),
                           'p95': sorted(values)[int(len(values)*0.95)], 'max': max(values)}
phases = []
elapsed = 0
for phase in range(0 if '--traversal' in sys.argv else 3):
    selected = []
    elapsed = 0
    for row in rows:
        try:
            elapsed += float(row['FrameTime']) / 1000
        except (ValueError, TypeError):
            continue
        if 8 + 24 * phase <= elapsed < 24 + 24 * phase:
            selected.append(row)
    stats = {}
    for key in ('FrameTime', 'GPUTime', 'GameThreadTime', 'RenderThreadTime', 'RHI/DrawCalls', 'GPUMem/LocalUsedMB'):
        values = []
        for row in selected:
            try:
                values.append(float(row[key]))
            except (KeyError, ValueError, TypeError):
                pass
        if values:
            stats[key] = {'median': statistics.median(values), 'mean': statistics.mean(values),
                          'p95': sorted(values)[int(len(values) * 0.95)]}
    phases.append({'discovered': [1, 7, 37][phase], 'stats': stats})
print(json.dumps({'file': str(path), 'frames': len(rows), 'stats': result, 'phases': phases}, indent=2))
