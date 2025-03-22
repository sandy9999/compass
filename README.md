# compass



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