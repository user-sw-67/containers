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


template<typename T, typename Allocator = std::allocator<T>>
class Vector{
private:
	using traits = std::allocator_traits<Allocator>;

	template<bool IsConst>
	class _BaseIterator;

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

	Vector() noexcept (
	        std::is_nothrow_default_constructible_v<Allocator>
        ) : Vector(Allocator()) {}

    explicit Vector(const Allocator& allocator) :
        _allocator(allocator), 
        _size(0),
        _capacity(0),
        _data(nullptr) {}

    explicit Vector(size_type count,
        const Allocator& allocator = Allocator()) : 
        _allocator(allocator), 
        _size(count > 0 ? count : 0),
	    _capacity(count > 0 ? count : 0),
	    _data(nullptr) {
            if (count <= 0){return;}
			if (_capacity > max_size()) {
				throw std::length_error("limit on the number of elements");
			}
	        size_type i = 0;
 	        _data = traits::allocate(_allocator, count);
 	        try{
 	            for (; i < count; ++i){
 	                traits::construct(_allocator, _data + i);
                 }
 	        }catch(...){
 	            for (size_type j = 0; j < i; ++j){
 	                traits::destroy(_allocator, _data + j);
 	            }
 	            traits::deallocate(_allocator, _data, count);
 	            _data = nullptr;
 	            _capacity = 0;
 	            _size = 0;
 	            throw;
 	        }
	    }

	Vector(size_type count, const T& value, 
	    const Allocator& allocator = Allocator()) : 
	    _allocator(allocator),
	    _size(count > 0 ? count : 0),
	    _capacity(count > 0 ? count : 0),
	    _data(nullptr) {
	        if (count <= 0){return;}
			if (_capacity > max_size()) {
				throw std::length_error("limit on the number of elements");
			}
	        size_type i = 0;
	        _data = traits::allocate(_allocator, count);
	        try{
	            for (; i < count; ++i){
	                traits::construct(_allocator, _data + i, value);
                }
	        }catch(...){
	            for (size_type j = 0; j < i; ++j){
	                traits::destroy(_allocator, _data + j);
	            }
	            traits::deallocate(_allocator, _data, count);
	            _data = nullptr;
 	            _capacity = 0;
 	            _size = 0;
	            throw;
	        }
	    }

	template<typename InputIterator, 
		typename = std::enable_if_t<!std::is_integral_v<InputIterator>>,
		typename = decltype(
			std::declval<InputIterator&>() != std::declval<InputIterator&>(),
			*std::declval<InputIterator&>(),
			++std::declval<InputIterator&>()
		)>
	Vector(InputIterator first, InputIterator last, 
	    const Allocator& allocator = Allocator()) : 
	    _allocator(allocator),
        _size(0),
	    _capacity(0),
	    _data(nullptr){
	        for(auto it = first; it != last; ++it){
	            push_back(*it);
	        }
	    }

	Vector(const Vector& other) : 
	    Vector(other, traits::select_on_container_copy_construction(
	        other._allocator)) {}
	
	Vector(const Vector& other, const Allocator& allocator) :
	    _allocator(allocator),
	    _size(other._size),
	    _capacity(other._size),
	    _data(nullptr) {
	        if(_size <= 0){return;}
	        _data = traits::allocate(_allocator, _capacity);
	        size_type i = 0;
	        try{
	            for(; i < _size; ++i){
	                traits::construct(_allocator, _data + i, other._data[i]);
	            }
	        }catch(...){
	            for(size_type j = 0; j < i; ++j){
	                traits::destroy(_allocator, _data + j);
	            }
	            traits::deallocate(_allocator, _data, _capacity);
	            _capacity = 0;
	            _size = 0;
	            _data = nullptr;
	            throw;
	        }
	    }

	Vector(Vector&& other) noexcept :
	    _allocator(std::move(other._allocator)),
	    _size(other._size), 
	    _capacity(other._size),
	    _data(other._data) {
	        other._data = nullptr;
 	        other._capacity = 0;
 	        other._size = 0;
	    }
	
