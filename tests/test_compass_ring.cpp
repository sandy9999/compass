#include "ArgMapping.h"
#include "net_io_channel.h"

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

// oram
#include "RemoteServerStorage.h"
#include "RemoteServerRing.h"
#include "RandForOramInterface.h"
#include "RandomForOram.h"
#include "OramRing.h"

// graph
#include "../../src/cluster.h"
#include "../../src/faiss/impl/HNSW.h"
#include "../../src/faiss/impl/ProductQuantizer.h"
#include "../../src/faiss/utils/io.h"
#include "../../src/faiss/index_io.h"

// json
#include "utils/config_parser.h"

using namespace std;


double elapsed() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}


// ground truth labels @gt, results to evaluate @I with @nq queries, returns @gt_size-Recall@k where gt had max gt_size NN's per query
float compute_recall(faiss::idx_t* gt, int gt_size, faiss::idx_t* I, int nq, int k, int gamma=1) 
{
    // printf("compute_recall params: gt.size(): %ld, gt_size: %d, I.size(): %ld, nq: %d, k: %d, gamma: %d\n", gt.size(), gt_size, I.size(), nq, k, gamma);
    int n_1 = 0, n_10 = 0, n_100 = 0;
    double mrr = 0;
    for (int i = 0; i < nq; i++) { // loop over all queries
        // int gt_nn = gt[i * k];
        faiss::idx_t* first = gt + i*gt_size;
        faiss::idx_t* last = gt + i*gt_size + (k / gamma);
        std::vector<faiss::idx_t> gt_nns_tmp(first, last);
        // if (gt_nns_tmp.size() > 10) {
        //     printf("gt_nns size: %ld\n", gt_nns_tmp.size());
        // }
        // gt_nns_tmp.resize(k); // truncate if gt_size > k
        // std::set<faiss::idx_t> gt_nns_100(gt_nns_tmp.begin(), gt_nns_tmp.end());
        // gt_nns_tmp.resize(10);
        std::set<faiss::idx_t> gt_nns_10(gt_nns_tmp.begin(), gt_nns_tmp.end());
        gt_nns_tmp.resize(1);
        std::set<faiss::idx_t> gt_nns_1(gt_nns_tmp.begin(), gt_nns_tmp.end());
        // if (gt_nns.size() > 10) {
        //     printf("gt_nns size: %ld\n", gt_nns.size());
        // }
        for (int j = 0; j < k; j++) { // iterate over returned nn results
            // if (gt_nns_100.count(I[i * k + j])!=0) {
            //     if (j < 100 * gamma)
            //         n_100++;
            // }
            if (gt_nns_10.count(I[i * k + j])!=0) {
                if (j < 10 * gamma)
                    n_10++;
            }
            if (gt_nns_1.count(I[i * k + j])!=0) {
                if (j < 1 * gamma)
                    n_1++;
            }
        }
        // Calculate MRR
        for(int rank = 0; rank < k; rank++){
            if(I[i * k + rank] == gt[i * gt_size]){
                mrr += 1.0/(rank+1);
                break;
            }
        }
    }

    // BASE ACCURACY
    printf("* Base HNSW accuracy relative to exact search:\n");
    printf("\tR@1 = %.4f\n", n_1 / float(nq) );
    printf("\tR@10 = %.4f\n", n_10 / float(nq));
    printf("\tMRR = %.4f\n", mrr / double(nq));
    // printf("\tR@100 = %.4f\n", n_100 / float(nq)); // not sure why this is always same as R@10
    // printf("\t---Results for %ld queries, k=%d, N=%ld, gt_size=%d\n", nq, k, N, gt_size);
    return (n_10 / float(nq));
}

void fvecs_write(const char* fname, float* data, size_t d, size_t n) {
    FILE* f = fopen(fname, "w");
    if (!f) {
        fprintf(stderr, "could not open %s\n", fname);
        perror("");
        abort();
    }
    for (size_t i = 0; i < n; i++){
        fwrite(&d, 1, sizeof(int), f);
        fwrite(data + i*d, d, sizeof(float), f);
    }
    fclose(f);
}

