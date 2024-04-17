#pragma once

#include <vector>
#include <string>
#include <memory>

#include <jni.h>

#include "../../../../src/ninedb/ninedb.hpp"

static std::string getClassName(JNIEnv *env, jobject entity, jclass clazz)
{
    jmethodID mid = env->GetMethodID(clazz, "getClass", "()Ljava/lang/Class;");
    jobject clsObj = env->CallObjectMethod(entity, mid);
    jclass clazzz = env->GetObjectClass(clsObj);
    mid = env->GetMethodID(clazzz, "getName", "()Ljava/lang/String;");
    jstring strObj = (jstring)env->CallObjectMethod(clsObj, mid);

    const char* str = env->GetStringUTFChars(strObj, NULL);
    std::string res(str);

    env->ReleaseStringUTFChars(strObj, str);

    return res;
}

struct ContextReduceCallback
{
private:
    JNIEnv *env;
    jobject obj_reduce;
    jobject obj_reduce_global_ref;

public:
    ContextReduceCallback(JNIEnv *env, jobject obj_reduce)
        : env(env), obj_reduce(obj_reduce), obj_reduce_global_ref(env->NewGlobalRef(obj_reduce))
    {
        std::cerr << "ContextReduceCallback::ContextReduceCallback" << std::endl;

        jclass cls_Function = env->GetObjectClass(obj_reduce);
        std::string cls_name = getClassName(env, obj_reduce, cls_Function);
        std::cerr << "ContextReduceCallback::ContextReduceCallback: cls_name=" << cls_name << std::endl;
    }

    ~ContextReduceCallback()
    {
        std::cerr << "ContextReduceCallback::~ContextReduceCallback" << std::endl;

        env->DeleteGlobalRef(obj_reduce_global_ref);
    }

    void call(const std::vector<std::string> &values, std::string &reduced_value)
    {
        std::cerr << "ContextReduceCallback::call" << std::endl;

        jclass cls_ByteArray = env->FindClass("[B");
        jobjectArray obj_values = env->NewObjectArray(values.size(), cls_ByteArray, nullptr);
        for (size_t i = 0; i < values.size(); i++)
        {
            jbyteArray obj_value = env->NewByteArray(values[i].size());
            env->SetByteArrayRegion(obj_value, 0, values[i].size(), reinterpret_cast<const jbyte *>(values[i].data()));
            env->SetObjectArrayElement(obj_values, i, obj_value);
        }

        std::cerr << "ContextReduceCallback::call: getting apply method" << std::endl;

        jclass cls_Function = env->GetObjectClass(obj_reduce);
        // jclass cls_Function = env->FindClass("java/util/function/Function");
        jmethodID mid_apply = env->GetMethodID(cls_Function, "apply", "([[B)[B");
        // jmethodID mid_apply = env->GetMethodID(cls_Function, "apply", "(Ljava/lang/Object;)Ljava/lang/Object;");

        std::cerr << "ContextReduceCallback::call: calling apply" << std::endl;

        jobject obj_return = env->CallObjectMethod(obj_reduce, mid_apply, obj_values);

        // jclass cls_debug = env->GetObjectClass(obj_return);
        // std::string cls_name = getClassName(env, obj_return, cls_debug);
        // std::cerr << "ContextReduceCallback::call: cls_name=" << cls_name << std::endl;

        std::cerr << "ContextReduceCallback::call: converting return value" << std::endl;

        jbyteArray obj_reduced_value = reinterpret_cast<jbyteArray>(obj_return);

        std::cerr << "ContextReduceCallback::call: making reduced value" << std::endl;

        jsize reduced_value_size = env->GetArrayLength(obj_reduced_value);

        std::cerr << "ContextReduceCallback::call: reduced_value_size=" << reduced_value_size << std::endl;

        jbyte *reduced_value_data = env->GetByteArrayElements(obj_reduced_value, nullptr);

        std::cerr << "ContextReduceCallback::call: reduced_value_data=" << reduced_value_data << std::endl;

        reduced_value = std::string(reinterpret_cast<char *>(reduced_value_data), reduced_value_size);
    }
};

struct KvDbContext
{
public:
    ninedb::KvDb kvdb;

private:
    std::unique_ptr<ContextReduceCallback> context_reduce_callback;

public:
    KvDbContext(const std::string &path, const ninedb::Config &config, std::unique_ptr<ContextReduceCallback> &&context_reduce_callback)
        : kvdb(ninedb::KvDb::open(path, config)), context_reduce_callback(std::move(context_reduce_callback)) {}
};
