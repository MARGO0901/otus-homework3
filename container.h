#pragma once

#include <iostream>
#include <memory>

#include "allocator.h"

template<typename T>
struct Node {
    T data; 
    Node* next;

    Node(T value): data(value), next(nullptr) {};
};

template<typename T, size_t N>
using MyPoolAllocator = PoolAllocator<T, N>;

template<typename T, typename Allocator = std::allocator<Node<T>>>
class CustomList {
public:
    using allocator_type = Allocator;
    using allocator_traits = std::allocator_traits<allocator_type>;

    CustomList() : head(nullptr), tail(nullptr), allocator(Allocator()) {};

    explicit CustomList(const Allocator &alloc)
        : head(nullptr), tail(nullptr), allocator(alloc) {}

    ~CustomList() { clear(); }

    void push_back(T val) {
        Node<T>* new_node = allocator_node(val);

        if (head == nullptr) {
            head = new_node;
            tail = new_node;
        }
        else {
            tail->next = new_node;
            tail = new_node;
        }
    }

    void display() {
        Node<T>* current = head;
        while(current) {
            std::cout << current->data << " ";
            current = current->next;
        }
        std::cout << std::endl;
    }

    void clear() {
        Node<T> *current = head;
        while(current) {
            Node<T>* next = current->next;
            destroy_node(current);
            current = next;
        }
        head = nullptr;
        tail = nullptr;
    }

    std::size_t size() const {
        std::size_t s = 0;
        Node<T> *current = head;
        while(current) {
            current = current->next;
            s++;
        }
        return s;
    }

    bool empty() const {
        return (!head);
    }

    Node<T>* begin() const {
        return head;
    }

    Node<T>* end() const {
        return tail->next;
    }

private:
    Node<T> *head;
    Node<T> *tail;
    allocator_type allocator;

    Node<T>* allocator_node(const T& val) {
        //выделение памяти под Node
        Node<T>*p = allocator_traits::allocate(allocator, 1);
        new (p) Node<T>(val);   //placement new
        p->next = nullptr;
        return p;
    }

    void destroy_node(Node<T>* p) {
        p->~Node<T>();   //вызов деструктора вручную
        allocator_traits::deallocate(allocator, p, 1);
    }
};