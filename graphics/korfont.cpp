/* ScummVM - Scumm Interpreter
 * Copyright (C) 2002-2005 The ScummVM project
 * ScummVM Kor. - Korean ScummVM Mod.
 * Copyright (C) 2002-2005 The ScummVM Kor Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /cvsroot/scummvm/scummvm/graphics/font.cpp,v 1.12 2005/06/04 13:19:53 fingolfin Exp $
 */

#ifdef SCUMMVMKOR

#include "scumm/scumm.h"
#include "graphics/font.h"
#include "graphics/cp949m.h"
#include "common/file.h"

namespace Graphics {
	
	// TODO: 다중 폰트 사용
	
	static byte *korfont = NULL;
	static int korwidth = 0;
	static int korheight = 0;
	
	int getKorFontWidth()
	{
		return korwidth;
	}
	
	int getKorFontHeight()
	{
		return korheight;
	}
	
	bool checkKorCode(byte hi, byte lo)
	{
		// hi: xx
		// lo: yy
		if ((0xA1 > lo) || (0xFE < lo)) {
			return false;
		}
		if ((hi >= 0xB0) && (hi <= 0xC8)) {
			return true;
		}
		return false;
	}
	
	byte *getKorCharPtr(int idx)
	{
		idx += 256;	// HACK
		if(!checkKorCode(idx % 256, idx / 256))
			return NULL;
		
		idx = ((idx % 256) - 0xb0) * 94 + (idx / 256) - 0xa1;
		if(idx < 0 || idx >= 2350) {
			//warning("Invaild index %d", idx);
			return NULL;
		}
		
		return korfont + ((korwidth + 7) / 8) * korheight * idx;	// FIXME
	}
	
	void loadKoreanGUIFont()
	{
		Common::File file;
		const char *fontFile = "korean_g.fnt";
		
		if (file.open(fontFile)) {
			int numChar = 2350;
			
			debug(2, "Loading Korean GUI Font");
			file.seek(2, SEEK_CUR);
			korwidth = file.readByte();
			korheight = file.readByte();
			
			korfont = new byte[((korwidth + 7) / 8) * korheight * numChar];
			file.read(korfont, ((korwidth + 7) / 8) * korheight * numChar);
			file.close();
			//warning("Font: %d %d at %x", korwidth, korheight, korfont);
		} else {
			//warning("Couldn't load any font");
		}
	}
	
	void unloadKoreanGUIFont()
	{
		delete []korfont;
	}
	
