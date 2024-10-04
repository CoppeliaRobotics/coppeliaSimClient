#include <string>
#include <vector>
#include <filesystem>
#include <thread>
#include <iostream>
#include <boost/program_options.hpp>
#include <simLib/simLib.h>

#if __unix__ || __linux__ || __APPLE__
#include <unistd.h>
#endif

// Following required to have Lua extension libraries work under Linux:
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

static LIBRARY simLib=nullptr;
static bool autoStart=false;
static bool autoQuit=false;
static int stopDelay=0;
static std::string sceneOrModel;
static std::string appDir;

void unloadSimLib()
{
    simAddLog("CoppeliaSimClient",sim_verbosity_loadinfos,"unloading the CoppeliaSim library...");
    unloadSimLibrary(simLib);
    simLib=nullptr;
    simAddLog("CoppeliaSimClient",sim_verbosity_loadinfos,"done.");
}

bool loadSimLib(std::string libName)
{
    simAddLog("CoppeliaSimClient",sim_verbosity_loadinfos,"loading the CoppeliaSim library...");
    #ifdef MAC_SIM
        libName="@executable_path/lib"+libName+".dylib";
    #endif
    #ifdef LIN_SIM
        libName="lib"+libName+".so";
    #endif
    simLib=loadSimLibrary(libName.c_str());
    if (simLib!=NULL)
    {
        if (getSimProcAddresses(simLib)!=0)
            simAddLog("CoppeliaSimClient",sim_verbosity_loadinfos,"done.");
        else
        {
            simAddLog("CoppeliaSimClient",sim_verbosity_errors,"could not find all required functions in the CoppeliaSim library.");
            unloadSimLib();
        }
    }
    else
        simAddLog("CoppeliaSimClient",sim_verbosity_errors,"could not find or correctly load the CoppeliaSim library.");
    return(simLib!=nullptr);
}

bool endsWith(const std::string& value,const std::string& ending)
{
    bool retVal=false;
    if (ending.size()<=value.size())
        retVal=std::equal(ending.rbegin(),ending.rend(),value.rbegin());
    return(retVal);
}

void simThreadStartAddress()
{
    simInitialize(appDir.c_str(),0);
    bool autoStarted=false;
    int simulationRunCnt=0;
    while (!simGetExitRequest())
    {
        if (sceneOrModel.size()>0)
        {
            if ( endsWith(sceneOrModel,".ttt")||endsWith(sceneOrModel,".simscene.xml") )
            {
                if (simLoadScene(sceneOrModel.c_str())==-1)
                    simAddLog("CoppeliaSimClient",sim_verbosity_errors,"scene could not be opened.");
            }
            if ( endsWith(sceneOrModel,".ttm")||endsWith(sceneOrModel,".simmodel.xml"))
            {
                if (simLoadModel(sceneOrModel.c_str())==-1)
                    simAddLog("CoppeliaSimClient",sim_verbosity_errors,"model could not be opened.");
            }
            sceneOrModel.clear();
        }
        if (autoStart)
        {
            autoStart=false;
            simStartSimulation();
            autoStarted=true;
        }
        int simState=simGetSimulationState();
        if (simState==sim_simulation_advancing_firstafterstop)
            simulationRunCnt++;
        if ( (simState==sim_simulation_stopped)&&(simulationRunCnt==1)&&autoQuit )
            simPostExitRequest(); // notifiy the GUI
        if ( (stopDelay>0)&& autoStarted && (simGetSimulationTime()*1000>double(stopDelay)) )
        {
            stopDelay=0;
            simStopSimulation();
        }

        simLoop(nullptr,0);
    }
    simDeinitialize();
}

