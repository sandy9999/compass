import argparse
import subprocess
import sys

datasets = ["laion", "sift"]
mal_datasets = ["laion_mal", "sift_mal"]

server_ip = "10.128.0.11"

def tc_reset(verbose):
    cmd = ["tcdel", "ens4", "--all"]
    subprocess.run(cmd, capture_output=not verbose, check=True)

def tc_set_slow(verbose):
    cmd = ["tcset", "ens4", "--delay", "40ms", "--rate", "400Mbps"]
    subprocess.run(cmd, capture_output=not verbose, check=True)

def tc_set_fast(verbose):
    cmd = ["tcset", "ens4", "--delay", "0.5ms", "--rate", "3Gbps"]
    subprocess.run(cmd, capture_output=not verbose, check=True)

def run_latency(role, verbose):

    # role
    r = 1 if role == "server" else 2

    # number of queries
    n =100

    tc_reset(verbose)

    print("> Simulate fast network")
    tc_set_fast(verbose)
    for d in datasets:
        f_latency = "latency_fast_" + d + ".fvecs"
        cmd = ["./test_compass_ring", "r=" + str(r), "d=" + d, "ip="+server_ip, "n="+str(n), "f_latency="+f_latency]
        print("Executing: ", cmd)
        subprocess.run(cmd, cwd="./build/", capture_output=not verbose, check=True)

    print("> Simulate slow network")
    tc_reset(verbose)
    tc_set_slow(verbose)
    for d in datasets:
        f_latency = "latency_slow_" + d + ".fvecs"
        cmd = ["./test_compass_ring", "r=" + str(r), "d=" + d, "ip="+server_ip, "n="+str(n), "f_latency="+f_latency]
        print("Executing: ", cmd)
        subprocess.run(cmd, cwd="./build/", capture_output=not verbose, check=True)
    
    tc_reset(verbose)
    return

def run_accuracy():
    return

def run_ablation():
    return

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
            "--role",
            choices=["server", "client"],
            help="role for latency task: server or client"
        )

        parser.add_argument(
            "--verbose", 
            action="store_true",
            help="enable verbose output")

        args = parser.parse_args()

        if args.task == "latency":
            if not args.role:
                print("Error: --role is required when task is 'latency'")
                sys.exit(1)
            run_latency(args.role, args.verbose)
        elif args.task == "accuracy":
            run_accuracy()
        elif args.task == "ablation":
            run_ablation()
        
    except subprocess.CalledProcessError as e:
        print("The command")
        cmd_str = " ".join("'" + a + "'" if " " in a else a for a in e.cmd)
        print(f"\t{cmd_str}")
        print(f"failed with exit code {e.returncode}")
        sys.exit(e.returncode)