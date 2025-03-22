#include "ArgMapping.h"

// stl
#include <iostream>
#include <iomanip>
#include <cmath>
#include <chrono>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <fstream>

// graph
#include "../../src/cluster.h"
#include "../../src/faiss/impl/HNSW.h"
#include "../../src/faiss/impl/ProductQuantizer.h"
#include "../../src/faiss/utils/io.h"
#include "../../src/faiss/index_io.h"

#include "evp.h"

// json
#include "../utils/config_parser.h"

#define RAPIDJSON_HAS_STDSTRING 1
// #define INTEGRITY_CHECK
#define NUM_THREADS 32

using namespace std;

string dataset = "";

int main(int argc, char **argv) {

    ArgMapping amap;
    amap.arg("d", dataset, "Dataset: [sift, trip, msmarco, laion]");
    amap.parse(argc, argv);

    Metadata md;
    
    cout << "-> Parsing json..." << endl;
    if(parseJson("../tests/config/config_ring_2.json", md, dataset)  != 0) {
        cerr << "Early exit..." << endl;
        return 0;
    }

    cout << "-> Loading files..." << endl;

    OptNode::n_neighbors = 2*md.M;
    OptNode::dim = md.dim;

    faiss::IndexHNSW* index = faiss::read_index(md.index_path.c_str(), 0);
    size_t nb, dim;
    float* xb = fvecs_read(md.base_path.c_str(), &dim, &nb);

    FILE* f = fopen(md.graph_cache_path.c_str(), "w");

    for(node_id_t nid = 0; nid < index->ntotal; nid++){
        for(int l = 1; l < index->hnsw.levels[nid]; l++){

            OptNode on = OptNode();
            on.set_id(nid);
            on.set_layer(l);

            size_t begin, end;
            index->hnsw.neighbor_range(nid, l, &begin, &end);
            std::vector<node_id_t> nb_list(
                index->hnsw.neighbors.begin() + begin,
                index->hnsw.neighbors.begin() + end
            );

            on.set_neighbors(nb_list);

            if(l >= 1){
                size_t begin, end;
                index->hnsw.neighbor_range(nid, l - 1, &begin, &end);
                std::vector<node_id_t> next_layer_nb_list(
                    index->hnsw.neighbors.begin() + begin,
                    index->hnsw.neighbors.begin() + end
                );
                if(l == 1){
                    next_layer_nb_list.resize(OptNode::n_neighbors / 2);
                }
                on.set_next_layer_neighbors(next_layer_nb_list);
            }


            size_t long_d = dim;
            size_t long_nid = nid;

            vector<float> coords;
            for(size_t offset = long_nid*long_d; offset < (long_nid+1) *long_d; offset++){
                coords.push_back(xb[offset]);
            }
            on.set_coord(coords);

            size_t total_written = 0;
            size_t bytes_to_write = on.data.size()*sizeof(int);
            while (total_written < bytes_to_write) {
                size_t written = fwrite(on.data.data() + total_written, 1, bytes_to_write - total_written, f);
                if (written == 0) {
                    if (ferror(f)) {
                        perror("Write error");
                        break;
                    }
                }
                total_written += written;
            }
        }
    }

    fclose(f);

    return 0;
}