#ifndef SYMCC_EXPR_H_
#define SYMCC_EXPR_H_

#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <unordered_set>
#include <vector>

#include <z3++.h>

#include "common.h"
#include "range.h"

// XXX: need to change into non-global variable?
namespace symcc {

class Expr;
class ConstantExpr;
class NonConstantExpr;
class BoolExpr;
class ExprBuilder;

using ExprRef = std::shared_ptr<Expr>;
using ConstantExprRef = std::shared_ptr<ConstantExpr>;
using NonConstantExprRef = std::shared_ptr<NonConstantExpr>;
using BoolExprRef = std::shared_ptr<BoolExpr>;
using WeakExprRef = std::weak_ptr<Expr>;
using WeakExprRefVector = std::vector<WeakExprRef>;
using DependencySet = std::set<size_t>;
extern z3::context* g_z3_context;
extern std::vector<Expr*> cachedReadExpressions;

enum Kind {
    Bool,     // 0
    Constant, // 1
    Read,     // 2
    Concat,   // 3
    Extract,  // 4

    ZExt, // 5
    SExt, // 6

    // Arithmetic
    Add,  // 7
    Sub,  // 8
    Mul,  // 9
    UDiv, // 10
    SDiv, // 11
    URem, // 12
    SRem, // 13
    Neg,  // 14

    // Bit
    Not,  // 15
    And,  // 16
    Or,   // 17
    Xor,  // 18
    Shl,  // 19
    LShr, // 20
    AShr, // 21

    // Compare
    Equal,    // 22
    Distinct, // 23
    Ult,      // 24
    Ule,      // 25
    Ugt,      // 26
    Uge,      // 27
    Slt,      // 28
    Sle,      // 29
    Sgt,      // 30
    Sge,      // 31

    // Logical
    LOr,  // 32
    LAnd, // 33
    LNot, // 34

    // Special
    Ite, // 35

    // Virtual operation
    Rol,
    Ror,
    Invalid
};

Kind swapKind(Kind kind);
Kind negateKind(Kind kind);
bool isNegatableKind(Kind kind);

template <class T>
inline std::shared_ptr<T> castAs(ExprRef e) {
    if (T::classOf(*e))
        return std::static_pointer_cast<T>(e);
    else
        return nullptr;
}

template <class T>
inline std::shared_ptr<T> castAsNonNull(ExprRef e) {
    assert(T::classOf(*e));
    return std::static_pointer_cast<T>(e);
}

template <class T>
class DependencyTree {
  public:
    using value_type = T;
    using node_type = std::shared_ptr<value_type>;
    using nodes_vector_type = std::vector<node_type>;
    using dependency_set_type = DependencySet;

    void addNode(node_type node) {
        dependency_set_type* deps = &node->getDeps();
        nodes_.push_back(node);
        deps_.insert(deps->begin(), deps->end());
    }

    void merge(const DependencyTree<T>& other) {
        const dependency_set_type& other_deps = other.getDependencies();
        const nodes_vector_type& other_nodes = other.getNodes();

        nodes_.insert(nodes_.end(), other_nodes.begin(), other_nodes.end());
        deps_.insert(other_deps.begin(), other_deps.end());
    }

    const dependency_set_type& getDependencies() const { return deps_; }

    const nodes_vector_type& getNodes() const { return nodes_; }

    void dump(std::ostream& os = std::cerr) {
        os << "\tnodes = [\n";
        for (const auto& sptr_DN : nodes_) {
            os << "\t\t" << sptr_DN->toString() << '\n';
        }
        os << "\t],\n";
        os << "\tdeps = [ ";
        for (const auto dep : deps_) {
            os << dep << ' ';
        }
        os << "]\n";
    }

  private:
    nodes_vector_type nodes_;
    dependency_set_type deps_;
};

template <class T>
class DependencyForest {
  public:
    using value_type = DependencyTree<T>;
    using reference = std::shared_ptr<value_type>;
    using size_type = size_t;
    using trees_vector_type = std::vector<reference>;
    using dependency_set_type = DependencySet;
    using iterator = typename trees_vector_type::iterator;
    using reverse_iterator = typename trees_vector_type::reverse_iterator;
    using const_iterator = typename trees_vector_type::const_iterator;
    using const_reverse_iterator =
        typename trees_vector_type::const_reverse_iterator;

    constexpr explicit DependencyForest(size_type size) : forest_(size){};
    constexpr DependencyForest() noexcept = default;
    constexpr DependencyForest(const DependencyForest&) noexcept = delete;
    constexpr DependencyForest(DependencyForest&&) noexcept = delete;
    constexpr DependencyForest& operator=(const DependencyForest&) = delete;
    constexpr DependencyForest& operator=(DependencyForest&&) = delete;
    ~DependencyForest() = default;

