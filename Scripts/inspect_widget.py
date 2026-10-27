import unreal
bp = unreal.load_asset('/Game/UI/WBP_Chat')
for obj in [bp, unreal.get_default_object(bp.generated_class())]:
    task = unreal.AssetExportTask()
    task.object = obj
    task.filename = 'C:/UE5/CX/Saved/' + obj.get_name() + '.copy'
    task.automated = True
    task.prompt = False
    task.exporter = unreal.ObjectExporterT3D()
    unreal.Exporter.run_asset_export_task(task)
unreal.log('CX_DONE')
