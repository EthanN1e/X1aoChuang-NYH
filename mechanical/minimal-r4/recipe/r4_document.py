from pathlib import Path
import sys,json,hashlib,shutil
from PIL import Image,ImageDraw,ImageFont
from reportlab.pdfgen import canvas
from reportlab.lib.pagesizes import A4,landscape
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.cidfonts import UnicodeCIDFont
from reportlab.lib.colors import HexColor
out=Path('outputs/R4-Minimal').resolve()
sys.path.insert(0,r'C:\Users\32561\.codex\skills\product-enclosure-design\scripts')
from stl_check import inspect
raw=inspect(out/'R4-Base1.STL','mm')
if not raw['exact_coordinate_topology_checks_passed']:
 shutil.copy2(out/'R4-Base1.STL','work/R4-Base1-solidworks-export-not-for-print.STL')
 shutil.copy2(out/'Base1-local-edit-source.stl',out/'R4-Base1.STL')
 (out/'mesh-export-provenance.json').write_text(json.dumps({'solidworks_STL_export_rejected':raw,'delivered_base1_STL':'Exact verified Boolean source mesh imported to create R4-Base1.SLDPRT; not an automatic repair of the rejected SolidWorks re-export.','new_fixture_STL_coordinates':'SolidWorks export translates all-positive for printing; use native assembly transforms for installation.'},indent=2),encoding='utf8')
checks={p.name:inspect(p,'mm') for p in out.glob('*.STL')}
assert all(c['exact_coordinate_topology_checks_passed'] for c in checks.values())
(out/'mesh-checks.json').write_text(json.dumps(checks,ensure_ascii=False,indent=2),encoding='utf8')
local=json.loads((out/'local-change-evidence.json').read_text())
e=json.loads((out/'native-evidence.json').read_text(encoding='utf8'))
plan={'plan_version':4,'source_brief':'按用户要求保留原设计，仅在固定底座原有两仓布置PCB和76x36x18电池；三处局部减料及独立承托、限位框，未新增底舱。','part_family':'assembly',
 'parameters':{'units':'mm','battery_user_nominal':[76,36,18],'battery_lead_envelope':'unknown','pcb_model':'hardware/r0/review-20261105/pcb-assembly-obj.zip','base1_source_sha256':local['source_sha256'],'base1_cuts_source_xyz_mm':local['cuts_mm'],'pcb_support_outer':[80,56,12.7],'pcb_support_opening':[78.8,54.8],'pcb_support_lip_thickness':2,'pcb_support_pad_diameter':3.4,'pcb_support_pad_centers':[[-38,-26],[-38,26],[38,-26],[38,26]],'battery_frame_outer':[79,39,18.8],'battery_frame_opening':[77,37],'battery_frame_notch_sketch':[8,3],'battery_frame_blind_cut_depth':3,'battery_frame_notch_centers':[[0,-19],[0,19]],'source_cover3_offset_y':1},
 'features':[{'name':'原底座1三处局部清让','type':'bounded_mesh_boolean_imported_solid','params':{'cuts_mm':local['cuts_mm']},'required':True,'fallback':'原件不覆盖；保留布尔参数和源hash，不宣称恢复原参数树'}, {'name':'PCB独立承托框','type':'native_boss_cut_support_pads','params':{'outer':[80,56],'inner':[78.8,54.8],'height':12.7},'required':True},{'name':'电池连续限位框','type':'native_boss_cut_blind_notches','params':{'outer':[79,39,18.8],'inner':[77,37],'blind_cut_depth':3},'required':True}],
 'operation_sequence':['verify_original_hash','local_mesh_boolean_and_solid_import','new_part','base_plate','pockets','slots','custom_properties','save_export_review','assemble_original_layout_and_reopen'],
 'assumptions':['打印工艺及材料待定；薄承托边为试装候选，不套用CNC默认壁厚。','电池尺寸为用户提供的裸外形，不含接头、出线或鼓胀余量。','原底座3向源Y外移1mm是装配候选，紧固件啮合长度待实测。','底座1来源为STL，局部修改件保留为导入实体；新增框为原生参数件。','PCB边缘支承和防抬起锁固未实物验证。'],
 'risk_register':[{'risk':'PCB边缘净空和焊脚/插座未进行精确实体求交','mitigation':'基于最新EDA OBJ包络初筛；用本版承托框试装，确认焊点、器件、插头后再定锁固。'},{'risk':'电池引线及完整线束尺寸未知','mitigation':'仅给出候选孔槽，按实际插头、护线件、线径和弯曲半径复核；避免裸线摩擦孔边。'},{'risk':'材料、薄壁强度及抗振未验证','mitigation':'新增框为TRIAL；先打印局部/分件试装。'},{'risk':'PCB和电池尚无验证后的防抬起约束','mitigation':'安装前需配合绝缘衬垫及盖内限位，实物确认可靠保持再制造放行。'}],
 'review_plan':{'expected_outputs':['SLDASM','SLDPRT','modified_parts_STEP_STL','design_review_PDF','drawing_SLDDrw_PDF','native_evidence','mesh_checks'],'views':['original_closed','original_open_front','part_first_angle'],'checks':['original_bytes_untouched','same_base1_outer_bounds','three_bounded_cuts_only','single_body_battery_frame','source_dimensions_readback','native_solid_interference','PCB_graphics_exclusion_disclosed','saved_assembly_references_reopened','PDF_dimension_text_match','STL_readback']},
 'solidworks_mapping':[{'operation':'local_mesh_boolean_and_solid_import','preferred_api':'manifold Boolean + ISldWorks.LoadFile4','notes':'导入实体，保留原STL和切削参数'}, {'operation':'assemble_original_layout_and_reopen','preferred_api':'AddComponent5 / Transform2 / OpenDoc6'}]}
