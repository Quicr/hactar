#include <type_traits>
#include <utility>

#pragma once

template <typename T>
class Vector
{
public:
    Vector(const unsigned int max_size=__UINT32_MAX__)
        : _max_size(max_size), _capacity(0), _size(0), array(new T[_capacity])
    {
    }
    Vector(const Vector<T>& v)
    {
        _max_size = v.max_size();
        _capacity = v.capacity();
        _size = v.size();
        array = new T[_capacity];
        for (unsigned int i = 0; i < _capacity; i++)
            array[i] = v[i];
    }
    Vector(Vector<T>&& v)
    {
        _max_size = v.max_size();
        _capacity = v.capacity();
        _size = v.size();
        array = v.array;
        v.array = nullptr;
    }

    virtual ~Vector()
    {
        delete [] array;
    }

    Vector<T>& operator=(const Vector<T>& v)
    {
        delete [] array;
        _max_size = v.max_size();
        _capacity = v.capacity();
        _size = v.size();
        array = new T[_capacity];
        for (unsigned int i = 0; i < _capacity; i++)
            array[i] = v[i];
        return *this;
    }

    Vector<T>& operator=(const Vector<T>&& v)
    {
        delete [] array;
        _max_size = v.max_size();
        _capacity = v.capacity();
        _size = v.size();
        array = v.array;
        v.array = nullptr;
        return *this;
    }

    bool operator==(const Vector<T> &other)
    {
        if (size() != other.size()) return false;

        for (unsigned int i = 0; i < _size; i++)
        {
            if (array[i] != other[i]) return false;
        }
        return true;
    }

    bool operator!=(const Vector<T> &other)
    {
        return !(*this == other);
    }

    bool push_back(const T &data)
    {
        if (_size >= _max_size) return false;

        perform_push_back_resize();

        array[_size++] = data;

        return true;
    }

    bool push_back(T &&data)
    {
        if (_size >= _max_size) return false;

        perform_push_back_resize();

        array[_size++] = std::move(data);

        return true;
    }

    void pop_back()
    {
        erase(_size--);
    }

    void erase(unsigned int idx)
    {
        if (idx > _size) return;

        // When the _size equals 1/4 of _capacity reduce _capacity by 1/2
        // Overwrite the whole array
        if (_size-1 <= _capacity/4)
        {
            unsigned int new_capacity = _capacity/2;
            T* new_array = new T[new_capacity];

            delete_resize_move_or_copy(new_array, idx);

            delete [] array;
            array = new_array;
            _capacity = new_capacity;
        }
        else
        {
            // Overwrite elements starting at the deleted idx
            delete_move_or_copy(idx);
        }

        _size--;
    }

    void resize(const unsigned int new_capacity)
    {
        T* new_array = new T[new_capacity];

        unsigned int max_cap = _capacity;
        if (new_capacity < _capacity)
            max_cap = new_capacity;

        move_or_copy(new_array, max_cap);

        delete [] array;
        array = new_array;
        _capacity = new_capacity;

        if (_size > _capacity)
        {
            _size = _capacity;
        }
    }

    T& operator[] (const unsigned int idx) const
    {
        return array[idx];
    }

    T& at(const unsigned int idx) const
    {
        return array[idx];
    }

    T front() const
    {
        return array[0];
    }

    T back() const
    {
        return array[_size - 1];
    }

    T* data() const
    {
        return array;
    }

    void clear()
    {
        if (_size == 0) return;
        delete [] array;
        _capacity = 0;
        _size = 0;
        array = new T[_capacity];
    }

    bool empty() const
    {
        return !_size;
    }

    unsigned int size() const
    {
        return _size;
    }

    unsigned int max_size() const
    {
        return _max_size;
    }

    unsigned int capacity() const
    {
        return _capacity;
    }

protected:
    inline void perform_push_back_resize()
    {
        if (_size == _capacity)
        {
            unsigned int new_capacity = _capacity == 0 ? 1 : _capacity << 1;

            // create a new pointer of double _capacity and copy
            T* new_array = new T[new_capacity];

            // Copies or Moves the previous elements to the new array
            move_or_copy(new_array, _capacity);

            delete [] array;
            array = new_array;
            _capacity = new_capacity;
        }
    }

    // If T is not a fundamental type then this function will be enabled
    template <bool B = std::is_fundamental<T>::value>
    inline typename std::enable_if<!B, void>::type
    move_or_copy(T* new_array, const unsigned int size)
    {
        // Move operation
        for (unsigned int i = 0; i < size; ++i)
        {
            new_array[i] = std::move(array[i]);
        }
    }

    // If T is a fundamental type then this function will be active
    template <bool B = std::is_fundamental<T>::value>
    inline typename std::enable_if<B, void>::type
    move_or_copy(T* new_array, const unsigned int size)
    {
        // Copy operation
        for (unsigned int i = 0; i < size; ++i)
        {
            new_array[i] = array[i];
        }
    }

    // If T is not a fundamental type then this function will be enabled
    template <bool B = std::is_fundamental<T>::value>
    inline typename std::enable_if<!B, void>::type
    delete_resize_move_or_copy(T* new_array, const unsigned int skip_idx)
    {
        // Move operation
        unsigned int i = 0; // Old array idx
        unsigned int j = 0; // New array idx

        // [n-1] writes, since we skip the erased element.
        while (i < _size)
        {
            if (i == skip_idx)
            {
                ++i;
                continue;
            }

            new_array[j] = std::move(array[i]);

            ++i;
            ++j;
        }
    }

    // If T is a fundamental type then this function will be enabled
    template <bool B = std::is_fundamental<T>::value>
    inline typename std::enable_if<B, void>::type
    delete_resize_move_or_copy(T* new_array, const unsigned int skip_idx)
    {

        // Copy operation
        unsigned int i = 0; // Old array idx
        unsigned int j = 0; // New array idx

        // [n-1] writes, since we skip the erased element.
        while (i < _size)
        {
            if (i == skip_idx)
            {
                ++i;
                continue;
            }

            new_array[j] = array[i];

            ++i;
            ++j;
        }
    }

    // If T is not a fundamental type then this function will be enabled
    template <bool B = std::is_fundamental<T>::value>
    inline typename std::enable_if<!B, void>::type
    delete_move_or_copy(unsigned int deleted_idx)
    {
        // Move operation
        for (unsigned int i = deleted_idx; i < _size; ++i)
        {
            array[i] = std::move(array[i+1]);
        }
    }

    // If T is a fundamental type then this function will be enabled
    template <bool B = std::is_fundamental<T>::value>
    inline typename std::enable_if<B, void>::type
    delete_move_or_copy(unsigned int deleted_idx)
    {
        // Copy operation
        for (unsigned int i = deleted_idx; i < _size; ++i)
        {
            array[i] = array[i+1];
        }
    }

    unsigned int _max_size;
    unsigned int _capacity;
    unsigned int _size;
    T *array;
};