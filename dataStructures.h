#pragma once
#include <stdexcept>

template<typename T>
struct myVector {

private:
    size_t arr_size;
    size_t arr_capacity;
    T* data;
public:

    myVector() {
        arr_size = 0;
        arr_capacity = 1;
        data = new T[1];
    }

    myVector(int Capacity, T default_value) {
        arr_size = Capacity;
        arr_capacity = Capacity;
        data = new T[arr_capacity];

        for (int i = 0; i < arr_capacity; i++) {
            data[i] = default_value;
        }
    }


    // Copy constructor.
    myVector(const myVector& other) {
        arr_size = other.arr_size;
        arr_capacity = other.arr_capacity;
        data = new T[arr_capacity];
        for (int i = 0; i < arr_size; i++) {
            data[i] = other.data[i];
        }
    }

    // Copy assignment operator.
    myVector& operator=(const myVector& other) {
        if (this != &other) {
            arr_size = other.arr_size;
            arr_capacity = other.arr_capacity;

            delete[] data;
            data = new T[arr_capacity];

            for (int i = 0; i < arr_size; i++) {
                data[i] = other.data[i];
            }
        }
        return *this;
    }


    void push_back(T element) {
        if (arr_size == arr_capacity) {
            arr_capacity *= 2;
            T* new_data = new T[arr_capacity];
            for (size_t i = 0; i < arr_size; i++) {
                new_data[i] = data[i];
            }

            delete[] data;

            data = new_data;
        }

        data[arr_size] = element;

        arr_size++;

    }

    void resize(int newCapacity) {
        T* new_data = new T[newCapacity];

        for (int i = 0; i < arr_size; i++) {
            new_data[i] = data[i];
        }

        delete[] data;

        data = new_data;

        arr_capacity = newCapacity;
        arr_size = newCapacity;
    }

    void pop_back() {
        if (arr_size > 0) {
            arr_size--;
        }
        else {
            throw std::runtime_error("Vector is empty. cannot pop back from vector.");
        }
    }

    size_t size() {
        return arr_size;
    }

    size_t capacity() {
        return arr_capacity;
    }

    void clear() {
        arr_size = 0;
    }

    bool empty() {
        return arr_size == 0;
    }


    ~myVector() {
        delete[] data;
        data = nullptr;
    }

    T& operator[](int index) {
        if (index >= 0 && index < arr_size) {
            return data[index];
        }
        else {
            throw std::out_of_range("Index out of range.");
        }
    }
};

template<typename T1, typename T2>
struct myPair {
    T1 first;
    T2 second;

    myPair() {}

    myPair(T1 F, T2 S) {
        first = F;
        second = S;
    }

    bool operator==(const myPair& other) {
        if (first == other.first && second == other.second) return true;
        else return false;
    }

};