	Vector(Vector&& other, const Allocator& allocator) :
	    _allocator(allocator),
	    _size(0),
	    _capacity(0),
	    _data(nullptr) {
	        if(other.get_allocator() == get_allocator()){
                _data = other._data;
                _capacity = other._capacity;
                _size = other._size;
	            other._data = nullptr;
     	        other._capacity = 0;
     	        other._size = 0;
	        }else{
	            if(other._size <= 0){return;}
	            _capacity = other._size;
	            _data = traits::allocate(_allocator, _capacity);
	            size_type i = 0;
	            try{
	                for(; i < other._size; ++i){
	                    traits::construct(_allocator, _data + i, 
	                        std::move_if_noexcept(other._data[i]));
	                }
	                _size = other._size;
	            }catch(...){
	                for(size_type j = 0; j < i; ++j){
	                    traits::destroy(_allocator, _data + j);
	                }
	                traits::deallocate(_allocator, _data, _capacity);
	                _data = nullptr;
	                _capacity = 0;
	                throw;
	            }
	            for(size_type n = 0; n < other._size; ++n){
	                traits::destroy(other._allocator, other._data + n);
	            }
	            other._size = 0;
	        }
	    }

	Vector(std::initializer_list<T> init, 
	    const Allocator& allocator = Allocator()) :
	    _allocator(allocator),
	    _size(init.size()),
	    _capacity(init.size()),
	    _data(nullptr) {
	        if(_size <= 0){return;}
			if (_capacity > max_size()) {
				throw std::length_error("limit on the number of elements");
			}
	        _data = traits::allocate(_allocator, _capacity);
	        size_type i = 0;
	        try{
	            for(auto it = init.begin(); it != init.end(); ++it, ++i){
	                traits::construct(_allocator, _data + i, *it);
	            }
	        }catch(...){
	            for(size_type j = 0; j < i; ++j){
	                traits::destroy(_allocator, _data + j);
	            }
	            traits::deallocate(_allocator, _data, _capacity);
	            _size = 0;
	            _capacity = 0;
	            _data = nullptr;
	            throw;
	        }
	    }

    ~Vector() noexcept {
        if (_data){
            for (size_type i = 0; i < _size; ++i){
                traits::destroy(_allocator, _data + i);
            }
            traits::deallocate(_allocator, _data, _capacity);
        }
        _data = nullptr;
        _size = 0;
        _capacity = 0;
	}

    Vector& operator = (const Vector& other) & {
        if(this == &other){return *this;}
        if(_data){
            for(size_type i = 0; i < _size; ++i){
                traits::destroy(_allocator, _data + i);
            }
			traits::deallocate(_allocator, _data, _capacity);
        }
		_size = other._size;
		_capacity = other._capacity;
		_allocator = other._allocator;
		_data = traits::allocate(_allocator, _capacity);
        size_type i = 0;
        try{
            for(; i < _size; ++i){
                traits::construct(_allocator, _data + i, other._data[i]);
            }
        }catch(...){
            for(size_type j = 0; j < i; ++j){
                traits::destroy(_allocator, _data + j);
            }
            traits::deallocate(_allocator, _data, _capacity);
            _capacity = 0;
            _size = 0;
            _data = nullptr;
			throw;
        }
		return *this;
    }
    
    Vector& operator = (Vector&& other) & {
		if(this == &other){return *this;}
        if(_data){
            for(size_type i = 0; i < _size; ++i){
                traits::destroy(_allocator, _data + i);
            }
			traits::deallocate(_allocator, _data, _capacity);
        }
		_size = other._size;
		_capacity = other._capacity;
		_allocator = std::move(other._allocator);
		_data = other._data;
		other._data = nullptr;
		other._capacity = 0;
		other._size = 0;
		return *this;
    }

	bool operator < (const Vector& other) const noexcept {
		for(size_type i = 0; i < std::min(_size, other._size); ++i){
			if (_data[i] < other._data[i]) return true;
        	if (other._data[i] < _data[i]) return false;
		}
		return _size < other._size;
	}

    bool operator > (const Vector& other) const noexcept 
	{return other < *this;}

    bool operator == (const Vector& other) const noexcept 
	{return !(other < *this) && !(other > *this);}

    bool operator != (const Vector& other) const noexcept 
	{return !(*this == other);}

    bool operator <= (const Vector& other) const noexcept 
	{return (*this < other) || (*this == other);}

    bool operator >= (const Vector& other) const noexcept 
	{return (*this > other) || (*this == other);}

	T& operator [](size_type index) 
	{return _data[index];}

    const T& operator [](size_type index) const 
	{return _data[index];}

