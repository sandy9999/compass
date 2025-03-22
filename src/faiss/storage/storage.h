// #ifndef STORAGE_H
// #define STORAGE_H


// #include <cassert>
// #include <cstring>
// #include <vector>
// #include <map>
// #include <iostream>
// // #include <pca.h>
// // #include <armadillo>
// #include <omp.h>
// #include "../MetricType.h"
// #include "../impl/HNSW.h"
// #include "../utils/io.h"
// #include "../utils/Heap.h"
// #include "../MetricType.h"


// using storage_idx_t = int32_t;
// using idx_t = int64_t;

// // std::vector<float> cum_variance_per_level = {0.7, 0.7, 0.7, 0.7, 0.7};

// float fvec_L2sqr(std::vector<float> &x, std::vector<float> &y, size_t d);
// float fvec_L2sqrd(const float* x, const float* y, size_t d);
// float fvec_neg_inner_product(const float* x, const float* y, size_t d);

// #ifdef ENABLE_PCA

// struct pca_compress {
//     arma::Mat<double> proj_eigvec_;
//     arma::Col<double> mean_;
//     std::vector<double> eigen_values;
//     int n_component;
// };

// struct pca_node {
//     int n_component;
//     arma::Mat<float> proj_eigvec_;
//     arma::Col<float> mean_;
//     std::map<storage_idx_t, std::vector<float>> neighbors;

//     pca_node(int n_comp, arma::Mat<float> evec, arma::Col<float> mean) {
//         n_component = n_comp;
//         proj_eigvec_ = evec;
//         mean_ = mean;
//         assert(proj_eigvec_.n_cols == n_component);
//     }

//     void to_principal_space_and_insert(storage_idx_t id, const std::vector<float>& data) {
//         arma::Col<float> column(&data.front(), data.size());
//         column -= mean_;
//         arma::Row<float> row(column.t() * proj_eigvec_);

//         arma::Row<float> row_(row.row(0));
//         const float* memptr = row_.memptr();
//         std::vector<float> result(memptr, memptr + row_.n_elem);

//         neighbors[id] = std::move(result);
//         assert(neighbors[id].size() == n_component);
//         return;
//     }

//     std::vector<float> to_principal_space(const std::vector<float>& data) {
//         arma::Col<float> column(&data.front(), data.size());
//         column -= mean_;
//         arma::Row<float> row(column.t() * proj_eigvec_);

//         arma::Row<float> row_(row.row(0));
//         const float* memptr = row_.memptr();
//         std::vector<float> result(memptr, memptr + row_.n_elem);

//         return std::move(result);
//     }

//     std::vector<float> to_variable_space(const std::vector<float>& data) {
//         const arma::Row<float> row(&data.front(), data.size());
//         arma::Col<float> column(arma::trans(row * proj_eigvec_.t()));
//         column += mean_;

//         const long n_rows = column.n_rows;
//         const float* memptr = column.colptr(0);
//         std::vector<float> result(memptr, memptr + n_rows);

//         return std::move(result);
//     }

//     storage_idx_t get_nearest(const std::vector<float>& query){
//         std::vector<float> p_query = to_principal_space(query);
        
//         storage_idx_t nearest = -1;
//         float distance = std::numeric_limits<float>::max();

//         std::map<storage_idx_t, std::vector<float>>::iterator it;
//         for (it = neighbors.begin(); it != neighbors.end(); ++it) {
//             float current_dis = fvec_L2sqr(p_query, it->second, n_component);
//             if (current_dis < distance){
//                 distance = current_dis;
//                 nearest = it->first;
//             }
//         }
//         return nearest;
//     }

//     float get_dis(const std::vector<float>& query, storage_idx_t i, bool decompressed){
//         std::vector<float> p_query = to_principal_space(query);

//         auto it = neighbors.find(i);
//         if(it != neighbors.end()){
//             if(decompressed){
//                 std::vector<float> decompressed_pq = query;
//                 std::vector<float> decompressed_ptarget = to_variable_space(neighbors[i]);

//                 assert(decompressed_pq.size() == mean_.size());

//                 return fvec_L2sqr(decompressed_pq, decompressed_ptarget, decompressed_pq.size());

//             } else{
//                 return fvec_L2sqr(p_query, neighbors[i], n_component);
//             }
//         } else{
//             assert(0);
//         }

//         return 0;
//     }


// };

// #endif

// // Abstract structure for the storage
// struct LinearStorage {
//     std::map<int, std::vector<float>> data_map;
//     std::map<int, struct pca_compress *> last_level_pca_map;

//     #ifdef ENABLE_PCA
//     std::vector<std::vector<pca_node *>> pca_cache;
//     #endif

//     faiss::MetricType metric_type;
//     int d;
//     int n;

//     void set(int id, const std::vector<float>& vec) {
//         data_map[id] = vec;
//     }

