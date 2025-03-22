// #include "storage.h"

// float fvec_L2sqr(std::vector<float> &x, std::vector<float> &y, size_t d) {
//     size_t i;
//     double res = 0;
//     for (i = 0; i < d; i++) {
//         double tmp = x[i] - y[i];
//         res += tmp * tmp;
//     }
//     return res;
// }

// float fvec_neg_inner_product(const float* x, const float* y, size_t d) {
//     float res = 0.F;
//     for (size_t i = 0; i != d; ++i) {
//         res += x[i] * y[i];
//     }
//     return -res;
// }


// float fvec_L2sqrd(const float* x, const float* y, size_t d) {
//     size_t i;
//     float res = 0;
//     for (i = 0; i < d; i++) {
//         const float tmp = x[i] - y[i];
//         res += tmp * tmp;
//     }
//     return res;
// }