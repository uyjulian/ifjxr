/////////////////////////////////////////////
//                                         //
//    Copyright (C) 2019-2019 Julian Uy    //
//  https://sites.google.com/site/awertyb  //
//                                         //
//   See details of license at "LICENSE"   //
//                                         //
/////////////////////////////////////////////

#include "extractor.h"
#include <JXRGlue.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

const char *plugin_info[4] = {
    "00IN",
    "JXR Plugin for Susie Image Viewer",
    "*.jxr",
    "JXR file (*.jxr)",
};

const int header_size = 64;

typedef struct {
	const uint8_t *buf;
	size_t size;
	size_t cur;
} data_pointer;

static ERR JXR_close(struct WMPStream **ppWS) { return WMP_errSuccess; }
static Bool JXR_EOS(struct WMPStream *pWS) {
	data_pointer *src = ((data_pointer *)(pWS->state.pvObj));
	return src->cur >= src->size;
}
static ERR JXR_read(struct WMPStream *pWS, void *pv, size_t cb) {
	data_pointer *src = ((data_pointer *)(pWS->state.pvObj));
	if (src->cur + cb > src->size)
		return WMP_errFileIO;
	memcpy(pv, src->buf + src->cur, cb);
	src->cur += cb;
	return WMP_errSuccess;
}
static ERR JXR_write(struct WMPStream *pWS, const void *pv, size_t cb) {
	return WMP_errFileIO;
}
static ERR JXR_set_pos(struct WMPStream *pWS, size_t offPos) {
	data_pointer *src = ((data_pointer *)(pWS->state.pvObj));
	if (offPos > src->size)
		return WMP_errFileIO;
	src->cur = offPos;
	return WMP_errSuccess;
}
static ERR JXR_get_pos(struct WMPStream *pWS, size_t *poffPos) {
	data_pointer *src = ((data_pointer *)(pWS->state.pvObj));
	*poffPos = src->cur;
	return WMP_errSuccess;
}

static uint32_t GetStride(const uint32_t width, const uint32_t bitCount) {
	const uint32_t byteCount = bitCount / 8;
	const uint32_t stride = (width * byteCount + 3) & ~3;
	return stride;
}

#define SAFE_CALL(func)                                                        \
	if (Failed(err = (func))) {                                                \
		return -1;                                                             \
	}

