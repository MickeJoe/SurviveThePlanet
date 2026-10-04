# Power connection assets — first asset step
Created in Blender 5.2 through native MCP; imported into Unreal Engine 5.8 through native MCP.
Source: PowerConnection.blend. Generator: Scripts/create_power_connection_blender.py.
Unreal folder: /Game/Units/Buildings/PowerConnection

## Meshes
- SM_PowerPole: 74 x 116 x 330.5 cm; 3176 triangles. Pivot at center of foundation bottom (Z=0).
- SM_PowerCableSpan: straight double 100 cm cable span along +X; 388 triangles. Use SplineMeshComponent ForwardAxis X; preserve Y/Z scale to retain cable thickness. Centerline starts (0,0,0), ends (100,0,0); two cables at Y +/-46 cm. Cable centerline coincides with pole clamp centers at local Z=327 cm.
- SM_PowerCableTerminal: 36 x 38 x 53 cm; 1076 triangles. Pivot at mounting foot bottom. Terminal clamp centers local (0,+/-11.5,51) cm. Needs a short adapting lead to the wider pole pair.

Five shared materials: Power_Ivory, Power_Orange, Power_Graphite, Power_Steel, Power_Cyan. Cyan base color also feeds emissive in Unreal; separated material slots support later animation.
No simple collisions on these decorative assets. Future components must explicitly disable collision and CanEverAffectNavigation; mesh import alone does not define component navigation behavior.

## Placement and cables — proposed next implementation
Start with pole spacing around 600 cm. Divide route into equal intervals so there is no tiny final gap.
Use terrain traces for each foundation; the mesh already has ground pivot. Do not copy the extender's -130 cm offset into poles.
Align each crossarm perpendicular to the route. A shallow sag, approximately 25-40 cm per 6m span, should follow a curve subdivided into multiple spline mesh pieces, rather than stretching a straight cable horizontally.
Attach endpoints to explicit roof/mast terminals; avoid routing through rocks. Reuse existing placement/network architecture when implementing.
Automatic decorative preview and placed connections are now implemented; see Implementation.md. Cable costs, obstacle detours, power rules and animation are not part of this implementation.

## Suggested animation / VFX (not implemented)
1. Wind sway: gentle 1-3 cm displacement in cable midpoints, fixed attachment points. Slow, varied phase per span; amplitude driven by existing wind. Prefer controlled spline motion/material displacement over physics simulation.
2. Status LEDs: low cyan emissive with a soft 2-3 second pulse. Offset phases per pole so the line does not blink in unison.
3. Activation: one short cyan pulse travels from Base Camp toward extender when the link becomes active; optional soft electrical click/hum.
4. Load feedback: slightly increase cyan intensity with load. Reserve amber blink for overload/disconnection.
5. Fault sparks: tiny, rare bursts only on actual damage/failure, not continuously during normal operation.

Recommended first animation pass: wind sway + status LEDs + one activation pulse. Keep bloom low and avoid bright permanent lightning beams.
