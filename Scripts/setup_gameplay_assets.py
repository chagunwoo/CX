"""Run with UnrealEditor-Cmd -run=pythonscript -script=... -EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities.
Idempotently connects native gameplay classes and places coin/buff spawners in both vehicle maps.
"""
import unreal
import os
import shutil
from datetime import datetime

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
backup = os.path.join(root, 'Saved', 'ImplementationBackup', datetime.now().strftime('%Y%m%d_%H%M%S'))

def preserve(path):
    relative = path.replace('/Game/', 'Content/')
    for ext in ['.uasset', '.umap']:
        src = os.path.join(root, relative + ext)
        if os.path.isfile(src):
            dst = os.path.join(backup, relative + ext)
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            shutil.copy2(src, dst)

def edit_bp(path, properties):
    preserve(path)
    bp = unreal.load_asset(path)
    assert bp, path
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    cdo = unreal.get_default_object(bp.generated_class())
    for name, value in properties.items(): cdo.set_editor_property(name, value)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    assert unreal.EditorAssetLibrary.save_loaded_asset(bp, False)
    unreal.log('CX_CONFIGURED ' + path)

for path in ['/Game/UI/WBP_Chat', '/Game/VehicleTemplate/Blueprints/W_VehicleAdvUI']:
    edit_bp(path, {})

tools = unreal.AssetToolsHelpers.get_asset_tools()
action_path = '/Game/VehicleTemplate/Input/Actions/IA_Attack'
action = unreal.load_asset(action_path) if unreal.EditorAssetLibrary.does_asset_exist(action_path) else tools.create_asset('IA_Attack', '/Game/VehicleTemplate/Input/Actions', unreal.InputAction, unreal.InputAction_Factory())
assert action
action.set_editor_property('value_type', unreal.InputActionValueType.BOOLEAN)
unreal.EditorAssetLibrary.save_loaded_asset(action, False)

for path in ['/Game/VehicleTemplate/Blueprints/SportsCar/BP_SportsCar_Pawn', '/Game/VehicleTemplate/Blueprints/OffroadCar/BP_OffroadCar_Pawn']:
    edit_bp(path, {'attack_action': action})
for path in ['/Game/VehicleTemplate/Blueprints/BP_VehicleAdvGameMode', '/Game/VehicleTemplate/Blueprints/BP_VehicleAdvOffroadGameMode']:
    edit_bp(path, {'player_state_class': unreal.CXPlayerState.static_class(), 'game_state_class': unreal.CXGameState.static_class()})

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors_api = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for map_path in ['/Game/VehicleTemplate/Maps/VehicleAdvExampleMap', '/Game/VehicleTemplate/Maps/VehicleOffroadExampleMap']:
    preserve(map_path)
    assert levels.load_level(map_path)
    actors = actors_api.get_all_level_actors()
    starts = sorted([a for a in actors if isinstance(a, unreal.PlayerStart)], key=lambda a: a.get_actor_label())
    assert starts, 'No PlayerStart in ' + map_path
    existing = {a.get_actor_label() for a in actors if isinstance(a, unreal.CXItemSpawner)}
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    count = 0
    for index, start in enumerate(starts):
        origin = start.get_actor_location()
        forward = start.get_actor_forward_vector()
        for step in range(4):
            label = 'CX_%s_%d_%d' % ('Speed' if step == 3 else 'Coin', index, step)
            if label in existing: continue
            location = origin + forward * (600.0 + 650.0 * step)
            hit = unreal.SystemLibrary.line_trace_single(world, location + unreal.Vector(0,0,500), location - unreal.Vector(0,0,2000), unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], unreal.DrawDebugTrace.NONE, True)
            if hit:
                # HitResult is a reflected struct; use the engine's stable break helper.
                result = hit.to_tuple()
                if result[0]: location = result[5] + unreal.Vector(0,0,65)
            spawner = actors_api.spawn_actor_from_class(unreal.CXItemSpawner, location)
            assert spawner
            spawner.set_actor_label(label)
            spawner.set_editor_property('item_class', unreal.CXSpeedBuff.static_class() if step == 3 else unreal.CXCoin.static_class())
            spawner.set_editor_property('is_spatially_loaded', False)
            count += 1
    assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log('CX_SPAWNERS %s added=%d total=%d' % (map_path, count, count + len(existing)))
unreal.log('CX_ASSET_SETUP_COMPLETE backup=' + backup)


