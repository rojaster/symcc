#ifndef SYMCC_EXPR_CACHE_H_
#define SYMCC_EXPR_CACHE_H_

#include "expr.h"
#include <queue>

namespace symcc {

const size_t kCacheSize = 1024;

struct WeakExprRefHash {
    XXH32_hash_t operator()(const WeakExprRef e) const {
        assert(!e.expired());
        return std::const_pointer_cast<Expr>(e.lock())->hash();
    }
};

struct WeakExprRefEqual {
    bool operator()(const WeakExprRef l, const WeakExprRef r) const {
        if (l.expired() || r.expired())
            return false;
        return equalShallowly(*l.lock(), *r.lock());
    }
};

using WeakExprRefSet =
    std::unordered_set<WeakExprRef, WeakExprRefHash, WeakExprRefEqual>;
using WeakExprRefQueue = std::queue<WeakExprRef>;

class ExprCache {
  public:
    ExprCache();
    void insert(WeakExprRef e);
    ExprRef find(ExprRef e);

  protected:
    uint32_t limit_;
    WeakExprRefSet set_;
    WeakExprRefQueue queue_;

    void shrink();
    void cleanup();
};

} // namespace symcc

#endif // SYMCC_EXPR_CACHE_H_
