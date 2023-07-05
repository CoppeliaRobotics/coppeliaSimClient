#ifdef WIN_SIM
    #include <direct.h>
    #include <Windows.h>
    #include <process.h>
    #define THREAD_RET_TYPE void
    #define THREAD_RET_VAL void()
#else
    #include <unistd.h>
    #include <pthread.h>
    #define THREAD_RET_TYPE void*
    #define THREAD_RET_VAL 0
#endif

#ifdef SIM_WITHOUT_QT_AT_ALL
    #include <algorithm>
    #ifdef WIN_SIM
        #include <simLib/_dirent.h>
    #else
        #include <dirent.h>
    #endif
#else
    #include <QtCore/QCoreApplication>
    #include <QLibrary>
    #include <QFileInfo>
    #include <QDir>
#endif

// Following required to have Lua extension libraries work under Linux. Strange...
#ifdef LIN_SIM
    extern "C" {
        #include <lua.h>
        #include <lualib.h>
        #include <lauxlib.h>
    }
    void dummyFunction()
    {
        lua_State *L;
        L=luaL_newstate();
    }
#endif

#include <simLib/simLib.h>
#include <vector>
#include <iostream>

static LIBRARY simLib=nullptr;
static int stopDelay;
static int options;
static std::string sceneOrModelToLoad;

int loadSimLib(const char* execPath,std::string& appDir)
{
    printf("[CoppeliaSimClient]    loading the CoppeliaSim library...\n"); // always print, do not use simAddLog
    #ifdef WIN_SIM
        // Set the current path same as the application path
        char basePath[2048];
        _fullpath(basePath,execPath,sizeof(basePath));
        std::string p(basePath);
        p.erase(p.begin()+p.find_last_of('\\')+1,p.end());
        // SetCurrentDirectory(p.c_str());
        QDir::setCurrent(p.c_str());
    #endif
    appDir=execPath;
    while ((appDir.length()>0)&&(appDir[appDir.length()-1]))
    {
        int l=(int)appDir.length();
        if (appDir[l-1]!='/')
            appDir.erase(appDir.end()-1);
        else
        { // we might have a 2 byte char:
            if (l>1)
            {
                if (appDir[l-2]>0x7F)
                    appDir.erase(appDir.end()-1);
                else
                    break;

            }
            else
                break;
        }
    }
    chdir(appDir.c_str());
    #ifdef WIN_SIM
        simLib=loadSimLibrary("coppeliaSim");
    #endif
    #ifdef MAC_SIM
        simLib=loadSimLibrary("@executable_path/libcoppeliaSim.dylib");
    #endif
    #ifdef LIN_SIM
        simLib=loadSimLibrary("libcoppeliaSim.so");
    #endif
    if (simLib!=NULL)
    {
        if (getSimProcAddresses(simLib)!=0)
        {
            printf("[CoppeliaSimClient]    done.\n"); // always print, do not use simAddLog
            return(1);
        }
        printf("[CoppeliaSimClient]    could not find all required functions in the CoppeliaSim library.\n"); // always print, do not use simAddLog
        return(0);
    }
    printf("[CoppeliaSimClient]    could not find or correctly load the CoppeliaSim library.\n"); // always print, do not use simAddLog
    return(-1);
}

void unloadSimLib()
{
    printf("[CoppeliaSimClient]    unloading the CoppeliaSim library...\n"); // always print, do not use simAddLog
    unloadSimLibrary(simLib);
    printf("[CoppeliaSimClient]    done.\n"); // always print, do not use simAddLog
}

