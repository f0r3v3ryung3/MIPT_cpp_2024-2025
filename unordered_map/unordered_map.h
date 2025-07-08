#include <cmath>
#include <memory>
#include <functional>
#include <utility>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <iostream>

template<typename T, typename Alloc = std::allocator<T>>
class List {

  struct BaseNode {
    BaseNode* prev;
    BaseNode* next;
    BaseNode() : prev(this), next(this) {}
  };

  struct Node : BaseNode {
    using value_type = T;
    T value;

    template<typename... Args>
    Node(Args&&... args) : value(std::forward<Args>(args)...) {}
    Node(T&& val) : value(std::move(val)) {}
    Node(const T& val) : value(val) {}
    Node() = default;
  };

  template <typename... Args>
  Node* create(Args&&... args) {
    Node* new_node = NodeAllocTraits::allocate(alloc, 1);
    try {
      NodeAllocTraits::construct(alloc, new_node, std::forward<Args>(args)...);
    } catch (...) {
      NodeAllocTraits::deallocate(alloc, new_node, 1);
      throw;
    }
    return new_node;
  }

  void insertNextTo(BaseNode* new_node, BaseNode* pos) {
    new_node->prev = pos;
    new_node->next = pos->next;
    pos->next->prev = new_node;
    pos->next = new_node;

    ++size_;
  }

  void deleteNode(BaseNode* node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    --size_;
    NodeAllocTraits::destroy(alloc, static_cast<Node*>(node));
    NodeAllocTraits::deallocate(alloc, static_cast<Node*>(node), 1);
  }

  template<bool isConst>
  class common_iterator {
    BaseNode* ptr;
  public:
    using pointer = std::conditional_t<isConst, const T*, T*>;
    using reference = std::conditional_t<isConst, const T&, T&>;
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = int64_t;
  

    common_iterator(BaseNode* ptr) : ptr(ptr) {}

    template <bool OtherIsConst>
    common_iterator(const common_iterator<OtherIsConst>& other)
      : ptr(other.get_ptr())
    {}

    reference operator*() const {
      return static_cast<Node*>(ptr)->value;
    }

    pointer operator->() const {
      return &static_cast<Node*>(ptr)->value;
    }

    common_iterator& operator++() {
      ptr = ptr->next;
      return *this;
    }

    common_iterator operator++(int) {
      auto tmp = *this;
      ++(*this);
      return tmp;
    }

    common_iterator& operator--() {
      ptr = ptr->prev;
      return *this;
    }

    common_iterator operator--(int) {
      auto tmp = *this;
      --(*this);
      return tmp;
    }

    bool operator==(const common_iterator& iter) const {
      return (ptr == iter.ptr);
    }

    bool operator!=(const common_iterator& iter) const {
      return !(this->operator==(iter));
    }

    BaseNode* get_ptr() const { return ptr; }
  };


  BaseNode endNode;
  size_t size_;
  using AllocTraits = std::allocator_traits<Alloc>;
  using NodeAlloc = typename AllocTraits::template rebind_alloc<Node>;
  using NodeAllocTraits = std::allocator_traits<NodeAlloc>;
  NodeAlloc alloc;

public:
  using iterator = common_iterator<false>;
  using const_iterator = common_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  List(): endNode(), size_(0) {}

  explicit List(size_t count, const Alloc& alloc = Alloc())
    : endNode(), size_(0), alloc(alloc) {
    try {
      for (size_t i = 0; i < count; ++i) {
        Node* new_node = create();
        insertNextTo(new_node, endNode.prev);
      }
    } catch(...) {
      clear();
      throw;
    }
  }

  explicit List(size_t count, const T& value, const Alloc& alloc = Alloc())
    : endNode(), size_(0), alloc(alloc) {
    try {
      for (size_t i = 0; i < count; ++i) {
        Node* new_node = create(value);
        insertNextTo(new_node, endNode.prev);
      }
    } catch(...) {
      clear();
      throw;
    }
  }

  explicit List(size_t count, T&& value, const Alloc& alloc = Alloc())
    : endNode(), size_(0), alloc(alloc) {
    try {
      for (size_t i = 0; i < count; ++i) {
        Node* new_node = create(std::move(value));
        insertNextTo(new_node, endNode.prev);
      }
    } catch(...) {
      clear();
      throw;
    }
  }

