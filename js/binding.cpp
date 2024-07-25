#include <algorithm>
#include <assert.h>
#include <string>
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <string_view>

#include <node_api.h>

// #define NINEDB_BINDING_DEBUG

#include "./include/contexts.hpp"
#include "./include/napi_macros.hpp"
#include "./include/napi_utils.hpp"

#include "../src/ninedb/ninedb.hpp"

NAPI_METHOD(kvdb_open)
{
#ifdef NINEDB_BINDING_DEBUG
    std::cerr << "kvdb_open" << std::endl;
#endif

    NAPI_ARGV(2);
    std::string path = napi_utf8_to_string(env, argv[0]);
    napi_value config_obj = argv[1];

    std::unique_ptr<ContextReduceCallback> context_reduce_callback;
    napi_value reduce_func = napi_object_get_property(env, config_obj, "reduce");
    if (reduce_func)
    {
        context_reduce_callback = std::make_unique<ContextReduceCallback>(env, reduce_func);
    }

    ninedb::Config config;
    config.create_if_missing = napi_object_get_property_boolean(env, config_obj, "createIfMissing", true);
    config.delete_if_exists = napi_object_get_property_boolean(env, config_obj, "deleteIfExists", false);
    config.error_if_exists = napi_object_get_property_boolean(env, config_obj, "errorIfExists", false);
    config.max_buffer_size = napi_object_get_property_uint32(env, config_obj, "maxBufferSize", 1 << 22);
    config.max_level_count = napi_object_get_property_uint32(env, config_obj, "maxLevelCount", 10);
    config.reader.internal_node_cache_size = napi_object_get_property_uint32(env, config_obj, "internalNodeCacheSize", 64);
    config.reader.leaf_node_cache_size = napi_object_get_property_uint32(env, config_obj, "leafNodeCacheSize", 8);
    config.writer.enable_compression = napi_object_get_property_boolean(env, config_obj, "enableCompression", true);
    config.writer.enable_prefix_encoding = napi_object_get_property_boolean(env, config_obj, "enablePrefixEncoding", true);
    config.writer.initial_pbt_size = napi_object_get_property_uint32(env, config_obj, "initialPbtSize", 1 << 23);
    config.writer.max_node_children = napi_object_get_property_uint32(env, config_obj, "maxNodeChildren", 16);
    if (context_reduce_callback)
    {
        config.writer.reduce = std::bind(&ContextReduceCallback::call, context_reduce_callback.get(), std::placeholders::_1, std::placeholders::_2);
    }

    try
    {
        // new: will delete itself when Node finalizes it
        ContextKvDb *context = new ContextKvDb(env, path, config, std::move(context_reduce_callback));
        return context->get_external();
    }
    catch (const std::exception &e)
    {
        napi_throw_error(env, "EINVAL", e.what());
        return NULL;
    }
}

NAPI_METHOD(kvdb_add)
{
#ifdef NINEDB_BINDING_DEBUG
    std::cerr << "kvdb_add" << std::endl;
#endif

    NAPI_ARGV(3);
    ContextKvDb *context = napi_external_value<ContextKvDb *>(env, argv[0]);
    std::string_view key = napi_buffer_to_string_view(env, argv[1]);
    std::string_view val = napi_buffer_to_string_view(env, argv[2]);

    try
    {
        context->kvdb.add(key, val);
    }
    catch (const std::exception &e)
    {
        napi_throw_error(env, "EINVAL", e.what());
        return NULL;
    }

    return napi_value_undefined();
}

NAPI_METHOD(kvdb_get)
{
#ifdef NINEDB_BINDING_DEBUG
    std::cerr << "kvdb_get" << std::endl;
#endif

    NAPI_ARGV(2);
    ContextKvDb *context = napi_external_value<ContextKvDb *>(env, argv[0]);
    std::string_view key = napi_buffer_to_string_view(env, argv[1]);

    try
    {
        napi_value result;
        std::string_view value;
        if (context->kvdb.get(key, value))
        {
            NAPI_STATUS_THROW_ERROR(napi_create_buffer_copy(env, value.size(), value.data(), NULL, &result));
        }
        else
        {
            NAPI_STATUS_THROW_ERROR(napi_get_null(env, &result));
        }
        return result;
    }
    catch (const std::exception &e)
    {
        napi_throw_error(env, "EINVAL", e.what());
        return NULL;
    }
}

