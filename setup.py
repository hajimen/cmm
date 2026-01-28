import os
import sys
import platform
import subprocess
from pathlib import Path
import shutil
from glob import glob

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext


class CMakeExtension(Extension):
    def __init__(self, name: str) -> None:
        super().__init__(name, [])
        self.sourcedir = os.fspath(Path('.').resolve())


class CMakeBuild(build_ext):
    def run(self):
        # Fetch git submodules if they're missing (needed for pip install from git)
        pybind11_dir = Path(__file__).parent / 'pybind11'
        if not (pybind11_dir / 'CMakeLists.txt').exists():
            subprocess.check_call(['git', 'submodule', 'update', '--init', '--recursive'],
                                  cwd=Path(__file__).parent)

        try:
            subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError("CMake must be installed to build the following extensions: " +
                               ", ".join(e.name for e in self.extensions))

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
            if self.plat_name == 'win32':
                cmake_args += ['-A', 'Win32']
            build_args += ['--', '/m']
        else:
            cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
            cmake_args += ['-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64']
            build_args += ['--', '-j2']

        cmake_args += [f"-DVERSION_INFO={self.distribution.get_version()}"]

        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        subprocess.check_call(['cmake', ext.sourcedir] + cmake_args, cwd=self.build_temp)
        subprocess.check_call(['cmake', '--build', '.'] + build_args, cwd=self.build_temp)

        env = os.environ.copy()
        env['PYTHONPATH'] = str(extdir)
        # Generate type stubs (optional - may fail in isolated build environments)
        try:
            subprocess.check_call(['pybind11-stubgen', '-o', '.', '--ignore-unresolved-names', 'capsule', ext.name], cwd=extdir, env=env)
        except (subprocess.CalledProcessError, FileNotFoundError):
            print("Warning: pybind11-stubgen failed, skipping .pyi generation")
        for f in glob(str(extdir) + '/*'):
            p = Path(f)
            if p.is_file():
                shutil.copy(f, Path.cwd() / 'build')  # for debugging and unit testing


setup(
    name='cmm-16bit',
    author='Hajime NAKAZATO',
    author_email='hajime@kaoriha.org',
    description='Color management module',
    long_description=(Path(__file__).parent / 'README.md').read_text(),
    long_description_content_type='text/markdown',
    url='https://github.com/hajimen/cmm',
    ext_modules=[CMakeExtension('cmm')],
    cmdclass=dict(build_ext=CMakeBuild),
    license='MIT',
    classifiers=[
        'Development Status :: 3 - Alpha',
        "Intended Audience :: Developers",
        'Operating System :: MacOS :: MacOS X',
        'Operating System :: Microsoft :: Windows',
        'Operating System :: POSIX',
        'Programming Language :: Python',
        'Programming Language :: C++',
        'Topic :: Software Development :: Libraries :: Python Modules',
    ],
    keywords=['color management']
)
