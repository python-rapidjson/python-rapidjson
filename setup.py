import os.path
import re
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

VERSION_H = os.path.join(ROOT_PATH, 'python-rapidjson', 'version.h')

with open(VERSION_H, encoding='utf-8') as f:
    data = f.read()

    VERSION = re.search(r'PYTHON_RAPIDJSON_VERSION\s+"([^"]+)"', data).group(1)
    AUTHOR = re.search(r'PYTHON_RAPIDJSON_AUTHOR\s+"([^"]+)"', data).group(1)
    EMAIL = re.search(r'PYTHON_RAPIDJSON_AUTHOR_EMAIL\s+"([^"]+)"', data).group(1)

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
    'sources': ['./python-rapidjson/rapidjson.cpp'],
    'include_dirs': [rj_include_dir],
}

cc = sysconfig.get_config_var('CC')
if cc and 'gcc' in cc:
    cflags = sysconfig.get_config_var('CFLAGS')
    # Avoid warning about invalid flag for C++
    if cflags and '-Wstrict-prototypes' in cflags:
        cflags = cflags.replace('-Wstrict-prototypes', '')
        sysconfig.get_config_vars()['CFLAGS'] = cflags

    # Add -pedantic, so we get a warning when using non-standard features, and
    # -Wno-long-long to pacify old gcc (or Apple's hybrids) that treat "long
    # long" as an error under C++ (see issue #69)
    extension_options['extra_compile_args'] = ['-pedantic', '-Wno-long-long']

rapidjson = Extension('rapidjson', **extension_options)

setup(
    name='python-rapidjson',
    version=VERSION,
    description='Python wrapper around rapidjson',
    long_description=LONG_DESCRIPTION + '\n\n' + CHANGES,
    license='MIT License',
    keywords='json rapidjson',
    author=AUTHOR,
    author_email=EMAIL,
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
    ext_modules=[rapidjson],
    **other_setup_options
)
