import time
import threading
import paramiko
import subprocess

from ssh_utils import *
from gcp.gcp_utils import *

ZONE = "us-central1-c"
VPC_NAME = "skypilot-vpc"
IMAGE_NAME = "artifact"
PROJECT_ID = "encrypted-search-404320"
PRIVATE_KEY_PATH = os.path.expanduser("~/.ssh/compass_artifact.pem")
USER_NAME = "artifact"

client_config = {
    # "name": "ae-client",
    "machine_type": "n2-standard-8",
    "boot_disk_image": f"projects/{PROJECT_ID}/global/images/{IMAGE_NAME}",
    "disk_size_gb": 512,  # Optional, defaults to 10GB
    "disk_type" :f"zones/{ZONE}/diskTypes/pd-ssd",
    "zone": ZONE,
    "vpc_name": VPC_NAME,
    "project_id": PROJECT_ID

}

server_config = {
    # "name": "ae-server",
    "machine_type": "n2-highmem-64",
    "boot_disk_image": f"projects/{PROJECT_ID}/global/images/{IMAGE_NAME}",
    "disk_size_gb": 512,  # Optional, defaults to 10GB
    "disk_type" :f"zones/{ZONE}/diskTypes/pd-ssd",
    "zone": ZONE,
    "vpc_name": VPC_NAME,
    "project_id": PROJECT_ID
}

compass_datasets = ["laion", "laion_mal", "sift", "sift_mal", "trip", "trip_mal", "msmarco", "msmarco_mal"]
# compass_datasets = ["laion"]

def prepare_instance(instance):
    cmds = [
    "ps aux | grep compass | grep -v grep | awk '{print $2}' | xargs kill",
    "cd /home/artifact/compass/ && git pull",
    "cd /home/artifact/compass/build/ && make"
    ]

    execute_commands_queit(
        instance["name"],
        instance["internal_ip"],
        cmds,
        PRIVATE_KEY_PATH,
        USER_NAME,
        False
    )


# accuracy only requires one instance
def run_compass_accuracy(instance):

    # prepare instance
    print("Preparing instance...")
    prepare_instance(instance)

    print("Run exp...")
    # run accuracy exp 
    # client_cmds = []
    # server_cmds = []

    accuracy_file_list = []

    for d in compass_datasets:
        print("-> dataset: ", d)
        f_accuracy = "accuracy_" + d + ".ivecs"
        accuracy_file_list.append(f_accuracy)

        s = "cd compass/build/ && " + f"./test_compass_ring r=1 d={d}"
        c = "cd compass/build/ && " + f"./test_compass_ring r=2 d={d} f_accuracy={f_accuracy}"
        
        # client_cmds.append(s)
        # server_cmds.append(c)

        threads = []

        s_thread = threading.Thread(target=execute_commands_queit, args=(instance["name"], instance["internal_ip"], [s], PRIVATE_KEY_PATH, USER_NAME, False))
        c_thread = threading.Thread(target=execute_commands_queit, args=(instance["name"], instance["internal_ip"], [c], PRIVATE_KEY_PATH, USER_NAME, False))
        
        threads.append(s_thread)
        threads.append(c_thread)
        s_thread.start()
        c_thread.start()

        for thread in threads:
            thread.join()
        
    # retrieve result
    remote_fpath_list = ["/home/artifact/compass/build/" + f for f in accuracy_file_list]


    scp_from_remote(
        instance["internal_ip"],
        USER_NAME,
        PRIVATE_KEY_PATH,
        remote_fpath_list, 
        "./script/artifact/results/"
    )

    return 

def run_baseline_accuracy():
    return


if __name__ == "__main__":
    os.chdir(os.path.expanduser('~/compass/'))
    # (sname, internal_ip) = create_instance(server_config, "ae-server")
    # instance = {
    #     "name": sname,
    #     "internal_ip": internal_ip
    # }

    # private_key = paramiko.RSAKey.from_private_key_file(PRIVATE_KEY_PATH)

    instance = {
        "name": "ae_server",
        "internal_ip": "10.128.0.26"
    }
    run_compass_accuracy(instance)
    # sname = "ae-server"
    # delete_instance(PROJECT_ID, ZONE, sname)