int getBMPFromJXR(const uint8_t *input_data, size_t file_size,
                  BITMAPFILEHEADER *bitmap_file_header,
                  BITMAPINFOHEADER *bitmap_info_header, uint8_t **data) {

	data_pointer d;
	d.buf = input_data;
	d.size = file_size;
	d.cur = 0;

	PKImageDecode *pDecoder = NULL;
	struct WMPStream *pStream = NULL;
	ERR err;
	const PKIID *pIID = NULL;
	SAFE_CALL(GetImageDecodeIID(".jxr", &pIID));
	pStream = (struct WMPStream *)calloc(1, sizeof(struct WMPStream));
	pStream->state.pvObj = &d;
	pStream->Close = JXR_close;
	pStream->EOS = JXR_EOS;
	pStream->Read = JXR_read;
	pStream->Write = JXR_write;
	pStream->SetPos = JXR_set_pos;
	pStream->GetPos = JXR_get_pos;
	SAFE_CALL(PKCodecFactory_CreateCodec(pIID, (void **)&pDecoder));
	SAFE_CALL(pDecoder->Initialize(pDecoder, pStream));
	pDecoder->fStreamOwner = !0;

	PKPixelFormatGUID srcFormat;
	pDecoder->GetPixelFormat(pDecoder, &srcFormat);
	PKPixelInfo PI;
	PI.pGUIDPixFmt = &srcFormat;
	PixelFormatLookup(&PI, LOOKUP_FORWARD);

	pDecoder->WMP.wmiSCP.bfBitstreamFormat = SPATIAL;
	if (!!(PI.grBit & PK_pixfmtHasAlpha))
		pDecoder->WMP.wmiSCP.uAlphaMode = 2;
	else
		pDecoder->WMP.wmiSCP.uAlphaMode = 0;
	pDecoder->WMP.wmiSCP.sbSubband = SB_ALL;
	pDecoder->WMP.bIgnoreOverlap = FALSE;
	pDecoder->WMP.wmiI.cfColorFormat = PI.cfColorFormat;
	pDecoder->WMP.wmiI.bdBitDepth = PI.bdBitDepth;
	pDecoder->WMP.wmiI.cBitsPerUnit = PI.cbitUnit;
	pDecoder->WMP.wmiI.cThumbnailWidth = pDecoder->WMP.wmiI.cWidth;
	pDecoder->WMP.wmiI.cThumbnailHeight = pDecoder->WMP.wmiI.cHeight;
	pDecoder->WMP.wmiI.bSkipFlexbits = FALSE;
	pDecoder->WMP.wmiI.cROILeftX = 0;
	pDecoder->WMP.wmiI.cROITopY = 0;
	pDecoder->WMP.wmiI.cROIWidth = pDecoder->WMP.wmiI.cThumbnailWidth;
	pDecoder->WMP.wmiI.cROIHeight = pDecoder->WMP.wmiI.cThumbnailHeight;
	pDecoder->WMP.wmiI.oOrientation = O_NONE;
	pDecoder->WMP.wmiI.cPostProcStrength = 0;
	pDecoder->WMP.wmiSCP.bVerbose = FALSE;
	int width = 0;
	int height = 0;
	pDecoder->GetSize(pDecoder, &width, &height);
	if (width == 0 || height == 0)
		return -1;
	const uint32_t stride = GetStride((uint32_t)width, (uint32_t)32);
	PKRect rect = {0, 0, width, height};
	int bit_width = width * 4;
	int bit_length = bit_width;
	uint8_t *bitmap_data =
	    (uint8_t *)malloc(sizeof(uint8_t) * bit_length * height);
	if (!bitmap_data)
		return -1;
	memset(bitmap_data, 0, bit_length * height);
	uint8_t *buff = (uint8_t *)malloc(sizeof(uint8_t) * stride * height);
	if (!buff)
		return -1;
	int offset = 0;
	if (!IsEqualGUID(&srcFormat, &GUID_PKPixelFormat32bppRGBA)) {
		if (IsEqualGUID(&srcFormat, &GUID_PKPixelFormat24bppRGB)) {
			pDecoder->Copy(pDecoder, &rect, (uint8_t *)&buff[0], stride);
			for (int i = 0; i < height; i++) {
				void *scanline = bitmap_data + i * bit_length;
				uint8_t *d = (uint8_t *)scanline;
				uint8_t *s = (uint8_t *)&buff[offset];
				for (int x = 0; x < width; x++) {
					d[0] = s[2];
					d[1] = s[1];
					d[2] = s[0];
					d[3] = 0xff;
					d += 4;
					s += 3;
				}
				offset += stride;
			}
		} else {
			return -1;
		}
	} else {
		pDecoder->Copy(pDecoder, &rect, (uint8_t *)&buff[0], stride);
		for (int i = 0; i < height; i++) {
			memcpy(bitmap_data + i * bit_length, &buff[offset], bit_length);
			offset += stride;
		}
	}
	free(buff);
	if (pDecoder)
		pDecoder->Release(&pDecoder);
	if (pStream)
		free(pStream);

	*data = bitmap_data;

	memset(bitmap_file_header, 0, sizeof(BITMAPFILEHEADER));
	memset(bitmap_info_header, 0, sizeof(BITMAPINFOHEADER));

	bitmap_file_header->bfType = 'M' * 256 + 'B';
	bitmap_file_header->bfSize = sizeof(BITMAPFILEHEADER) +
	                             sizeof(BITMAPINFOHEADER) +
	                             sizeof(uint8_t) * bit_length * height;
	bitmap_file_header->bfOffBits =
	    sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bitmap_file_header->bfReserved1 = 0;
	bitmap_file_header->bfReserved2 = 0;

	bitmap_info_header->biSize = 40;
	bitmap_info_header->biWidth = width;
	bitmap_info_header->biHeight = height;
	bitmap_info_header->biPlanes = 1;
	bitmap_info_header->biBitCount = 32;
	bitmap_info_header->biCompression = 0;
	bitmap_info_header->biSizeImage = bitmap_file_header->bfSize;
	bitmap_info_header->biXPelsPerMeter = bitmap_info_header->biYPelsPerMeter =
	    0;
	bitmap_info_header->biClrUsed = 0;
	bitmap_info_header->biClrImportant = 0;
	return 0;
}

