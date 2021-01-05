// JavaWrapper.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include "GravitGuardAPI.h"
#include "win32/jni_md.h"
#include "jni.h"
#include <windows.h>
#include "LoggerWrapper.h"
std::string get_env(LPCSTR s)
{
	std::string appdata(1024, 'X');

	DWORD len = GetEnvironmentVariableA(s, &appdata[0], appdata.size());
	appdata.resize(len);
	return appdata;
}
std::string get_dll_path()
{
	std::string appdata(1024, 'X');

	DWORD len = GetDllDirectoryA(appdata.size(), &appdata[0]);
	appdata.resize(len);
	return appdata;
}
char* replace_mainclass(char* text, int len)
{
	for (int i = 0; i < len; ++i)
	{
		if (text[i] == '.') text[i] = '/';
	}
	return text;
}
char* replace_mainclass(char* text)
{
	return replace_mainclass(text, strlen(text));
}
LoggerWrapper logger;
int main(int argc, char** argv)
{
	initGuard();
	GravitGuardAPI api = getGuardAPI();
	logger.init(api);
	logger.debug("JavaWrapper", "Starting Java Virtual Machine");
	std::string javahome = get_env("JAVA_HOME");
	std::string jvmdll_path = javahome + "\\bin\\server\\jvm.dll"; //May be JDK
	std::string jvmdll_path2 = javahome + "\\jre\\bin\\server\\jvm.dll"; //May be JRE
	std::string jvmdll_path3 = javahome + "\\jre\\bin\\client\\jvm.dll"; //May be JDK
	std::string jvmdll_path4 = javahome + "\\bin\\client\\jvm.dll"; //May be JRE
	std::cout << jvmdll_path << std::endl;
	std::string dllpath = get_dll_path();
	dllpath += (javahome + "\\bin");
	//dllpath += (javahome + "\\bin\\server");
	SetDllDirectoryA(dllpath.c_str());
	HINSTANCE hinstLib = LoadLibraryA(jvmdll_path.c_str());
	int Error = 0;
	if (hinstLib == NULL) {
		Error = GetLastError();
		hinstLib = LoadLibraryA(jvmdll_path2.c_str());
		logger.trace("JavaWrapper", "Find jvm.dll " + jvmdll_path + " error code " + std::to_string(Error));
	}
	if (hinstLib == NULL) {
		Error = GetLastError();
		hinstLib = LoadLibraryA(jvmdll_path3.c_str());
		logger.trace("JavaWrapper", "Find jvm.dll " + jvmdll_path2 + " error code " + std::to_string(Error));
	}
	if (hinstLib == NULL) {
		Error = GetLastError();
		hinstLib = LoadLibraryA(jvmdll_path4.c_str());
		logger.trace("JavaWrapper", "Find jvm.dll " + jvmdll_path3 + " error code " + std::to_string(Error));
	}
	if (hinstLib == NULL) {
		Error = GetLastError();
		logger.trace("JavaWrapper", "Find jvm.dll " + jvmdll_path4 + " error code " + std::to_string(Error));
		MessageBoxA(NULL, "jvm.dll not found. Please contact administrator or launcher developer", "Java Wrapper", 0);
		exit(-98);
	}
	logger.debug("JavaWrapper", "Start GravitGuard");
	api.licenseData(get_env("GUARD_SIGNATURE"));
	api.init();
	if (argc == 1 || argc == 0)
	{
		logger.crash("JavaWrapper", "Arguments not found");
		return 0;
	}
	JNIEnv* env = nullptr;
	JavaVM* jvm = nullptr;
	JavaVMInitArgs vm_args;
	int jvm_args = 0;
	JavaVMOption* options = new JavaVMOption[argc];
	for (int i = 0; i < argc; ++i)
	{
		if (argv[i][0] != '-' && i != 0) {
			jvm_args = i;
			break;
		}
		options[i].optionString = argv[i];
		std::string str(argv[i]);
		if (str.substr(0, 15) == "-Xbootclasspath")
		{
			logger.crash("JavaWrapper", "BootClassPath not support");
			return 0;
		}
	}
	vm_args.version = JNI_VERSION_1_8;
	typedef jint(JNICALL* PtrGetDefaultJavaVMInitArgs)(void*);
	PtrGetDefaultJavaVMInitArgs ptrGetDefaultJavaVMInitArgs = (PtrGetDefaultJavaVMInitArgs)GetProcAddress(hinstLib, "JNI_GetDefaultJavaVMInitArgs");
	ptrGetDefaultJavaVMInitArgs(&vm_args);
	vm_args.nOptions = jvm_args;
	vm_args.options = options;
	vm_args.ignoreUnrecognized = JNI_TRUE; // remove unrecognized options
	typedef jint(JNICALL* PtrCreateJavaVM)(JavaVM**, void**, void*);
	PtrCreateJavaVM ptrCreateJavaVM = (PtrCreateJavaVM)GetProcAddress(hinstLib, "JNI_CreateJavaVM");
	int ret1 = ptrCreateJavaVM(&jvm, (void**)&env, &vm_args);
	if (ret1 == JNI_ERR) std::cout << "[JavaWrapper] Error creating JVM\n";
	else if (ret1 == JNI_OK) std::cout << "[JavaWrapper] Java Virtual Machine started\n";
	api.jvmCreatedCallback(jvm, env);

	char* mainclass = replace_mainclass(argv[jvm_args]);
	jclass MainClass = env->FindClass(mainclass);
	jmethodID MainMethod = env->GetStaticMethodID(MainClass, "main", "([Ljava/lang/String;)V");
	jclass StringClass = env->FindClass("java/lang/String");
	int numOfArguments = argc - jvm_args - 1;
	int argumentIndex = 0;

	jobjectArray jargs = env->NewObjectArray(numOfArguments, StringClass, NULL);
	for (int i = jvm_args + 1; i < argc; ++i)
	{
		env->SetObjectArrayElement(jargs, i - jvm_args - 1, env->NewStringUTF(argv[i]));
		std::cout << argv[i] << std::endl;
	}
	env->CallStaticObjectMethod(MainClass, MainMethod, jargs);
	jthrowable exc;
	if (exc = env->ExceptionOccurred())
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
	}
	jvm->DestroyJavaVM();
	logger.debug("JavaWrapper", "Stopped Java Virtual Machine");
	return 0;
}

// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
