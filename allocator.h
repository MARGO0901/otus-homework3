#pragma once

#include <iostream>
#include <new>

constexpr bool verbose = false;

#define LOG if (verbose) std::cout

struct PoolAllocatorConfig {
    static bool allow_expand;
    static bool elem_deall;
};


// SIMPLEST pool allocator - fixed size blocks with free list
template <typename T, size_t N> class PoolAllocator {
private:
  // Our memory pool
  static char *pool;
  static void **free_list; // Linked list of free blocks
  static size_t block_size;
  static bool initialized;

  // Initialize pool with free list
  static void init_pool() {
    if (initialized)
      return;

    block_size = sizeof(T) > sizeof(void *) ? sizeof(T) : sizeof(void *);

    // Allocate big chunk
    pool = static_cast<char *>(::operator new(block_size * N));

    // Build free list - each block points to next free block
    free_list = nullptr;
    for (int i = N - 1; i >= 0; --i) {
      void **block = reinterpret_cast<void **>(pool + i * block_size);
      *block = free_list; // Point to previous free block
      free_list = block;  // This block becomes head of free list
    }

    initialized = true;
    LOG << "Pool initialized: " << N << " blocks of " << block_size
        << " bytes\n";
    }

public:
    using value_type = T;

    PoolAllocator() = default;

    template <typename U> PoolAllocator(const PoolAllocator<U, N> &other) {}

    // Take first block from free list
    T *allocate(size_t n) {

        init_pool();

        if (n != 1) {
            LOG << "Pool only supports single object allocation, falling "
                        "back to malloc\n";
            return static_cast<T *>(::operator new(n * sizeof(T)));
        }

        // Check if we have free blocks
        if (!free_list) {
            if (PoolAllocatorConfig::allow_expand) {
                LOG << "Pool exhausted! Falling back to malloc\n";
                return static_cast<T *>(::operator new(sizeof(T)));
            } else {
                throw std::bad_alloc();
            }
        }

        // Take first free block
        void *result = free_list;
        free_list =
            reinterpret_cast<void **>(*free_list); // Move to next free block

        LOG << "Allocated block from pool\n";
        return static_cast<T *>(result);
    }

    // Return block to free list
    void deallocate(T *ptr, size_t n) {
        if (n != 1) {
            LOG << "Freeing non-pool memory\n";
            ::operator delete(ptr);
            return;
        }

        // Check if pointer is from our pool
        char *char_ptr = reinterpret_cast<char *>(ptr);
        if (char_ptr < pool || char_ptr >= pool + (block_size * N)) {
            LOG << "Freeing non-pool memory\n";
            ::operator delete(ptr);
            return;
        }

        if (!PoolAllocatorConfig::elem_deall) {
            // Не возвращаем в пул — пул очистится в cleanup
            LOG << "Element deallocation disabled, skipping return to pool\n";
            return;
        }

        // Return to free list
        void **block = reinterpret_cast<void **>(ptr);
        *block = free_list; // Point to current free list head
        free_list = block;  // This block becomes new head

        LOG << "Returned block to pool\n";
    }

    static void print_free_blocks() {
        int count = 0;
        void **current = free_list;
        while (current) {
        count++;
        current = static_cast<void **>(*current);
        }
        LOG << "Free blocks available: " << count << "\n";
    }

    static void cleanup() {
        if (pool) {
            LOG << "[DEBUG] cleanup() - pool is allocated, freeing. "
                        << typeid(T).name() << "\n";
            ::operator delete(pool);
            pool = nullptr;
            free_list = nullptr;
            initialized = false;
            LOG << "Pool destroyed\n";
        } else {
            LOG << "[DEBUG] cleanup() - pool is already nullptr"
                        << typeid(T).name() << "\n";
        }
    }

    template <typename U, typename... Args> 
    void construct(U *p, Args &&...args) {
        ::new ((void *)p) U(std::forward<Args>(args)...);
    }

    template <typename U> 
    void destroy(U *p) { p->~U(); }

    template <typename U> 
    struct rebind {
        using other = PoolAllocator<U, N>;
    };

    static void SetExpand(bool expand) {
        PoolAllocatorConfig::allow_expand = expand;
    }

    static void SetElDeall(bool el_deall) {
        PoolAllocatorConfig::elem_deall = el_deall;
    }
};

// Initialize static members
template <typename T, size_t N>
char *PoolAllocator<T, N>::pool = nullptr;

template <typename T, size_t N>
void **PoolAllocator<T, N>::free_list = nullptr;

template <typename T, size_t N>
size_t PoolAllocator<T, N>::block_size = 0;

template <typename T, size_t N>
bool PoolAllocator<T, N>::initialized = false;


// Required comparison operators
template <typename T, size_t N, typename U, size_t M>
bool operator==(const PoolAllocator<T, N> &, const PoolAllocator<U, M> &) {
    return true;
}

template <typename T, size_t N, typename U, size_t M>
bool operator!=(const PoolAllocator<T, N> &, const PoolAllocator<U, M> &) {
    return false;
}
