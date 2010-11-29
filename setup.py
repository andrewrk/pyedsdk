from distutils.core import setup, Extension

# http://docs.python.org/distutils/apiref.html#distutils.core.Extension
camera = Extension(
    'edsdk.Camera',
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
    version='0.2',
    author="Andrew Kelley",
    author_email="superjoe30@gmail.com",
    url="http://github.com/superjoe30/pyedsdk",
    description='Python library to control Canon cameras via EDSDK',
    license="LGPL",
    ext_modules=[camera],
    packages=["edsdk"],
)
