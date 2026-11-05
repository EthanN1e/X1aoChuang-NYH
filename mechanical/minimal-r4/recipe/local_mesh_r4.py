from pathlib import Path
import numpy as np,trimesh,json,hashlib
src=Path(r'C:\Users\32561\Downloads\开源模型\底座1.STL')
raw=trimesh.load_mesh(src,process=False)
# Canonicalize identical float32 STL vertex coordinates; no positional welding tolerance.
v,inv=np.unique(np.asarray(raw.vertices),axis=0,return_inverse=True)
m=trimesh.Trimesh(vertices=v,faces=inv[np.asarray(raw.faces)],process=False)
assert m.is_watertight and m.is_volume
def box(size,center):
 a=trimesh.creation.box(size);a.apply_translation(center);return a
cuts=[box([81,26.001,57],[86.60254,(27.485271+53.486271)/2,41.8]),
      box([8,12,14],[86.60254,38,76]),
      box([6,6,12],[45.5,34,56])]
result=trimesh.boolean.difference([m,*cuts],engine='manifold')
assert result.is_watertight and result.is_volume
assert np.allclose(m.bounds,result.bounds,atol=2e-5)
out=Path('outputs/R4-Minimal');out.mkdir(exist_ok=True)
result.export(out/'Base1-local-edit-source.stl')
report={'source_sha256':hashlib.sha256(src.read_bytes()).hexdigest(),'source_untouched':True,'method':'exact duplicate vertex indexing, manifold difference of three recorded boxes, no exterior additions','cuts_mm':[{'bounds':x.bounds.tolist()} for x in cuts],'original_volume_mm3':float(m.volume),'result_volume_mm3':float(result.volume),'removed_volume_mm3':float(m.volume-result.volume),'source_bounds':m.bounds.tolist(),'result_bounds':result.bounds.tolist(),'watertight':bool(result.is_watertight),'orientation_consistent':bool(result.is_winding_consistent)}
(out/'local-change-evidence.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8')
print(report)
