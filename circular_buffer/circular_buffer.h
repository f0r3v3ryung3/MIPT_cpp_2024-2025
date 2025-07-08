#include <iostream>
#include <limits>
#include <type_traits>
#include <array>
#include <iterator>

class DynamicCapacity {
public:
  size_t capacity_ = 0;
  size_t Capacity() const noexcept { return capacity_; }
};
  
template <size_t N>
class StaticCapacity {
public:
  static constexpr size_t Capacity() noexcept { return N; }
};

constexpr std::size_t DYNAMIC_CAPACITY = std::numeric_limits<std::size_t>::max();

namespace Storages {}

template <typename T, std::size_t Capacity = DYNAMIC_CAPACITY>
class CircularBuffer {
  static constexpr bool isDynamic = (Capacity == DYNAMIC_CAPACITY);
  
  struct DynamicStorage {
    T* data;
    std::size_t capacity;

    explicit DynamicStorage(std::size_t cap)
      : data(reinterpret_cast<T*>(new std::byte[cap * sizeof(T)]))
      , capacity(cap)
    {}

    ~DynamicStorage() {
      delete[] reinterpret_cast<std::byte*>(data);
    }

    DynamicStorage(const DynamicStorage& other) : DynamicStorage(other.capacity) {}

    T* get_ptr_storage(size_t i = 0) noexcept {
      return reinterpret_cast<T*>(&data[i]);
    }

    const T* get_ptr_storage(size_t i = 0) const noexcept {
      return reinterpret_cast<const T*>(&data[i]);
    }
  };

  struct StaticStorage {
    alignas(T) std::array<std::byte, Capacity * sizeof(T)> data;

    T* get_ptr_storage(size_t i = 0) noexcept {
      return reinterpret_cast<T*>(&data[i * sizeof(T)]);
    }

    const T* get_ptr_storage(size_t i = 0) const noexcept {
      return reinterpret_cast<const T*>(&data[i * sizeof(T)]);
    }
  };

  template<bool isConst>
  class common_iterator: std::conditional_t<isDynamic, DynamicCapacity, StaticCapacity<Capacity>>  {

    public:
    using pointer = std::conditional_t<isConst, const T*, T*>;
    using reference = std::conditional_t<isConst, const T&, T&>;
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = size_t;
  
    pointer data;
    std::size_t ind;
    std::size_t start;

    common_iterator(pointer data, size_t ind, size_t start, size_t cap = 0)
      : data(data)
      , ind(ind)
      , start(start)
    {
      if constexpr (isDynamic) {
        this->capacity_ = cap;
      }
    }

    template <bool OtherIsConst>
    common_iterator(const common_iterator<OtherIsConst>& other) requires(!OtherIsConst && isConst)
      : data(other.data)
      , ind(other.ind)
      , start(other.start)
    {}

    reference operator*() const {
      return data[(start + ind) % this->Capacity()];
    }

    pointer operator->() const {
      return &data[(start + ind) % this->Capacity()];
    }

    reference operator[](size_t n) const {
      return *(*this + n);
    }

    common_iterator& operator++() {
      ++ind;
      return *this;
    }

    common_iterator operator++(int) {
      auto tmp = *this;
      ++(*this);
      return tmp;
    }

    common_iterator& operator--() {
      --ind;
      return *this;
    }

    common_iterator operator--(int) {
      auto tmp = *this;
      --(*this);
      return tmp;
    }

    common_iterator& operator+=(size_t n) {
      ind += n;
      return *this;
    }

    common_iterator& operator-=(size_t n) {
      return *this += (-n);
    }

    friend common_iterator operator+(common_iterator iter, size_t n) {
      iter += n;
      return iter;
    }

    friend common_iterator operator+(size_t n, common_iterator iter) {
      iter += n;
      return iter;
    }

    friend common_iterator operator-(common_iterator iter, size_t n) {
      iter -= n;
      return iter;
    }

    friend size_t operator-(const common_iterator& first, const common_iterator& second) {
      return first.ind - second.ind;
    }

    bool operator==(const common_iterator& iter) const {
      return (data == iter.data && ind == iter.ind);
    }

    auto operator<=>(const common_iterator& iter) const {
      return ind <=> iter.ind;
    }
  };

  using Storage = std::conditional_t<isDynamic, DynamicStorage, StaticStorage>;
  Storage storage;
  
  std::size_t head_ = 0;
  std::size_t size_ = 0;

  size_t getIndex(size_t i) const noexcept {
    return (head_ + i + capacity()) % capacity();
  }
  
  T* get_ptr(size_t i = 0) noexcept {
    return storage.get_ptr_storage(getIndex(i));
  }

  const T* get_ptr(size_t i = 0) const noexcept {
    return storage.get_ptr_storage(getIndex(i));
  }
  
public:
  using iterator = common_iterator<false>;
  using const_iterator = common_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  explicit CircularBuffer(std::size_t capacity, int = 0) requires(isDynamic)
    : storage(capacity)
  {}

  CircularBuffer() requires(!isDynamic) {};

