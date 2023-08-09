#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include <lcms2.h>
#include <lcms2_internal.h>

namespace py = pybind11;
using namespace pybind11::literals;

template <typename T, typename U>
void do_transform(cmsHTRANSFORM ht, py::array_t <T> input_buf, py::array_t <U> output_buf, int num_pixel) {
	py::buffer_info input_bi = input_buf.request();
	py::buffer_info output_bi = output_buf.request();
	cmsDoTransform(ht, input_bi.ptr, output_bi.ptr, num_pixel);
}

bool setAsciiTag(std::string str, cmsHPROFILE hProfile, cmsTagSignature tag) {
	auto m = cmsMLUalloc(NULL, 0);
	cmsMLUsetASCII(m, cmsNoLanguage, cmsNoCountry, str.c_str());
	auto rc = cmsWriteTag(hProfile, tag, m);
	cmsMLUfree(m);
	return rc;
}

typedef union {
	char s[5];
	cmsTagSignature tag;
} tag_str;

bool invalidChar (char c) {  
    return c < 32 || c >= 127;
} 

void stripNonAscii(std::string &str) { 
    str.erase(remove_if(str.begin(),str.end(), invalidChar), str.end());  
}

std::map<std::string, cmsTagSignature> get_lut_tag_map() {
	auto lut_tag_map = std::map<std::string, cmsTagSignature>();
	lut_tag_map["B2A0"] = cmsSigBToA0Tag;
	lut_tag_map["B2A1"] = cmsSigBToA1Tag;
	lut_tag_map["B2A2"] = cmsSigBToA2Tag;
	lut_tag_map["A2B0"] = cmsSigAToB0Tag;
	lut_tag_map["A2B1"] = cmsSigAToB1Tag;
	lut_tag_map["A2B2"] = cmsSigAToB2Tag;
	lut_tag_map["gamt"] = cmsSigGamutTag;
	return lut_tag_map;
}

static py::function ERROR_HANDLER;
void CmmLogErrorHandler(cmsContext context, cmsUInt32Number error_code, const char *text)
{
	std::string msg = text;
	ERROR_HANDLER(error_code, msg);
}

