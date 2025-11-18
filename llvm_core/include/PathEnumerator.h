#ifndef PATH_ENUMERATOR_H
#define PATH_ENUMERATOR_H

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include <unordered_map>
#include <vector>

namespace hepf {

using Path = std::vector<llvm::BasicBlock *>;

class PathEnumerator {
public:
  // maxPaths: maximum number of paths to enumerate
  // maxLoopIterations: maximum times to traverse each back edge (0 = skip loop
  // bodies)
  explicit PathEnumerator(llvm::Function &F, size_t maxPaths,
                          size_t maxLoopIterations);

  const std::vector<Path> &getPaths() const;
  bool hasReachedLimit() const { return reachedLimit; }
  size_t getPathCount() const { return paths.size(); }

private:
  void findAllPaths(llvm::BasicBlock *current,
                    std::vector<llvm::BasicBlock *> &currentPath,
                    std::unordered_map<llvm::BasicBlock *, size_t> &visitCount);

  bool isExitBlock(llvm::BasicBlock *BB) const;

  llvm::Function &F;
  std::vector<Path> paths;
  size_t maxPaths;
  size_t maxLoopIterations;
  bool reachedLimit;
};

} // namespace hepf

#endif // PATH_ENUMERATOR_H