import jsonschema
schema=json.loads(Path(r'C:\Users\32561\.codex\skills\solidworks-automation-skill\subskills\solidworks-vibecad\schemas\design_plan.schema.json').read_text(encoding='utf8'))
jsonschema.validate(plan,schema)
(out/'design_plan.json').write_text(json.dumps(plan,ensure_ascii=False,indent=2),encoding='utf8')
for src,dst in [('Open-Front.bmp','Open-Front.png'),('Closed-Front.bmp','Closed-Front.png')]:
 Image.open(Path('work')/src).save(out/dst)
# Native CAD image annotated for reading, geometry unchanged.
im=Image.open(out/'Open-Front.png').convert('RGB');im.thumbnail((1040,680))
panel=Image.new('RGB',(1450,820),'#f6f8fb');panel.paste(im,(30,100));draw=ImageDraw.Draw(panel)
font=lambda n:ImageFont.truetype(r'C:\Windows\Fonts\msyh.ttc',n)
draw.text((45,25),'R4  原有双仓内整合 · 无新增底舱',font=font(30),fill='#17334b')
for y,title,lines,col in [(150,'上仓：电池',['76 × 36 × 18 mm','独立限位框 / 预留出线缺口'], '#176e99'),(340,'下仓：PCB',['采用最新 EDA 导出几何','独立承托框 / 不给 PCB 钻孔'],'#7c7221'),(540,'仅三处局部减料',['下仓侧壁清让','仓间内侧线槽','原侧出线口扩大'],'#d26938')]:
 draw.rounded_rectangle((1080,y-12,1410,y+135),12,fill='white',outline='#d6e0e7',width=2)
 draw.text((1100,y),title,font=font(23),fill=col)
 for i,t in enumerate(lines):draw.text((1100,y+40+i*28),t,font=font(18),fill='#334654')
draw.text((45,775),'审阅/试装版；隐藏原盖板以展示内部。线材、锁固及打印配合仍待实物验证。',font=font(19),fill='#495e6c')
panel.save(out/'R4-Layout-Annotated.png')
pdfmetrics.registerFont(UnicodeCIDFont('STSong-Light'))
W,H=landscape(A4);c=canvas.Canvas(str(out/'R4-Design-Review.pdf'),pagesize=(W,H))
c.setTitle('R4 原有底座双仓整合方案 - 76x36x18 mm')
def text(x,y,t,size=11,color='#243e51'):
 c.setFillColor(HexColor(color));c.setFont('STSong-Light',size);c.drawString(x,y,t)
