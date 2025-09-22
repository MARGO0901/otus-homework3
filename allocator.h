#pragma once

#include <iostream>
#include <new>

struct PoolAllocatorConfig {
  static bool allow_expand;
  static bool elem_deall;
};


// SIMPLEST pool allocator - fixed size blocks with free list
template <typename T> class PoolAllocator {
private:
  // Our memory pool
  static char *pool;
  static void **free_list; // Linked list of free blocks
  static size_t block_size;
  static size_t total_blocks;
  static bool initialized;

  static size_t POOL_BLOCKS; // Number of blocks in pool

  // Initialize pool with free list
  static void init_pool() {
    if (initialized)
      return;

    block_size = sizeof(T) > sizeof(void *) ? sizeof(T) : sizeof(void *);
    total_blocks = POOL_BLOCKS;

    // Allocate big chunk
    pool = static_cast<char *>(std::malloc(block_size * total_blocks));

    // Build free list - each block points to next free block
    free_list = nullptr;
    for (int i = total_blocks - 1; i >= 0; --i) {
      void **block = reinterpret_cast<void **>(pool + i * block_size);
      *block = free_list; // Point to previous free block
      free_list = block;  // This block becomes head of free list
    }

    initialized = true;
    std::cout << "Pool initialized: " << total_blocks << " blocks of "
              << block_size << " bytes\n";
  }

public:
  using value_type = T;

  PoolAllocator() = default;

  template <typename U> PoolAllocator(const PoolAllocator<U> &other) {}

  // Take first block from free list
  T *allocate(size_t n) {
    if (n != 1) {
      std::cout << "Pool only supports single object allocation, falling "
                   "back to malloc\n";
      return static_cast<T *>(std::malloc(n * sizeof(T)));
    }

    init_pool();

    // Check if we have free blocks
    if (!free_list) {
      if (PoolAllocatorConfig::allow_expand) {
        std::cout << "Pool exhausted! Falling back to malloc\n";
        return static_cast<T *>(std::malloc(sizeof(T)));
      } else {
        throw std::bad_alloc();
      }
    }

    // Take first free block
    void *result = free_list;
    free_list =
        reinterpret_cast<void **>(*free_list); // Move to next free block

    std::cout << "Allocated block from pool\n";
    return static_cast<T *>(result);
  }

  // Return block to free list
  void deallocate(T *ptr, size_t n) {
    if (n != 1) {
      std::cout << "Freeing non-pool memory\n";
      std::free(ptr);
      return;
    }

    // Check if pointer is from our pool
    char *char_ptr = reinterpret_cast<char *>(ptr);
    if (char_ptr < pool || char_ptr >= pool + (block_size * total_blocks)) {
      std::cout << "Freeing non-pool memory\n";
      std::free(ptr);
      return;
    }

    if (!PoolAllocatorConfig::elem_deall) {
      // Не возвращаем в пул — пул очистится в cleanup
      std::cout << "Element deallocation disabled, skipping return to pool\n";
      return;
    }

    // Return to free list
    void **block = reinterpret_cast<void **>(ptr);
    *block = free_list; // Point to current free list head
    free_list = block;  // This block becomes new head

    std::cout << "Returned block to pool\n";
  }

  static void print_free_blocks() {
    int count = 0;
    void **current = free_list;
    while (current) {
      count++;
      current = static_cast<void **>(*current);
    }
    std::cout << "Free blocks available: " << count << "\n";
  }

  static void cleanup() {
    if (pool) {
      std::cout << "[DEBUG] cleanup() - pool is allocated, freeing. "
                << typeid(T).name() << "\n";
      std::free(pool);
      pool = nullptr;
      free_list = nullptr;
      initialized = false;
      std::cout << "Pool destroyed\n";
    } else {
      std::cout << "[DEBUG] cleanup() - pool is already nullptr"
                << typeid(T).name() << "\n";
    }
  }

  template <typename U, typename... Args> void construct(U *p, Args &&...args) {
    ::new ((void *)p) U(std::forward<Args>(args)...);
  }

  template <typename U> void destroy(U *p) { p->~U(); }

  template <typename U> struct rebind {
    using other = PoolAllocator<U>;
  };

  static void SetExpand(bool expand) {
    PoolAllocatorConfig::allow_expand = expand;
  }

  static void SetElDeall(bool el_deall) {
    PoolAllocatorConfig::elem_deall = el_deall;
  }
};

// Initialize static members
template <typename T> char *PoolAllocator<T>::pool = nullptr;

template <typename T> void **PoolAllocator<T>::free_list = nullptr;

template <typename T> size_t PoolAllocator<T>::POOL_BLOCKS = 5;

template <typename T> size_t PoolAllocator<T>::block_size = 0;

template <typename T> size_t PoolAllocator<T>::total_blocks = 0;

template <typename T> bool PoolAllocator<T>::initialized = false;

// Required comparison operators
template <typename T, typename U>
bool operator==(const PoolAllocator<T> &, const PoolAllocator<U> &) {
  return true;
}

template <typename T, typename U>
bool operator!=(const PoolAllocator<T> &, const PoolAllocator<U> &) {
  return false;
}
