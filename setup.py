import os.path
import re

try:
    from setuptools import setup, Extension
except ImportError:
    from distutils.core import setup, Extension

ROOT_PATH = os.path.abspath(os.path.dirname(__file__))

def find_version():
    with open(os.path.join(ROOT_PATH, 'python-rapidjson/version.h')) as f:
        data = f.read()

    v = re.search(r'PYTHON_RAPIDJSON_VERSION\s+(\S+)', data).group(1)
    return v.replace('"', '')

def find_author():
    with open(os.path.join(ROOT_PATH, 'python-rapidjson/version.h')) as f:
        data = f.read()

    author = re.search(r'PYTHON_RAPIDJSON_AUTHOR\s+(\S+)', data).group(1)
    email = re.search(r'PYTHON_RAPIDJSON_AUTHOR_EMAIL\s+(\S+)', data).group(1)
    return (author.replace('"', ''), email.replace('"', ''))

with open('README.rst') as f:
    long_description = f.read()

rapidjson = Extension(
    'rapidjson',
    sources=['./python-rapidjson/rapidjson.cpp'],
    include_dirs=['./rapidjson/include'],
)

setup(
    name='python-rapidjson',
    version=find_version(),
    description='Python wrapper around rapidjson',
    long_description=long_description,
    license='MIT License',
    keywords='json rapidjson',
    author=find_author()[0],
    author_email=find_author()[1],
    url='https://github.com/kenrobbins/python-rapidjson',
    download_url='https://github.com/kenrobbins/python-rapidjson',
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: MIT License',
        'Programming Language :: C++',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python',
    ],
    ext_modules=[rapidjson],
)
