#pragma once
#include <memory>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

namespace banli {

enum variable_type {
    NUMBER,
    STRING,
    BOOLEAN,
    VECTOR,
    CLASS,
    VOID
};

class basic_object;
class type_manager;
template <typename _T, typename enable = void>
class type_pointer_wrapper_impl;
template <typename _T>
class type_pointer_wrapper_impl<_T, typename std::enable_if<std::is_arithmetic<_T>::value, void>::type>;
template <typename _T>
class type_pointer_wrapper_impl<_T, typename std::enable_if<std::is_class<_T>::value, void>::type>;

namespace interface {
    class type_manager_interface {
    public:
        virtual ~type_manager_interface() = default;
        virtual basic_object make_instance(const std::string& class_name) = 0;
        virtual basic_object get_field(basic_object& class_obj, const std::string& field_name) = 0;
    };

    class type_pointer_wrapper {
    public:
        virtual ~type_pointer_wrapper() = default;
        virtual void* get() const = 0;
        virtual const std::type_info& type() const = 0;
        virtual void assign(const void* value_ptr, const std::type_info& type) const = 0;
        virtual std::unique_ptr<type_pointer_wrapper> clone() const = 0;
        virtual std::string to_json() const = 0;
        virtual variable_type get_variable_type() const = 0;
        virtual basic_object get_vector_element(size_t index) const = 0;
        virtual void vector_resize(size_t size) = 0;
    };

    class class_field_generator {
    public:
        class_field_generator() = default;
        virtual ~class_field_generator() = default;
        virtual basic_object generate(basic_object& class_obj) = 0;
    };

    class basic_object_generator {
    public:
        basic_object_generator() = default;
        virtual ~basic_object_generator() = default;
        virtual basic_object generate() = 0;
    };

}

interface::type_manager_interface& get_type_manager();

template <typename T>
struct is_vector : std::false_type { };

template <typename T, typename A>
struct is_vector<std::vector<T, A>> : std::true_type { };

class basic_object {
public:
    basic_object();
    basic_object(const basic_object& other);
    basic_object(basic_object&& other);
    template <typename _T>
    basic_object(const _T& value);
    template <typename Class, typename _T>
    basic_object(Class& value, _T Class::*ptr);
    template <typename _T>
    basic_object(_T* ptr, bool should_delete = false);
    ~basic_object() = default;

    template <typename _T>
    inline basic_object& operator=(const _T& value);
    inline basic_object& operator=(const char* value);
    inline basic_object& operator=(const basic_object& other);

    inline std::type_index get_type() const;
    inline bool is_valid() const;
    inline variable_type get_variable_type() const;
    inline std::string to_json() const;

    inline basic_object get_vector_element(size_t index);
    inline basic_object operator[](size_t index);
    inline void vector_resize(size_t size);
    inline basic_object get_class_element(const std::string& field_name);
    inline basic_object operator[](const std::string& field_name);

    template <typename _T>
    inline _T& data_as();

protected:
    std::type_index type;
    std::unique_ptr<interface::type_pointer_wrapper> ptr_wrapper;
};

template <typename _T, typename _U>
class class_field_generator_impl : public interface::class_field_generator {
public:
    class_field_generator_impl(_U _T::*field_ptr)
        : field_ptr(field_ptr)
    {
    }
    ~class_field_generator_impl() = default;

