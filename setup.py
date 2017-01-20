import os.path
import re
import sys

try:
    from setuptools import setup, Extension
except ImportError:
    from distutils.core import setup, Extension


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

rapidjson = Extension(
    'rapidjson',
    sources=['./python-rapidjson/rapidjson.cpp'],
    include_dirs=['./rapidjson/include'],
)

setup(
    name='python-rapidjson',
    version=VERSION,
    description='Python wrapper around rapidjson',
    long_description=LONG_DESCRIPTION,
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
        'Programming Language :: Python',
    ],
    python_requires=">=3",
    ext_modules=[rapidjson],
)
