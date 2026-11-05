from pathlib import Path
import json
s=Path('work/build_r4.py').read_text();exec(s.split('out=Path(')[0])
out=Path('outputs/R4-Minimal').resolve();sw,_=connect_solidworks();e=json.loads((out/'native-evidence.json').read_text(encoding='utf8'))
for kind in (3,2,1):
 for d in list(m(sw,'GetDocuments') or []):
  if str(out).lower() in str(m(d,'GetPathName')).lower() and m(d,'GetType')==kind:sw.CloseDoc(m(d,'GetTitle'))
tpl=Path(r'C:\ProgramData\SOLIDWORKS\SOLIDWORKS 2025\templates')
asm=new_document(sw,'assembly',str(tpl/'Assembly.ASMDOT'))
for item in e['components']:
 c=add_component(asm,str(out/item['file']),sw=sw);assert c is not None
 if m(c,'IsFixed'):
  c.Select4(False,create_empty_dispatch_variant(),False);m(asm,'UnfixComponent');asm.ClearSelection2(True)
 c.Transform2=create_math_transform(sw,item['sw_transform'])
 c.Select4(False,create_empty_dispatch_variant(),False);m(asm,'FixComponent');asm.ClearSelection2(True)
 assert not m(c,'IsSuppressed')
assert asm.ForceRebuild3(False);assert save_document(asm,str(out/'R4-Original-Layout.SLDASM'))
sw.CloseDoc(m(asm,'GetTitle'))
asm=open_document(sw,str(out/'R4-Original-Layout.SLDASM'));cs=list(m(asm,'GetComponents',True));assert len(cs)==8
assert all(not m(c,'IsSuppressed') for c in cs)
e['interference']=get_interference_detection(asm)
e['reopen_checks']={}
def record(model):
 return [{'name':m(c,'Name2'),'file':m(c,'GetPathName'),'suppressed':m(c,'IsSuppressed'),'transform':list(m(m(c,'Transform2'),'ArrayData'))} for c in m(model,'GetComponents',True)]
e['reopen_checks']['R4-Original-Layout.SLDASM']=record(asm)
m(asm,'ViewDisplayShaded');save_preview(asm,str(Path('work/Closed-Front.bmp').resolve()),'back')
for c in m(asm,'GetComponents',True):
 if str(m(c,'Name2')).startswith(('R4-Base2-','R4-Base3-','R4-Base4-')):
  c.Select4(False,create_empty_dispatch_variant(),False);m(asm,'EditSuppress');asm.ClearSelection2(True)
assert save_document(asm,str(out/'R4-Open-Review.SLDASM'));sw.CloseDoc(m(asm,'GetTitle'))
asm=open_document(sw,str(out/'R4-Open-Review.SLDASM'))
e['reopen_checks']['R4-Open-Review.SLDASM']=record(asm)
for c in m(asm,'GetComponents',True):
 expected=str(m(c,'Name2')).startswith(('R4-Base2-','R4-Base3-','R4-Base4-'))
 assert bool(m(c,'IsSuppressed'))==expected
m(asm,'ViewDisplayShaded');save_preview(asm,str(Path('work/Open-Front.bmp').resolve()),'back');save_document(asm,str(out/'R4-Open-Review.SLDASM'));sw.CloseDoc(m(asm,'GetTitle'))
(out/'native-evidence.json').write_text(json.dumps(e,ensure_ascii=False,indent=2),encoding='utf8')
print(json.dumps({'interference':e['interference'],'all_closed_components_resolved':True},ensure_ascii=False),flush=True)
