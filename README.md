# Artifact Evaluation for Compass

This is the artifact for the paper **"Compass: Encrypted Semantic Search with High Accuracy"** (to appear in OSDI 2025).



**WARNING:** This is an academic proof-of-concept prototype and has not received careful code review. This implementation is NOT ready for production use.

## Overview 

This repository is structured as follows:
- **`data/`**: Contains source files, indices, and initial states for both clients and servers corresponding to each dataset used in the evaluation.
- **`script/`**: Includes scripts for uploading/downloading files to/from Google Cloud Storage, and scripts for artifact evaluation.
- **`src/`**: Implements the Compass protocol.
- **`tests/`**: Contains implementations for the Compass client and server, and code for initializing ORAM from a standard HNSW index.
  - `compass_init` is for building the initial client and server states based on an HNSW index and configuration.
  - `test_compass_ring` is a reference Compass client/server built on Ring ORAM.
  - `test_compass_accuracy` is a variant that leaks access pattern. This is for faster accuracy estimation.
  - `test_compass_tp` is a variant that keeps sending queries. This is for throughput experiments.
  - `config/config_ring.json` contains the parameter configuration for each of the evaluated dataset.
- **`third_party/`**: Contains third-party libraries utilized by Compass and baseline implementations.


## Setup

### Dependencies

```bash
./script/config.sh
```

### Dataset
We've uploaded the datasets, indices, and pre-built initial client and server states to a public Google Cloud Storage bucket `compass_osdi`. To download all the files, run

```bash
python3 ./script/gcs_download.py
```

### Build 

```bash
mkdir build && cd build
cmake ..
make
```

### Usage

```bash
Usage: ./test_compass_ring [ name=value ]...
        r       Role of party: Server = 1; Client = 2  # required
        p       Port Number  [ default=8000 ]
        d       Dataset: [sift, trip, msmarco, laion]  # required
        n       number of queries  [ default=config ]
        ip      IP Address of server  [ default=127.0.0.1 ]
        efspec  Size of speculation set  [ default=config ]
        efn     Size of directional filter  [ default=config ]
        batch   Disable batching  [ default=1 ]
        lazy    Disable lazy eviction  [ default=1 ]
        f_latency       Save latency  [ default= None]
        f_accuracy      Save accuracy  [ default= None]
        f_comm  Save communication  [ default= None]
```

Currently we support four datasets: `laion`, `sift`, `trip`, `msmarco`. To quickly verify the local dependencies, run the following commands in two separate terminals:

```bash
./test_compass_ring r=1 d=laion n=10 
./test_compass_ring r=2 d=laion n=10
```

For separate machines, run
```bash
# server
./test_compass_ring r=1 d=laion ip=$server_ip

# client
./test_compass_ring r=2 d=laion ip=$server_ip
```

### Network Simulation

We use `tcconfig` to limit each instance's bandwidth and latency. 

```bash
# fast network
tcset $device_name --delay 0.5ms --rate 3Gbps 

# slow network
tcset $device_name --delay 40ms --rate 400Mbps

# reset
tcdel $device_name --all
```


## Reproducing results 

To reproduce the paper's results, we provide a driver script (`driver.py`) and a provisioned GCP instance for artifact evaluators. The provisioned instance contains sufficient credentials for the script to launch testing instances during the experiments. 

**Note：** As our experiments take long time, we recommend running our script in a `tmux` session. Please avoid running multiple experiments at the same time. 


### Performance Experiments

[Approx. 2hrs] The performance experiments includes the accuracy and latency experiments.This command creates two instances (server and client) to run latency experiments. Accuracy experiments run locally on the server instance for faster results. After completion, results are fetched to './script/artifact/results/', and instances are terminated.

```bash
python3 driver.py --task performance --verbose
```

### Ablation study

[Approx. 2hrs] We perform the ablation study on `msmarco` dataset under `slow` network configuration. Similar to the `performance experiments`, two instances will be launched, and after the experiment, the results will be fetched to './script/artifact/results/'.

```bash
python3 driver.py --task ablation --verbose
```

### Figures
Once `performance` experiment is done, run following command to render figures or print tables. Figures are saved in pdf format under `eval_fig/`. The communication results may slightly differ from the results in the paper because of batching multiple paths during eviction. 

#### Figure 6

```bash
python3 driver.py --plot figure6
```

#### Figure 7

```bash
python3 driver.py --plot figure7
```

#### Table 3

```bash
python3 driver.py --plot table3
```

#### Table 4

```bash
python3 driver.py --plot table4
```

#### Figure 8

```bash
python3 driver.py --plot figure8
```

### Throughput
Our throughput experiments launches 25 client instances that keeps sending request and one server instance that stores the index for all clients. In our script we have one (non-stop) monitor thread that collects throughput (qps) from each client. To stop the throuput experiment, use key-board (Ctrl+C) interupt. After throuput experiment, please run the cleanup command to terminated all instances.

```bash
python3 driver.py --task throughput
```

### Cleanup
The following command terminate all artifact evaluation related instances.

```bash
python3 driver.py --task stop_instances
```

### Common Issues

Both `performance` and `ablation` experiments automatically terminate instances upon completion.  If an abnormal termination occurs (e.g., due to instance failure or GCP staging delays), run the cleanup command above before restarting the experiment.

### Contact us

If you run into any issues or have any questions, please contact us on HotCRP or via email at `jinhao.zhu@berkeley.edu`, and we will reply promptly!