NAPI_METHOD(kvdb_at)
{
#ifdef NINEDB_BINDING_DEBUG
    std::cerr << "kvdb_at" << std::endl;
#endif

    NAPI_ARGV(2);
    ContextKvDb *context = napi_external_value<ContextKvDb *>(env, argv[0]);
    NAPI_ARGV_UINT32(index, 1);

    try
    {
        napi_value result;
        std::string key;
        std::string_view value;
        if (context->kvdb.at(index, key, value))
        {
            napi_value result_key;
            napi_value result_value;
            NAPI_STATUS_THROW_ERROR(napi_create_buffer_copy(env, key.size(), key.data(), NULL, &result_key));
            NAPI_STATUS_THROW_ERROR(napi_create_buffer_copy(env, value.size(), value.data(), NULL, &result_value));
            NAPI_STATUS_THROW_ERROR(napi_create_object(env, &result));
            NAPI_STATUS_THROW_ERROR(napi_set_named_property(env, result, "key", result_key));
            NAPI_STATUS_THROW_ERROR(napi_set_named_property(env, result, "value", result_value));
        }
        else
        {
            NAPI_STATUS_THROW_ERROR(napi_get_null(env, &result));
        }
        return result;
    }
    catch (const std::exception &e)
    {
        napi_throw_error(env, "EINVAL", e.what());
        return NULL;
    }
}

NAPI_METHOD(kvdb_traverse)
{
#ifdef NINEDB_BINDING_DEBUG
    std::cerr << "kvdb_traverse" << std::endl;
#endif

    NAPI_ARGV(2);
    ContextKvDb *context = napi_external_value<ContextKvDb *>(env, argv[0]);

    napi_value callback = argv[1];
    napi_value global;
    NAPI_STATUS_THROW_ERROR(napi_get_global(env, &global));

    try
    {
        std::vector<std::string_view> accumulator;
        context->kvdb.traverse(
            [=](std::string_view value)
            {
                napi_value result;
                napi_value buffer;
                bool result_bool;
                NAPI_STATUS_THROW_ERROR(napi_create_buffer_copy(env, value.size(), value.data(), NULL, &buffer));
                NAPI_STATUS_THROW_ERROR(napi_call_function(env, global, callback, 1, &buffer, &result));
                NAPI_STATUS_THROW_ERROR(napi_get_value_bool(env, result, &result_bool));
                return result_bool;
            },
            accumulator);

        napi_value result;
        NAPI_STATUS_THROW_ERROR(napi_create_array_with_length(env, accumulator.size(), &result));
        for (size_t i = 0; i < accumulator.size(); i++)
        {
            napi_value buffer;
            NAPI_STATUS_THROW_ERROR(napi_create_buffer_copy(env, accumulator[i].size(), accumulator[i].data(), NULL, &buffer));
            NAPI_STATUS_THROW_ERROR(napi_set_element(env, result, i, buffer));
        }

        return result;
    }
    catch (const std::exception &e)
    {
        napi_throw_error(env, "EINVAL", e.what());
        return NULL;
    }
}

NAPI_METHOD(kvdb_flush)
{
#ifdef NINEDB_BINDING_DEBUG
    std::cerr << "kvdb_flush" << std::endl;
#endif

    NAPI_ARGV(1);
    ContextKvDb *context = napi_external_value<ContextKvDb *>(env, argv[0]);

    try
    {
        context->kvdb.flush();
    }
    catch (const std::exception &e)
    {
        napi_throw_error(env, "EINVAL", e.what());
        return NULL;
    }

    return napi_value_undefined();
}

NAPI_METHOD(kvdb_compact)
{
#ifdef NINEDB_BINDING_DEBUG
    std::cerr << "kvdb_compact" << std::endl;
#endif

    NAPI_ARGV(1);
    ContextKvDb *context = napi_external_value<ContextKvDb *>(env, argv[0]);

    try
    {
        context->kvdb.compact();
    }
    catch (const std::exception &e)
    {
        napi_throw_error(env, "EINVAL", e.what());
        return NULL;
    }

    return napi_value_undefined();
}

NAPI_METHOD(kvdb_begin)
{
#ifdef NINEDB_BINDING_DEBUG
    std::cerr << "kvdb_begin" << std::endl;
#endif

    NAPI_ARGV(1);
    ContextKvDb *context = napi_external_value<ContextKvDb *>(env, argv[0]);

    try
    {
        // new: will delete itself when Node finalizes it
        ContextKvIterator *iterator = new ContextKvIterator(env, context, context->kvdb.begin());
        return iterator->get_external();
    }
    catch (const std::exception &e)
    {
        napi_throw_error(env, "EINVAL", e.what());
        return NULL;
    }
}

NAPI_METHOD(kvdb_seek_key)
{
#ifdef NINEDB_BINDING_DEBUG
    std::cerr << "kvdb_seek_key" << std::endl;
#endif

    NAPI_ARGV(2);
    ContextKvDb *context = napi_external_value<ContextKvDb *>(env, argv[0]);
    std::string_view key = napi_buffer_to_string_view(env, argv[1]);

    try
    {
        // new: will delete itself when Node finalizes it
        ContextKvIterator *iterator = new ContextKvIterator(env, context, context->kvdb.seek(key));
        return iterator->get_external();
    }
    catch (const std::exception &e)
    {
        napi_throw_error(env, "EINVAL", e.what());
        return NULL;
    }
}

