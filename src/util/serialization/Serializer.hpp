#pragma once




#include <type_traits>
#include <tuple>
#include <cstring>
#include <unordered_map>
#include <vector>
#include <unordered_set>
#include <string>
#include <stdexcept>


/**
 * This macro allows the byte serializer to know what fields a struct has, so it can
 * iterate over them and serialize/deserialize them.
 */
#define DECLARE_FIELDS(...) \
    static constexpr auto fields() { return std::make_tuple(__VA_ARGS__); }


// Helper to get the type of a field
template <typename T, typename F>
struct field_type;

template <typename T, typename R>
struct field_type<T, R T::*> {
    using type = R;
};


// stuff to check if a type is a vector during runtime
template <typename T, typename E = void>
struct is_vector : std::false_type {};

template <typename E>
struct is_vector<std::vector<E>> : std::true_type {};


// stuff to check if a type is an array during runtime
template <typename T>
struct is_std_array : std::false_type {};

template <typename T, std::size_t N>
struct is_std_array<std::array<T, N>> : std::true_type {};


// stuff to check if a type is a set during runtime
template <typename T, typename E = void>
struct is_set : std::false_type {};

template <typename E>
struct is_set<std::unordered_set<E>> : std::true_type {};


// stuff to check if a type is a map during runtime
template <typename T>
struct is_map : std::false_type {};

template <typename K, typename V>
struct is_map<std::unordered_map<K, V>> : std::true_type {};


// stuff to check if a type is a string during runtime
template <typename T>
struct is_string : std::false_type {};

template <>
struct is_string<std::string> : std::true_type {};


// stuff to check if a type is a struct or class during runtime
template <typename T>
struct is_struct {
    static constexpr bool value = std::is_class<T>::value && !is_vector<T>::value && !is_std_array<T>::value && !is_set<T>::value && !is_map<T>::value && !is_string<T>::value;
};


// stuff to check if type is a primitive value during runtime
template <typename T>
struct is_primitive {
    static constexpr bool value = !is_vector<T>::value && !is_std_array<T>::value && !is_set<T>::value && !is_struct<T>::value && !is_map<T>::value && !is_string<T>::value;
};


// stuff to iterate over a tuple
#include <utility> // for std::index_sequence and std::make_index_sequence
template <typename Tuple, typename Func, std::size_t... Is>
void for_each_impl(Tuple&& tuple, Func&& func, std::index_sequence<Is...>) {
    (func(std::get<Is>(tuple)), ...);
}

template <typename Tuple, typename Func>
void for_each(Tuple&& tuple, Func&& func) {
    constexpr auto size = std::tuple_size<std::decay_t<Tuple>>::value;
    for_each_impl(std::forward<Tuple>(tuple), std::forward<Func>(func), std::make_index_sequence<size>{});
}
//




using namespace std;


/**
 * Serializer class
 * 
 * This class is used to serialize and deserialize objects into a byte array. It has three 
 * main functions:
 * - serialize
 * - deserialize
 * - size_of
 * 
 * Which are implemented for the following types:
 * - struct/class
 * - vector
 * - array
 * - unordered_set
 * - unordered_map
 * - string
 * - primitive types (int, float, uint8_t, char, etc)
 * 
 * 
 * WARNING: primitive arrays like 'char c[15]' don't work becuase the serializer can't determine the size of the array at runtime
 * 
 * 
 * You would use it like this:
 * 
```cpp
    struct test {
        uint8_t a;
        unordered_set<string> b;

        DECLARE_FIELDS(&test::a, &test::b)
    };

    int main() {

        Serializer s;

        test t = {1, {"hello"}};

        int size = s.size_of(t);
        char* buffer = new char[size];
        s.serialize(buffer, t);

        test t2;
        s.deserialize(buffer, t2);

    }
```
 * 
 * Notice how the struct test has a DECLARE_FIELDS macro that tells the serializer what fields it should serialize. 
 * 
 * 
 * The serializer can handle nested structs, vectors, unordered_sets, unordered_maps, and strings. It can also handle
 * these data structures nested within each other, as well as any variable size of these data structures.
 * 
 * WARNING: it's important to note that the serializer only supports vectors, arrays, unordered_sets, and unordered_maps up to size 65535
 * This is because when serializing a data structure, it first writes the size of the data structure as a uint16_t or 2 bytes. 
 * 
 * A good practice for primitive types is to use types with platform independent byte sizes, such as int32_t, uint32_t, int64_t, uint64_t, etc.
 * This way you can also specify the number of bytes you want to serialize the value too explicity.
 * 
 * 
 * 
 * 
 * ######### FORMATS ##########
 * 
 * Primitive types:
 * {sizeof(T) bytes}
 * 
 * string:
 * {size of string, 2 bytes}{string, n bytes}
 * 
 * vector:
 * {size of vector, 2 bytes}{elements, n bytes}
 * 
 * array:
 * {size of array, 2 bytes}{elements, n bytes}
 * 
 * unordered_set:
 * {size of unordered_set, 2 bytes}{elements, n bytes}
 * 
 * unordered_map:
 * {size of unordered_map, 2 bytes}{elements, n bytes}
 * 
 * struct:
 * {fields, n bytes}
 * 
 * 
 */
