#include "llvm/Pass.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;
using namespace cl;

cl::opt<std::string> Variablename("loop-partitions", cl::desc("Specify number of partitions"), cl::value_desc("partitions"));

namespace {
    struct LoopSplitting : public FunctionPass {
        static char ID;

        LoopSplitting() : FunctionPass(ID) {}

        bool runOnFunction(Function &F) override {
            auto &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

            ScalarEvolution *SE = &getAnalysis<ScalarEvolution> (); 


            for (Loop* loop : LI) {
                // Skip non-canonical loops.
                if (!loop->isCanonical(*SE)) {
                    continue;
                }
                // if (*SE->isCanonical(L) == false)
                // continue;

                // Get the loop's induction variable.
                auto *IV = loop->getInductionVariable(*SE);


                // Get the loop's trip count.
                // uint64_t tripCount = 0;
                // if (auto *tripCountExpr = loop->getTripCount()) {
                //     tripCount = tripCountExpr->getSExtValue();
                // } else {
                //     tripCount = -1; // infinite loop
                // }
                auto tripCount = SE->getBackedgeTakenCount(loop);

                // Calculate the number of sub-loops required.
                auto numSubLoops = tripCount;
                
                // Create the new loops.
                auto i = numSubLoops;
                for (i = 0; i < numSubLoops; i++) {
                    // Clone the loop body.
                    BasicBlock *newBB = CloneBasicBlock(loop->getLoopPreheader(), loop->getLoopLatch(), loop->getHeader(), F);

                    // Rename the loop's induction variable to reflect the current sub-loop.
                    auto *newIV = dyn_cast<PHINode>(newBB->getFirstNonPHI());
                    newIV->setName("iv." + std::to_string(i));

                    // Adjust the new loop's terminator to branch to the correct sub-loop.
                    auto *newTerminator = dyn_cast<BranchInst>(newBB->getTerminator());
                    BasicBlock *targetBB = i == numSubLoops - 1 ? loop->getExitBlock() : newBB;
                    newTerminator->setSuccessor(0, targetBB);

                    // Replace all uses of the original induction variable with the appropriate sub-loop's induction variable.
                    IV->replaceAllUsesWith(newIV);

                    // Insert the new sub-loop into the function.
                    F.getBasicBlockList().insertAfter(loop->getHeader(), newBB);
                }

                // Remove the original loop body.
                loop->getHeader()->replaceAllUsesWith(loop->getLoopLatch());
                loop->getLoopLatch()->eraseFromParent();
                loop->getHeader()->eraseFromParent();
                loop->getLoopPreheader()->eraseFromParent();
                loop->getExitBlock()->eraseFromParent();
                loop->eraseFromParent();
            }

            return false;
        }

        void getAnalysisUsage(AnalysisUsage &AU) const override {
            AU.addRequired<LoopInfoWrapperPass>();
            AU.setPreservesAll();
        }
    };
}

char LoopSplitting::ID = 0;
static RegisterPass<LoopSplitting> X("index-set-splitting", "Index Set Splitting Pass", false, false);


// namespace {
//     struct LoopSplitting : public FunctionPass {
//         static char ID;

//         LoopSplitting() : FunctionPass(ID) {}

//         bool runOnFunction(Function &F) override {
//             for (auto &BB : F) {
//                 for (auto &I : BB) {
//                     if (auto *loop = dyn_cast<Loop>(&I)) {
//                         // Get the loop's induction variable.
//                         auto *IV = loop->getCanonicalInductionVariable();

//                         // Get the loop's trip count.
//                         uint64_t tripCount = loop->getSmallConstantTripCount();
//                         if (tripCount == 0) {
//                             tripCount = loop->getRuntimeTripCount()->getSExtValue();
//                         }

//                         // Calculate the number of sub-loops required.
//                         int numSubLoops = tripCount;

//                         // Create the new loops.
//                         for (int i = 0; i < numSubLoops; i++) {
//                             // Clone the loop body.
//                             BasicBlock *newBB = CloneBasicBlock(loop->getLoopPreheader(), loop->getLoopLatch(), loop->getHeader(), F);

//                             // Rename the loop's induction variable to reflect the current sub-loop.
//                             auto *newIV = dyn_cast<PHINode>(newBB->getFirstNonPHI());
//                             newIV->setName("iv." + std::to_string(i));

//                             // Adjust the new loop's terminator to branch to the correct sub-loop.
//                             auto *newTerminator = dyn_cast<BranchInst>(newBB->getTerminator());
//                             BasicBlock *targetBB = i == numSubLoops - 1 ? loop->getExitBlock() : newBB;
//                             newTerminator->setSuccessor(0, targetBB);

//                             // Replace all uses of the original induction variable with the appropriate sub-loop's induction variable.
//                             IV->replaceAllUsesWith(newIV);

//                             // Insert the new sub-loop into the function.
//                             F.getBasicBlockList().insertAfter(BB, newBB);

//                             // Remove the original loop body if this is the first sub-loop.
//                             if (i == 0) {
//                                 BasicBlock *loopBody = loop->getHeader();
//                                 for (auto *user : loopBody->users()) {
//                                     if (auto *br = dyn_cast<BranchInst>(user)) {
//                                         if (br->isConditional()) {
//                                             // Replace the loop condition with a constant.
//                                             auto *cond = br->getCondition();
//                                             auto *falseVal = ConstantInt::get(cond->getType(), 0);
//                                             br->setCondition(falseVal);
//                                         } else {
//                                             // Remove the unconditional branch.
//                                             br->eraseFromParent();
//                                         }
//                                     }
//                                 }
//                                 loopBody->eraseFromParent();
//                             }
//                         }

//                         // Remove the original loop header if it is now empty.
//                         BasicBlock *loopHeader = loop->getHeader();
//                         if (loopHeader->empty()) {
//                             loopHeader->eraseFromParent();
//                         }
//                     }
//                 }
//             }

//             return true;
//         }
//     };
// }

// char LoopSplitting::ID = 0;
// static RegisterPass<LoopSplitting> X("loop-splitting", "Index Set Splitting Pass", false, false);
