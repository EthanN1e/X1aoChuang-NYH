exec(open('work/r4_final_native.py',encoding='utf8').read().split("p=out/'Drawings")[0])
from scripts.sw_export import export_to_step,export_to_stl
from scripts.sw_assembly import get_interference_detection
for kind in (3,2,1):
 for dd in list(m(sw,'GetDocuments') or []):
  if str(out).lower() in str(m(dd,'GetPathName')).lower() and m(dd,'GetType')==kind:sw.CloseDoc(m(dd,'GetTitle'))
p=out/'R4-Battery-Cradle-TRIAL.SLDPRT';d=open_document(sw,str(p))
parameter=m(d,'Parameter','D1@Cut-Extrude2');assert parameter is not None
assert min(abs(parameter.SystemValue-.003),abs(parameter.SystemValue-.006))<1e-8
parameter.SystemValue=.006;assert d.ForceRebuild3(False)
assert len(list(m(d,'GetBodies2',0,False)))==1
assert abs(m(m(d,'Parameter','D1@Cut-Extrude2'),'SystemValue')-.006)<1e-8
assert save_document(d,str(p));assert export_to_step(d,str(p.with_suffix('.step')));assert export_to_stl(d,str(p.with_suffix('.STL')));sw.CloseDoc(m(d,'GetTitle'))
specpath=out/'R4-Battery-Cradle-TRIAL-drawing-spec.json';spec=json.loads(specpath.read_text())
spec['notes']=[n.replace('blind cut depth3','blind cut depth6') for n in spec['notes']];specpath.write_text(json.dumps(spec,indent=2))
p=out/'Drawings/Battery-Cradle-R4-DRAFT.SLDDRW';d=open_document(sw,str(p))
from win32com.client import Dispatch
for sheetviews in m(d,'GetViews') or []:
 for v in sheetviews if isinstance(sheetviews,(list,tuple)) else [sheetviews]:
  v=Dispatch(v)
  for note in m(v,'GetNotes',default=[]) or []:
   note=Dispatch(note)
   t=str(m(note,'GetText') or '')
   if 'blind cut depth3' in t:assert m(note,'SetText',t.replace('blind cut depth3','blind cut depth6'))
assert d.ForceRebuild3(False);assert save_document(d,str(p));sw.CloseDoc(m(d,'GetTitle'))
e=json.loads((out/'native-evidence.json').read_text(encoding='utf8'));e['wire_relief']={'blind_cut_depth_mm':6,'channel_source_Y_mm':[32,44],'notch_source_Y_mm':[40.5,46.5],'depth_overlap_mm':3.5,'not_established':['actual wire bundle size','plug insertion envelope','bend radius','strain relief']}
d=open_document(sw,str(out/'R4-Original-Layout.SLDASM'));cs=list(m(d,'GetComponents',True));assert len(cs)==8 and all(not m(co,'IsSuppressed') for co in cs)
e['interference']=get_interference_detection(d);assert e['interference']['interference_count']==0
sw.CloseDoc(m(d,'GetTitle'));(out/'native-evidence.json').write_text(json.dumps(e,ensure_ascii=False,indent=2),encoding='utf8')
print('WIRE_RELIEF_6MM_SINGLE_BODY; ALL_8_RESOLVED; SOLID_INTERFERENCE_0',flush=True)
