# Artifact Evaluation for Compass

This is the artifact for the paper "Compass: Encrypted Semantic Search with High Accuracy". (To appear in OSDI 2025).


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
- **`third_party/`**: Contains third-party libraries utilized by Compass and baseline implementations.


## Setup

### Dependencies

```bash
./script/config.sh
```

### Dataset
We've uploaded the datasets, indices, and pre-built initial client and server states to a public Google Cloud Storage bucket. To download all the files, run

```bash
python3 ./script/gcp_download.py
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
        f_latency       Save latency  [ default= ]
        f_accuracy      Save accuracy  [ default= ]
        f_comm  Save communication  [ default= ]
```

Currently we support four datasets: `laion`, `sift`, `trip`, `msmarco`. To quickly verify the local dependencies, run the following commands in two separate terminals:

```bash
./test_compass_ring r=1 d=laion n=10 
./test_compass_ring r=2 d=laion n=10
```

For separate machines, run
```bash
# server
./test_compass_ring r=1 d=laion n=10 ip=$server_ip

# client
./test_compass_ring r=2 d=laion n=10 ip=$server_ip
```

### Network Simulation

We use `tcconfig` to limite each instance's bandwidth and latency. Here are some 

```bash
# fast network
tcset $device_name --delay 0.5ms --rate 3Gbps 

# slow network
tcset $device_name --delay 40ms --rate 400Mbps
```



## Artifact Evaluation - Reproducing results 

To reproduce the results in the paper, we have prepared a driver script `driver.py` and provisioned a GCP instance for our artifact evaluators. This script needs to be run within the server for launching instances for benchmarking.


### Performance Experiments

```bash
python3 driver.py --task performance
```

### Ablation study



### Figure 6

### Figure 7

### Table 3

### Table 4

### Figure 8

### Throughput
