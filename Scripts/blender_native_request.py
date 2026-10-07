import json, socket, sys
from pathlib import Path
code=Path(sys.argv[1]).read_text(encoding="utf-8-sig") if len(sys.argv)>1 else "import bpy\nresult={'scene':bpy.context.scene.name,'file':bpy.data.filepath,'objects':len(bpy.data.objects)}"
with socket.create_connection(("127.0.0.1",9876),timeout=120) as s:
    s.sendall(json.dumps({"type":"execute","strict_json":True,"code":code}).encode()+b"\0")
    data=b""
    while b"\0" not in data:
        chunk=s.recv(65536)
        if not chunk: raise RuntimeError("Blender MCP disconnected")
        data+=chunk
    response=json.loads(data.split(b"\0")[0])
    print(json.dumps(response,ensure_ascii=False))
    if response.get("status")!="ok": sys.exit(1)