    constexpr iterator begin() noexcept { forest_.begin(); };
    constexpr iterator end() noexcept { forest_.end(); };
    constexpr reverse_iterator rbegin() noexcept { forest_.rbegin(); };
    constexpr reverse_iterator rend() noexcept { forest_.rend(); };
    constexpr const_iterator cbegin() const noexcept { forest_.cbegin(); };
    constexpr const_iterator cend() const noexcept { forest_.cend(); };
    constexpr const_reverse_iterator crbegin() const noexcept {
        forest_.crbegin();
    };
    constexpr const_reverse_iterator crend() const noexcept {
        forest_.crend();
    };

    reference find(size_type index) {
        if (forest_[index] == nullptr) {
            forest_[index] = std::make_shared<value_type>();
        }

        assert(forest_[index] != nullptr);
        return forest_[index];
    }

    void addNode(std::shared_ptr<T> node) {
        dependency_set_type* deps = &node->getDeps();
        reference tree = nullptr;
        for (const size_type& index : *deps) {
            reference other_tree = find(index);
            if (tree == nullptr)
                tree = other_tree;
            else if (tree != other_tree) {
                tree->merge(*other_tree);
                // Update existing reference
                for (const size_type& index : other_tree->getDependencies())
                    forest_[index] = tree;
            }
            forest_[index] = tree;
        }
        tree->addNode(node);
    }

    void dump(std::ostream& os = std::cerr) {
        // Naming is absolutely terrible with these smart pointers....
        // never know whether it is raw ptr, some ref, or sptr
        size_type idx = 0;
        for (const auto& sptr_DT : forest_) {
            os << "DT[index=" << idx << "] :: {\n";
            if (sptr_DT)
                sptr_DT->dump(os);
            ++idx;
            os << "}\n";
        }
    }

    constexpr size_type size() const noexcept { return forest_.size(); }
    constexpr bool empty() const noexcept { return forest_.empty(); }
    constexpr void resize(size_type sz) { forest_.resize(sz); }
    constexpr void reserve(size_type sz) { forest_.reserve(sz); }
    constexpr void clear() noexcept { forest_.clear(); }

  private:
    trees_vector_type forest_;
};

class Expr {
  public:
    Expr(Kind kind, uint32_t bits);
    virtual ~Expr();
    Expr(const Expr& that) = delete;

    XXH32_hash_t hash();

    Kind kind() const { return kind_; }

    uint32_t bits() const { return bits_; }

    uint32_t bytes() const {
        // utility function to convert from bits to bytes
        assert(bits() % CHAR_BIT == 0);
        return bits() / CHAR_BIT;
    }

    inline ExprRef getChild(size_t index) const { return children_[index]; }

    inline size_t num_children() const { return children_.size(); }

    inline ExprRef getFirstChild() const { return getChild(0); }

    inline ExprRef getSecondChild() const { return getChild(1); }

    inline ExprRef getLeft() const { return getFirstChild(); }

    inline ExprRef getRight() const { return getSecondChild(); }

    int32_t depth();

    inline void inval() {
        isInvalidated_ = true;
        // @Info(alekum): We are invalidated, invalidate our users as well.
        for (auto it = uses_.begin(); it != uses_.end(); it++) {
            WeakExprRef& ref = *it;
            if (!ref.expired()) {
                // std::cerr << "Invalidating... " << ref.lock()->toString()
                //           << std::endl;
                if (!ref.lock()->isInvalidated()) {
                    // std::cerr << "\t\tProcessed ^^^^\n";
                    ref.lock()->inval();
                }
            }
        }
    }

    inline bool isInvalidated() const { return isInvalidated_; }

    inline bool isConcrete() const { return isConcrete_; }

    inline bool isConstant() const { return kind_ == Constant; }

    inline bool isBool() const { return kind_ == Bool; }

    bool isZero() const;
    bool isAllOnes() const;
    bool isOne() const;

    DependencySet& getDeps() {
        if (deps_ == nullptr) {
            deps_ = new DependencySet();
            DependencySet& deps = *deps_;
            for (size_t i = 0; i < num_children(); i++) {
                DependencySet& other = getChild(i)->getDeps();
                deps.insert(other.begin(), other.end());
            }
        }
        return *deps_;
    }

    DependencySet computeDependencies();

    uint32_t countLeadingZeros() {
        if (leading_zeros_ == (uint32_t)-1)
            leading_zeros_ = _countLeadingZeros();
        return leading_zeros_;
    }
    virtual uint32_t _countLeadingZeros() const { return 0; }
    virtual void print(std::ostream& os = std::cerr,
                       unsigned int depth = 0) const;
    friend std::ostream& operator<<(std::ostream& os, const Expr& e) {
        os << e.toString();
        return os;
    }
    void printConstraints();
    std::string toString() const;
    void simplify();

