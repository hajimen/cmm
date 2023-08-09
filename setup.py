import os
import re
import sys
import platform
import subprocess
from pathlib import Path
import shutil
from glob import glob

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
from distutils.version import LooseVersion


class CMakeExtension(Extension):
    def __init__(self, name: str) -> None:
        super().__init__(name, [])
        self.sourcedir = os.fspath(Path('.').resolve())


class CMakeBuild(build_ext):
    def run(self):
        try:
            out = subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError("CMake must be installed to build the following extensions: " +
                               ", ".join(e.name for e in self.extensions))

        if platform.system() == "Windows":
            cmake_version = LooseVersion(re.search(r'version\s*([\d.]+)', out.decode()).group(1))
            if cmake_version < '3.1.0':
                raise RuntimeError("CMake >= 3.1.0 is required on Windows")

        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        ext_fullpath = Path.cwd() / self.get_ext_fullpath(ext.name)
        extdir = ext_fullpath.parent.resolve()
        if extdir.exists():
            shutil.rmtree(extdir)
        os.makedirs(extdir)

        cmake_args = ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + str(extdir) + os.sep,
                      '-DPYTHON_EXECUTABLE=' + sys.executable]

        cfg = 'Debug' if self.debug else 'Release'
        build_args = ['--config', cfg]

        if platform.system() == "Windows":
            cmake_args += ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{}={}'.format(cfg.upper(), str(extdir))]
            # # self.plat_name is always 'win32' regardless of --plat-name. Unmaintained?
            # if self.plat_name == 'win32':
            #     cmake_args += ['-A', 'Win32']
            build_args += ['--', '/m']
        else:
            cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
            build_args += ['--', '-j2']

        cmake_args += [f"-DVERSION_INFO={self.distribution.get_version()}"]

        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        subprocess.check_call(['cmake', ext.sourcedir] + cmake_args, cwd=self.build_temp)
        subprocess.check_call(['cmake', '--build', '.'] + build_args, cwd=self.build_temp)

        env = os.environ.copy()
        env['PYTHONPATH'] = str(extdir)
        subprocess.check_call(['pybind11-stubgen', '-o', '.', '--no-setup-py', ext.name], cwd=extdir, env=env)
        for f in glob(str(extdir) + '/*'):
            p = Path(f)
            if p.is_file():
                shutil.copy(f, Path.cwd() / 'build')  # for debugging and unit testing


setup(
    name='cmm',
    author='Hajime NAKAZATO',
    author_email='hajime@kaoriha.org',
    description='Color management module',
    long_description='Color management module based on lcms2. Not a full wrapper.',
    ext_modules=[CMakeExtension('cmm')],
    cmdclass=dict(build_ext=CMakeBuild),
    license='LICENSE',
    classifiers=[
        'Development Status :: 3 - Alpha',
        "Intended Audience :: Developers",
        'License :: OSI Approved :: MIT License',
        'Operating System :: MacOS :: MacOS X',
        'Operating System :: Microsoft :: Windows',
        'Operating System :: POSIX',
        'Programming Language :: Python',
        'Programming Language :: C++',
        'Topic :: Software Development :: Libraries :: Python Modules',
    ],
    keywords=['color management']
)