	T& at(size_type index){
		if (index >= _size) {
			throw std::out_of_range("out_of_range");
		}
		return _data[index];
	}

    const T& at(size_type index) const {
		if (index >= _size) {
			throw std::out_of_range("out_of_range");
		}
		return _data[index];
	}

	T* data() noexcept 
	{return _data;}

	const T* data() const noexcept 
	{return _data;}

	T& front() 
	{return at(0);}

	const T& front() const 
	{return at(0);}

	T& back() 
	{return at(_size - 1);}

	const T& back() const 
	{return at(_size - 1);}

	iterator begin() noexcept 
	{return iterator(_data);}

	const_iterator begin() const noexcept 
	{return const_iterator(_data);}

	const_iterator cbegin() const noexcept 
	{return const_iterator(_data);}

	iterator end() noexcept 
	{return iterator(_data + _size);}

	const_iterator end() const noexcept 
	{return const_iterator(_data + _size);}

	const_iterator cend() const noexcept 
	{return const_iterator(_data + _size);}

	reverse_iterator rbegin() noexcept
	{return reverse_iterator(end());}

	const_reverse_iterator rbegin() const noexcept
	{return const_reverse_iterator(cend());}

	const_reverse_iterator crbegin() const noexcept
	{return const_reverse_iterator(cend());}

	reverse_iterator rend() noexcept
	{return reverse_iterator(begin());}

	const_reverse_iterator rend() const noexcept
	{return const_reverse_iterator(cbegin());}

	const_reverse_iterator crend() const noexcept
	{return const_reverse_iterator(cbegin());}

    bool empty() const noexcept 
	{return _size <= 0;}

	size_type size() const noexcept 
	{return _size;}

	size_type max_size() const noexcept 
	{return std::numeric_limits<size_type>::max() / sizeof(value_type);}

	size_type capacity() const noexcept 
	{return _capacity;}

	void reserve(size_type new_cap){
		if(new_cap <= _capacity){return;}
		if (new_cap > max_size()) {
			throw std::length_error("limit on the number of elements");
		}
		T* new_data = traits::allocate(_allocator, new_cap);
		if(_data){
			size_type i = 0;
			try{
				for (; i < _size; ++i){
					traits::construct(_allocator, new_data + i,
						std::move_if_noexcept(_data[i]));
				}
			}catch(...){
				for(size_type j = 0; j < i; ++j){
					traits::destroy(_allocator, new_data + j);
				}
				traits::deallocate(_allocator, new_data, new_cap);
				throw;
			}
			for(size_type n = 0; n < _size; ++n){
				traits::destroy(_allocator, _data + n);
			}
			traits::deallocate(_allocator, _data, _capacity);
		}
		_capacity = new_cap;
		_data = new_data;
	}

	void shrink_to_fit(){
		if(_size >= _capacity){return;}
		if(_size == 0){
			if(_data){
				traits::deallocate(_allocator, _data, _capacity);
				_data = nullptr;
            	_capacity = 0;
			}
			return;
		}
		T* new_data = traits::allocate(_allocator, _size);
		size_type i = 0;
		try{
			for (; i < _size; ++i){
				traits::construct(_allocator, new_data + i,
					std::move_if_noexcept(_data[i]));
			}
		}catch(...){
			for(size_type j = 0; j < i; ++j){
				traits::destroy(_allocator, new_data + j);
			}
			traits::deallocate(_allocator, new_data, _size);
			throw;
		}
		for(size_type n = 0; n < _size; ++n){
			traits::destroy(_allocator, _data + n);
		}
		traits::deallocate(_allocator, _data, _capacity);
		_capacity = _size;
		_data = new_data;
	}
    
	void clear() noexcept {
		for(size_type i = 0; i < _size; ++i){
			traits::destroy(_allocator, _data + i);
		}
		_size = 0;
	}

	template<typename... Args>
	iterator emplace(const_iterator pos, Args&&... args){
		if(cbegin() > pos || cend() < pos){
			throw std::out_of_range("out_of_range");
		}
		size_type index = pos - cbegin();
		if (_size >= _capacity){
			reserve(_capacity == 0 ? 1 : _capacity * 2);
		}
		if(index < _size){
			traits::construct(_allocator, _data + _size, 
				std::forward<Args>(args)...);
			try{
				for(size_type i = _size; i > index; --i){
					std::swap(_data[i], _data[i - 1]);
				}
			}catch(...){
				traits::destroy(_allocator, _data + _size);
				throw;
			}
		}else{
			traits::construct(_allocator, _data + _size, 
				std::forward<Args>(args)...);
		}
		++_size;
		return begin() + index;
	}

