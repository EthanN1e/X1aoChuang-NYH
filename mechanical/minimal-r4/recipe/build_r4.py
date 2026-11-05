from pathlib import Path
import sys,json,shutil,hashlib
S=Path(r'C:\Users\32561\.codex\skills\solidworks-automation-skill');sys.path.insert(0,str(S/'scripts'));sys.path.insert(0,str(S/'apps/desktop'))
from sw_connect import connect_solidworks,new_document,open_document,save_document,get_com_member as m,create_empty_dispatch_variant
from sw_part import sketch,sketch_rectangle,sketch_circle,extrude_boss,extrude_cut,_ensure_sketch_selected
from sw_sketch_quality import fully_define_sketch,inspect_model_sketches
from sw_assembly import add_component,create_math_transform,_activate_model_document,get_interference_detection
from sw_appearance import set_document_appearance
from sw_export import export_to_step,export_to_stl
from sw_review import save_preview
from cad_workbench.engineering_orchestrator import build_engineering_plan
from sw_import_mesh_reference import configure_mesh_import_preferences
out=Path('outputs/R4-Minimal').resolve();out.mkdir(exist_ok=True)
src=Path(r'C:\Users\32561\Downloads\开源模型');old=Path('outputs/Base-R3').resolve();tpl=Path(r'C:\ProgramData\SOLIDWORKS\SOLIDWORKS 2025\templates')
e={'source':'Downloads/开源模型 and BV1of8y6vEQ5, assembly frames 70-85 s','battery_user_mm':[76,36,18],'parts':{},'source_hashes':{}}
sw,_=connect_solidworks(visible=True)
for kind in (2,1):
 for d in list(m(sw,'GetDocuments') or []):
  if str(Path('outputs').resolve()).lower() in str(m(d,'GetPathName')).lower() and m(d,'GetType')==kind:sw.CloseDoc(m(d,'GetTitle'))
for i in range(1,5):
 p=src/f'底座{i}.STL';q=Path('X1aoChuang-NYH/original-project/开源模型')/p.name
 assert p.read_bytes()==q.read_bytes();e['source_hashes'][p.name]=hashlib.sha256(p.read_bytes()).hexdigest()
 shutil.copy2(old/f'Original-Base{i}.SLDPRT',out/f'R4-Base{i}.SLDPRT')
shutil.copy2(old/'PCB-Latest-OBJ-Reference.SLDPRT',out/'R4-PCB-OBJ.SLDPRT')
sw,_=connect_solidworks(visible=True)
for kind in (2,1):
 for d in list(m(sw,'GetDocuments') or []):
  if str(Path('outputs').resolve()).lower() in str(m(d,'GetPathName')).lower() and m(d,'GetType')==kind:sw.CloseDoc(m(d,'GetTitle'))
def rect(d,w,h,depth,rev=False,inner=None,cx=0,cy=0,plane='Front Plane'):
 assert _activate_model_document(sw,d)
 assert m(m(sw,'ActiveDoc'),'GetTitle')==m(d,'GetTitle')
 with sketch(d,plane) as sk:
  d.SketchManager.AddToDB=True
  try:
   sketch_rectangle(d,cx/1000,cy/1000,w/1000,h/1000)
  finally:d.SketchManager.AddToDB=False
  fully_define_sketch(d)
 f=extrude_boss(d,sk,depth/1000,direction=rev);assert f is not None
 if inner:
  with sketch(d,plane) as cutsk:
   d.SketchManager.AddToDB=True
   try:sketch_rectangle(d,cx/1000,cy/1000,inner[0]/1000,inner[1]/1000)
   finally:d.SketchManager.AddToDB=False
   fully_define_sketch(d)
  assert extrude_cut(d,cutsk,0,both_directions=True) is not None
 return f
def finish(d,name):
 assert _activate_model_document(sw,d)
 assert d.ForceRebuild3(False);p=out/name;assert save_document(d,str(p));e['parts'][name]=inspect_model_sketches(d)
 for ext,func in [('step',export_to_step),('stl',export_to_stl)]:
  q=p.with_suffix('.'+ext);assert func(d,str(q));assert q.exists() and q.stat().st_size>0
 sw.CloseDoc(m(d,'GetTitle'))
