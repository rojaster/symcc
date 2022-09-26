#ifndef QSYM_DEPENDENCY_H_
#define QSYM_DEPENDENCY_H_

#include <iostream>
#include <memory>
#include <set>
#include <vector>

namespace qsym {

  typedef std::set<size_t> DependencySet;

  // @Cleanup(alekum): get rid of this class, it doesn't make sense ... expr
  // already provides everything we need including deps_...
  // class DependencyNode {
  //   public:
  //     DependencyNode();
  //     virtual ~DependencyNode();
  //     DependencySet* getDependencies();
  //   virtual DependencySet computeDependencies() = 0;
  //   private:
  //     DependencySet* dependencies_;
  // };

  template<class T>
  class DependencyTree {
    public:
      void addNode(std::shared_ptr<T> node) {
        DependencySet* deps = &node->getDeps();
        nodes_.push_back(node);
        deps_.insert(deps->begin(), deps->end());
      }

      void merge(const DependencyTree<T>& other) {
        const DependencySet& other_deps = other.getDependencies();
        const std::vector<std::shared_ptr<T>>& other_nodes = other.getNodes();

        nodes_.insert(nodes_.end(), other_nodes.begin(), other_nodes.end());
        deps_.insert(other_deps.begin(), other_deps.end());
      }

      const DependencySet & getDependencies() const {
        return deps_;
      }

      const std::vector<std::shared_ptr<T>>& getNodes() const {
        return nodes_;
      }

    void printTree(std::ostream &os=std::cerr) {
      os << "\tnodes = [\n";
      for(const auto &sptr_DN : nodes_) {
        os << "\t\t" << sptr_DN->toString() << '\n';
      }
      os << "\t],\n";
      os << "\tdeps = [ ";
      for(const auto dep : deps_) {
        os << dep << ' ';
      }
      os << "]\n";
    }

    private:
      std::vector<std::shared_ptr<T>> nodes_;
      DependencySet deps_;
  };

  template<class T>
  class DependencyForest {
    public:
      std::shared_ptr<DependencyTree<T>> find(size_t index) {
        if (forest_.size() <= index)
          forest_.resize(index + 1);

        if (forest_[index] == NULL)
          forest_[index] = std::make_shared<DependencyTree<T>>();

        assert(forest_[index] != NULL);
        return forest_[index];
      }

      void addNode(std::shared_ptr<T> node) {
        DependencySet* deps = &node->getDeps();
        std::shared_ptr<DependencyTree<T>> tree = NULL;
        for (const size_t& index : *deps) {
          std::shared_ptr<DependencyTree<T>> other_tree = find(index);
          if (tree == NULL)
            tree = other_tree;
          else if (tree != other_tree) {
            tree->merge(*other_tree);
            // Update existing reference
            for (const size_t& index : other_tree->getDependencies())
              forest_[index] = tree;
          }
          forest_[index] = tree;
        }
        tree->addNode(node);
      }

    void printForest(std::ostream &os = std::cerr) {
      // Naming is absolutely terrible with these smart pointers....
      // never know whether it is raw ptr, some ref, or sptr
      size_t idx = 0;
      for(const auto &sptr_DT : forest_){
        os << "DT[index=" << idx << "] :: {\n";
        if(sptr_DT) sptr_DT->printTree(os);
        ++idx;
        os << "}\n";
      }
    }
    private:
      std::vector<std::shared_ptr<DependencyTree<T>>> forest_;
  };

} // namespace qsym

#endif