  explicit List(const Alloc& alloc) noexcept
    : endNode(), size_(0), alloc(alloc) {}

  NodeAlloc get_allocator() const noexcept { return alloc; }

  List(const List& other) 
    : endNode()
    , size_(0)
    , alloc(NodeAllocTraits::select_on_container_copy_construction(other.get_allocator()))
  {
    try {
      BaseNode* node = other.endNode.next;
      while (node != &other.endNode) {
        this->push_back(static_cast<Node*>(node)->value);
        node = node->next;
      }
    } catch(...) {
      clear();
      throw;
    }
  }

  List(List&& other) noexcept
    : endNode()
    , size_(other.size_)
    , alloc(std::move(other.alloc)) 
  {
    if (other.empty()) return;
    endNode.next = other.endNode.next;
    endNode.prev = other.endNode.prev;
    endNode.next->prev = &endNode;
    endNode.prev->next = &endNode;
    other.endNode.prev = &other.endNode;
    other.endNode.next = &other.endNode;
    other.size_ = 0;
  }

  List& operator=(List&& other) noexcept(
    NodeAllocTraits::propagate_on_container_move_assignment::value
  ) {
    if (this != &other) {
      clear();
      
      if constexpr (NodeAllocTraits::propagate_on_container_move_assignment::value) {
        alloc = std::move(other.alloc);
      }
      size_ = other.size_;
      if (!other.empty()) {
        endNode.next = other.endNode.next;
        endNode.prev = other.endNode.prev;
        endNode.next->prev = &endNode;
        endNode.prev->next = &endNode; 

        other.endNode.prev = &other.endNode;
        other.endNode.next = &other.endNode;
        other.size_ = 0;
      }
    }
    return *this;
  }

  List& operator=(const List& other) {
    if (this != &other) {
      if constexpr (AllocTraits::propagate_on_container_copy_assignment::
                        value) {
        alloc = other.alloc;
      }
      List tmp(NodeAllocTraits::select_on_container_copy_construction(
          other.alloc));
      for (decltype(auto) elem : other) {
        tmp.push_back(elem);
      }
      this->swap(tmp);
    }
    return *this;
  }

  void swap(List& other) noexcept {
    if (this == &other) return;
    std::swap(endNode.next, other.endNode.next);
    std::swap(endNode.prev, other.endNode.prev);

    if (endNode.next != &endNode) {
      endNode.next->prev = &endNode;
      endNode.prev->next = &endNode;
    } else {
      endNode.next = &endNode;
      endNode.prev = &endNode;
    }

    if (other.endNode.next != &other.endNode) {
      other.endNode.next->prev = &other.endNode;
      other.endNode.prev->next = &other.endNode;
    } else {
      other.endNode.next = &other.endNode;
      other.endNode.prev = &other.endNode;
    }

    std::swap(size_, other.size_);
  }

  ~List() { clear(); }

  size_t size() const noexcept { return size_; }

  bool empty() const noexcept { return size_ == 0; }

  void push_back(const T& val) {
    Node* new_node = create(val);
    insertNextTo(new_node, endNode.prev);
  }

  void push_back(T&& val) {
    Node* new_node = create(std::move(val));
    insertNextTo(new_node, endNode.prev);
  }

  void push_front(const T& val) {
    Node* new_node = create(val);
    insertNextTo(new_node, &endNode);
  }

  void push_front(T&& val) {
    Node* new_node = create(std::move(val));
    insertNextTo(new_node, &endNode);
  }

  void pop_back() {
    erase(endNode.prev);
  }

  void pop_front() {
    erase(cbegin());
  }

  iterator begin() { return iterator(endNode.next); }
  iterator end() { return iterator(&endNode); }

  const_iterator begin() const { return const_iterator(endNode.next); }
  const_iterator end() const { return const_iterator(const_cast<BaseNode*>(&endNode)); }

  const_iterator cbegin() const { return begin(); }
  const_iterator cend() const { return begin(); }

  reverse_iterator rbegin() { return reverse_iterator(end()); }
  reverse_iterator rend() { return reverse_iterator(begin()); }

  const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
  const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

  const_reverse_iterator crbegin() const { return const_reverse_iterator(end()); }
  const_reverse_iterator crend() const { return const_reverse_iterator(begin()); }

