import sys
from pathlib import Path
import faulthandler
faulthandler.enable()
import os
import unittest
import PIL.Image as PILImageModule
import numpy as np


CURRENT_DIR = Path(__file__).parent.parent
sys.path.append(str(CURRENT_DIR / 'build'))
import cmm


TEST_IMG = CURRENT_DIR / 'tests/resource/img.jpg'
pil_img = PILImageModule.open(TEST_IMG)
TEST_PROFILE = CURRENT_DIR / 'tests/resource/profile.icc'
ORACLE_DIR = CURRENT_DIR / 'tests/oracle'


class TestSimple(unittest.TestCase):
    def test_version(self):
        self.assertTrue(isinstance(cmm.__version__, str))
        self.assertTrue(isinstance(cmm.__lcms_version__, int))

    def test_open_close(self):
        with open(TEST_PROFILE, 'rb') as f:
            hp = cmm.open_profile_from_mem(f.read())
        self.assertIsNotNone(hp)
        cmm.close_profile(hp)

    def test_error_handler(self):
        code = 0
        msg = ''

        def error_handler(error_code: int, error_msg: str):
            nonlocal code, msg
            code = error_code
            msg = error_msg

        cmm.set_log_error_handler(error_handler)
        cmm.open_profile_from_mem(b'    ')
        self.assertEqual(code, cmm.cmsERROR_READ)
        self.assertEqual(msg, 'Read from memory error')
        cmm.unset_log_error_handler()

    def test_fmt(self):
        cmm.get_transform_formatter(0, cmm.PT_RGB, 3, 1, 0, 0)


class TestCombined(unittest.TestCase):
    def setUp(self) -> None:
        self.fmt = cmm.get_transform_formatter(0, cmm.PT_RGB, 3, 1, 0, 0)
        with open(TEST_PROFILE, 'rb') as f:
            self.hp = cmm.open_profile_from_mem(f.read())
        self.srgb = cmm.create_srgb_profile()

        self.code = 0
        self.msg = ''

        def error_handler(error_code: int, error_msg: str):
            nonlocal self
            self.code = error_code
            self.msg = error_msg

        cmm.set_log_error_handler(error_handler)

    def tearDown(self) -> None:
        cmm.close_profile(self.hp)
        cmm.close_profile(self.srgb)
        self.assertEqual(self.code, 0)
        self.assertEqual(self.msg, '')
        cmm.unset_log_error_handler()

    def test_get_color_space(self):
        cs = cmm.get_color_space(self.hp)
        self.assertEqual(cs, cmm.cmsSigRgbData)

    def test_get_profile_description(self):
        desc = cmm.get_profile_description(self.hp)
        self.assertEqual(desc, 'sub20191126@sRGB')

    def test_get_device_class(self):
        dc = cmm.get_device_class(self.hp)
        self.assertEqual(dc, cmm.cmsSigOutputClass)

    def test_create_delete_transform(self):
        tr = cmm.create_transform(
            self.srgb, self.fmt,
            self.hp, self.fmt,
            cmm.INTENT_RELATIVE_COLORIMETRIC,
            cmm.cmsFLAGS_BLACKPOINTCOMPENSATION)
        self.assertIsNotNone(tr)
        cmm.delete_transform(tr)

    def test_create_delete_proofing_transform(self):
        tr = cmm.create_proofing_transform(
            self.srgb, self.fmt,
            self.srgb, self.fmt,
            self.hp,
            cmm.INTENT_RELATIVE_COLORIMETRIC,
            cmm.INTENT_PERCEPTUAL,
            cmm.cmsFLAGS_BLACKPOINTCOMPENSATION)
        self.assertIsNotNone(tr)
        cmm.delete_transform(tr)


