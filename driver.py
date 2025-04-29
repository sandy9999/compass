import argparse
import subprocess
import sys

from script.artifact.task import *
from script.artifact.graph.ablation_study import render_msmarco_ablation
from script.artifact.graph.latency_breakdown import render_latency_breakdown

def run_performance():
    s_instance = create_instance(server_config, "ae-perf-server")
    c_instance = create_instance(client_config, "ae-perf-client")

    # accuracy
    run_compass_accuracy(s_instance)

    # latency
    run_compass_latency(s_instance, c_instance)

    # latency breakdown
    render_latency_breakdown()


    delete_instance(PROJECT_ID, ZONE, s_instance["name"])
    delete_instance(PROJECT_ID, ZONE, c_instance["name"])
    return 


def run_ablation():
    s_instance = create_instance(server_config, "ae-ablation-server")
    c_instance = create_instance(client_config, "ae-ablation-client")

    # accuracy 
    run_ablation_accuracy(s_instance)

    # latency
    run_ablation_latency(s_instance, c_instance)

    # render
    render_msmarco_ablation()

    delete_instance(PROJECT_ID, ZONE, s_instance["name"])
    delete_instance(PROJECT_ID, ZONE, c_instance["name"])

if __name__ == "__main__":
    try:
        parser = argparse.ArgumentParser(description="Script for artifact evaluation")
        
        parser.add_argument(
            "--task",
            choices=["latency", "accuracy", "ablation"],
            required=True,
            help="specify the task to run: latency, accuracy_latency, or ablation"
        )

        parser.add_argument(
            "--verbose", 
            action="store_true",
            help="enable verbose output")

        args = parser.parse_args()


        
    except subprocess.CalledProcessError as e:
        print("The command")
        cmd_str = " ".join("'" + a + "'" if " " in a else a for a in e.cmd)
        print(f"\t{cmd_str}")
        print(f"failed with exit code {e.returncode}")
        sys.exit(e.returncode)