#include "cluster_search.h"
#include "net_io_channel.h"
#include "ArgMapping.h"

#include <chrono>
#include <omp.h>

using namespace std;
using namespace seal;

int party = 0;
int port = 8000;
string address = "127.0.0.1";
string dataset = "";
string f_latency = "";
string f_comm = "";

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

void send_ciphertext(NetIO *io, Ciphertext &ct) {
  stringstream os;
  uint64_t ct_size;
  ct.save(os);
  ct_size = os.tellp();
  string ct_ser = os.str();
  io->send_data(&ct_size, sizeof(uint64_t));
  io->send_data(ct_ser.c_str(), ct_ser.size());
}

void recv_ciphertext(NetIO *io, Ciphertext &ct, SEALContext& context) {
  stringstream is;
  uint64_t ct_size;
  io->recv_data(&ct_size, sizeof(uint64_t));
  char *c_enc_result = new char[ct_size];
  io->recv_data(c_enc_result, ct_size);
  is.write(c_enc_result, ct_size);
  ct.unsafe_load(context, is);
  delete[] c_enc_result;
}

void send_encrypted_vector(NetIO *io, vector<Ciphertext> &ct_vec) {
    assert(ct_vec.size() > 0);
    stringstream os;
    uint64_t ct_size;
    for (size_t ct = 0; ct < ct_vec.size(); ct++) {
        ct_vec[ct].save(os, compr_mode_type::none);
        if (!ct)
        ct_size = os.tellp();
    }
    string ct_ser = os.str();
    io->send_data(&ct_size, sizeof(uint64_t));
    io->send_data(ct_ser.c_str(), ct_ser.size());
    // for(auto& ctx : ct_vec){
    //     send_ciphertext(io, ctx);
    // }
}

void recv_encrypted_vector(NetIO *io, vector<Ciphertext> &ct_vec, SEALContext& context) {
    assert(ct_vec.size() > 0);
    stringstream is;
    uint64_t ct_size;
    io->recv_data(&ct_size, sizeof(uint64_t));
    char *c_enc_result = new char[ct_size * ct_vec.size()];
    io->recv_data(c_enc_result, ct_size * ct_vec.size());
    for (size_t ct = 0; ct < ct_vec.size(); ct++) {
        is.write(c_enc_result + ct_size * ct, ct_size);
        ct_vec[ct].unsafe_load(context, is);
    }
    delete[] c_enc_result;
//     for(auto& ctx : ct_vec){
//         recv_ciphertext(io, ctx, context);
//     }
}

inline double interval(chrono::_V2::system_clock::time_point start){
    auto end = std::chrono::high_resolution_clock::now();
    auto interval = (end - start)/1e+9;
    return interval.count();
}



