import os

def parse_compass_comm(fname):
    with open(fname, "r") as f:
        val1, val2 = map(int, f.read().split())
        return val1 / 100, int(val2 / 200)

def parse_tfidf_comm(fname):
    with open(fname, "r") as f:
        val1, val2 = map(int, f.read().split())
        return val1 / 10, int(val2 / 20)

def parse_cluster_comm(fname):
    with open(fname, "r") as f:
        val1, val2 = map(int, f.read().split())
        return val1 , int(val2 / 2)


if __name__ == "__main__":
    os.chdir(os.path.expanduser('~/compass/'))
    compass_datasets = ["laion", "laion_mal", "sift", "sift_mal", "trip", "trip_mal", "msmarco", "msmarco_mal"]
    for d in compass_datasets:
        print(d)
        print("./script/artifact/results/comm")