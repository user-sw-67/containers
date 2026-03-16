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
#include <functional>
#include <concepts>

#include "../allocators/allocators.hpp"


template<
    typename Key, // Тип ключа
    typename T, // Тип значения
    typename Hash = std::hash<Key>, // Тип функции хеширования
    typename KeyEqual = std::equal_to<Key>, // Тип функции сравнения ключей
    typename Allocator = std::allocator<std::pair<const Key, T>>> // Тип аллокатора
class UnorderedMap{
private:

    struct Node{
        std::pair<const Key, T> value;
        Node* next = nullptr;
        size_t hash_value;

        void push(const Node& node) noexcept {
            next = &node;
        }
    };

    using AllocatorTraits = typename 
        std::allocator_traits<Allocator>::rebind_alloc<Node>;

    Node** buckets = nullptr; // bucket-массив
    size_t size_bucket; // Кол-во bucket-ов
    size_t size_elem; // Кол-во элементов

    const double max_load_factor = 1.0; // Максимальный фактор rehash, load_factor вычисляется (size_elem / size_bucket)

    Hash hash_func; // Хеш-функция
    KeyEqual key_equal_func; // Функтор-равенства
    AllocatorTraits allocator; // Аллокатор






};

int main(int argc, char const *argv[])
{
    
    return 0;
}