	void drawKorChar(Surface *dst, uint32 chr, int tx, int ty, uint32 color)
	{
		const byte revBitMask[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
		
		assert(dst != 0);
		byte *ptr = (byte *)dst->getBasePtr(tx, ty);
		
		byte bits = 0;
		
		const byte *tmp = getKorCharPtr(chr);
		if(tmp == NULL) {
			//warning("can't draw char");
			return;
		}
		
		for (int y = 0; y < korheight; y++) {
			if(ty + y < 0 || ty + y >= dst->h)
				continue;
			for (int x = 0; x < korwidth; x++) {
				if (tx + x < 0 || tx + x >= dst->w)
					continue;
				if ((x % 8) == 0)
					bits = *tmp++;
				if ((bits & revBitMask[x % 8])) {
					if (dst->format.bytesPerPixel == 1)
						ptr[x] = color;
					else if (dst->format.bytesPerPixel == 2)
						((uint16 *)ptr)[x] = color;
				}
			}
			ptr += dst->pitch;
		}
	}


int get_oem_code_page(uint16 code)
{
	int i,j,k;
	if((code>=0x8141)&&(code<=0xc8fe)){

		i = j = k = 0;
		
		// 좀 더 탐색 시간을 단축하기 위한
		
		k = (int)((code&0x7f00)>>8); // 1~ 72;
		
		if((k>=1)&&(k<=36))        j = (k-1)*3;
		else if((k>=37)&&(k<=48))  j = 109 + (k-37)*3;
		else if((k>=49)&&(k<=70))  j = 146 + (k-49)*4;
		else                       j=232;

		for(i=j;i<_CP949_PAGE;i++)
		{
			if((code>=cp949[i].first)&&(code<=cp949[i].last))return(i);
		}
	}
	return(-1);
}


uint16 _KS_to_unicode(uint16 ksc)
{
	uint16 c,i,j,k;
	int page;
	uint16 u,oem,offset,cnt;    
	unsigned char *p;   
	
	c = (ksc << 8) | (ksc >> 8 & 0xFF);
	u=oem=offset=cnt=0;
	i=j=k=0;
	page =  get_oem_code_page(c);
	if(page>=0)
	{
		oem =    cp949[page].first;
		u =      cp949[page].uc_first;
		offset = cp949[page].offset;
		cnt =    cp949[page].cnt;
		p = (unsigned char *)&_OEM_DATA[offset];
		for(i=0;i<cnt;i++,oem++)
		{
			if(oem<c) u+=(*p++);
			else{
				if(i) u+=(*p++);
				break;
			}
		}
	}
	return(u);
}


FontKorean *FontKorean::createFont(const char * fontFile) {
	FontKorean *ret = 0;

	// Try ScummVM's font.
	ret = new FontKoreanSVM();
	if (ret->loadData(fontFile))
		return ret;
	delete ret;

	// Try Wansung font.
	ret = new FontKoreanWansung();
	if (ret->loadData(fontFile))
		return ret;
	delete ret;

	return 0;
}

void FontKorean::drawChar(Graphics::Surface &dst, uint16 ch, int x, int y, uint32 c1, uint32 c2) const {
	drawChar(dst.getBasePtr(x, y), ch, dst.pitch, dst.format.bytesPerPixel, c1, c2, dst.w - x, dst.h - y);
}

FontKoreanBase::FontKoreanBase()
	: _drawMode(kDefaultMode), _flippedMode(false), _fontWidth(16), _fontHeight(16) {
}

void FontKoreanBase::setDrawingMode(DrawingMode mode) {
	if (hasFeature(1 << mode))
		_drawMode = mode;
	else
		warning("Unsupported drawing mode selected");
}

void FontKoreanBase::toggleFlippedMode(bool enable) {
	if (hasFeature(kFeatFlipped))
		_flippedMode = enable;
	else
		warning("Flipped mode unsupported by this font");
}

uint FontKoreanBase::getFontHeight() const {
	switch (_drawMode) {
	case kOutlineMode:
		return _fontHeight + 2;

	case kDefaultMode:
		return _fontHeight;

	default:
		return _fontHeight + 1;
	}
}

uint FontKoreanBase::getMaxFontWidth() const {
	switch (_drawMode) {
	case kOutlineMode:
		return _fontWidth + 2;

	case kDefaultMode:
		return _fontWidth;

	default:
		return _fontWidth + 1;
	}
}

uint FontKoreanBase::getCharWidth(uint16 ch) const {
	if (isASCII(ch))
		//return ((_drawMode == kOutlineMode) ? 10 : (_drawMode == kDefaultMode ? 8 : 9));
		return ((_drawMode == kOutlineMode) ? _fontWidth / 2 + 2 : (_drawMode == kDefaultMode ? _fontWidth / 2 : _fontWidth / 2 + 1));
	else
		return getMaxFontWidth();
}

template<typename Color>
void FontKoreanBase::blitCharacter(const uint8 *glyph, const int w, const int h, uint8 *dst, int pitch, Color c) const {
	uint8 bitPos = 0;
	uint8 mask = 0;

	for (int y = 0; y < h; ++y) {
		Color *d = (Color *)dst;
		dst += pitch;

		bitPos &= _bitPosNewLineMask;
		for (int x = 0; x < w; ++x) {
			if (!(bitPos % 8))
				mask = *glyph++;

			if (mask & 0x80)
				*d = c;

			++d;
			++bitPos;
			mask <<= 1;
		}
	}
}

void FontKoreanBase::createOutline(uint8 *outline, const uint8 *glyph, const int w, const int h) const {
	const int glyphPitch = (w + 7) / 8;
	const int outlinePitch = (w + 9) / 8;

	uint8 *line1 = outline + 0 * outlinePitch;
	uint8 *line2 = outline + 1 * outlinePitch;
	uint8 *line3 = outline + 2 * outlinePitch;

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < glyphPitch; ++x) {
			const uint8 mask = *glyph++;

			const uint8 b1 = mask | (mask >> 1) | (mask >> 2);
			const uint8 b2 = (mask << 7) | ((mask << 6) & 0xC0);

			line1[x] |= b1;
			line2[x] |= b1;
			line3[x] |= b1;

			if (x + 1 < outlinePitch) {
				line1[x + 1] |= b2;
				line2[x + 1] |= b2;
				line3[x + 1] |= b2;
			}
		}

		line1 += outlinePitch;
		line2 += outlinePitch;
		line3 += outlinePitch;
	}
}