	iterator insert(const_iterator pos, const T& value)
	{return emplace(pos, value);}

	iterator insert(const_iterator pos, T&& value)
	{return emplace(pos, std::move(value));}

	iterator insert(const_iterator pos, size_type count, const T& value){
		size_type index = pos - cbegin();
		if(_size + count > _capacity){reserve(_size + count);}
		auto iter = std::views::repeat(value, count);
		return _insert_by_iterator(const_iterator(_data + index), 
			iter.begin(), iter.end());
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
		return _insert_by_iterator(pos, first, last);
	}

	iterator insert(const_iterator pos, std::initializer_list<T> init){
		size_type index = pos - begin();
		if(init.size() + _size > _capacity){reserve(init.size() + _size);}
		return _insert_by_iterator(const_iterator(_data + index),
			init.begin(), init.end());
	}

	template<typename... Args>
    void emplace_back(Args&&... args)
	{emplace(cend(), std::forward<Args>(args)...);}

	void push_back(const T& value)
	{emplace_back(value);}

	void push_back(T&& value)
	{emplace_back(std::move(value));}

	iterator erase(const_iterator pos){
		return erase(pos, pos + 1);
	}

	iterator erase(const_iterator first, const_iterator last){
		if(cbegin() > first || cend() < first || 
			cbegin() > last || cend() < last){
				throw std::out_of_range("out_of_range");
		}
		size_type index = first - cbegin();
		size_type count_alements_del = last - first;
		for(size_type i = index + count_alements_del; i < _size; ++i){
			std::swap(_data[i - count_alements_del], _data[i]);
		}
		for(size_type i = 0; i < count_alements_del; ++i){
			pop_back();
		}
		return begin() + index;
	}

	void pop_back(){
		--_size;
		traits::destroy(_allocator, _data + _size);
	}

	void resize(size_type count)
	{_resize(count);}

	void resize(size_type count, const T& value)
	{_resize(count, value);}

	void swap(Vector& other) noexcept {
		std::swap(_data, other._data);
		std::swap(_capacity, other._capacity);
		std::swap(_size, other._size);
		std::swap(_allocator, other._allocator);
	}

    allocator_type& get_allocator() noexcept 
	{return _allocator;}

	const allocator_type& get_allocator() const noexcept 
	{return _allocator;}

private:
	pointer _data;
	size_type _capacity;
	size_type _size;
	Allocator _allocator;

	template<typename... Args>
	void _resize(size_type count, Args&&... args){
		if(count <= _size){
			for(size_type i = _size; i > count; --i){
				traits::destroy(_allocator, _data + i - 1);
			}
			_size = count;
			return;
		}
		if(count > _capacity){reserve(count);}
		size_type old_size = _size;
		for(size_type i = 0; i < count - old_size; ++i){
			emplace_back(std::forward<Args>(args)...);
		}
	}

	template<typename InputIterator, 
		typename = std::enable_if_t<!std::is_integral_v<InputIterator>>,
		typename = decltype(
			std::declval<InputIterator&>() != std::declval<InputIterator&>(),
			*std::declval<InputIterator&>(),
			++std::declval<InputIterator&>()
		)>
	iterator _insert_by_iterator(const_iterator pos, 
		InputIterator first, InputIterator last){
			if(cbegin() > pos || cend() < pos){
				throw std::out_of_range("out_of_range");
			}
			size_type index = pos - cbegin();
			size_type counter = 0;
			try{
				for (auto it = first; it != last; ++it, ++counter){
					push_back(std::move_if_noexcept(*it));
				}
				for(size_type n = 0; n < counter; ++n){
					for(size_type m = _size - counter + n; m > index + n; --m){
						std::swap(_data[m], _data[m - 1]);
					}
				}
			}catch(...){
				for(size_type j = 0; j < counter; ++j){
					pop_back();
				}
				throw;
			}
			return begin() + index;
		}

	template <bool IsConst>
    class _BaseIterator {
    public:
        template <bool IsConst2>
        friend class _BaseIterator;

