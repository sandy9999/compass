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
// #include "RemoteServerStorage.h"
// #include "RemoteServerRing.h"
// #include "RemoteRing.h"
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

inline void tprint(string s, double t0){
    cout << "[" << std::fixed << std::setprecision(3) << elapsed() - t0 << "s] " << s;
}


int party = 0;
int port = 8000;
string address = "127.0.0.1";
string dataset = "";
int n = -1;

int efspec = -1;
int efn = -1;
bool batching = true;
bool lazy_evict = true;
string f_latency = "";
string f_accuracy = "";
string f_comm = "";

int main(int argc, char **argv) {
    ArgMapping amap;
    amap.arg("r", party, "Role of party: Server = 1; Client = 2");
    amap.arg("p", port, "Port Number");
    amap.arg("d", dataset, "Dataset: [sift, trip, msmarco, laion]");
    amap.arg("n", n, "# queries");
    amap.arg("ip", address, "IP Address of server");
    amap.arg("efspec", efspec, "Size of speculation set");
    amap.arg("efn", efn, "Size of directional filter");
    amap.arg("batch", batching, "Disable batching");
    amap.arg("lazy", lazy_evict, "Disable lazy eviction");
    amap.arg("f_latency", f_latency, "Save latency");
    amap.arg("f_accuracy", f_accuracy, "Save accuracy");
    amap.arg("f_comm", f_comm, "Save communication");
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
        md.num_levels,
        md.oram_cached_levels
    );

    OptNode::n_neighbors = 2*md.M;
    OptNode::dim = md.dim;

    Bucket::setMaxSize(1);
    SBucket::setCipherSize(compute_ctx_len((1 + md.block_size)*sizeof(int)));

    // cout << "block_size: " << md.block_size << endl;
    // cout << "bucket_size: " << md.bucket_size << endl;
    // cout << "sbucket_size: " << SBucket::getCipherSize() << endl;

    // RemoteServerStorage* rss = new RemoteServerStorage(config.block_size, io, isServer, config.num_levels, md.integrity, config);

    RandForOramInterface* random = new RandomForOram();

    bool in_memory = true;
    RemoteRing* remote_storage = new RemoteRing(io, config, isServer, in_memory, md.integrity);

    long comm;
    long round;

    double t0 = elapsed();

    if(isServer){
        // Config remote storage server
        tprint("Starting remote server... \n", t0);
        
        bool in_memory = true;

        tprint("Faster initialization...\n", t0);

        remote_storage->load_server_state(md.buckets_path.c_str());
        
        bool ready;
        bf_io->send_data(&ready, sizeof(bool));

        if(md.integrity){
            remote_storage->load_server_hash(md.hash_path.c_str());
            remote_storage->sync_roots();
        }

        tprint("Run server...\n", t0);

        comm = io->counter;
        round = io->num_rounds;
        
		remote_storage->run_server();

        if(f_comm != ""){
            long final_comm = io->counter - comm;
            io->send_data(&final_comm, sizeof(long));
        }

        
        long final_comm = io->counter - comm;
        io->send_data(&final_comm, sizeof(long));
        
    } else{
        // Client
        tprint("Initializing ORAM... \n", t0);

        OramInterface* oram = new OramRing(
            remote_storage, 
            random, 
            config,
            md.num_levels,
            md.oram_cached_levels,
            batching,
            lazy_evict
        );

        tprint("Loading ORAM metadata\n", t0);
        ((OramRing*)oram)->loadMetaDataFromFile(md.metadata_path.c_str());

        // # search result
        size_t k = 10;

        tprint("Loading prebuilt index\n", t0);

        faiss::IndexHNSW* index = faiss::read_index(md.index_path.c_str(), 0);

        tprint("Loading pq index\n", t0);

        faiss::ProductQuantizer* pq = faiss::read_pq(md.pq_path.c_str(), 0);
        pq->print_stats();

        tprint("Clustering\n", t0);

        ObliviousBlockFetcherRing* obf = new ObliviousBlockFetcherRing(
            md.base_size, 
            index->hnsw.max_level + 1 ,
            oram
        );

        // bool tmp;
        // io->recv_data(&tmp, 1);        

        // Read client states
        {
            obf->read_block_mapping(md.block_map_path.c_str());

            // stash
            ((OramRing*)oram)->loadPositionMapFromFile(md.pos_map_path.c_str());
            
            bool ready;
            bf_io->recv_data(&ready, sizeof(bool));

            if(md.integrity){
                remote_storage->sync_roots();
            }

            // Graph cache
            index->set_graph_cache(md.ef_lowest_cached_layer, obf, md.graph_cache_path.c_str());

            // ORAM cache
            ((OramRing*)oram)->init_cache_top();
            remote_storage->server_comm_for_oram_access = 0;
            remote_storage->server_comm_for_reshuffles = 0;
            // remote_storage->server_comm_for_evictions = 0;

            remote_storage->comm_for_oram_access = 0;
            remote_storage->comm_for_reshuffles = 0;
            remote_storage->comm_for_evictions = 0;

            remote_storage->rounds_for_oram_access = 0;
            remote_storage->rounds_for_reshuffles = 0;
            remote_storage->rounds_for_evictions = 0;
        }

        tprint("Loading queries\n", t0);
        size_t nq;
        float* xq;

        {
            size_t d2;
            xq = fvecs_read(md.query_path.c_str(), &d2, &nq);
            n = n == -1 ? nq : n;
            assert(md.dim == d2 || !"query does not have same dimension as train set");
            assert(n <= nq || !"# queries larger than query set");
        }

        tprint("Loading GT\n", t0);

        faiss::idx_t* gt; // nq * gt_size matrix of ground-truth nearest-neighbors
        size_t gt_size;

        {

            tprint("Loading ground truth for " + to_string(nq) + "queries\n", t0);

            // load ground-truth and convert int to long
            size_t nq2;
            int* gt_int = ivecs_read(md.gt_path.c_str(), &gt_size, &nq2);
            assert(nq2 == nq || !"incorrect nb of ground truth entries");
            // assert(k == k1 || !"incorrect k");

            gt = new faiss::idx_t[gt_size * nq];
            for (int i = 0; i < gt_size * nq; i++) {
                gt[i] = gt_int[i];
            }
            delete[] gt_int;
        }

        // bool tmp;
        // io->recv_data(&tmp, 1);

        nq = n;

        comm = io->counter;
        round = io->num_rounds;

        faiss::SearchParametersHNSW* params = new faiss::SearchParametersHNSW();
        params->efSpec = md.ef_spec;
        params->efNeighbor = md.ef_neighbor;
        params->efLayer = md.ef_lowest_cached_layer;
        params->efUpperSteps = md.ef_upper_steps;
        params->efSearch = md.ef_search;

        if(efn != -1){
            params->efNeighbor = efn;
        }

        if(efspec != -1){
            params->efSpec = efspec;
        }
        
        // output buffers
        faiss::idx_t* I = new faiss::idx_t[nq * k];
        float* D = new float[nq * k];

        tprint("Searching with efs: " + to_string(md.ef_search) + "\n", t0);

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

        if(f_accuracy != ""){
            // write result if search over the whole query set
            // string result_path = dataset + ".ivecs";
            ivecs_write(
                f_accuracy.c_str(), 
                result, 
                k,
                nq
            );
        }

        if(f_latency != ""){
            // string perceived_latency_path = "perceived_latency_" + dataset + ".bin";
            // string full_latency_path = "full_latency_" + dataset + ".bin";

            vector<float> latency = s_stats.perceived_latency;
            latency.insert(latency.end(), s_stats.full_latency.begin(), s_stats.full_latency.end());

            fvecs_write(
                f_latency.c_str(),
                latency.data(),
                latency.size(),
                1
            );
        }

        double perceived_latency_sum = std::accumulate(s_stats.perceived_latency.begin(), s_stats.perceived_latency.end(), 0.0);
        double avg_perceived_latency = perceived_latency_sum / n;

        double full_latency_sum = std::accumulate(s_stats.full_latency.begin(), s_stats.full_latency.end(), 0.0);
        double avg_full_latency = full_latency_sum / n;

        std::cout << "Avg. perceived latency: " << avg_perceived_latency << std::endl;
        std::cout << "Avg. full latency: " <<  avg_full_latency << std::endl;

        delete params;

        delete[] result;
        delete[] I;
        delete[] D;
        delete[] gt;
        
        remote_storage->close_server();
        io->num_rounds--;
        io->counter -= sizeof(int);

        if(f_comm != ""){
            long final_comm = io->counter - comm;
            long final_rnd = io->num_rounds - round;
            long server_comm;
            io->recv_data(&server_comm, sizeof(long));

            FILE* file = fopen(f_comm.c_str(), "w");
            if (file != nullptr) {
                fprintf(file, "%ld %ld\n", final_comm+server_comm, final_rnd);
                fclose(file);
            } else {
                perror("Error opening file");
            }
        }

        
        long server_comm;
        io->recv_data(&server_comm, sizeof(long));
        io->num_rounds--;

        std::cout << "Communication: " << (io->counter - comm + server_comm)*1.0/(1024*1024) << std::endl;
        std::cout << "---> Oram Access: " << (remote_storage->comm_for_oram_access + remote_storage->server_comm_for_oram_access)*1.0/(1024*1024) << std::endl;
        std::cout << "---> Reshuffles: " << (remote_storage->comm_for_reshuffles + remote_storage->server_comm_for_reshuffles)*1.0/(1024*1024) << std::endl;
        std::cout << "---> Evictions: " << (remote_storage->comm_for_evictions + remote_storage->server_comm_for_evictions)*1.0/(1024*1024) << std::endl;

        std::cout << "Round: " << io->num_rounds - round << std::endl;
        std::cout << "---> Oram Access: " << (remote_storage->rounds_for_oram_access) << std::endl;
        std::cout << "---> Reshuffles: " << (remote_storage->rounds_for_reshuffles) << std::endl;
        std::cout << "---> Evictions: " << (remote_storage->rounds_for_evictions) << std::endl;

    }

    // delete io;
    // delete bf_io;
    return 0;
}