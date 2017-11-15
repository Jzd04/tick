
// License: BSD 3 clause

#include "online_forest_regressor.h"

/*********************************************************************************
 * NodeRegressor methods
 *********************************************************************************/

NodeRegressor::NodeRegressor(TreeRegressor &tree, ulong parent)
    : _tree(tree) {
  _parent = parent;
  _left = 0;
  _right = 0;
  _n_samples = 0;
  _is_leaf = true;
  _weight = 1;
  _weight_tree = 1;

  _predict = 0;
}

NodeRegressor::NodeRegressor(const NodeRegressor &node)
    : _tree(node._tree),
      _left(node._left), _right(node._right), _parent(node._parent),
      _feature(node._feature), _threshold(node._threshold),
      _n_samples(node._n_samples),
      _x_t(node._x_t),
      _y_t(node._y_t),
      _weight(node._weight), _weight_tree(node._weight_tree),
      _is_leaf(node._is_leaf),
      _predict(node._predict) {}

NodeRegressor::NodeRegressor(const NodeRegressor &&node) : _tree(_tree) {
  _left = node._left;
  _right = node._right;
  _parent = node._parent;
  _feature = node._feature;
  _threshold = node._threshold;
  _n_samples = node._n_samples;
  _weight = node._weight;
  _weight_tree = node._weight_tree;
  _is_leaf = node._is_leaf;
  _x_t = node._x_t;
  _y_t = node._y_t;
}

void NodeRegressor::update_downwards(const ArrayDouble &x_t, const double y_t) {
  _n_samples++;
  _weight *= exp(-step() * loss(y_t));
  update_predict(y_t);
}

void NodeRegressor::update_upwards() {
  if (_is_leaf) {
    _weight_tree = _weight;
  } else {
    _weight_tree = (_weight + node(_left).weight_tree() * node(_right).weight_tree()) / 2;
  }
}

void NodeRegressor::update_predict(const double y_t) {
  // When a node is updated, it necessarily contains already a sample
  _predict = ((_n_samples - 1) * _predict + y_t) / _n_samples;
}

double NodeRegressor::loss(const double y_t) {
  double diff = _predict - y_t;
  return diff * diff / 2;
}

inline NodeRegressor &NodeRegressor::node(ulong index) const {
  return _tree.node(index);
}

ulong NodeRegressor::n_features() const {
  return _tree.n_features();
}

inline double NodeRegressor::step() const {
  return _tree.step();
}

inline ulong NodeRegressor::parent() const {
  return _parent;
}

inline ulong NodeRegressor::left() const {
  return _left;
}

inline NodeRegressor &NodeRegressor::set_left(ulong left) {
  _left = left;
  return *this;
}

inline ulong NodeRegressor::right() const {
  return _right;
}

inline NodeRegressor &NodeRegressor::set_right(ulong right) {
  _right = right;
  return *this;
}

inline bool NodeRegressor::is_leaf() const {
  return _is_leaf;
}

inline NodeRegressor &NodeRegressor::set_is_leaf(bool is_leaf) {
  _is_leaf = is_leaf;
  return *this;
}

inline ulong NodeRegressor::feature() const {
  return _feature;
}

inline NodeRegressor &NodeRegressor::set_feature(ulong feature) {
  _feature = feature;
  return *this;
}

inline double NodeRegressor::threshold() const {
  return _threshold;
}

inline NodeRegressor &NodeRegressor::set_threshold(double threshold) {
  _threshold = threshold;
  return *this;
}

inline ulong NodeRegressor::n_samples() const {
  return _n_samples;
}

inline NodeRegressor &NodeRegressor::set_n_samples(ulong n_samples) {
  _n_samples = n_samples;
  return *this;
}

inline double NodeRegressor::weight() const {
  return _weight;
}

inline NodeRegressor &NodeRegressor::set_weight(double weight) {
  _weight = weight;
  return *this;
}

inline double NodeRegressor::weight_tree() const {
  return _weight_tree;
}

inline NodeRegressor &NodeRegressor::set_weight_tree(double weight_tree) {
  _weight_tree = weight_tree;
  return *this;
}

inline const ArrayDouble &NodeRegressor::x_t() const {
  return _x_t;
}

