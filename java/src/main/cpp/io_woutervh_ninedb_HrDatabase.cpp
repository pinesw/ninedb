#include <jni.h>
#include <iostream>

#include "./io_woutervh_ninedb_HrDatabase.h"

#include "../../../../src/ninedb/ninedb.hpp"

struct KvDbContext
{
    ninedb::KvDb kvdb;

    KvDbContext(const std::string &path, const ninedb::Config &config)
        : kvdb(ninedb::KvDb::open(path, config)) {}
};

JNIEXPORT jlong JNICALL Java_io_woutervh_ninedb_HrDatabase_db_1open(JNIEnv *env, jclass, jstring str_path, jobject obj_config)
{
    const char *path = env->GetStringUTFChars(str_path, nullptr);

    ninedb::Config config;

    jclass cls_Boolean = env->FindClass("java/lang/Boolean");
    jmethodID mid_booleanValue = env->GetMethodID(cls_Boolean, "booleanValue", "()Z");

    jclass cls_db_config = env->GetObjectClass(obj_config);
    jfieldID fid_createIfMissing = env->GetFieldID(cls_db_config, "createIfMissing", "Ljava/lang/Boolean;");
    jobject obj_createIfMissing = env->GetObjectField(obj_config, fid_createIfMissing);
    if (obj_createIfMissing != NULL)
    {
        jboolean bln_createIfMissing = env->CallBooleanMethod(obj_createIfMissing, mid_booleanValue);
        config.create_if_missing = bln_createIfMissing;
    }
    else
    {
        config.create_if_missing = true;
    }

    // TODO: Implement the rest of the config

    // config.create_if_missing = napi_object_get_property_boolean(env, config_obj, "createIfMissing", true);
    // config.delete_if_exists = napi_object_get_property_boolean(env, config_obj, "deleteIfExists", false);
    // config.error_if_exists = napi_object_get_property_boolean(env, config_obj, "errorIfExists", false);
    // config.max_buffer_size = napi_object_get_property_uint32(env, config_obj, "maxBufferSize", 1 << 22);
    // config.max_level_count = napi_object_get_property_uint32(env, config_obj, "maxLevelCount", 10);
    // config.reader.internal_node_cache_size = napi_object_get_property_uint32(env, config_obj, "internalNodeCacheSize", 64);
    // config.reader.leaf_node_cache_size = napi_object_get_property_uint32(env, config_obj, "leafNodeCacheSize", 8);
    // config.writer.enable_compression = napi_object_get_property_boolean(env, config_obj, "enableCompression", true);
    // config.writer.enable_prefix_encoding = napi_object_get_property_boolean(env, config_obj, "enablePrefixEncoding", true);
    // config.writer.initial_pbt_size = napi_object_get_property_uint32(env, config_obj, "initialPbtSize", 1 << 23);
    // config.writer.max_node_entries = napi_object_get_property_uint32(env, config_obj, "maxNodeEntries", 16);
    // config.writer.reduce = nullptr;

    env->ReleaseStringUTFChars(str_path, path);

    KvDbContext *context = new KvDbContext(path, config);
    jlong ptr = static_cast<jlong>(reinterpret_cast<size_t>(context));
    return ptr;
}

JNIEXPORT void JNICALL Java_io_woutervh_ninedb_HrDatabase_db_1close(JNIEnv *, jclass, jlong)
{
    KvDbContext *context = reinterpret_cast<KvDbContext *>(db_handle);
    context->kvdb.flush();
    delete context;
}

JNIEXPORT void JNICALL Java_io_woutervh_ninedb_HrDatabase_kvdb_1add(JNIEnv *, jclass, jlong, jbyteArray, jbyteArray)
{
    // TODO: Implement
}

JNIEXPORT jbyteArray JNICALL Java_io_woutervh_ninedb_HrDatabase_kvdb_1get(JNIEnv *, jclass, jlong, jbyteArray)
{
    // TODO: Implement

    // NewByteArray
    jbyteArray result = nullptr;
    return result;
}

JNIEXPORT jobject JNICALL Java_io_woutervh_ninedb_HrDatabase_kvdb_1at(JNIEnv *env, jclass, jlong db_handle, jlong)
{
    // TODO: Implement

    jbyteArray key = nullptr;
    jbyteArray value = nullptr;

    jclass jclass_KeyValuePair = env->FindClass("io/woutervh/ninedb/KeyValuePair");

    if (jclass_KeyValuePair == nullptr)
    {
        jclass Exception = env->FindClass("java/lang/Exception");
        env->ThrowNew(Exception, "Cannot find KeyValuePair class.");
    }

    jmethodID jmethodID_KeyValuePair = env->GetMethodID(jclass_KeyValuePair, "<init>", "([B[B)V");

    return env->NewObject(jclass_KeyValuePair, jmethodID_KeyValuePair, key, value);
}

JNIEXPORT void JNICALL Java_io_woutervh_ninedb_HrDatabase_kvdb_1flush(JNIEnv *, jclass, jlong db_handle)
{
    KvDbContext *context = reinterpret_cast<KvDbContext *>(db_handle);
    context->kvdb.flush();
}

JNIEXPORT void JNICALL Java_io_woutervh_ninedb_HrDatabase_kvdb_1compact(JNIEnv *, jclass, jlong)
{
    KvDbContext *context = reinterpret_cast<KvDbContext *>(db_handle);
    context->kvdb.compact();
}
