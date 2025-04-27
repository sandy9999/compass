# Artifact Evaluation for Compass

This is the artifact for the paper "Compass: Encrypted Semantic Search with High Accuracy".


**WARNING:** This is an academic proof-of-concept prototype and has not received careful code review. This implementation is NOT ready for production use.

## Overview & Setup


## Reproducing results 

To reproduce the results in the paper, we have the following settings:



### Figure 6

### Figure 7

### Table 3

### Table 4

### Figure 8

### Throughput


## Dependencies

```bash
./script/config.sh
```

## Data

```bash
cd data 
./download_data.sh
```

## Build


```bash
mkdir build && cd build
cmake ..
make
```


## Run 

```bash
# server
./test_compass_ring r=1 d=sift

# client
./test_compass_ring r=2 d=sift
```

## Currently support datasets
sift, laion