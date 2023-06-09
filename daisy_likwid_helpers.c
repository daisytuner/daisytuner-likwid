/**
 * Original file: https://github.com/RRZE-HPC/pylikwid/blob/master/pylikwid.c
 * 
 * Modifications:
 * - Removed all functions unrelated to CPU and GPU topology.
 * - Renamed the package to avoid compatibility issues.
*/

#include <Python.h>
#include <likwid.h>

#define PYSTR(str) (Py_BuildValue("s", str))
#define PYINT(val) (Py_BuildValue("i", val))
#define PYUINT(val) (Py_BuildValue("I", val))

static int topo_initialized = 0;
CpuInfo_t cpuinfo = NULL;
CpuTopology_t cputopo = NULL;
static int numa_initialized = 0;
NumaTopology_t numainfo = NULL;

static PyObject *
likwid_lversion(PyObject *self, PyObject *args)
{
    int v = LIKWID_MAJOR;
    int r = LIKWID_RELEASE;
    int m = LIKWID_MINOR;
    return Py_BuildValue("iii", v, r, m);
}

static PyObject *
likwid_inittopology(PyObject *self, PyObject *args)
{
    int ret = topology_init();
    if (ret == 0)
    {
        topo_initialized = 1;
        return Py_True;
    }
    return Py_False;
}

static PyObject *
likwid_finalizetopology(PyObject *self, PyObject *args)
{
    topology_finalize();
    topo_initialized = 0;
    cputopo = NULL;
    cpuinfo = NULL;
    Py_RETURN_NONE;
}

static PyObject *
likwid_getcputopology(PyObject *self, PyObject *args)
{
    int i, ret;
    PyObject *d = PyDict_New();
    if (!topo_initialized)
    {
        ret = topology_init();
        if (ret == 0)
        {
            topo_initialized = 1;
        }
        else
        {
            return d;
        }
    }
    PyObject *threads = PyDict_New();
    PyObject *caches = PyDict_New();
    PyObject *tmp;
    if ((topo_initialized) && (cputopo == NULL))
    {
        cputopo = get_cpuTopology();
    }
    if (!numa_initialized)
    {
        if (numa_init() == 0)
        {
            numa_initialized = 1;
            numainfo = get_numaTopology();
        }
    }
    if ((numa_initialized) && (numainfo == NULL))
    {
        numainfo = get_numaTopology();
    }
    PyDict_SetItem(d, PYSTR("numHWThreads"), PYINT(cputopo->numHWThreads));
    PyDict_SetItem(d, PYSTR("activeHWThreads"), PYINT(cputopo->activeHWThreads));
    PyDict_SetItem(d, PYSTR("numSockets"), PYINT(cputopo->numSockets));
    PyDict_SetItem(d, PYSTR("numCoresPerSocket"), PYINT(cputopo->numCoresPerSocket));
    PyDict_SetItem(d, PYSTR("numThreadsPerCore"), PYINT(cputopo->numThreadsPerCore));
    PyDict_SetItem(d, PYSTR("numCacheLevels"), PYINT(cputopo->numCacheLevels));
    for (i = 0; i < (int)cputopo->numHWThreads; i++)
    {
        tmp = PyDict_New();
        PyDict_SetItem(tmp, PYSTR("threadId"), PYUINT(cputopo->threadPool[i].threadId));
        PyDict_SetItem(tmp, PYSTR("coreId"), PYUINT(cputopo->threadPool[i].coreId));
        PyDict_SetItem(tmp, PYSTR("packageId"), PYUINT(cputopo->threadPool[i].packageId));
        PyDict_SetItem(tmp, PYSTR("apicId"), PYUINT(cputopo->threadPool[i].apicId));
        PyDict_SetItem(threads, PYINT(i), tmp);
    }
    PyDict_SetItem(d, PYSTR("threadPool"), threads);
    for (i = 0; i < (int)cputopo->numCacheLevels; i++)
    {
        tmp = PyDict_New();
        PyDict_SetItem(tmp, PYSTR("level"), PYUINT(cputopo->cacheLevels[i].level));
        PyDict_SetItem(tmp, PYSTR("associativity"), PYUINT(cputopo->cacheLevels[i].associativity));
        PyDict_SetItem(tmp, PYSTR("sets"), PYUINT(cputopo->cacheLevels[i].sets));
        PyDict_SetItem(tmp, PYSTR("lineSize"), PYUINT(cputopo->cacheLevels[i].lineSize));
        PyDict_SetItem(tmp, PYSTR("size"), PYUINT(cputopo->cacheLevels[i].size));
        PyDict_SetItem(tmp, PYSTR("threads"), PYUINT(cputopo->cacheLevels[i].threads));
        PyDict_SetItem(tmp, PYSTR("inclusive"), PYUINT(cputopo->cacheLevels[i].inclusive));
        switch(cputopo->cacheLevels[i].type)
        {
            case DATACACHE:
                PyDict_SetItem(tmp, PYSTR("type"), PYSTR("data"));
                break;
            case INSTRUCTIONCACHE:
                PyDict_SetItem(tmp, PYSTR("type"), PYSTR("instruction"));
                break;
            case UNIFIEDCACHE:
                PyDict_SetItem(tmp, PYSTR("type"), PYSTR("unified"));
                break;
            case ITLB:
                PyDict_SetItem(tmp, PYSTR("type"), PYSTR("itlb"));
                break;
            case DTLB:
                PyDict_SetItem(tmp, PYSTR("type"), PYSTR("dtlb"));
                break;
            case NOCACHE:
                break;
        }
        PyDict_SetItem(caches, PYUINT(cputopo->cacheLevels[i].level), tmp);
    }
    PyDict_SetItem(d, PYSTR("cacheLevels"), caches);
    return d;
}

