#ifdef WIN_VREP
    #include <direct.h>
    #include <Windows.h>
#endif
#ifdef LIN_VREP
    #include <unistd.h>
#endif

#include "v_repLib.h"
#include <vector>
#include <iostream>

#ifdef SIM_WITHOUT_QT_AT_ALL
    #include <algorithm>
    #ifdef WIN_VREP
        #include "_dirent.h"
    #else
        #include <unistd.h>
        #include <dirent.h>
        #include <stdio.h>
    #endif
#else
    #include <QtCore/QCoreApplication>
    #include <QLibrary>
    #include <QFileInfo>
    #include <QDir>
    #ifndef WIN_VREP
        #include <unistd.h>
    #endif
#endif

// Following required to have Lua extension libraries work under Linux. Strange...
#ifdef LIN_VREP
    extern "C" {
        #include "lua.h"
        #include "lualib.h"
        #include "lauxlib.h"
    }
    void dummyFunction()
    {
        lua_State *L;
        L=luaL_newstate();
    }
#endif

std::vector<int> pluginHandles;
std::string sceneOrModelOrUiToLoad;
bool autoStart=false;
int simStopDelay=0;
bool autoQuit=false;

int loadPlugin(const char* theName,const char* theDirAndName)
{
    std::cout << "Plugin '" << theName << "': loading...\n";
    int pluginHandle=simLoadModule(theDirAndName,theName);
    if (pluginHandle==-3)
    #ifdef WIN_VREP
        std::cout << "Error with plugin '" << theName << "': load failed (could not load). The plugin probably couldn't load dependency libraries. Try rebuilding the plugin.\n";
    #endif
    #ifdef MAC_VREP
        std::cout << "Error with plugin '" << theName << "': load failed (could not load). The plugin probably couldn't load dependency libraries. Try 'otool -L pluginName.dylib' for more infos, or simply rebuild the plugin.\n";
    #endif
    #ifdef LIN_VREP
        std::cout << "Error with plugin '" << theName << "': load failed (could not load). The plugin probably couldn't load dependency libraries. For additional infos, modify the script 'libLoadErrorCheck.sh', run it and inspect the output.\n";
    #endif

    if (pluginHandle==-2)
        std::cout << "Error with plugin '" << theName << "': load failed (missing entry points).\n";
    if (pluginHandle==-1)
        std::cout << "Error with plugin '" << theName << "': load failed (failed initialization).\n";
    if (pluginHandle>=0)
        std::cout << "Plugin '" << theName << "': load succeeded.\n";
    return(pluginHandle);
}

