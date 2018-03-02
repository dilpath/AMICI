
#include <include/symbolic_functions.h>
#include <include/amici_defines.h> //realtype definition
typedef amici::realtype realtype;
#include <cmath> 

void J_model_neuron(realtype *J, const realtype t, const realtype *x, const double *p, const double *k, const realtype *h, const realtype *w, const realtype *dwdx) {
  J[0+0*2] = x[0]*(2.0/2.5E1)+5.0;
  J[0+1*2] = -1.0;
  J[1+0*2] = p[0]*p[1];
  J[1+1*2] = -p[0];
}