        using value_type = typename Vector::value_type;
        using size_type = typename Vector::size_type;
        using difference_type = typename Vector::difference_type;
        using pointer = std::conditional_t<IsConst, 
			typename Vector::const_pointer, typename Vector::pointer>;
        using reference = std::conditional_t<IsConst, 
			typename Vector::const_reference, typename Vector::reference>;
        using iterator_category = std::contiguous_iterator_tag;

        _BaseIterator() : ptr(nullptr) {}

        _BaseIterator(pointer ptr) : ptr(ptr) {}

        template <bool IsConst2>
        _BaseIterator(const _BaseIterator<IsConst2>& other) requires 
			(!IsConst2 || IsConst) : ptr(other.ptr) {}

        reference operator*() const {return *ptr;}

        pointer operator->() const {return ptr;}

        reference operator[] (size_type i) const {return ptr[i];}

        template <bool IsConst2>
        bool operator == (const _BaseIterator<IsConst2>& right) const 
		{return ptr == right.ptr;}

        template <bool IsConst2>
        bool operator != (const _BaseIterator<IsConst2>& right) const 
		{return ptr != right.ptr;}

        template <bool IsConst2>
        bool operator < (const _BaseIterator<IsConst2>& right) const 
		{return ptr < right.ptr;}

        template <bool IsConst2>
        bool operator <= (const _BaseIterator<IsConst2>& right) const 
		{return ptr <= right.ptr;}

        template <bool IsConst2>
        bool operator > (const _BaseIterator<IsConst2>& right) const 
		{return ptr > right.ptr;}

        template <bool IsConst2>
        bool operator >= (const _BaseIterator<IsConst2>& right) const 
		{return ptr >= right.ptr;}

        template <bool IsConst2>
        difference_type operator - (const _BaseIterator<IsConst2>& right) const 
		{return ptr - right.ptr;}

        _BaseIterator& operator ++ () 
		{++ptr; return *this;}

        _BaseIterator operator ++ (int) 
		{_BaseIterator iter(*this); ++ptr; return iter;}

        _BaseIterator& operator -- () 
		{--ptr; return *this;}

        _BaseIterator operator -- (int) 
		{_BaseIterator iter(*this); --ptr; return iter;}

        _BaseIterator& operator -= (difference_type n) 
		{ptr -= n; return *this;}

        _BaseIterator& operator += (difference_type n) 
		{ptr += n; return *this;}

        _BaseIterator operator + (difference_type n) const 
		{_BaseIterator iter(*this); iter += n; return iter;}

        _BaseIterator operator - (difference_type n) const 
		{_BaseIterator iter(*this); iter -= n; return iter;}
    
    private:
        pointer ptr;
    };

};


template<typename T, typename Allocator>
std::ostream& operator<<(std::ostream& os, const Vector<T, Allocator>& vec){
	for (std::size_t i = 0; i < vec.size(); ++i){
		os << vec[i] << " ";
	}
	return os;
}


struct Test{
public:
	void operator() (){
		std::cout << std::endl;
		test_construct();
		std::cout << std::endl;
		test_operator_comparison();
		std::cout << std::endl;
		test_index();
		std::cout << std::endl;
		test_iterator();
		std::cout << std::endl;
		test_reallocate_memory();
		std::cout << std::endl;
		test_additions_elements();
		std::cout << std::endl;
		test_deletions_elements();
		std::cout << std::endl;
	}
private:

