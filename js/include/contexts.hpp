#pragma once

#include <memory>
#include <string>
#include <vector>

#include <node_api.h>

#include "./napi_utils.hpp"
#include "../../src/ninedb/ninedb.hpp"

// Forward declarations
struct ContextKvDb;
struct ContextHrDb;
struct ContextKvIterator;
struct ContextReduceCallback;

struct ContextReduceCallback : FunctionContext<void, const std::vector<std::string> &, std::string &>
{
    ContextReduceCallback(napi_env env, napi_value func)
        : FunctionContext<void, const std::vector<std::string> &, std::string &>(env, func, to_napi_args, from_napi_result) {}

private:
    static std::vector<napi_value> to_napi_args(napi_env env, const std::vector<std::string> &values, std::string &reduced_value)
    {
        napi_value buffers;
        NAPI_STATUS_THROW_ERROR(napi_create_array_with_length(env, values.size(), &buffers));
        for (size_t i = 0; i < values.size(); i++)
        {
            napi_value buffer;
            NAPI_STATUS_THROW_ERROR(napi_create_buffer_copy(env, values[i].size(), values[i].data(), NULL, &buffer));
            NAPI_STATUS_THROW_ERROR(napi_set_element(env, buffers, i, buffer));
        }
        return {buffers};
    }

    static void from_napi_result(napi_env env, napi_value result, const std::vector<std::string> &values, std::string &reduced_value)
    {
        char *result_string;
        size_t result_string_len;
        NAPI_STATUS_THROW_ERROR(napi_get_buffer_info(env, result, (void **)&result_string, &result_string_len));
        reduced_value.assign(result_string, result_string_len);
    }
};

struct ContextKvDb
{
public:
    ninedb::KvDb kvdb;

private:
    napi_value external;
    napi_ref external_ref;
    std::unique_ptr<ContextReduceCallback> context_reduce_callback;
    std::vector<ContextKvIterator *> iterators;

public:
    ContextKvDb(napi_env env, const std::string &path, const ninedb::Config &config, std::unique_ptr<ContextReduceCallback> &&context_reduce_callback)
        : kvdb(ninedb::KvDb::open(path, config)), context_reduce_callback(std::move(context_reduce_callback))
    {
        NAPI_STATUS_THROW_ERROR(napi_add_env_cleanup_hook(env, cleanup_hook, this));
        NAPI_STATUS_THROW_ERROR(napi_create_external(env, this, finalize_cb, NULL, &external));
        NAPI_STATUS_THROW_ERROR(napi_create_reference(env, external, 0, &external_ref));
    }

    napi_value get_external() const
    {
        return external;
    }

    void add_iterator(ContextKvIterator *iterator)
    {
        iterators.push_back(iterator);
    }

    void remove_iterator(ContextKvIterator *iterator)
    {
        iterators.erase(std::remove(iterators.begin(), iterators.end(), iterator), iterators.end());
    }

    void ref(napi_env env)
    {
        NAPI_STATUS_THROW_ERROR(napi_reference_ref(env, external_ref, NULL));
    }

    void unref(napi_env env)
    {
        NAPI_STATUS_THROW_ERROR(napi_reference_unref(env, external_ref, NULL));
    }

private:
    static void cleanup_hook(void *arg)
    {
        if (arg)
        {
            ContextKvDb *context = (ContextKvDb *)arg;
            context->kvdb.flush();
        }
    }

    static void finalize_cb(napi_env env, void *data, void *hint)
    {
        if (data)
        {
            ContextKvDb *context = (ContextKvDb *)data;
            context->kvdb.flush();
            napi_delete_reference(env, context->external_ref);
            napi_remove_env_cleanup_hook(env, cleanup_hook, context);
            delete context;
        }
    }
};

struct ContextHrDb
{
public:
    ninedb::HrDb hrdb;

private:
    napi_value external;

public:
    ContextHrDb(napi_env env, const std::string &path, const ninedb::Config &config)
        : hrdb(ninedb::HrDb::open(path, config))
    {
        NAPI_STATUS_THROW_ERROR(napi_add_env_cleanup_hook(env, cleanup_hook, this));
        NAPI_STATUS_THROW_ERROR(napi_create_external(env, this, finalize_cb, NULL, &external));
    }

    napi_value get_external() const
    {
        return external;
    }

private:
    static void cleanup_hook(void *arg)
    {
        if (arg)
        {
            ContextHrDb *context = (ContextHrDb *)arg;
            context->hrdb.flush();
        }
    }

    static void finalize_cb(napi_env env, void *data, void *hint)
    {
        if (data)
        {
            ContextHrDb *context = (ContextHrDb *)data;
            context->hrdb.flush();
            napi_remove_env_cleanup_hook(env, cleanup_hook, context);
            delete context;
        }
    }
};

struct ContextKvIterator
{
public:
    ninedb::Iterator itr;

private:
    napi_value external;
    ContextKvDb *context_db;

public:
    ContextKvIterator(napi_env env, ContextKvDb *context_db, ninedb::Iterator &&itr)
        : context_db(context_db), itr(std::move(itr))
    {
        NAPI_STATUS_THROW_ERROR(napi_create_external(env, this, finalize_cb, NULL, &external));
        context_db->ref(env);
        context_db->add_iterator(this);
    }

    napi_value get_external() const
    {
        return external;
    }

private:
    static void finalize_cb(napi_env env, void *data, void *hint)
    {
        if (data)
        {
            ContextKvIterator *iterator = (ContextKvIterator *)data;
            ContextKvDb *context_db = iterator->context_db;
            context_db->remove_iterator(iterator);
            context_db->unref(env);
            delete iterator;
        }
    }
};
