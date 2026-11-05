from pathlib import Path
import json
script=Path('work/build_r4.py').read_text()
exec(script.split('out=Path(')[0])
out=Path('outputs/R4-Minimal').resolve();tpl=Path(r'C:\ProgramData\SOLIDWORKS\SOLIDWORKS 2025\templates')
e={'parts':{}};sw,_=connect_solidworks()
for kind in (3,2,1):
 for d in list(m(sw,'GetDocuments') or []):
  if str(out).lower() in str(m(d,'GetPathName')).lower() and m(d,'GetType')==kind:sw.CloseDoc(m(d,'GetTitle'))
exec(script[script.index('def rect('):script.index('# Only Base1')])
d=new_document(sw,'part',str(tpl/'Part.PRTDOT'));rect(d,80,56,2,False,(78.8,54.8))
with sketch(d) as sk:
 for x in [-38,38]:
  for y in [-26,26]:sketch_circle(d,x/1000,y/1000,.0017)
 fully_define_sketch(d)
assert extrude_boss(d,sk,.0127,direction=False) is not None
assert set_document_appearance(d,'silver');finish(d,'R4-PCB-Support-TRIAL.SLDPRT')
d=new_document(sw,'part',str(tpl/'Part.PRTDOT'));rect(d,79,39,18.8,False,(77,37))
with sketch(d) as sk:
 for y in [-19,19]:sketch_rectangle(d,0,y/1000,.008,.003)
 fully_define_sketch(d)
assert extrude_cut(d,sk,.003,direction=True) is not None
bodies=list(m(d,'GetBodies2',0,False));assert len(bodies)==1,len(bodies)
e['battery_frame_body_count']=len(bodies)
assert set_document_appearance(d,'graphite');finish(d,'R4-Battery-Cradle-TRIAL.SLDPRT')
old=json.loads((out/'native-evidence.json').read_text(encoding='utf8'))
old['parts'].update(e['parts']);old['battery_frame_body_count']=1
for filename in ['R4-Original-Layout.SLDASM','R4-Open-Review.SLDASM']:
 asm=open_document(sw,str(out/filename))
 for c in m(asm,'GetComponents',True):
  if str(m(c,'Name2')).startswith('R4-PCB-Support-TRIAL-'):
   a=list(m(m(c,'Transform2'),'ArrayData'));a[11]=-.0401853
   c.Select4(False,create_empty_dispatch_variant(),False);m(asm,'UnfixComponent');asm.ClearSelection2(True)
   c.Transform2=create_math_transform(sw,a)
   c.Select4(False,create_empty_dispatch_variant(),False);m(asm,'FixComponent');asm.ClearSelection2(True)
   for item in old['components']:
    if item['file']=='R4-PCB-Support-TRIAL.SLDPRT':item['sw_transform']=a;item['source_transform'][10]=.0401853
 assert asm.ForceRebuild3(False);assert save_document(asm,str(out/filename))
 if filename.startswith('R4-Original'):old['interference']=get_interference_detection(asm)
 else:save_preview(asm,str(Path('work/R4-open-back.bmp').resolve()),'back')
 old.setdefault('reopen_checks',{})[filename]=[{'name':m(c,'Name2'),'file':m(c,'GetPathName'),'suppressed':m(c,'IsSuppressed')} for c in m(asm,'GetComponents',True)]
 sw.CloseDoc(m(asm,'GetTitle'))
(out/'native-evidence.json').write_text(json.dumps(old,ensure_ascii=False,indent=2),encoding='utf8')
print(json.dumps({'battery_frame_body_count':1,'interference':old['interference']},ensure_ascii=False),flush=True)
