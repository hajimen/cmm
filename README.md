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
python -m pip install -U pip build
python -m build --wheel
```

For macOS, `MACOSX_DEPLOYMENT_TARGET=11 python -m build --wheel` is preferable.

The generated whl file should be found in `dist` directory.

## Unit testing

Build with `python -m build --wheel`. Do not forget `--wheel`. It generates `build` directory and `tests/test_from_python.py` uses the output.

`pip install pillow`

`python -m unittest .\tests\test_from_python.py`

## License

MIT License.
