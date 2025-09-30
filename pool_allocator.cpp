#include <iostream>
#include <map>

#include "allocator.h"
#include "container.h"

bool PoolAllocatorConfig::allow_expand = false;
bool PoolAllocatorConfig::elem_deall = false;

int factorial(int n) {
    if (n <= 1)
        return 1;
    return n * factorial(n - 1);
}

int main(int argc, char** argv) {

    bool allow_expand = false;
    bool elem_deall = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--exp") {
            allow_expand = true;
        }
        if (arg == "--el_deall") {
            elem_deall = true;
        }
    }

    std::map<int, int> simpleMap;
    for (int i = 0; i < 10; ++i) {
        simpleMap.emplace(i, factorial(i));
    }
    //-----------------------------------------------------

    {
        using MapAllocator = PoolAllocator<std::pair<const int, int>>;

        MapAllocator::SetExpand(allow_expand);
        MapAllocator::SetElDeall(elem_deall);

        std::map<int, int, std::less<int>, MapAllocator> myMap{MapAllocator(10)};

        // Вставка элементов
        for (int i = 0; i < 10; i++) {
            myMap.emplace(i, factorial(i));
        }

        LOG << "Map contents:\n";
        for (const auto& [key, value] : myMap)
            std::cout << key << " " << value << '\n';

        // Удаление элемента с key == 2
        // myMap.erase(2);

        // std::cout << "After erasing key=2:\n";
        // for (const auto& [key, value] : myMap)
        //     std::cout <<  key << " " << value << '\n';

        MapAllocator::cleanup();
    }

    //-----------------------------------------------

    CustomList<int> simpleList;
    for (int i = 0; i < 10; ++i) {
        simpleList.push_back(i);
    }
    //simpleList.display();

    //-----------------------------------------
    {
        using IntAllocator = PoolAllocator<Node<int>>;

        CustomList<int, IntAllocator> myList{IntAllocator(10)};
        for (int i = 0; i < 10; ++i) {
            myList.push_back(i);
        }
        myList.display();

        LOG << "myList.size() = " << myList.size() << std::endl;

        auto it = myList.begin();
        auto it_end = myList.end();

        while (it != it_end) {
            std::cout << it->data << " ";   // тут должен быть at()
            it = it->next;                    //а здесь ++
        }

        myList.clear();

        IntAllocator::cleanup();
    }

    return 0;
}