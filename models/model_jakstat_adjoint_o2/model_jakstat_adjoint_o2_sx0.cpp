
#include <include/symbolic_functions.h>
#include <include/amici_defines.h> //realtype definition
typedef amici::realtype realtype;
#include <cmath> 

void sx0_model_jakstat_adjoint_o2(realtype *sx0, const realtype t,const realtype *x0, const realtype *p, const realtype *k, const int ip) {
switch (ip) {
  case 4: {
  sx0[0] = 1.0;

  } break;

}
}
