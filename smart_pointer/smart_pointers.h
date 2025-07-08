#include <iostream>
#include <memory>
#include <type_traits>

template <typename T>
class SharedPtr;

template <typename T>
class WeakPtr;

template <typename T>
class EnableSharedFromThis;

struct BaseControlBlock {
	size_t shared_counter;
	size_t weak_counter;

	BaseControlBlock(size_t sh, size_t wk)
		: shared_counter(sh), weak_counter(wk) {}

	virtual ~BaseControlBlock() = default;
	virtual void destroyPtr() = 0;
	virtual void deallocCntrlBlk() = 0;
};

template <typename U, typename Deleter, typename Alloc>
struct ControlBlockPtr : BaseControlBlock {
	U* ptr;
	[[no_unique_address]] Deleter del;
	[[no_unique_address]] Alloc alloc;

	ControlBlockPtr(U* ptr, Deleter del, Alloc alloc)
		: BaseControlBlock(1, 0), ptr(ptr), del(del), alloc(alloc) {}
	
	~ControlBlockPtr() override = default;

	void destroyPtr() override {
		del(ptr);
	}

	void deallocCntrlBlk() override {
		using CtrlBlockAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<ControlBlockPtr<U, Deleter, Alloc>>;
		using CntrlBlockAllocTraits = typename std::allocator_traits<CtrlBlockAlloc>;
		CtrlBlockAlloc cb_alloc = alloc;
		CntrlBlockAllocTraits::deallocate(cb_alloc, this, 1);
	}
};

template <typename U, typename Alloc>
struct ControlBlockMakeShared : BaseControlBlock {
	[[no_unique_address]] Alloc alloc;
	U value;

	template <typename... Args>
	ControlBlockMakeShared(size_t sh, size_t wk, Alloc a, Args&&... args)
		: BaseControlBlock(sh, wk)
		, alloc(a)
		, value(std::forward<Args>(args)...) {}
	
	~ControlBlockMakeShared() override = default;

	void destroyPtr() override {
		using ValAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<U>;
		using ValAllocTraits = std::allocator_traits<ValAlloc>;
		ValAlloc val_alloc = alloc;
		ValAllocTraits::destroy(val_alloc, &value);
	}

	void deallocCntrlBlk() override {
		using CtrlBlockAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<ControlBlockMakeShared<U, Alloc>>;
		using CntrlBlockAllocTraits = typename std::allocator_traits<CtrlBlockAlloc>;
		CtrlBlockAlloc cb_alloc = alloc;
		CntrlBlockAllocTraits::deallocate(cb_alloc, this, 1);
	}
};

template <typename T>
class SharedPtr {
private:
	template <typename U, typename... Args>
	friend SharedPtr<U> makeShared(Args&&...gs);

	template <typename U, typename Allocator, typename... Args>
	friend SharedPtr<U> allocateShared(Allocator, Args&&...);

	template <typename U>
	friend class SharedPtr;

	template <typename U>
	friend class WeakPtr;

private:	
	T* ptr_{nullptr};
	BaseControlBlock* cb_{nullptr};

private:
	template <typename Alloc>
	SharedPtr(ControlBlockMakeShared<T, Alloc>* cb) : ptr_(&(cb->value)), cb_(cb) {}

	SharedPtr(WeakPtr<T> wk) : ptr_(wk.ptr_), cb_(wk.cb_) {
		if (cb_ != nullptr) {
			++cb_->shared_counter;
		}
	}

public:

	SharedPtr() {}

  template <typename Y, typename Deleter = std::default_delete<Y>, typename Alloc = std::allocator<T>> requires(std::is_base_of_v<T, Y> || std::is_same_v<Y, T>)
	SharedPtr(Y* ptr, Deleter del = Deleter(), Alloc alloc = Alloc()) : ptr_(ptr) {
		UpdateEnableSharedFromThis();
    using CtrlBlockAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<ControlBlockPtr<Y, Deleter, Alloc>>;
    using CtrlBlockAllocTraits = typename std::allocator_traits<CtrlBlockAlloc>;

    CtrlBlockAlloc cb_alloc = alloc;
    cb_ = CtrlBlockAllocTraits::allocate(cb_alloc, 1);
	
    new (cb_) ControlBlockPtr(ptr_, del, cb_alloc);
	}

