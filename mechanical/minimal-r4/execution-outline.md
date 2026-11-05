# SolidWorks 执行摘要

- 零件族: `assembly`
- 执行模式: `solidworks_com_serial`
- 单位策略: plan uses mm; SolidWorks API receives meters via sw_connect.mm()

## 步骤

1. `verify_original_hash` -> `api_lookup_required` [needs_api_lookup]
2. `local_mesh_boolean_and_solid_import` -> `api_lookup_required` [needs_api_lookup]
3. `new_part` -> `sw_session.SolidWorksSession.new_part` [ready]
4. `base_plate` -> `sw_part.sketch_rectangle + sw_part.extrude_boss` [ready]
5. `pockets` -> `sw_part.sketch_rectangle or sketch_slot + extrude_cut` [ready]
6. `slots` -> `sw_part.sketch_slot + extrude_cut` [ready]
7. `custom_properties` -> `model.Extension.CustomPropertyManager` [ready]
8. `save_export_review` -> `session.save + session.export + sw_review.run_review` [ready]
9. `assemble_original_layout_and_reopen` -> `api_lookup_required` [needs_api_lookup]

## 特征门禁

- `原底座1三处局部清让` (bounded_mesh_boolean_imported_solid), required=True；降级：原件不覆盖；保留布尔参数和源hash，不宣称恢复原参数树
- `PCB独立承托框` (native_boss_cut_support_pads), required=True
- `电池连续限位框` (native_boss_cut_blind_notches), required=True

## 停止条件

- SolidWorks preflight fails
- required base geometry feature returns None
- save/export output file missing or zero bytes
- review report marks previews blank
