/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class io_woutervh_ninedb_HrDatabase */

#ifndef _Included_io_woutervh_ninedb_HrDatabase
#define _Included_io_woutervh_ninedb_HrDatabase
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     io_woutervh_ninedb_HrDatabase
 * Method:    hrdb_open
 * Signature: (Ljava/lang/String;Lio/woutervh/ninedb/DbConfig;)J
 */
JNIEXPORT jlong JNICALL Java_io_woutervh_ninedb_HrDatabase_hrdb_1open
  (JNIEnv *, jclass, jstring, jobject);

/*
 * Class:     io_woutervh_ninedb_HrDatabase
 * Method:    hrdb_close
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_io_woutervh_ninedb_HrDatabase_hrdb_1close
  (JNIEnv *, jclass, jlong);

/*
 * Class:     io_woutervh_ninedb_HrDatabase
 * Method:    hrdb_add
 * Signature: (JJJJJ[B)V
 */
JNIEXPORT void JNICALL Java_io_woutervh_ninedb_HrDatabase_hrdb_1add
  (JNIEnv *, jclass, jlong, jlong, jlong, jlong, jlong, jbyteArray);

/*
 * Class:     io_woutervh_ninedb_HrDatabase
 * Method:    hrdb_search
 * Signature: (JJJJJ)[[B
 */
JNIEXPORT jobjectArray JNICALL Java_io_woutervh_ninedb_HrDatabase_hrdb_1search
  (JNIEnv *, jclass, jlong, jlong, jlong, jlong, jlong);

/*
 * Class:     io_woutervh_ninedb_HrDatabase
 * Method:    hrdb_flush
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_io_woutervh_ninedb_HrDatabase_hrdb_1flush
  (JNIEnv *, jclass, jlong);

/*
 * Class:     io_woutervh_ninedb_HrDatabase
 * Method:    hrdb_compact
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_io_woutervh_ninedb_HrDatabase_hrdb_1compact
  (JNIEnv *, jclass, jlong);

#ifdef __cplusplus
}
#endif
#endif