NAPI_METHOD(kvdb_seek_index)
{
#ifdef NINEDB_BINDING_DEBUG
    std::cerr << "kvdb_seek_index" << std::endl;
#endif

    NAPI_ARGV(2);
    ContextKvDb *context = napi_external_value<ContextKvDb *>(env, argv[0]);
    NAPI_ARGV_UINT32(index, 1);

    try
    {
        // new: will delete itself when Node finalizes it
        ContextKvIterator *iterator = new ContextKvIterator(env, context, context->kvdb.seek(index));
        return iterator->get_external();
    }
    catch (const std::exception &e)
    {
        napi_throw_error(env, "EINVAL", e.what());
        return NULL;
    }
}

NAPI_METHOD(itr_next)
{
#ifdef NINEDB_BINDING_DEBUG
    std::cerr << "itr_next" << std::endl;
#endif

    NAPI_ARGV(1);
    ContextKvIterator *iterator = napi_external_value<ContextKvIterator *>(env, argv[0]);

    try
    {
        iterator->itr.next();
    }
    catch (const std::exception &e)
    {
        napi_throw_error(env, "EINVAL", e.what());
        return NULL;
    }

    return napi_value_undefined();
}

NAPI_METHOD(itr_has_next)
{
#ifdef NINEDB_BINDING_DEBUG
    std::cerr << "itr_has_next" << std::endl;
#endif

    NAPI_ARGV(1);
    ContextKvIterator *iterator = napi_external_value<ContextKvIterator *>(env, argv[0]);

    try
    {
        return napi_value_boolean(env, !iterator->itr.is_end());
    }
    catch (const std::exception &e)
    {
        napi_throw_error(env, "EINVAL", e.what());
        return NULL;
    }
}

NAPI_METHOD(itr_get_key)
{
#ifdef NINEDB_BINDING_DEBUG
    std::cerr << "itr_get_key" << std::endl;
#endif

    NAPI_ARGV(1);
    ContextKvIterator *iterator = napi_external_value<ContextKvIterator *>(env, argv[0]);

    try
    {
        std::string key = iterator->itr.get_key();
        return string_to_napi_buffer(env, key);
    }
    catch (const std::exception &e)
    {
        napi_throw_error(env, "EINVAL", e.what());
        return NULL;
    }
}

NAPI_METHOD(itr_get_value)
{
#ifdef NINEDB_BINDING_DEBUG
    std::cerr << "itr_get_value" << std::endl;
#endif

    NAPI_ARGV(1);
    ContextKvIterator *iterator = napi_external_value<ContextKvIterator *>(env, argv[0]);

    try
    {
        std::string_view value = iterator->itr.get_value();
        return string_view_to_napi_buffer(env, value);
    }
    catch (const std::exception &e)
    {
        napi_throw_error(env, "EINVAL", e.what());
        return NULL;
    }
}

NAPI_METHOD(hrdb_open)
{
#ifdef NINEDB_BINDING_DEBUG
    std::cerr << "hrdb_open" << std::endl;
#endif

    NAPI_ARGV(2);
    std::string path = napi_utf8_to_string(env, argv[0]);
    napi_value config_obj = argv[1];

    ninedb::Config config;
    config.create_if_missing = napi_object_get_property_boolean(env, config_obj, "createIfMissing", true);
    config.delete_if_exists = napi_object_get_property_boolean(env, config_obj, "deleteIfExists", false);
    config.error_if_exists = napi_object_get_property_boolean(env, config_obj, "errorIfExists", false);
    config.max_buffer_size = napi_object_get_property_uint32(env, config_obj, "maxBufferSize", 1 << 22);
    config.max_level_count = napi_object_get_property_uint32(env, config_obj, "maxLevelCount", 10);
    config.reader.internal_node_cache_size = napi_object_get_property_uint32(env, config_obj, "internalNodeCacheSize", 64);
    config.reader.leaf_node_cache_size = napi_object_get_property_uint32(env, config_obj, "leafNodeCacheSize", 8);
    config.writer.enable_compression = napi_object_get_property_boolean(env, config_obj, "enableCompression", true);
    config.writer.enable_prefix_encoding = napi_object_get_property_boolean(env, config_obj, "enablePrefixEncoding", true);
    config.writer.initial_pbt_size = napi_object_get_property_uint32(env, config_obj, "initialPbtSize", 1 << 23);
    config.writer.max_node_children = napi_object_get_property_uint32(env, config_obj, "maxNodeChildren", 16);

    try
    {
        // new: will delete itself when Node finalizes it
        ContextHrDb *context = new ContextHrDb(env, path, config);
        return context->get_external();
    }
    catch (const std::exception &e)
    {
        napi_throw_error(env, "EINVAL", e.what());
        return NULL;
    }
}