    z3::expr& toZ3Expr(bool verbose = false) {
        if (isInvalidated() || expr_ == nullptr) {
            z3::expr z3_expr = toZ3ExprRecursively(verbose);
            delete expr_;
            expr_ = new z3::expr(z3_expr);
            isInvalidated_ = false;
        }
        return *expr_;
    }

    friend bool equalMetadata(const Expr& l, const Expr& r) {
        return (const_cast<Expr&>(l).hash() == const_cast<Expr&>(r).hash() &&
                l.kind_ == r.kind_ && l.num_children() == r.num_children() &&
                l.bits_ == r.bits_ && l.equalAux(r));
    }

    friend bool equalShallowly(const Expr& l, const Expr& r) {
        // Check equality only in 1 depth if not return false
        if (!equalMetadata(l, r))
            return false;

        // If one of childrens is different, then false
        for (size_t i = 0; i < l.num_children(); i++) {
            if (l.children_[i] != r.children_[i])
                return false;
        }
        return true;
    }

    friend bool operator==(const Expr& l, const Expr& r) {
        // 1. if metadata are different -> different
        if (!equalMetadata(l, r))
            return false;

        // 2. if metadata of children are different -> different
        for (size_t i = 0; i < l.num_children(); i++) {
            if (!equalMetadata(*l.children_[i], *r.children_[i]))
                return false;
        }

        // 3. if all childrens are same --> same
        for (size_t i = 0; i < l.num_children(); i++) {
            if (l.children_[i] != r.children_[i] &&
                *l.children_[i] != *r.children_[i])
                return false;
        }
        return true;
    }

    friend bool operator!=(const Expr& l, const Expr& r) { return !(l == r); }

    inline void addChild(ExprRef e) {
        children_.push_back(e);
        if (!e->isConcrete())
            isConcrete_ = false;
    }
    inline void addUse(WeakExprRef e) { uses_.push_back(e); }

    void addConstraint(Kind kind, llvm::APInt rhs, llvm::APInt adjustment);
    RangeSet* getRangeSet(bool is_unsigned) const {
        return range_sets[is_unsigned];
    }
    void setRangeSet(bool is_unsigned, RangeSet* rs) {
        range_sets[is_unsigned] = rs;
    }
    RangeSet* getSignedRangeSet() const { return getRangeSet(false); }
    RangeSet* getUnsignedRangeSet() const { return getRangeSet(true); }

    // @Cleanup(alekum): Revise symbolize and trySymbolize methods
    // Currently we force symbolization and invalidation of parents for
    // symbolized children nodes. Though, we should investigate that we
    // not doing it twice..and maybe with witty way to do this.
    // In worst case, we just invalidate only expression once we already
    // symbolized it. However, we might try to avoid even that penalty

    /// Make Expr symbolic and make every user of us as symbolic
    void symbolize() {
        inval();
        if (isConcrete()) {
            isConcrete_ = false;
            for (auto it = uses_.begin(); it != uses_.end(); it++) {
                WeakExprRef& ref = *it;
                if (!ref.expired())
                    ref.lock()->symbolize();
            }
        }
    }
    /// Try to symbolize our children first and symbolize itself after
    void trySymbolize() {
        for (size_t i = 0; i < num_children(); ++i) {
            auto e = getChild(i);
            e->trySymbolize();
        }
        symbolize();
    }

    /// Make Expr concrete and try to make every user of us concrete
    void concretize() {
        inval(); // @TODO(alekum 26/10/2022) Sure that we should invalidate
                 // here? what if it is concrete already?
        if (!isConcrete()) {
            isConcrete_ = true;
            for (auto it = uses_.begin(); it != uses_.end(); it++) {
                WeakExprRef& ref = *it;
                if (ref.expired())
                    continue;
                ref.lock()->tryConcretize();
            }
        }
    }

    /// Try to make concrete iff every chil is concrete
    void tryConcretize() {
        if (isConcrete())
            return;

        for (size_t i = 0; i < num_children(); i++) {
            ExprRef e = getChild(i);
            // std::cerr << "tryConcretize..." << e->toString() << std::endl;
            if (!e->isConcrete()) {
                // std::cerr << "\t\t\tnope - it is symbolic, no further "
                //              "concretization, "
                //              "probably we lost invalidation track\n";
                return;
            }
        }

        concretize();
    }

    ExprRef evaluate();

  protected:
    Kind kind_;
    uint32_t bits_;
    std::vector<ExprRef> children_;
    z3::context& context_;
    z3::expr* expr_;
    XXH32_hash_t* hash_;
    RangeSet* range_sets[2];

    // concretization
    bool isConcrete_;
    bool isInvalidated_;