class TestConversion(unittest.TestCase):
    def setUp(self) -> None:
        self.fmt = cmm.get_transform_formatter(0, cmm.PT_RGB, 3, 1, 0, 0)
        self.src_img = np.array(pil_img.convert('RGB'))
        self.trg_img = np.zeros_like(self.src_img)

        with open(TEST_PROFILE, 'rb') as f:
            self.hp = cmm.open_profile_from_mem(f.read())
        self.srgb = cmm.create_srgb_profile()

        self.code = 0
        self.msg = ''

        def error_handler(error_code: int, error_msg: str):
            nonlocal self
            self.code = error_code
            self.msg = error_msg

        cmm.set_log_error_handler(error_handler)

    def tearDown(self) -> None:
        cmm.close_profile(self.hp)
        cmm.close_profile(self.srgb)
        self.assertEqual(self.code, 0)
        self.assertEqual(self.msg, '')
        cmm.unset_log_error_handler()

    def assert_image(self, oracle_name: str, make_oracle=False):
        if make_oracle:
            PILImageModule.fromarray(self.trg_img).save(ORACLE_DIR / oracle_name)
        else:
            oracle = np.array(PILImageModule.open(ORACLE_DIR / oracle_name), dtype=np.uint8)
            self.assertTrue(np.all(np.isclose(self.trg_img, oracle, atol=1)))

    @unittest.skipIf(sys.platform == 'emscripten',
                     "Emscripten float seems different from other CPUs.")
    def test_8_8(self):
        tr = cmm.create_transform(
            self.srgb, self.fmt,
            self.hp, self.fmt,
            cmm.INTENT_RELATIVE_COLORIMETRIC,
            cmm.cmsFLAGS_BLACKPOINTCOMPENSATION)
        cmm.do_transform_8_8(tr, self.src_img, self.trg_img, self.src_img.size // 3)
        self.assert_image('test_8_8.png')
        cmm.delete_transform(tr)

    @unittest.skipIf(sys.platform == 'emscripten',
                     "Emscripten float seems different from other CPUs.")
    def test_8_8_proofing(self):
        self.assertNotEqual(cmm.set_alarm_codes(np.zeros(16, dtype=np.uint16)), 0)
        tr = cmm.create_proofing_transform(
            self.srgb, self.fmt,
            self.srgb, self.fmt,
            self.hp,
            cmm.INTENT_RELATIVE_COLORIMETRIC,
            cmm.INTENT_RELATIVE_COLORIMETRIC,
            cmm.cmsFLAGS_BLACKPOINTCOMPENSATION | cmm.cmsFLAGS_GAMUTCHECK | cmm.cmsFLAGS_SOFTPROOFING)
        cmm.do_transform_8_8(tr, self.src_img, self.trg_img, self.src_img.size // 3)
        self.assert_image('test_8_8_proofing.png')
        cmm.delete_transform(tr)

    @unittest.skipIf(sys.platform == 'emscripten',
                     "Emscripten float seems different from other CPUs.")
    def test_patch(self):
        with open(CURRENT_DIR / 'tests/resource/Linear P3D65.icc', 'rb') as f:
            WS_HP = cmm.open_profile_from_mem(f.read())

        with open(CURRENT_DIR / r'tests\resource\sublinova-epson4pigment-PBT-20231121_srgb.icc', 'rb') as f:
            SUBLINOVA_HP = cmm.open_profile_from_mem(f.read())

        fmt = cmm.get_transform_formatter(0, cmm.PT_RGB, 3, 2, 0, 0)

        tr = cmm.create_transform(
            WS_HP, fmt,
            SUBLINOVA_HP, self.fmt,
            cmm.INTENT_RELATIVE_COLORIMETRIC,
            cmm.cmsFLAGS_BLACKPOINTCOMPENSATION)

        ws_img = np.load(CURRENT_DIR / 'tests/resource/ws_img.npy')
        self.trg_img = np.zeros_like(ws_img, dtype=np.uint8)
        cmm.do_transform_16_8(tr, ws_img[:, :, ::-1].copy(), self.trg_img, ws_img.size // 3)
        self.assert_image('test_patch.png')
        cmm.delete_transform(tr)