struct Serializer {


    // SIZE FUNCTIONS

    /**
     * size function if T is a primitive
     * 
     * Gets the size of the object for copying into a message.
     */
    template <typename T>
    static std::enable_if_t<is_primitive<T>::value, size_t>
    size_of(T value) {
        return sizeof(value);
    }

    /**
     * size function if T is a string
     * 
     * Gets the size of the object for copying into a message.
     */
    template <typename T>
    static std::enable_if_t<is_string<T>::value, size_t>
    size_of(T value) {
        return sizeof(uint16_t) + value.size();
    }

    /**
     * size function if T is a vector
     * 
     * Gets the size of the object for copying into a message.
     */
    template <typename T>
    static std::enable_if_t<is_vector<T>::value, size_t>
    size_of(T value) {
        size_t size = value.size();
        size_t element_size = sizeof(typename T::value_type);

        return sizeof(uint16_t) + size * element_size;
    }

    /**
     * size function if T is an array
     * 
     * Gets the size of the object for copying into a message.
     */
    template <typename T>
    static std::enable_if_t<is_std_array<T>::value, size_t>
    size_of(T value) {
        size_t size = value.size();
        size_t element_size = sizeof(typename T::value_type);

        return sizeof(uint16_t) + size * element_size;
    }

    /**
     * size function if T is a unordered_set
     * 
     * Gets the size of the object for copying into a message.
     */
    template <typename T>
    static std::enable_if_t<is_set<T>::value, size_t>
    size_of(T value) {
        size_t size = value.size();
        size_t element_size = sizeof(typename T::value_type);

        return sizeof(uint16_t) + size * element_size;
    }

    /**
     * size function if T is a struct
     * 
     * Gets the size of the object for copying into a message.
     */
    template <typename T>
    static std::enable_if_t<is_struct<T>::value, size_t>
    size_of(const T& value) {
        size_t total_size = 0;
        for_each(T::fields(), [&total_size, &value](auto field) {
            using FieldType = typename field_type<T, decltype(field)>::type;
            total_size += size_of(value.*field);
        });
        return total_size;
    }


    /**
     * size function if T is a unordered_map
     * 
     * Gets the size of the object for copying into a message.
     */
    template <typename T>
    static std::enable_if_t<is_map<T>::value, size_t>
    size_of(const T& value) {
        size_t total_size = sizeof(uint16_t); // For storing the size of the map
        for (const auto& [key, val] : value) {
            total_size += size_of(key) + size_of(val);
        }
        return total_size;
    }


    // SERIALIZE FUNCTIONS

    /**
     * serialize function if T is a primitive
     * 
     * Copies the bytes of the value into the message at index 0
     */
    template <typename T>
    static std::enable_if_t<is_primitive<T>::value, void>
    serialize(char* message, size_t capacity, T value) {
        if (sizeof(value) > capacity) throw runtime_error("Serializer write overflow");
        memcpy(message, &value, sizeof(value));
    }

    /**
     * serialize function if T is a string
     * 
     * Copies the bytes of the value into the message at index 0
     */
    template <typename T>
    static std::enable_if_t<is_string<T>::value, void>
    serialize(char* message, size_t capacity, T value) {
        uint16_t size = static_cast<uint16_t>(value.size());
        size_t needed = sizeof(size) + size;
        if (needed > capacity) throw runtime_error("Serializer write overflow");
        memcpy(message, &size, sizeof(size));
        memcpy(message + sizeof(size), value.c_str(), size);
    }

