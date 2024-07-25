#include <jni.h>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "./io_pinesw_ninedb_KvDatabase.h"
#include "./io_pinesw_ninedb_KvDbIterator.h"
#include "./io_pinesw_ninedb_HrDatabase.h"

#include "./jni_utils.hpp"
#include "./contexts.hpp"

#include "../../../../src/ninedb/ninedb.hpp"

JNIEXPORT jlong JNICALL Java_io_pinesw_ninedb_KvDatabase_kvdb_1open(JNIEnv *env, jclass, jstring str_path, jobject obj_config)
{
    const char *path = env->GetStringUTFChars(str_path, nullptr);
    std::string path_str(path);
    env->ReleaseStringUTFChars(str_path, path);

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
    config.writer.max_node_children = jni_object_get_property_integer_boxed(env, obj_config, "maxNodeChildren", 16);
    if (context_reduce_callback)
    {
        config.writer.reduce = std::bind(&ContextReduceCallback::call, context_reduce_callback.get(), std::placeholders::_1, std::placeholders::_2);
    }

    try
    {
        // new: will be deleted in kvdb_close
        ContextKvDb *context = new ContextKvDb(path_str, config, std::move(context_reduce_callback));
        jlong ptr = static_cast<jlong>(reinterpret_cast<size_t>(context));
        return ptr;
    }
    catch (const std::exception &e)
    {
        jni_throw_exception(env, "java/lang/Exception", e.what());
        return 0;
    }
}

JNIEXPORT void JNICALL Java_io_pinesw_ninedb_KvDatabase_kvdb_1close(JNIEnv *env, jclass, jlong j_handle_db)
{
    ContextKvDb *context = reinterpret_cast<ContextKvDb *>(j_handle_db);

    try
    {
        context->kvdb.flush();
    }
    catch (const std::exception &e)
    {
        jni_throw_exception(env, "java/lang/Exception", e.what());
    }

    delete context;
}

JNIEXPORT void JNICALL Java_io_pinesw_ninedb_KvDatabase_kvdb_1add(JNIEnv *env, jclass, jlong j_handle_db, jbyteArray j_key, jbyteArray j_value)
{
    ContextKvDb *context = reinterpret_cast<ContextKvDb *>(j_handle_db);
    std::string key = jni_byte_array_to_string(env, j_key);
    std::string value = jni_byte_array_to_string(env, j_value);

    try
    {
        context->kvdb.add(key, value);
    }
    catch (const std::exception &e)
    {
        jni_throw_exception(env, "java/lang/Exception", e.what());
    }
}

