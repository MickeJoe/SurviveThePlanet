from pathlib import Path
import numpy as np
from PIL import Image
root=Path('C:/UE5/SurviveThePlanet 5.8')
out=root/'ContentSource/Geothermal'
n=512
y,x=np.mgrid[0:n,0:n]/n
rng=np.random.default_rng(31871)
def noise():
 result=np.zeros((n,n))
 for frequency,weight in [(2,.4),(5,.22),(13,.15),(31,.1),(73,.07),(151,.035)]:
  for _ in range(5):
   a,b=rng.integers(-frequency,frequency+1,2)
   result+=weight/5*np.sin(2*np.pi*(a*x+b*y)+rng.uniform(0,2*np.pi))
 return result
def save(name,data):Image.fromarray(np.uint8(np.clip(data,0,1)*255)).save(out/(name+'.png'))
cloud=noise()
micro=noise()
scratch=(np.sin(x*2*np.pi*73+cloud*4)>.994).astype(float)
dirt=np.clip(.35-cloud*2,0,1)
value=np.clip(.86+cloud*.65-scratch*.18-dirt*.08,.5,1)
save('T_GeoSurface',np.stack([value,value,value],axis=-1))
save('T_GeoRoughness',np.clip(.58+cloud*.5+scratch*.22,.25,.92))
height=cloud*.6+micro*.4-scratch*.04
dx=(np.roll(height,1,1)-np.roll(height,-1,1))*3
dy=(np.roll(height,1,0)-np.roll(height,-1,0))*3
normal=np.stack([dx,dy,np.ones_like(dx)],axis=-1)
normal/=np.linalg.norm(normal,axis=-1,keepdims=True)
save('T_GeoNormal',normal*.5+.5)
rock=np.clip(.7+cloud*1.6+micro*.5,.3,1)
save('T_GeoRock',np.stack([rock,rock,rock],axis=-1))
# Soft, multi-lobed vapor, with turbulent density instead of sphere silhouettes.
px=x*2-1;py=y*2-1
density=np.zeros_like(x)
for cx,cy,size in [(-.22,-.10,.46),(.22,.08,.42),(-.06,.32,.36),(.12,-.30,.31),(0,0,.52)]:
 density+=np.exp(-((px-cx)**2+(py-cy)**2)/(size*size)*2)
edge=np.clip(1-px*px-py*py,0,1)**2
alpha=np.clip(density*.45*(.7+cloud*2.0+micro)*edge,0,.95)
save('T_GeoSteam',np.stack([np.ones_like(x)*.88,np.ones_like(x)*.92,np.ones_like(x)*.96,alpha],axis=-1))
print('Generated seamless surface, roughness, normal and vapor textures')
