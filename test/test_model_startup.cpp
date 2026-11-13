#include "model_startup.h"
#include <cassert>
#include <cstdio>
struct Build {
  int failAt, step = 0, nodes = 0, active = 0;
  bool reserve = true, failedFlag = false, allocationFailed = false;
  explicit Build(int failure) : failAt(failure) {}
  bool next() { return ++step != failAt; }
  bool construct() {
    for (unsigned i = 0; i < 17; ++i) { if (!next()) return false; ++nodes; }
    return true;
  }
  bool healthy() { return next() && !allocationFailed; }
  bool activate() {
    assert(nodes == 17 && reserve);
    for (unsigned i = 0; i < 17; ++i) { if (!next()) return false; ++active; }
    return true;
  }
  void release() { assert(reserve); reserve = false; }
  void rollback() { assert(!reserve); nodes = active = 0; }
  void failed() { assert(!reserve && !nodes && !active); failedFlag = true; }
};
int main() {
  for (int failAt = 1; failAt <= 36; ++failAt) {
    Build failure(failAt);
    assert(!bridge::buildMatterModel(failure));
    assert(failure.failedFlag && !failure.nodes && !failure.active && !failure.reserve);
  }
  Build success(0);
  assert(bridge::buildMatterModel(success));
  assert(success.nodes == 17 && success.active == 17 && !success.reserve && !success.failedFlag);
  Build allocationFailure(0); allocationFailure.allocationFailed = true;
  assert(!bridge::buildMatterModel(allocationFailure) && allocationFailure.active == 0);
  puts("Matter model startup passed: 16 lights+bridge, construction/activation/heap failures, release before rollback, no partial ready state.");
}
