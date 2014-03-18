/* Residual - A 3D game interpreter
 *
 * Residual is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the AUTHORS
 * file distributed with this source distribution.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 * $URL$
 * $Id$
 *
 */

#include "engines/stark/xmg.h"
#include "engines/stark/debug.h"

#include "graphics/pixelformat.h"
#include "common/stream.h"

namespace Stark {

// XMG DECODER

class XMGDecoder {
public:
	static Graphics::Surface *decode(Common::ReadStream *stream);

private:
	XMGDecoder() {}

	Graphics::Surface *decodeImage(Common::ReadStream *stream);

	void processYCrCb();
	void processTrans();
	void processRGB();

	uint32 *_pixels;
	Common::ReadStream *_stream;

	uint32 _transColor;
	uint32 _scanLen;
};

Graphics::Surface *XMGDecoder::decode(Common::ReadStream *stream) {
	XMGDecoder dec;
	return dec.decodeImage(stream);
}

// TODO: fix color handling in BE machines?
Graphics::Surface *XMGDecoder::decodeImage(Common::ReadStream *stream) {
	_stream = stream;

	// Read the file version
	uint32 version = stream->readUint32LE();
	if (version != 3) {
		warning("Stark::XMG: File version unknown: %d", version);
	}

	// Read the transparency color (RGBA)
	_transColor = stream->readUint32LE();

	// Read the image size
	uint32 width = stream->readUint32LE();
	uint32 height = stream->readUint32LE();
	debugC(10, kDebugXMG, "Stark::XMG: Version=%d, TransparencyColor=0x%08x, size=%dx%d", version, _transColor, width, height);

	// Read the scan length
	_scanLen = stream->readUint32LE();
	if (_scanLen != 3 * width) {
		warning("Stark::XMG: The scan length (%d) doesn't match the width bytes (%d)", _scanLen, 3 * width);
	}
	// We're using RGBA instead of RGB, so we use our own scanlength
	_scanLen = width;

	// Unknown
	uint32 unknown2 = stream->readUint32LE();
	debugC(kDebugUnknown, "Stark::XMG: unknown2 = %08x = %d", unknown2, unknown2);
	uint32 unknown3 = stream->readUint32LE();
	debugC(kDebugUnknown, "Stark::XMG: unknown3 = %08x = %d", unknown3, unknown3);

	// Create the destination surface
	Graphics::Surface *surface = new Graphics::Surface();
	if (!surface)
		return NULL;
	// Placeholder pixelformat
	Graphics::PixelFormat pixFormat(4, 8, 8, 8, 8, 24, 16, 8 ,0);
	surface->create(width, height, pixFormat);

	_pixels = (uint32 *)surface->getPixels();
	uint32 currX = 0, currY = 0;
	while (!stream->eos()) {
		// TODO: Handle odd-sized images
		if (currX >= width) {
			//assert (currX == width);
			currX -= width;
			currY += 2;
			_pixels += _scanLen;
			if (currY + 1 >= height)
				break;
		}

		// Read the number and mode of the tiles
		byte op = stream->readByte();
		uint16 count;
		if ((op & 0xC0) != 0xC0) {
			count = op & 0x3F;
		} else {
			count = ((op & 0xF) << 8) + stream->readByte();
			op <<= 4;
		}
		op &= 0xC0;

		// Process the current serie
		for (int i = 0; i < count; i++) {
			switch (op) {
			case 0x00:
				// YCrCb
				processYCrCb();
				break;
			case 0x40:
				// Trans
				processTrans();
				break;
			case 0x80:
				// RGB
				processRGB();
				break;
			case 0xC0:
				error("Unsupported colour mode");
				break;
			}
			_pixels += 2;
		}
		currX += 2 * count;
	}

	return surface;
}

inline static void YUV2RGB(byte y, byte u, byte v, byte &r, byte &g, byte &b) {
	r = CLIP<int>(y + ((1357 * (v - 128)) >> 10), 0, 255);
	g = CLIP<int>(y - (( 691 * (v - 128)) >> 10) - ((333 * (u - 128)) >> 10), 0, 255);
	b = CLIP<int>(y + ((1715 * (u - 128)) >> 10), 0, 255);
}

void XMGDecoder::processYCrCb() {
	byte y0, y1, y2, y3;
	byte cr, cb;

	y0 = _stream->readByte();
	y1 = _stream->readByte();
	y2 = _stream->readByte();
	y3 = _stream->readByte();
	cr = _stream->readByte();
	cb = _stream->readByte();

	byte r, g, b;

	YUV2RGB(y0, cb, cr, r, g, b);
	_pixels[0] = (255 << 24) + (b << 16) + (g << 8) + r;

	YUV2RGB(y1, cb, cr, r, g, b);
	_pixels[1] = (255 << 24) + (b << 16) + (g << 8) + r;

	YUV2RGB(y2, cb, cr, r, g, b);
	_pixels[_scanLen + 0] = (255 << 24) + (b << 16) + (g << 8) + r;

	YUV2RGB(y3, cb, cr, r, g, b);
	_pixels[_scanLen + 1] = (255 << 24) + (b << 16) + (g << 8) + r;
}

void XMGDecoder::processTrans() {
	_pixels[0] = _transColor;
	_pixels[1] = _transColor;

	_pixels[_scanLen + 0] = _transColor;
	_pixels[_scanLen + 1] = _transColor;
}

void XMGDecoder::processRGB() {
	uint32 color;

	color = _stream->readUint16LE();
	color += _stream->readByte() << 16;
	if (color != _transColor)
		color += 255 << 24;
	_pixels[0] = color;

	color = _stream->readUint16LE();
	color += _stream->readByte() << 16;
	if (color != _transColor)
		color += 255 << 24;
	_pixels[1] = color;

	color = _stream->readUint16LE();
	color += _stream->readByte() << 16;
	if (color != _transColor)
		color += 255 << 24;
	_pixels[_scanLen + 0] = color;

	color = _stream->readUint16LE();
	color += _stream->readByte() << 16;
	if (color != _transColor)
		color += 255 << 24;
	_pixels[_scanLen + 1] = color;
}


// SCENE ELEMENT XMG

SceneElementXMG::SceneElementXMG() : _surface(NULL) {
}

SceneElementXMG::~SceneElementXMG() {
	// Free the surface
	if (_surface)
		_surface->free();
	delete _surface;
}

SceneElementXMG *SceneElementXMG::load(const Common::Archive *archive, const Common::String &name, uint16 x, uint16 y) {
	// Open the resource
	Common::SeekableReadStream *res = archive->createReadStreamForMember(name);
	if (!res)
		return NULL;

	// Create the element to return
	SceneElementXMG *element = new SceneElementXMG();

	// Decode the XMG
	element->_surface = XMGDecoder::decode(res);
	delete res;

	// Save the rendering coordinates
	element->_x = x;
	element->_y = y;

	return element;
}

void SceneElementXMG::render(GfxDriver *gfx) {
	// Draw the current element
	gfx->drawSurface(_surface, Common::Point(_x, _y));
}

} // End of namespace Stark
