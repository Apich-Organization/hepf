#include "PathEnumerator.h"
#include "llvm/IR/CFG.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace hepf;

PathEnumerator::PathEnumerator(Function &F, size_t maxPaths,
                               size_t maxLoopIterations)
    : F(F), maxPaths(maxPaths), maxLoopIterations(maxLoopIterations),
      reachedLimit(false) {

  errs() << "=== Path Enumerator ===\n\n";

  // Sanity check: function must have an entry block
  if (F.empty()) {
    errs() << "Warning: Function " << F.getName() << " is empty\n";
    return;
  }

  // Start path enumeration from entry block
  std::vector<BasicBlock *> currentPath;
  std::unordered_map<BasicBlock *, size_t> visitCount;

  findAllPaths(&F.getEntryBlock(), currentPath, visitCount);

  if (reachedLimit) {
    errs() << "Warning: Path enumeration limit (" << maxPaths
           << ") reached for function " << F.getName() << "\n";
  }
}

const std::vector<Path> &PathEnumerator::getPaths() const { return paths; }

bool PathEnumerator::isExitBlock(BasicBlock *BB) const {
  // Exit blocks have no successors
  return succ_begin(BB) == succ_end(BB);
}

void PathEnumerator::findAllPaths(
    BasicBlock *current, std::vector<BasicBlock *> &currentPath,
    std::unordered_map<BasicBlock *, size_t> &visitCount) {
  // Check if we've reached the path limit
  if (paths.size() >= maxPaths) {
    reachedLimit = true;
    return;
  }

  // Check how many times we've visited this block in the current path
  size_t currentVisits = visitCount[current];

  // If we've exceeded the loop iteration limit for this block, stop exploring
  if (currentVisits > maxLoopIterations) {
    return;
  }

  // Add current block to path and increment visit count
  currentPath.push_back(current);
  visitCount[current]++;

  // If this is an exit block, save the complete path
  if (isExitBlock(current)) {
    paths.push_back(currentPath);
  } else {
    // Explore all successors
    bool hasSuccessors = false;
    for (BasicBlock *succ : successors(current)) {
      hasSuccessors = true;

      // Early exit if limit reached
      if (paths.size() >= maxPaths) {
        reachedLimit = true;
        break;
      }

      findAllPaths(succ, currentPath, visitCount);
    }

    // Handle unreachable blocks (no successors but not a proper exit)
    if (!hasSuccessors && !isExitBlock(current)) {
      // This could be a block ending in unreachable or similar
      // Treat it as a complete path
      paths.push_back(currentPath);
    }
  }

  // Backtrack: remove current block from path and decrement visit count
  currentPath.pop_back();
  visitCount[current]--;

  // Clean up the map if count reaches zero (optional optimization)
  if (visitCount[current] == 0) {
    visitCount.erase(current);
  }
}
