#include <jni.h>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "./io_woutervh_ninedb_KvDatabase.h"
#include "./jni_utils.hpp"
#include "./contexts.hpp"

#include "../../../../src/ninedb/ninedb.hpp"

JNIEXPORT jlong JNICALL Java_io_woutervh_ninedb_KvDatabase_kvdb_1open(JNIEnv *env, jclass, jstring str_path, jobject obj_config)
{
    const char *path = env->GetStringUTFChars(str_path, nullptr);

    std::unique_ptr<ContextReduceCallback> context_reduce_callback;
    jobject obj_reduce = jni_object_get_property_object(env, obj_config, "reduce", "Ljava/util/function/Function;");
    if (obj_reduce != nullptr)
    {
        context_reduce_callback = std::make_unique<ContextReduceCallback>(env, obj_reduce);
    }

    ninedb::Config config;
    config.create_if_missing = jni_object_get_property_boolean_boxed(env, obj_config, "createIfMissing", true);
    config.delete_if_exists = jni_object_get_property_boolean_boxed(env, obj_config, "deleteIfExists", false);
    config.error_if_exists = jni_object_get_property_boolean_boxed(env, obj_config, "errorIfExists", false);
    config.max_buffer_size = jni_object_get_property_integer_boxed(env, obj_config, "maxBufferSize", 1 << 22);
    config.max_level_count = jni_object_get_property_integer_boxed(env, obj_config, "maxLevelCount", 10);
    config.reader.internal_node_cache_size = jni_object_get_property_integer_boxed(env, obj_config, "internalNodeCacheSize", 64);
    config.reader.leaf_node_cache_size = jni_object_get_property_integer_boxed(env, obj_config, "leafNodeCacheSize", 8);
    config.writer.enable_compression = jni_object_get_property_boolean_boxed(env, obj_config, "enableCompression", true);
    config.writer.enable_prefix_encoding = jni_object_get_property_boolean_boxed(env, obj_config, "enablePrefixEncoding", true);
    config.writer.initial_pbt_size = jni_object_get_property_integer_boxed(env, obj_config, "initialPbtSize", 1 << 23);
    config.writer.max_node_entries = jni_object_get_property_integer_boxed(env, obj_config, "maxNodeEntries", 16);
    if (context_reduce_callback)
    {
        config.writer.reduce = std::bind(&ContextReduceCallback::call, context_reduce_callback.get(), std::placeholders::_1, std::placeholders::_2);
    }

    KvDbContext *context = new KvDbContext(path, config, std::move(context_reduce_callback));
    jlong ptr = static_cast<jlong>(reinterpret_cast<size_t>(context));

    env->ReleaseStringUTFChars(str_path, path);

    return ptr;
}

JNIEXPORT void JNICALL Java_io_woutervh_ninedb_KvDatabase_kvdb_1close(JNIEnv *, jclass, jlong j_db_handle)
{
    KvDbContext *context = reinterpret_cast<KvDbContext *>(j_db_handle);
    context->kvdb.flush();
    delete context;
}

JNIEXPORT void JNICALL Java_io_woutervh_ninedb_KvDatabase_kvdb_1add(JNIEnv *env, jclass, jlong j_db_handle, jbyteArray j_key, jbyteArray j_value)
{
    KvDbContext *context = reinterpret_cast<KvDbContext *>(j_db_handle);
    std::string key = jni_byte_array_to_string(env, j_key);
    std::string value = jni_byte_array_to_string(env, j_value);
    context->kvdb.add(key, value);
}

JNIEXPORT jbyteArray JNICALL Java_io_woutervh_ninedb_KvDatabase_kvdb_1get(JNIEnv *env, jclass, jlong j_db_handle, jbyteArray j_key)
{
    KvDbContext *context = reinterpret_cast<KvDbContext *>(j_db_handle);
    std::string key = jni_byte_array_to_string(env, j_key);
    std::optional<std::string_view> value = context->kvdb.get(key);

    if (value.has_value())
    {
        jbyteArray result = env->NewByteArray(value->size());
        env->SetByteArrayRegion(result, 0, value->size(), reinterpret_cast<const jbyte *>(value->data()));
        return result;
    }
    else
    {
        return nullptr;
    }
}

JNIEXPORT jobject JNICALL Java_io_woutervh_ninedb_KvDatabase_kvdb_1at(JNIEnv *env, jclass, jlong j_db_handle, jlong at)
{
    KvDbContext *context = reinterpret_cast<KvDbContext *>(j_db_handle);

    std::optional<std::pair<std::string, std::string_view>> kv = context->kvdb.at(at);
    if (!kv.has_value())
    {
        return nullptr;
    }

    jclass cls_KeyValuePair = env->FindClass("io/woutervh/ninedb/KeyValuePair");
    if (cls_KeyValuePair == nullptr)
    {
        jclass cls_Exception = env->FindClass("java/lang/Exception");
        env->ThrowNew(cls_Exception, "Cannot find KeyValuePair class.");
    }

    jbyteArray key = env->NewByteArray(kv->first.size());
    env->SetByteArrayRegion(key, 0, kv->first.size(), reinterpret_cast<const jbyte *>(kv->first.data()));
    jbyteArray value = env->NewByteArray(kv->second.size());
    env->SetByteArrayRegion(value, 0, kv->second.size(), reinterpret_cast<const jbyte *>(kv->second.data()));
    jmethodID mid_KeyValuePair = env->GetMethodID(cls_KeyValuePair, "<init>", "([B[B)V");
    jobject obj_result = env->NewObject(cls_KeyValuePair, mid_KeyValuePair, key, value);

    return obj_result;
}

JNIEXPORT void JNICALL Java_io_woutervh_ninedb_KvDatabase_kvdb_1flush(JNIEnv *, jclass, jlong j_db_handle)
{
    KvDbContext *context = reinterpret_cast<KvDbContext *>(j_db_handle);
    context->kvdb.flush();
}

JNIEXPORT void JNICALL Java_io_woutervh_ninedb_KvDatabase_kvdb_1compact(JNIEnv *, jclass, jlong j_db_handle)
{
    KvDbContext *context = reinterpret_cast<KvDbContext *>(j_db_handle);
    context->kvdb.compact();
}