	void test_construct(){
		std::cout << "----------Конструкторы----------" << std::endl;

		std::cout << "Конструктор по умолчанию" << std::endl;
		{
			Vector<int> v;
			assert(v.size() == 0);
			assert(v.capacity() == 0);
			assert(v.empty());
			assert((std::is_same_v<
				decltype(v.get_allocator()),
				std::allocator<int>&
			>));
		}

		std::cout << "Конструктор с аллокатором" << std::endl;
		{
			DefaultAllocator<int> alloc;
			Vector<int, DefaultAllocator<int>> v(alloc);
			assert(v.size() == 0);
			assert(v.capacity() == 0);
			assert(v.empty());
			assert((std::is_same_v<
				decltype(v.get_allocator()),
				DefaultAllocator<int>&
			>));
		}

		std::cout << "Конструктор c количеством элементов "
			"со значением по умолчанию и аллокатором" << std::endl;
		{
			DefaultAllocator<int> alloc;
			Vector<int, DefaultAllocator<int>> v(10, alloc);
			assert(v.size() == 10);
			assert(v.capacity() == 10);
			assert(!(v.empty()));
			assert((std::is_same_v<
				decltype(v.get_allocator()),
				DefaultAllocator<int>&
			>));
			assert(v[0] == 0);
			assert(v[5] == 0);
			assert(v[9] == 0);
		}

		std::cout << "Конструктор c количеством элементов "
			"с указанным значением и аллокатором" << std::endl;
		{
			DefaultAllocator<int> alloc;
			Vector<int, DefaultAllocator<int>> v(15, 99, alloc);
			assert(v.size() == 15);
			assert(v.capacity() == 15);
			assert(!(v.empty()));
			assert((std::is_same_v<
				decltype(v.get_allocator()),
				DefaultAllocator<int>&
			>));
			assert(v[0] == 99);
			assert(v[7] == 99);
			assert(v[14] == 99);
		}

		std::cout << "Конструктор от двух итераторов "
			"и аллокатором" << std::endl;
		{
			Vector<int> iter {1,2,3,4,5,6,7};
			DefaultAllocator<int> alloc;
			Vector<int, DefaultAllocator<int>> v(
				iter.begin() + 1, iter.end() - 1, alloc);
			assert(v.size() == 5);
			assert(v.capacity() == 8);
			assert(!(v.empty()));
			assert((std::is_same_v<
				decltype(v.get_allocator()),
				DefaultAllocator<int>&
			>));
			assert(v[0] == 2);
			assert(v[1] == 3);
			assert(v[2] == 4);
			assert(v[3] == 5);
			assert(v[4] == 6);
		}

		std::cout << "Конструктор со списком инициализации "
			"и аллокатором" << std::endl;
		{
			DefaultAllocator<int> alloc;
			Vector<int, DefaultAllocator<int>> v({1,2,3}, alloc);
			assert(v.size() == 3);
			assert(v.capacity() == 3);
			assert(!(v.empty()));
			assert((std::is_same_v<
				decltype(v.get_allocator()),
				DefaultAllocator<int>&
			>));
			assert(v[0] == 1);
			assert(v[1] == 2);
			assert(v[2] == 3);
		}

		std::cout << "Конструктор копирования" << std::endl;
		{
			Vector<int> v1 {1,2,3,4,5,6,7};
			Vector<int> v(v1);
			assert(v.size() == 7);
			assert(v.capacity() == 7);
			assert(!(v.empty()));
			assert((std::is_same_v<
				decltype(v.get_allocator()),
				std::allocator<int>&
			>));
			assert(v1[0] == 1);
			assert(v1[3] == 4);
			assert(v1[6] == 7);
			assert(v[0] == 1);
			assert(v[3] == 4);
			assert(v[6] == 7);
		}

		std::cout << "Конструктор копирования с аллокатором" << std::endl;
		{
			Vector<int, DefaultAllocator<int>> v1 {1,2,3,4,5,6,7};
			DefaultAllocator<int> alloc;
			Vector<int, DefaultAllocator<int>> v(v1, alloc);
			assert(v.size() == 7);
			assert(v.capacity() == 7);
			assert(!(v.empty()));
			assert((std::is_same_v<
				decltype(v.get_allocator()),
				DefaultAllocator<int>&
			>));
			assert(v1[0] == 1);
			assert(v1[3] == 4);
			assert(v1[6] == 7);
			assert(v[0] == 1);
			assert(v[3] == 4);
			assert(v[6] == 7);
		}

		std::cout << "Конструктор перемещения" << std::endl;
		{
			Vector<int> v1 {1,2,3,4,5,6,7};
			Vector<int> v(std::move(v1));
			assert(v.size() == 7);
			assert(v.capacity() == 7);
			assert(!(v.empty()));
			assert((std::is_same_v<
				decltype(v.get_allocator()),
				std::allocator<int>&
			>));
			assert(v1.empty());
			assert(v1.size() == 0);
			assert(v1.capacity() == 0);
			assert(v1.data() == nullptr);
			assert(v[0] == 1);
			assert(v[3] == 4);
			assert(v[6] == 7);
		}

		std::cout << "Конструктор перемещения с аллокатором" << std::endl;
		{
			Vector<int, DefaultAllocator<int>> v1 {1,2,3,4,5,6,7};
			DefaultAllocator<int> alloc;
			Vector<int, DefaultAllocator<int>> v(std::move(v1), alloc);
			assert(v.size() == 7);
			assert(v.capacity() == 7);
			assert(!(v.empty()));
			assert((std::is_same_v<
				decltype(v.get_allocator()),
				DefaultAllocator<int>&
			>));
			assert(v1.empty());
			assert(v1.size() == 0);
			assert(v1.capacity() == 0);
			assert(v1.data() == nullptr);
			assert(v[0] == 1);
			assert(v[3] == 4);
			assert(v[6] == 7);
		}
	}

