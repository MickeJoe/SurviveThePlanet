"""Headless economic stress test. Reuses the landing model; no UI or free finished goods.
Construction, miners, builder allocation, atomic recipes, inputs, power, finite coal,
confidence, contracts and skipped requests are modelled. Spatial feasibility remains a UE playtest.
"""
from pathlib import Path
import collections,itertools,json,random,statistics,hashlib,time,csv
from simulate_early_balance import Game,COSTS,MINER_RATE,BUILDER_RATE,STYLES,read
ROOT=Path(__file__).resolve().parents[1]
CFG=read('Config/Objectives.json'); PROD=CFG['cargoProduction']
DEFS={o['id']:o for o in CFG['objectives']}
for name,s in PROD.items():COSTS[name]=s['costs']
COSTS['SilicaMine']=COSTS['SilicaExtractor']
OFFERS=read('Config/ContractOffers.json')['contractOffers']
class PackageGame(Game):
 def __init__(self,seed,planet,style,package,order,skip=None,scale=1.,delay=0,trade=False):
  offer=OFFERS[seed%len(OFFERS)];tier=offer['tiers'][seed%3]
  super().__init__(seed,planet,style,DEFS[order[0]],offer,tier,'objective_first' if trade else 'no_trade',scale)
  self.package=package;self.order=order;self.skip=skip;self.delay=delay
  self.required=[x for x in order if x!=skip];self.done={};self.plant=None;self.pump=False
  self.miners['Silica']=False;self.deposits['Silica']=self.rng.randint(1800,4000)
  self.new_work=collections.defaultdict(float);self.low_power=0
 def finish_build(self):
  name=self.pending_build;super().finish_build()
  if name=='Cargo':
   for e in DEFS['mission_build_cargo_bay']['rewards']:
    if e['type']=='give_resource':self.r[e['resource']]+=e['amount']
 def request_done(self):return all(x in self.done for x in self.required)
 def objective_target(self):
  oid=next((x for x in self.required if x not in self.done),self.order[0]);c=DEFS[oid]['conditions'][0]
  return c['resource'],c['amount']
 def deliver_objective(self):
  for oid in self.required:
   c=DEFS[oid]['conditions'][0]
   if oid not in self.done and self.affordable({c['resource']:c['amount']}):
    self.spend({c['resource']:c['amount']});self.done[oid]=self.t
    for e in DEFS[oid]['rewards']:
     if e['type']=='give_mission_confidence':self.conf=min(100,self.conf+e['amount'])
     elif e['type']=='give_resource':self.r[e['resource']]+=e['amount']
     elif e['type']=='give_mission_progress':self.progress+=e['amount']
    self.event('delivered '+oid)
 def supply(self,res,target):
  if self.r[res]>=target:return True
  if res=='Water':
   if not self.build('GroundwaterPump'):return False
   self.pump=True;return False
  return super().supply(res,target)
 def plan(self):
  self.plant=None;self.pump=False
  if self.cargo is None:
   return super().plan()
  self.recipe=None
  if self.pending_build or self.t<self.idle_until or self.t<self.cargo+self.delay:return
  if self.rng.random()<self.mistake*.015:
   self.idle_until=self.t+self.rng.uniform(20,70);return
  if self.skip=='ALL':return
  self.deliver_objective()
  if self.request_done():
   if self.accepted is None:
    if not self.supply(self.offer['itemId'],self.tier['deliveryAmount']):return
    self.accepted=self.t;self.deadline=self.t+self.tier['deliveryIntervalHours']*60
   if self.contract_at is None:self.supply(self.offer['itemId'],self.tier['deliveryAmount'])
   return
  res,target=self.objective_target();name=next(k for k,v in PROD.items() if v.get('output')==res)
  if not self.build(name):return
  settings=PROD[name]
  # Collect enough inputs for the entire current shipment, then allocate builders to processing.
  batches=-(-(target-int(self.r[res]))//settings['outputAmount'])
  for raw,n in settings['inputs'].items():
   if not self.supply(raw,n*batches):return
  self.plant=name
 def tick(self,dt=5):
  super().tick(dt)
  if self.cargo is None:return
  extra=sum(PROD[x]['energy'] for x in PROD if x in self.buildings)
  self.energy=max(0,self.energy-extra*dt/60)
  if self.energy<1:self.low_power+=dt;return
  name='GroundwaterPump' if self.pump else self.plant
  if not name or self.pending_build:return
  s=PROD[name];eff=(self.builders*BUILDER_RATE*self.eff if s.get('drone') else 1.)
  duration=s['cycleSeconds']/3
  if not self.affordable(s['inputs']):return
  self.new_work[name]+=dt*eff*self.production_scale
  while self.new_work[name]>=duration and self.affordable(s['inputs']):
   self.spend(s['inputs']);self.r[s['output']]+=s['outputAmount'];self.new_work[name]-=duration
 def run(self):
  while self.t<7200 and self.conf>0 and not (self.request_done() and self.contract_at is not None):self.tick()
  return dict(package=self.package,planet=self.planet,style=self.style,seed=self.seed,skip=self.skip or '',scale=self.production_scale,
   delay=self.delay,trade=self.policy!='no_trade',order=','.join(self.order),completed=len(self.done),expected=len(self.required),
   success=self.skip!='ALL' and self.request_done() and self.contract_at is not None and self.missed==0 and self.conf>0,
   cargo_seconds=self.cargo,after_cargo_seconds=(max(self.done.values())-self.cargo if self.done else None),
   confidence=round(self.conf,2),low_power_seconds=self.low_power,missed=self.missed,elapsed=self.t,
   retained_requests=3-len(self.done),progress=self.progress)
def main():
 rows=[];start=time.time()
 for p in CFG['cargoPackages']:
  for planet,style,order,seed in itertools.product(['Nexaris','Aridus','Borealis'],STYLES,itertools.permutations(p['objectives']),range(10)):
   rows.append(PackageGame(seed+1000,planet,style,p['id'],order).run())
  for planet,skip,seed in itertools.product(['Nexaris','Aridus','Borealis'],p['objectives']+['ALL'],range(20)):
   rows.append(PackageGame(seed+2000,planet,'inexperienced',p['id'],p['objectives'],skip=skip).run())
  for planet,scale,delay,trade,seed in itertools.product(['Nexaris','Aridus','Borealis'],[.5,1.],[0,600],[False,True],range(5)):
   rows.append(PackageGame(seed+3000,planet,'inexperienced',p['id'],p['objectives'],scale=scale,delay=delay,trade=trade).run())
 dest=ROOT/'Balance/cargo_packages';dest.mkdir(parents=True,exist_ok=True)
 with (dest/'runs.csv').open('w',newline='') as f:
  writer=csv.DictWriter(f,fieldnames=rows[0]);writer.writeheader();writer.writerows(rows)
 groups={}
 for p in ['A','B']:
  for mode in ['normal','skip_one','no_deliveries','stress']:
   rs=[r for r in rows if r['package']==p and (r['seed']<2000 if mode=='normal' else 2000<=r['seed']<3000 and r['skip']!='ALL' if mode=='skip_one' else r['skip']=='ALL' if mode=='no_deliveries' else r['seed']>=3000)]
   durations=[r['after_cargo_seconds']/60 for r in rs if r['after_cargo_seconds'] is not None]
   groups[p+'_'+mode]={'runs':len(rs),'stage_complete_pct':round(100*sum(r['completed']==r['expected'] and r['skip']!='ALL' for r in rs)/len(rs),1),'stage_and_following_contract_success_pct':round(100*sum(r['success'] for r in rs)/len(rs),1),'median_delivery_minutes_after_cargo':round(statistics.median(durations),2) if durations else None,'minimum_confidence':min(r['confidence'] for r in rs),'max_missed_shipments':max(r['missed'] for r in rs),'max_retained_requests':max(r['retained_requests'] for r in rs)}
 result={'runs':len(rows),'wall_seconds':round(time.time()-start,2),'summary':groups,'model_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'config_sha256':hashlib.sha256((ROOT/'Config/Objectives.json').read_bytes()).hexdigest(),'limits':['Economic model, not UE collision/pathfinding simulation.','Builder and mining travel estimated from existing styles.','Fixed authored generator power; renewable/weather power interaction needs runtime playtest.','No recurring objectives; skipped requests are retained.','Completing this stage is not the whole mission: contracts provide remaining progression.']}
 files=['Scripts/simulate_cargo_packages.py','Scripts/simulate_early_balance.py','Balance/RuntimeSnapshot.json','Config/Objectives.json','Config/ContractOffers.json','Config/Traders.json','Source/SurviveThePlanet/Gameplay/Buildings/ProductionBalance.h','Source/SurviveThePlanet/Gameplay/Buildings/ThermalProcessingBuilding.cpp','Balance/CargoPackageAssets.json']
 result['input_sha256']={f:hashlib.sha256((ROOT/f).read_bytes()).hexdigest() for f in files}
 (dest/'summary.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
if __name__=='__main__':main()
