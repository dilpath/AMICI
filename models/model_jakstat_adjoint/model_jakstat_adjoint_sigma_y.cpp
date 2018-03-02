
#include <include/symbolic_functions.h>
#include <include/amici_defines.h> //realtype definition
typedef amici::realtype realtype;
#include <cmath> 

void sigma_y_model_jakstat_adjoint(double *sigmay, const realtype t, const realtype *p, const realtype *k) {
  sigmay[0] = p[14];
  sigmay[1] = p[15];
  sigmay[2] = p[16];
}
