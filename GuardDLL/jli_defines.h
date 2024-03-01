/*
 * Copyright (c) 1995, 2023, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */
#pragma once
#include "jni.h"
JNIEXPORT int JNICALL
JLI_Launch(int argc, char** argv,              /* main argc, argv */
    int jargc, const char** jargv,          /* java args */
    int appclassc, const char** appclassv,  /* app classpath */
    const char* fullversion,                /* full version defined */
    const char* dotversion,                 /* UNUSED dot version defined */
    const char* pname,                      /* program name */
    const char* lname,                      /* launcher name */
    jboolean javaargs,                      /* JAVA_ARGS */
    jboolean cpwildcard,                    /* classpath wildcard*/
    jboolean javaw,                         /* windows-only javaw */
    jint ergo                               /* unused */
);
JNIEXPORT void JNICALL
JLI_CmdToArgs(char* cmdline);
typedef struct {
    char* arg;
    jboolean has_wildcard;
} StdArg;
JNIEXPORT StdArg* JNICALL
JLI_GetStdArgs();

JNIEXPORT int JNICALL
JLI_GetStdArgc();

JNIEXPORT void JNICALL
JLI_InitArgProcessing(jboolean hasJavaArgs, jboolean disableArgFile);