import json,base64,io,os
from PIL import Image,ImageChops,ImageStat
temp=os.environ['TEMP']
report={}
for kind,filename in [('Bloom','stp09_frames.json'),('Wind','stp09_wind_frames.json')]:
    data=json.load(open(os.path.join(temp,filename),encoding='utf-8-sig'))
    result=json.loads(data['returnValue']);rows=result['frames'];frames=[]
    for row in rows:
        frames.append(Image.open(io.BytesIO(base64.b64decode(row['image']['data']))).convert('RGB'))
    durations=[max(100,round((rows[i+1]['time']-rows[i]['time'])*1000)) for i in range(len(rows)-1)]
    durations.append(durations[-1])
    path=os.path.join(temp,'STP09_'+kind+'.gif')
    frames[0].save(path,save_all=True,append_images=frames[1:],duration=durations,loop=0,optimize=False)
    diffs=[]
    for i in range(1,len(frames)):
        d=ImageChops.difference(frames[0],frames[i]);diffs.append({'frame':i,'mean_difference':sum(ImageStat.Stat(d).mean)/3,'changed_bounds':d.getbbox()})
    report[kind]={'frames':len(frames),'seconds':rows[-1]['time']-rows[0]['time'],'differences':diffs}
    frames[0].save(os.path.join(temp,'STP09_'+kind+'_First.png'))
    frames[5].save(os.path.join(temp,'STP09_'+kind+'_Later.png'))
json.dump(report,open(os.path.join(temp,'STP09_VisualCheck.json'),'w'),indent=2)
print(json.dumps(report))