  iterator insert(const_iterator iter, const T& value) {
    Node* new_node = create(value);
    insertNextTo(new_node, iter.get_ptr()->prev);
    return new_node;
  }

  iterator insert(const_iterator iter, T&& value) {
    Node* new_node = create(std::move(value));
    insertNextTo(new_node, iter.get_ptr()->prev);
    return new_node;
  }

  iterator erase(const_iterator iter) {
    BaseNode* current_node = iter.get_ptr();
    if (current_node == &endNode) {
      return end();
    }
    BaseNode* next_node = current_node->next;
    deleteNode(current_node);
    return next_node;
  }

  void clear() {
    BaseNode* it = endNode.next;
    while (it != &endNode) {
      BaseNode* tmp = it;
      it = it->next;
      NodeAllocTraits::destroy(alloc, static_cast<Node*>(tmp));
      NodeAllocTraits::deallocate(alloc, static_cast<Node*>(tmp), 1);
    }
    endNode.prev = &endNode;
    endNode.next = &endNode;
  }

  template<typename... Args>
  void emplace_back(Args&&... args) {
    Node* new_node = NodeAllocTraits::allocate(alloc, 1);
    try {
      NodeAllocTraits::construct(alloc, new_node, std::forward<Args>(args)...);
    } catch (...) {
      NodeAllocTraits::deallocate(alloc, new_node, 1);
      throw;
    }
    insertNextTo(new_node, endNode.prev);
  }

  size_t size() {
    return size_;
  }
};

template <
    typename Key,
    typename Value,
    typename Hash = std::hash<Key>,
    typename Equal = std::equal_to<Key>,
    typename Alloc = std::allocator<std::pair<const Key, Value>>
