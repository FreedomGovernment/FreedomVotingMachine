// Copyright AStarship <https://astarship.net>.
#include <_Config.h>
//
#include "../_Package.hxx"
//
#include "01.Core.hxx"
#include "02.Release.hxx"
//
#include "../Test.hpp"
using namespace ::_;

inline const CHA* FVMTests(const CHA* args) {
  return TTestTree<FVMTest::Core, FVMTest::FVMRelease>(args);
}
