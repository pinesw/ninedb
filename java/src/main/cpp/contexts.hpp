#pragma once

#include <vector>
#include <string>
#include <memory>

#include <jni.h>

#include "../../../../src/ninedb/ninedb.hpp"

struct ContextKvDb;
struct ContextHrDb;
struct ContextKvIterator;
struct ContextReduceCallback;

struct ContextKvIterator
{
public:
    ninedb::Iterator itr;

    ContextKvIterator(ninedb::Iterator &&itr) : itr(std::move(itr)) {}
};

struct ContextReduceCallback
{
private:
    JNIEnv *env;
    jobject obj_reduce_global_ref;

public:
    ContextReduceCallback(JNIEnv *env, jobject obj_reduce)
        : env(env), obj_reduce_global_ref(env->NewGlobalRef(obj_reduce)) {}

    ~ContextReduceCallback()
    {
        env->DeleteGlobalRef(obj_reduce_global_ref);
    }

    void call(const std::vector<std::string> &values, std::string &reduced_value)
    {
        jclass cls_ByteArray = env->FindClass("[B");
        jobjectArray obj_values = env->NewObjectArray(values.size(), cls_ByteArray, nullptr);
        for (size_t i = 0; i < values.size(); i++)
        {
            jbyteArray obj_value = env->NewByteArray(values[i].size());
            env->SetByteArrayRegion(obj_value, 0, values[i].size(), reinterpret_cast<const jbyte *>(values[i].data()));
            env->SetObjectArrayElement(obj_values, i, obj_value);
        }

        jclass cls_Function = env->FindClass("java/util/function/Function");
        jmethodID mid_apply = env->GetMethodID(cls_Function, "apply", "(Ljava/lang/Object;)Ljava/lang/Object;");
        jobject obj_return = env->CallObjectMethod(obj_reduce_global_ref, mid_apply, obj_values);

        if (obj_return == nullptr)
        {
            throw std::runtime_error("Reduce function returned null.");
        }

        jbyteArray obj_reduced_value = reinterpret_cast<jbyteArray>(obj_return);
        jsize reduced_value_size = env->GetArrayLength(obj_reduced_value);
        jbyte *reduced_value_data = env->GetByteArrayElements(obj_reduced_value, nullptr);

        reduced_value = std::string(reinterpret_cast<char *>(reduced_value_data), reduced_value_size);
    }
};

struct ContextKvDb
{
public:
    ninedb::KvDb kvdb;

private:
    std::unique_ptr<ContextReduceCallback> context_reduce_callback;

public:
    ContextKvDb(const std::string &path, const ninedb::Config &config, std::unique_ptr<ContextReduceCallback> &&context_reduce_callback)
        : kvdb(ninedb::KvDb::open(path, config)), context_reduce_callback(std::move(context_reduce_callback)) {}
};

struct ContextHrDb {
    
};
