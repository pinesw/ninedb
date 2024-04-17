#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <jni.h>

bool jni_object_get_property_boolean(JNIEnv *env, jobject obj, const char *property, bool default_value)
{
    jclass cls = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(cls, property, "Z");
    if (fid != NULL)
    {
        return env->GetBooleanField(obj, fid);
    }
    return default_value;
}

bool jni_object_get_property_boolean_boxed(JNIEnv *env, jobject obj, const char *property, bool default_value)
{
    jclass cls = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(cls, property, "Ljava/lang/Boolean;");
    if (fid != NULL)
    {
        jobject obj_value = env->GetObjectField(obj, fid);
        if (obj_value != NULL)
        {
            jclass cls_Boolean = env->FindClass("java/lang/Boolean");
            jmethodID mid_booleanValue = env->GetMethodID(cls_Boolean, "booleanValue", "()Z");
            return env->CallBooleanMethod(obj_value, mid_booleanValue);
        }
    }
    return default_value;
}

int32_t jni_object_get_property_integer(JNIEnv *env, jobject obj, const char *property, int default_value)
{
    jclass cls = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(cls, property, "I");
    if (fid != NULL)
    {
        return env->GetIntField(obj, fid);
    }
    return default_value;
}

int32_t jni_object_get_property_integer_boxed(JNIEnv *env, jobject obj, const char *property, int default_value)
{
    jclass cls = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(cls, property, "Ljava/lang/Integer;");
    if (fid != NULL)
    {
        jobject obj_value = env->GetObjectField(obj, fid);
        if (obj_value != NULL)
        {
            jclass cls_Integer = env->FindClass("java/lang/Integer");
            jmethodID mid_intValue = env->GetMethodID(cls_Integer, "intValue", "()I");
            return env->CallIntMethod(obj_value, mid_intValue);
        }
    }
    return default_value;
}

jobject jni_object_get_property_object(JNIEnv *env, jobject obj, const char *property, const char *signature)
{
    jclass cls = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(cls, property, signature);
    if (fid != NULL)
    {
        return env->GetObjectField(obj, fid);
    }
    return NULL;
}

std::string jni_byte_array_to_string(JNIEnv *env, jbyteArray j_byte_array)
{
    jsize byte_array_size = env->GetArrayLength(j_byte_array);
    jbyte *byte_array_data = env->GetByteArrayElements(j_byte_array, nullptr);
    std::string byte_array_string(reinterpret_cast<char *>(byte_array_data), byte_array_size);
    env->ReleaseByteArrayElements(j_byte_array, byte_array_data, JNI_ABORT);
    return byte_array_string;
}