inline NodeRegressor &NodeRegressor::set_x_t(const ArrayDouble &x_t) {
  _x_t = x_t;
  return *this;
}

inline double NodeRegressor::y_t() const {
  return _y_t;
}

inline NodeRegressor &NodeRegressor::set_y_t(const double y_t) {
  _y_t = y_t;
  return *this;
}

inline double NodeRegressor::predict() const {
  return _predict;
}

void NodeRegressor::print() {
  std::cout // << "Node(idx: " << _index << ", parent: " << _parent
      // << ", f: " << _feature
      // << ", th: " << _threshold
      << ", left: " << _left
      << ", right: " << _right
      // << ", d: " << _depth
      // << ", n: " << n_samples()
      // << ", i: " << _is_leaf
      << ", thresh: " << _threshold
      << ", y_hat: " << _predict
      << ", sample: ";
  // << ", has_sample:" << _has_sample;
  if (_is_leaf) {
    std::cout << "[" << std::setprecision(2) << _x_t[0] << ", " << std::setprecision(2) << _x_t[1]
              << "]";
  } else {
    std::cout << "null";
  }
  std::cout << ", weight: " << _weight;
  std::cout << ", weight_tree: " << _weight_tree;
  std::cout << ")\n";
}

/*********************************************************************************
* TreeRegressor methods
*********************************************************************************/

TreeRegressor::TreeRegressor(const TreeRegressor &tree)
    : nodes(tree.nodes), forest(tree.forest) {}

TreeRegressor::TreeRegressor(const TreeRegressor &&tree)
    : nodes(tree.nodes), forest(tree.forest) {}

TreeRegressor::TreeRegressor(OnlineForestRegressor &forest) : forest(forest) {
  // TODO: pre-allocate the vector to make things faster ?
  add_node(0);
}

ulong TreeRegressor::split_leaf(ulong index, const ArrayDouble &x_t, double y_t) {
  // std::cout << "Splitting node " << index << std::endl;
  ulong left = add_node(index);
  ulong right = add_node(index);
  node(index).set_left(left).set_right(right).set_is_leaf(false);

  // TODO: better feature sampling
  ulong feature = forest.sample_feature();

  double x1_tj = x_t[feature];
  double x2_tj = node(index).x_t()[feature];
  double threshold;

  // The leaf that contains the passed sample (x_t, y_t)
  ulong data_leaf;
  ulong other_leaf;

  // std::cout << "x1_tj= " << x1_tj << " x2_tj= " << x2_tj << " threshold= " << threshold << std::endl;
  // TODO: what if x1_tj == x2_tj. Must be taken care of by sample_feature()
  if (x1_tj < x2_tj) {
    threshold = forest.sample_threshold(x1_tj, x2_tj);
    data_leaf = left;
    other_leaf = right;
  } else {
    threshold = forest.sample_threshold(x2_tj, x1_tj);
    data_leaf = right;
    other_leaf = left;
  }
  // TODO: code a move_sample

  node(index).set_feature(feature).set_threshold(threshold);

  // We pass the sample to the new leaves, and initialize the _label_average with the value
  node(data_leaf).set_x_t(x_t).set_y_t(y_t);
  node(other_leaf).set_x_t(node(index).x_t()).set_y_t(node(index).y_t());

  // Update downwards of v'
  node(other_leaf).update_downwards(node(index).x_t(), node(index).y_t());
  // Update upwards of v': it's a leaf
  node(other_leaf).update_upwards();
  // node(other_leaf).set_weight_tree(node(other_leaf).weight());
  // Update downwards of v''
  node(data_leaf).update_downwards(x_t, y_t);
  // Note: the update_up of v'' is done in the go_up method, called in fit()

  return data_leaf;
}

ulong TreeRegressor::go_downwards(const ArrayDouble &x_t, double y_t, bool predict) {
  // Find the leaf that contains the sample
  // Start at the root. Index of the root is always 0
  // If predict == true, this call to find_leaf is for
  // prediction only, so that no leaf update and splits can be done
  ulong index_current_node = 0;
  bool is_leaf = false;
  while (!is_leaf) {
    // Get the current node
    NodeRegressor &current_node = node(index_current_node);
    if (!predict) {
      current_node.update_downwards(x_t, y_t);
    }
    // Is the node a leaf ?
    is_leaf = current_node.is_leaf();
    if (!is_leaf) {
      if (x_t[current_node.feature()] <= current_node.threshold()) {
        index_current_node = current_node.left();
      } else {
        index_current_node = current_node.right();
      }
    }
  }
  return index_current_node;
}

