/*
    Gasper-dumping (casper misspelled)
    Project Description: Injected DLL that will hook a function in java.dll export table to dump all loaded classes.

    Necessities:
        - Brain
        - Warning level: 0
        - CPP 17
*/
#include "entry.h"

java_defineclass_fn original_fn;

/* copied from native code */
static char*
getUTF(JNIEnv* env, jstring str, char* localBuf, int bufSize)
{
    char* utfStr = NULL;

    int len = env->GetStringUTFLength(str);
    int unicode_len = env->GetStringLength(str);
    if (len >= bufSize) {
        utfStr = (char*)malloc(len + 1);
        if (utfStr == NULL) {
            return NULL;
        }
    }
    else {
        utfStr = localBuf;
    }
    env->GetStringUTFRegion(str, 0, unicode_len, utfStr);

    return utfStr;
}

jclass __stdcall
java_defineClass1_hk(JNIEnv* env,
    jobject loader,
    jstring name,
    jbyteArray data,
    jint offset,
    jint length,
    jobject pd,
    jstring source)
{
    /* we can just copy some of the logic from the native code */
    if (data == NULL || length < 0) {
        return original_fn(env, loader, name, data, offset, length, pd, source);
    }

    jbyte* body = (jbyte*)malloc(length);
    env->GetByteArrayRegion(data, offset, length, body);

    auto chars = env->GetStringUTFChars(name, nullptr);

    std::string name_str(chars);
    
    /* bruh */
    const auto str_contains = [&](std::string text) {
        return name_str.find(text) != std::string::npos;
    };

    /* Skip basic classes */
    if (str_contains("java.lang") || str_contains("com.sun"))
        return original_fn(env, loader, name, data, offset, length, pd, source);

    std::replace(name_str.begin(), name_str.end(), '.', '\\');

    std::string path = std::string("C:\\Dump\\");

    /* This means we have to allocate files 
       Also: this can be simplified, by just copying the text and just replacing every instance of '/' with "//" but that'd require
       a whole new function, and I decided against it for simplicity (lol)
    */
    for (auto t = name_str.begin(); t < name_str.end(); t++)
    {
            auto character = *t;

            if ('\\' == character)
                path += "\\"; /* make sure we account for formatting*/
            else
                path += character;
    }

    path += ".class"; /* append file extension */

    /* permission issues bruhh

        TODO: fix this shit


    std::ofstream ofstream;
    ofstream.open(path, std::ios::binary | std::ios::out);

    if (!ofstream.is_open())
        printf("Unable to open path \"%s\"?\n", path.c_str());

    ofstream.write((char*)body, length);
    ofstream.close();
    */


    printf("Dumped (?): %s -> \"%s\"\n", chars, path.c_str());
    env->ReleaseStringUTFChars(name, chars);

    
    return original_fn(env, loader, name, data, offset, length, pd, source);
}

auto start_gasper_dumping()
{//Give it a console for debugging purposes
    AllocConsole();
    //Allow you to close the console without the host process closing too
    SetConsoleCtrlHandler(NULL, true);
    //Assign console in and out to pass to the create console rather than minecraft's
    FILE* fIn;
    FILE* fOut;
    freopen_s(&fIn, "conin$", "r", stdin);
    freopen_s(&fOut, "conout$", "w", stdout);
    freopen_s(&fOut, "conout$", "w", stderr);
    auto java_dll = GetModuleHandleA("java.dll");

    if (!java_dll)
    {
        printf("Unable to find java.dll, are we injected into a java process?\n");
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::exit(0);
    }

    /* find exports for the function */
    auto define_class_fn = GetProcAddress(java_dll, "Java_java_lang_ClassLoader_defineClass1");

    if (!define_class_fn)
    {
        printf("Unable to find define_class_fn, wtf?\n");
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::exit(0);
    }

    MH_Initialize();

    MH_CreateHook(define_class_fn, java_defineClass1_hk, reinterpret_cast<void**>(&original_fn));
    auto res = MH_EnableHook(MH_ALL_HOOKS);

    if (res != MH_OK) {
        printf("Unable to hook define_class_fn, are you sure the address is correct?\n");
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::exit(0);
    }

    printf("Succesfuly hooked define_class_fn, dumping will begin.");
}

bool __stdcall DllMain(
    HINSTANCE hinstDLL,
    DWORD     fdwReason,
    LPVOID    lpvReserved
) {

    DisableThreadLibraryCalls(nullptr);

    static void* h_thread = nullptr;
    
    if (fdwReason == DLL_PROCESS_ATTACH)
        h_thread = CreateThread(0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(start_gasper_dumping), 0, 0, 0);
    else
        if (h_thread)
            CloseHandle(h_thread);

    return true;
}