#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <node_api.h>

#define NAPI_STATUS_THROW_ERROR(call)                  \
    if ((call) != napi_ok)                             \
    {                                                  \
        std::cerr << #call " failed!" << std::endl;    \
        napi_throw_error(env, NULL, #call " failed!"); \
        throw std::runtime_error(#call " failed!");    \
    }

std::string napi_utf8_to_string(napi_env env, napi_value val)
{
    std::string result;
    size_t size;
    NAPI_STATUS_THROW_ERROR(napi_get_value_string_utf8(env, val, nullptr, 0, &size));
    result.reserve(size + 1);
    result.resize(size);
    NAPI_STATUS_THROW_ERROR(napi_get_value_string_utf8(env, val, &result[0], result.capacity(), nullptr));
    return result;
}

std::string_view napi_buffer_to_string_view(napi_env env, napi_value val)
{
    char *data;
    size_t size;
    NAPI_STATUS_THROW_ERROR(napi_get_buffer_info(env, val, (void **)&data, &size));
    return std::string_view(data, size);
}

uint32_t napi_value_to_uint32(napi_env env, napi_value val)
{
    uint32_t result;
    NAPI_STATUS_THROW_ERROR(napi_get_value_uint32(env, val, &result));
    return result;
}

napi_value string_to_napi_buffer(napi_env env, const std::string &str)
{
    napi_value result;
    NAPI_STATUS_THROW_ERROR(napi_create_buffer_copy(env, str.size(), str.data(), nullptr, &result));
    return result;
}

napi_value string_view_to_napi_buffer(napi_env env, const std::string_view &str)
{
    napi_value result;
    NAPI_STATUS_THROW_ERROR(napi_create_buffer_copy(env, str.size(), str.data(), nullptr, &result));
    return result;
}

constexpr napi_value napi_value_undefined()
{
    return 0;
}

napi_value napi_value_boolean(napi_env env, bool value)
{
    napi_value result;
    NAPI_STATUS_THROW_ERROR(napi_get_boolean(env, value, &result));
    return result;
}

template <typename R, typename... Args>
struct FunctionContext
{
private:
    napi_env env;
    napi_ref func_ref;
    std::function<std::vector<napi_value>(napi_env, Args...)> to_napi_args;
    std::function<R(napi_env, napi_value, Args...)> from_napi_result;

public:
    FunctionContext(napi_env env, napi_value func, std::function<std::vector<napi_value>(napi_env, Args...)> to_napi_args, std::function<R(napi_env, napi_value, Args...)> from_napi_result)
        : env(env), to_napi_args(to_napi_args), from_napi_result(from_napi_result)
    {
        NAPI_STATUS_THROW_ERROR(napi_create_reference(env, func, 1, &func_ref));
    }

    ~FunctionContext()
    {
        napi_delete_reference(env, func_ref);
    }

    R call(Args... args)
    {
        napi_value global;
        napi_value func;
        NAPI_STATUS_THROW_ERROR(napi_get_global(env, &global));
        NAPI_STATUS_THROW_ERROR(napi_get_reference_value(env, func_ref, &func));
        std::vector<napi_value> argv = to_napi_args(env, args...);
        napi_value result;
        NAPI_STATUS_THROW_ERROR(napi_call_function(env, global, func, argv.size(), argv.data(), &result));
        return from_napi_result(env, result, args...);
    }
};

bool napi_object_has_property(napi_env env, napi_value obj, const char *prop)
{
    bool prop_exists;
    NAPI_STATUS_THROW_ERROR(napi_has_named_property(env, obj, prop, &prop_exists));
    return prop_exists;
}

bool napi_object_get_property_boolean(napi_env env, napi_value obj, const char *prop, bool default_value)
{
    if (napi_object_has_property(env, obj, prop))
    {
        napi_value value;
        NAPI_STATUS_THROW_ERROR(napi_get_named_property(env, obj, prop, &value));
        bool result;
        NAPI_STATUS_THROW_ERROR(napi_get_value_bool(env, value, &result));
        return result;
    }
    return default_value;
}

uint32_t napi_object_get_property_uint32(napi_env env, napi_value obj, const char *prop, uint32_t default_value)
{
    if (napi_object_has_property(env, obj, prop))
    {
        napi_value value;
        NAPI_STATUS_THROW_ERROR(napi_get_named_property(env, obj, prop, &value));
        uint32_t result;
        NAPI_STATUS_THROW_ERROR(napi_get_value_uint32(env, value, &result));
        return result;
    }
    return default_value;
}

napi_value napi_object_get_property(napi_env env, napi_value obj, const char *prop)
{
    if (napi_object_has_property(env, obj, prop))
    {
        napi_value value;
        NAPI_STATUS_THROW_ERROR(napi_get_named_property(env, obj, prop, &value));
        return value;
    }
    return nullptr;
}

template <typename T>
T napi_external_value(napi_env env, napi_value val)
{
    T result;
    NAPI_STATUS_THROW_ERROR(napi_get_value_external(env, val, (void **)&result));
    return result;
}