    int32_t depth_;
    DependencySet* deps_;
    WeakExprRefVector uses_;
    uint32_t leading_zeros_;
    ExprRef evaluation_;

    void printChildren(std::ostream& os, bool start, unsigned int depth) const;

    virtual bool printAux([[maybe_unused]] std::ostream& os) const {
        return false;
    }

    void addConstraint(RangeSet& rs, Kind kind, llvm::APInt& rhs,
                       llvm::APInt& adjustment);
    void addConstraint(bool is_unsigned, Kind kind, llvm::APInt& rhs,
                       llvm::APInt& adjustment);

    virtual std::string getName() const = 0;
    virtual z3::expr toZ3ExprRecursively([[maybe_unused]] bool verbose) = 0;
    virtual void hashAux([[maybe_unused]] XXH32_state_t* state) { return; }
    virtual bool equalAux([[maybe_unused]] const Expr& other) const {
        return true;
    }
    virtual ExprRef evaluateImpl() = 0;

}; // class Expr

class ConstantExpr : public Expr {
  public:
    ConstantExpr(ADDRINT value, uint32_t bits)
        : Expr(Constant, bits), value_(bits, value) {}

    ConstantExpr(const llvm::APInt& value, uint32_t bits)
        : Expr(Constant, bits), value_(value) {}

    inline llvm::APInt value() const { return value_; }
    inline bool isZero() const { return value_ == 0; }
    inline bool isOne() const { return value_ == 1; }
    inline bool isAllOnes() const { return value_.isAllOnesValue(); }
    static bool classOf(const Expr& e) { return e.kind() == Constant; }
    uint32_t getActiveBits() const { return value_.getActiveBits(); }
    void print(std::ostream& os, unsigned int depth) const override;
    uint32_t _countLeadingZeros() const override {
        return value_.countLeadingZeros();
    }

  protected:
    std::string getName() const override { return "Constant"; }

    bool printAux(std::ostream& os) const override {
        os << "value=0x" << value_.toString(16, false) << ", bits=" << bits_;
        return true;
    }

    z3::expr toZ3ExprRecursively([[maybe_unused]] bool verbose) override {
        if (value_.getNumWords() == 1)
            return context_.bv_val((uint64_t)value_.getZExtValue(), bits_);
        else
            return context_.bv_val(value_.toString(10, false).c_str(), bits_);
    }

    void hashAux(XXH32_state_t* state) override {
        XXH32_update(state, value_.getRawData(),
                     value_.getNumWords() * sizeof(uint64_t));
    }

    bool equalAux(const Expr& other) const override {
        const ConstantExpr& typed_other =
            static_cast<const ConstantExpr&>(other);
        return value_ == typed_other.value();
    }

    ExprRef evaluateImpl() override;
    llvm::APInt value_;
};

class NonConstantExpr : public Expr {
  public:
    using Expr::Expr;
    static bool classOf(const Expr& e) { return !ConstantExpr::classOf(e); }
};

class UnaryExpr : public NonConstantExpr {
  public:
    UnaryExpr(Kind kind, ExprRef e, uint32_t bits)
        : NonConstantExpr(kind, bits) {
        addChild(e);
    }
    UnaryExpr(Kind kind, ExprRef e) : UnaryExpr(kind, e, e->bits()) {}

    ExprRef expr() const { return getFirstChild(); }

  protected:
    ExprRef evaluateImpl() override;
};

class BinaryExpr : public NonConstantExpr {
  public:
    BinaryExpr(Kind kind, ExprRef l, ExprRef r, uint32_t bits)
        : NonConstantExpr(kind, bits) {
        addChild(l);
        addChild(r);
        SYMCC_ASSERT(l->bits() == r->bits());
    }

    BinaryExpr(Kind kind, ExprRef l, ExprRef r)
        : BinaryExpr(kind, l, r, l->bits()) {}

    void print_op(std::ostream& os, unsigned int depth, const char* op) const;

  protected:
    ExprRef evaluateImpl() override;
};

class LinearBinaryExpr : public BinaryExpr {
    using BinaryExpr::BinaryExpr;
};

class NonLinearBinaryExpr : public BinaryExpr {
    using BinaryExpr::BinaryExpr;
};

class CompareExpr : public LinearBinaryExpr {
  public:
    CompareExpr(Kind kind, ExprRef l, ExprRef r)
        : LinearBinaryExpr(kind, l, r, 1) {
        assert(l->bits() == r->bits());
    }
};

class BoolExpr : public NonConstantExpr {
  public:
    BoolExpr(bool value) : NonConstantExpr(Bool, 1), value_(value) {}

    inline bool value() const { return value_; }
    static bool classOf(const Expr& e) { return e.kind() == Bool; }

  protected:
    std::string getName() const override { return "Bool"; }

    bool printAux(std::ostream& os) const override {
        os << "value=" << value_;
        return true;
    }

