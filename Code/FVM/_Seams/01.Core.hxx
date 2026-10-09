// Copyright AStarship <https://astarship.net>.
#if SEAM >= FVM_COUT
#include "../Uniprinter.hpp"
#if SEAM == FVM_COUT
#include "../_Debug.h"
#else
#include "../_Release.h"
#endif
#endif
using namespace ::_;
namespace FVMTest {

inline const CHA* COut(const CHA* args) {
#if SEAM >= FVM_COUT && USING_CONSOLE == YES_0
  A_TEST_BEGIN;
  
  D_COUT(Headingf("Testing your mom and dez nutz..."));
  
#endif

  return 0;
}

}  //< namespace FVMTest