void TreeRegressor::go_upwards(ulong leaf_index) {

  ulong current = leaf_index;

  while (true) {
    // TODO: use a node::update_upward
    NodeRegressor &current_node = node(current);
    current_node.update_upwards();
//    if (current_node.is_leaf()) {
//      current_node.set_weight_tree(current_node.weight());
//    } else {
//      double w = current_node.weight();
//      double w0 = node(current_node.left()).weight_tree();
//      double w1 = node(current_node.right()).weight_tree();
//      current_node.set_weight_tree((w + w0 * w1) / 2);
////      double a = current_node.weight();
////      double b = weight_tree_left + weight_tree_right;
////      double toto;
////      if(a > b) {
////        toto = a + log(1 + exp(b - a)) - log(2);
////      } else {
////        toto = b + log(1 + exp(a - b)) - log(2);
////      }
//    }
    if (current == 0) {
      break;
    }
    // We must update the root node
    current = node(current).parent();
  }
}

inline ulong TreeRegressor::n_nodes() const {
  return _n_nodes;
}

void TreeRegressor::fit(const ArrayDouble &x_t, double y_t) {
  // TODO: Test that the size does not change within successive calls to fit
  if (iteration == 0) {
    nodes[0].set_x_t(x_t).set_y_t(y_t);
    iteration++;
    return;
  }

  ulong leaf = go_downwards(x_t, y_t, false);
  ulong new_leaf = split_leaf(leaf, x_t, y_t);

//  for(ulong j=0; j < n_features(); ++j) {
//    double delta = std::abs(x_t[j] - node(leaf).sample().first[j]);
//    if (delta > 0.) {
//      new_leaf = split_node(leaf, x_t, y_t);
//      break;
//    }
//  }
  go_upwards(new_leaf);
  iteration++;
}

double TreeRegressor::predict(const ArrayDouble &x_t, bool use_aggregation) {
  ulong leaf = go_downwards(x_t, 0., true);
  if (!use_aggregation) {
    return node(leaf).y_t();
  }
  ulong current = leaf;
  // The child of the current node that does not contain the data
  ulong other;
  ulong parent;
  double weight;
  while (true) {
    NodeRegressor &current_node = node(current);
    if (current_node.is_leaf()) {
      weight = current_node.weight() * current_node.predict();
      // weight = std::exp(current_node.weight()) * current_node.labels_average();
    } else {
      weight = 0.5 * current_node.weight() * current_node.predict()
          + 0.5 * node(other).weight_tree() * weight;
//      weight = 0.5 * std::exp(current_node.weight()) * current_node.labels_average()
//          + 0.5 * std::exp(node(other).weight_tree() + weight);
    }
    parent = node(current).parent();
    if (node(parent).left() == current) {
      other = node(parent).right();
    } else {
      other = node(parent).left();
    }
    // Root must be updated as well
    if (current == 0) {
      break;
    }
    current = parent;
  }
  return weight / nodes[0].weight_tree();
  // return weight / std::exp(nodes[0].weight_tree());
}

ulong TreeRegressor::add_node(ulong parent) {
  nodes.emplace_back(*this, parent);
  return _n_nodes++;
}

inline ulong TreeRegressor::n_features() const {
  return forest.n_features();
}

inline double TreeRegressor::step() const {
  return forest.step();
}

inline Criterion TreeRegressor::criterion() const {
  return forest.criterion();
}

/*********************************************************************************
 * OnlineForestRegressor methods
 *********************************************************************************/

OnlineForestRegressor::OnlineForestRegressor(uint32_t n_trees,
                                             double step,
                                             Criterion criterion,
                                             int32_t n_threads,
                                             int seed,
                                             bool verbose)
    : // _n_trees(n_trees),
    _n_threads(n_threads), _criterion(criterion), _step(step), _verbose(verbose), trees() {
  // No iteration so far
  _n_trees = 10;
  _iteration = 0;
  create_trees();
  // Seed the random number generators
  set_seed(seed);
}