    z3::expr toZ3ExprRecursively([[maybe_unused]] bool verbose) override {
        return context_.bool_val(value_);
    }

    void hashAux(XXH32_state_t* state) override {
        XXH32_update(state, &value_, sizeof(value_));
    }

    bool equalAux(const Expr& other) const override {
        const BoolExpr& typed_other = static_cast<const BoolExpr&>(other);
        return value_ == typed_other.value();
    }

    ExprRef evaluateImpl() override;
    bool value_;
};

class ReadExpr : public NonConstantExpr {
  public:
    ReadExpr(size_t index) : NonConstantExpr(Read, 8), index_(index) {
        deps_ = new DependencySet();
        deps_->insert(index);
        isConcrete_ = false;
    }

    std::string getName() const override { return "Read"; }

    inline size_t index() const { return index_; }
    static bool classOf(const Expr& e) { return e.kind() == Read; }

  protected:
    bool printAux(std::ostream& os) const override {
        os << "ptr=" << this << ", idx=" << index_;
        return true;
    }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        if (isConcrete()) {
            return evaluate()->toZ3Expr(verbose);
        }
        z3::symbol symbol = context_.int_symbol(index_);
        z3::sort sort = context_.bv_sort(8);
        return context_.constant(symbol, sort);
    }

    void hashAux(XXH32_state_t* state) override {
        XXH32_update(state, &index_, sizeof(index_));
    }

    bool equalAux(const Expr& other) const override {
        const ReadExpr& typed_other = static_cast<const ReadExpr&>(other);
        return index_ == typed_other.index();
    }

    ExprRef evaluateImpl() override;
    size_t index_;
};

class ConcatExpr : public NonConstantExpr {
  public:
    ConcatExpr(ExprRef l, ExprRef r)
        : NonConstantExpr(Concat, l->bits() + r->bits()) {
        addChild(l);
        addChild(r);
    }

    void print(std::ostream& os, unsigned int depth) const override;

    std::string getName() const override { return "Concat"; }

    static bool classOf(const Expr& e) { return e.kind() == Concat; }
    uint32_t _countLeadingZeros() const override {
        uint32_t result = getChild(0)->countLeadingZeros();
        if (result == getChild(0)->bits())
            result += getChild(1)->countLeadingZeros();
        return result;
    }

  protected:
    z3::expr toZ3ExprRecursively(bool verbose) override {
        return z3::concat(children_[0]->toZ3Expr(verbose),
                          children_[1]->toZ3Expr(verbose));
    }

    ExprRef evaluateImpl() override;
};

class ExtractExpr : public UnaryExpr {
  public:
    ExtractExpr(ExprRef e, uint32_t index, uint32_t bits)
        : UnaryExpr(Extract, e, bits), index_(index) {
        assert(bits + index <= e->bits());
    }

    uint32_t index() const { return index_; }
    ExprRef expr() const { return getFirstChild(); }
    static bool classOf(const Expr& e) { return e.kind() == Extract; }

  protected:
    std::string getName() const override { return "Extract"; }

    bool printAux(std::ostream& os) const override {
        os << "index=" << index_ << ", bits=" << bits_;
        return true;
    }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        z3::expr e = children_[0]->toZ3Expr(verbose);
        return e.extract(index_ + bits_ - 1, index_);
    }

    void hashAux(XXH32_state_t* state) override {
        XXH32_update(state, &index_, sizeof(index_));
    }

    bool equalAux(const Expr& other) const override {
        const ExtractExpr& typed_other = static_cast<const ExtractExpr&>(other);
        return index_ == typed_other.index();
    }

    ExprRef evaluateImpl() override;

    uint32_t index_;
};

class ExtExpr : public UnaryExpr {
  public:
    using UnaryExpr::UnaryExpr;

    ExprRef expr() const { return getFirstChild(); }
    static bool classOf(const Expr& e) {
        return e.kind() == ZExt || e.kind() == SExt;
    }
};

class ZExtExpr : public ExtExpr {
  public:
    ZExtExpr(ExprRef e, uint32_t bits) : ExtExpr(ZExt, e, bits) {}

    std::string getName() const override { return "ZExt"; }

    static bool classOf(const Expr& e) { return e.kind() == ZExt; }
    uint32_t _countLeadingZeros() const override {
        return bits_ - getChild(0)->bits();
    }

  protected:
    z3::expr toZ3ExprRecursively(bool verbose) override {
        ExprRef e = getChild(0);
        if (e->isConcrete())
            return evaluate()->toZ3Expr(verbose);
        return z3::zext(e->toZ3Expr(verbose), bits_ - e->bits());
    }

    bool printAux(std::ostream& os) const override {
        os << "bits=" << bits_;
        return true;
    }

