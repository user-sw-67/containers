#include <memory>
#include <cstddef>
#include <utility>
#include <type_traits>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <algorithm>
#include <ranges>
#include <cassert>

#include "../allocators/allocators.hpp"

template<typename T, typename Allocator = std::allocator<int>>
class Deque{
private:
	using traits = std::allocator_traits<Allocator>;
	template<bool IsConst>
	class _BaseIterator;
};

int main(int argc, char const *argv[])
{
    
    return 0;
}
