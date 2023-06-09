#
# Original file: https://github.com/RRZE-HPC/pylikwid/blob/master/setup.py
#
# Modifications:
# - Changed setup to new package.
#


import os, os.path, glob, re, sys
from distutils.core import setup, Extension


def generic_iglob(pattern):
    result = None
    if sys.version_info.major == 3 and sys.version_info.minor > 4:
        result = glob.iglob(pattern, recursive=True)
    else:
        result = glob.iglob(pattern)
    return result


def find_file(path_iterator):
    path = None
    path_to_explore = None
    is_searching = True
    if path_iterator:
        while is_searching:
            try:
                path_to_explore = next(path_iterator)
                if os.path.exists(path_to_explore):
                    path = path_to_explore
                is_searching = False
            except StopIteration:
                is_searching = False
    return path


def get_hierarchy():
    include_path = None
    library_path = None
    library = None
    prefix_path = os.getenv("LIKWID_PREFIX", None)
    library_pattern = "lib*/liblikwid.so*"
    if prefix_path is not None:
        iterator = generic_iglob(os.path.join(prefix_path, library_pattern))
        library = find_file(iterator)
        if library is not None:
            library_path = os.path.dirname(library)
    if library is None:
        paths_iterator = iter(os.environ["PATH"].split(":"))
        is_searching = True
        while is_searching:
            try:
                path = next(paths_iterator)
                prefix_path = os.path.abspath(os.path.join(path, ".."))
                iterator = generic_iglob(os.path.join(prefix_path, library_pattern))
                library = find_file(iterator)
                if library is not None:
                    library_path = os.path.dirname(library)
                    is_searching = False
            except StopIteration:
                is_searching = False
        if library is None:
            prefix_path = "/usr/local"
            iterator = generic_iglob(os.path.join(prefix_path, library_pattern))
            library = find_file(iterator)
            if library is not None:
                library_path = os.path.dirname(library)

    include_path = os.path.join(prefix_path, "include")
    if not os.path.exists(os.path.join(include_path, "likwid.h")):
        iterator = generic_iglob(prefix_path + "**/likwid.h")
        include_path = find_file(iterator)
    if prefix_path is None or not os.path.exists(prefix_path):
        raise Exception("Error the likwid prefix directory was not found")
    if library is None or not os.path.exists(library):
        raise Exception("Error the likwid library was not found")
    if library_path is None or not os.path.exists(library_path):
        raise Exception("Error the likwid library directory was not found")
    if include_path is None or not os.path.exists(include_path):
        raise Exception("Error the likwid include directory was not found")
    m = re.match("lib(.*)\.so", os.path.basename(library))
    if m:
        library = m.group(1)
    return prefix_path, library_path, library, include_path


try:
    LIKWID_PREFIX, LIKWID_LIBPATH, LIKWID_LIB, LIKWID_INCPATH = get_hierarchy()
    print(LIKWID_PREFIX, LIKWID_LIBPATH, LIKWID_LIB, LIKWID_INCPATH)
except Exception as e:
    print(e)
    sys.exit(1)


daisy_likwid_helpers = Extension(
    "daisy_likwid_helpers",
    include_dirs=[LIKWID_INCPATH],
    libraries=[LIKWID_LIB],
    library_dirs=[LIKWID_LIBPATH],
    sources=["daisy_likwid_helpers.c"],
)

setup(
    name="daisytuner-likwid",
    version="0.0.1",
    author="Lukas Truemper",
    author_email="lukas.truemper@outlook.de",
    description="Likwid functions used by the daisytuner",
    long_description="Likwid functions used by the daisytuner",
    url="https://github.com/daisytuner/daisytuner-likwid",
    classifiers=[
        "Topic :: Utilities",
    ],
    python_requires=">=3.6",
    package_data={"daisytuner-likwid": ["daisy_likwid_helpers.c", "LICENSE"]},
    ext_modules=[daisy_likwid_helpers],
)