> class UnorderedMap {

public:
  using NodeType = std::pair<const Key, Value>;
private:
  struct Node {
    NodeType data_;
    
    explicit Node(const Node&) = default;
    Node& operator=(const Node&) = default;

    explicit Node(Node&& other) noexcept
      : Node(std::move(const_cast<Key&>(other.data_.first)), std::move(other.data_.second)) {}
    Node& operator=(Node&&) noexcept = default;

    template <typename Data>
    Node(Data&& data) : data_(std::forward<Data>(data)) {}

    Node(const NodeType& data) : data_(data) {}

    Node(NodeType&& data)
      : data_(std::move(const_cast<Key&>(data.first)), std::move(data.second)) {}

    template <typename K, typename V>
    Node(K&& key, V&& value)
      : data_(std::piecewise_construct,
           std::forward_as_tuple(std::forward<K>(key)),
           std::forward_as_tuple(std::forward<V>(value)))
    {}

    template<typename T, typename ListAlloc>
    friend class List;
  };
  
  using AllocTraits = std::allocator_traits<Alloc>;
  
  using NodeAlloc = typename AllocTraits::template rebind_alloc<Node>;
  using NodeAllocTraits = std::allocator_traits<NodeAlloc>;
  
  using ListType = List<Node, NodeAlloc>;
  using ListIterator = typename ListType::iterator;

  struct Bucket {
    ListIterator it_;
    size_t size_;
    
    Bucket(const ListIterator& it, size_t size): it_(it), size_(size) {}
  };

  using BucketAlloc = typename AllocTraits::template rebind_alloc<Bucket>;
  using BucketAllocTraits = std::allocator_traits<BucketAlloc>;

  template<bool isConst>
  class common_iterator {

  public:
    using value_type = std::pair<const Key, Value>;
    using pointer = std::conditional_t<isConst, const value_type*, value_type*>;
    using reference = std::conditional_t<isConst, const value_type&, value_type&>;
    using iterator_category = std::forward_iterator_tag;
    using difference_type = size_t;

    using node_iterator = std::conditional_t<isConst, typename ListType::const_iterator, ListIterator>;
  
    node_iterator it;
    common_iterator() = default;
    common_iterator(node_iterator it) : it(it) {}

    template <bool OtherIsConst>
    common_iterator(const common_iterator<OtherIsConst>& other)
      : it(other.it)
    {}

    template <bool OtherIsConst>
    common_iterator(common_iterator<OtherIsConst>&& other)
      : it(std::move(other.it))
    {}

    reference operator*() const { return it->data_; }
    pointer operator->() const { return &(it->data_); }
    common_iterator& operator++() { ++it; return *this; }
    common_iterator operator++(int) { auto tmp = *this; ++it; return tmp; }
    bool operator==(const common_iterator& o) const { return (it == o.it); }
    bool operator!=(const common_iterator& o) const { return !(this->operator==(o)); }
    node_iterator get_base() { return it; }
  };

private:
  List<Node, NodeAlloc> nodes_;
  Bucket* buckets_;
  size_t bucket_count_;
  float max_load_factor_;
  [[no_unique_address]] Equal equal;
  [[no_unique_address]] Hash hash;
  [[no_unique_address]] BucketAlloc bucket_alloc;

  void allocate_buckets(size_t bucket_to_init) {
    bucket_count_ = bucket_to_init;
    buckets_ = BucketAllocTraits::allocate(bucket_alloc, bucket_count_);
    for (size_t i = 0; i < bucket_count_; ++i) {
      BucketAllocTraits::construct(bucket_alloc, &buckets_[i], nodes_.end(), 0);
    }
  }

  void deallocate_buckets() {
    if (buckets_ == nullptr) return;
    for (size_t i = 0; i < bucket_count_; ++i) {
      BucketAllocTraits::destroy(bucket_alloc, &buckets_[i]);
    }
    BucketAllocTraits::deallocate(bucket_alloc, buckets_, bucket_count_);
    buckets_ = nullptr;
    bucket_count_ = 0;
  }

  void rehash(size_t new_bucket_count) {
    if (new_bucket_count <= bucket_count_) {
      return;
    }
    Bucket* old_buckets = buckets_;
    size_t old_bucket_count = bucket_count_;
    allocate_buckets(new_bucket_count);

    for (auto node = nodes_.begin(); node != nodes_.end(); ++node) {
      size_t ind = hash(node->data_.first) % bucket_count_;
      if (buckets_[ind].size_++ == 0) buckets_[ind].it_ = node;
    }

    for (size_t i = 0; i < old_bucket_count; ++i) {
      BucketAllocTraits::destroy(bucket_alloc, &old_buckets[i]);
    }
    BucketAllocTraits::deallocate(bucket_alloc, old_buckets, old_bucket_count);
  }

  std::pair<ListIterator, bool> insert_node(ListIterator other) {
    size_t ind = hash(other->data_.first) % bucket_count_;
    auto& bucket = buckets_[ind];
    auto it_list = bucket.it_;
    for (size_t i = 0; i < bucket.size_; ++i, ++it_list) {
      if (equal(it_list->data_.first, other->data_.first))
        return {it_list, false};
    }

    if (bucket.size_ == 0) bucket.it_ = other;
    ++bucket.size_;
    if (load_factor() > max_load_factor_) {
      rehash(bucket_count_ * 2);
    }
    return {other, true};
  }

  ListIterator remove_node(ListIterator node) {
    const Key& key = node->data_.first;
    size_t ind = hash(key) % bucket_count_;
    if (node == buckets_[ind].it_) {
      buckets_[ind].it_ = (buckets_[ind].size_ > 1 ? std::next(node) : nodes_.end());
    }
    --buckets_[ind].size_;
    return nodes_.erase(node);
  }

public:
  using iterator = common_iterator<false>;
  using const_iterator = common_iterator<true>;

  explicit UnorderedMap( size_t bucket_to_init = 16,
                         const Equal& equal = Equal(),
                         const Hash& hash = Hash(),
                         const Alloc& alloc = Alloc() )
    : nodes_(alloc)
    , buckets_(nullptr)
    , bucket_count_(0)
    , max_load_factor_(1.0f)
    , equal(equal)
    , hash(hash)
    , bucket_alloc(alloc)
  {
    allocate_buckets(bucket_to_init);
  }

  UnorderedMap(const UnorderedMap& other)
    : nodes_( NodeAllocTraits::select_on_container_copy_construction(other.nodes_.get_allocator()) )
    , buckets_(nullptr)
    , bucket_count_(0)
    , max_load_factor_(other.max_load_factor_)
    , equal(other.equal)
    , hash(other.hash)
    , bucket_alloc(NodeAllocTraits::select_on_container_copy_construction(other.bucket_alloc))
  {
    if (other.bucket_count_ > 0) {
      allocate_buckets(other.bucket_count_);
      for (auto& p : other) {
        emplace(p.first, p.second);
      }
    }
  }

  UnorderedMap( UnorderedMap&& other ) noexcept (
    BucketAllocTraits::propagate_on_container_move_assignment::value
  )
    : nodes_(std::move(other.nodes_))
    , buckets_(other.buckets_)
    , bucket_count_(other.bucket_count_)
    , max_load_factor_(other.max_load_factor_)
    , equal(std::move(other.equal))
    , hash(std::move(other.hash))
    , bucket_alloc(std::move(other.bucket_alloc))
  {
    other.buckets_ = nullptr;
    other.bucket_count_ = 0;
    other.max_load_factor_ = 1.0f;
  }

  UnorderedMap& operator=( const UnorderedMap& other ) {
    if (this == &other) return *this;
    clear();
    deallocate_buckets();
    hash = other.hash;
    equal = other.equal;
    nodes_ = other.nodes_;
    max_load_factor_ = other.max_load_factor_;
    if constexpr (BucketAllocTraits::propogate_on_container_copy_assignment::value) {
      bucket_alloc = other.bucket_alloc;
    }
    allocate_buckets();
    for (auto& node : nodes_) {
        size_t ind = hash(node.data.first) % bucket_count_;
        if (buckets_[ind].size_ == 0) buckets_[ind].it_ = node;
        buckets_[ind].size_++;
    }
  }

  UnorderedMap& operator=(UnorderedMap&& other) noexcept(
    BucketAllocTraits::propagate_on_container_move_assignment::value
    && std::allocator_traits<Alloc>::is_always_equal::value)
  {
    if (this == &other) return *this;
    clear();
    deallocate_buckets();
    if constexpr (std::allocator_traits<Alloc>::is_always_equal::value ||
                  BucketAllocTraits::propagate_on_container_move_assignment::value) {
      nodes_ = std::move(other.nodes_);
      buckets_ = other.buckets_;
      bucket_count_ = other.bucket_count_;
      max_load_factor_ = other.max_load_factor_;
      hash = std::move(other.hash);
      equal = std::move(other.equal);
      if constexpr (BucketAllocTraits::propagate_on_container_move_assignment::value) {
        bucket_alloc = std::move(other.bucket_alloc);
      }
      other.buckets_ = nullptr;
      other.bucket_count_ = 0;
      other.max_load_factor_ = 1.0f;
      return *this;
    } else {
      bucket_count_ = other.bucket_count_;
      allocate_buckets(bucket_count_);
      max_load_factor_ = other.max_load_factor_;
      hash = std::move(other.hash);
      equal = std::move(other.equal);
      for (auto it = other.begin(); it != other.end(); ++it) {
        emplace(std::move(const_cast<Key&>(it->first)),
                std::move(it->second));
      }
      other.clear();
      other.deallocate_buckets();
      other.bucket_count_ = 0;
      other.max_load_factor_ = 1.0f;
      return *this;
    }
  }

  ~UnorderedMap() {
    clear();
    deallocate_buckets();
  }

  size_t size() const noexcept { return nodes_.size(); }

  bool empty() const noexcept { return nodes_->size() == 0; }

  float load_factor() const noexcept { return static_cast<float>(nodes_.size()) / bucket_count_; }

  float max_load_factor() const noexcept { return max_load_factor_; }

  void max_load_factor(float mf) noexcept {
    max_load_factor_ = mf;
    if (load_factor() > max_load_factor_) {
      rehash(bucket_count_ * 2);
    }
  }

  void reserve(size_t n) { rehash(std::ceil(n/max_load_factor_)); }

  void clear() noexcept {
    nodes_.clear();
    for (size_t i = 0; i < bucket_count_; ++i) {
      buckets_[i].size_ = 0;
      buckets_[i].it_ = nodes_.end();
    }
  }

  std::pair<iterator, bool> insert(NodeType&& value) {
    return emplace(std::move(*const_cast<Key*>(&value.first)), std::move(value.second));
  }

  std::pair<iterator, bool> insert(const NodeType& value) {
    return emplace(value.first, value.second);
  }

  template <typename P>
  std::pair<iterator, bool> insert(P&& value) {
    return emplace(std::move(value.first), std::move(value.second));
  }

  template <typename P>
  std::pair<iterator, bool> insert(P& value) {
    return emplace(value.first, value.second);
  }

  template<class InputIt>
  void insert(InputIt start, InputIt finish) {
    for (; start != finish; ++start) {
      insert(std::forward<decltype(*start)>(*start));
    }
  }

  iterator begin() noexcept { return iterator(nodes_.begin()); }
  iterator end() noexcept { return iterator(nodes_.end()); }
  const_iterator begin() const noexcept { return const_iterator(nodes_.begin()); }
  const_iterator end() const noexcept { return const_iterator(nodes_.end()); }
  const_iterator cbegin() const noexcept { return begin(); }
  const_iterator cend() const noexcept { return end(); }

  iterator find(const Key& key) {
    size_t ind = hash(key) % bucket_count_;
    for (size_t i = 0; i < buckets_[ind].size_; ++i) {
      if (equal(buckets_[ind].it_->data_.first, key)) return iterator(buckets_[ind].it_);
    }
    return end();
}

  Value& operator[] (const Key& key) {
    auto it = find(key);
    if (it == end()) {
      auto pr = insert({key, Value()});
      return pr.first->second;
    }
    return it->second;
  }

  Value& operator[] (Key&& key) {
    auto it = find(key);
    if (it == end()) {
      auto pr = insert({std::move(key), Value()});
      return pr.first->second;
    }
    return it->second;
  }

  Value& at(const Key& key) {
    auto it = find(key);
    if (it != end()) return it->second;
    throw std::out_of_range("key is not in unordered map");
  }

  const Value& at(const Key& key) const {
    auto it = find(key);
    if (it != end()) return it->second;
    throw std::out_of_range("key is not in unordered map");
  }
  
  template <typename... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {

    using AllocNodeType = typename AllocTraits::template rebind_alloc<NodeType>;
    using AllocNodeTraitsType = std::allocator_traits<AllocNodeType>;
    AllocNodeType alloc(bucket_alloc);

    NodeType* new_val = AllocNodeTraitsType::allocate(alloc, 1);
    try {
      AllocNodeTraitsType::construct(alloc, new_val, std::forward<Args>(args)...);
    } catch (...) {
      AllocNodeTraitsType::deallocate(alloc, new_val, 1);
      throw;
    }
    
    size_t ind = hash((*new_val).first) % bucket_count_;
    auto& bucket = buckets_[ind];
    auto it_list = bucket.it_;
    for (size_t i = 0; i < bucket.size_; ++i, ++it_list) {
      if (equal(it_list->data_.first, (*new_val).first)) {
        AllocNodeTraitsType::destroy(alloc, new_val);
        AllocNodeTraitsType::deallocate(alloc, new_val, 1);
        return {iterator(it_list), false};
      }
    }

    Node new_node(std::move(*new_val));
    AllocNodeTraitsType::destroy(alloc, new_val);
    AllocNodeTraitsType::deallocate(alloc, new_val, 1);
    if (bucket.size_ == 0) {
      nodes_.push_back(std::move(new_node));
      bucket.it_ = --nodes_.end();
    } else {
      nodes_.insert(bucket.it_, std::move(new_node));
    }
    ++bucket.size_;
    if (load_factor() > max_load_factor_) {
      rehash(bucket_count_ * 2);
    }
    return { --nodes_.end(), true };
  }
  
  iterator erase(const_iterator pos) {
    auto node_iter = pos.get_base();
    return iterator(remove_node(node_iter));
  }

  iterator erase(const_iterator first, const_iterator last) {
    while (first != last) first = erase(first);
    return last;
  }

  void swap(UnorderedMap& o) noexcept (
    BucketAllocTraits::propagate_on_container_swap::value) {
    std::swap(nodes_, o.nodes_);
    std::swap(buckets_, o.buckets_);
    std::swap(bucket_count_, o.bucket_count_);
    if constexpr (BucketAllocTraits::propagate_on_container_swap::value) {
      std::swap(bucket_alloc, o.bucket_alloc);
    }
    std::swap(hash, o.hash);
    std::swap(equal, o.equal);
  }
};