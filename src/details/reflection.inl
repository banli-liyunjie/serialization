#pragma once
namespace banli {

basic_object::basic_object()
    : type(std::type_index(typeid(void)))
    , ptr_wrapper(nullptr)
{
}

basic_object::basic_object(const basic_object& other)
    : type(other.type)
    , ptr_wrapper(other.ptr_wrapper->clone())
{
}

basic_object::basic_object(basic_object&& other)
    : type(other.type)
    , ptr_wrapper(std::move(other.ptr_wrapper))
{
    other.ptr_wrapper = nullptr;
}

template <typename _T>
basic_object::basic_object(const _T& value)
    : type(typeid(_T))
    , ptr_wrapper(std::make_unique<type_pointer_wrapper_impl<_T>>(value))
{
}

template <typename Class, typename _T>
basic_object::basic_object(Class& value, _T Class::*ptr)
    : type(typeid(_T))
    , ptr_wrapper(std::make_unique<type_pointer_wrapper_impl<_T>>(&(value.*ptr)))
{
}

template <typename Class, typename _T>
basic_object::basic_object(const Class& value, _T Class::*ptr)
    : type(typeid(_T))
    , ptr_wrapper(std::make_unique<type_pointer_wrapper_impl<_T>>(const_cast<_T*>(&(value.*ptr))))
{
}

template <typename _T>
basic_object::basic_object(_T* ptr, bool should_delete)
    : type(typeid(_T))
    , ptr_wrapper(std::make_unique<type_pointer_wrapper_impl<_T>>(ptr, should_delete))
{
}

template <typename _T>
inline basic_object& basic_object::operator=(const _T& value)
{
    if (is_valid()) {
        ptr_wrapper->assign(&value, typeid(_T));
    }
    return *this;
}

inline basic_object& basic_object::operator=(const char* value)
{
    if (is_valid()) {
        std::string str(value);
        ptr_wrapper->assign(&str, typeid(std::string));
    }
    return *this;
}

inline basic_object& basic_object::operator=(const basic_object& other)
{
    if (is_valid() && other.is_valid()) {
        ptr_wrapper->assign(other.ptr_wrapper->get(), other.ptr_wrapper->type());
    }
    return *this;
}

inline std::type_index basic_object::get_type() const
{
    return type;
}

inline bool basic_object::is_valid() const
{
    return ptr_wrapper != nullptr;
}

inline variable_type basic_object::get_variable_type() const
{
    if (is_valid()) {
        return ptr_wrapper->get_variable_type();
    }
    return variable_type::VOID;
}

inline std::string basic_object::to_json() const
{
    if (is_valid()) {
        return ptr_wrapper->to_json();
    }
    return "null";
}

inline basic_object basic_object::get_vector_element(size_t index)
{
    if (is_valid() && variable_type::VECTOR == get_variable_type()) {
        return ptr_wrapper->get_vector_element(index);
    }
    return basic_object();
}

inline basic_object basic_object::operator[](size_t index)
{
    return get_vector_element(index);
}

inline void basic_object::vector_resize(size_t size)
{
    if (is_valid() && variable_type::VECTOR == get_variable_type()) {
        ptr_wrapper->vector_resize(size);
    }
}

inline basic_object basic_object::get_class_element(const std::string& field_name)
{
    if (is_valid() && variable_type::CLASS == get_variable_type()) {
        return get_type_manager().get_field(*this, field_name);
    }
    return basic_object();
}

inline basic_object basic_object::operator[](const std::string& field_name)
{
    return get_class_element(field_name);
}

template <typename... Args>
inline basic_object basic_object::invoke_method(const std::string& method_name, Args&&... args)
{
    std::vector<basic_object> arg_list;
    arg_list.reserve(sizeof...(args));
    (arg_list.emplace_back(std::forward<Args>(args)), ...);
    return get_type_manager().invoke_method_impl(*this, method_name, arg_list);
}

template <typename _T>
inline const _T& basic_object::data_as() const
{
    if (is_valid()) {
        return *(const _T*)ptr_wrapper->get();
    }
    throw std::runtime_error("Invalid object");
}
template <typename _T>
inline const _T& basic_object::data_as()
{
    if (is_valid()) {
        return *(const _T*)ptr_wrapper->get();
    }
    throw std::runtime_error("Invalid object");
}

inline void* basic_object::as_pointer() const
{
    if (is_valid()) {
        return ptr_wrapper->get();
    }
    return nullptr;
}

template <typename _T>
inline typename std::enable_if<std::is_class<_T>::value, void>::type type_manager::register_type(const std::string& class_name)
{
    std::type_index type = typeid(_T);
    if (type_map.find(class_name) != type_map.end()) {
        return;
    }
    type_map.emplace(class_name, type);
    class_map.emplace(type, std::make_unique<basic_object_generator_impl<_T>>());
}

inline basic_object type_manager::make_instance(const std::string& class_name)
{
    auto it = type_map.find(class_name);
    if (it == type_map.end()) {
        return basic_object();
        // throw std::runtime_error("Class not found");
    }
    return class_map.at(it->second)->generate();
}

template <typename _T, typename _U>
inline typename std::enable_if<std::is_class<_T>::value, void>::type type_manager::register_field(const std::string& field_name, _U _T::*field_ptr)
{
    std::type_index type = typeid(_T);
    auto& field_entries = field_map[type];
    if (field_entries.find(field_name) != field_entries.end()) {
        return;
    }
    field_entries.emplace(
        field_name,
        std::make_unique<class_field_generator_impl<_T, _U>>(field_ptr));
}

inline basic_object type_manager::get_field(basic_object& class_obj, const std::string& field_name)
{
    std::type_index type = class_obj.get_type();
    if (field_map.find(type) == field_map.end() || field_map[type].find(field_name) == field_map[type].end()) {
        return basic_object();
        // throw std::runtime_error("Class not found");
        //  throw std::runtime_error("field not found");
    }
    return field_map[type][field_name]->generate(class_obj);
}

template <typename Class, typename Ret, typename... Args>
inline typename std::enable_if<std::is_class<Class>::value, void>::type type_manager::register_method(const std::string& method_name, Ret (Class::*method)(Args...))
{
    std::type_index type = typeid(Class);
    auto& method_entries = method_map[type];
    if (method_entries.find(method_name) != method_entries.end()) {
        return;
    }
    method_entries.emplace(
        method_name,
        std::make_unique<class_method_wrapper_impl<Class, Ret, Args...>>(method));
}

inline basic_object type_manager::invoke_method(basic_object& class_obj, const std::string& method_name)
{
    std::vector<basic_object> empty_args;
    return invoke_method(class_obj, method_name, empty_args);
}

template <typename... Args>
inline basic_object type_manager::invoke_method(basic_object& class_obj, const std::string& method_name, Args&&... args)
{
    std::vector<basic_object> arg_list;
    arg_list.reserve(sizeof...(args));
    (arg_list.emplace_back(std::forward<Args>(args)), ...);
    return invoke_method_impl(class_obj, method_name, arg_list);
}

inline basic_object type_manager::invoke_method_impl(basic_object& class_obj, const std::string& method_name, const std::vector<basic_object>& args)
{
    std::type_index type = class_obj.get_type();
    if (method_map.find(type) == method_map.end() || method_map[type].find(method_name) == method_map[type].end()) {
        return basic_object();
    }
    return method_map[type][method_name]->invoke(class_obj, args);
}

inline interface::type_manager_interface& get_type_manager()
{
    return Type_Manager;
}

}