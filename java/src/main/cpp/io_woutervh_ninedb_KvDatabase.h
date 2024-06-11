/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class io_pinesw_ninedb_KvDatabase */

#ifndef _Included_io_pinesw_ninedb_KvDatabase
#define _Included_io_pinesw_ninedb_KvDatabase
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     io_pinesw_ninedb_KvDatabase
 * Method:    kvdb_open
 * Signature: (Ljava/lang/String;Lio/pinesw/ninedb/DbConfig;)J
 */
JNIEXPORT jlong JNICALL Java_io_pinesw_ninedb_KvDatabase_kvdb_1open
  (JNIEnv *, jclass, jstring, jobject);

/*
 * Class:     io_pinesw_ninedb_KvDatabase
 * Method:    kvdb_close
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_io_pinesw_ninedb_KvDatabase_kvdb_1close
  (JNIEnv *, jclass, jlong);

/*
 * Class:     io_pinesw_ninedb_KvDatabase
 * Method:    kvdb_add
 * Signature: (J[B[B)V
 */
JNIEXPORT void JNICALL Java_io_pinesw_ninedb_KvDatabase_kvdb_1add
  (JNIEnv *, jclass, jlong, jbyteArray, jbyteArray);

/*
 * Class:     io_pinesw_ninedb_KvDatabase
 * Method:    kvdb_get
 * Signature: (J[B)[B
 */
JNIEXPORT jbyteArray JNICALL Java_io_pinesw_ninedb_KvDatabase_kvdb_1get
  (JNIEnv *, jclass, jlong, jbyteArray);

/*
 * Class:     io_pinesw_ninedb_KvDatabase
 * Method:    kvdb_at
 * Signature: (JJ)Lio/pinesw/ninedb/KeyValuePair;
 */
JNIEXPORT jobject JNICALL Java_io_pinesw_ninedb_KvDatabase_kvdb_1at
  (JNIEnv *, jclass, jlong, jlong);

/*
 * Class:     io_pinesw_ninedb_KvDatabase
 * Method:    kvdb_traverse
 * Signature: (JLjava/util/function/Predicate;)[[B
 */
JNIEXPORT jobjectArray JNICALL Java_io_pinesw_ninedb_KvDatabase_kvdb_1traverse
  (JNIEnv *, jclass, jlong, jobject);

/*
 * Class:     io_pinesw_ninedb_KvDatabase
 * Method:    kvdb_flush
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_io_pinesw_ninedb_KvDatabase_kvdb_1flush
  (JNIEnv *, jclass, jlong);

/*
 * Class:     io_pinesw_ninedb_KvDatabase
 * Method:    kvdb_compact
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_io_pinesw_ninedb_KvDatabase_kvdb_1compact
  (JNIEnv *, jclass, jlong);

/*
 * Class:     io_pinesw_ninedb_KvDatabase
 * Method:    kvdb_begin
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_io_pinesw_ninedb_KvDatabase_kvdb_1begin
  (JNIEnv *, jclass, jlong);

/*
 * Class:     io_pinesw_ninedb_KvDatabase
 * Method:    kvdb_seek_key
 * Signature: (J[B)J
 */
JNIEXPORT jlong JNICALL Java_io_pinesw_ninedb_KvDatabase_kvdb_1seek_1key
  (JNIEnv *, jclass, jlong, jbyteArray);

/*
 * Class:     io_pinesw_ninedb_KvDatabase
 * Method:    kvdb_seek_index
 * Signature: (JJ)J
 */
JNIEXPORT jlong JNICALL Java_io_pinesw_ninedb_KvDatabase_kvdb_1seek_1index
  (JNIEnv *, jclass, jlong, jlong);

#ifdef __cplusplus
}
#endif
#endif