//     void load_data(size_t dim, size_t nb, float* data, std::vector<int>& levels){
//         assert(dim == d);
//         int cnt = 0;
//         for(int i = 0; i < nb; i++){
//             if(levels[i] > 1){
//                 std::vector<float> vec(data + i*dim, data + (i+1)*dim);
//                 data_map[i] = vec;
//                 cnt ++;   
//             }
//         }
//         cout << "upper layer contains: " << cnt << endl;
//         n = nb;
//     }

//     #ifdef ENABLE_PCA

//     void pca_computing(faiss::HNSW& hnsw, int M){

//         // ignore top layer
//         // which usually contains few node
//         for(int layer = 0; layer < hnsw.max_level; layer++){
//             std::cout << "Computing PCA for layer: " << layer << std::endl;
//             std::map<int, pca_compress*> pca_result;
//             #pragma omp parallel for num_threads(32)
//             for(storage_idx_t i = 0; i < n; i++){
//                 if(hnsw.levels[i] > layer){
//                     // This node exist on this layer
//                     // std::cout << i << std::endl;
//                     size_t begin, end;
//                     hnsw.neighbor_range(i, layer, &begin, &end);

//                     // stats::pca* pca = new stats::pca(d);
//                     stats::pca* pca = new stats::pca(d);
//                     struct pca_compress* pcac = new pca_compress();
//                     pca->add_record(get_double(i));
//                     for (size_t i = begin; i < end; i++) {
//                         int32_t v = hnsw.neighbors[i];
//                         if (v < 0)
//                             break;
//                         pca->add_record(get_double(v));
//                     }

//                     int max_dim = M;
//                     if(layer == 0){
//                         max_dim = max_dim*2;
//                     }

//                     pca->solve();
//                     pcac->mean_ = pca->mean_;
//                     pcac->proj_eigvec_ = pca->proj_eigvec_.cols(0, max_dim - 1);
//                     pcac->eigen_values = pca->get_eigenvalues();

//                     delete pca;

//                     #pragma omp critical
//                     {
//                         pca_result[i] = pcac;
//                     }
//                 }
//             }

//             std::cout << "Saving PCA for layer: " << layer << std::endl;
//             std::string evec_fname = "./pca_" + std::to_string(layer) + ".evec";
//             std::string mean_fname = "./pca_" + std::to_string(layer) + ".mean";
//             std::string eval_fname = "./pca_" + std::to_string(layer) + ".eval";
//             FILE* f_evec = fopen(evec_fname.c_str(), "w");
//             FILE* f_mean = fopen(mean_fname.c_str(), "w");
//             FILE* f_eval = fopen(eval_fname.c_str(), "w");

//             for (auto it = pca_result.begin(); it != pca_result.end(); ++it) {
//                 storage_idx_t id = it->first;
//                 pca_compress* pca = it->second;

//                 // format id size vec

//                 // writing mean
//                 int mean_size = pca->mean_.size();
//                 fwrite(&id, sizeof(storage_idx_t), 1, f_mean);
//                 fwrite(&mean_size, sizeof(int), 1, f_mean);
//                 std::vector<float> tmp_mean(pca->mean_.begin(), pca->mean_.end());

//                 // std::cout << tmp_mean[0] << std::endl;
//                 // std::cout << pca->mean_[0] << std::endl;
//                 // assert(0);
//                 fwrite(tmp_mean.data(), sizeof(float), mean_size, f_mean);

//                 int evec_size = pca->proj_eigvec_.size();
//                 fwrite(&id, sizeof(storage_idx_t), 1, f_evec);
//                 fwrite(&evec_size, sizeof(int), 1, f_evec);
//                 std::vector<float> tmp_evec(pca->proj_eigvec_.begin(), pca->proj_eigvec_.end());
//                 fwrite(tmp_evec.data(), sizeof(float), evec_size, f_evec);

//                 int eval_size = pca->mean_.size();
//                 fwrite(&id, sizeof(storage_idx_t), 1, f_eval);
//                 fwrite(&eval_size, sizeof(int), 1, f_eval);
//                 std::vector<float> tmp_eval(pca->eigen_values.begin(), pca->eigen_values.end());
//                 fwrite(tmp_eval.data(), sizeof(float), eval_size, f_eval);

//                 delete pca;
//             }

//             fclose(f_eval);
//             fclose(f_evec);
//             fclose(f_mean);
//         }
//     }

//     void load_pca(faiss::HNSW& hnsw){
//         std::map<int, std::vector<int>> dist_neighbors;
//         std::map<int, std::vector<int>> dist_pca_dims;

//         // pca config
//         std::vector<float> cum_variance_per_level = {0.7, 0.7, 0.7, 0.7, 0.7};
//         pca_cache.resize(n);

//         for(int layer = 0; layer < hnsw.max_level; layer++){
//             dist_neighbors[layer] = {};
//             dist_pca_dims[layer] = {};

//             float min_variance = cum_variance_per_level[layer];

