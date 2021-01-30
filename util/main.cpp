#include "measure.h"

#include <numeric>
#include <iostream>
#include <list>
#include <vector>

// Compare the performance of std::vector vs std::list
void vector_vs_list()
{
    // assuming 32bit elements:
    // 1024 = 4KB, size of L1 cache
    // 1024 * 8 = 32KB, size of L2
    // 1024 * 2000 = 8MB, size of L3
    const auto size = 1024 * 2000;
    std::vector<int> v;
    v.resize(size);
    std::list<int> l;

    for (auto i = 0; i < size; i ++)
    {
        v.push_back(i);
        l.push_back(i);
    }

    auto fv = [&v]() { return std::accumulate(v.begin(), v.end(), 0); };
    auto fl = [&l]() { return std::accumulate(l.begin(), l.end(), 0); };

    auto tv = measure(fv);
    auto tl = measure(fl);

    std::cout << "tv = " << tv << "; tl = " << tl << std::endl;
}

int main()
{
    vector_vs_list();

    return 0;
}