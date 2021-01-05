#pragma once
#include "jni.h"
namespace GuardLibraryDefines
{
	typedef jclass
		(JNICALL *JVM_DefineClassWithSource)(JNIEnv* env, const char* name, jobject loader,

			const jbyte* buf, jsize len, jobject pd,

			const char* source) ;
}