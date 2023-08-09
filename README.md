# cmm: Color Management Module based on lcms2. Not a full wrapper.

Color management is a kind of rocket science. You should be a specialist of ICC color management 
before struggling this module. You should be familiar with soft proofing, color conversion intent, gamut, etc. first.
These are the ABC of color management.
Please don't try to learn such deep knowledge from this module and document.

I don't want to repeat [lcms2](https://github.com/mm2/Little-CMS) document here.
This module is a thin wrapper of lcms2. Not a full wrapper, but covers most common usage.

## How to use

See `tests/test_from_python.py`. Advanced usage (partial profile and LUT) is not shown there. 
If you are informed enough to use it, you don't need any instruction.

`import faulthandler; faulthandler.enable()` is strongly recommended. There is no memory protection in this module.
You can easily make a segmentation fault.

## Versioning

It is the default versioning scheme of `setuptools-scm`. Commit and tag with a version number
when you need clean version.

## Build

Requires cmake. VS2022 for Windows, Homebrew for macOS, `sudo apt install python3.9-dev` for Ubuntu, etc.

Retrieve all submodules by `git submodule update --init`.

```
python -m pip install -U pip
pip install -U setuptools setuptools-scm wheel pybind11-stubgen
```

Use `python setup.py bdist_wheel` and ignore deprecation warnings. `python -m build` is not supported.
There is no way to copy `Little-CMS` and `pybind11` directories into venv isolated environment of `python -m build`.
pyproject.toml lacks such feature.

For Windows, it should be `python setup.py bdist_wheel --plat-name win_amd64` in "Developer PowerShell for VS 2022".
You can build x86 but you need to modify `setup.py` to do so.

For macOS, `MACOSX_DEPLOYMENT_TARGET=11 python setup.py bdist_wheel` is preferable.

The generated whl file should be found in `dist` directory.

## Unit testing

Build first.

`pip install pillow numpy`

`python -m unittest .\tests\test_from_python.py`

## License

MIT License.
