# -*- coding: utf-8 -*-
# :Project:   python-rapidjson -- Packaging
# :Author:    Ken Robbins <ken@kenrobbins.com>
# :License:   MIT License
# :Copyright: © 2015 Ken Robbins
# :Copyright: © 2016, 2017 Lele Gaifax
#

import os.path
import sys

try:
    from setuptools import setup, Extension
    try:
        # This is needed for some old versions of setuptools
        import packaging.specifiers
    except ImportError:
        pass
    other_setup_options = {'python_requires': '>=3.4'}
except ImportError:
    from distutils.core import setup, Extension
    other_setup_options = {}

from distutils import sysconfig


if sys.version_info < (3,):
    raise NotImplementedError("Only Python 3+ is supported.")

ROOT_PATH = os.path.abspath(os.path.dirname(__file__))

if not os.path.isdir(os.path.join(ROOT_PATH, 'rapidjson', 'include')):
    raise RuntimeError("RapidJSON sources not found: if you cloned the git repository,"
                       " you should initialize the rapidjson submodule as explained in"
                       " the README.rst; in all other cases you may want to report the"
                       " issue.")

with open('version.txt', encoding='utf-8') as f:
    VERSION = f.read()

with open('README.rst', encoding='utf-8') as f:
    LONG_DESCRIPTION = f.read()

with open('CHANGES.rst', encoding='utf-8') as f:
    CHANGES = f.read()

rj_include_dir = './rapidjson/include'

for idx, arg in enumerate(sys.argv[:]):
    if arg.startswith('--rj-include-dir='):
        sys.argv.pop(idx)
        rj_include_dir = arg.split('=', 1)[1]
        break

extension_options = {
    'sources': ['./rapidjson.cpp'],
    'include_dirs': [rj_include_dir],
    'define_macros': [('PYTHON_RAPIDJSON_VERSION', VERSION)],
}


cxx = sysconfig.get_config_var('CXX')
if cxx and 'gnu' in cxx:
    # Avoid warning about invalid flag for C++
    cflags = sysconfig.get_config_var('CFLAGS')
    if cflags and '-Wstrict-prototypes' in cflags:
        cflags = cflags.replace('-Wstrict-prototypes', '')
        sysconfig.get_config_vars()['CFLAGS'] = cflags
    opt = sysconfig.get_config_var('OPT')
    if opt and '-Wstrict-prototypes' in opt:
        opt = opt.replace('-Wstrict-prototypes', '')
        sysconfig.get_config_vars()['OPT'] = opt

    # Add -pedantic, so we get a warning when using non-standard features, and
    # -Wno-long-long to pacify old gcc (or Apple's hybrids) that treat "long
    # long" as an error under C++ (see issue #69)
    extension_options['extra_compile_args'] = ['-pedantic', '-Wno-long-long']

    # Up to Python 3.7, some structures use "char*" instead of "const char*",
    # and ISO C++ forbids assigning string literal constants
    if sys.version_info < (3,7):
        extension_options['extra_compile_args'].append('-Wno-write-strings')


setup(
    name='python-rapidjson',
    version=VERSION,
    description='Python wrapper around rapidjson',
    long_description=LONG_DESCRIPTION + '\n\n' + CHANGES,
    license='MIT License',
    keywords='json rapidjson',
    author='Ken Robbins',
    author_email='ken@kenrobbins.com',
    maintainer='Lele Gaifax',
    maintainer_email='lele@metapensiero.it',
    url='https://github.com/python-rapidjson/python-rapidjson',
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: MIT License',
        'Programming Language :: C++',
        'Programming Language :: Python :: 3 :: Only',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.4',
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python',
    ],
    ext_modules=[Extension('rapidjson', **extension_options)],
    **other_setup_options
)