	void test_operator_comparison(){
		std::cout << "----------Операторы сравнения----------" << std::endl;
		
		Vector<int> v1{1,2,3,4};
		Vector<int> v2{1,2,3};
		Vector<int> v3{10,20,30};

		{
			std::cout << "Оператор равно" << std::endl;
			assert(v3 == v3);
			assert(!(v3 == v2));

			std::cout << "Оператор не равно" << std::endl;
			assert(v3 != v2);
			assert(!(v3 != v3));

			std::cout << "Оператор больше" << std::endl;
			assert(v1 > v2);
			assert(v3 > v2);

			std::cout << "Оператор меньше" << std::endl;
			assert(v2 < v1);
			assert(v1 < v3);

			std::cout << "Оператор больше или равно" << std::endl;
			assert(v3 >= v3);
			assert(v1 >= v2);

			std::cout << "Оператор меньше или равно" << std::endl;
			assert(v2 <= v2);
			assert(v1 <= v3);
		}
	}

	void test_index(){
		std::cout << "----------Доступ по индексу----------" << std::endl;

		Vector<int> v{1,2,3,4,5,6,7,8};

		{
			std::cout << "Доступ без проверки" << std::endl;
			assert(v[0] == 1);
			assert(v[5] == 6);

			std::cout << "Доступ с проверкой" << std::endl;
			assert(v.at(0) == 1);
			assert(v.at(5) == 6);
			bool var = false;
			try{v.at(10);}catch(...){var = true;}
			assert(var);
		}
	}

	void test_iterator(){
		std::cout << "----------Итераторы----------" << std::endl;

		Vector<int> v{1,2,3,4,5,6,7,8};

		{
			std::cout << "Обычные" << std::endl;
			auto it = v.begin();
			assert((std::is_same_v<
				decltype(it),
				Vector<int>::iterator
			>));
			assert(*it == 1);
			assert(*(++it) == 2);
			assert(*(it+=2) == 4);
		}

		{
			std::cout << "Обычные константные" << std::endl;
			auto it = v.cbegin();
			assert((std::is_same_v<
				decltype(it),
				Vector<int>::const_iterator
			>));
			assert(*it == 1);
			assert(*(++it) == 2);
			assert(*(it+=2) == 4);
		}

		{
			std::cout << "Обратные" << std::endl;
			auto it = v.rbegin();
			assert((std::is_same_v<
				decltype(it),
				Vector<int>::reverse_iterator
			>));
			assert(*it == 8);
			assert(*(++it) == 7);
			assert(*(it+=2) == 5);
		}

		{
			std::cout << "Обратные константные" << std::endl;
			auto it = v.crbegin();
			assert((std::is_same_v<
				decltype(it),
				Vector<int>::const_reverse_iterator
			>));
			assert(*it == 8);
			assert(*(++it) == 7);
			assert(*(it+=2) == 5);
		}
	}

	void test_reallocate_memory(){
		std::cout << "----------Реаллокация памяти----------" << std::endl;

		Vector<int> v1{1,2,3,4,5};
		Vector<int> v{v1.begin(), v1.end()};
		assert(v.size() == 5);
		assert(v.capacity() == 8);

		{
			std::cout << "Увеличение ёмкости" << std::endl;
			v.reserve(20);
			assert(v.size() == 5);
			assert(v.capacity() == 20);
		}

		{
			std::cout << "Уменьшение ёмкости до фактического "
				"размера" << std::endl;
			v.shrink_to_fit();
			assert(v.size() == 5);
			assert(v.capacity() == 5);
		}

		{
			std::cout << "Уменьшение размера" << std::endl;
			v.resize(3);
			assert(v.size() == 3);
			assert(v.capacity() == 5);
			assert(v[0] == 1);
			assert(v[2] == 3);
		}

		{
			std::cout << "Увеличение размера" << std::endl;
			v.resize(10, 99);
			assert(v.size() == 10);
			assert(v.capacity() == 10);
			assert(v[0] == 1);
			assert(v[2] == 3);
			assert(v[3] == 99);
			assert(v[6] == 99);
			assert(v[9] == 99);
		}
	}

