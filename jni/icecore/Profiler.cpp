#include "Profiler.h"

namespace icecore {

Mutex Profiler::s_globalLock;
std::vector<Profiler*> Profiler::s_profilers;

}