  explicit CircularBuffer(std::size_t capacity) requires(!isDynamic)
  {
    if (capacity != Capacity) {
      throw std::invalid_argument("capacity != static capacity");
    }
  }

  CircularBuffer(const CircularBuffer& other)
    : CircularBuffer(other.capacity())
  {
    head_ = other.head_;
    std::size_t i = 0;
    for (; i < other.size_; ++i) {
      push_back(*other.get_ptr(i));
    }
  }

  ~CircularBuffer() {
    for (size_t i = 0; i < size_; ++i) {
      get_ptr(i)->~T();
    }
    size_ = 0;
  }

  CircularBuffer& operator=(const CircularBuffer& other) {
    if (this == &other) {
      return *this;
    }
    if constexpr (isDynamic) {
      CircularBuffer temp_other(other);
      swap(temp_other);
    } else {
      for (size_t i = 0; i < size_; ++i) {
        get_ptr(i)->~T();
      }
      size_ = 0;
      
      size_ = other.size_;
      head_ = other.head_;
      for (size_t i = 0; i < size_; ++i) {
        new (get_ptr(i)) T(*other.get_ptr(i));
      }
    }
    return *this;
  }

  void swap(CircularBuffer& other) noexcept(std::is_nothrow_swappable_v<T>) {
    if (this == &other) {
      return;
    }
    if constexpr (isDynamic) {
      std::swap(storage.data, other.storage.data);
      std::swap(storage.capacity, other.storage.capacity);
      std::swap(size_, other.size_);
      std::swap(head_, other.head_);
    } else {
      size_t max_sz = std::max(size_, other.size_);
      swap_ranges(begin(), end() + max_sz + 1, other.begin());
      swap(head_, other.head_);
      swap(size_, other.size_);

    }
  }

  size_t size() const noexcept { return size_; }

  size_t capacity() const noexcept {
    if constexpr (isDynamic) {
      return storage.capacity;
    }
    return Capacity;
  }

  bool empty() const noexcept { return size() == 0; }

  bool full() const noexcept { return size() == capacity(); }

  void push_back(const T& value) {
    if (full()) {
      T* cell = get_ptr(size_);
      *cell = value;
      head_ = getIndex(1);
      return;
    }
    new (get_ptr(size_)) T(value);
    ++size_;
  }

  void push_front(const T& value) {
    if (full()) {
      T* cell = get_ptr(-1);
      *cell = value;
      head_ = getIndex(-1);
      return;
    }
    new (get_ptr(-1)) T(value);
    head_ = getIndex(-1);
    ++size_;
  }

  void pop_back() {
    if (empty()) return;
    --size_;
    get_ptr(size_)->~T();
  }

  void pop_front() {
    if (empty()) return;
    get_ptr()->~T();
    head_ = getIndex(1);
    --size_;
  }

  T& operator[](size_t i) {
    return *get_ptr(i);
  }

  const T& operator[](size_t i) const {
    return *get_ptr(i);
  }

  T& at(size_t i) {
    if (i >= size_) {
      throw std::out_of_range("CircularBuffer out of range");
    }
    return operator[](i);
  }

  const T& at(size_t i) const {
    return at(i);
  }

  iterator begin() { return iterator(storage.get_ptr_storage(), 0, head_, capacity()); }
  iterator end() { return iterator(storage.get_ptr_storage(), size_, head_, capacity()); }

  const_iterator begin() const { return const_iterator(storage.get_ptr_storage(), 0, head_, capacity()); }
  const_iterator end() const { return const_iterator(storage.get_ptr_storage(), size_, head_, capacity()); }

  const_iterator cbegin() const { return const_iterator(storage.get_ptr_storage(), 0, head_, capacity()); }
  const_iterator cend() const { return const_iterator(storage.get_ptr_storage(), size_, head_, capacity()); }

  reverse_iterator rbegin() { return reverse_iterator(end()); }
  reverse_iterator rend() { return reverse_iterator(begin()); }

  const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
  const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

  const_reverse_iterator crbegin() const { return const_reverse_iterator(end()); }
  const_reverse_iterator crend() const { return const_reverse_iterator(begin()); }

  void insert(const_iterator iter, const T& value) {
    std::size_t ind = iter.ind;
    if (empty()) {
      size_ = 1;
      new (get_ptr()) T(value);
      return;
    }
    if (full()) {
      if (ind == 0) {
        return;
      }
      --ind;
      pop_front();
    }
    new (get_ptr(size_)) T(*get_ptr(size_ - 1));
    for (size_t i = size_ - 1; i > ind; --i) {
      new (get_ptr(i)) T(*get_ptr(i - 1));
    }
    new (get_ptr(ind)) T(value);
    ++size_;
  }

  void erase(const_iterator iter) {
    std::size_t ind = iter.ind;
    if (ind >= size_) {
      return;
    }
    for (size_t i = ind + 1; i < size_; ++i) {
      new(get_ptr(i - 1)) T(*get_ptr(i));
    }
    --size_;
    get_ptr(size_)->~T();
  }
};