/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// -*- c++ -*-

#pragma once

#include <queue>
#include <unordered_set>
#include <map>
#include <vector>
#include <iostream>
#include <omp.h>

// #include <faiss/Index.h>
#include "FaissAssert.h"
#include "../utils/Heap.h"
#include "../utils/random.h"
#include "../MetricType.h"
#include "../../graph.h"
#include "ProductQuantizer.h"

namespace faiss {

/** Implementation of the Hierarchical Navigable Small World
 * datastructure.
 *
 * Efficient and robust approximate nearest neighbor search using
 * Hierarchical Navigable Small World graphs
 *
 *  Yu. A. Malkov, D. A. Yashunin, arXiv 2017
 *
 * This implementation is heavily influenced by the NMSlib
 * implementation by Yury Malkov and Leonid Boystov
 * (https://github.com/searchivarius/nmslib)
 *
 * The HNSW object stores only the neighbor link structure, see
 * IndexHNSW.h for the full index object.
 */

struct SearchParametersHNSW {
    
    int efSearch = 32;
    int efSpec = 1;
    int efNeighbor = -1;
    int efLayer = -1;
    int efUpperSteps = -1;
    std::vector<int> steps_per_layer;

    bool check_relative_distance = false;
    ~SearchParametersHNSW() {}
};

struct VisitedTable;
struct GenericDistanceComputer; // from AuxIndexStructures
struct HNSWStats;

struct HNSW {
    /// internal storage of vectors (32 bits: this is expensive)
    using storage_idx_t = int32_t;

    typedef std::pair<float, storage_idx_t> Node;

    /** Heap structure that allows fast
     */
    struct MinimaxHeap {
        int n;
        int k;
        int nvalid;

        std::vector<storage_idx_t> ids;
        std::vector<float> dis;
        typedef faiss::CMax<float, storage_idx_t> HC;

        explicit MinimaxHeap(int n) : n(n), k(0), nvalid(0), ids(n), dis(n) {}

        void push(storage_idx_t i, float v);

        float max() const;

        int size() const;

        void clear();

        int pop_min(float* vmin_out = nullptr);

        int count_below(float thresh);
    };

    /// to sort pairs of (id, distance) from nearest to fathest or the reverse
    struct NodeDistCloser {
        float d;
        int id;
        NodeDistCloser(float d, int id) : d(d), id(id) {}
        bool operator<(const NodeDistCloser& obj1) const {
            return d < obj1.d;
        }
    };

    struct NodeDistFarther {
        float d;
        int id;
        NodeDistFarther(float d, int id) : d(d), id(id) {}
        bool operator<(const NodeDistFarther& obj1) const {
            return d > obj1.d;
        }
    };

    /// assignment probability to each layer (sum=1)
    std::vector<double> assign_probas;

    /// number of neighbors stored per layer (cumulative), should not
    /// be changed after first add
    std::vector<int> cum_nneighbor_per_level;

    /// level of each vector (base level = 1), size = ntotal
    std::vector<int> levels;

    /// offsets[i] is the offset in the neighbors array where vector i is stored
    /// size ntotal + 1
    std::vector<size_t> offsets;

    /// neighbors[offsets[i]:offsets[i+1]] is the list of neighbors of vector i
    /// for all levels. this is where all storage goes.
    std::vector<storage_idx_t> neighbors;

    /// stats
    std::map<int, std::vector<int>> steps_cnt;

    /// entry point in the search structure (one of the points with maximum
    /// level
    storage_idx_t entry_point = -1;

    faiss::RandomGenerator rng;

    /// maximum level
    int max_level = -1;

    /// expansion factor at construction time
    int efConstruction = 40;

    /// expansion factor at search time
    int efSearch = 16;

    /// during search: do we check whether the next best distance is good
    /// enough?
    bool check_relative_distance = true;

    /// number of entry points in levels > 0.
    int upper_beam = 1;

    /// use bounded queue during exploration
    bool search_bounded_queue = true;

    // methods that initialize the tree sizes

    // /// initialize the assign_probas and cum_nneighbor_per_level to
    // /// have 2*M links on level 0 and M links on levels > 0
    // void set_default_probas(int M, float levelMult);

    // /// set nb of neighbors for this level (before adding anything)
    // void set_nb_neighbors(int level_no, int n);

    // // methods that access the tree sizes

    // /// nb of neighbors for this level
    // int nb_neighbors(int layer_no) const;

    /// cumumlative nb up to (and excluding) this level
    int cum_nb_neighbors(int layer_no) const;

