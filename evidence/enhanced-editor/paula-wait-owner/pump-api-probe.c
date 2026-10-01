#include "../../src/native/paula_pump.h"
int probe(struct pt_native_paula_pump *p,struct pt_native_paula_transport *t,ULONG mask)
{enum pt_native_pump_result r;if(!pt_native_paula_pump_bind(p,t))return PT_NATIVE_PUMP_INVALID;r=pt_native_paula_pump_step(p,mask);return pt_native_paula_pump_close(p)?r:PT_NATIVE_PUMP_HOLD;}