void simulatorInit()
{
    std::cout << "Simulator launched.\n";

    std::vector<std::string> theNames;
    std::vector<std::string> theDirAndNames;

#ifdef SIM_WITHOUT_QT_AT_ALL
    char curDirAndFile[2048];
    #ifdef WIN_VREP
        GetModuleFileNameA(NULL,curDirAndFile,2000);
        int i=0;
        while (true)
        {
            if (curDirAndFile[i]==0)
                break;
            if (curDirAndFile[i]=='\\')
                curDirAndFile[i]='/';
            i++;
        }
        std::string theDir(curDirAndFile);
        while ( (theDir.size()>0)&&(theDir[theDir.size()-1]!='/') )
            theDir.erase(theDir.end()-1);
        if (theDir.size()>0)
            theDir.erase(theDir.end()-1);
    #else
        getcwd(curDirAndFile,2000);
        std::string theDir(curDirAndFile);
    #endif

    DIR* dir;
    struct dirent* ent;
    if ((dir=opendir(theDir.c_str()))!=NULL)
    {
        while ((ent=readdir(dir))!=NULL)
        {
            if ( (ent->d_type==DT_LNK)||(ent->d_type==DT_REG) )
            {
                std::string nm(ent->d_name);
                std::transform(nm.begin(),nm.end(),nm.begin(),::tolower);
                int pre=0;
                int po=0;
                #ifdef WIN_VREP
                    pre=8;
                    po=4;
                    if ( (nm.compare(0,8,"v_repext")==0)&&(nm.compare(nm.size()-4,4,".dll")==0) )
                #endif
                #ifdef LIN_VREP
                    pre=11;
                    po=3;
                    if ( (nm.compare(0,11,"libv_repext")==0)&&(nm.compare(nm.size()-3,3,".so")==0) )
                #endif
                #ifdef MAC_VREP
                    pre=11;
                    po=6;
                    if ( (nm.compare(0,11,"libv_repext")==0)&&(nm.compare(nm.size()-6,6,".dylib")==0) )
                #endif
                {
                    if (nm.find('_',6)==std::string::npos)
                    {
                        nm=ent->d_name;
                        nm.assign(nm.begin()+pre,nm.end()-po);
                        theNames.push_back(nm);
                        theDirAndNames.push_back(theDir+'/'+ent->d_name);
                    }
                }
            }
        }
        closedir(dir);
    }
#else // SIM_WITHOUT_QT_AT_ALL

    QFileInfo pathInfo(QCoreApplication::applicationFilePath());
    std::string pa=pathInfo.path().toStdString();
    QDir dir(pa.c_str());
    dir.setFilter(QDir::Files|QDir::Hidden); // |QDir::NoSymLinks); // removed on 11/4/2013 thanks to Karl Robillard
    dir.setSorting(QDir::Name);
    QStringList filters;
    int bnl=8;
    #ifdef WIN_VREP
        std::string tmp("v_repExt*.dll");
    #endif
    #ifdef MAC_VREP
        std::string tmp("libv_repExt*.dylib");
        bnl=11;
    #endif
    #ifdef LIN_VREP
        std::string tmp("libv_repExt*.so");
        bnl=11;
    #endif
    filters << tmp.c_str();
    dir.setNameFilters(filters);
    QFileInfoList list=dir.entryInfoList();
    for (int i=0;i<list.size();++i)
    {
        QFileInfo fileInfo=list.at(i);
        std::string bla(fileInfo.baseName().toLocal8Bit());
        std::string tmp;
        tmp.assign(bla.begin()+bnl,bla.end());
        if (tmp.find('_')==std::string::npos)
        {
            theNames.push_back(tmp);
            theDirAndNames.push_back(fileInfo.absoluteFilePath().toLocal8Bit().data());
        }
    }
#endif // SIM_WITHOUT_QT_AT_ALL

    // Load the system plugins first:
    for (int i=0;i<int(theNames.size());i++)
    {
        if ((theNames[i].compare("MeshCalc")==0)||(theNames[i].compare("Dynamics")==0)||(theNames[i].compare("PathPlanning")==0))
        {
            int pluginHandle=loadPlugin(theNames[i].c_str(),theDirAndNames[i].c_str());
            if (pluginHandle>=0)
                pluginHandles.push_back(pluginHandle);
            theDirAndNames[i]=""; // mark as 'already loaded'
        }
    }
    simLoadModule("",""); // indicate that we are done with the system plugins

    // Now load the other plugins too:
    for (int i=0;i<int(theNames.size());i++)
    {
        if (theDirAndNames[i].compare("")!=0)
        { // not yet loaded
            int pluginHandle=loadPlugin(theNames[i].c_str(),theDirAndNames[i].c_str());
            if (pluginHandle>=0)
                pluginHandles.push_back(pluginHandle);
        }
    }

    if (sceneOrModelOrUiToLoad.length()!=0)
    { // Here we double-clicked a V-REP file or dragged-and-dropped it onto this application
        int l=int(sceneOrModelOrUiToLoad.length());
        if ((l>4)&&(sceneOrModelOrUiToLoad[l-4]=='.')&&(sceneOrModelOrUiToLoad[l-3]=='t')&&(sceneOrModelOrUiToLoad[l-2]=='t'))
        {
            simSetBooleanParameter(sim_boolparam_scene_and_model_load_messages,1);
            if (sceneOrModelOrUiToLoad[l-1]=='t') // trying to load a scene?
            {
                if (simLoadScene(sceneOrModelOrUiToLoad.c_str())==-1)
                    simAddStatusbarMessage("Scene could not be opened.");
            }
            if (sceneOrModelOrUiToLoad[l-1]=='m') // trying to load a model?
            {
                if (simLoadModel(sceneOrModelOrUiToLoad.c_str())==-1)
                    simAddStatusbarMessage("Model could not be loaded.");
            }
            if (sceneOrModelOrUiToLoad[l-1]=='b') // trying to load a UI?
            {
                if (simLoadUI(sceneOrModelOrUiToLoad.c_str(),0,NULL)==-1)
                    simAddStatusbarMessage("UI could not be loaded.");
            }
            simSetBooleanParameter(sim_boolparam_scene_and_model_load_messages,0);
        }
    }
}

void simulatorLoop()
{   // The main application loop (excluding the GUI part)
    static bool wasRunning=false;
    int auxValues[4];
    int messageID=0;
    int dataSize;
    if (autoStart)
    {
        simStartSimulation();
        autoStart=false;
    }
    while (messageID!=-1)
    {
        simChar* data=simGetSimulatorMessage(&messageID,auxValues,&dataSize);
        if (messageID!=-1)
        {
            if (messageID==sim_message_simulation_start_resume_request)
                simStartSimulation();
            if (messageID==sim_message_simulation_pause_request)
                simPauseSimulation();
            if (messageID==sim_message_simulation_stop_request)
                simStopSimulation();
            if (data!=NULL)
                simReleaseBuffer(data);
        }
    }

    // Handle a running simulation:
    if ( (simGetSimulationState()&sim_simulation_advancing)!=0 )
    {
        wasRunning=true;
        if ( (simGetRealTimeSimulation()!=1)||(simIsRealTimeSimulationStepNeeded()==1) )
        {
            if ((simHandleMainScript()&sim_script_main_script_not_called)==0)
                simAdvanceSimulationByOneStep();
            if ((simStopDelay>0)&&(simGetSimulationTime()>=float(simStopDelay)/1000.0f))
            {
                simStopDelay=0;
                simStopSimulation();
            }
        }
    }
    if ( (simGetSimulationState()==sim_simulation_stopped)&&wasRunning&&autoQuit )
    {
        wasRunning=false;
        simQuitSimulator(true); // will post the quit command
    }
}