    basic_object generate(basic_object& class_obj) override
    {
        return basic_object(class_obj.data_as<_T>(), field_ptr);
    }

private:
    _U _T::*field_ptr;
};

template <typename _T>
class basic_object_generator_impl : public interface::basic_object_generator {
public:
    basic_object_generator_impl() = default;
    ~basic_object_generator_impl() = default;
    basic_object generate() override
    {
        return basic_object(new _T(), true);
    }
};

class type_manager : public interface::type_manager_interface {
public:
    template <typename _T, typename enable>
    friend class type_pointer_wrapper_impl;
    template <typename _T>
    inline typename std::enable_if<std::is_class<_T>::value, void>::type register_type(const std::string& class_name);
    inline basic_object make_instance(const std::string& class_name) override;
    template <typename _T, typename _U>
    inline typename std::enable_if<std::is_class<_T>::value, void>::type register_field(const std::string& field_name, _U _T::*field_ptr);
    inline basic_object get_field(basic_object& class_obj, const std::string& field_name) override;

private:
    inline static std::unordered_map<std::string, std::type_index> type_map;
    inline static std::unordered_map<std::type_index, std::unique_ptr<interface::basic_object_generator>> class_map;
    inline static std::unordered_map<std::type_index, std::unordered_map<std::string, std::unique_ptr<interface::class_field_generator>>> field_map;
};

inline static type_manager Type_Manager;

template <typename _T>
class type_pointer_wrapper_impl<_T, typename std::enable_if<std::is_arithmetic<_T>::value, void>::type> : public interface::type_pointer_wrapper {
public:
    static_assert(std::is_arithmetic<_T>::value,
        "type_pointer_wrapper_impl only supports arithmetic types");
    type_pointer_wrapper_impl() = delete;
    type_pointer_wrapper_impl(const _T& value)
        : ptr_type(new _T(value))
        , should_delete(true)
    {
    }
    type_pointer_wrapper_impl(_T* ptr, bool _should_delete = false)
        : ptr_type(ptr)
        , should_delete(_should_delete)
    {
    }
    ~type_pointer_wrapper_impl()
    {
        if (should_delete) {
            delete ptr_type;
        }
    }
    void* get() const override
    {
        return ptr_type;
    }
    const std::type_info& type() const override
    {
        return typeid(_T);
    }
    void assign(const void* value_ptr, const std::type_info& type) const override
    {
        if (type == typeid(_T)) {
            *ptr_type = *static_cast<const _T*>(value_ptr);
        }
        // 整数类型
        else if (type == typeid(short)) {
            *ptr_type = static_cast<_T>(*static_cast<const short*>(value_ptr));
        } else if (type == typeid(int)) {
            *ptr_type = static_cast<_T>(*static_cast<const int*>(value_ptr));
        } else if (type == typeid(long)) {
            *ptr_type = static_cast<_T>(*static_cast<const long*>(value_ptr));
        } else if (type == typeid(long long)) {
            *ptr_type = static_cast<_T>(*static_cast<const long long*>(value_ptr));
        } else if (type == typeid(unsigned short)) {
            *ptr_type = static_cast<_T>(*static_cast<const unsigned short*>(value_ptr));
        } else if (type == typeid(unsigned int)) {
            *ptr_type = static_cast<_T>(*static_cast<const unsigned int*>(value_ptr));
        } else if (type == typeid(unsigned long)) {
            *ptr_type = static_cast<_T>(*static_cast<const unsigned long*>(value_ptr));
        } else if (type == typeid(unsigned long long)) {
            *ptr_type = static_cast<_T>(*static_cast<const unsigned long long*>(value_ptr));
        }
        // 浮点类型
        else if (type == typeid(float)) {
            *ptr_type = static_cast<_T>(*static_cast<const float*>(value_ptr));
        } else if (type == typeid(double)) {
            *ptr_type = static_cast<_T>(*static_cast<const double*>(value_ptr));
        } else if (type == typeid(long double)) {
            *ptr_type = static_cast<_T>(*static_cast<const long double*>(value_ptr));
        }
        // 字符类型
        else if (type == typeid(char)) {
            *ptr_type = static_cast<_T>(*static_cast<const char*>(value_ptr));
        } else if (type == typeid(signed char)) {
            *ptr_type = static_cast<_T>(*static_cast<const signed char*>(value_ptr));
        } else if (type == typeid(unsigned char)) {
            *ptr_type = static_cast<_T>(*static_cast<const unsigned char*>(value_ptr));
        } else if (type == typeid(wchar_t)) {
            *ptr_type = static_cast<_T>(*static_cast<const wchar_t*>(value_ptr));
        }
        // 布尔类型
        else if (type == typeid(bool)) {
            *ptr_type = static_cast<_T>(*static_cast<const bool*>(value_ptr));
        } else {
            // throw std::runtime_error("Unsupported type conversion");
        }
    }
    std::string to_json() const override
    {
        if constexpr (std::is_same_v<_T, bool>) {
            return *ptr_type ? "true" : "false";
        } else {
            return std::to_string(*ptr_type);
        }
    }
    std::unique_ptr<type_pointer_wrapper> clone() const override
    {
        return std::make_unique<type_pointer_wrapper_impl<_T>>(*ptr_type);
    }
    variable_type get_variable_type() const override
    {
        if constexpr (std::is_same_v<_T, short>) {
            return variable_type::NUMBER;
        } else if constexpr (std::is_same_v<_T, int>) {
            return variable_type::NUMBER;
        } else if constexpr (std::is_same_v<_T, long>) {
            return variable_type::NUMBER;
        } else if constexpr (std::is_same_v<_T, long long>) {
            return variable_type::NUMBER;
        } else if constexpr (std::is_same_v<_T, unsigned short>) {
            return variable_type::NUMBER;
        } else if constexpr (std::is_same_v<_T, unsigned int>) {
            return variable_type::NUMBER;
        } else if constexpr (std::is_same_v<_T, unsigned long>) {
            return variable_type::NUMBER;
        } else if constexpr (std::is_same_v<_T, unsigned long long>) {
            return variable_type::NUMBER;
        } else if constexpr (std::is_same_v<_T, float>) {
            return variable_type::NUMBER;
        } else if constexpr (std::is_same_v<_T, double>) {
            return variable_type::NUMBER;
        } else if constexpr (std::is_same_v<_T, long double>) {
            return variable_type::NUMBER;
        } else if constexpr (std::is_same_v<_T, char>) {
            return variable_type::NUMBER;
        } else if constexpr (std::is_same_v<_T, signed char>) {
            return variable_type::NUMBER;
        } else if constexpr (std::is_same_v<_T, unsigned char>) {
            return variable_type::NUMBER;
        } else if constexpr (std::is_same_v<_T, wchar_t>) {
            return variable_type::NUMBER;
        } else if constexpr (std::is_same_v<_T, bool>) {
            return variable_type::BOOLEAN;
        } else {
            return variable_type::VOID;
        }
    }
    basic_object get_vector_element(size_t index) const override
    {
        return basic_object();
    }
    void vector_resize(size_t size) override
    {
        return;
    }

private:
    _T* ptr_type;
    bool should_delete = true;
};

template <typename _T>
class type_pointer_wrapper_impl<_T, typename std::enable_if<std::is_class<_T>::value, void>::type> : public interface::type_pointer_wrapper {
public:
    static_assert(std::is_class<_T>::value,
        "type_pointer_wrapper_impl only supports class types");
    type_pointer_wrapper_impl() = delete;
    type_pointer_wrapper_impl(const _T& value)
        : ptr_type(new _T(value))
        , should_delete(true)
    {
    }
    type_pointer_wrapper_impl(_T* ptr, bool _should_delete = false)
        : ptr_type(ptr)
        , should_delete(_should_delete)
    {
    }
    ~type_pointer_wrapper_impl()
    {
        if (should_delete) {
            delete ptr_type;
        }
    }
    void* get() const override
    {
        return ptr_type;
    }
    const std::type_info& type() const override
    {
        return typeid(_T);
    }
    void assign(const void* value_ptr, const std::type_info& type) const override
    {
        if (type == typeid(_T)) {
            *ptr_type = *static_cast<const _T*>(value_ptr);
        } else {
            // throw std::runtime_error("Unsupported type conversion");
        }
    }
    std::unique_ptr<type_pointer_wrapper> clone() const override
    {
        return std::make_unique<type_pointer_wrapper_impl<_T>>(*ptr_type);
    }
    variable_type get_variable_type() const override
    {
        if constexpr (std::is_same_v<_T, std::string>) {
            return variable_type::STRING;
        } else if constexpr (is_vector<_T>::value) {
            return variable_type::VECTOR;
        } else {
            return variable_type::CLASS;
        }
    }
    basic_object get_vector_element(size_t index) const override
    {
        if constexpr (is_vector<_T>::value) {
            if (index < ptr_type->size()) {
                return basic_object((&(*ptr_type)[index]));
            }
        }
        return basic_object();
    }
    std::string to_json() const override
    {
        if constexpr (std::is_same_v<_T, std::string>) {
            return "\"" + *ptr_type + "\"";
        } else if constexpr (is_vector<_T>::value) {
            std::string json_str = "[";
            for (size_t i = 0; i < ptr_type->size(); ++i) {
                json_str += basic_object(&(ptr_type->at(i))).to_json();
                if (i < ptr_type->size() - 1) {
                    json_str += ", ";
                }
            }
            return json_str + "]";
        } else {
            std::string json_str = "{";
            bool first = true;
            for (auto& field : type_manager::field_map[typeid(_T)]) {
                if (first) {
                    first = false;
                } else {
                    json_str += ", ";
                }
                json_str += "\"" + field.first + "\": " + basic_object(ptr_type)[field.first].to_json();
            }
            return json_str + "}";
        }
    }
    void vector_resize(size_t size) override
    {
        if constexpr (is_vector<_T>::value) {
            ptr_type->resize(size);
        }
    }

private:
    _T* ptr_type;
    bool should_delete = true;
};

}

#include "details/reflection.inl"
