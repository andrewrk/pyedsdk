from distutils.core import setup, Extension

# http://docs.python.org/distutils/apiref.html#distutils.core.Extension
camera = Extension(
    'Camera',
    sources = [
        'edsdk/Camera.cpp',
        'edsdk/Utils.cpp',
        'edsdk/Filesystem.cpp',
        'edsdk/CameraModule.cpp',
    ],
    include_dirs = [
        'edsdk',
    ],
    libraries = [
        'ole32',
        'EDSDK',
    ],
)

setup(
    name='edsdk',
    version='0.1',
    author="Andrew Kelley",
    author_email="superjoe30@gmail.com",
    url="http://github.com/superjoe30/pyedsdk",
    description='Python library to control Canon cameras via EDSDK',
    license="GPL",
    ext_modules=[camera],
    ext_package="edsdk",
    packages=["edsdk"],
)