    ExprRef evaluateImpl() override;
};

class SExtExpr : public ExtExpr {
  public:
    SExtExpr(ExprRef e, uint32_t bits) : ExtExpr(SExt, e, bits) {}

    std::string getName() const override { return "SExt"; }

    static bool classOf(const Expr& e) { return e.kind() == SExt; }

  protected:
    bool printAux(std::ostream& os) const override {
        os << "bits=" << bits_;
        return true;
    }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        ExprRef e = getChild(0);
        if (e->isConcrete())
            return evaluate()->toZ3Expr(verbose);
        return z3::sext(e->toZ3Expr(verbose), bits_ - e->bits());
    }

    ExprRef evaluateImpl() override;
};

class NotExpr : public UnaryExpr {
  public:
    NotExpr(ExprRef e) : UnaryExpr(Not, e) {}
    static bool classOf(const Expr& e) { return e.kind() == Not; }

  protected:
    std::string getName() const override { return "Not"; }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        ExprRef e = getChild(0);
        return ~e->toZ3Expr(verbose);
    }
};

class AndExpr : public NonLinearBinaryExpr {
  public:
    AndExpr(ExprRef l, ExprRef h) : NonLinearBinaryExpr(And, l, h) {}

    static bool classOf(const Expr& e) { return e.kind() == And; }

  protected:
    std::string getName() const override { return "And"; }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        return children_[0]->toZ3Expr(verbose) &
               children_[1]->toZ3Expr(verbose);
    }
};

class OrExpr : public NonLinearBinaryExpr {
  public:
    OrExpr(ExprRef l, ExprRef h) : NonLinearBinaryExpr(Or, l, h) {}

    static bool classOf(const Expr& e) { return e.kind() == Or; }

  protected:
    std::string getName() const override { return "Or"; }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        return children_[0]->toZ3Expr(verbose) |
               children_[1]->toZ3Expr(verbose);
    }
};

class XorExpr : public NonLinearBinaryExpr {
  public:
    XorExpr(ExprRef l, ExprRef h) : NonLinearBinaryExpr(Xor, l, h) {}

    static bool classOf(const Expr& e) { return e.kind() == Xor; }

  protected:
    std::string getName() const override { return "Xor"; }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        return children_[0]->toZ3Expr(verbose) ^
               children_[1]->toZ3Expr(verbose);
    }
};

class ShlExpr : public NonLinearBinaryExpr {
  public:
    ShlExpr(ExprRef l, ExprRef r) : NonLinearBinaryExpr(Shl, l, r) {}

    static bool classOf(const Expr& e) { return e.kind() == Shl; }

  protected:
    std::string getName() const override { return "Shl"; }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        return z3::shl(children_[0]->toZ3Expr(verbose),
                       children_[1]->toZ3Expr(verbose));
    }
};

class LShrExpr : public NonLinearBinaryExpr {
  public:
    LShrExpr(ExprRef l, ExprRef r) : NonLinearBinaryExpr(LShr, l, r) {}

    static bool classOf(const Expr& e) { return e.kind() == LShr; }

  protected:
    std::string getName() const override { return "LShr"; }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        return z3::lshr(children_[0]->toZ3Expr(verbose),
                        children_[1]->toZ3Expr(verbose));
    }
};

class AShrExpr : public NonLinearBinaryExpr {
  public:
    AShrExpr(ExprRef l, ExprRef h) : NonLinearBinaryExpr(AShr, l, h) {}

    static bool classOf(const Expr& e) { return e.kind() == AShr; }

  protected:
    std::string getName() const override { return "AShr"; }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        return z3::ashr(children_[0]->toZ3Expr(verbose),
                        children_[1]->toZ3Expr(verbose));
    }
};

class AddExpr : public LinearBinaryExpr {
  public:
    AddExpr(ExprRef l, ExprRef h) : LinearBinaryExpr(Add, l, h) {}

    static bool classOf(const Expr& e) { return e.kind() == Add; }
    void print(std::ostream& os, unsigned int depth) const override;

  protected:
    std::string getName() const override { return "Add"; }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        return children_[0]->toZ3Expr(verbose) +
               children_[1]->toZ3Expr(verbose);
    }
};

class SubExpr : public LinearBinaryExpr {
  public:
    SubExpr(ExprRef l, ExprRef h) : LinearBinaryExpr(Sub, l, h) {}

    static bool classOf(const Expr& e) { return e.kind() == Sub; }

  protected:
    std::string getName() const override { return "Sub"; }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        return children_[0]->toZ3Expr(verbose) -
               children_[1]->toZ3Expr(verbose);
    }

    void print(std::ostream& os = std::cerr,
               unsigned int depth = 0) const override;
};

class MulExpr : public NonLinearBinaryExpr {
  public:
    MulExpr(ExprRef l, ExprRef h) : NonLinearBinaryExpr(Mul, l, h) {}

