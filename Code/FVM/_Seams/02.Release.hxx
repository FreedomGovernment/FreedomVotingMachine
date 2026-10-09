// Copyright AStarship <https://astarship.net>.
#if SEAM >= FVM_RELEASE
#if SEAM == FVM_RELEASE
#include "../_Debug.h"
#else
#include "../_Release.h"
#endif
#endif
namespace FVMTest {

inline const CHA* Release(const CHA* args) {
#if SEAM == FVM_RELEASE
#endif
  return NILP;
}

}  //< namespace FVMTest