int main(int argc,char* argv[])
{
#if __unix__ || __linux__ || __APPLE__
    if(argc >= 3 && (!strcmp(argv[1], "--exec-program") || !strcmp(argv[1], "-X")))
        return execv(argv[2], &argv[2]);
#endif

    namespace po = boost::program_options;
    po::options_description desc;
    desc.add_options()
        ("headless,h", "runs CoppeliaSim in an emulated headless mode: this simply suppresses all GUI elements (e.g. doesn't open the main window, etc.), but otherwise runs normally")
        ("true-headless,H", "runs CoppeliaSim in true headless mode (i.e. without any GUI or GUI dependencies). A display server is however still required. Instead of using library coppeliaSim, library coppeliaSimHeadless will be used")
        ("auto-start,s", po::value<int>(), "automatically start the simulation. If an argument is specified, simulation will automatically stop after the specified amount of milliseconds.")
        ("auto-quit,q", "automatically quits CoppeliaSim after the first simulation run ended.")
        ("cmd,c", po::value<std::string>(), "executes the specified script string as soon as the sandbox script is initialized.")
        ("verbosity,v", po::value<std::string>()->default_value("loadinfos"), "sets the verbosity level, in the console. Default is loadinfos. Accepted values are: none, errors, warnings, loadinfos, scripterrors, scriptwarnings, scriptinfos, infos, debug, trace, tracelua and traceall.")
        ("statusbar-verbosity,w", po::value<std::string>()->default_value("scriptinfos"), "sets the verbosity level, in the statusbar. Default is scriptinfos. Accepted values are: none, errors, warnings, loadinfos, scripterrors, scriptwarnings, scriptinfos, infos, debug, trace, tracelua and traceall.")
        ("dialogs-verbosity,x", po::value<std::string>()->default_value("infos"), "sets the verbosity level, for simple dialogs. Default is infos. Accepted values are: none, errors, warnings and questions.")
        ("addon,a", po::value<std::string>(), "loads and runs an additional add-on specified via its filename.")
        ("addon2,b", po::value<std::string>(), "loads and runs an additional add-on specified via its filename.")
        ("param,G", po::value<std::vector<std::string>>(), "sets a named param YYY=XXX: named parameter: YYY represents the key, XXX the value, that can be queried within CoppeliaSim with sim.getNamedStringParam.")
        ("arg,g", po::value<std::vector<std::string>>(), "sets an optional argument that can be queried within CoppeliaSim with the sim.stringparam_app_arg1... sim.stringparam_app_arg9 parameters. The argument can be used for various custom purposes.")
        ("options,O", po::value<int>(), "options for the GUI.")
        ("scene-or-model-file,f", po::value<std::vector<std::string> >(), "input file");
    ;
    po::positional_options_description p;
    p.add("scene-or-model-file", 1);
    po::variables_map vm;
    try
    {
        po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
        po::notify(vm);
    }
    catch(boost::program_options::error &ex)
    {
        std::cerr << ex.what() << "\n\nusage:\n" << desc << std::endl;
        return 2;
    }

    int exitCode=255;
    std::filesystem::path path(argv[0]);
    appDir=path.parent_path().string();

    int options=sim_gui_all;
    bool trueHeadless=false;

    if (vm.count("true-headless"))
    {
        trueHeadless=true;
        options=0;
    }
    if (vm.count("headless"))
        options=sim_gui_headless;
    else
    {
        if (vm.count("options") && (!trueHeadless))
            options=vm["options"].as<int>();
    }
    if (vm.count("auto-start"))
    {
        autoStart=true;
        stopDelay=vm["auto-start"].as<int>();
    }
    if (vm.count("auto-quit"))
        autoQuit=true;
    if (vm.count("scene-or-model-file"))
    {
        sceneOrModel=vm["scene-or-model-file"].as<std::vector<std::string>>().at(0);
    }

    std::string libName("coppeliaSim");
    if (trueHeadless)
        libName="coppeliaSimHeadless";
    if (loadSimLib(libName))
    {
        if (vm.count("cmd"))
            simSetStringParam(sim_stringparam_startupscriptstring,vm["cmd"].as<std::string>().c_str());
        if (vm.count("verbosity"))
            simSetStringParam(sim_stringparam_verbosity,vm["verbosity"].as<std::string>().c_str());
        if (vm.count("statusbar-verbosity"))
            simSetStringParam(sim_stringparam_statusbarverbosity,vm["statusbar-verbosity"].as<std::string>().c_str());
        if (vm.count("dialogs-verbosity"))
            simSetStringParam(sim_stringparam_dlgverbosity,vm["dialogs-verbosity"].as<std::string>().c_str());
        if (vm.count("addon"))
            simSetStringParam(sim_stringparam_additional_addonscript1,vm["addon"].as<std::string>().c_str()); // normally, never call API functions before simRunSimulator!!
        if (vm.count("addon2"))
            simSetStringParam(sim_stringparam_additional_addonscript2,vm["addon2"].as<std::string>().c_str()); // normally, never call API functions before simRunSimulator!!
        if (vm.count("arg"))
        {
            auto args = vm["arg"].as<std::vector<std::string>>();
            for (size_t i = 0; i < args.size(); i++)
            {
                if (i>=9) break;
                simSetStringParam(sim_stringparam_app_arg1+i,args[i].c_str()); // normally, never call API functions before simRunSimulator!!
            }
        }
        if (vm.count("param"))
        {
            auto params = vm["param"].as<std::vector<std::string>>();
            for (const std::string &param : params)
            {
                size_t pos=param.find('=');
                if ( (pos!=std::string::npos)&&(pos!=param.length()-1) )
                {
                    std::string key(param.begin(),param.begin()+pos);
                    std::string value(param.begin()+pos+1,param.end());
                    simSetNamedStringParam(key.c_str(),value.c_str(),int(value.size()));
                }
            }
        }

        simAddLog("CoppeliaSimClient",sim_verbosity_loadinfos,"launching CoppeliaSim...");
        if (trueHeadless)
            simThreadStartAddress();
        else
        {
            std::thread simThread(simThreadStartAddress);
            simRunGui(options);
            simThread.join();
        }
        exitCode=0;
        unloadSimLib();
    }
    return(exitCode);
}