JNIEXPORT jbyteArray JNICALL Java_io_pinesw_ninedb_KvDatabase_kvdb_1get(JNIEnv *env, jclass, jlong j_handle_db, jbyteArray j_key)
{
    ContextKvDb *context = reinterpret_cast<ContextKvDb *>(j_handle_db);
    std::string key = jni_byte_array_to_string(env, j_key);

    try
    {
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
    catch (const std::exception &e)
    {
        jni_throw_exception(env, "java/lang/Exception", e.what());
        return nullptr;
    }
}

JNIEXPORT jobject JNICALL Java_io_pinesw_ninedb_KvDatabase_kvdb_1at(JNIEnv *env, jclass, jlong j_handle_db, jlong at)
{
    ContextKvDb *context = reinterpret_cast<ContextKvDb *>(j_handle_db);

    try
    {
        std::optional<std::pair<std::string_view, std::string_view>> kv = context->kvdb.at(at);
        if (!kv.has_value())
        {
            return nullptr;
        }

        jclass cls_KeyValuePair = env->FindClass("io/pinesw/ninedb/KeyValuePair");
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
    catch (const std::exception &e)
    {
        jni_throw_exception(env, "java/lang/Exception", e.what());
        return nullptr;
    }
}

JNIEXPORT jobjectArray JNICALL Java_io_pinesw_ninedb_KvDatabase_kvdb_1traverse(JNIEnv *env, jclass, jlong j_handle_db, jobject obj_predicate)
{
    ContextKvDb *context = reinterpret_cast<ContextKvDb *>(j_handle_db);

    jclass cls_Predicate = env->GetObjectClass(obj_predicate);
    jmethodID mid_test = env->GetMethodID(cls_Predicate, "test", "(Ljava/lang/Object;)Z");

    try
    {
        std::vector<std::string_view> accumulator;
        context->kvdb.traverse(
            [=](std::string_view value)
            {
                jbyteArray j_value = env->NewByteArray(value.size());
                env->SetByteArrayRegion(j_value, 0, value.size(), reinterpret_cast<const jbyte *>(value.data()));
                return env->CallBooleanMethod(obj_predicate, mid_test, j_value);
            },
            accumulator);

        jobjectArray result = env->NewObjectArray(accumulator.size(), env->FindClass("[B"), nullptr);
        for (size_t i = 0; i < accumulator.size(); i++)
        {
            jbyteArray j_value = env->NewByteArray(accumulator[i].size());
            env->SetByteArrayRegion(j_value, 0, accumulator[i].size(), reinterpret_cast<const jbyte *>(accumulator[i].data()));
            env->SetObjectArrayElement(result, i, j_value);
        }

        return result;
    }
    catch (const std::exception &e)
    {
        jni_throw_exception(env, "java/lang/Exception", e.what());
        return nullptr;
    }
}

JNIEXPORT void JNICALL Java_io_pinesw_ninedb_KvDatabase_kvdb_1flush(JNIEnv *env, jclass, jlong j_handle_db)
{
    ContextKvDb *context = reinterpret_cast<ContextKvDb *>(j_handle_db);

    try
    {
        context->kvdb.flush();
    }
    catch (const std::exception &e)
    {
        jni_throw_exception(env, "java/lang/Exception", e.what());
    }
}

JNIEXPORT void JNICALL Java_io_pinesw_ninedb_KvDatabase_kvdb_1compact(JNIEnv *env, jclass, jlong j_handle_db)
{
    ContextKvDb *context = reinterpret_cast<ContextKvDb *>(j_handle_db);

    try
    {
        context->kvdb.compact();
    }
    catch (const std::exception &e)
    {
        jni_throw_exception(env, "java/lang/Exception", e.what());
    }
}

JNIEXPORT jlong JNICALL Java_io_pinesw_ninedb_KvDatabase_kvdb_1begin(JNIEnv *env, jclass, jlong j_handle_db)
{
    ContextKvDb *context = reinterpret_cast<ContextKvDb *>(j_handle_db);

    try
    {
        // new: will be deleted in itr_close
        ContextKvIterator *context_itr = new ContextKvIterator(context->kvdb.begin());
        jlong ptr = static_cast<jlong>(reinterpret_cast<size_t>(context_itr));
        return ptr;
    }
    catch (const std::exception &e)
    {
        jni_throw_exception(env, "java/lang/Exception", e.what());
        return 0;
    }
}

JNIEXPORT jlong JNICALL Java_io_pinesw_ninedb_KvDatabase_kvdb_1seek_1key(JNIEnv *env, jclass, jlong j_handle_db, jbyteArray j_key)
{
    ContextKvDb *context = reinterpret_cast<ContextKvDb *>(j_handle_db);
    std::string key = jni_byte_array_to_string(env, j_key);

    try
    {
        // new: will be deleted in itr_close
        ContextKvIterator *context_itr = new ContextKvIterator(context->kvdb.seek(key));
        jlong ptr = static_cast<jlong>(reinterpret_cast<size_t>(context_itr));
        return ptr;
    }
    catch (const std::exception &e)
    {
        jni_throw_exception(env, "java/lang/Exception", e.what());
        return 0;
    }
}

JNIEXPORT jlong JNICALL Java_io_pinesw_ninedb_KvDatabase_kvdb_1seek_1index(JNIEnv *env, jclass, jlong j_handle_db, jlong at)
{
    ContextKvDb *context = reinterpret_cast<ContextKvDb *>(j_handle_db);

    try
    {
        // new: will be deleted in itr_close
        ContextKvIterator *context_itr = new ContextKvIterator(context->kvdb.seek(at));
        jlong ptr = static_cast<jlong>(reinterpret_cast<size_t>(context_itr));
        return ptr;
    }
    catch (const std::exception &e)
    {
        jni_throw_exception(env, "java/lang/Exception", e.what());
        return 0;
    }
}

JNIEXPORT void JNICALL Java_io_pinesw_ninedb_KvDbIterator_itr_1close(JNIEnv *env, jclass, jlong j_handle_iterator)
{
    ContextKvIterator *context = reinterpret_cast<ContextKvIterator *>(j_handle_iterator);
    delete context;
}

JNIEXPORT void JNICALL Java_io_pinesw_ninedb_KvDbIterator_itr_1next(JNIEnv *env, jclass, jlong j_handle_iterator)
{
    ContextKvIterator *context = reinterpret_cast<ContextKvIterator *>(j_handle_iterator);

    try
    {
        context->itr.next();
    }
    catch (const std::exception &e)
    {
        jni_throw_exception(env, "java/lang/Exception", e.what());
    }
}

JNIEXPORT jboolean JNICALL Java_io_pinesw_ninedb_KvDbIterator_itr_1has_1next(JNIEnv *env, jclass, jlong j_handle_iterator)
{
    ContextKvIterator *context = reinterpret_cast<ContextKvIterator *>(j_handle_iterator);

    try
    {
        return !context->itr.is_end();
    }
    catch (const std::exception &e)
    {
        jni_throw_exception(env, "java/lang/Exception", e.what());
        return false;
    }
}

JNIEXPORT jbyteArray JNICALL Java_io_pinesw_ninedb_KvDbIterator_itr_1get_1key(JNIEnv *env, jclass, jlong j_handle_iterator)
{
    ContextKvIterator *context = reinterpret_cast<ContextKvIterator *>(j_handle_iterator);

    try
    {
        std::string_view key = context->itr.get_key();
        jbyteArray result = env->NewByteArray(key.size());
        env->SetByteArrayRegion(result, 0, key.size(), reinterpret_cast<const jbyte *>(key.data()));
        return result;
    }
    catch (const std::exception &e)
    {
        jni_throw_exception(env, "java/lang/Exception", e.what());
        return nullptr;
    }
}

JNIEXPORT jbyteArray JNICALL Java_io_pinesw_ninedb_KvDbIterator_itr_1get_1value(JNIEnv *env, jclass, jlong j_handle_iterator)
{
    ContextKvIterator *context = reinterpret_cast<ContextKvIterator *>(j_handle_iterator);

    try
    {
        std::string_view value = context->itr.get_value();
        jbyteArray result = env->NewByteArray(value.size());
        env->SetByteArrayRegion(result, 0, value.size(), reinterpret_cast<const jbyte *>(value.data()));
        return result;
    }
    catch (const std::exception &e)
    {
        jni_throw_exception(env, "java/lang/Exception", e.what());
        return nullptr;
    }
}

JNIEXPORT jlong JNICALL Java_io_pinesw_ninedb_HrDatabase_hrdb_1open(JNIEnv *env, jclass, jstring str_path, jobject obj_config)
{
    const char *path = env->GetStringUTFChars(str_path, nullptr);
    std::string path_str(path);
    env->ReleaseStringUTFChars(str_path, path);

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
    config.writer.max_node_children = jni_object_get_property_integer_boxed(env, obj_config, "maxNodeChildren", 16);

    try
    {
        // new: will be deleted in hrdb_close
        ContextHrDb *context = new ContextHrDb(path_str, config);
        jlong ptr = static_cast<jlong>(reinterpret_cast<size_t>(context));
        return ptr;
    }
    catch (const std::exception &e)
    {
        jni_throw_exception(env, "java/lang/Exception", e.what());
        return 0;
    }
}

JNIEXPORT void JNICALL Java_io_pinesw_ninedb_HrDatabase_hrdb_1close(JNIEnv *env, jclass, jlong j_handle_db)
{
    ContextHrDb *context = reinterpret_cast<ContextHrDb *>(j_handle_db);

    try
    {
        context->hrdb.flush();
    }
    catch (const std::exception &e)
    {
        jni_throw_exception(env, "java/lang/Exception", e.what());
    }

    delete context;
}

JNIEXPORT void JNICALL Java_io_pinesw_ninedb_HrDatabase_hrdb_1add(JNIEnv *env, jclass, jlong j_handle_db, jlong x0, jlong y0, jlong x1, jlong y1, jbyteArray j_value)
{
    ContextHrDb *context = reinterpret_cast<ContextHrDb *>(j_handle_db);
    std::string value = jni_byte_array_to_string(env, j_value);

    try
    {
        context->hrdb.add(x0, y0, x1, y1, value);
    }
    catch (const std::exception &e)
    {
        jni_throw_exception(env, "java/lang/Exception", e.what());
    }
}

JNIEXPORT jobjectArray JNICALL Java_io_pinesw_ninedb_HrDatabase_hrdb_1search(JNIEnv *env, jclass, jlong j_handle_db, jlong x0, jlong y0, jlong x1, jlong y1)
{
    ContextHrDb *context = reinterpret_cast<ContextHrDb *>(j_handle_db);

    try
    {
        std::vector<std::string_view> values = context->hrdb.search(x0, y0, x1, y1);
        jobjectArray result = env->NewObjectArray(values.size(), env->FindClass("[B"), nullptr);
        for (size_t i = 0; i < values.size(); i++)
        {
            jbyteArray j_value = env->NewByteArray(values[i].size());
            env->SetByteArrayRegion(j_value, 0, values[i].size(), reinterpret_cast<const jbyte *>(values[i].data()));
            env->SetObjectArrayElement(result, i, j_value);
        }
        return result;
    }
    catch (const std::exception &e)
    {
        jni_throw_exception(env, "java/lang/Exception", e.what());
        return nullptr;
    }
}

JNIEXPORT void JNICALL Java_io_pinesw_ninedb_HrDatabase_hrdb_1flush(JNIEnv *env, jclass, jlong j_handle_db)
{
    ContextHrDb *context = reinterpret_cast<ContextHrDb *>(j_handle_db);

    try
    {
        context->hrdb.flush();
    }
    catch (const std::exception &e)
    {
        jni_throw_exception(env, "java/lang/Exception", e.what());
    }
}

JNIEXPORT void JNICALL Java_io_pinesw_ninedb_HrDatabase_hrdb_1compact(JNIEnv *env, jclass, jlong j_handle_db)
{
    ContextHrDb *context = reinterpret_cast<ContextHrDb *>(j_handle_db);

    try
    {
        context->hrdb.compact();
    }
    catch (const std::exception &e)
    {
        jni_throw_exception(env, "java/lang/Exception", e.what());
    }
}