    /**
     * serialize function if T is a vector
     * 
     * Copies the bytes of the value into the message at index 0 with the following
     * format:
     * 
     * {size of vector, 2 bytes}{elements, n bytes}
     * 
     * WARNING: only supports vector up to size 65535 elements or max size of uint16_t
     */
    template <typename T>
    static std::enable_if_t<is_vector<T>::value, void>
    serialize(char* message, size_t capacity, T value) {
        uint16_t size = static_cast<uint16_t>(value.size());
        if (sizeof(size) > capacity) throw runtime_error("Serializer write overflow");
        memcpy(message, &size, sizeof(size));

        size_t i = sizeof(size);
        for (const auto& elem : value) {
            serialize(message + i, capacity - i, elem);
            i += size_of(elem);
        }
    }


    /**
     * serialize function if T is an array
     * 
     * Copies the bytes of the value into the message at index 0 with the following
     * format:
     * 
     * {size of array, 2 bytes}{elements, n bytes}
     * 
     * WARNING: only supports arrays up to size 65535 elements or max size of uint16_t
     */
    template <typename T>
    static std::enable_if_t<is_std_array<T>::value, void>
    serialize(char* message, size_t capacity, T value) {
        uint16_t size = static_cast<uint16_t>(value.size());
        if (sizeof(size) > capacity) throw runtime_error("Serializer write overflow");
        memcpy(message, &size, sizeof(size));

        size_t i = sizeof(size);
        for (const auto& elem : value) {
            serialize(message + i, capacity - i, elem);
            i += size_of(elem);
        }
    }

    /**
     * serialize function if T is a unordered_set
     * 
     * Copies the bytes of the value into the message at index 0 with the following
     * format:
     * 
     * {size of unordered_set, 2 bytes}{elements, n bytes}
     * 
     * WARNING: only supports unordered_set up to size 65535 elements or max size of uint16_t
     */
    template <typename T>
    static std::enable_if_t<is_set<T>::value, void>
    serialize(char* message, size_t capacity, T value) {
        uint16_t size = static_cast<uint16_t>(value.size());
        if (sizeof(size) > capacity) throw runtime_error("Serializer write overflow");
        memcpy(message, &size, sizeof(size));

        size_t i = sizeof(size);
        for (const auto& elem : value) {
            serialize(message + i, capacity - i, elem);
            i += size_of(elem);
        }
    }

    /**
     * serialize function if T is a struct or class
     * 
     * Copies the bytes of the value into the message at index 0
     */
    template <typename T>
    static std::enable_if_t<is_struct<T>::value, void>
    serialize(char* message, size_t capacity, const T& value) {
        size_t offset = 0;
        for_each(T::fields(), [&message, &capacity, &offset, &value](auto field) {
            if (offset > capacity) throw runtime_error("Serializer struct overflow");
            using FieldType = typename field_type<T, decltype(field)>::type;
            serialize(message + offset, capacity - offset, value.*field);
            offset += size_of(value.*field);
        });
    }

    /**
     * serialize function if T is a unordered_map
     * 
     * Copies the bytes of the value into the message at index 0 with the following
     * format:
     * 
     * {size of unordered_map, 2 bytes}{elements, n bytes}
     * 
     * WARNING: only supports unordered_map up to size 65535 elements or max size of uint16_t
     */
    template <typename T>
    static std::enable_if_t<is_map<T>::value, void>
    serialize(char* message, size_t capacity, const T& value) {
        uint16_t size = static_cast<uint16_t>(value.size());
        if (sizeof(size) > capacity) throw runtime_error("Serializer write overflow");
        memcpy(message, &size, sizeof(size));

        size_t offset = sizeof(size);
        for (const auto& [key, val] : value) {
            serialize(message + offset, capacity - offset, key);
            offset += size_of(key);

            serialize(message + offset, capacity - offset, val);
            offset += size_of(val);
        }
    }


    // DESERIALIZE FUNCTIONS

    /**
     * deserialize function if T is a primitive
     */
    template <typename T>
    static std::enable_if_t<is_primitive<T>::value, void>
    deserialize(const char* message, size_t capacity, T& value) {
        if (sizeof(value) > capacity) throw runtime_error("Serializer read overflow");
        memcpy(&value, message, sizeof(value));
    }