OnlineForestRegressor::~OnlineForestRegressor() {}

void OnlineForestRegressor::create_trees() {
  // Just in case...
  trees.clear();
  trees.reserve(_n_trees);
  for (uint32_t i = 0; i < _n_trees; ++i) {
    trees.emplace_back(*this);
  }
}

void OnlineForestRegressor::fit(const SArrayDouble2dPtr features,
                                const SArrayDoublePtr labels) {
  ulong n_samples = features->n_rows();
  ulong n_features = features->n_cols();
  set_n_features(n_features);
  for (ulong i = 0; i < n_samples; ++i) {
    for (TreeRegressor &tree : trees) {
      // Fit the tree online using the new data point
      tree.fit(view_row(*features, i), (*labels)[i]);
    }
    _iteration++;
  }
}

void OnlineForestRegressor::predict(const SArrayDouble2dPtr features,
                                    SArrayDoublePtr predictions,
                                    bool use_aggregation) {
  if (_iteration > 0) {
    ulong n_samples = features->n_rows();
    for (ulong i = 0; i < n_samples; ++i) {
      // The prediction is simply the average of the predictions
      double y_pred = 0;
      for (TreeRegressor &tree : trees) {
        y_pred += tree.predict(view_row(*features, i), use_aggregation);
      }
      (*predictions)[i] = y_pred / _n_trees;
    }
  } else {
    TICK_ERROR("You must call ``fit`` before ``predict``.")
  }
}

inline ulong OnlineForestRegressor::sample_feature() {
  return rand.uniform_int(0L, n_features() - 1);
}

inline double OnlineForestRegressor::sample_threshold(double left, double right) {
  return rand.uniform(left, right);
}

//inline double OnlineForestRegressor::step() const {
//  return _step;
//}
//
//void OnlineForestRegressor::print() {
//  for (Tree<NodeRegressor> &tree: trees) {
//    tree.print();
//  }
//}
//
//inline ulong OnlineForestRegressor::n_samples() const {
//  if (_iteration > 0) {
//    return _iteration;
//  } else {
//    TICK_ERROR("You must call ``fit`` before asking for ``n_samples``.")
//  }
//}

//inline ulong OnlineForestRegressor::n_features() const {
//  if (_iteration > 0) {
//    return _n_features;
//  } else {
//    TICK_ERROR("You must call ``fit`` before asking for ``n_features``.")
//  }
//}

//inline OnlineForestRegressor &OnlineForestRegressor::set_n_features(ulong n_features) {
//  if (_iteration == 0) {
//    _n_features = n_features;
//  } else {
//    TICK_ERROR("OnlineForest::set_n_features can be called only once !")
//  }
//  return *this;
//}

//inline uint32_t OnlineForestRegressor::n_trees() const {
//  return _n_trees;
//}


//inline OnlineForestRegressor &OnlineForestRegressor::set_n_trees(uint32_t n_trees) {
//  _n_trees = n_trees;
//  return *this;
//}

//inline int32_t OnlineForestRegressor::n_threads() const {
//  return _n_threads;
//}

//OnlineForestRegressor &OnlineForestRegressor::set_n_threads(int32_t n_threads) {
//  _n_threads = n_threads;
//  return *this;
//}

//inline Criterion OnlineForestRegressor::criterion() const {
//  return _criterion;
//}

//inline OnlineForestRegressor &OnlineForestRegressor::set_criterion(Criterion criterion) {
//  _criterion = criterion;
//  return *this;
//}
//
//inline int OnlineForestRegressor::seed() const {
//  return _seed;
//}

//inline OnlineForestRegressor &OnlineForestRegressor::set_seed(int seed) {
//  _seed = seed;
//  rand.reseed(seed);
//  return *this;
//}

//inline bool OnlineForestRegressor::verbose() const {
//  return _verbose;
//}
//
//inline OnlineForestRegressor &OnlineForestRegressor::set_verbose(bool verbose) {
//  _verbose = verbose;
//  return *this;
//}