static PyObject *
likwid_getcpuinfo(PyObject *self, PyObject *args)
{
    int ret;
    PyObject *d = PyDict_New();
    if (!topo_initialized)
    {
        ret = topology_init();
        if (ret == 0)
        {
            topo_initialized = 1;
        }
        else
        {
            return d;
        }
    }
    if ((topo_initialized) && (cputopo == NULL))
    {
        cputopo = get_cpuTopology();
    }
    if (!numa_initialized)
    {
        if (numa_init() == 0)
        {
            numa_initialized = 1;
            numainfo = get_numaTopology();
        }
    }
    if ((numa_initialized) && (numainfo == NULL))
    {
        numainfo = get_numaTopology();
    }
    CpuInfo_t info = get_cpuInfo();
    PyDict_SetItem(d, PYSTR("family"), PYUINT(info->family));
    PyDict_SetItem(d, PYSTR("model"), PYUINT(info->model));
    PyDict_SetItem(d, PYSTR("stepping"), PYUINT(info->stepping));
    PyDict_SetItem(d, PYSTR("clock"), Py_BuildValue("k", info->clock));
    if (info->turbo)
    {
        PyDict_SetItem(d, PYSTR("turbo"), Py_True);
    }
    else
    {
        PyDict_SetItem(d, PYSTR("turbo"), Py_False);
    }
    if (info->isIntel)
    {
        PyDict_SetItem(d, PYSTR("isIntel"), Py_True);
    }
    else
    {
        PyDict_SetItem(d, PYSTR("isIntel"), Py_False);
    }
    if (info->supportUncore)
    {
        PyDict_SetItem(d, PYSTR("supportUncore"), Py_True);
    }
    else
    {
        PyDict_SetItem(d, PYSTR("supportUncore"), Py_False);
    }
    PyDict_SetItem(d, PYSTR("osname"), PYSTR(info->osname));
    PyDict_SetItem(d, PYSTR("name"), PYSTR(info->name));
    PyDict_SetItem(d, PYSTR("short_name"), PYSTR(info->short_name));
    PyDict_SetItem(d, PYSTR("features"), PYSTR(info->features));
    PyDict_SetItem(d, PYSTR("featureFlags"), PYUINT(info->featureFlags));
    PyDict_SetItem(d, PYSTR("perf_version"), PYUINT(info->perf_version));
    PyDict_SetItem(d, PYSTR("perf_num_ctr"), PYUINT(info->perf_num_ctr));
    PyDict_SetItem(d, PYSTR("perf_width_ctr"), PYUINT(info->perf_width_ctr));
    PyDict_SetItem(d, PYSTR("perf_num_fixed_ctr"), PYUINT(info->perf_num_fixed_ctr));
    PyDict_SetItem(d, PYSTR("architecture"), PYSTR(info->architecture));
    return d;
}


