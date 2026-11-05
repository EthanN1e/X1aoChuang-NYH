exec(open('work/r4_final_native.py',encoding='utf8').read().split("p=out/'Drawings")[0])
for stem,part in [('PCB-Support-R4-DRAFT','R4-PCB-Support-TRIAL'),('Battery-Cradle-R4-DRAFT','R4-Battery-Cradle-TRIAL')]:
 p=out/'Drawings'/(stem+'.SLDDRW');d=open_document(sw,str(p));sheet=m(d,'GetCurrentSheet')
 before=m(sheet,'GetProperties2');assert m(sheet,'SetScale',1.,1.,False,False)
 assert d.ForceRebuild3(False);assert save_document(d,str(p));sw.CloseDoc(m(d,'GetTitle'))
 d=open_document(sw,str(p));after=m(m(d,'GetCurrentSheet'),'GetProperties2');assert abs(after[2]/after[3]-1)<1e-8
 assert export_sheet_to_pdf(d,str(p.with_suffix('.pdf')),sw_app=sw)
 st=inspect_drawing_structure(d,paper_size_hint='A3');spec=json.loads((out/(part+'-drawing-spec.json')).read_text())
 previews=save_review_previews(d,out/'Drawings/previews',basename=stem,views=('front',))
 r=review_drawing_artifacts(spec,structure=st,pdf_path=p.with_suffix('.pdf'),preview_evidence=[inspect_bmp_preview(x) for x in previews])
 (out/'Drawings'/(stem+'-final-review.json')).write_text(json.dumps({'status':r['status'],'sheet_properties_before':before,'sheet_properties_reopened':after,'structure':st,'review':r},indent=2),encoding='utf8')
 print(stem,r['status'],after,flush=True);sw.CloseDoc(m(d,'GetTitle'))
