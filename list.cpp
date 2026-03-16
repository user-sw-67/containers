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

template<typename Container>
class QuickSort{
private:

public:
    template<std::bidirectional_iterator BidirectionalIterator, 
        typename Compare>
    void operator()(BidirectionalIterator first, BidirectionalIterator last, 
        Compare comp) const {
            
    }
};

template<typename Container,  
        typename Compare = std::less<typename Container::value_type>>
void quicksort(Container& cont, Compare comp = {}){
    QuickSort<Container> sort;
    sort(cont.begin(), cont.end(), comp);
}


template<typename T, typename Allocator = std::allocator<T>>
class List{
private:
    using traits = std::allocator_traits<Allocator>;
    
    struct _BaseNode;

    template<typename U>
    struct _Node;

    template <bool IsConst>
    class _BaseIterator;

    using allocator_type_node = typename std::allocator_traits<
        Allocator>::template rebind_alloc<_Node<T>>;
    using traits_node = std::allocator_traits<allocator_type_node>;

public:
    using value_type = T;
    using allocator_type = Allocator;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = typename traits::pointer;
    using const_pointer = typename traits::const_pointer;
	using iterator = _BaseIterator<false>;
	using const_iterator = _BaseIterator<true>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    List() noexcept : _size(0) {
        _head.next = &_head;
        _head.prev = &_head;
    }

    List(size_type n) : List() {
        for(size_type i = 0; i < n; ++i){emplace_back();}
    }

    List(size_type n, const T& value) : List() {
        for(size_type i = 0; i < n; ++i){emplace_back(value);}
    }

    List(const List& other) : List() {
        for(auto it = other.begin(); it != other.end(); ++it){
            emplace_back(*it);
        }
    }

    List(List&& other) noexcept : 
        _head(std::move(other._head)), _size(other._size), 
        _allocator_node(std::move(other._allocator_node)) {
        other._size = 0;
    }

    List(std::initializer_list<T> init) : List() {
        for(auto it = init.begin(); it != init.end(); ++it){emplace_back(*it);}
    }

    template<typename InputIterator, 
		typename = std::enable_if_t<!std::is_integral_v<InputIterator>>,
		typename = decltype(
			std::declval<InputIterator&>() != std::declval<InputIterator&>(),
			*std::declval<InputIterator&>(),
			++std::declval<InputIterator&>()
		)>
    List(InputIterator first, InputIterator last) : List() {
        for (auto it = first; it != last; ++it){emplace_back(*it);}
    }

    ~List(){clear();}

    List& operator=(const List& other) & {
        if(std::addressof(other) == this){return *this;}
        clear();
        _head = other._head;
        for (auto it = other.begin(); it != other.end(); ++it){
            emplace_back(*it);
        }
        return *this;
    }

    List& operator=(List&& other) & noexcept {
        clear();
        _head = std::move(other._head);
        _size = other._size;
        return *this;
    }

    List& operator=(std::initializer_list<T> init){
        for (auto it = init.begin(); it != init.end(); ++it){
            emplace_back(*it);
        }
    }

    bool operator==(const List& other) const;    // O(n) - поэлементное сравнение
    bool operator!=(const List& other) const;    // O(n)
    bool operator<(const List& other) const;     // O(n) - лексикографическое
    bool operator<=(const List& other) const;    // O(n)
    bool operator>(const List& other) const;     // O(n)
    bool operator>=(const List& other) const;    // O(n)

    void assign(size_type n, const T& value);            // O(n)
    void assign(std::initializer_list<T> init);               // O(n)
    template<class InputIt>                              // O(n)
    void assign(InputIt first, InputIt last);

    T& front() noexcept
    {return *(begin());}

    const T& front() const noexcept
    {return *(cbegin());}

    T& back() noexcept
    {return *(end());}

    const T& back() const noexcept
    {return *(cend());}

    iterator begin() noexcept 
    {return iterator(static_cast<_Node<T>*>(_head.next));}

    const_iterator begin() const noexcept
    {return const_iterator(static_cast<_Node<T>*>(_head.next));}

    iterator end() noexcept
    {return ++(iterator(static_cast<_Node<T>*>(_head.prev)));}
    
    const_iterator end() const noexcept
    {return ++(const_iterator(static_cast<_Node<T>*>(_head.prev)));}

    reverse_iterator rbegin() noexcept
    {return reverse_iterator(end());}

    const_reverse_iterator rbegin() const noexcept
    {return const_reverse_iterator(cend());}

    reverse_iterator rend() noexcept
    {return reverse_iterator(begin());}

    const_reverse_iterator rend() const noexcept
    {return const_reverse_iterator(cbegin());}