static PyObject *
likwid_printsupportedcpus(PyObject *self, PyObject *args)
{
    print_supportedCPUs();
    Py_RETURN_NONE;
}


#if (LIKWID_NVMON)
static int gpuTopology_initialized = 0;
static GpuTopology_t gputopo = NULL;
static int nvmon_initialized = 1;

static PyObject *
likwid_initgputopology(PyObject *self, PyObject *args)
{
    int ret = 0;
    if (!gpuTopology_initialized)
    {
        ret = topology_gpu_init();
        if (ret == EXIT_SUCCESS)
        {
            gpuTopology_initialized = 1;
        }
    }
    return Py_BuildValue("i", ret);
}

static PyObject *
likwid_finalizegputopology(PyObject *self, PyObject *args)
{
    if (gpuTopology_initialized)
    {
        topology_gpu_finalize();
    }
    Py_RETURN_NONE;
}

static PyObject *
likwid_getgputopology(PyObject *self, PyObject *args)
{
    int i, ret;

    if (!gpuTopology_initialized)
    {
        ret = topology_gpu_init();
        if (ret == EXIT_SUCCESS)
        {
            gpuTopology_initialized = 1;
        }
        else
        {
            Py_RETURN_NONE;
        }
    }
    gputopo = get_gpuTopology();
    if (gputopo)
    {
        PyObject *l = PyList_New(gputopo->numDevices);
        for(i = 0; i < gputopo->numDevices; i++)
        {
            GpuDevice* dev = &gputopo->devices[i];
            PyObject *d = PyDict_New();

            PyDict_SetItem(d, PYSTR("devid"), PYUINT(dev->devid));
            PyDict_SetItem(d, PYSTR("numaNode"), PYUINT(dev->numaNode));
            PyDict_SetItem(d, PYSTR("name"), PYSTR(dev->name));
            PyDict_SetItem(d, PYSTR("mem"), PYUINT(dev->mem));
            PyDict_SetItem(d, PYSTR("ccapMajor"), PYUINT(dev->ccapMajor));
            PyDict_SetItem(d, PYSTR("ccapMinor"), PYUINT(dev->ccapMinor));
            PyDict_SetItem(d, PYSTR("maxThreadsPerBlock"), PYUINT(dev->maxThreadsPerBlock));
            PyDict_SetItem(d, PYSTR("sharedMemPerBlock"), PYUINT(dev->sharedMemPerBlock));
            PyDict_SetItem(d, PYSTR("totalConstantMemory"), PYUINT(dev->totalConstantMemory));
            PyDict_SetItem(d, PYSTR("simdWidth"), PYUINT(dev->simdWidth));
            PyDict_SetItem(d, PYSTR("memPitch"), PYUINT(dev->memPitch));
            PyDict_SetItem(d, PYSTR("regsPerBlock"), PYUINT(dev->regsPerBlock));
            PyDict_SetItem(d, PYSTR("clockRatekHz"), PYUINT(dev->clockRatekHz));
            PyDict_SetItem(d, PYSTR("textureAlign"), PYUINT(dev->textureAlign));
            PyDict_SetItem(d, PYSTR("l2Size"), PYUINT(dev->l2Size));
            PyDict_SetItem(d, PYSTR("memClockRatekHz"), PYUINT(dev->memClockRatekHz));
            PyDict_SetItem(d, PYSTR("pciBus"), PYUINT(dev->pciBus));
            PyDict_SetItem(d, PYSTR("pciDev"), PYUINT(dev->pciDev));
            PyDict_SetItem(d, PYSTR("pciDom"), PYUINT(dev->pciDom));
            PyDict_SetItem(d, PYSTR("maxBlockRegs"), PYUINT(dev->maxBlockRegs));
            PyDict_SetItem(d, PYSTR("numMultiProcs"), PYUINT(dev->numMultiProcs));
            PyDict_SetItem(d, PYSTR("maxThreadPerMultiProc"), PYUINT(dev->maxThreadPerMultiProc));
            PyDict_SetItem(d, PYSTR("memBusWidth"), PYUINT(dev->memBusWidth));
            PyDict_SetItem(d, PYSTR("unifiedAddrSpace"), PYUINT(dev->unifiedAddrSpace));
            PyDict_SetItem(d, PYSTR("ecc"), PYUINT(dev->ecc));
            PyDict_SetItem(d, PYSTR("asyncEngines"), PYUINT(dev->asyncEngines));
            PyDict_SetItem(d, PYSTR("mapHostMem"), PYUINT(dev->mapHostMem));
            PyDict_SetItem(d, PYSTR("integrated"), PYUINT(dev->integrated));

            PyObject *maxThreadsDim = PyList_New(3);
            PyList_SET_ITEM(maxThreadsDim, 0, PYUINT(dev->maxThreadsDim[0]));
            PyList_SET_ITEM(maxThreadsDim, 1, PYUINT(dev->maxThreadsDim[1]));
            PyList_SET_ITEM(maxThreadsDim, 2, PYUINT(dev->maxThreadsDim[2]));
            PyDict_SetItem(d, PYSTR("maxThreadsDim"), maxThreadsDim);

            PyObject *maxGridSize = PyList_New(3);
            PyList_SET_ITEM(maxGridSize, 0, PYUINT(dev->maxGridSize[0]));
            PyList_SET_ITEM(maxGridSize, 1, PYUINT(dev->maxGridSize[1]));
            PyList_SET_ITEM(maxGridSize, 2, PYUINT(dev->maxGridSize[2]));
            PyDict_SetItem(d, PYSTR("maxGridSize"), maxGridSize);

            PyList_SET_ITEM(l, (Py_ssize_t)i, d);
        }
        return l;
    }
    Py_RETURN_NONE;
}