def heading(title,page):
 c.setFillColor(HexColor('#17334b'));c.rect(0,H-66,W,66,fill=1,stroke=0)
 text(32,H-36,title,22,'#ffffff');text(34,22,f'R4 / 2026-11-05 / 审阅与试装候选 / {page}',9,'#677d8d')
heading('保留原有外形，在原双仓内安放 PCB 与电池',1)
c.drawImage(str(out/'R4-Layout-Annotated.png'),49,100,width=744,height=421,preserveAspectRatio=True,anchor='c')
text(34,87,'电池以用户确认 76×36×18 mm 为依据；未增加 R3 的33 mm高底舱。',12)
text(34,64,'最新闭合装配实体干涉0项；PCB为图形参照，结果不包含其精确碰撞、插头和线缆。',11)
c.showPage();heading('三处局部修改及尺寸基准',2)
text(34,H-92,'单位 mm；以下均使用原始 STL 的 X/Y/Z 源坐标。显示总装变换为 (X,Z,-Y)。',11)
rows=[('A 下仓侧壁清让','X 46.10254～127.10254；Z 13.3～70.3','净宽81、净高57；Y 27.485271～53.486271'),('B 两仓内侧线槽','X 82.60254～90.60254；Z 69～83','切削包络8×12×14；Y 32～44'),('C 原侧出线口扩大','X 42.5～48.5；Z 50～62','切削包络6×6×12；Y 31～37')]
for i,(title,a,b) in enumerate(rows):
 y=H-135-i*83;c.setFillColor(HexColor('#edf3f8'));c.roundRect(32,y-49,W-64,72,6,fill=1,stroke=0)
 text(46,y,title,13);text(250,y,a,11);text(250,y-24,b,11)
text(34,199,'原底座1外包络保持 173.205078×27.600000×150；局部减料约1949.56 mm³（3.75%）。',11)
text(34,172,'PCB承托：外80×56，内78.8×54.8，边厚2，总高12.7；4个实心Ø3.4脚位于(±38,±26)。',11)
text(34,145,'电池框：外79×39×18.8，内77×37；两处8×3草图缺口，盲切3，框体保持连通。',11)
text(34,118,'底座2/4几何不改；底座3几何不改，装配中沿源Y外移1。需核对螺钉长度及盖内限位。',11)
text(34,80,'本页为修改定位表，不是完整关联尺寸的制造图。原件为网格，局部改件是导入实体。',10,'#9a4c21')
text(34,58,'参考：原视频 BV1of8y6vEQ5（约70～85秒装配），原开源模型；未附视频画面。',10)
c.showPage();heading('复核记录与试装要点',3)
items=[('已核对','原件字节校验、局部改件相同外包络；电池框单实体；新件STEP/STL回读。'),('装配','两份SLDASM均重开，8个组件引用可读；闭合版实体干涉0项，PCB图形不计入。'),('PCB净空','最新OBJ总包络初筛：最高几何距原下盖结合平面约1.389；未含插头和线束。'),('电池净空','框内长宽相对名义电池各增加1；底部约1.015空间用于绝缘衬垫候选。'),('走线','原侧口和仓间槽已留；电源/负载与信号线应分束固定，端子不受外拉。'),('尚需试配','实际接头、线径、护线件、弯曲半径、线束应力释放、PCB防抬起及电池保持。'),('工艺限制','薄边/壁厚为试装候选；未验证全局壁厚、自交、承载、热、振动、EMC和防水。'),('电气检查','本轮未改电路，未现场重新运行ERC/DRC或电气仿真；不引用历史结果作为本轮通过。')]
for i,(a,b) in enumerate(items):
 y=H-106-i*49;text(36,y,a,12);text(139,y,b,11)
text(34,59,'先试装并校准固定结构，再按实际接头定孔槽和锁固；本包不作制造放行。',11,'#9a4c21')
c.save()
print('DESIGN_DOCUMENT_READY',flush=True)
