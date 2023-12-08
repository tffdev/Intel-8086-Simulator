#pragma once

template <typename T>
class List {
public:
    List() : data(nullptr), size(0), capacity(0) {}

    ~List() {
        if (data) {
            delete[] data;
        }
    }

    void Add(const T& value) {
        if (size >= capacity) {
            Expand();
        }
        data[size] = value;
        size++;
    }

    T& operator[](int index) {
        return data[index];
    }

    const T& operator[](int index) const {
        return data[index];
    }

    int Size() const {
        return size;
    }

    // copy constructor
    List(const List<T>& other) : data(nullptr), size(0), capacity(0) {
        if (data) delete[] data;
        data = new T[other.capacity];
        size = other.size;
        capacity = other.capacity;

        for (int i = 0; i < size; ++i) {
            data[i] = other.data[i];
        }
	}

    // = operator just do a shallow copy
    List<T> operator=(const List<T>& other) {
        if (this == &other) {
			return *this;
		}

		delete[] data;
		data = new T[other.capacity];
		size = other.size;
		capacity = other.capacity;

		for (int i = 0; i < size; ++i) {
			data[i] = other.data[i];
		}

		return *this;
	}

private:
    T* data;
    int size;
    int capacity;

    void Expand() {
        int newCapacity = (capacity == 0) ? 1 : capacity * 2;
        T* newData = new T[newCapacity];

        for (int i = 0; i < size; ++i) {
            newData[i] = data[i];
        }

        delete[] data;
        data = newData;
        capacity = newCapacity;
    }
};
