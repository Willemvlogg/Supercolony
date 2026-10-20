"""Record an Unreal Insights trace of the running PIE session and print per-frame thread and timer costs.

Usage:
    python -I profile_pie.py <label> [--seconds 15]

Starts a trace in the PIE world through Python Remote Execution (see editor_python.py), stops it after
--seconds, exports the timing events with headless Unreal Insights, and prints:
  - frames per second and average game-thread frame time (FEngineLoop::Tick)
  - render-thread and GPU (graphics queue) busy time per frame (sum of top-level events)
  - per-frame cost of the named timers below (ant processors, pheromone grid step, ISM updates)
Outputs land in Saved/Profiling/<label>.*
"""
import argparse
import csv
import glob
import math
import os
import subprocess
import sys
import time
from collections import defaultdict

ENGINE = r"C:\Program Files\Epic Games\UE_5.8\Engine"
PROJECT_DIR = r"C:\Dev\Supercolony"
PROFILING = os.path.join(PROJECT_DIR, "Saved", "Profiling")
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

WATCHED_TIMERS = [
    "AntSteerProcessor_0", "AntMoveProcessor_0", "AntDepositProcessor_0", "AntRenderProcessor_0",
    "PheromoneGrid_Step", "UInstancedStaticMeshComponent::CalcBoundsImpl",
    "UInstancedStaticMeshComponent::BatchUpdateInstancesTransforms",
]


def run_in_editor(statement):
    subprocess.run([sys.executable, "-I", os.path.join(SCRIPT_DIR, "editor_python.py"), statement], check=True)


def console(command):
    run_in_editor("w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world(); "
                  f"unreal.SystemLibrary.execute_console_command(w, '{command}')")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("label")
    parser.add_argument("--seconds", type=float, default=15.0)
    parser.add_argument("--trace", help="Analyse this existing .utrace instead of recording a new one")
    args = parser.parse_args()

    if args.trace:
        trace = args.trace
    else:
        before = set(glob.glob(os.path.join(PROFILING, "*.utrace")))
        console("trace.file default,cpu,gpu,frame")
        time.sleep(args.seconds)
        console("trace.stop")
        time.sleep(2.0)
        new_traces = sorted(set(glob.glob(os.path.join(PROFILING, "*.utrace"))) - before, key=os.path.getmtime)
        if not new_traces:
            print("No new trace file appeared in Saved/Profiling.")
            return 1
        trace = new_traces[-1]

    events_csv = os.path.join(PROFILING, f"{args.label}_events.csv")
    log = os.path.join(PROFILING, f"{args.label}_insights.log")
    export = (f"TimingInsights.ExportTimingEvents {events_csv} "
              "-columns=ThreadName,TimerName,StartTime,EndTime,Duration,Depth -threads=* -timers=*")
    # Insights only reads a quoted value when the quote comes straight after the '=', so build the command line by hand.
    insights = os.path.join(ENGINE, "Binaries", "Win64", "UnrealInsights.exe")
    command_line = (f'"{insights}" -OpenTraceFile="{trace}" -ABSLOG="{log}" -AutoQuit -NoUI '
                    f'-ExecOnAnalysisCompleteCmd="{export}" -log')
    subprocess.run(command_line, check=True, timeout=600)

    frames = 0
    game_tick = 0.0
    render_busy = 0.0
    gpu_busy = 0.0
    watched = defaultdict(float)
    with open(events_csv, encoding="utf-8-sig") as file:
        for row in csv.DictReader(file):
            name, thread, depth = row["TimerName"], row["ThreadName"], int(row["Depth"])
            duration = float(row["Duration"])
            if not math.isfinite(duration):
                continue  # still open when the trace stopped
            if name == "FEngineLoop::Tick":
                frames += 1
                game_tick += duration
            # Insights names the threads "RenderThread 0" and "GPU0-Graphics0".
            if depth == 0 and thread.startswith("RenderThread"):
                render_busy += duration
            if depth == 0 and thread.startswith("GPU0-Graphics"):
                gpu_busy += duration
            if name in WATCHED_TIMERS:
                watched[name] += duration

    if frames == 0:
        print("No frames found in the trace.")
        return 1
    frame_ms = game_tick * 1000 / frames
    print(f"Trace: {os.path.basename(trace)}  ({frames} frames, ~{1000 / frame_ms:.0f} fps)")
    print(f"Game thread frame (FEngineLoop::Tick): {game_tick * 1000 / frames:6.2f} ms")
    print(f"Render thread busy:                    {render_busy * 1000 / frames:6.2f} ms")
    print(f"GPU graphics queue busy:               {gpu_busy * 1000 / frames:6.2f} ms")
    for name in WATCHED_TIMERS:
        print(f"  {name:62} {watched[name] * 1000 / frames:6.3f} ms")
    return 0


if __name__ == "__main__":
    sys.exit(main())