# Only Base1 internal clearances change. All outside extents and other source parts stay intact.
configure_mesh_import_preferences(sw,units='mm',model_type='solid',as_ps_mesh=False,segmented=False)
from sw_preflight import import_com_dependencies
pythoncom,_,VARIANT=import_com_dependencies()
errors=VARIANT(pythoncom.VT_BYREF|pythoncom.VT_I4,0)
d=sw.LoadFile4(str(out/'Base1-local-edit-source.stl'),'r',create_empty_dispatch_variant(),errors);assert d is not None
before=[list(m(b,'GetBodyBox')) for b in m(d,'GetBodies2',0,False)]
e['base1_geometry_method']='Derived STL local booleans imported into SolidWorks. Cut parameters and untouched source hash are in local-change-evidence.json; original mesh is not claimed to have a recovered parameter tree.'
after=[list(m(b,'GetBodyBox')) for b in m(d,'GetBodies2',0,False)]
assert len(before)==len(after)==1 and all(abs(a-b)<1e-6 for a,b in zip(before[0],after[0])),(before,after)
e['base1_bounds_before']=before;e['base1_bounds_after']=after
finish(d,'R4-Base1.SLDPRT')
# Thin removable PCB support: frame contact only, no PCB drilling.
d=new_document(sw,'part',str(tpl/'Part.PRTDOT'));rect(d,80,56,2,False,(78.8,54.8))
with sketch(d) as sk:
 for x in [-38,38]:
  for y in [-26,26]:sketch_circle(d,x/1000,y/1000,.0017)
 fully_define_sketch(d)
assert extrude_boss(d,sk,.012715,direction=False) is not None
assert set_document_appearance(d,'silver');finish(d,'R4-PCB-Support-TRIAL.SLDPRT')
# Battery cradle within the existing upper bay, 0.5 mm candidate side clearances.
d=new_document(sw,'part',str(tpl/'Part.PRTDOT'));rect(d,79,39,18.8,False,(77,37))
with sketch(d) as sk:
 for y in [-19,19]:sketch_rectangle(d,0,y/1000,.008,.003)
 fully_define_sketch(d)
assert extrude_cut(d,sk,0,both_directions=True) is not None
assert set_document_appearance(d,'graphite');finish(d,'R4-Battery-Cradle-TRIAL.SLDPRT')
d=new_document(sw,'part',str(tpl/'Part.PRTDOT'));rect(d,76,36,18,True)
assert set_document_appearance(d,'arc_blue');assert save_document(d,str(out/'R4-Battery76x36x18-TEMP.SLDPRT'));sw.CloseDoc(m(d,'GetTitle'))
I=(1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0)
def pose(x,y,z):return (1,0,0,0,0,1,0,-1,0,x/1000,y/1000,z/1000,1,0,0,0)
items=[(out/f'R4-Base{i}.SLDPRT',I if i!=3 else (1,0,0,0,1,0,0,0,1,0,.001,0,1,0,0,0)) for i in range(1,5)]
items += [(out/'R4-PCB-OBJ.SLDPRT',(0,0,1,1,0,0,0,1,0,.12660254,.042,.0138,1,0,0,0)),(out/'R4-PCB-Support-TRIAL.SLDPRT',pose(86.60254,40.2,41.8)),(out/'R4-Battery-Cradle-TRIAL.SLDPRT',pose(86.60254,46.5,105)),(out/'R4-Battery76x36x18-TEMP.SLDPRT',pose(86.60254,28.5,105))]
asm=new_document(sw,'assembly',str(tpl/'Assembly.ASMDOT'));e['components']=[]
for p,t in items:
 a=[]
 for j in [0,3,6]:a.extend([t[j],t[j+2],-t[j+1]])
 a += [t[9],t[11],-t[10],1,0,0,0]
 c=add_component(asm,str(p),sw=sw);assert c is not None
 if m(c,'IsFixed'):
  c.Select4(False,create_empty_dispatch_variant(),False);m(asm,'UnfixComponent');asm.ClearSelection2(True)
 c.Transform2=create_math_transform(sw,a);assert all(abs(x-y)<1e-8 for x,y in zip(a,m(m(c,'Transform2'),'ArrayData')))
 c.Select4(False,create_empty_dispatch_variant(),False);m(asm,'FixComponent');asm.ClearSelection2(True)
 e['components'].append({'file':p.name,'source_transform':t,'sw_transform':a})
assert asm.ForceRebuild3(False);assert save_document(asm,str(out/'R4-Original-Layout.SLDASM'))
e['interference']=get_interference_detection(asm)
save_preview(asm,str(Path('work/R4-closed.bmp').resolve()),'isometric')
for c in list(m(asm,'GetComponents',True)):
 if m(c,'Name2').startswith(('R4-Base2-','R4-Base3-','R4-Base4-')):
  asm.ClearSelection2(True);c.Select4(False,create_empty_dispatch_variant(),False);m(asm,'EditSuppress')
assert save_document(asm,str(out/'R4-Open-Review.SLDASM'));save_preview(asm,str(Path('work/R4-open.bmp').resolve()),'isometric')
(out/'native-evidence.json').write_text(json.dumps(e,ensure_ascii=False,indent=2),encoding='utf8')
brief='按原视频和开源件最小改动：PCB下仓、电池76x36x18上仓；只改Base1内侧0.25mm局部清让及8x12过线槽，保留外轮廓。'
(out/'engineering-dag.json').write_text(json.dumps(build_engineering_plan(brief).to_dict(),ensure_ascii=False,indent=2),encoding='utf8')
print('DONE',flush=True)
