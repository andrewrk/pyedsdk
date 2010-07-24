from distutils.core import setup, Extension

# http://docs.python.org/distutils/apiref.html#distutils.core.Extension
edsdk = Extension(
    'edsdkmodule',
    sources = [
        'daw/Flp.cpp',
        'daw/Utils.cpp',
        'daw/FlpModule.cpp',
    ],
    include_dirs = [
        'daw'
    ],
)

setup(
    name='edsdk',
    version=__import__('edsdk').__version__,
    author="Andrew Kelley",
    author_email="superjoe30@gmail.com",
    url="http://github.com/superjoe30/pyedsdk",
    description='Python library to control cameras via EDSDK',
    license="GPL",
    ext_modules=[flp],
    packages=["daw"],
)
