#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

float* pca_read(const char* fname, size_t* d_out, size_t* n_out, std::vector<int> &id_list);

float* fvecs_read(const char* fname, size_t* d_out, size_t* n_out);

int* ivecs_read(const char* fname, size_t* d_out, size_t* n_out);