//             std::cout << "Loading PCA for layer: " << layer << std::endl;
//             std::string evec_fname = "./pca_" + std::to_string(layer) + ".evec";
//             std::string mean_fname = "./pca_" + std::to_string(layer) + ".mean";
//             std::string eval_fname = "./pca_" + std::to_string(layer) + ".eval";
//             // FILE* f_evec = fopen(evec_fname.c_str(), "r");
//             // FILE* f_mean = fopen(mean_fname.c_str(), "r");
//             // FILE* f_eval = fopen(eval_fname.c_str(), "r");

//             size_t num_record;
//             size_t mean_size;
//             size_t eval_size;
//             size_t evec_size;

//             std::vector<int> mean_id_list;
//             std::vector<int> eval_id_list;
//             std::vector<int> evec_id_list;

//             float* mean = pca_read(mean_fname.c_str(), &mean_size, &num_record, mean_id_list);

//             std::cout << "Loading mean: " << num_record << " " << mean_size << std::endl;

//             float* eval = pca_read(eval_fname.c_str(), &eval_size, &num_record, eval_id_list);

//             std::cout << "Loading eval: " << num_record << " " << eval_size << std::endl;

//             float* evec = pca_read(evec_fname.c_str(), &evec_size, &num_record, evec_id_list);

//             std::cout << "Loading evec: " << num_record << " " << evec_size << std::endl;

//             // return;
//             #pragma omp parallel for num_threads(32)
//             for(int offset = 0; offset < mean_id_list.size(); offset++){
//                 storage_idx_t i = mean_id_list[offset];
//                 // std::cout  << i  << std::endl;
//                 // return;
//                 if(hnsw.levels[i] > layer){

//                     arma::Mat<float> proj_eigvec_(
//                         evec + offset*evec_size,
//                         mean_size,
//                         evec_size / mean_size,
//                         false
//                     );

//                     arma::Col<float> mean_(
//                         mean + offset*mean_size,
//                         mean_size,
//                         false
//                     );

//                     std::vector<float> eigen_values(
//                         eval + offset*eval_size,
//                         eval + offset*eval_size + eval_size
//                     );

//                     int principle_dim = 0;
//                     double eigen_acc = 0;
//                     while(eigen_acc < min_variance) {
//                         eigen_acc += eigen_values[principle_dim];
//                         principle_dim += 1;
//                     }

//                     pca_node* pnode = new pca_node(principle_dim, proj_eigvec_.cols(0, principle_dim - 1), mean_);

//                     size_t begin, end;
//                     hnsw.neighbor_range(i, layer, &begin, &end);

//                     pnode->to_principal_space_and_insert(i, get(i));

//                     int num_neighbors = 0;

//                     for (size_t i = begin; i < end; i++) {
//                         storage_idx_t v = hnsw.neighbors[i];
//                         if (v < 0)
//                             break;
//                         pnode->to_principal_space_and_insert(v, get(v));
//                         num_neighbors++;
//                     }

//                     #pragma omp critical
//                     {
//                         dist_neighbors[layer].push_back(num_neighbors);
//                         dist_pca_dims[layer].push_back(principle_dim);
//                         pca_cache[i].push_back(pnode);
//                     }

//                 }
//             }

//             delete[] mean;
//             delete[] eval;
//             delete[] evec;

//         }

//         {
//         std::ofstream f_nb("dist_nb.txt");
//         std::ofstream f_p("dist_p_07.txt");

//         for (const auto& pair : dist_neighbors) {
//             f_nb << pair.first << " ";
//             for(auto &s: pair.second){
//                 f_nb << s << " ";
//             }
//             f_nb << std::endl;
//         }

//         for (const auto& pair : dist_pca_dims) {
//             f_p << pair.first << " ";
//             for(auto &v: pair.second){
//                 f_p << v << " ";
//             }
//             f_p << std::endl;
//         }

//         f_nb.close();
//         f_p.close();
//     }
//     }

//     storage_idx_t get_pca_nearest(int layer, storage_idx_t i, std::vector<float>& q)  {
//         return pca_cache[i][layer]->get_nearest(q);
//     }

//     float get_pca_dis(int layer, storage_idx_t center, storage_idx_t nb, std::vector<float>& q, bool decompressed)  {
//         return pca_cache[center][layer]->get_dis(q, nb, decompressed);
//     }

//     #endif

//     void reconstruct(int i, float* data){
//         if (data_map.find(i) != data_map.end()) {
//             memcpy(data, data_map[i].data(), d*sizeof(float));
//             return;
//         } else {
//             std::cerr << "ID not found in the database.\n";
//             return;
//         }
//     }

//     std::vector<float> get(int id) {
//         if (data_map.find(id) != data_map.end()) {
//             return data_map[id];
//         } else {

//             std::cerr << "ID not found in the database.\n";
//             return std::vector<float>();
//         }
//     }

//     std::vector<double> get_double(int i) {
//         std::vector<float> coord = get(i);
//         std::vector<double> d_coord(coord.begin(), coord.end());
//         return d_coord;
//     }

//     LinearStorage(int d) : d(d){}
// };

// #endif