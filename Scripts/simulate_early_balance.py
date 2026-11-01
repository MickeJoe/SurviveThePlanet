"""Fast economic model of landing -> uplink -> cargo -> one Mother Base request + one contract.
Uses live-exported assets and runtime JSON. No graphics, no inventory/resource cheats.
One tick = 5 simulation seconds = 5 game minutes; not a substitute for UE spatial playtests.
"""
from pathlib import Path
import argparse, collections, csv, hashlib, json, math, random, statistics, time

ROOT = Path(__file__).resolve().parents[1]
SNAP = json.loads((ROOT/'Balance/RuntimeSnapshot.json').read_text())
ASSETS = SNAP['assets']
MULTIPLIER = 3.0
# Native fallback recipes: the live ConcretePlant asset is a base BuildingDataAsset.
RECIPES = {'Concrete': ('Stone',4,20.0), 'Steel':('Iron',4,30.0), 'CopperMetal':('Copper',3,25.0)}
STYLES = {'careful':(0.95,8,0.05), 'expansion':(0.85,15,0.12), 'inexperienced':(0.65,35,0.25)}
def read(path): return json.loads((ROOT/path).read_text(encoding='utf-8-sig'))
def asset(name):
    return next(v for k,v in ASSETS.items() if k.split('.')[-1]==name)
COSTS = {name:{x['resource']:x['cost'] for x in asset(key)['ConstructionCosts']} for name,key in {
'IronMine':'DA_IronMiningQuerry','StoneMine':'DA_StoneMiningQuerry','CopperMine':'DA_CopparMiningQuerry',
'Workshop':'DA_CrudeWorkshop','Solar':'DA_STP_SolarArray','Wind':'DA_STP_WindGenerator',
'Cargo':'DA_CargoBay','Collector':'DA_WaterCollector','ConcretePlant':'DA_ConcretePlant','Battery':'DA_EnergyBatteryStorage'}.items()}
COSTS['Uplink']={'Iron':20,'Concrete':10}
PRICES={x['resource']:(x['buyPrice'],x['sellPrice'],x['startingStock']) for x in SNAP['trade']['trade']['Items']}
MINER_RATE = SNAP['drones']['miner']['data']['WorkingRates']['mining'] * 10 * MULTIPLIER / 60
BUILDER_RATE = SNAP['drones']['builder']['data']['WorkingRates']['construction']

