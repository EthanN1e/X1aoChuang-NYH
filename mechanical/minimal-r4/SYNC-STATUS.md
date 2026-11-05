# R4 同步、下载与验证说明

更新时间：2026-11-05。当前机械版本为 **R4 原底座双仓整合试装版**。PCB位于原下仓，76×36×18 mm电池位于原上仓，保留原外轮廓和悬臂。底座1三处局部减料；电池框出线缺口最终盲切深度为6 mm。

## 版本与仓库入口

- R4设计交付提交：`f247c6f48cc5b0e602e83f207ec6c90f9e0bfa1c`。
- [当前方案目录](https://github.com/EthanN1e/X1aoChuang-NYH/tree/main/mechanical/minimal-r4)。
- [R4同步PR #3](https://github.com/EthanN1e/X1aoChuang-NYH/pull/3)。主分支收录本版供下载和试装，不等于制造放行。
- [R3历史PR #2](https://github.com/EthanN1e/X1aoChuang-NYH/pull/2)的新增33 mm底舱方向已被用户后续要求替代，不作为当前制作依据。

## 已同步的交付内容

| 内容 | 位置 |
|---|---|
| 布局预览与结构说明 | 本目录README.md、R4-Layout-Annotated.png、Open-Front.png、Closed-Front.png |
| 中文方案、三处孔槽定位及验证边界 | R4-Design-Review.pdf |
| 底座局部改件、PCB承托框、电池框 | 三组STEP/STL文件 |
| 两张新增件试装图及最终检查记录 | Drawings/内PDF和final-review.json；原生SLDDRW在完整包内 |
| 完整SolidWorks原生文件 | download/内5个ZIP分卷，共19,128,494字节 |
| 设计参数、几何和装配检查、导出来源 | design_plan.json、native-evidence.json、mesh-checks.json、mesh-export-provenance.json等 |
| 建模及复核脚本记录 | recipe/；依赖相应本机技能和源文件路径，不作为独立通用生成器 |

完整包包含 **8个SLDPRT、2个SLDASM、2个SLDDRW**，以及PDF、STEP/STL和验证记录。原始下载模型没有被覆盖。PCB为实际EDA图形参照，不将完整装配冒充为精确B-rep STEP。

## 下载与还原（Windows）

1. 在仓库 `Code → Download ZIP` 下载主分支并解压，或克隆仓库。保留整个 `mechanical/minimal-r4/` 目录结构。
2. 在该目录打开PowerShell，运行 `./Restore-Review.ps1`。脚本会校验5个分卷的SHA-256，合并出 `R4-SolidWorks.zip`，再校验完整ZIP。分卷需要全部下载，不能把 `.001` 当作独立ZIP解压。
3. 解压 `R4-SolidWorks.zip` 到一个文件夹，保持原生零件与装配在同一目录。
4. 打开 `R4-Original-Layout.SLDASM` 查看闭合布局；打开 `R4-Open-Review.SLDASM` 查看内部。开盖版有意压缩底座2/3/4，另外5个组件应正常解析；闭合版8个组件均应解析。

脚本若提示 `Output already exists`，说明目标ZIP已存在；可直接对照manifest检查其SHA-256，或在另一个空目录还原。脚本不会覆盖既有ZIP。

原生ZIP保留设计交付快照，最新同步说明以仓库本文件为准。

完整ZIP SHA-256：`b71e1402cee0eb16fa37d98fc4c77ec17e73ffc6ac886ba2d114344db98f843b`。分卷校验值见 [download/manifest.json](download/manifest.json)。

## 本次同步核对结果

- 已上传的43个交付文件与本地按Git blob SHA逐一比对：43个匹配，0个缺失或不一致。
- 已在独立目录实际运行Restore-Review.ps1，分卷合并、SHA-256及还原后ZIP CRC检查通过；分卷、完整包SHA-256及原生文件清单见 [SYNC-VERIFICATION.json](SYNC-VERIFICATION.json)。
- 闭合装配已重开：8个组件全部解析，实体干涉0项；PCB图形不参与实体干涉求解。
- 两张新增零件图最终PDF尺寸文字关联和重叠检查通过；交付STL基础拓扑检查通过。

## 使用边界

本版为可继续修改的审阅/试装候选。插头、护线件、完整线束弯曲、防抬起锁固、电池保持、盖板紧固件和打印配合需实物验证。底座3装配让位1 mm；底座1由原网格局部修改后导入实体，未恢复原参数树。新增框件为原生参数零件。

底座1的SolidWorks再次STL导出出现精确开放边，已拒绝用于打印；交付STL采用验证过且用于创建该实体的布尔源网格，异常来源记录保留。基础拓扑通过不表示强度或打印认证。

本轮未修改PCB电路，也未重新运行ERC、DRC或电气仿真。仓库历史电气检查结果不作为本轮现场通过证明。

## Git 检出时的文件完整性

仓库使用 .gitattributes 禁止对 PDF、CAD、图片和 ZIP 分卷进行自动换行转换；STEP 保留可查看文本差异的方式并禁止换行转换。Windows 的 core.autocrlf=true 不应改变这些交付文件的字节及校验值。