    static bool classOf(const Expr& e) { return e.kind() == Mul; }
    void print(std::ostream& os, unsigned int depth) const override;

  protected:
    std::string getName() const override { return "Mul"; }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        return children_[0]->toZ3Expr(verbose) *
               children_[1]->toZ3Expr(verbose);
    }
};

class UDivExpr : public NonLinearBinaryExpr {
  public:
    UDivExpr(ExprRef l, ExprRef h) : NonLinearBinaryExpr(UDiv, l, h) {}

    static bool classOf(const Expr& e) { return e.kind() == UDiv; }
    void print(std::ostream& os, unsigned int depth) const override;

  protected:
    std::string getName() const override { return "UDiv"; }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        return z3::udiv(children_[0]->toZ3Expr(verbose),
                        children_[1]->toZ3Expr(verbose));
    }
};

class SDivExpr : public NonLinearBinaryExpr {
  public:
    SDivExpr(ExprRef l, ExprRef h) : NonLinearBinaryExpr(SDiv, l, h) {}

    static bool classOf(const Expr& e) { return e.kind() == SDiv; }
    void print(std::ostream& os, unsigned int depth) const override;

  protected:
    std::string getName() const override { return "SDiv"; }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        return children_[0]->toZ3Expr(verbose) /
               children_[1]->toZ3Expr(verbose);
    }
};

class URemExpr : public NonLinearBinaryExpr {
  public:
    URemExpr(ExprRef l, ExprRef h) : NonLinearBinaryExpr(URem, l, h) {}

    static bool classOf(const Expr& e) { return e.kind() == URem; }
    void print(std::ostream& os, unsigned int depth) const override;

  protected:
    std::string getName() const override { return "URem"; }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        return z3::urem(children_[0]->toZ3Expr(verbose),
                        children_[1]->toZ3Expr(verbose));
    }
};

class SRemExpr : public NonLinearBinaryExpr {
  public:
    SRemExpr(ExprRef l, ExprRef h) : NonLinearBinaryExpr(SRem, l, h) {}

    static bool classOf(const Expr& e) { return e.kind() == SRem; }
    void print(std::ostream& os, unsigned int depth) const override;

  protected:
    std::string getName() const override { return "SRem"; }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        return z3::srem(children_[0]->toZ3Expr(verbose),
                        children_[1]->toZ3Expr(verbose));
    }
};

class NegExpr : public UnaryExpr {
  public:
    NegExpr(ExprRef e) : UnaryExpr(Neg, e) {}

    static bool classOf(const Expr& e) { return e.kind() == Neg; }

  protected:
    std::string getName() const override { return "Neg"; }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        ExprRef e = children_[0];
        return -e->toZ3Expr(verbose);
    }
};

class EqualExpr : public CompareExpr {
  public:
    EqualExpr(ExprRef l, ExprRef h) : CompareExpr(Equal, l, h) {}

    static bool classOf(const Expr& e) { return e.kind() == Equal; }

  protected:
    std::string getName() const override { return "Equal"; }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        return children_[0]->toZ3Expr(verbose) ==
               children_[1]->toZ3Expr(verbose);
    }
};

class DistinctExpr : public CompareExpr {
  public:
    DistinctExpr(ExprRef l, ExprRef h) : CompareExpr(Distinct, l, h) {}

    static bool classOf(const Expr& e) { return e.kind() == Distinct; }

  protected:
    std::string getName() const override { return "Distinct"; }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        return children_[0]->toZ3Expr(verbose) !=
               children_[1]->toZ3Expr(verbose);
    }
};

class UltExpr : public CompareExpr {
  public:
    UltExpr(ExprRef l, ExprRef h) : CompareExpr(Ult, l, h) {}

    static bool classOf(const Expr& e) { return e.kind() == Ult; }

  protected:
    std::string getName() const override { return "Ult"; }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        return z3::ult(children_[0]->toZ3Expr(verbose),
                       children_[1]->toZ3Expr(verbose));
    }
};

class UleExpr : public CompareExpr {
  public:
    UleExpr(ExprRef l, ExprRef h) : CompareExpr(Ule, l, h) {}

    static bool classOf(const Expr& e) { return e.kind() == Ule; }

  protected:
    std::string getName() const override { return "Ule"; }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        return z3::ule(children_[0]->toZ3Expr(verbose),
                       children_[1]->toZ3Expr(verbose));
    }
};

class UgtExpr : public CompareExpr {
  public:
    UgtExpr(ExprRef l, ExprRef h) : CompareExpr(Ugt, l, h) {}

    static bool classOf(const Expr& e) { return e.kind() == Ugt; }