BOOL IsSupportedEx(const char *data) {
	const char header[] = {0x49, 0x49, 0xbc, 0x01};
	for (int i = 0; i < sizeof(header); i++) {
		if (header[i] == 0x00)
			continue;
		if (data[i] != header[i])
			return FALSE;
	}
	return TRUE;
}

int GetPictureInfoEx(size_t data_size, const char *data,
                     SusiePictureInfo *picture_info) {
	data_pointer d;
	d.buf = (uint8_t *)data;
	d.size = data_size;
	d.cur = 0;

	PKImageDecode *pDecoder = NULL;
	struct WMPStream *pStream = NULL;
	ERR err;
	const PKIID *pIID = NULL;
	SAFE_CALL(GetImageDecodeIID(".jxr", &pIID));
	pStream = (struct WMPStream *)calloc(1, sizeof(struct WMPStream));
	pStream->state.pvObj = &d;
	pStream->Close = JXR_close;
	pStream->EOS = JXR_EOS;
	pStream->Read = JXR_read;
	pStream->Write = JXR_write;
	pStream->SetPos = JXR_set_pos;
	pStream->GetPos = JXR_get_pos;
	SAFE_CALL(PKCodecFactory_CreateCodec(pIID, (void **)&pDecoder));
	SAFE_CALL(pDecoder->Initialize(pDecoder, pStream));
	pDecoder->fStreamOwner = !0;

	PKPixelFormatGUID srcFormat;
	pDecoder->GetPixelFormat(pDecoder, &srcFormat);
	PKPixelInfo PI;
	PI.pGUIDPixFmt = &srcFormat;
	PixelFormatLookup(&PI, LOOKUP_FORWARD);

	pDecoder->WMP.wmiSCP.bfBitstreamFormat = SPATIAL;
	if (!!(PI.grBit & PK_pixfmtHasAlpha))
		pDecoder->WMP.wmiSCP.uAlphaMode = 2;
	else
		pDecoder->WMP.wmiSCP.uAlphaMode = 0;
	pDecoder->WMP.wmiSCP.sbSubband = SB_ALL;
	pDecoder->WMP.bIgnoreOverlap = FALSE;
	pDecoder->WMP.wmiI.cfColorFormat = PI.cfColorFormat;
	pDecoder->WMP.wmiI.bdBitDepth = PI.bdBitDepth;
	pDecoder->WMP.wmiI.cBitsPerUnit = PI.cbitUnit;
	pDecoder->WMP.wmiI.cThumbnailWidth = pDecoder->WMP.wmiI.cWidth;
	pDecoder->WMP.wmiI.cThumbnailHeight = pDecoder->WMP.wmiI.cHeight;
	pDecoder->WMP.wmiI.bSkipFlexbits = FALSE;
	pDecoder->WMP.wmiI.cROILeftX = 0;
	pDecoder->WMP.wmiI.cROITopY = 0;
	pDecoder->WMP.wmiI.cROIWidth = pDecoder->WMP.wmiI.cThumbnailWidth;
	pDecoder->WMP.wmiI.cROIHeight = pDecoder->WMP.wmiI.cThumbnailHeight;
	pDecoder->WMP.wmiI.oOrientation = O_NONE;
	pDecoder->WMP.wmiI.cPostProcStrength = 0;
	pDecoder->WMP.wmiSCP.bVerbose = FALSE;
	int width = 0;
	int height = 0;
	pDecoder->GetSize(pDecoder, &width, &height);
	if (width == 0 || height == 0)
		return SPI_MEMORY_ERROR;
	if (pDecoder)
		pDecoder->Release(&pDecoder);
	if (pStream)
		free(pStream);
	picture_info->left = 0;
	picture_info->top = 0;
	picture_info->width = width;
	picture_info->height = height;
	picture_info->x_density = 0;
	picture_info->y_density = 0;
	picture_info->colorDepth = 32;
	picture_info->hInfo = NULL;

	return SPI_ALL_RIGHT;
}

