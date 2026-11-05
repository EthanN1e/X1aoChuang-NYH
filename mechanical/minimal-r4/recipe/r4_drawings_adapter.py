from pathlib import Path
import sys,json
S=Path(r'C:\Users\32561\.codex\skills\solidworks-automation-skill')
sys.path.insert(0,str(S))
from scripts import sw_drawing
from scripts.sw_drawing_package import generate_package
original_arrange=sw_drawing._MODULE.auto_arrange_drawing_dimensions
def arrange(drawing_model,**kwargs):
    r=original_arrange(drawing_model,spacing_m=0.018)
    m=sw_drawing._MODULE._safe_member
    for view in m(m(drawing_model,'GetCurrentSheet'),'GetViews',default=[]) or []:
        entries=[]
        for dd in sw_drawing._MODULE._view_display_dimensions(view):
            a=m(dd,'GetAnnotation');pos=list(m(a,'GetPosition'));entries.append((dd,a,pos,m(dd,'Type2')))
        outline=m(view,'GetOutline')
        if not outline:continue
        centers=[(outline[0]+outline[2])/2,(outline[1]+outline[3])/2]
        for typ,axis in [(11,1),(12,0)]:
            for sign in [-1,1]:
                group=[e for e in entries if e[3]==typ and (e[2][axis]-centers[axis])*sign>0]
                levels=sorted(set(round(e[2][axis]*sign,5) for e in group))
                if not levels:continue
                mapped={levels[0]:levels[0]}
                for previous,current in zip(levels,levels[1:]):mapped[current]=max(current,mapped[previous]+.009)
                for dd,a,pos,_ in group:
                    pos[axis]=mapped[round(pos[axis]*sign,5)]*sign
                    assert m(a,'SetPosition2',*pos)
        for dd,a,pos,typ in entries:
            dim=m(dd,'GetDimension2',0);name=str(m(dim,'FullName',default=''))
            if typ==2 and 'Boss-Extrude2' in name and str(m(view,'Name')).endswith('2'):
                pos[0]+=.006;assert m(a,'SetPosition2',*pos)
    # Correct the supplied GB template's literal scale label.
    for view in m(drawing_model,'GetViews',default=[]) or []:
        for v in view if isinstance(view,(list,tuple)) else [view]:
            for note in m(v,'GetNotes',default=[]) or []:
                if str(m(note,'GetText',default='')).strip()=='2:1':m(note,'SetText','1:1')
    return r
sw_drawing._MODULE.auto_arrange_drawing_dimensions=arrange
out=Path('outputs/R4-Minimal').resolve()
for name,target in [('R4-PCB-Support-TRIAL','PCB-Support-R4-DRAFT.SLDDRW'),('R4-Battery-Cradle-TRIAL','Battery-Cradle-R4-DRAFT.SLDDRW')]:
    path=out/(name+'-drawing-spec.json')
    spec=json.loads(path.read_text(encoding='utf8'));spec['scale']='1:1'
    spec['modelSizeMm']=[80,56,12.7] if 'PCB' in name else [79,39,18.8]
    spec['notes']=[n.replace('12.715','12.7').replace('Two 8x3 wire notches, centers (0,+/-19) on cradle face.','Two 8x3 notch sketches at (0,+/-19); blind cut depth3, keep frame connected.') for n in spec['notes']]
    path.write_text(json.dumps(spec,indent=2),encoding='utf8')
    result=generate_package(path,out/'Drawings',target)
    print(json.dumps({'target':target,'status':result.get('status'),'error_code':result.get('error_code'),'message':result.get('message')},ensure_ascii=False),flush=True)
