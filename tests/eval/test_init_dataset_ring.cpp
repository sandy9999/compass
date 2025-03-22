#include "ArgMapping.h"
#include "net_io_channel.h"
#include "evp.h"

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
#include "RemoteServer.h"
#include "RandForOramInterface.h"
#include "RandomForOram.h"

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

size_t get_sibling3(size_t me){
	if(me % 2 == 1){
		// odd
		return me + 1;
	} else if (me == 0){
		return 0;
	} else{
		return me - 1;
	}
}

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

    faiss::IndexHNSW* index = faiss::read_index(md.index_path.c_str(), 0);
    size_t nb, dim;
    float* xb = fvecs_read(md.base_path.c_str(), &dim, &nb);

    RandForOramInterface* random = new RandomForOram();

    FakeBlockFetcherRing* fbf = new FakeBlockFetcherRing(
        nb, 
        index->hnsw.max_level+1, 
        config,
        random
    );

    cout << "-> Building ORAM Tree w/o dummies..." << endl;

    fully_naive_cluster(fbf, index, xb, vector<uint8_t>());

    delete [] xb;

    cout << "-> Encrypting ORAM Tree..." << endl;
    int per_block_size = (1 + config.block_size) * sizeof(int);

    std::vector<SBucket*> buckets;
    buckets.resize(config.num_buckets * config.bucket_size);

    vector<int> block_ids;
    block_ids.resize(config.num_buckets * config.bucket_size);

    #pragma omp parallel for num_threads(NUM_THREADS)
	for(int i = 0; i < config.num_buckets; i++){
        // fill in the real blocks
        for(int j = 0; j < config.real_bucket_size; j++){
            SBucket* sbkt = new SBucket(md.integrity);
            Bucket* bkt = fbf->bkts[i*config.real_bucket_size + j];

            unsigned char* payload = new unsigned char[SBucket::getCipherSize()];

            vector<Block> blocks = bkt->getBlocks();
            assert(blocks.size() == 1);
            
            blocks[0].to_ptr(payload);
            block_ids[i*config.bucket_size + j] = blocks[0].index;

            // Encrypt and write
            int ctx_len = encrypt_wrapper(payload, per_block_size*Bucket::getMaxSize(), sbkt->data);
            
            buckets[i*config.bucket_size + j] = sbkt;
            assert(ctx_len == SBucket::getCipherSize());
            delete bkt;
            delete[] payload;
        }

        // fill in the dummy blocks
        for(int j = config.real_bucket_size; j < config.bucket_size; j++){
            SBucket* sbkt = new SBucket(md.integrity);
            unsigned char* payload = new unsigned char[SBucket::getCipherSize()];
            Block dummy = Block(md.block_size);
            dummy.to_ptr(payload);
            
            block_ids[i*config.bucket_size + j] = -1;
            int ctx_len = encrypt_wrapper(payload, per_block_size*Bucket::getMaxSize(), sbkt->data);
        
            buckets[i*config.bucket_size + j] = sbkt;
            assert(ctx_len == SBucket::getCipherSize());
            delete[] payload;
        }
	}

    assert(block_ids.size() == config.num_buckets * config.bucket_size);

    // integrity
    if(md.integrity){
        cout << "Construct Merkle Tree... " << endl;
        // Assume the oram tree is an binary tree
        // , where 2*i for left child and 2*i + 1 for right child

        size_t num_buckets = config.num_buckets;
        size_t bucket_size = config.bucket_size;

        // compute hash for each block 
        cout << "-> Compute hash for each block " << endl;
        size_t num_total_blocks = num_buckets * bucket_size;
        uint8_t* per_block_hash = new uint8_t[SHA256_DIGEST_LENGTH * num_total_blocks];
        for(size_t i = 0; i < num_total_blocks; i++){
            sha256_wrapper(buckets[i]->data, SBucket::getCipherSize(), per_block_hash + i*SHA256_DIGEST_LENGTH);
        }

        // compute hash for each bucket
        cout << "-> Compute hash for each bucket " << endl;
        // some metadata:
        size_t bucket_tree_height = ceil(log10(bucket_size) / log10(2)) + 1;
        size_t bucket_num_hashes = pow(2, bucket_tree_height) - 1;
        size_t per_bucket_hash_size = SHA256_DIGEST_LENGTH * bucket_num_hashes * (num_buckets);
        uint8_t* per_bucket_hash = new uint8_t[per_bucket_hash_size];
        memset(per_bucket_hash, 0, per_bucket_hash_size);

        cout << "-> tree height:  " << bucket_tree_height << endl;
        cout << "-> num hashes:  " << bucket_num_hashes << endl;

        for(size_t i = 0; i < num_buckets; i++){
            // construct secondary tree for each bucket
            // size_t bucket_offset = i * bucket_num_hashes * SHA256_DIGEST_LENGTH;
            for(size_t j = bucket_num_hashes; j >= 1; j--){
                size_t id = j-1;
                size_t lid = 2*j-1;
                size_t rid = 2*j;
                if(lid > bucket_num_hashes - 1){
                    // leaf
                    size_t offset = j - (bucket_num_hashes + 1) / 2;
                    if(offset < bucket_size){
                        uint8_t* src = per_block_hash + i * bucket_size * SHA256_DIGEST_LENGTH + offset * SHA256_DIGEST_LENGTH;
                        uint8_t* dst = per_bucket_hash + i * bucket_num_hashes * SHA256_DIGEST_LENGTH + id * SHA256_DIGEST_LENGTH;
                        memcpy(dst, src, SHA256_DIGEST_LENGTH);
                    }
                } else{
                    // internal node
                    uint8_t* dst = per_bucket_hash + i * bucket_num_hashes * SHA256_DIGEST_LENGTH + id * SHA256_DIGEST_LENGTH;
                    uint8_t* lsrc = per_bucket_hash + i * bucket_num_hashes * SHA256_DIGEST_LENGTH + lid * SHA256_DIGEST_LENGTH;
                    uint8_t* rsrc = per_bucket_hash + i * bucket_num_hashes * SHA256_DIGEST_LENGTH + rid * SHA256_DIGEST_LENGTH;

                    sha256_twin_input_wrapper(
                        lsrc, SHA256_DIGEST_LENGTH,
                        rsrc, SHA256_DIGEST_LENGTH,
                        dst
                    );
                }
            }
        }

        // cout << "Verify Merkle Tree... " << endl;
  
        // for(size_t pos = 1; pos < num_buckets; pos++){
        //     cout << "-> verifying pos:  " << pos << endl;
        //     for(size_t offset = 0; offset < bucket_size; offset++){
        //         size_t num_hashes = (bucket_tree_height - 1);
        //         uint8_t* payload = new uint8_t[num_hashes*SHA256_DIGEST_LENGTH];
        //         size_t cur_offset = offset + (bucket_num_hashes + 1) / 2 - 1;

        //         // get sib hashes
        //         for(int cur_height = bucket_tree_height; cur_height > 1; cur_height --){
        //             size_t sib_offset = get_sibling3(cur_offset);
        //             // cout << "-> cur_offset:  " << cur_offset << " sib_offset: " << sib_offset << endl;
        //             uint8_t* src = per_bucket_hash + pos * bucket_num_hashes * SHA256_DIGEST_LENGTH + sib_offset * SHA256_DIGEST_LENGTH;
        //             uint8_t* dst = payload  + (bucket_tree_height - cur_height) * SHA256_DIGEST_LENGTH;
        //             memcpy(dst, src, SHA256_DIGEST_LENGTH);
        //             // cout << "sib_hash for height - " << cur_height << " cur_offset: " << cur_offset << ": " ;
        //             // for(int j = 0; j < 5; j++){
        //             // 	cout << (int)(dst[j]) << " ";
        //             // }
        //             // cout << endl;
        //             cur_offset = (cur_offset - 1) / 2;
        //         }

        //         // reconstruct root
        //         uint8_t* hash = new uint8_t[SHA256_DIGEST_LENGTH];
        //         memcpy(hash, per_block_hash + pos * bucket_size * SHA256_DIGEST_LENGTH + offset * SHA256_DIGEST_LENGTH, SHA256_DIGEST_LENGTH);
        //         cur_offset = offset + (bucket_num_hashes + 1) / 2 - 1;

        //         for(int cur_height = bucket_tree_height; cur_height > 1; cur_height --){
        //             uint8_t* sib_hash = payload + (bucket_tree_height - cur_height) * SHA256_DIGEST_LENGTH;
        //             // size_t sib_offset = get_sibling3(cur_offset);

        //             // cout << "sib_hash for height - " << cur_height << " cur_offset: " << cur_offset << ": " ;
        //             // for(int j = 0; j < 5; j++){
        //             // 	cout << (int)(sib_hash[j]) << " ";
        //             // }
        //             // cout << endl;

        //             // determine left and right
        //             uint8_t* l_hash = hash;
        //             uint8_t* r_hash = sib_hash;

        //             if(cur_offset % 2 == 0){
        //                 l_hash = sib_hash;
        //                 r_hash = hash;
        //             } 

        //             sha256_twin_input_wrapper(
        //                 l_hash, SHA256_DIGEST_LENGTH,
        //                 r_hash, SHA256_DIGEST_LENGTH,
        //                 hash
        //             );
        //             cur_offset = (cur_offset - 1) / 2;
        //         }

        //         // Now the hash should contain root hash
        //         uint8_t* ref = per_bucket_hash + pos * bucket_num_hashes * SHA256_DIGEST_LENGTH;
        //         for(int i = 0; i < SHA256_DIGEST_LENGTH; i++){
        //             if(ref[i] != hash[i]){
        //                 // Hash verification fails!
        //                 cout << "ref: ";
        //                 for(int j = 0; j < SHA256_DIGEST_LENGTH; j++){
        //                     cout << (int)(ref[j]) << " ";
        //                 }
        //                 cout << endl;
        //                 cout << "hash: ";
        //                 for(int j = 0; j < SHA256_DIGEST_LENGTH; j++){
        //                     cout << (int)(hash[j]) << " ";
        //                 }
        //                 cout << endl;
        //                 cout << "-> verifying fail pos:  " << pos << "offset: " << offset << endl;
        //                 assert(0);
        //                 break;
        //             }
        //         }

        //     }
        // }



        {
            FILE* f = fopen(md.hash_path.c_str(), "w");
            if (!f) {
                fprintf(stderr, "could not open %s\n", md.metadata_path.c_str());
                perror("");
                abort();
            }
            size_t total_written = 0;
            size_t bytes_to_write = per_bucket_hash_size;
            while (total_written < bytes_to_write) {
                size_t written = fwrite(per_bucket_hash + total_written, 1, bytes_to_write - total_written, f);
                if (written == 0) {
                    if (ferror(f)) {
                        perror("Write error");
                        break;
                    }
                }
                total_written += written;
            }
            fclose(f);
        }

        delete[] per_block_hash;
        delete[] per_bucket_hash;
    }

    // save results
    cout << "-> Saving results..." << endl;

    fbf->save_block_mapping(md.block_map_path.c_str());
    fbf->save_position_map(md.pos_map_path.c_str());

    // Save metadata
    {
        int s = block_ids.size();
        FILE* f = fopen(md.metadata_path.c_str(), "w");
        if (!f) {
            fprintf(stderr, "could not open %s\n", md.metadata_path.c_str());
            perror("");
            abort();
        }
        fwrite(&s, 1, sizeof(int), f);
        fwrite(block_ids.data(), 1, block_ids.size()*sizeof(int), f);
        fclose(f);
    }

    FILE* f = fopen(md.buckets_path.c_str(), "w");
    if (!f) {
        fprintf(stderr, "could not open %s\n", md.buckets_path.c_str());
        perror("");
        abort();
    }
	// Write capacity
	fwrite(&(config.num_buckets), 1, sizeof(int), f);
	// Write SBucket size
	int sb_size = SBucket::getCipherSize();
	fwrite(&sb_size, 1, sizeof(int), f);
	fwrite(&(md.integrity), 1, sizeof(bool), f);


    for (size_t i = 0; i < config.num_buckets * config.bucket_size; i++){
        size_t total_written = 0;
        size_t bytes_to_write = SBucket::getCipherSize();
        while (total_written < bytes_to_write) {
            size_t written = fwrite(buckets[i]->data, 1, bytes_to_write - total_written, f);
            if (written == 0) {
                if (ferror(f)) {
                    perror("Write error");
                    break;
                }
            }
            total_written += written;
        }
        // fwrite(buckets[i]->data, 1, SBucket::getCipherSize(), f);
		// if(md.integrity){
        //     fwrite(buckets[i]->hash, 32, sizeof(uint8_t), f);
        // }

        // SBucket* sbkt = buckets[i];
        // delete sbkt;
    }
    fclose(f);

    return 0;
}