void simulatorDeinit()
{
    // Unload all plugins:
    for (int i=0;i<int(pluginHandles.size());i++)
        simUnloadModule(pluginHandles[i]);
    pluginHandles.clear();
    std::cout << "Simulator ended.\n";
}

int main(int argc, char* argv[])
{
    #ifdef WIN_VREP
        timeBeginPeriod(1);

        // Set the current path same as the application path
        char basePath[2048];
        _fullpath(basePath,argv[0],sizeof(basePath));
        std::string p(basePath);
        p.erase(p.begin()+p.find_last_of('\\')+1,p.end());
        QDir::setCurrent(p.c_str());
    #endif

    std::string libLoc(argv[0]);
    while ((libLoc.length()>0)&&(libLoc[libLoc.length()-1]))
    {
        int l=(int)libLoc.length();
        if (libLoc[l-1]!='/')
            libLoc.erase(libLoc.end()-1);
        else
        { // we might have a 2 byte char:
            if (l>1)
            {
                if (libLoc[l-2]>0x7F)
                    libLoc.erase(libLoc.end()-1);
                else
                    break;

            }
            else
                break;
        }
    }
    chdir(libLoc.c_str());

    #ifdef WIN_VREP
        LIBRARY lib=loadVrepLibrary("v_rep");
    #endif
    #ifdef MAC_VREP
        LIBRARY lib=loadVrepLibrary("libv_rep.dylib");
    #endif
    #ifdef LIN_VREP
        LIBRARY lib=loadVrepLibrary("libv_rep.so");
    #endif

    bool wasRunning=false;
    if (lib!=NULL)
    {
        if (getVrepProcAddresses(lib)!=0)
        {
            int guiItems=sim_gui_all;
            for (int i=1;i<argc;i++)
            {
                std::string arg(argv[i]);
                if (arg[0]=='-')
                {
                    if (arg.length()>=2)
                    {
                        if (arg[1]=='h')
                            guiItems=sim_gui_headless;
                        if (arg[1]=='s')
                        {
                            autoStart=true;
                            simStopDelay=0;
                            unsigned int p=2;
                            while (arg.length()>p)
                            {
                                simStopDelay*=10;
                                if ((arg[p]>='0')&&(arg[p]<='9'))
                                    simStopDelay+=arg[p]-'0';
                                else
                                {
                                    simStopDelay=0;
                                    break;
                                }
                                p++;
                            }
                        }
                        if (arg[1]=='q')
                            autoQuit=true;
                        if ((arg[1]=='a')&&(arg.length()>2))
                        {
                            std::string tmp;
                            tmp.assign(arg.begin()+2,arg.end());
                            simSetStringParameter(sim_stringparam_additional_addonscript_firstscene,tmp.c_str()); // normally, never call API functions before simRunSimulator!!
                        }
                        if ((arg[1]=='b')&&(arg.length()>2))
                        {
                            std::string tmp;
                            tmp.assign(arg.begin()+2,arg.end());
                            simSetStringParameter(sim_stringparam_additional_addonscript,tmp.c_str()); // normally, never call API functions before simRunSimulator!!
                        }
                        if ((arg[1]=='g')&&(arg.length()>2))
                        {
                            static int cnt=0;
                            std::string tmp;
                            tmp.assign(arg.begin()+2,arg.end());
                            if (cnt<9)
                                simSetStringParameter(sim_stringparam_app_arg1+cnt,tmp.c_str()); // normally, never call API functions before simRunSimulator!!
                            cnt++;
                        }
                    }
                }
                else
                {
                    if (sceneOrModelOrUiToLoad.length()==0)
                        sceneOrModelOrUiToLoad=arg;
                }
            }

            if (simRunSimulator("V-REP",guiItems,simulatorInit,simulatorLoop,simulatorDeinit)!=1)
                std::cout << "Failed initializing and running V-REP\n";
            else
                wasRunning=true;
        }
        else
            std::cout << "Error: could not find all required functions in the V-REP library\n";
        unloadVrepLibrary(lib);
    }
    else
        printf("Error: could not find or correctly load the V-REP library\n");

    #ifdef WIN_VREP
        timeEndPeriod(1);
    #endif

    if (!wasRunning)
        getchar();
    return(0);
}