PYBIND11_MODULE(cmm, m) {

#define PY_ATTR_PT(_a) m.attr(#_a) = _a
#define PY_ATTR_ENUM(_a) m.attr(#_a) = (int)_a

    m.doc() = R"pbdoc(
        Color Management Module
        -----------------------

        .. currentmodule:: cmm

        .. autosummary::
           :toctree: _generate

           set_log_error_handler
           open_profile_from_mem
		   close_profile
		   get_device_class
		   get_color_space
		   get_available_b2an_list
		   create_srgb_profile
		   get_profile_description
		   create_transform
		   create_proofing_transform
		   set_alarm_codes
		   get_transform_formatter
		   delete_transform
		   do_transform_8_8
		   do_transform_8_16
		   do_transform_16_8
		   do_transform_16_16
		   create_partial_profile
		   add_lut16
		   link_tag
		   eval_lut16
		   eval_pre_table
		   dump_profile
    )pbdoc";

	m.def("set_log_error_handler", [](py::function handler) {
		ERROR_HANDLER = handler;
		cmsSetLogErrorHandler(CmmLogErrorHandler);
	}, "handler"_a, R"pbdoc(
		Set log error handler.

		Parameters
		----------
		handler: Callable[[uint32, str], None]
			uint32:
				cmsERROR_UNDEFINED           0
				cmsERROR_FILE                1
				cmsERROR_RANGE               2
				cmsERROR_INTERNAL            3
				cmsERROR_NULL                4
				cmsERROR_READ                5
				cmsERROR_SEEK                6
				cmsERROR_WRITE               7
				cmsERROR_UNKNOWN_EXTENSION   8
				cmsERROR_COLORSPACE_CHECK    9
				cmsERROR_ALREADY_DEFINED     10
				cmsERROR_BAD_SIGNATURE       11
				cmsERROR_CORRUPTION_DETECTED 12
				cmsERROR_NOT_SUITABLE        13
			str: Error message
	)pbdoc");

	PY_ATTR_PT(cmsERROR_UNDEFINED);
	PY_ATTR_PT(cmsERROR_FILE);
	PY_ATTR_PT(cmsERROR_RANGE);
	PY_ATTR_PT(cmsERROR_INTERNAL);
	PY_ATTR_PT(cmsERROR_NULL);
	PY_ATTR_PT(cmsERROR_READ);
	PY_ATTR_PT(cmsERROR_SEEK);
	PY_ATTR_PT(cmsERROR_WRITE);
	PY_ATTR_PT(cmsERROR_UNKNOWN_EXTENSION);
	PY_ATTR_PT(cmsERROR_COLORSPACE_CHECK);
	PY_ATTR_PT(cmsERROR_ALREADY_DEFINED);
	PY_ATTR_PT(cmsERROR_BAD_SIGNATURE);
	PY_ATTR_PT(cmsERROR_CORRUPTION_DETECTED);
	PY_ATTR_PT(cmsERROR_NOT_SUITABLE);

	m.def("unset_log_error_handler", [](void) {
		cmsSetLogErrorHandler(NULL);
		ERROR_HANDLER = py::function();
	}, R"pbdoc(
		Unset log error handler.
	)pbdoc");

	m.def("open_profile_from_mem", [](py::bytes profile_content) {
		auto s = (std::string)profile_content;
		cmsHPROFILE hp = cmsOpenProfileFromMem(s.c_str(), (cmsUInt32Number)s.length());
		if (!hp) {
			return (cmsHPROFILE)NULL;
		}
		return hp;
	}, "profile_content"_a, R"pbdoc(
		Opens ICC profile from memory.

		Parameters
		----------
		profile_content: bytes

		Returns
		-------
		PyCapsule
			Profile handle. None if error.
	)pbdoc");

	m.def("close_profile", [](cmsHPROFILE hp) {
		cmsCloseProfile(hp);
	}, "hprofile"_a, R"pbdoc(
		Closes ICC profile.

		Parameters
		----------
		hp: PyCapsule
			Profile handle
	)pbdoc");

	m.def("get_device_class", [](cmsHPROFILE hp) {
		return (int)cmsGetDeviceClass(hp);
	}, "hprofile"_a, R"pbdoc(
		Gets device class of a profile.

		Parameters
		----------
		hp: PyCapsule
			Profile handle

		Returns
		-------
		int
			cmsSigInputClass 0x73636E72 'scnr'
			cmsSigDisplayClass 0x6D6E7472 'mntr'
			cmsSigOutputClass 0x70727472 'prtr'
			cmsSigLinkClass 0x6C696E6B 'link'
			cmsSigAbstractClass 0x61627374 'abst'
			cmsSigColorSpaceClass 0x73706163 'spac'
			cmsSigNamedColorClass 0x6e6d636c 'nmcl' 
	)pbdoc");

	PY_ATTR_ENUM(cmsSigInputClass);
	PY_ATTR_ENUM(cmsSigDisplayClass);
	PY_ATTR_ENUM(cmsSigOutputClass);
	PY_ATTR_ENUM(cmsSigLinkClass);
	PY_ATTR_ENUM(cmsSigAbstractClass);
	PY_ATTR_ENUM(cmsSigColorSpaceClass);
	PY_ATTR_ENUM(cmsSigNamedColorClass);

	m.def("get_color_space", [](cmsHPROFILE hp) {
		return (int)cmsGetColorSpace(hp);
	}, "hprofile"_a, R"pbdoc(
		Gets color space.

		Parameters
		----------
		hp: PyCapsule
			Profile handle

		Returns
		-------
		int
			cmsSigXYZData 0x58595A20 'XYZ '
			cmsSigLabData 0x4C616220 'Lab '
			cmsSigLuvData 0x4C757620 'Luv '
			cmsSigYCbCrData 0x59436272 'YCbr'
			cmsSigYxyData 0x59787920 'Yxy '
			cmsSigRgbData 0x52474220 'RGB '
			cmsSigGrayData 0x47524159 'GRAY'
			cmsSigHsvData 0x48535620 'HSV '
			cmsSigHlsData 0x484C5320 'HLS '
			cmsSigCmykData 0x434D594B 'CMYK'
			cmsSigCmyData 0x434D5920 'CMY '
			cmsSigMCH1Data 0x4D434831 'MCH1'
			cmsSigMCH2Data 0x4D434832 'MCH2'
			cmsSigMCH3Data 0x4D434833 'MCH3'
			cmsSigMCH4Data 0x4D434834 'MCH4'
			cmsSigMCH5Data 0x4D434835 'MCH5'
			cmsSigMCH6Data 0x4D434836 'MCH6'
			cmsSigMCH7Data 0x4D434837 'MCH7'
			cmsSigMCH8Data 0x4D434838 'MCH8'
			cmsSigMCH9Data 0x4D434839 'MCH9'
			cmsSigMCHAData 0x4D43483A 'MCHA'
			cmsSigMCHBData 0x4D43483B 'MCHB'
			cmsSigMCHCData 0x4D43483C 'MCHC'
			cmsSigMCHDData 0x4D43483D 'MCHD'
			cmsSigMCHEData 0x4D43483E 'MCHE'
			cmsSigMCHFData 0x4D43483F 'MCHF'
			cmsSigNamedData 0x6e6d636c 'nmcl'
			cmsSig1colorData 0x31434C52 '1CLR'
			cmsSig2colorData 0x32434C52 '2CLR'
			cmsSig3colorData 0x33434C52 '3CLR'
			cmsSig4colorData 0x34434C52 '4CLR'
			cmsSig5colorData 0x35434C52 '5CLR'
			cmsSig6colorData 0x36434C52 '6CLR'
			cmsSig7colorData 0x37434C52 '7CLR'
			cmsSig8colorData 0x38434C52 '8CLR'
			cmsSig9colorData 0x39434C52 '9CLR'
			cmsSig10colorData 0x41434C52 'ACLR'
			cmsSig11colorData 0x42434C52 'BCLR'
			cmsSig12colorData 0x43434C52 'CCLR'
			cmsSig13colorData 0x44434C52 'DCLR'
			cmsSig14colorData 0x45434C52 'ECLR'
			cmsSig15colorData 0x46434C52 'FCLR'
			cmsSigLuvKData 0x4C75764B 'LuvK'
	)pbdoc");

	PY_ATTR_ENUM(cmsSigXYZData);
	PY_ATTR_ENUM(cmsSigLabData);
	PY_ATTR_ENUM(cmsSigLuvData);
	PY_ATTR_ENUM(cmsSigYCbCrData);
	PY_ATTR_ENUM(cmsSigYxyData);
	PY_ATTR_ENUM(cmsSigRgbData);
	PY_ATTR_ENUM(cmsSigGrayData);
	PY_ATTR_ENUM(cmsSigHsvData);
	PY_ATTR_ENUM(cmsSigHlsData);
	PY_ATTR_ENUM(cmsSigCmykData);
	PY_ATTR_ENUM(cmsSigCmyData);
	PY_ATTR_ENUM(cmsSigMCH1Data);
	PY_ATTR_ENUM(cmsSigMCH2Data);
	PY_ATTR_ENUM(cmsSigMCH3Data);
	PY_ATTR_ENUM(cmsSigMCH4Data);
	PY_ATTR_ENUM(cmsSigMCH5Data);
	PY_ATTR_ENUM(cmsSigMCH6Data);
	PY_ATTR_ENUM(cmsSigMCH7Data);
	PY_ATTR_ENUM(cmsSigMCH8Data);
	PY_ATTR_ENUM(cmsSigMCH9Data);
	PY_ATTR_ENUM(cmsSigMCHAData);
	PY_ATTR_ENUM(cmsSigMCHBData);
	PY_ATTR_ENUM(cmsSigMCHCData);
	PY_ATTR_ENUM(cmsSigMCHDData);
	PY_ATTR_ENUM(cmsSigMCHEData);
	PY_ATTR_ENUM(cmsSigMCHFData);
	PY_ATTR_ENUM(cmsSigNamedData);
	PY_ATTR_ENUM(cmsSig1colorData);
	PY_ATTR_ENUM(cmsSig2colorData);
	PY_ATTR_ENUM(cmsSig3colorData);
	PY_ATTR_ENUM(cmsSig4colorData);
	PY_ATTR_ENUM(cmsSig5colorData);
	PY_ATTR_ENUM(cmsSig6colorData);
	PY_ATTR_ENUM(cmsSig7colorData);
	PY_ATTR_ENUM(cmsSig8colorData);
	PY_ATTR_ENUM(cmsSig9colorData);
	PY_ATTR_ENUM(cmsSig10colorData);
	PY_ATTR_ENUM(cmsSig11colorData);
	PY_ATTR_ENUM(cmsSig12colorData);
	PY_ATTR_ENUM(cmsSig13colorData);
	PY_ATTR_ENUM(cmsSig14colorData);
	PY_ATTR_ENUM(cmsSig15colorData);
	PY_ATTR_ENUM(cmsSigLuvKData);

	m.def("get_available_b2an_list", [](cmsHPROFILE hp) {
		void *p0 = NULL, *p1 = NULL, *p2 = NULL;
		if (cmsIsTag(hp, cmsSigBToA0Tag)) {
			p0 = cmsReadTag(hp, cmsSigBToA0Tag);
		}
		if (cmsIsTag(hp, cmsSigBToA1Tag)) {
			p1 = cmsReadTag(hp, cmsSigBToA1Tag);
		}
		if (cmsIsTag(hp, cmsSigBToA2Tag)) {
			p2 = cmsReadTag(hp, cmsSigBToA2Tag);
		}
		std::vector<std::string> r;
		if (p1) {
			r.push_back(std::string("B2A1"));
		}
		if (p0 != p1) {
			r.push_back(std::string("B2A0"));
		}
		if (p2 != p1) {
			r.push_back(std::string("B2A2"));
		}
		return r;
	}, "hprofile"_a, R"pbdoc(
		Gets available B2An list

		Parameters
		----------
		hp: PyCapsule
			Profile handle

		Returns
		-------
		[str]
			'B2A0', 'B2A1', and/or 'B2A2'
	)pbdoc");

	m.def("create_srgb_profile", []() {
		return cmsCreate_sRGBProfile();
	}, R"pbdoc(
		Creates sRGB profile.
	)pbdoc");

	m.def("get_profile_description", [](cmsHPROFILE hp) -> py::object {
		int len = cmsGetProfileInfoASCII(hp, cmsInfoDescription, "eng", "USA", NULL, 0);
		if (!len) {
			return py::cast<py::none>(Py_None);
		}

		std::unique_ptr<char[]> buffer(new char[len]);
		if (!cmsGetProfileInfoASCII(hp, cmsInfoDescription, "eng", "USA", buffer.get(), len))
			return py::cast<py::none>(Py_None);

		auto ret = std::string(buffer.get());
		stripNonAscii(ret);
		return (py::str)ret;
	}, "hprofile"_a, R"pbdoc(
		Gets profile description. eng/USA only.

		Parameters
		----------
		hp: PyCapsule
			Profile handle

		Returns
		-------
		Optional[str]
			None if error
	)pbdoc");

	m.def("create_transform", [](cmsHPROFILE src_hp, int src_format, cmsHPROFILE trg_hp, int trg_format, int intent, int flags) {
		return cmsCreateTransform(src_hp, src_format, trg_hp, trg_format, intent, flags);
	}, "src_hp"_a, "src_format"_a, "trg_hp"_a, "trg_format"_a, "intent"_a, "flags"_a, R"pbdoc(
		Creates transform.

		Parameters
		----------
		src_hp: PyCapsule
			Profile handle of source

		src_format: int
			Source format

		trg_hp: PyCapsule
			Profile handle of target

		trg_format: int
			Target format

		intent: int
			Color conversion intent
			INTENT_PERCEPTUAL				0
			INTENT_RELATIVE_COLORIMETRIC	1
			INTENT_SATURATION				2
			INTENT_ABSOLUTE_COLORIMETRIC	3

		flags: int
			Conversion flag
			cmsFLAGS_BLACKPOINTCOMPENSATION	0x2000
			cmsFLAGS_HIGHRESPRECALC			0x0400
			cmsFLAGS_NULLTRANSFORM			0x0200
			cmsFLAGS_NOOPTIMIZE				0x0100
			cmsFLAGS_KEEP_SEQUENCE			0x0080

		Returns
		-------
		PyCapsule
			Transform handle
	)pbdoc");

	PY_ATTR_PT(INTENT_PERCEPTUAL);
	PY_ATTR_PT(INTENT_RELATIVE_COLORIMETRIC);
	PY_ATTR_PT(INTENT_SATURATION);
	PY_ATTR_PT(INTENT_ABSOLUTE_COLORIMETRIC);

	PY_ATTR_PT(cmsFLAGS_BLACKPOINTCOMPENSATION);
	PY_ATTR_PT(cmsFLAGS_HIGHRESPRECALC);
	PY_ATTR_PT(cmsFLAGS_NULLTRANSFORM);
	PY_ATTR_PT(cmsFLAGS_NOOPTIMIZE);
	PY_ATTR_PT(cmsFLAGS_KEEP_SEQUENCE);

	m.def("create_proofing_transform", [](cmsHPROFILE src_hp, int src_format, cmsHPROFILE trg_hp, int trg_format, cmsHPROFILE proof_hp, int intent, int proof_intent, int flags) {
		return cmsCreateProofingTransform(src_hp, src_format, trg_hp, trg_format, proof_hp, intent, proof_intent, flags | cmsFLAGS_SOFTPROOFING);
	}, "src_hp"_a, "src_format"_a, "trg_hp"_a, "trg_format"_a, "proof_hp"_a, "intent"_a, "proof_intent"_a, "flags"_a, R"pbdoc(
		Creates soft proof transform.

		Parameters
		----------
		src_hp: PyCapsule
			Profile handle of source

		src_format: int
			Source format

		trg_hp: PyCapsule
			Profile handle of target

		trg_format: int
			Target format

		proof_hp: PyCapsule
			Profile handle to proof

		intent: int
			Color conversion intent
			INTENT_PERCEPTUAL				0
			INTENT_RELATIVE_COLORIMETRIC	1
			INTENT_SATURATION				2
			INTENT_ABSOLUTE_COLORIMETRIC	3

		flags: int
			Conversion flag
			cmsFLAGS_BLACKPOINTCOMPENSATION	0x2000
			cmsFLAGS_HIGHRESPRECALC			0x0400
			cmsFLAGS_NULLTRANSFORM			0x0200
			cmsFLAGS_NOOPTIMIZE				0x0100
			cmsFLAGS_KEEP_SEQUENCE			0x0080
			cmsFLAGS_GAMUTCHECK				0x1000
			cmsFLAGS_SOFTPROOFING			0x4000

		Returns
		-------
		PyCapsule
			Transform handle
	)pbdoc");

	PY_ATTR_PT(cmsFLAGS_GAMUTCHECK);
	PY_ATTR_PT(cmsFLAGS_SOFTPROOFING);

	m.def("set_alarm_codes", [](py::array_t<cmsUInt16Number> alarm_codes) {
		py::buffer_info alarm_codes_bi = alarm_codes.request();
		if (alarm_codes_bi.ndim != 1 || alarm_codes_bi.shape[0] != cmsMAXCHANNELS) {
			return 0;
		}
		auto alarm_codes_ptr = static_cast<cmsUInt16Number *>(alarm_codes_bi.ptr);
		cmsSetAlarmCodes(alarm_codes_ptr);
		return -1;
	}, "alarm_codes"_a, R"pbdoc(
		Sets the global codes used to mark out-out-gamut on Proofing transforms. Values are meant to be encoded in 16 bits.
		Set cmsFLAGS_GAMUTCHECK and cmsFLAGS_SOFTPROOFING in create_proofing_transform().

		Parameters
		----------
		alarm_codes: [uint16], shape=(16)

		Returns
		-------
		int
			0 if fail
	)pbdoc");
	
	m.def("get_transform_formatter", [](int fl, int pt, int n_ch, int n_byte, int swap, int extra) {
		return (FLOAT_SH(fl) | COLORSPACE_SH(pt) | CHANNELS_SH(n_ch) | BYTES_SH(n_byte) | DOSWAP_SH(swap) | EXTRA_SH(extra));
	}, "is_float"_a, "pixel_type"_a, "n_ch"_a, "n_byte"_a, "swap"_a, "extra"_a, R"pbdoc(
		Calculates transform formatter.

		Parameters
		----------
		is_float: int
			0 or 1
		pixel_type: int
			Colorspace type
			PT_ANY       0    // Don't check colorspace
			PT_GRAY      3
			PT_RGB       4
			PT_CMY       5
			PT_CMYK      6
			PT_YCbCr     7
			PT_YUV       8      // Lu'v'
			PT_XYZ       9
			PT_Lab       10
			PT_YUVK      11     // Lu'v'K
			PT_HSV       12
			PT_HLS       13
			PT_Yxy       14

		n_ch: int
			Number of channel. Alpha channel is not included here.

		n_byte: int
			Number of byte of a channel. uint16 should be 2.

		swap: int
			1 if BGR order, not RGB
		
		extra: int
			1 if there is alpha channel
	)pbdoc");

	PY_ATTR_PT(PT_ANY);
	PY_ATTR_PT(PT_GRAY);
	PY_ATTR_PT(PT_RGB);
	PY_ATTR_PT(PT_CMY);
	PY_ATTR_PT(PT_CMYK);
	PY_ATTR_PT(PT_YCbCr);
	PY_ATTR_PT(PT_YUV);
	PY_ATTR_PT(PT_XYZ);
	PY_ATTR_PT(PT_Lab);
	PY_ATTR_PT(PT_YUVK);
	PY_ATTR_PT(PT_HSV);
	PY_ATTR_PT(PT_HLS);
	PY_ATTR_PT(PT_Yxy);

	m.def("delete_transform", [](cmsHTRANSFORM ht) {
		return cmsDeleteTransform(ht);
	}, "htransform"_a, R"pbdoc(
		Deletes transform.

		Parameters
		----------
		htransform: PyCapsule
			Transform handle
	)pbdoc");

	m.def("do_transform_8_8", &do_transform<cmsUInt8Number, cmsUInt8Number>,
		"htransform"_a, "input_buf"_a, "output_buf"_a, "num_pixel"_a,
	R"pbdoc(
		Does transform from uint8 to uint8.

		Parameters
		----------
		htransform: PyCapsule
			Transform handle

		input_buf: ndarray[uint8]
		output_buf: ndarray[uint8]
		num_pixel: int
	)pbdoc");

	m.def("do_transform_16_8", &do_transform<cmsUInt16Number, cmsUInt8Number>,
		"htransform"_a, "input_buf"_a, "output_buf"_a, "num_pixel"_a,
		R"pbdoc(
		Does transform from uint16 to uint8.

		Parameters
		----------
		htransform: PyCapsule
			Transform handle

		input_buf: ndarray[uint16]
		output_buf: ndarray[uint8]
		num_pixel: int
	)pbdoc");

	m.def("do_transform_8_16", &do_transform<cmsUInt8Number, cmsUInt16Number>,
		"htransform"_a, "input_buf"_a, "output_buf"_a, "num_pixel"_a,
		R"pbdoc(
		Does transform from uint8 to uint16.

		Parameters
		----------
		htransform: PyCapsule
			Transform handle

		input_buf: ndarray[uint8]
		output_buf: ndarray[uint16]
		num_pixel: int
	)pbdoc");

	m.def("do_transform_16_16", &do_transform<cmsUInt16Number, cmsUInt16Number>,
		"htransform"_a, "input_buf"_a, "output_buf"_a, "num_pixel"_a,
		R"pbdoc(
		Does transform from uint16 to uint16.

		Parameters
		----------
		htransform: PyCapsule
			Transform handle

		input_buf: ndarray[uint16]
		output_buf: ndarray[uint16]
		num_pixel: int
	)pbdoc");

	m.def("create_partial_profile", [](std::string desc, std::string cprt, bool is_glossy, py::array_t<double> wtpt) {
		py::buffer_info wtpt_bi = wtpt.request();
		if (wtpt_bi.ndim != 1 || wtpt_bi.shape[0] != 3) {
			return (cmsHPROFILE)NULL;
		}
		auto hProfile = cmsCreateProfilePlaceholder(NULL);
		if (!hProfile) {
			return (cmsHPROFILE)NULL;
		}
		auto icc = (_cmsICCPROFILE*)hProfile;

		cmsSetProfileVersion(hProfile, 2.4);
		cmsSetDeviceClass(hProfile, cmsSigOutputClass);
		cmsSetColorSpace(hProfile, cmsSigRgbData);
		cmsSetPCS(hProfile, cmsSigLabData);
		// device manufacturer: null
		// device model: null
		auto attr = cmsReflective | (is_glossy ? cmsGlossy : cmsMatte);
		cmsSetHeaderAttributes(hProfile, attr);
		// rendering intent: zero
		// illuminant?
		icc->creator = _cmsAdjustEndianess32(0x5a59474f); // 'ZYGO'

		if (!setAsciiTag(desc, hProfile, cmsSigProfileDescriptionTag)) {
			cmsCloseProfile(hProfile);
			return (cmsHPROFILE)NULL;
		}
		if (!setAsciiTag(cprt, hProfile, cmsSigCopyrightTag)) {
			cmsCloseProfile(hProfile);
			return (cmsHPROFILE)NULL;
		}
		auto wtpt_ptr = static_cast<double *>(wtpt_bi.ptr);
		cmsCIEXYZ wtptXYZ;
		wtptXYZ.X = wtpt_ptr[0];
		wtptXYZ.Y = wtpt_ptr[1];
		wtptXYZ.Z = wtpt_ptr[2];
		if (!cmsWriteTag(hProfile, cmsSigMediaWhitePointTag, &wtptXYZ)) {
			cmsCloseProfile(hProfile);
			return (cmsHPROFILE)NULL;
		}
		return hProfile;
	}, "desc"_a, "cprt"_a, "is_glossy"_a, "wtpt"_a, R"pbdoc(
		Creates a partial profile. Partial profile should be completed before dump_profile().

		Parameters
		----------
		desc: str
			Description string
		cprt: str
			Copyright string
		is_glossy: bool
		wtpt: ndarray[float64]
			XYZ values of white point

		Returns
		-------
		PyCapsule
			Profile handle
	)pbdoc");

	m.def("add_lut16", [](cmsHPROFILE hp, std::string tag, int n_out_ch,
		py::array_t<cmsUInt16Number> clut, py::array_t<cmsUInt16Number> pre_table, py::array_t<cmsUInt16Number> post_table) {
		const int N_IN_CH = 3;
		auto pre_table_bi = pre_table.request();
		auto clut_bi = clut.request();
		auto post_table_bi = post_table.request();
		if (pre_table_bi.ndim != 2 || pre_table_bi.shape[1] != N_IN_CH
			|| clut_bi.ndim != N_IN_CH + 1 || clut_bi.shape[3] != n_out_ch 
			|| clut_bi.shape[0] != clut_bi.shape[1] || clut_bi.shape[1] != clut_bi.shape[2]
			|| post_table_bi.ndim != 2 || post_table_bi.shape[1] != n_out_ch) {
			return 0;
		}
		auto lut_tag_map = get_lut_tag_map();
		if (!lut_tag_map.count(tag)) {
			return 0;
		}
		cmsTagSignature tag_sig = lut_tag_map[tag];
		auto n_clut_point = clut_bi.shape[0];
		auto pre_c = pre_table.unchecked<2>();
		auto clut_c = clut.unchecked<N_IN_CH + 1>();
		auto post_c = post_table.unchecked<2>();
		auto pipeline = cmsPipelineAlloc(NULL, N_IN_CH, n_out_ch);
		if (!pipeline) {
			return 0;
		}
		auto _table_stage = [](py::detail::unchecked_reference<cmsUInt16Number, 2> table_c) {
			auto nEntries = table_c.shape(0);
			auto nCh = table_c.shape(1);
			std::vector<cmsToneCurve *> tc(nCh);
			for (int i = 0; i < nCh; i++) {
				std::vector <cmsUInt16Number> values(nEntries);
				for (int ii = 0; ii < nEntries; ii++) {
					values[ii] = table_c(ii, i);
				}
				tc[i] = cmsBuildTabulatedToneCurve16(NULL, (cmsUInt32Number)nEntries, values.data());
			}
			auto stage = cmsStageAllocToneCurves(NULL, (cmsUInt32Number)nCh, tc.data());
			return stage;
		};
		auto pre_stage = _table_stage(pre_c);
		cmsPipelineInsertStage(pipeline, cmsAT_BEGIN, pre_stage);
		std::vector<cmsUInt16Number> clut_table(n_clut_point * n_clut_point * n_clut_point * n_out_ch);
		for (int i = 0; i < n_clut_point; i++) {
			for (int ii = 0; ii < n_clut_point; ii++) {
				for (int iii = 0; iii < n_clut_point; iii++) {
					auto pos = i * n_clut_point * n_clut_point + ii * n_clut_point + iii;
					for (int a = 0; a < n_out_ch; a++) {
						clut_table[pos * n_out_ch + a] = clut_c(i, ii, iii, a);
					}
					int r = clut_c(i, ii, iii, 0);
					int g = clut_c(i, ii, iii, 1);
					int b = clut_c(i, ii, iii, 2);
					if (r > 32768 && g > 32768 && b > 32768) {
						int k = 0;
						k++;
					}
				}
			}
		}
		auto clut_stage = cmsStageAllocCLut16bit(NULL, (cmsUInt32Number)n_clut_point, N_IN_CH, n_out_ch, clut_table.data());
		cmsPipelineInsertStage(pipeline, cmsAT_END, clut_stage);
		auto post_stage = _table_stage(post_c);
		cmsPipelineInsertStage(pipeline, cmsAT_END, post_stage);
		if (!cmsWriteTag(hp, tag_sig, (void*)pipeline)) {
			cmsPipelineFree(pipeline);
			return 0;
		}
		cmsPipelineFree(pipeline);
		return -1;
	}, "hprofile"_a, "tag"_a, "n_out_ch"_a, "clut"_a, "pre_table"_a, "post_table"_a, R"pbdoc(
		Adds a lut16 to a profile.

		Parameters
		----------
		hprofile: PyCapsule
			Profile handle
		tag: str
			AnBm, BnAm, or 'gamt'
		n_out_ch: int
			Number of output channel
		clut: ndarray[uint16]
			CLUT
		pre_table: ndarray[uint16]
			Tone curve before CLUT stage
		post_table: ndarray[uint16]
			Tone curve after CLUT stage

		Returns
		-------
		int
			0 if fail
	)pbdoc");

	m.def("link_tag", [](cmsHPROFILE hp, std::string link_tag, std::string dest_tag) {
		auto lut_tag_map = get_lut_tag_map();
		if (!lut_tag_map.count(link_tag) || !lut_tag_map.count(dest_tag)) {
			return 0;
		}
		return cmsLinkTag(hp, lut_tag_map[link_tag], lut_tag_map[dest_tag]);
	}, "hprofile"_a, "link_tag"_a, "dest_tag"_a, R"pbdoc(
		Links a tag to another tag.

		Parameters
		----------
		hprofile: PyCapsule
			Profile handle
		link_tag: str
			AnBm or BnAm
		dest_tag: str
			AnBm or BnAm

		Returns
		-------
		int
			0 if fail
	)pbdoc");

	m.def("eval_lut16", [](cmsHPROFILE hp, std::string tag, py::array_t<cmsUInt16Number> input_array, py::array_t<cmsUInt16Number> output_array) {
		auto lut_tag_map = get_lut_tag_map();
		if (!lut_tag_map.count(tag)) {
			return 0;
		}
		cmsPipeline *pipeline = (cmsPipeline *)cmsReadTag(hp, lut_tag_map[tag]);
		int inCh = cmsPipelineInputChannels(pipeline);
		int outCh = cmsPipelineOutputChannels(pipeline);
		auto input_array_bi = input_array.request();
		auto output_array_bi = output_array.request();
		if (input_array_bi.ndim != 2 || input_array_bi.shape[1] != inCh
			|| output_array_bi.ndim != 2 || output_array_bi.shape[1] != outCh 
			|| input_array_bi.shape[0] != output_array_bi.shape[0]) {
			return 0;
		}
		auto input_c = input_array.unchecked<2>();
		auto output_c = output_array.mutable_unchecked<2>();
		for (int i = 0; i < input_array_bi.shape[0]; i++) {
			cmsPipelineEval16(input_c.data(i, 0), output_c.mutable_data(i, 0), pipeline);
		}
		return -1;
	}, "hprofile"_a, "tag"_a, "input_array"_a, py::arg("output_array").noconvert(), R"pbdoc(
		Evaluates lut16 by input_array.

		Parameters
		----------
		hprofile: PyCapsule
			Profile handle
		tag: str
			AnBm, BnAm, or 'gamt'
		input_array: ndarray[uint16]
		output_array: ndarray[uint16]

		Returns
		-------
		int
			0 if fail
	)pbdoc");

	m.def("eval_pre_table", [](cmsHPROFILE hp, std::string tag, py::array_t<cmsUInt16Number> input_array, py::array_t<cmsUInt16Number> output_array) {
		auto lut_tag_map = get_lut_tag_map();
		if (!lut_tag_map.count(tag)) {
			return 0;
		}
		cmsPipeline *pipeline = (cmsPipeline *)cmsReadTag(hp, lut_tag_map[tag]);
		cmsStage *preStage = cmsPipelineGetPtrToFirstStage(pipeline);
		_cmsStageToneCurvesData* tcData = (_cmsStageToneCurvesData*)preStage->Data;
		auto input_array_bi = input_array.request();
		auto output_array_bi = output_array.request();
		if (input_array_bi.ndim != 2 || input_array_bi.shape[1] != tcData->nCurves
			|| output_array_bi.ndim != 2 || output_array_bi.shape[1] != tcData->nCurves
			|| input_array_bi.shape[0] != output_array_bi.shape[0]) {
			return 0;
		}
		auto input_c = input_array.unchecked<2>();
		auto output_c = output_array.mutable_unchecked<2>();
		for (cmsUInt32Number i = 0; i < tcData->nCurves; i++) {
			cmsToneCurve *tc = tcData->TheCurves[i];
			for (int ii = 0; ii < input_array_bi.shape[0]; ii++) {
				*output_c.mutable_data(ii, i) = cmsEvalToneCurve16(tc, input_c(ii, i));
			}
		}
		return -1;
	}, "hprofile"_a, "tag"_a, "input_array"_a, py::arg("output_array").noconvert(), R"pbdoc(
		Evaluates pre_table of the tag by input_array.

		Parameters
		----------
		hprofile: PyCapsule
			Profile handle
		tag: str
			AnBm, BnAm, or 'gamt'
		input_array: ndarray[uint16]
		output_array: ndarray[uint16]

		Returns
		-------
		int
			0 if fail
	)pbdoc");

	m.def("dump_profile", [](cmsHPROFILE hp) {
		cmsUInt32Number bytesNeeded;
		cmsSaveProfileToMem(hp, NULL, &bytesNeeded);
		auto buf = std::vector<char>(bytesNeeded);
		cmsSaveProfileToMem(hp, buf.data(), &bytesNeeded);
		return py::bytes(buf.data(), bytesNeeded);
	}, "hprofile"_a, R"pbdoc(
		Dumps a profile.

		Parameters
		----------
		hprofile: PyCapsule
			Profile handle

		Returns
		-------
		bytes
			Profile content
	)pbdoc");

	m.attr("__lcms_version__") = LCMS_VERSION;

#ifdef VERSION_INFO
    m.attr("__version__") = VERSION_INFO;
#else
    m.attr("__version__") = "dev";
#endif
}
