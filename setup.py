try:
    from setuptools import setup, Extension
except ImportError:
    from distutils.core import setup, Extension

with open('README.rst') as f:
    long_description = f.read()

rapidjson = Extension(
    'rapidjson',
    sources=['./python-rapidjson/rapidjson.cpp'],
    include_dirs=['./thirdparty/rapidjson/include'],
)

setup(
    name='python-rapidjson',
    version='0.0.5',
    description='Python wrapper around rapidjson',
    long_description=long_description,
    license='MIT License',
    keywords='json rapidjson',
    author='Ken Robbins',
    author_email='ken@kenrobbins.com',
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
