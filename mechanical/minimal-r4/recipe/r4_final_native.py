from pathlib import Path
import sys,json
S=Path(r'C:\Users\32561\.codex\skills\solidworks-automation-skill');sys.path.insert(0,str(S))
from scripts.sw_connect import connect_solidworks,open_document,save_document,get_com_member as m
from scripts.sw_drawing import inspect_drawing_structure,export_sheet_to_pdf,_MODULE
from scripts.sw_drawing_review import review_drawing_artifacts
from scripts.sw_review import save_review_previews,inspect_bmp_preview,save_preview
out=Path('outputs/R4-Minimal').resolve();sw,_=connect_solidworks()
p=out/'Drawings/PCB-Support-R4-DRAFT.SLDDRW';d=open_document(sw,str(p))
changed=[]
for v in m(m(d,'GetCurrentSheet'),'GetViews') or []:
 for dd in _MODULE._view_display_dimensions(v):
  if str(m(v,'Name')).endswith('1') and int(m(dd,'Type2'))==6:
   origin=list(m(v,'Position'));a=m(dd,'GetAnnotation')
   pos=[origin[0]-.060,origin[1]-.026,0]
   assert m(a,'SetPosition2',*pos)
   m(dd,'SetText',1,'4X<MOD-DIAM>')
   changed.append({'view_origin':origin,'position':pos})
assert d.ForceRebuild3(False);assert save_document(d,str(p));sw.CloseDoc(m(d,'GetTitle'))
d=open_document(sw,str(p));assert export_sheet_to_pdf(d,str(p.with_suffix('.pdf')),sw_app=sw)
st=inspect_drawing_structure(d,paper_size_hint='A3');spec=json.loads((out/'R4-PCB-Support-TRIAL-drawing-spec.json').read_text())
previews=save_review_previews(d,out/'Drawings/previews',basename='PCB-Support-R4-DRAFT',views=('front',))
r=review_drawing_artifacts(spec,structure=st,pdf_path=p.with_suffix('.pdf'),preview_evidence=[inspect_bmp_preview(x) for x in previews])
(out/'Drawings/PCB-Support-R4-final-review.json').write_text(json.dumps({'status':r['status'],'changed_positions':changed,'structure':st,'review':r},indent=2),encoding='utf8')
print('PCB_DRAWING',r['status'],changed,flush=True);sw.CloseDoc(m(d,'GetTitle'))
for name,preview in []:
 d=open_document(sw,str(out/name));m(d,'ViewDisplayShaded');m(d,'GraphicsRedraw2');save_preview(d,str(Path('work')/(preview+'.bmp')),'back');assert save_document(d,str(out/name));sw.CloseDoc(m(d,'GetTitle'))