    /**
     * deserialize function if T is a string
     */
    template <typename T>
    static std::enable_if_t<is_string<T>::value, void>
    deserialize(const char* message, size_t capacity, T& value) {
        uint16_t size;
        if (sizeof(size) > capacity) throw runtime_error("Serializer read overflow");
        memcpy(&size, message, sizeof(size));

        size_t needed = sizeof(size) + size;
        if (needed > capacity) throw runtime_error("Serializer read overflow");
        value = string(message + sizeof(size), size);
    }

    /**
     * deserialize function if T is a vector
     * 
     * Deserializes the message into a vector of type T.
     * 
     * WARNING: only supports vector up to size 65535 elements or max size of uint16_t
     */
    template <typename T>
    static std::enable_if_t<is_vector<T>::value, void>
    deserialize(const char* message, size_t capacity, T& value) {
        uint16_t size;
        if (sizeof(size) > capacity) throw runtime_error("Serializer read overflow");
        memcpy(&size, message, sizeof(size));

        size_t i = sizeof(size);
        for (int j = 0; j < size; ++j) {
            typename T::value_type elem;
            deserialize<typename T::value_type>(message + i, capacity - i, elem);
            value.push_back(elem);
            i += size_of(value[j]);
        }
    }

    /**
     * deserialize function if T is an array
     * 
     * Deserializes the message into a array of type T.
     * 
     * WARNING: only supports arrays up to size 65535 elements or max size of uint16_t
     */
    template <typename T>
    static std::enable_if_t<is_std_array<T>::value, void>
    deserialize(const char* message, size_t capacity, T& value) {
        uint16_t size;
        if (sizeof(size) > capacity) throw runtime_error("Serializer read overflow");
        memcpy(&size, message, sizeof(size));

        constexpr size_t N = std::tuple_size<T>::value;
        if (size > N) size = N;

        size_t i = sizeof(size);
        for (int j = 0; j < size; ++j) {
            typename T::value_type elem;
            deserialize<typename T::value_type>(message + i, capacity - i, elem);
            value[j] = elem;
            i += size_of(value[j]);
        }
        for (size_t j = size; j < N; ++j) {
            value[j] = typename T::value_type{};
        }
    }

    /**
     * deserialize function if T is a unordered_set
     * 
     * Deserializes the message into a unordered_set of type T.
     * 
     * WARNING: only supports unordered_set up to size 65535 elements or max size of uint16_t
     */
    template <typename T>
    static std::enable_if_t<is_set<T>::value, void>
    deserialize(const char* message, size_t capacity, T& value) {
        uint16_t size;
        if (sizeof(size) > capacity) throw runtime_error("Serializer read overflow");
        memcpy(&size, message, sizeof(size));

        size_t i = sizeof(size);
        for (int j = 0; j < size; ++j) {
            typename T::value_type elem;
            deserialize<typename T::value_type>(message + i, capacity - i, elem);
            value.insert(elem);
            i += size_of<typename T::value_type>(elem);
        }
    }

    /**
     * deserialize function if T is a struct
     * 
     * Deserializes the message into a struct of type T.
     */
    template <typename T>
    static std::enable_if_t<is_struct<T>::value, void>
    deserialize(const char* message, size_t capacity, T& value) {
        size_t offset = 0;
        for_each(T::fields(), [&message, &capacity, &offset, &value](auto field) {
            if (offset > capacity) throw runtime_error("Serializer struct overflow");
            using FieldType = typename field_type<T, decltype(field)>::type;
            deserialize(message + offset, capacity - offset, value.*field);
            offset += size_of(value.*field);
        });
    }

    /**
     * deserialize function if T is a unordered_map
     * 
     * Deserializes the message into a unordered_map of type T.
     * 
     * WARNING: only supports unordered_map up to size 65535 elements or max size of uint16_t
     */
    template <typename T>
    static std::enable_if_t<is_map<T>::value, void>
    deserialize(const char* message, size_t capacity, T& value) {
        uint16_t size;
        if (sizeof(size) > capacity) throw runtime_error("Serializer read overflow");
        memcpy(&size, message, sizeof(size));

        size_t offset = sizeof(size);
        for (int i = 0; i < size; ++i) {
            typename T::key_type key;
            typename T::mapped_type val;

            deserialize(message + offset, capacity - offset, key);
            offset += size_of(key);

            deserialize(message + offset, capacity - offset, val);
            offset += size_of(val);

            value[key] = val;
        }
    }
};