	SharedPtr(const SharedPtr& other) noexcept : ptr_(other.ptr_), cb_(other.cb_) {
		UpdateEnableSharedFromThis();
		if (cb_) {
			++cb_->shared_counter;
		}
	}

  template <typename Y> requires(std::is_base_of_v<T, Y>)
	SharedPtr(const SharedPtr<Y>& other) noexcept : ptr_(other.ptr_), cb_(other.cb_) {
		UpdateEnableSharedFromThis();
		if (cb_) {
			++cb_->shared_counter;
		}
	}

	SharedPtr(SharedPtr&& other) noexcept : ptr_(other.ptr_), cb_(other.cb_) {
		other.ptr_ = nullptr;
    other.cb_ = nullptr;
	}

  template <typename Y> requires(std::is_base_of_v<T, Y>)
	SharedPtr(SharedPtr<Y>&& other) noexcept : ptr_(other.ptr_), cb_(other.cb_) {
		other.ptr_ = nullptr;
    other.cb_ = nullptr;
	}
	
  SharedPtr(const SharedPtr& other, T* ptr) noexcept : ptr_(ptr), cb_(other.cb_) {
		if (cb_) {
			++cb_->shared_counter;
		}
	}

	template<typename Y>
	SharedPtr(const SharedPtr<Y>& other, T* ptr) noexcept : ptr_(ptr), cb_(other.cb_) {
		if (cb_) {
			++cb_->shared_counter;
		}
	}

	SharedPtr& operator=(const SharedPtr& other) noexcept {
    SharedPtr cp(other);
    swap(cp);
    return *this;
  }

  template <typename Y> requires(std::is_base_of_v<T, Y> || std::is_same_v<Y, T>)
	SharedPtr& operator=(const SharedPtr<Y>& other) noexcept {
    SharedPtr cp(other);
    swap(cp);
    return *this;
  }

  template <typename Y> requires(std::is_base_of_v<T, Y>)
	SharedPtr& operator=(SharedPtr<Y>&& other) noexcept {
    SharedPtr cp(std::move(other));
    swap(cp);
    return *this;
  }

	SharedPtr& operator=(SharedPtr&& other) noexcept {
    SharedPtr cp(std::move(other));
    swap(cp);
    return *this;
  }

  void swap(SharedPtr& other) {
    std::swap(ptr_, other.ptr_);
    std::swap(cb_, other.cb_);
  }

	T& operator*() const noexcept {
		return *ptr_;
	}

	T* operator->() const noexcept {
		return ptr_;
	}

	T* get() const noexcept {
		return ptr_;
	}

	size_t use_count() const noexcept {
		return cb_->shared_counter;
	}

	void reset() {
		if (!cb_) {
			return;
		}
		if (--cb_->shared_counter == 0) {
			cb_->destroyPtr();
			if (cb_->weak_counter == 0) {
				cb_->deallocCntrlBlk();
			}
		}
		ptr_ = nullptr;
		cb_ = nullptr;
	}

	template <typename Y> requires(std::is_base_of_v<T, Y>)
	void reset(Y* ptr) {
		SharedPtr<T>(ptr).swap(*this);
	}

	~SharedPtr() {
		reset();
	}

	void UpdateEnableSharedFromThis() {
		if (!cb_) {
			return;
		}
		if constexpr (std::is_base_of_v<EnableSharedFromThis<T>, T>) {
			ptr_->ptr = *this;
		}
	}
};


template <typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
	auto* cb = new ControlBlockMakeShared<T, std::allocator<T>>(1, 0, std::allocator<T>{}, std::forward<Args>(args)...);
	return SharedPtr<T>(cb);
}

