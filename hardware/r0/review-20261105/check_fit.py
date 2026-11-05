import json,struct,math
from pathlib import Path
P=Path(__file__).resolve().parent
ASSET=next(d for parent in [P.parent,*P.parents] for d in [parent/'开源模型',parent/'original-project/开源模型'] if d.exists())
tris=[]
for name in ['枪体壳1.STL','枪体壳2.STL','枪体壳3.STL']:
 b=(ASSET/name).read_bytes();n=struct.unpack_from('<I',b,80)[0]
 for j in range(n):tris.append((name,[struct.unpack_from('<3f',b,84+50*j+12+12*k) for k in range(3)]))
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def cross(a,b):return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]
def hit(t,lo,hi):
 c=[(a+b)/2 for a,b in zip(lo,hi)];h=[(b-a)/2 for a,b in zip(lo,hi)];v=[[p[i]-c[i] for i in range(3)] for p in t];e=[[v[(j+1)%3][i]-v[j][i] for i in range(3)] for j in range(3)];axes=[[1,0,0],[0,1,0],[0,0,1],cross(e[0],e[1])]+[cross(a,b) for a in e for b in [[1,0,0],[0,1,0],[0,0,1]]]
 for a in axes:
  if dot(a,a)<1e-18:continue
  ds=[dot(a,p) for p in v];r=sum(abs(a[i])*h[i] for i in range(3))
  if min(ds)>r+1e-7 or max(ds)<-r-1e-7:return False
 return True
out=[]
for z in range(90,161,5):
 lo=[13.4,1,z];hi=[15,57,z+80];hits={}
 for n,t in tris:
  if hit(t,lo,hi):hits[n]=hits.get(n,0)+1
 out.append({'bare_board_candidate_bounds_mm':[lo,hi],'intersecting_shell_triangles':hits,'total':sum(hits.values())})
r={'method':'triangle-box SAT, 13 separating axes; rectangular bare PCB envelope 56x80x1.6 mm','units':'mm assumed for original STL','confidence':'candidate placement scan only','tested_candidates':out,'complete_fit_validation':False,'limitations':['Does not model rounded PCB corners or exact mounted parts','Does not prove cavity containment or evaluate arbitrary rotations','No full mesh-to-mesh collision, wall-thickness or assembly path test']};(P/'候选装配碰撞扫描.json').write_text(json.dumps(r,ensure_ascii=False,indent=2));print([(x['bare_board_candidate_bounds_mm'][0][2],x['total']) for x in out])