void FontKoreanBase::drawChar(void *dst, uint16 ch, int pitch, int bpp, uint32 c1, uint32 c2, int maxW, int maxH) const {
	const uint8 *glyphSource = 0;
	int width = 0, height = 0;
	int outlineExtraWidth = 2, outlineExtraHeight = 2;
	int outlineXOffset = 0, outlineYOffset = 0;

	if (isASCII(ch)) {
		glyphSource = getCharData(ch);
		//width = 8;
		width = _fontWidth / 2;
		height = _fontHeight;
	} else {
		glyphSource = getCharData(ch);
		width = _fontWidth;
		height = _fontHeight;
	}

	if (maxW != -1 && maxW < width) {
		width = maxW;
		outlineExtraWidth = 0;
		outlineXOffset = 1;
	}

	if (maxH != -1 && maxH < height) {
		height = maxH;
		outlineExtraHeight = 0;
		outlineYOffset = 1;
	}

	if (width <= 0 || height <= 0)
		return;

	if (!glyphSource) {
		warning("FontKoreanBase::drawChar: Font does not offer data for %02X %02X", ch & 0xFF, ch >> 8);
		return;
	}

	uint8 outline[18 * 18];
	if (_drawMode == kOutlineMode) {
		memset(outline, 0, sizeof(outline));
		createOutline(outline, glyphSource, width, height);
	}

	if (bpp == 1) {
		if (_drawMode == kOutlineMode) {
			blitCharacter<uint8>(outline, width + outlineExtraWidth, height + outlineExtraHeight, (uint8 *)dst, pitch, c2);
			blitCharacter<uint8>(glyphSource, width - outlineXOffset, height - outlineYOffset, (uint8 *)dst + pitch + 1, pitch, c1);
		} else {
			if (_drawMode != kDefaultMode) {
				blitCharacter<uint8>(glyphSource, width - outlineXOffset, height, ((uint8 *)dst) + 1, pitch, c2);
				blitCharacter<uint8>(glyphSource, width, height - outlineYOffset, ((uint8 *)dst) + pitch, pitch, c2);
				if (_drawMode == kShadowMode)
					blitCharacter<uint8>(glyphSource, width - outlineXOffset, height - outlineYOffset, ((uint8 *)dst) + pitch + 1, pitch, c2);
			}

			blitCharacter<uint8>(glyphSource, width, height, (uint8 *)dst, pitch, c1);
		}
	} else if (bpp == 2) {
		if (_drawMode == kOutlineMode) {
			blitCharacter<uint16>(outline, width + outlineExtraWidth, height + outlineExtraHeight, (uint8 *)dst, pitch, c2);
			blitCharacter<uint16>(glyphSource, width - outlineXOffset, height - outlineYOffset, (uint8 *)dst + pitch + 2, pitch, c1);
		} else {
			if (_drawMode != kDefaultMode) {
				blitCharacter<uint16>(glyphSource, width - outlineXOffset, height, ((uint8 *)dst) + 2, pitch, c2);
				blitCharacter<uint16>(glyphSource, width, height - outlineYOffset, ((uint8 *)dst) + pitch, pitch, c2);
				if (_drawMode == kShadowMode)
					blitCharacter<uint16>(glyphSource, width - outlineXOffset, height - outlineYOffset, ((uint8 *)dst) + pitch + 2, pitch, c2);
			}

			blitCharacter<uint16>(glyphSource, width, height, (uint8 *)dst, pitch, c1);
		}
	} else {
		error("FontKoreanBase::drawChar: unsupported bpp: %d", bpp);
	}
}