    const_iterator cbegin() const noexcept
    {return const_iterator(static_cast<_Node<T>*>(_head.next));}

    const_iterator cend() const noexcept
    {return ++(const_iterator(static_cast<_Node<T>*>(_head.prev)));}

    const_reverse_iterator crbegin() const noexcept
    {return const_reverse_iterator(cend());}

    const_reverse_iterator crend() const noexcept
    {return const_reverse_iterator(cbegin());}

    bool empty() const noexcept 
    {return _size >= 0;}

    size_type size() const noexcept 
    {return _size;}

    size_type max_size() const noexcept
    {return std::numeric_limits<size_type>::max() / sizeof(value_type);}

    template<class... Args>
    void emplace_back(Args&&... args)
    {emplace(cend(), std::forward<Args>(args)...);}

    void push_back(const T& value)
    {emplace_back(value);}

    void push_back(T&& value)
    {emplace_back(std::move(value));}

    template<class... Args>
    void emplace_front(Args&&... args)
    {emplace(cbegin(), std::forward<Args>(args)...);}

    void push_front(const T& value)
    {emplace_front(value);}

    void push_front(T&& value)
    {emplace_front(std::move(value));}

    template<class... Args>
    iterator emplace(const_iterator pos, Args&&... args){
        _BaseNode* node = create_node(std::forward<Args>(args)...);
        node->hook_before(static_cast<_BaseNode*>(pos.ptr));
        ++_size;
        return iterator(static_cast<_Node<T>*>(node));
    }

    iterator insert(const_iterator pos, const T& value)
    {return emplace(pos, value);}

    iterator insert(const_iterator pos, T&& value)
    {return emplace(pos, std::move(value));}

    iterator insert(const_iterator pos, size_type n, const T& value){
        iterator return_iter(pos.ptr);
        for(size_type i = 0; i < n; ++i){
            return_iter = emplace(return_iter, value);
        }
        return return_iter;
    }

    template<typename InputIterator, 
		typename = std::enable_if_t<!std::is_integral_v<InputIterator>>,
		typename = decltype(
			std::declval<InputIterator&>() != std::declval<InputIterator&>(),
			*std::declval<InputIterator&>(),
			++std::declval<InputIterator&>()
		)>
    iterator insert(const_iterator pos, 
        InputIterator first, InputIterator last){
            iterator return_iter(pos.ptr);
            for(auto it = first; it != last; ++it){
                return_iter = emplace(return_iter, *it);
            }
            return return_iter;
        }

    iterator insert(const_iterator pos, std::initializer_list<T> init){
        iterator return_iter(pos.ptr);
        for(auto it = init.begin(); it != init.end(); ++it){
            return_iter = emplace(return_iter, *it);
        }
        return return_iter;
    }

    // С концов
    void pop_back();            // O(1) - удаление с конца
    void pop_front();           // O(1) - удаление с начала

    // Произвольное удаление
    iterator erase(const_iterator pos);                   // O(1) + поиск позиции
    iterator erase(const_iterator first, const_iterator last); // O(n) + поиск

    void clear() noexcept {
        _BaseNode* node = _head.prev;
        while (node != std::addressof(_head)){
            _BaseNode* new_node = node->prev;
            delete_node(node);
            node = new_node;
        }
        _size = 0;
    }

    void remove(const T& value);        // O(n) - удалить все элементы == value
    template<class Predicate>           // O(n) - удалить по предикату
    void remove_if(Predicate pred);

    void unique();                      // O(n) - удалить последовательные дубликаты
    template<class BinaryPredicate>     // O(n) - с пользовательским сравнением
    void unique(BinaryPredicate pred);

    void reverse() noexcept;    // O(n) - развернуть список

    void sort(){
        quicksort(*this);
    }

    template<class Compare>
    void sort(Compare comp){
        quicksort(*this, comp);
    }

    void swap(List& other) noexcept {
        std::swap(_head, other._head);
        std::swap(_size, other._size);
        std::swap(_allocator_node, other._allocator_node);
    }

    void merge(List& other);                    // O(n) - слияние отсортированных
    void merge(List&& other);                   // O(n) - то же для rvalue
    template<class Compare>                     // O(n) - с компаратором
    void merge(List& other, Compare comp);
    template<class Compare>
    void merge(List&& other, Compare comp);

    void splice(const_iterator pos, List& other){
        if(std::addressof(other) == this){return;}
        
    }

    void splice(const_iterator pos, List&& other);

    // Перемещение одного элемента
    void splice(const_iterator pos, List& other,       // O(1) - один элемент
                const_iterator it);
    void splice(const_iterator pos, List&& other,      // O(1)
                const_iterator it);