int main(int argc, char **argv){

    ArgMapping amap;
    amap.arg("r", party, "Role of party: Server = 1; Client = 2");
    amap.arg("p", port, "Port Number");
    amap.arg("ip", address, "IP Address of server");
    amap.arg("d", dataset, "Dataset: [sift, trip, msmarco, laion]");
    amap.arg("f_latency", f_latency, "Save latency");
    amap.arg("f_comm", f_comm, "Save communication");

    amap.parse(argc, argv);
    
    int num_queries = 1;
    size_t slot_count = 8192;
    int k = 10;
    int nc, dim, node_per_cluster;

    if(dataset == "laion"){
        // LAION
        nc = 407;
        dim = 512;
        node_per_cluster = 703;
        
    } else if (dataset =="sift"){
        // sift
        nc = 1452;
        dim = 128;
        node_per_cluster = 1361;
    } else if (dataset == "trip"){
        // sift
        nc = 1530;
        dim = 768;
        node_per_cluster = 3272;
    } else {
        // msmarco
        // int nc = 3801;
        // size_t slot_count = 8192;
        // int dim = 768;
        // int node_per_cluster = 7224;
        // int num_queries = 1;
        // int k = 10;
        assert(0);
    }

    cout << ">>> Setting up..." << endl;
    cout << "-> Role: " << party << endl;
    cout << "-> Address: " << address << endl;
    cout << "-> Port: " << port << endl;
    cout << "<<<" << endl << endl;

    bool isServer = party == 1;
    NetIO* io = new NetIO(isServer ? nullptr : address.c_str(), port);

    // Initialize HE Params

    EncryptionParameters parms(scheme_type::bfv);
    size_t poly_modulus_degree = slot_count;
    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(CoeffModulus::BFVDefault(poly_modulus_degree));
    parms.set_plain_modulus(PlainModulus::Batching(poly_modulus_degree, 37));

    SEALContext context(parms);
    print_parameters(context);
    cout << endl;

    Evaluator evaluator(context);

    // Encoder
    BatchEncoder batch_encoder(context);

    long comm;
    long round;

    if(isServer){

        // Recieve keys (INSECURE)
        cout << "Waiting for keys from client (INSECURE)..." << endl;
        PublicKey pub_key;
        SecretKey sec_key;
        RelinKeys relin_keys;

        // Public key
        {
            cout << " - Pub Key" << endl;
            uint64_t pk_size;
            io->recv_data(&pk_size, sizeof(uint64_t));
            char *key_share = new char[pk_size];
            io->recv_data(key_share, pk_size);
            stringstream is;
            is.write(key_share, pk_size);
            pub_key.load(context, is);
            delete[] key_share;
        }
        

        // Secret key
        // {
        //     cout << " - Sec Key" << endl;
        //     uint64_t sk_size;
        //     io->recv_data(&sk_size, sizeof(uint64_t));
        //     char *key_share_sk = new char[sk_size];
        //     io->recv_data(key_share_sk, sk_size);
        //     stringstream is_sk;
        //     SecretKey sec_key;
        //     is_sk.write(key_share_sk, sk_size);
        //     sec_key.load(context, is_sk);
        //     delete[] key_share_sk;
        // }
		

        // Relin key
        {
            cout << " - Relin Key" << endl;
            uint64_t relin_size;
            io->recv_data(&relin_size, sizeof(uint64_t));
            char *key_share_relin = new char[relin_size];
            io->recv_data(key_share_relin, relin_size);
            stringstream is_relin;
            is_relin.write(key_share_relin, relin_size);
            relin_keys.load(context, is_relin);
            delete[] key_share_relin;
        }

        Encryptor encryptor(context, pub_key);
        // encryptor.set_secret_key(sec_key);
		// Decryptor decryptor(context, sec_key);
		

        // Prepare database
        cout << "Preparing database..." << endl;
        vector<vector<Ciphertext>> db;
        int num_embed_per_ctx = slot_count / dim;
        int num_ctx_per_cluster = (node_per_cluster / num_embed_per_ctx) + 1;

        cout << "- num_embed_per_ctx: " << num_embed_per_ctx << endl;
        cout << "- num_ctx_per_cluster:" << num_ctx_per_cluster << endl;
        db.resize(nc);

        #pragma omp parallel for
        for(int cid = 0; cid < nc; cid++){
            for(int i = 0; i < num_ctx_per_cluster; i++){
                vector<uint64_t> pod(slot_count, 0ULL);
                // TODO
                Plaintext ptx;
                Ciphertext ctx;
                batch_encoder.encode(pod, ptx);
                encryptor.encrypt(ptx, ctx);
                db[cid].push_back(ctx);
            }
        }

        // Server is ready
        cout << "Server is ready..." << endl;
        bool signal = true;
        io->send_data(&signal, sizeof(bool));

        comm = io->counter;
        round = io->num_rounds;

        for(int iq = 0; iq < num_queries; iq++){
            vector<Ciphertext> query_ctxs(nc);
            vector<Ciphertext> results_ctxs(num_ctx_per_cluster);

            // recv ctx from server
            cout << "Waiting for query..." << endl;
            recv_encrypted_vector(io, query_ctxs, context);
            cout << "Received query from client..." << endl;

            // auto t_start = std::chrono::high_resolution_clock::now();

            // compute
            #pragma omp parallel for
            for(int i = 0; i < num_ctx_per_cluster; i++){
                Ciphertext sum;
                vector<Ciphertext> tmp_mult;
                for(int cid = 0; cid < nc; cid++){
                    Ciphertext mult;
                    evaluator.multiply(db[cid][i], query_ctxs[cid], mult);
                    evaluator.relinearize_inplace(mult, relin_keys);
                    tmp_mult.push_back(mult);
                    if(cid == 0){
                        sum = mult;
                    } else{
                        evaluator.add_inplace(sum, mult);
                    }
                }

                // sum = tmp_mult[0];
                // for(int j = 1; j < tmp_mult.size(); j++){
                //     // cout << "Adding ..." << endl;
                //     evaluator.add_inplace(sum, tmp_mult[j]);
                // }

                results_ctxs[i] = sum;
            }

            // cout << "> [TIMING]: server computation: " << interval(t_start) << "sec" << endl;

            cout << "Sending query result back to client..." << endl;
            send_encrypted_vector(io, results_ctxs);
        }

        if(f_comm != ""){
            long final_comm = io->counter - comm;
            io->send_data(&final_comm, sizeof(long));
        }

    } else{


        // Generate Keys
        cout << "Generating keys..." << endl;
        KeyGenerator keygen(context);
        SecretKey secret_key = keygen.secret_key();
        PublicKey public_key;
        keygen.create_public_key(public_key);
        RelinKeys relin_keys;
        keygen.create_relin_keys(relin_keys);
        Encryptor encryptor(context, public_key);
        Decryptor decryptor(context, secret_key);

        // Share the keys (INSECURE)
        cout << "Sharing keys with server (INSECURE)..." << endl;
        {
            stringstream os;
            public_key.save(os);
            uint64_t pk_size = os.tellp();
            string keys_ser = os.str();
            io->send_data(&pk_size, sizeof(uint64_t));
            io->send_data(keys_ser.c_str(), pk_size);
        }

        // {
        //     stringstream os_sk;
        //     secret_key.save(os_sk);
        //     uint64_t sk_size = os_sk.tellp();
        //     string keys_ser_sk = os_sk.str();
        //     io->send_data(&sk_size, sizeof(uint64_t));
        //     io->send_data(keys_ser_sk.c_str(), sk_size);
        // }

        {
            stringstream os_relin;
            relin_keys.save(os_relin);
            uint64_t relin_size = os_relin.tellp();
            string keys_ser_relin = os_relin.str();
            io->send_data(&relin_size, sizeof(uint64_t));
            io->send_data(keys_ser_relin.c_str(), relin_size);

        }

        // Read Queries
        size_t nq;
        float* xq;
        size_t d2;

        // xq = fvecs_read(query_path, &d2, &nq);
        // assert(dim == d2);

        int num_embed_per_ctx = slot_count / dim;
        int num_ctx_per_cluster = (node_per_cluster / num_embed_per_ctx) + 1;

        // return 0;

        // Waiting for server to be ready
        cout << "Waiting for server to be ready..." << endl;
        bool signal;
        io->recv_data(&signal, sizeof(bool));

        comm = io->counter;
        round = io->num_rounds;

        vector<vector<int>> search_results;

        auto t_start = std::chrono::high_resolution_clock::now();

        // Perform Search
        for(int i = 0; i < num_queries; i++){
            cout << "Performing search: " << i << endl;

            // Determine cluster
            // TODO
            int cluster = 1;

            // Generate query ctx
            cout << "Generating query ctx... " << endl;
            vector<Ciphertext> query_ctxs;
            for(int cid = 0; cid < nc; cid++){
                if(cid == cluster){
                    // Encrypt some thing else
                    vector<uint64_t> pod(slot_count, 0ULL);
                    // TODO
                    Plaintext ptx;
                    Ciphertext ctx;
                    batch_encoder.encode(pod, ptx);
                    encryptor.encrypt(ptx, ctx);
                    query_ctxs.push_back(ctx);
                } else{
                    // Encrypt zeros
                    vector<uint64_t> pod(slot_count, 0ULL);
                    Plaintext ptx;
                    Ciphertext ctx;
                    batch_encoder.encode(pod, ptx);
                    encryptor.encrypt(ptx, ctx);
                    query_ctxs.push_back(ctx);
                }
            }


            // Send ctx to server
            assert(query_ctxs.size() == nc);
            cout << "Sending query ctx... " << endl;
            send_encrypted_vector(io, query_ctxs);

           
            // Recv ctx from server
            cout << "Waiting for query result... " << endl;
            vector<Ciphertext> result_ctxs(num_ctx_per_cluster);
            recv_encrypted_vector(io, result_ctxs, context);
            cout << " - Recieved!" << endl;

            vector<uint64_t> results;
            int cnt = 0;

            // Decrypt and compute sum
            cout << "Computing sum... " << endl;
            for(auto ctx : result_ctxs){
                Plaintext ptx;
                vector<uint64_t> pod(slot_count, 0ULL);
                decryptor.decrypt(ctx, ptx);
                batch_encoder.decode(ptx, pod);

                for(int j = 0; j < num_embed_per_ctx; j++){
                    uint64_t result = 0;
                    for(int k = 0; k < d2; k++){
                        // TODO convert it back to float
                        result += pod[j * d2 + k];
                    }
                    results.push_back(result);
                    cnt ++;
                    if(cnt > node_per_cluster){
                        break;
                    }
                }

                if(cnt > node_per_cluster){
                    break;
                }
            }

            assert(results.size() == node_per_cluster);

            // Sort
            cout << "Ranking... " << endl;
            vector<int> arg;

            for(int i = 0; i < node_per_cluster; i++){
                arg.push_back(i);
            }

            std::sort(arg.begin(), arg.end(), [&results](int a, int b) {
                return results[a] < results[b];
            });

            search_results.push_back(vector<int>(arg.begin(), arg.begin() + k));
        }

        if(f_latency != ""){
            // string perceived_latency_path = "perceived_latency_" + dataset + ".bin";
            // string full_latency_path = "full_latency_" + dataset + ".bin";

            float lat = interval(t_start);

            vector<float> latency = {lat};

            fvecs_write(
                f_latency.c_str(),
                latency.data(),
                latency.size(),
                1
            );

        }

        cout << "> [TIMING]: server computation: " << interval(t_start) << "sec" << endl;

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
    }

   

    std::cout << "Communication cost: " << io->counter - comm << std::endl;
    std::cout << "Round: " << io->num_rounds - round << std::endl;

    return 0;
}