bool FontKoreanBase::isASCII(uint16 ch) const {
	if (ch >= 0xFF)
		return false;
	else if (ch <= 0x7F)
		return true;
	else
		return false;
}

// ScummVM Korean font

FontKoreanSVM::FontKoreanSVM()
	: _fontData16x16(0), _fontData16x16Size(0), _fontData8x16(0), _fontData8x16Size(0),
	  _fontData8x8(0), _fontData8x8Size(0) {

	//_fontWidth = 16;
	//_fontHeight = 16;
}

FontKoreanSVM::~FontKoreanSVM() {
	delete[] _fontData16x16;
	delete[] _fontData8x16;
	delete[] _fontData8x8;
}

bool FontKoreanSVM::loadData(const char *fontFile) {
	Common::SeekableReadStream *data = SearchMan.createReadStreamForMember(fontFile);
	if (!data)
		return false;

	uint32 magic1 = data->readUint32BE();
	uint32 magic2 = data->readUint32BE();

	if (magic1 != MKTAG('S', 'C', 'V', 'M') || magic2 != MKTAG('S', 'J', 'I', 'S')) {
		delete data;
		return false;
	}

	uint32 version = data->readUint32BE();
	if (version != kKoreanFontVersion) {
		warning("Korean font version mismatch, expected: %d found: %u", kKoreanFontVersion, version);
		delete data;
		return false;
	}
	uint numChars16x16 = data->readUint16BE();
	uint numChars8x16 = data->readUint16BE();
	uint numChars8x8 = data->readUint16BE();

	if (_fontWidth == 16) {
		_fontData16x16Size = numChars16x16 * 32;
		_fontData16x16 = new uint8[_fontData16x16Size];
		assert(_fontData16x16);
		data->read(_fontData16x16, _fontData16x16Size);

		//_fontData8x16Size = numChars8x16 * 32;
		//_fontData8x16 = new uint8[numChars8x16 * 32];
		//assert(_fontData8x16);
		//data->read(_fontData8x16, _fontData8x16Size);
		_fontData8x16Size = numChars8x16 * 16;
		_fontData8x16 = new uint8[numChars8x16 * 16];
		assert(_fontData8x16);
		for (uint i=0; i < _fontData8x16Size; i++)
			data->read(&_fontData8x16[i], 2);
	} else {
		data->skip(numChars16x16 * 32);
		data->skip(numChars8x16 * 32);

		_fontData8x8Size = numChars8x8 * 8;
		_fontData8x8 = new uint8[_fontData8x8Size];
		assert(_fontData8x8);
		data->read(_fontData8x8, _fontData8x8Size);
	}

	bool retValue = !data->err();
	delete data;
	return retValue;
}

const uint8 *FontKoreanSVM::getCharData(uint16 c) const {
	//warning("FontKoreanSVM::getCharData");
	if (_fontWidth == 8)
		return getCharDataPCE(c);
	else
		return getCharDataDefault(c);
}

