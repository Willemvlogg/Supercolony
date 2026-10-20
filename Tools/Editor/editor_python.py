"""Run one Python statement inside the running Supercolony editor via Python Remote Execution.

Requires Project Settings > Plugins > Python > Enable Remote Execution (loopback only, TTL 0).
Uses the engine's own client, Engine/Plugins/Experimental/PythonScriptPlugin/Content/Python/remote_execution.py.

Usage:
    python editor_python.py "unreal.SystemLibrary.quit_editor()"

Exit codes: 0 sent (and succeeded if a result came back), 1 the command failed, 2 no editor found.
"""
import argparse
import os
import sys
import time

DEFAULT_ENGINE_ROOT = r"C:\Program Files\Epic Games\UE_5.8\Engine"
PROJECT_NAME = "Supercolony"
DISCOVERY_SECONDS = 10.0


def load_remote_execution(engine_root):
    sys.path.insert(0, os.path.join(engine_root, "Plugins", "Experimental", "PythonScriptPlugin", "Content", "Python"))
    import remote_execution
    return remote_execution


def find_node(remote, timeout):
    deadline = time.time() + timeout
    while time.time() < deadline:
        for node in remote.remote_nodes:
            if node.get("project_name") == PROJECT_NAME:
                return node
        time.sleep(0.25)
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("statement", help="Python statement to run; 'import unreal' is added in front")
    parser.add_argument("--engine-root", default=DEFAULT_ENGINE_ROOT)
    parser.add_argument("--expect-exit", action="store_true",
                        help="The statement closes the editor, so a lost connection counts as success")
    args = parser.parse_args()

    remote_execution = load_remote_execution(args.engine_root)
    remote = remote_execution.RemoteExecution()
    remote.start()
    try:
        node = find_node(remote, DISCOVERY_SECONDS)
        if node is None:
            print(f"No {PROJECT_NAME} editor answered within {DISCOVERY_SECONDS:.0f} s. Is it running with Remote Execution enabled?")
            return 2

        remote.open_command_connection(node["node_id"])
        try:
            result = remote.run_command(f"import unreal; {args.statement}", exec_mode=remote_execution.MODE_EXEC_FILE)
        except Exception as error:
            if args.expect_exit:
                print(f"Sent; connection closed as the editor shut down ({type(error).__name__}).")
                return 0
            raise

        for line in result.get("output", []):
            print(line.get("output", "").rstrip())
        if not result.get("success", False):
            print(f"Command failed: {result.get('result')}")
            return 1
        return 0
    finally:
        remote.stop()


if __name__ == "__main__":
    sys.exit(main())
