import os

def get_size_mb(fname):
    file_size_bytes = os.path.getsize(fname)
    file_size_mb = file_size_bytes / (1024 * 1024)
    return file_size_mb

def get_size_gb(fname):
    file_size_bytes = os.path.getsize(fname)
    file_size_gb = file_size_bytes / (1024 * 1024 * 1024)
    return file_size_gb

def pretty_print_mem():
    os.chdir(os.path.expanduser('~/compass/'))
    print("# LAION: ")
    print("--> Embed Size: {:.2f} GB".format(get_size_gb("data/dataset/laion1m/100k/laion_base.fvecs")))
    print("--> Graph Size: {:.2f} GB".format(get_size_gb("data/dataset/laion1m/100k/hnsw_64_80_2_ip.index")))
    print("--> Server SH: {:.2f} GB".format(get_size_gb("data/snap/server/laion/buckets.bin")))
    print("--> Server MAL: {:.2f} GB".format(get_size_gb("data/snap/server/laion/buckets.bin") + get_size_gb("data/snap/client/laion/hash.bin")))
    print("--> Client Hints: {:.2f} MB".format(get_size_mb("data/dataset/laion1m/100k/pq_full_32_8_ip.index")))
    print("--> Client ORAM: {:.2f} MB".format(get_size_mb("data/snap/client/laion/position_map.bin") + get_size_mb("data/snap/client/laion/metadata.bin")))
    print("--> Client Graph: {:.2f} MB".format(get_size_mb("data/snap/client/laion/graph_cache.bin")))
    total = get_size_mb("data/dataset/laion1m/100k/pq_full_32_8_ip.index") + get_size_mb("data/snap/client/laion/position_map.bin") + get_size_mb("data/snap/client/laion/metadata.bin") + get_size_mb("data/snap/client/laion/graph_cache.bin")
    print("--> Client Total: {:.2f} MB".format(total))

    print("# SIFT1M: ")
    print("--> Embed Size: {:.2f} GB".format(get_size_gb("data/dataset/sift/base.fvecs")))
    print("--> Graph Size: {:.2f} GB".format(get_size_gb("data/dataset/sift/hnsw_64_80_2_ip.index")))
    print("--> Server SH: {:.2f} GB".format(get_size_gb("data/snap/server/sift/buckets.bin")))
    print("--> Server MAL: {:.2f} GB".format(get_size_gb("data/snap/server/sift/buckets.bin") + get_size_gb("data/snap/client/sift/hash.bin")))
    print("--> Client Hints: {:.2f} MB".format(get_size_mb("data/dataset/sift/pq_full_8.index")))
    print("--> Client ORAM: {:.2f} MB".format(get_size_mb("data/snap/client/sift/position_map.bin") + get_size_mb("data/snap/client/sift/metadata.bin")))
    print("--> Client Graph: {:.2f} MB".format(get_size_mb("data/snap/client/sift/graph_cache.bin")))
    total = get_size_mb("data/dataset/sift/pq_full_8.index") + get_size_mb("data/snap/client/sift/position_map.bin") + get_size_mb("data/snap/client/sift/metadata.bin") + get_size_mb("data/snap/client/sift/graph_cache.bin")
    print("--> Client Total: {:.2f} MB".format(total))

    print("# TripClick: ")
    print("--> Embed Size: {:.2f} GB".format(get_size_gb("data/dataset/trip_distilbert/queries.fvecs")))
    print("--> Graph Size: {:.2f} GB".format(get_size_gb("data/dataset/trip_distilbert/hnsw_128_160_2_ip.index")))
    print("--> Server SH: {:.2f} GB".format(get_size_gb("data/snap/server/trip/buckets.bin")))
    print("--> Server MAL: {:.2f} GB".format(get_size_gb("data/snap/server/trip/buckets.bin") + get_size_gb("data/snap/client/trip/hash.bin")))
    print("--> Client Hints: {:.2f} MB".format(get_size_mb("data/dataset/trip_distilbert/pq_full_32_ip.index")))
    print("--> Client ORAM: {:.2f} MB".format(get_size_mb("data/snap/client/trip/position_map.bin") + get_size_mb("data/snap/client/trip/metadata.bin")))
    print("--> Client Graph: {:.2f} MB".format(get_size_mb("data/snap/client/trip/graph_cache.bin")))
    total = get_size_mb("data/dataset/trip_distilbert/pq_full_32_ip.index") + get_size_mb("data/snap/client/trip/position_map.bin") + get_size_mb("data/snap/client/trip/metadata.bin") + get_size_mb("data/snap/client/trip/graph_cache.bin")
    print("--> Client Total: {:.2f} MB".format(total))

    print("# MSMARCO: ")
    print("--> Embed Size: {:.2f} GB".format(get_size_gb("data/dataset/msmarco_bert/passages.fvecs")))
    print("--> Graph Size: {:.2f} GB".format(get_size_gb("data/dataset/msmarco_bert/hnsw_128_200_2_ip.index")))
    print("--> Server SH: {:.2f} GB".format(get_size_gb("data/snap/server/msmarco/buckets.bin")))
    print("--> Server MAL: {:.2f} GB".format(get_size_gb("data/snap/server/msmarco/buckets.bin") + get_size_gb("data/snap/client/msmarco/hash.bin")))
    print("--> Client Hints: {:.2f} MB".format(get_size_mb("data/dataset/msmarco_bert/pq_full_32_ip.index")))
    print("--> Client ORAM: {:.2f} MB".format(get_size_mb("data/snap/client/msmarco/position_map.bin") + get_size_mb("data/snap/client/msmarco/metadata.bin")))
    print("--> Client Graph: {:.2f} MB".format(get_size_mb("data/snap/client/msmarco/graph_cache.bin")))
    total = get_size_mb("data/dataset/msmarco_bert/pq_full_32_ip.index") + get_size_mb("data/snap/client/msmarco/position_map.bin") + get_size_mb("data/snap/client/msmarco/metadata.bin") + get_size_mb("data/snap/client/msmarco/graph_cache.bin")
    print("--> Client Total: {:.2f} MB".format(total))


if __name__ == "__main__":
    pretty_print_mem()