class Game:
    def __init__(self,seed,planet,style,objective,offer,tier,policy='objective_first',production_scale=1.0):
        self.rng=random.Random(seed); self.seed=seed; self.planet=planet
        self.data=ASSETS[planet]; self.style=style; self.eff,self.travel,self.mistake=STYLES[style]
        self.objective=objective; self.offer=offer; self.tier=tier; self.policy=policy; self.production_scale=production_scale
        initial=SNAP['world']['ResourceManager'][0]['props']['InitialResourceAmounts']
        self.r=collections.defaultdict(float,{k[0].upper()+k[1:]:float(v) for k,v in initial.items()})
        self.buildings=set(); self.miners={'Iron':False,'Stone':False,'Copper':False}
        self.deposits={r:self.rng.randint(1800,4000) for r in self.miners}
        self.t=0; self.conf=100.; self.progress=0; self.credits=self.data['StartingCredits']
        self.energy=initial['energy']; self.capacity=1000.; self.blackouts=0
        self.builders=2; self.pending=collections.defaultdict(float); self.work=collections.defaultdict(float)
        self.uplink=None; self.cargo=None; self.objective_at=None; self.contract_at=None
        self.shipments=0; self.missed=0; self.deadline=None; self.accepted=None; self.stock={}
        self.buy_spend=0; self.log=[]; self.rain=0.; self.target_rain=0.; self.weather_at=0.
        self.idle_until=0.; self.mining='Iron'; self.recipe=None; self.selected_recipe='Concrete'; self.pending_build=None
        self.ready_at=0.; self.trace=[]; self.min_conf=100.
    def event(self,text):
        self.log.append({'seconds':self.t,'event':text,'confidence':round(self.conf,2),'energy':round(self.energy,1)})
    def affordable(self,costs):
        return all(math.floor(self.r[k]+1e-8)>=v for k,v in costs.items())
    def spend(self,costs):
        assert self.affordable(costs)
        for k,v in costs.items():self.r[k]-=v
    def supply(self,res,target):
        """Choose a real resource operation; production consumes actual inputs on completion."""
        if math.floor(self.r[res]+1e-8)>=target:return True
        if res in self.miners:
            if not self.miners[res]: return self.build(res+'Mine')
            self.mining=res
            return False
        if res in RECIPES:
            raw,n,cycle=RECIPES[res]
            if not self.supply(raw,n):return False
            self.recipe=res; self.mining=raw
            return False
        return False
    def build(self,name):
        if name in self.buildings:return True
        if self.pending_build:return False
        for res,n in COSTS[name].items():
            if not self.supply(res,n):return False
        self.spend(COSTS[name]); self.pending_build=name
        self.ready_at=self.t+self.travel*self.rng.uniform(0.7,1.5)+1/(self.builders*BUILDER_RATE)
        self.recipe=None
        return False
    def finish_build(self):
        name=self.pending_build; self.pending_build=None; self.buildings.add(name)
        if name.endswith('Mine'):self.miners[name[:-4]]=True
        if name=='Uplink':
            self.uplink=self.t; self.builders+=1; self.conf=min(100,self.conf+40)
            uplink=next(o for o in read('Config/Objectives.json')['objectives'] if o['id']=='mission_build_communication')
            for reward in uplink['rewards']:
                if reward['type']=='give_resource':self.r[reward['resource']]+=reward['amount']
            self.progress+=1; self.event('uplink complete')
        if name=='Cargo':
            self.cargo=self.t;self.progress+=1;self.event('cargo complete; Mother Base request assigned')
        if name=='Battery':self.capacity+=500
        self.event('built '+name)
    def effects(self,effects):
        for e in effects:
            if e['type']=='credits':self.credits+=e['amount']
            elif e['type']=='resource':self.r[e['id']]+=e['amount']
            elif e['type']=='missionConfidence':self.conf=max(0,min(100,self.conf+e['amount']))
            elif e['type']=='missionProgress':self.progress+=e['amount']
    def request_done(self):
        return self.objective_at is not None
    def objective_target(self):
        c=self.objective['conditions'][0];return c['resource'],c['amount']
    def deliver_objective(self):
        res,n=self.objective_target()
        if not self.request_done() and self.affordable({res:n}):
            reserve=self.tier['deliveryAmount'] if self.accepted is not None and self.contract_at is None and self.offer['itemId']==res and self.policy=='contract_first' else 0
            if not self.affordable({res:n+reserve}):return
            self.spend({res:n})
            for e in self.objective['rewards']:
                if e['type']=='give_resource':self.r[e['resource']]+=e['amount']
                elif e['type']=='give_mission_confidence':self.conf=min(100,self.conf+e['amount'])
                elif e['type']=='give_mission_progress':self.progress+=e['amount']
            self.objective_at=self.t;self.event('Mother Base delivery complete')
    def merchant(self):
        if self.cargo is None:return
        schedule=SNAP['trade'][self.planet]['MerchantVisits'];elapsed=self.t-self.cargo
        cycle=schedule['repeatCycleDays']*1440
        for i,v in enumerate(schedule['visits']):
            start=math.floor(elapsed/cycle)*cycle+v['arrivalAfterHours']*60
            if not start<=elapsed<start+v['stayHours']*60:continue
            # Only emergency inputs. No unlimited stock or free purchases.
            res,n=self.objective_target()
            if self.request_done():res,n=self.offer['itemId'],self.tier['deliveryAmount']
            merchant=v['merchant']['refPath'].split('.')[-1].replace('DA_','')
            offered=SNAP['merchants'][merchant]['OfferedItemIds']
            if ('resource_'+res) not in offered or res not in PRICES or self.policy=='no_trade':return
            buy,sell,stock=PRICES[res]
            available=self.stock.setdefault((i,res),stock)
            need=max(0,n-math.floor(self.r[res]))
            qty=min(need,available,max(0,(self.credits-300)//buy))
            if qty:
                self.r[res]+=qty;self.credits-=qty*buy;self.buy_spend+=qty*buy;self.stock[(i,res)]-=qty
            return
    def plan(self):
        self.recipe=None
        if self.pending_build:return
        if self.t<self.idle_until:return
        if self.rng.random()<self.mistake*0.015:
            self.idle_until=self.t+self.rng.uniform(20,70);return
        for name in ['IronMine','Solar','Workshop','StoneMine']:
            if not self.build(name):return
        if self.uplink is None:
            self.build('Uplink');return
        # At most two generators needed in this bounded early-game model.
        if 'Wind' not in self.buildings:
            self.build('Wind');return
        if self.cargo is None:
            self.build('Cargo');return
        target_res,target_n=self.objective_target()
        if self.style=='expansion' and target_res=='Concrete' and not self.request_done():
            for name in ['Collector','ConcretePlant']:
                if not self.build(name):return
        self.deliver_objective()
        # Only accept after a real stock threshold, matching faction eligibility.
        if self.accepted is None and (self.request_done() or self.policy=='contract_first') and self.r[self.offer['itemId']]>=self.tier['deliveryAmount']:
            self.accepted=self.t; self.deadline=self.t+self.tier['deliveryIntervalHours']*60
            self.event('contract accepted')
        if self.policy=='contract_first' and self.accepted is not None and self.contract_at is None and not self.affordable({self.offer['itemId']:self.tier['deliveryAmount']}):
            self.supply(self.offer['itemId'],self.tier['deliveryAmount'])
        elif not self.request_done():
            res,n=self.objective_target()
            if self.policy=='contract_first' and self.accepted is not None and self.contract_at is None and self.offer['itemId']==res:
                n+=self.tier['deliveryAmount']
            self.supply(res,n)
        elif self.contract_at is None:
            self.supply(self.offer['itemId'],self.tier['deliveryAmount'])
    def tick(self,dt=5):
        self.plan()
        if self.recipe is not None and self.recipe!=self.selected_recipe:
            self.work.clear()  # Runtime SetRecipe discards unfinished cycles.
            self.selected_recipe=self.recipe
        if self.pending_build and self.t>=self.ready_at:self.finish_build()
        # Rain follows source transition/hold durations; energy is fixed authored generation in current C++.
        w=self.data['Weather'];phase=w['transitionDuration']+w['stableDuration']
        if self.t>=self.weather_at:
            self.previous_rain=self.rain
            pr=w['precipitationRange']
            self.target_rain=self.rng.uniform(pr['minimum'],pr['maximum']) if self.rng.random()<self.data['RainProbability'] else 0.
            self.weather_start=self.t;self.weather_at=self.t+phase
        alpha=min(1,(self.t-self.weather_start)/w['transitionDuration'])
        self.rain=self.previous_rain+(self.target_rain-self.previous_rain)*alpha
        generation=(35 if 'Solar' in self.buildings else 0)+(50 if 'Wind' in self.buildings else 0)
        demand=(10 if self.miners.get(self.mining) else 0)+(6*[1,1,1.8,2.4][min(3,self.builders)] if 'Workshop' in self.buildings else 0)+(5 if self.uplink is not None else 0)+(3 if self.cargo is not None else 0)+(5 if 'Collector' in self.buildings and self.rain>0 else 0)+(10 if 'ConcretePlant' in self.buildings else 0)
        powered=demand<=generation or self.energy>=1.
        self.energy=max(0,min(self.capacity,self.energy+(generation-(demand if powered else 0))*dt/60))
        if not powered:self.blackouts+=dt
        if powered:
            if self.miners.get(self.mining) and self.deposits[self.mining]>0:
                amount=min(self.deposits[self.mining],MINER_RATE*self.eff*self.production_scale*dt)
                self.deposits[self.mining]-=amount;self.r[self.mining]+=amount
            if 'Collector' in self.buildings:
                self.r['Water']+=self.rain*3*dt/60
            if 'ConcretePlant' in self.buildings:
                self.pending['plant']+=dt*self.production_scale
                while self.pending['plant']>=20 and self.affordable({'Stone':4,'Water':2}):
                    self.spend({'Stone':4,'Water':2});self.r['Concrete']+=3;self.pending['plant']-=20
                self.pending['plant']=min(20,self.pending['plant'])
            if self.recipe and not self.pending_build and 'Workshop' in self.buildings:
                raw,n,cycle=RECIPES[self.recipe]
                self.work[self.recipe]+=dt*self.builders*BUILDER_RATE*self.eff*self.production_scale
                while self.work[self.recipe]>=cycle and self.affordable({raw:n}):
                    self.spend({raw:n});self.r[self.recipe]+=1;self.work[self.recipe]-=cycle
        if self.cargo is not None:
            self.merchant()
            if self.accepted is not None and self.contract_at is None and self.t>=self.deadline:
                res=self.offer['itemId'];n=self.tier['deliveryAmount']
                if self.affordable({res:n}):
                    self.spend({res:n});self.shipments+=1;self.effects(self.tier['deliveryRewards'])
                else:
                    self.missed+=1;self.effects(self.tier['missedDeliveryPenalties'])
                self.deadline=self.t+self.tier['deliveryIntervalHours']*60
                if self.shipments+self.missed>=self.tier['deliveryCount']:
                    self.contract_at=self.t
                    if self.missed==0:self.effects(self.tier['completionBonus'])
                    self.event('contract resolved')
        self.conf=max(0,self.conf-self.data['MissionConfidence']['decayPerGameHour']*dt/60)
        self.min_conf=min(self.min_conf,self.conf);self.t+=dt
        assert self.energy>=0 and self.credits>=0 and all(v>=-1e-7 for v in self.r.values())
    def run(self):
        while self.t<7200 and self.conf>0 and not (self.request_done() and self.contract_at is not None):
            self.tick()
        return dict(seed=self.seed,planet=self.planet,style=self.style,policy=self.policy,objective=self.objective['id'],
                    contract=self.offer['id'],tier=self.tier['id'],cargo_seconds=self.cargo,objective_seconds=self.objective_at,
                    objective_after_cargo=None if self.objective_at is None else self.objective_at-self.cargo,
                    success=self.request_done() and self.contract_at is not None and self.missed==0 and self.conf>0,
                    objective_complete=self.request_done(),missed=self.missed,shipments=self.shipments,elapsed=self.t,
                    confidence=round(self.conf,2),min_confidence=round(self.min_conf,2),credits=self.credits,
                    trade_spend=self.buy_spend,blackout_seconds=self.blackouts)

def summary(rows):
    result={}
    for field in ['style','planet','objective','tier','contract','policy']:
        result[field]={}
        for value in sorted({r[field] for r in rows}):
            group=[r for r in rows if r[field]==value]
            durations=[r['objective_after_cargo']/60 for r in group if r['objective_after_cargo'] is not None]
            result[field][value]={'runs':len(group),'success_pct':round(100*sum(r['success'] for r in group)/len(group),1),
              'objective_complete_pct':round(100*sum(r['objective_complete'] for r in group)/len(group),1),
              'median_objective_minutes':round(statistics.median(durations),2) if durations else None,
              'median_end_confidence':round(statistics.median(r['confidence'] for r in group),1),
              'missed_shipments':sum(r['missed'] for r in group)}
    return result

def main():
    raise SystemExit('Historical five-request profile is superseded. Run Scripts/simulate_cargo_packages.py for the active A/B configuration.')

if __name__ == '__main__':
    main()