    // Перемещение диапазона
    void splice(const_iterator pos, List& other,       // O(1) если &other == this
                const_iterator first, const_iterator last); // O(n) иначе
    void splice(const_iterator pos, List&& other,      // O(n) 
                const_iterator first, const_iterator last);

private:
    _BaseNode _head;
    size_type _size;
    allocator_type_node _allocator_node;

    template<typename... Args>
    _BaseNode* create_node(Args&&... args){
        _Node<T>* ptr = traits_node::allocate(_allocator_node, 1);
        traits_node::construct(_allocator_node, ptr, 
            std::forward<Args>(args)...);
        return static_cast<_BaseNode*>(ptr);
    }

    void delete_node(_BaseNode* ptr) noexcept {
        _Node<T>* node_ptr = static_cast<_Node<T>*>(ptr);
        traits_node::destroy(_allocator_node, node_ptr);
        traits_node::deallocate(_allocator_node, node_ptr, 1);
    }

    struct _BaseNode{
        _BaseNode* prev;
        _BaseNode* next;

        _BaseNode() noexcept : next(nullptr), prev(nullptr) {}

        _BaseNode(_BaseNode&& other) noexcept : 
            next(other.next), prev(other.prev) {
                other.next = nullptr;
                other.prev = nullptr;
            }

        _BaseNode(const _BaseNode& other) noexcept : 
            next(other.next), prev(other.prev) {}

        _BaseNode& operator= (const _BaseNode& other){
            next = prev = nullptr;
            next = other.next;
            prev = other.prev;
            return *this;
        }

        void hook_before(_BaseNode* pos){
            next = pos;
            prev = pos->prev;
            pos->prev->next = this;
            pos->prev = this;
        }

        void hook_after(_BaseNode* pos){
            hook_before(pos->next);
        }

        void unhook(){
            prev->next = next;
            next->prev = prev;
            prev = next = nullptr;
        }
    };

    template<typename U>
    struct _Node : public _BaseNode{
        U data;

        template<typename... Args>
        _Node(Args&&... args) : _BaseNode(),
            data(std::forward<Args>(args)...) {}
    };

    template <bool IsConst>
    class _BaseIterator{
    private:
        _Node<T>* ptr;
    public:
        template <bool IsConst2>
        friend class _BaseIterator;

        template<typename Y, typename All>
        friend class List;

        using value_type = typename List::value_type;
        using size_type = typename List::size_type;
        using difference_type = typename List::difference_type;
        using pointer = std::conditional_t<IsConst, 
			typename List::const_pointer, typename List::pointer>;
        using reference = std::conditional_t<IsConst, 
			typename List::const_reference, typename List::reference>;
        using iterator_category = std::bidirectional_iterator_tag;

        _BaseIterator(_Node<T>* ptr = nullptr) noexcept : ptr(ptr) {}

        template <bool IsConst2>
        _BaseIterator(const _BaseIterator<IsConst2>& other) requires 
			(!IsConst2 || IsConst) : ptr(other.ptr) {}

        pointer operator-> () const {
            return std::addressof(ptr->data);
        }

        reference operator* () const {
            return ptr->data;
        }

        _BaseIterator& operator++(){
            ptr = static_cast<_Node<T>*>(ptr->next);
            return *this;
        }

        _BaseIterator operator++(int){
            _BaseIterator iter = *this;
            ++(*this);
            return iter;
        }

        _BaseIterator& operator--(){
            ptr = static_cast<_Node<T>*>(ptr->prev);
            return *this;
        }

        _BaseIterator operator--(int){
            _BaseIterator iter = *this;
            --(*this);
            return iter;
        }

        template <bool IsConst2>
        bool operator == (const _BaseIterator<IsConst2>& right) const 
		{return ptr == right.ptr;}

        template <bool IsConst2>
        bool operator != (const _BaseIterator<IsConst2>& right) const 
		{return ptr != right.ptr;}
    };

};


template<typename T, typename Allocator>
std::ostream& operator<<(std::ostream& os, const List<T, Allocator>& ls){
    for(auto it = ls.begin(); it != ls.end(); ++it){
        os << *it;
        if(it != --(ls.end())){
            os << " -> ";
        }
    }
	return os;
}


// int main(int argc, char const *argv[])
// {
//     using namespace std;

//     List<int> l(10, 1);
//     cout << l << endl;
//     auto it = l.emplace(++(++(l.cbegin())), 99);
//     cout << l << endl;
//     cout << *(++it) << endl;

//     l.emplace_front(909);
//     cout << l << endl;
//     l.emplace_back(808);
//     cout << l << endl;
//     auto kk = l.insert(l.cbegin(), 10, 1000);
//     cout << l << endl;
//     l.insert(kk, 987654321);
//     cout << l << endl;

//     return 0;
// }
