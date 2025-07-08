#include <array>
#include <new>
#include <iostream>
#include <memory>
#include <iterator>

template <size_t N>
class StackStorage {
  std::array<std::byte, N> data_{};
  size_t size;
  
public:
  StackStorage() noexcept : size(0) {}
  StackStorage(const StackStorage&) = delete;
  ~StackStorage() = default;

  std::byte* allocate(size_t n, size_t align_n) {
    void* ptr = data_.data() + size;
    size_t space = N - size;
    void* aligned = std::align(align_n, n, ptr, space);
    if (!aligned) {
      throw std::bad_alloc();
    }
    size = static_cast<std::byte*>(aligned) - data_.data() + n;
    return static_cast<std::byte*>(aligned);
  }
};

template <typename T, size_t N>
class StackAllocator {
  static constexpr size_t align_n = alignof(T);
  StackStorage<N>& storage_;
  
  public:
  template<typename U, size_t M>
  friend class StackAllocator;
  using value_type = T;

  // StackAllocator() noexcept : storage_(*(new StackStorage<N>())) {}

  StackAllocator(StackStorage<N>& storage) noexcept : storage_(storage) {}

  template<typename U>
  StackAllocator(const StackAllocator<U, N>& other) noexcept : storage_(other.storage_) {}
  
  template<typename U>
  StackAllocator& operator=(StackAllocator<U, N>& other) noexcept {
    storage_ = other.storage_;
    return *this;
  }

  ~StackAllocator() = default;
  
  T* allocate(size_t n) {
    return reinterpret_cast<T*>(storage_.allocate(n * sizeof(T), align_n));
  }
  
  void deallocate(T*, size_t) {}

  template<typename U>
  struct rebind {
    using other = StackAllocator<U, N>;
  };

  template<typename T1, size_t N1, typename T2, size_t N2>
  friend bool operator==(const StackAllocator<T1, N1>& first, const StackAllocator<T2, N2>& second) noexcept;
};

template<typename T1, size_t N1, typename T2, size_t N2>
inline bool operator==(const StackAllocator<T1, N1>& first, const StackAllocator<T2, N2>& second) noexcept {
  return (N1 == N2) && (&first.storage_ == &second.storage_);
}

template<typename T1, size_t N1, typename T2, size_t N2>
inline bool operator!=(const StackAllocator<T1, N1>& first, const StackAllocator<T2, N2>& second) noexcept {
  return !(first == second);
}

template<typename T, typename Alloc = std::allocator<T>>
class List {

  struct BaseNode {
    BaseNode* prev;
    BaseNode* next;
    BaseNode() : prev(this), next(this) {}
  };

  struct Node : BaseNode {
    using value_type = T;
    alignas(T) std::byte stor[sizeof(T)];

    Node() = default;

    ~Node() {
      value_ptr()->~T();
    }

    T* value_ptr() { return reinterpret_cast<T*>(stor); }
  };

  template <typename... Args>
  Node* create(Args&&... args) {
    Node* new_node = NodeAllocTraits::allocate(alloc, 1);
    try {
      Alloc standart_alloc(alloc);
      NodeAllocTraits::construct(alloc, new_node);
      AllocTraits::construct(standart_alloc, new_node->value_ptr(), std::forward<Args>(args)...);
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
    common_iterator(const common_iterator<OtherIsConst>& other) requires(!OtherIsConst && isConst)
      : ptr(other.get_ptr())
    {}

    reference operator*() const {
      return *static_cast<Node*>(ptr)->value_ptr();
    }

    pointer operator->() const {
      return static_cast<Node*>(ptr)->value_ptr();
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

  explicit List(size_t count, const Alloc& alloc_ = Alloc())
    : endNode(), size_(0), alloc(alloc_) {
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

  explicit List(size_t count, const T& value, const Alloc& alloc_ = Alloc())
    : List(count, alloc_) {
    BaseNode* node = endNode.next;
    while (node != &endNode) {
      node->value = value;
      node = node->next;
    }
  }

  explicit List(size_t count, T&& value, const Alloc& alloc_ = Alloc())
    : List(count, alloc_) {
    BaseNode* node = endNode.next;
    while (node != &endNode) {
      node->value = std::move(value);
      node = node->next;
    }
  }

  explicit List(const Alloc& alloc_) noexcept
    : endNode(), size_(0), alloc(alloc_) {}

  NodeAlloc get_allocator() const noexcept { return alloc; }

  List(const List& other) 
    : List(NodeAllocTraits::select_on_container_copy_construction(other.get_allocator()))
  {
    BaseNode* node = other.endNode.next;
    while (node != &other.endNode) {
      push_back(*static_cast<Node*>(node)->value_ptr());
      node = node->next;
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

  List& operator=(List&& other) noexcept (
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
      List tmp(other);
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
    Node* new_node = create(std::forward<Args>(args)...);
    insertNextTo(new_node, endNode.prev);
  }
};