int GetPictureEx(size_t data_size, HANDLE *bitmap_info, HANDLE *bitmap_data,
                 SPI_PROGRESS progress_callback, intptr_t user_data, const char *data) {
	uint8_t *data_u8;
	BITMAPINFOHEADER bitmap_info_header;
	BITMAPFILEHEADER bitmap_file_header;
	BITMAPINFO *bitmap_info_locked;
	unsigned char *bitmap_data_locked;

	if (progress_callback != NULL)
		if (progress_callback(1, 1, user_data))
			return SPI_ABORT;

	if (!getBMPFromJXR((const uint8_t *)data, data_size, &bitmap_file_header,
	                   &bitmap_info_header, &data_u8))
		return SPI_MEMORY_ERROR;
	*bitmap_info = LocalAlloc(LMEM_MOVEABLE, sizeof(BITMAPINFOHEADER));
	*bitmap_data = LocalAlloc(LMEM_MOVEABLE, bitmap_file_header.bfSize -
	                                             bitmap_file_header.bfOffBits);
	if (*bitmap_info == NULL || *bitmap_data == NULL) {
		if (*bitmap_info != NULL)
			LocalFree(*bitmap_info);
		if (*bitmap_data != NULL)
			LocalFree(*bitmap_data);
		return SPI_NO_MEMORY;
	}
	bitmap_info_locked = (BITMAPINFO *)LocalLock(*bitmap_info);
	bitmap_data_locked = (unsigned char *)LocalLock(*bitmap_data);
	if (bitmap_info_locked == NULL || bitmap_data_locked == NULL) {
		LocalFree(*bitmap_info);
		LocalFree(*bitmap_data);
		return SPI_MEMORY_ERROR;
	}
	bitmap_info_locked->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmap_info_locked->bmiHeader.biWidth = bitmap_info_header.biWidth;
	bitmap_info_locked->bmiHeader.biHeight = bitmap_info_header.biHeight;
	bitmap_info_locked->bmiHeader.biPlanes = 1;
	bitmap_info_locked->bmiHeader.biBitCount = 32;
	bitmap_info_locked->bmiHeader.biCompression = BI_RGB;
	bitmap_info_locked->bmiHeader.biSizeImage = 0;
	bitmap_info_locked->bmiHeader.biXPelsPerMeter = 0;
	bitmap_info_locked->bmiHeader.biYPelsPerMeter = 0;
	bitmap_info_locked->bmiHeader.biClrUsed = 0;
	bitmap_info_locked->bmiHeader.biClrImportant = 0;
	memcpy(bitmap_data_locked, data_u8,
	       bitmap_file_header.bfSize - bitmap_file_header.bfOffBits);

	LocalUnlock(*bitmap_info);
	LocalUnlock(*bitmap_data);
	free(data_u8);

	if (progress_callback != NULL)
		if (progress_callback(1, 1, user_data))
			return SPI_ABORT;

	return SPI_ALL_RIGHT;
}
