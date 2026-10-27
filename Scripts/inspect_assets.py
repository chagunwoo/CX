import unreal
assets = ["/Game/UI/WBP_Chat", "/Game/VehicleTemplate/Blueprints/W_VehicleAdvUI", "/Game/VehicleTemplate/Blueprints/BP_VehicleAdvGameMode", "/Game/VehicleTemplate/Blueprints/BP_VehicleAdvPlayerController"]
for path in assets:
    bp = unreal.load_asset(path)
    unreal.log("CX_INSPECT %s class=%s" % (path, bp.generated_class()))
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    cdo = unreal.get_default_object(bp.generated_class())
    for prop in ["player_state_class", "game_state_class", "vehicle_ui_class"]:
        try: unreal.log("CX_INSPECT %s=%s" % (prop, cdo.get_editor_property(prop)))
        except Exception: pass
world = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
world.load_level("/Game/VehicleTemplate/Maps/VehicleAdvExampleMap")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
for a in actors:
    if isinstance(a, unreal.PlayerStart): unreal.log("CX_START %s %s %s" % (a.get_actor_label(), a.get_actor_location(), a.get_actor_rotation()))
unreal.log("CX_INSPECT actor_count=%d" % len(actors))