#endif

static PyMethodDef LikwidMethods[] = {
    {"likwidversion", likwid_lversion, METH_VARARGS, "Get the likwid version numbers."},
    /* topology functions */
    {"inittopology", likwid_inittopology, METH_VARARGS, "Initialize the topology module."},
    {"finalizetopology", likwid_finalizetopology, METH_VARARGS, "Finalize the topology module."},
    {"getcputopology", likwid_getcputopology, METH_VARARGS, "Get the topology information for the current system."},
    {"getcpuinfo", likwid_getcpuinfo, METH_VARARGS, "Get the system information for the current system."},
    {"printsupportedcpus", likwid_printsupportedcpus, METH_VARARGS, "Print all CPU variants supported by current version of LIKWID."},
    /* GPU functions */
#ifdef LIKWID_NVMON
    {"initgputopology", likwid_initgputopology, METH_VARARGS, "Initialize the topology module for NVIDIA GPUs."},
    {"finalizegputopology", likwid_finalizegputopology, METH_VARARGS, "Finalize the topology module for NVIDIA GPUs."},
    {"getgputopology", likwid_getgputopology, METH_VARARGS, "Get the topology information for the current system for NVIDIA GPUs."},
#endif
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "daisy_likwid_helpers",
    .m_doc = NULL,
    .m_size = -1,
    .m_methods = LikwidMethods,
};

PyMODINIT_FUNC
PyInit_daisy_likwid_helpers(void)
{
    return PyModule_Create(&module);
}
