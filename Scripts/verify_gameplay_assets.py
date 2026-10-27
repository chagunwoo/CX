"""Read-only validation of the saved gameplay assets and level instances."""
import unreal
blueprints = [
    '/Game/UI/WBP_Chat',
    '/Game/VehicleTemplate/Blueprints/W_VehicleAdvUI',
    '/Game/VehicleTemplate/Blueprints/BP_VehicleAdvGameMode',
    '/Game/VehicleTemplate/Blueprints/BP_VehicleAdvOffroadGameMode',
    '/Game/VehicleTemplate/Blueprints/SportsCar/BP_SportsCar_Pawn',
    '/Game/VehicleTemplate/Blueprints/OffroadCar/BP_OffroadCar_Pawn',
]
for path in blueprints:
    bp = unreal.load_asset(path)
    assert bp
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    cdo = unreal.get_default_object(bp.generated_class())
    if 'GameMode' in path:
        assert cdo.get_editor_property('player_state_class') == unreal.CXPlayerState.static_class()
        assert cdo.get_editor_property('game_state_class') == unreal.CXGameState.static_class()
    if 'Pawn' in path:
        assert cdo.get_editor_property('attack_action').get_path_name() == '/Game/VehicleTemplate/Input/Actions/IA_Attack.IA_Attack'
for cls in [unreal.CXCoin, unreal.CXSpeedBuff]:
    cdo = unreal.get_default_object(cls.static_class())
    assert cdo.get_editor_property('replicates')
    assert cdo.get_editor_property('net_update_frequency') == 1.0
for name, expected in [('VehicleAdvExampleMap', 12), ('VehicleOffroadExampleMap', 4)]:
    assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/VehicleTemplate/Maps/' + name)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    spawners = [a for a in actors if isinstance(a, unreal.CXItemSpawner)]
    assert len(spawners) == expected, (name, len(spawners))
    assert all(a.get_editor_property('replicates') for a in spawners)
    coins = sum(a.get_editor_property('item_class') == unreal.CXCoin.static_class() for a in spawners)
    assert coins == expected * 3 // 4
    unreal.log('CX_ASSET_CHECK %s coins=%d buffs=%d PASS' % (name, coins, expected - coins))
unreal.log('CX_ASSET_CHECK ALL PASS')
