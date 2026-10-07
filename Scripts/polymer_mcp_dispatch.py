import sys,json,runpy,io,contextlib
from pathlib import Path
args=sys.argv[1:];sys.argv=[str(Path(__file__).with_name("unreal_mcp_request.py"))]
with contextlib.redirect_stdout(io.StringIO()):api=runpy.run_path(sys.argv[0])
payload=json.loads(Path(args[0]).read_text(encoding="utf-8-sig"))
payloads=payload if isinstance(payload,list) else [payload]
for payload in payloads:
 result=api["request"](payload)
 for c in result.get("result",{}).get("content",[]):
  if c.get("type")=="text":
   try:c["text"]=json.loads(c["text"])
   except (ValueError,TypeError):pass

 def save_images(value):
  if isinstance(value,dict):
   if isinstance(value.get("mimeType"),str) and value["mimeType"].startswith("image/") and isinstance(value.get("data"),str):
    import base64
    path=Path(__file__).resolve().parents[1]/"Saved/PolymerViewport.png"
    path.write_bytes(base64.b64decode(value["data"]))
    return {"image":str(path)}
   if value.get("type")=="image" and isinstance(value.get("data"),str):
    import base64
    path=Path(__file__).resolve().parents[1]/"Saved/PolymerViewport.png"
    path.write_bytes(base64.b64decode(value["data"]))
    return {"image":str(path)}
   return {k:save_images(v) for k,v in value.items()}
  if isinstance(value,list):return [save_images(v) for v in value]
  return value
 result=save_images(result)
 Path(__file__).resolve().parents[1].joinpath("tmp/polymer_mcp_response.json").write_text(json.dumps(result,indent=2))
 if len(args)>1 and args[1]=="names":
  for c in result.get("result",{}).get("content",[]):
   t=c.get("text",{})
   if isinstance(t,dict):
    for tool in t.get("tools",[]):print(tool["name"])
 else:print(json.dumps(result,indent=2))