void ivecs_write(const char* fname, int* data, size_t d, size_t n) {
    fvecs_write(fname, (float*)data, d, n);
}

inline void tprint(string s, double t0){
    cout << "[" << std::fixed << std::setprecision(3) << elapsed() - t0 << "s] " << s;
}


int party = 0;
int port = 8000;
string address = "127.0.0.1";

string dataset = "";

int main(int argc, char **argv) {
    ArgMapping amap;
    amap.arg("r", party, "Role of party: Server = 1; Client = 2");
    amap.arg("p", port, "Port Number");
    amap.arg("d", dataset, "Dataset: [sift, trip, msmarco, laion]");
    amap.arg("ip", address, "IP Address of server");
    amap.parse(argc, argv);

    cout << ">>> Setting up..." << endl;
    cout << "-> Role: " << party << endl;
    cout << "-> Address: " << address << endl;
    cout << "-> Port: " << port << endl;
    cout << "<<<" << endl << endl;

    bool isServer = party == 1;
    NetIO* io = new NetIO(isServer ? nullptr : address.c_str(), port);
    NetIO* bf_io = new NetIO(isServer ? nullptr : address.c_str(), port+1);

    Metadata md;
    
    cout << "-> Parsing json..." << endl;
    if(parseJson("../tests/config/config_ring.json", md, dataset) != 0) {
        cerr << "Early exit..." << endl;
        return 0;
    }

     // ORAM for graph
    struct RingOramConfig config(
        md.block_size, 
        md.real_bucket_size, 
        md.dummy_size, 
        md.evict_rate, 
        md.base_size,
        md.num_levels
    );

    OptNode::n_neighbors = 2*md.M;
    OptNode::dim = md.dim;

    Bucket::setMaxSize(1);
    SBucket::setCipherSize(compute_ctx_len((1 + md.block_size)*sizeof(int)));

    // cout << "block_size: " << md.block_size << endl;
    // cout << "bucket_size: " << md.bucket_size << endl;
    // cout << "sbucket_size: " << SBucket::getCipherSize() << endl;

    RemoteServerStorage* rss = new RemoteServerStorage(config.block_size, io, isServer, config.num_levels, md.integrity);

    RandForOramInterface* random = new RandomForOram();

    long comm;
    long round;

    double t0 = elapsed();

    if(isServer){
        // Config remote storage server
        tprint("Starting remote server... \n", t0);
        
        bool in_memory = true;
        RemoteServerRing server = RemoteServerRing(io, config.num_buckets, config.bucket_size, in_memory, md.integrity);

        tprint("Faster initialization...\n", t0);

        server.load(md.buckets_path.c_str());
        
        bool ready;
        bf_io->send_data(&ready, sizeof(bool));

        if(md.integrity){
            server.load_hash(md.hash_path.c_str());
            server.sync_hash();
        }

        tprint("Run server...\n", t0);

        comm = io->counter;
        round = io->num_rounds;
        
		server.RunServer();
    } else{

        // 14KB

        // Client
        tprint("Initializing ORAM... \n", t0);

        rss->setCapacity(config.num_buckets, md.integrity);
        OramInterface* oram = new OramRing(
            rss, 
            random, 
            config.block_size,
            config.real_bucket_size,
            config.dummy_size,
            config.evict_rate,
            config.num_blocks,
            md.num_levels,
            md.oram_cached_levels
        );

        tprint("Loading ORAM metadata\n", t0);
        ((OramRing*)oram)->loadMetaDataFromFile(md.metadata_path.c_str());

        // Loading data
        size_t k = 10;

        tprint("Loading prebuilt index\n", t0);

        faiss::IndexHNSW* index = faiss::read_index(md.index_path.c_str(), 0);

        // 276 MB

        tprint("Loading pq index\n", t0);

        faiss::ProductQuantizer* pq = faiss::read_pq(md.pq_path.c_str(), 0);
        pq->print_stats();

        // 284 MB


        tprint("Loading storage\n", t0);

        // size_t nb, dim;
        // float* xb = fvecs_read(md.base_path.c_str(), &dim, &nb);

        // 776 MB

        tprint("Clustering\n", t0);

        ObliviousBlockFetcherRing* obf = new ObliviousBlockFetcherRing(
            md.base_size, 
            index->hnsw.max_level + 1 ,
            oram
        );

        // bool tmp;
        // io->recv_data(&tmp, 1);        

        // 828 MB

        // For faster initialization only
        {
            obf->read_block_mapping(md.block_map_path.c_str());
            ((OramRing*)oram)->loadPositionMapFromFile(md.pos_map_path.c_str());
            
            bool ready;
            bf_io->recv_data(&ready, sizeof(bool));

            if(md.integrity){
                rss->sync_hash(config);
            }
            
        }

        // 852 MB

        index->set_graph_cache(md.ef_lowest_cached_layer, obf, md.graph_cache_path.c_str());
        // delete[] xb;

        // 113MB

        tprint("Loading queries\n", t0);
        size_t nq;
        float* xq;

        {
            size_t d2;
            xq = fvecs_read(md.query_path.c_str(), &d2, &nq);
            assert(md.dim == d2 || !"query does not have same dimension as train set");
        }

        tprint("Loading GT\n", t0);

        faiss::idx_t* gt; // nq * k matrix of ground-truth nearest-neighbors
        size_t gt_size;

        {
            size_t k1;         // nb of results per query in the GT
            printf("[%.3f s] Loading ground truth for %ld queries\n",
                elapsed() - t0,
                nq);

            // load ground-truth and convert int to long
            size_t nq2;
            int* gt_int = ivecs_read(md.gt_path.c_str(), &k1, &nq2);
            assert(nq2 == nq || !"incorrect nb of ground truth entries");
            // assert(k == k1 || !"incorrect k");

            gt = new faiss::idx_t[k1 * nq];
            gt_size = k1;
            for (int i = 0; i < k1 * nq; i++) {
                gt[i] = gt_int[i];
            }
            delete[] gt_int;
        }

        ((OramRing*)oram)->init_cache_top();

        // bool tmp;
        // io->recv_data(&tmp, 1);

        // faster test for first 100 queries
        nq = 10000;

        comm = io->counter;
        round = io->num_rounds;

        faiss::SearchParametersHNSW* params = new faiss::SearchParametersHNSW();
        params->efSpec = md.ef_spec;
        params->efNeighbor = md.ef_neighbor;
        params->efLayer = md.ef_lowest_cached_layer;
        params->efUpperSteps = md.ef_upper_steps;

        for(int efs = md.ef_search; efs <= md.ef_search; efs*=2){
            
            params->efSearch = efs;
            
            // output buffers
            faiss::idx_t* I = new faiss::idx_t[nq * k];
            float* D = new float[nq * k];

            tprint("Searching with efs: " + to_string(efs) + "\n", t0);

            double t1 = elapsed();

            faiss::SearchStats s_stats = index->oblivious_search(
                nq,
                xq,
                k,
                D,
                I,
                pq,
                params
            );

            unsigned long total = 0;
            for(int nrt : s_stats.nsteps_map[0]){
                total += nrt;
            }

            printf("No. of early reshuffles called: %d\n", ((OramRing*)oram)->cnt_early_reshuffle);

            printf("[%.3f s] Search time: %.3f s\n", elapsed() - t0, elapsed() - t1);

            printf("[%.3f s] Computing average round trip: %ld\n", elapsed() - t0, total / nq);

            printf("[%.3f s] Compute recalls\n", elapsed() - t0);

            compute_recall(
                gt, 
                gt_size,
                I,
                nq,
                k
            );

            int* result = new int[nq * k];

            for(int i = 0; i < nq * k; i++){
                result[i] = I[i];
            }
            
            delete[] result;
            delete[] I;
            delete[] D;
        }

        rss->CloseServer();
    }

    std::cout << "Communication cost: " << io->counter - comm << std::endl;
    std::cout << "Round: " << io->num_rounds - round << std::endl;

    return 0;
}
