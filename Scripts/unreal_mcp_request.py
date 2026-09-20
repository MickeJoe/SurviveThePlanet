"""Small client for Unreal's native local MCP server."""
import json
import sys
import urllib.request
import urllib.error
import socket

SESSION = None

def request(payload):
    global SESSION
    headers={'Content-Type':'application/json','Accept':'application/json, text/event-stream'}
    if SESSION: headers['Mcp-Session-Id']=SESSION
    if payload.get('method')=='tools/call':
        # UE 5.8 streams SSE after an initial Content-Length: 0 header.
        # Read the event stream directly rather than urllib's zero-length body.
        body=json.dumps(payload).encode()
        wire='POST /mcp HTTP/1.1\r\nHost: 127.0.0.1:8000\r\n'
        wire+=''.join(k+': '+v+'\r\n' for k,v in headers.items())
        wire+='Content-Length: '+str(len(body))+'\r\n\r\n'
        with socket.create_connection(('127.0.0.1',8000),timeout=60) as stream:
            stream.sendall(wire.encode()+body)
            received=''
            while True:
                chunk=stream.recv(65536)
                if not chunk: raise RuntimeError(received)
                received+=chunk.decode()
                for line in received.splitlines():
                    if line.startswith('data:'):
                        try: result=json.loads(line[5:].strip())
                        except ValueError: continue
                        if result.get('id')==payload['id']: return result
    req=urllib.request.Request('http://127.0.0.1:8000/mcp',json.dumps(payload).encode(),headers)
    try:
        with urllib.request.urlopen(req,timeout=120) as response:
            SESSION=response.headers.get('Mcp-Session-Id',SESSION)
            text=response.read().decode()
    except urllib.error.HTTPError as error:
        raise RuntimeError(error.read().decode()) from error
    if any(line.startswith('data:') for line in text.splitlines()):
        text=next(line[5:].strip() for line in text.splitlines() if line.startswith('data:'))
    if not text and 'id' not in payload:
        return {}
    try:
        return json.loads(text)
    except ValueError:
        raise RuntimeError(repr(text[:4000]))

request({'jsonrpc':'2.0','id':0,'method':'initialize','params':{'protocolVersion':'2024-11-05','capabilities':{},'clientInfo':{'name':'Codex','version':'1'}}})
request({'jsonrpc':'2.0','method':'notifications/initialized'})
payload={'jsonrpc':'2.0','id':1,'method':'tools/list','params':{}}
if len(sys.argv)>1:
    payload=json.loads(sys.argv[1])
def unpack(value):
    if isinstance(value, dict):
        if value.get('mimeType', '').startswith('image/') and 'data' in value:
            import base64
            from pathlib import Path
            path=Path(__file__).resolve().parent.parent/'Saved'/'ClusterPOC_MCP.png'
            path.write_bytes(base64.b64decode(value['data']))
            return {'image':str(path)}
        return {k:unpack(v) for k,v in value.items()}
    if isinstance(value,list): return [unpack(v) for v in value]
    if isinstance(value,str):
        try: return unpack(json.loads(value))
        except (ValueError,TypeError): pass
    return value
print(json.dumps(unpack(request(payload)),indent=2))
