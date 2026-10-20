"""Place a food source in the running PIE world: python -I pie_place_food.py <x_m> <y_m>

Editor scripting can only spawn into the editor world, so this uses the engine's `summon` cheat, which spawns
72 cm in front of the player's view (plus 15 cm up). It moves the PIE pawn so the source lands exactly at
(x, y, 1 m), then puts the camera back where it was. Food sources register with the colony on BeginPlay.
"""
import os
import subprocess
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))


def main():
    x_cm, y_cm = float(sys.argv[1]) * 100.0, float(sys.argv[2]) * 100.0
    statement = (
        "w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world(); "
        "pc = unreal.GameplayStatics.get_player_controller(w, 0); pawn = pc.get_controlled_pawn(); "
        "home = pawn.get_actor_location(); home_rot = pc.get_control_rotation(); "
        # Face +Y from 72 cm short of the target, 15 cm below the 1 m spawn height.
        f"pawn.set_actor_location(unreal.Vector({x_cm}, {y_cm - 72.0}, 85.0), False, True); "
        "pc.set_control_rotation(unreal.Rotator(0.0, 0.0, 90.0)); "
        "unreal.SystemLibrary.execute_console_command(w, 'summon /Script/Supercolony.FoodSourceActor', pc); "
        "pawn.set_actor_location(home, False, True); pc.set_control_rotation(home_rot); "
        "foods = unreal.GameplayStatics.get_all_actors_of_class(w, unreal.FoodSourceActor); "
        "print('food sources:', [(f.get_name(), round(f.get_actor_location().x), round(f.get_actor_location().y)) for f in foods]); "
        "print('game time %.1f s' % unreal.GameplayStatics.get_time_seconds(w))"
    )
    return subprocess.run([sys.executable, "-I", os.path.join(SCRIPT_DIR, "editor_python.py"), statement]).returncode


if __name__ == "__main__":
    sys.exit(main())
