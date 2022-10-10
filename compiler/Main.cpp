// This file is part of SymCC.
//
// SymCC is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// SymCC is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// SymCC. If not, see <https://www.gnu.org/licenses/>.

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

#include "Pass.h"

void addSymbolizePassLegacy(const llvm::PassManagerBuilder& /* unused */,
                            llvm::legacy::PassManagerBase& PM) {
    PM.add(new SymbolizePassLegacy());
}

// Make the pass known to opt.
static llvm::RegisterPass<SymbolizePassLegacy> X("symbolize",
                                                 "Symbolization Pass");
// Tell frontends to run the pass automatically.
static struct llvm::RegisterStandardPasses
    Y(llvm::PassManagerBuilder::EP_VectorizerStart, addSymbolizePassLegacy);
static struct llvm::RegisterStandardPasses
    Z(llvm::PassManagerBuilder::EP_EnabledOnOptLevel0, addSymbolizePassLegacy);