bool FontKoreanSVM::hasFeature(int feat) const {
	// Flipped mode is not supported since the hard coded table (taken from SCUMM 5 FM-TOWNS)
	// is set up for font sizes of 8/16. This mode is also not required at the moment, since
	// there aren't any SCUMM 5 PC-Engine games.
	static const int features16 = kFeatDefault | kFeatOutline | kFeatShadow | kFeatFMTownsShadow | kFeatFlipped;
	static const int features8 = kFeatDefault | kFeatOutline | kFeatShadow | kFeatFMTownsShadow;
	return (((_fontWidth == 8) ? features8 : features16) & feat) ? true : false;
}

const uint8 *FontKoreanSVM::getCharDataPCE(uint16 c) const {
	if (isASCII(c))
		return 0;

	//c += 256;	// HACK
	if(!checkKorCode(c % 256, c / 256))
	//if (c < 0xAC00 || c >= 0xD7A4)
		return 0;
	uint16 uc = _KS_to_unicode(c);

	const uint offset = (uc - 0xAC00) * 8;
	assert(offset + 96 <= _fontData8x8Size); // 11184 * 8
	return _fontData8x8 + offset;
}

const uint8 *FontKoreanSVM::getCharDataDefault(uint16 c) const {
	if (isASCII(c)) {
		//const uint offset = c * 32;
		const uint offset = c * 16;
		//warning("getCharDataDefault: ascii c=%02x, offset=%d, size=%d", c, offset, _fontData8x16Size);
		assert(offset <= _fontData8x16Size);
		return _fontData8x16 + offset;
	} else {
		//c += 256;	// HACK
		if(!checkKorCode(c % 256, c / 256))
		//if (c < 0xAC00 || c >= 0xD7B0)
			return 0;
		uint16 uc = _KS_to_unicode(c);

		const uint offset = (uc - 0xAC00) * 32;
		//warning("getCharDataDefault: c=%04x, uc=%04x, offset=%d, size=%d", c, uc, offset, _fontData16x16Size);
		assert(offset + 384 <= _fontData16x16Size); // 11184 * 32
		return _fontData16x16 + offset;
	}
}

// Korean Wansung font

bool FontKoreanWansung::loadData(const char *fontFile) {
	Common::SeekableReadStream *data = SearchMan.createReadStreamForMember(fontFile);
	if (!data)
		return false;

	data->skip(1);
	_fontShadow = data->readByte();
	_fontWidth = data->readByte();
	_fontHeight = data->readByte();
	_fontDataSize = ((_fontWidth + 7) / 8) * _fontHeight * kFontNumChars;
	_fontData = new byte[_fontDataSize];
	data->read(_fontData, _fontDataSize);

	englishLoadData("english.fnt");

	bool retValue = !data->err();
	delete data;
	return retValue;
}

const uint8 *FontKoreanWansung::getCharData(uint16 ch) const {
	if (isASCII(ch) && _englishFontData) {
		const uint offset = ((_englishFontWidth + 7) / 8) * _englishFontHeight * (ch & 0xFF);
		assert(offset <= _englishFontDataSize);
		return _englishFontData + offset;
	} else {
		int idx = ((ch % 256) - 0xb0) * 94 + (ch / 256) - 0xa1;
		return _fontData + ((_fontWidth + 7) / 8) * _fontHeight * idx;
	}
}

bool FontKoreanWansung::hasFeature(int feat) const {
	static const int features = kFeatDefault | kFeatOutline | kFeatShadow | kFeatFMTownsShadow | kFeatFlipped;
	return (features & feat) ? true : false;
}

// English font

bool FontKoreanWansung::englishLoadData(const char *fontFile) {
	Common::SeekableReadStream *data = SearchMan.createReadStreamForMember(fontFile);
	if (!data)
		return false;

	data->skip(2);
	_englishFontWidth = data->readByte();
	_englishFontHeight = data->readByte();
	_englishFontDataSize = ((_englishFontWidth + 7) / 8) * _englishFontHeight * eFontNumChars;
	_englishFontData = new byte[_englishFontDataSize];
	data->read(_englishFontData, _englishFontDataSize);

	bool retValue = !data->err();
	delete data;
	return retValue;
}

} // End of namespace Graphics

#endif
