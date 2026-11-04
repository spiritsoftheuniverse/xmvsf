#ifndef XMVSF_STAGE87_137_DHC_ARBITER_H
#define XMVSF_STAGE87_137_DHC_ARBITER_H
// Compatibility entry point for the single 87.137 DHC coordinator.
// The original simplistic INCOMING->LOCKED->IDLE implementation could unlock
// on a premature action exit and did not guarantee precharged startup.
// Use the timing coordinator exclusively. No native mutation here.
#include "xmvsf_stage87_137_dhc_timing.h"
namespace XM87137 {
using XM87137Timing::Command;
using XM87137Timing::Signal;
using XM87137Timing::Result;
using XM87137Timing::Coordinator;
}
#endif