	void test_additions_elements(){
		std::cout << "----------Добавление элементов----------" << std::endl;

		{
			std::cout << "Добавление элементов по итератору "
				"cо списком инициализации" << std::endl;
			Vector<int> v{1,2,3,4,5,6,7,8,9,10};
			auto it = v.insert(v.begin() + 2, {1000,2000,3000});
			assert(*it == 1000);
			assert(v[0] == 1);
			assert(v[1] == 2);
			assert(v[2] == 1000);
			assert(v[4] == 3000);
			assert(v[5] == 3);
			assert(v.size() == 13);
			assert(v.capacity() == 13);
		}

		{
			std::cout << "Добавление элемента по итератору" << std::endl;
			Vector<int> v{1,2,3,4,5,6,7,8,9,10};
			auto it = v.insert(v.begin() + 2, 999);
			assert(*it == 999);
			assert(v[0] == 1);
			assert(v[1] == 2);
			assert(v[2] == 999);
			assert(v[3] == 3);
			assert(v.size() == 11);
			assert(v.capacity() == 20);
			v.emplace(v.end() - 1, 9999);
			assert(v[10] == 9999);
			assert(v.size() == 12);
			assert(v.capacity() == 20);
		}

		{
			std::cout << "Добавление нескольких элементов по итератору" << std::endl;
			Vector<int> v{1,2,3,4,5,6,7,8,9,10};
			auto it = v.insert(v.begin(), 3, 999);
			assert(*it == 999);
			assert(v[0] == 999);
			assert(v[2] == 999);
			assert(v[3] == 1);
			assert(v[12] == 10);
			assert(v.size() == 13);
			assert(v.capacity() == 13);
		}

		{
			std::cout << "Добавление диапазона элементов "
				"по итератору" << std::endl;
			Vector<int> v1{100,200,300,400,500,600,700,800,900,1000};
			Vector<int> v{1,2,3,4,5,6,7,8,9,10};
			auto it = v.insert(v.begin() + 1, v1.begin(), v1.begin() + 5);
			assert(*it == 100);
			assert(v[0] == 1);
			assert(v[1] == 100);
			assert(v[5] == 500);
			assert(v[6] == 2);
			assert(v[9] == 5);
			assert(v[14] == 10);
			assert(v.size() == 15);
			assert(v.capacity() == 20);
		}

		{
			std::cout << "Добавление элемента в конец" << std::endl;
			Vector<int> v{1,2,3,4,5};
			v.emplace_back(100);
			assert(v.size() == 6);
			assert(v.capacity() == 10);
			assert(v.back() == 100);
			v.push_back(1000);
			assert(v.size() == 7);
			assert(v.capacity() == 10);
			assert(v.back() == 1000);
		}
	}

	void test_deletions_elements(){
		std::cout << "----------Удаление элементов----------" << std::endl;
		
		{
			std::cout << "Удаление элемента по итератору" << std::endl;
			Vector<int> v{1,2,3,4,5,6,7,8};
			v.erase(v.begin());
			assert(v[0] == 2);
			assert(v.size() == 7);
			assert(v.capacity() == 8);
		}

		{
			std::cout << "Удаление диапазона элементов "
				"по итератору" << std::endl;
			Vector<int> v{1,2,3,4,5,6,7,8};
			v.erase(v.begin(), v.end());
			assert(v.size() == 0);
			assert(v.capacity() == 8);
		}

		{
			std::cout << "Удаление элемента с конца" << std::endl;
			Vector<int> v{1,2,3,4,5,6,7,8};
			v.pop_back();
			assert(v.back() == 7);
			assert(v.size() == 7);
			assert(v.capacity() == 8);
		}
	}
} test;


int main(int argc, char const *argv[])
{	
	using namespace std;
	test();
	return 0;
}