template <typename T, typename Alloc, typename... Args>
SharedPtr<T> allocateShared(Alloc alloc, Args&&... args) {
	using CtrlBlockAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<ControlBlockMakeShared<T, Alloc>>;
	CtrlBlockAlloc cb_alloc = alloc;
	auto* cb = cb_alloc.allocate(1);
	std::allocator_traits<CtrlBlockAlloc>::construct(cb_alloc, cb, 1, 0, cb_alloc, std::forward<Args>(args)...);
	return SharedPtr<T>(cb);
};


template <typename T>
class EnableSharedFromThis {
  WeakPtr<T> ptr;

public:
  SharedPtr<T> shared_from_this() const noexcept {
    return ptr.lock();
  }
};


template <typename T>
class WeakPtr {
	T* ptr_{nullptr};
	BaseControlBlock* cb_{nullptr};

public:
  WeakPtr() {}

	WeakPtr(const SharedPtr<T>& sh) : ptr_(sh.ptr_), cb_(sh.cb_) {
		if (cb_) {
			++cb_->weak_counter;
		}
	}

	template <typename Y> requires(std::is_base_of_v<T, Y>)
	WeakPtr(const SharedPtr<Y>& sh) : ptr_(sh.ptr_), cb_(sh.cb_) {
		if (cb_) {
			++cb_->weak_counter;
		}
	}

	WeakPtr(const WeakPtr& other) : ptr_(other.ptr_), cb_(other.cb_) {
		if (cb_) {
			++cb_->weak_counter;
		}
	}

	template <typename Y> requires(std::is_base_of_v<T, Y>)
	WeakPtr(const WeakPtr<Y>& other) : ptr_(other.ptr_), cb_(other.cb_) {
		if (cb_) {
			++cb_->weak_counter;
		}
	}

	WeakPtr(WeakPtr<T>&& other) : ptr_(other.ptr_), cb_(other.cb_) {
		other.ptr_ = nullptr;
		other.cb_ = nullptr;
	}

	template <typename Y> requires(std::is_base_of_v<T, Y>)
	WeakPtr(WeakPtr<Y>&& other) : ptr_(other.ptr_), cb_(other.cb_) {
		other.ptr_ = nullptr;
		other.cb_ = nullptr;
	}

	WeakPtr<T>& operator=(const WeakPtr<T>& other) {
		if (this == &other) {
			return *this;
		}
		WeakPtr cp(other);
		swap(cp);
		return *this;
	}

	template <typename Y> requires(std::is_base_of_v<T, Y>)
	WeakPtr<T>& operator=(const WeakPtr<Y>& other) {
		WeakPtr<T> cp(other);
		swap(cp);
		return *this;
	}

	WeakPtr<T>& operator=(WeakPtr<T>&& other) {
		WeakPtr cp(std::move(other));
		swap(cp);
		return *this;
	}

	template <typename Y> requires(std::is_base_of_v<T, Y>)
	WeakPtr<T>& operator=(WeakPtr<Y>&& other) {
		WeakPtr<T> cp(std::move(other));
		swap(cp);
		return *this;
	}

	void swap(WeakPtr& other) {
		std::swap(ptr_, other.ptr_);
		std::swap(cb_, other.cb_);
	}
	
	~WeakPtr() {
		if (cb_ == nullptr) {
			return;
		}
		if (--cb_->weak_counter == 0 && cb_->shared_counter == 0) {
			cb_->deallocCntrlBlk();
		}
	}

	size_t use_count() const noexcept {
		if (cb_ == nullptr) {
			return 0;
		}
		return cb_->shared_counter;
	}

	bool expired() const noexcept {
		return use_count() == 0;
	}

	SharedPtr<T> lock() const noexcept {
		if (expired()) {
			return SharedPtr<T>();
		}
		return SharedPtr<T>(*this);
	}

	template <typename U>
	friend class WeakPtr;

	template <typename U>
	friend class SharedPtr;
};