    /// range of entries in the neighbors table of vertex no at layer_no
    void neighbor_range(idx_t no, int layer_no, size_t* begin, size_t* end)
            const;

    /// only mandatory parameter: nb of neighbors
    explicit HNSW(int M = 32);

    /// pick a random level for a new point
    // int random_level();

    /// add n random levels to table (for debugging...)
    // void fill_with_random_links(size_t n);

    // void add_links_starting_from(
    //         GenericDistanceComputer& ptdis,
    //         storage_idx_t pt_id,
    //         storage_idx_t nearest,
    //         float d_nearest,
    //         int level,
    //         omp_lock_t* locks,
    //         VisitedTable& vt);

    /** add point pt_id on all levels <= pt_level and build the link
     * structure for them. */
    // void add_with_locks(
    //         GenericDistanceComputer& ptdis,
    //         int pt_level,
    //         int pt_id,
    //         std::vector<omp_lock_t>& locks,
    //         VisitedTable& vt);

    /// search interface for 1 point, single thread
    // HNSWStats search(
    //         GenericDistanceComputer& qdis,
    //         int k,
    //         idx_t* I,
    //         float* D,
    //         VisitedTable& vt,
    //         const SearchParametersHNSW* params = nullptr) const;
    
    // HNSWStats search_with_cache(
    //         GraphCache& gcache,
    //         GenericDistanceComputer& qdis,
    //         int k,
    //         idx_t* I,
    //         float* D,
    //         VisitedTable& vt,
    //         const SearchParametersHNSW* params = nullptr) const;
        
    HNSWStats oblivious_search_preload_beam_cache_upper(
            GraphCache& gcache,
            // GenericDistanceComputer& qdis,
            PQDistanceComputer& pqdis,
            int k,
            idx_t* I,
            float* D,
            VisitedTable& vt,
            const SearchParametersHNSW* params = nullptr) const;

    HNSWStats oblivious_search_preload_beam_cache_upper_ablation(
            GraphCache& gcache,
            // GenericDistanceComputer& qdis,
            PQDistanceComputer& pqdis,
            int k,
            idx_t* I,
            float* D,
            VisitedTable& vt,
            const SearchParametersHNSW* params = nullptr) const;

    // /// search only in level 0 from a given vertex
    // void search_level_0(
    //         GenericDistanceComputer& qdis,
    //         int k,
    //         idx_t* idxi,
    //         float* simi,
    //         idx_t nprobe,
    //         const storage_idx_t* nearest_i,
    //         const float* nearest_d,
    //         int search_type,
    //         HNSWStats& search_stats,
    //         VisitedTable& vt) const;

    // void reset();

    // void clear_neighbor_tables(int level);
    // void print_neighbor_stats(int level) const;

    // int prepare_level_tab(size_t n, bool preset_levels = false);

    // static void shrink_neighbor_list(
    //         GenericDistanceComputer& qdis,
    //         std::priority_queue<NodeDistFarther>& input,
    //         std::vector<NodeDistFarther>& output,
    //         int max_size);

    // void permute_entries(const idx_t* map);
};

struct HNSWStats {
    size_t n1, n2, n3;
    size_t ndis;
    size_t nreorder;

    double oram_fetch;

    std::vector<size_t> nsteps;
    std::vector<size_t> nvisited;

    std::vector<size_t> principle_dim;

    HNSWStats(
            size_t n1 = 0,
            size_t n2 = 0,
            size_t n3 = 0,
            size_t ndis = 0,
            size_t nreorder = 0)
            : n1(n1), n2(n2), n3(n3), ndis(ndis), nreorder(nreorder), nsteps({}), nvisited({}) {
                oram_fetch = 0;
            }

    void reset() {
        n1 = n2 = n3 = 0;
        ndis = 0;
        nreorder = 0;
    }

    void combine(const HNSWStats& other) {
        n1 += other.n1;
        n2 += other.n2;
        n3 += other.n3;
        ndis += other.ndis;
        nreorder += other.nreorder;
    }
};

struct SearchStats {
    std::map<int, std::vector<int>> nsteps_map;
    std::map<int, std::vector<int>> nvisited_map;
    std::vector<size_t> principle_dim;
    std::vector<int> stash_size_before_evict;
    std::vector<int> stash_size_after_evict;

    std::vector<float> perceived_latency;
    std::vector<float> full_latency;

    SearchStats() {}
};

// global var that collects them all
// FAISS_API extern HNSWStats hnsw_stats;

} // namespace faiss