NAPI_METHOD(hrdb_add)
{
#ifdef NINEDB_BINDING_DEBUG
    std::cerr << "hrdb_add" << std::endl;
#endif

    NAPI_ARGV(6);
    ContextHrDb *context = napi_external_value<ContextHrDb *>(env, argv[0]);
    uint32_t x0 = napi_value_to_uint32(env, argv[1]);
    uint32_t y0 = napi_value_to_uint32(env, argv[2]);
    uint32_t x1 = napi_value_to_uint32(env, argv[3]);
    uint32_t y1 = napi_value_to_uint32(env, argv[4]);
    std::string_view val = napi_buffer_to_string_view(env, argv[5]);

    try
    {
        context->hrdb.add(x0, y0, x1, y1, val);
    }
    catch (const std::exception &e)
    {
        napi_throw_error(env, "EINVAL", e.what());
        return NULL;
    }

    return napi_value_undefined();
}

NAPI_METHOD(hrdb_search)
{
#ifdef NINEDB_BINDING_DEBUG
    std::cerr << "hrdb_search" << std::endl;
#endif

    NAPI_ARGV(5);
    ContextHrDb *context = napi_external_value<ContextHrDb *>(env, argv[0]);
    uint32_t x0 = napi_value_to_uint32(env, argv[1]);
    uint32_t y0 = napi_value_to_uint32(env, argv[2]);
    uint32_t x1 = napi_value_to_uint32(env, argv[3]);
    uint32_t y1 = napi_value_to_uint32(env, argv[4]);

    try
    {
        std::vector<std::string_view> values = context->hrdb.search(x0, y0, x1, y1);
        napi_value result;
        NAPI_STATUS_THROW_ERROR(napi_create_array_with_length(env, values.size(), &result));
        for (size_t i = 0; i < values.size(); i++)
        {
            napi_value buffer;
            NAPI_STATUS_THROW_ERROR(napi_create_buffer_copy(env, values[i].size(), values[i].data(), NULL, &buffer));
            NAPI_STATUS_THROW_ERROR(napi_set_element(env, result, i, buffer));
        }
        return result;
    }
    catch (const std::exception &e)
    {
        napi_throw_error(env, "EINVAL", e.what());
        return NULL;
    }
}

NAPI_METHOD(hrdb_flush)
{
#ifdef NINEDB_BINDING_DEBUG
    std::cerr << "hrdb_flush" << std::endl;
#endif

    NAPI_ARGV(1);
    ContextHrDb *context = napi_external_value<ContextHrDb *>(env, argv[0]);

    try
    {
        context->hrdb.flush();
    }
    catch (const std::exception &e)
    {
        napi_throw_error(env, "EINVAL", e.what());
        return NULL;
    }

    return napi_value_undefined();
}

NAPI_METHOD(hrdb_compact)
{
#ifdef NINEDB_BINDING_DEBUG
    std::cerr << "hrdb_compact" << std::endl;
#endif

    NAPI_ARGV(1);
    ContextHrDb *context = napi_external_value<ContextHrDb *>(env, argv[0]);

    try
    {
        context->hrdb.compact();
    }
    catch (const std::exception &e)
    {
        napi_throw_error(env, "EINVAL", e.what());
        return NULL;
    }

    return napi_value_undefined();
}

NAPI_INIT()
{
    NAPI_EXPORT_FUNCTION(kvdb_open);
    NAPI_EXPORT_FUNCTION(kvdb_add);
    NAPI_EXPORT_FUNCTION(kvdb_get);
    NAPI_EXPORT_FUNCTION(kvdb_at);
    NAPI_EXPORT_FUNCTION(kvdb_traverse);
    NAPI_EXPORT_FUNCTION(kvdb_flush);
    NAPI_EXPORT_FUNCTION(kvdb_compact);
    NAPI_EXPORT_FUNCTION(kvdb_begin);
    NAPI_EXPORT_FUNCTION(kvdb_seek_key);
    NAPI_EXPORT_FUNCTION(kvdb_seek_index);

    NAPI_EXPORT_FUNCTION(itr_next);
    NAPI_EXPORT_FUNCTION(itr_has_next);
    NAPI_EXPORT_FUNCTION(itr_get_key);
    NAPI_EXPORT_FUNCTION(itr_get_value);

    NAPI_EXPORT_FUNCTION(hrdb_open);
    NAPI_EXPORT_FUNCTION(hrdb_add);
    NAPI_EXPORT_FUNCTION(hrdb_search);
    NAPI_EXPORT_FUNCTION(hrdb_flush);
    NAPI_EXPORT_FUNCTION(hrdb_compact);
}