bool run(int argc,char* argv[],const char* appDir)
{
    options=sim_gui_all;
    stopDelay=0;
    for (int i=1;i<argc;i++)
    {
        std::string arg(argv[i]);
        if (arg[0]=='-')
        {
            if (arg.length()>=2)
            {
                if (arg[1]=='h')
                {
                    options|=sim_gui_all|sim_gui_headless;
                    options-=sim_gui_all;
                }
                if (arg[1]=='s')
                {
                    options|=sim_autostart;
                    stopDelay=0;
                    unsigned int p=2;
                    while (arg.length()>p)
                    {
                        stopDelay*=10;
                        if ((arg[p]>='0')&&(arg[p]<='9'))
                            stopDelay+=arg[p]-'0';
                        else
                        {
                            stopDelay=0;
                            break;
                        }
                        p++;
                    }
                }
                if (arg[1]=='q')
                    options|=sim_autoquit;
                if (arg[1]=='c')
                {
                    std::string tmp;
                    tmp.assign(arg.begin()+2,arg.end());
                    simSetStringParam(sim_stringparam_startupscriptstring,tmp.c_str());
                }
                if (arg[1]=='v')
                {
                    std::string tmp;
                    tmp.assign(arg.begin()+2,arg.end());
                    simSetStringParam(sim_stringparam_verbosity,tmp.c_str());
                }
                if (arg[1]=='w')
                {
                    std::string tmp;
                    tmp.assign(arg.begin()+2,arg.end());
                    simSetStringParam(sim_stringparam_statusbarverbosity,tmp.c_str());
                }
                if (arg[1]=='x')
                {
                    std::string tmp;
                    tmp.assign(arg.begin()+2,arg.end());
                    simSetStringParam(sim_stringparam_dlgverbosity,tmp.c_str());
                }
                if ((arg[1]=='a')&&(arg.length()>2))
                {
                    std::string tmp;
                    tmp.assign(arg.begin()+2,arg.end());
                    simSetStringParam(sim_stringparam_additional_addonscript1,tmp.c_str()); // normally, never call API functions before simRunSimulator!!
                }
                if ((arg[1]=='b')&&(arg.length()>2))
                {
                    std::string tmp;
                    tmp.assign(arg.begin()+2,arg.end());
                    simSetStringParam(sim_stringparam_additional_addonscript2,tmp.c_str()); // normally, never call API functions before simRunSimulator!!
                }
                if ((arg[1]=='g')&&(arg.length()>2))
                {
                    static int cnt=0;
                    std::string tmp;
                    tmp.assign(arg.begin()+2,arg.end());
                    if (cnt<9)
                        simSetStringParam(sim_stringparam_app_arg1+cnt,tmp.c_str()); // normally, never call API functions before simRunSimulator!!
                    cnt++;
                }
                if ((arg[1]=='G')&&(arg.length()>3))
                {
                    size_t pos=arg.find('=',3);
                    if ( (pos!=std::string::npos)&&(pos!=arg.length()-1) )
                    {
                        std::string key(arg.begin()+2,arg.begin()+pos);
                        std::string param(arg.begin()+pos+1,arg.end());
                        simSetNamedStringParam(key.c_str(),param.c_str(),int(param.size()));
                    }
                }
            }
        }
        else
        {
            if (sceneOrModelToLoad.length()==0)
                sceneOrModelToLoad=arg;
        }
    }

    return(simRunGui("CoppeliaSim",options,stopDelay,sceneOrModelToLoad.c_str(),appDir)==1);
}

THREAD_RET_TYPE simThreadStartAddress(void*)
{
    while (!simCanInitSimThread());
    simInitSimThread();
    while (!simGetExitRequest())
        simLoop(0);
    simCleanupSimThread();
    return(THREAD_RET_VAL);
}

int main(int argc,char* argv[])
{
    #ifdef WIN_SIM
        timeBeginPeriod(1);
    #endif
    std::string appDir;
    int res=loadSimLib(argv[0],appDir);
    int exitCode=255;
    if (res==1)
    {
        exitCode=254;
        #ifdef WIN_SIM
            _beginthread(simThreadStartAddress,0,0);
        #else
            pthread_t th;
            pthread_create(&th,nullptr,simThreadStartAddress,nullptr);
        #endif

        simAddLog("CoppeliaSimClient",sim_verbosity_loadinfos,"launching CoppeliaSim...");
        if (run(argc,argv,appDir.c_str()))
        {
            exitCode=0;
            simGetInt32Param(sim_intparam_exitcode,&exitCode);
        }
        else
            simAddLog("CoppeliaSimClient",sim_verbosity_errors,"failed launching CoppeliaSim.");
    }
    if (res>=0)
        unloadSimLib();
    #ifdef WIN_SIM
        timeEndPeriod(1);
    #endif
    return(exitCode);
}