  protected:
    std::string getName() const override { return "Ugt"; }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        return z3::ugt(children_[0]->toZ3Expr(verbose),
                       children_[1]->toZ3Expr(verbose));
    }
};

class UgeExpr : public CompareExpr {
  public:
    UgeExpr(ExprRef l, ExprRef h) : CompareExpr(Uge, l, h) {}

    static bool classOf(const Expr& e) { return e.kind() == Uge; }

  protected:
    std::string getName() const override { return "Uge"; }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        return z3::uge(children_[0]->toZ3Expr(verbose),
                       children_[1]->toZ3Expr(verbose));
    }
};

class SltExpr : public CompareExpr {
  public:
    SltExpr(ExprRef l, ExprRef h) : CompareExpr(Slt, l, h) {}

    static bool classOf(const Expr& e) { return e.kind() == Slt; }

  protected:
    std::string getName() const override { return "Slt"; }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        return children_[0]->toZ3Expr(verbose) <
               children_[1]->toZ3Expr(verbose);
    }
};

class SleExpr : public CompareExpr {
  public:
    SleExpr(ExprRef l, ExprRef h) : CompareExpr(Sle, l, h) {}

    static bool classOf(const Expr& e) { return e.kind() == Sle; }

  protected:
    std::string getName() const override { return "Sle"; }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        return children_[0]->toZ3Expr(verbose) <=
               children_[1]->toZ3Expr(verbose);
    }
};

class SgtExpr : public CompareExpr {
  public:
    SgtExpr(ExprRef l, ExprRef h) : CompareExpr(Sgt, l, h) {}

    static bool classOf(const Expr& e) { return e.kind() == Sgt; }

  protected:
    std::string getName() const override { return "Sgt"; }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        return children_[0]->toZ3Expr(verbose) >
               children_[1]->toZ3Expr(verbose);
    }
};

class SgeExpr : public CompareExpr {
  public:
    SgeExpr(ExprRef l, ExprRef h) : CompareExpr(Sge, l, h) {}

    static bool classOf(const Expr& e) { return e.kind() == Sge; }

  protected:
    std::string getName() const override { return "Sge"; }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        return children_[0]->toZ3Expr(verbose) >=
               children_[1]->toZ3Expr(verbose);
    }
};

class LAndExpr : public NonLinearBinaryExpr {
  public:
    LAndExpr(ExprRef l, ExprRef h) : NonLinearBinaryExpr(LAnd, l, h) {}

    static bool classOf(const Expr& e) { return e.kind() == LAnd; }

  protected:
    std::string getName() const override { return "LAnd"; }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        return children_[0]->toZ3Expr(verbose) &&
               children_[1]->toZ3Expr(verbose);
    }
};

class LOrExpr : public NonLinearBinaryExpr {
  public:
    LOrExpr(ExprRef l, ExprRef h) : NonLinearBinaryExpr(LOr, l, h) {}

    static bool classOf(const Expr& e) { return e.kind() == LOr; }

  protected:
    std::string getName() const override { return "LOr"; }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        return children_[0]->toZ3Expr(verbose) ||
               children_[1]->toZ3Expr(verbose);
    }
};

class LNotExpr : public UnaryExpr {
  public:
    LNotExpr(ExprRef e) : UnaryExpr(LNot, e) {}

    static bool classOf(const Expr& e) { return e.kind() == LNot; }

  protected:
    std::string getName() const override { return "LNot"; }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        ExprRef e = children_[0];
        return !e->toZ3Expr(verbose);
    }
};

class IteExpr : public NonConstantExpr {
  public:
    IteExpr(ExprRef expr_cond, ExprRef expr_true, ExprRef expr_false)
        : NonConstantExpr(Ite, expr_true->bits()) {
        assert(expr_true->bits() == expr_false->bits());
        addChild(expr_cond);
        addChild(expr_true);
        addChild(expr_false);
    }

    static bool classOf(const Expr& e) { return e.kind() == Ite; }
    ExprRef expr_cond() const { return getChild(0); }
    ExprRef expr_true() const { return getChild(1); }
    ExprRef expr_false() const { return getChild(2); }

  protected:
    std::string getName() const override { return "Ite"; }

    z3::expr toZ3ExprRecursively(bool verbose) override {
        return z3::ite(children_[0]->toZ3Expr(verbose),
                       children_[1]->toZ3Expr(verbose),
                       children_[2]->toZ3Expr(verbose));
    }

    ExprRef evaluateImpl() override;
};

// utility functions
bool isZeroBit(ExprRef e, uint32_t idx);
bool isOneBit(ExprRef e, uint32_t idx);
bool isRelational(const Expr* e);
bool isConstant(ExprRef e);
bool isConstSym(ExprRef e);
uint32_t getMSB(ExprRef e);

} // namespace symcc
#endif // SYMCC_EXPR_H_
