/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

/*
 * This code is based on original Tony Tough source code
 *
 * Copyright (c) 1997-2003 Nayma Software
 */

#include "tony/utils.h"
#include "tony/tony.h"
#include "tony/mpal/lzo.h"

namespace Tony {

/**
 * Extracts a string from a data stream
 * @param df                data stream
 * @param var               String
 */
RMDataStream &operator>>(RMDataStream &df, Common::String &var) {
	uint8 len;
	int i;

	df >> len;

	for (i = 0; i < len; i++) {
		char c;
		df >> c;
		var += c;
	}

	return df;
}

/****************************************************************************\
*       RMFileStreamSlow Methods
\****************************************************************************/

RMFileStreamSlow::RMFileStreamSlow() : RMDataStream() {
	_stream = NULL;
}

RMFileStreamSlow::~RMFileStreamSlow() {
	close();
}

void RMFileStreamSlow::close() {
	delete _stream;
}

bool RMFileStreamSlow::openFile(Common::File &file) {
	_stream = file.readStream(file.size());

	_length = _stream->pos();

	return true;
}

bool RMFileStreamSlow::openFile(const char *lpFN) {
	// Open file for reading
	Common::File f;
	if (!f.open(lpFN))
		return false;

	_length = f.size();
	_stream = f.readStream(f.size());

	return true;
}

RMDataStream &RMFileStreamSlow::operator+=(int nBytes) {
	seek(nBytes);
	return *this;
}

int RMFileStreamSlow::pos() {
	return _stream->pos();
}

bool RMFileStreamSlow::isEOF() {
	return (pos() >= _length);
}

int RMFileStreamSlow::seek(int nBytes, RMDSPos where) {
	switch (where) {
	case START:
		return _stream->seek(nBytes);

	case END:
		return _stream->seek(nBytes, SEEK_END);

	case CUR:
		return _stream->seek(nBytes, SEEK_CUR);

	default:
		return 0;
	}
}

bool RMFileStreamSlow::read(void *buf, int size) {
	uint32 dwRead;

	dwRead = _stream->read(buf, size);
	return ((int)dwRead == size);
}

RMFileStreamSlow &operator>>(RMFileStreamSlow &df, char &var) {
	df.read(&var, 1);
	return df;
}

RMFileStreamSlow &operator>>(RMFileStreamSlow &df, byte &var) {
	df.read(&var, 1);
	return df;
}

RMFileStreamSlow &operator>>(RMFileStreamSlow &df, uint16 &var) {
	uint16 v;
	df.read(&v, 2);
	v = FROM_LE_16(v);
	return df;
}

RMFileStreamSlow &operator>>(RMFileStreamSlow &df, int16 &var) {
	uint16 v;
	df.read(&v, 2);
	var = (int16)FROM_LE_16(v);
	return df;
}

RMFileStreamSlow &operator>>(RMFileStreamSlow &df, int &var) {
	int v;
	df.read(&v, 4);
	var = FROM_LE_32(v);
	return df;
}

RMFileStreamSlow &operator>>(RMFileStreamSlow &df, uint32 &var) {
	uint32 v;
	df.read(&v, 4);
	var = FROM_LE_32(v);
	return df;
}

/****************************************************************************\
*       RMDataStream methods
\****************************************************************************/

/**
 * Constructor
 */
RMDataStream::RMDataStream() {
	_length = 0;
	_pos = 0;
	_bError = false;

	_buf = NULL;
	_ecode = 0;
}

/**
 * Destructor
 */
RMDataStream::~RMDataStream() {
	close();
}

/**
 * Close a stream
 */
void RMDataStream::close() {
	_length = 0;
	_pos = 0;
}

/**
 * Takes the address of the buffer from which will be read the data.
 * @param lpBuf         Data buffer
 * @param size          Size of the buffer
 * @remarks             If the length of the buffer is not known, and cannot be
 *                      specified, then EOF() and Seek() to end won't work.
 */
void RMDataStream::openBuffer(const byte *lpBuf, int size) {
	_length = size;
	_buf = lpBuf;
	_bError = false;
	_pos = 0;
}

/**
 * Returns the length of the stream
 * @returns             Stream length
 */
int RMDataStream::length() {
	return _length;
}

/**
 * Determines if the end of the stream has been reached
 * @returns             true if end of stream reached, false if not
 */
bool RMDataStream::isEOF() {
	return (_pos >= _length);
}

/**
 * Extracts data from the stream
 * @param df                Stream
 * @param var               Variable of a supported type
 * @returns                 Value read from the stream
 */
RMDataStream &operator>>(RMDataStream &df, char &var) {
	df.read(&var, 1);
	return df;
}

/**
 * Extracts data from the stream
 * @param df                Stream
 * @param var               Variable of a supported type
 * @returns                 Value read from the stream
 */
RMDataStream &operator>>(RMDataStream &df, uint8 &var) {
	df.read(&var, 1);
	return df;
}

/**
 * Extracts data from the stream
 * @param df                Stream
 * @param var               Variable of a supported type
 * @returns                 Value read from the stream
 */
RMDataStream &operator>>(RMDataStream &df, uint16 &var) {
	uint16 v;
	df.read(&v, 2);

	var = FROM_LE_16(v);
	return df;
}

/**
 * Extracts data from the stream
 * @param df                Stream
 * @param var               Variable of a supported type
 * @returns                 Value read from the stream
 */
RMDataStream &operator>>(RMDataStream &df, int16 &var) {
	uint16 v;
	df.read(&v, 2);

	var = (int16)FROM_LE_16(v);
	return df;
}

/**
 * Extracts data from the stream
 * @param df                Stream
 * @param var               Variable of a supported type
 * @returns                 Value read from the stream
 */
RMDataStream &operator>>(RMDataStream &df, int &var) {
	uint32 v;
	df.read(&v, 4);

	var = (int)FROM_LE_32(v);
	return df;
}

/**
 * Extracts data from the stream
 * @param df                Stream
 * @param var               Variable of a supported type
 * @returns                 Value read from the stream
 */
RMDataStream &operator>>(RMDataStream &df, uint32 &var) {
	uint32 v;
	df.read(&v, 4);

	var = FROM_LE_32(v);
	return df;
}

/**
 * Reads a series of data from the stream in a buffer
 * @param lpBuf             Data buffer
 * @param size              Size of the buffer
 * @returns                 true if we have reached the end, false if not
 */
bool RMDataStream::read(void *lpBuf, int size) {
	byte *dest = (byte *)lpBuf;

	if ((_pos + size) > _length) {
		Common::copy(_buf + _pos, _buf + _pos + (_length - _pos), dest);

		return true;
	} else {
		Common::copy(_buf + _pos, _buf + _pos + size, dest);

		_pos += size;
		return false;
	}
}

/**
 * Skips a number of bytes in the stream
 * @param nBytres           Number of bytes to skip
 * @returns                 The stream
 */
RMDataStream &RMDataStream::operator+=(int nBytes) {
	_pos += nBytes;
	return *this;
}

/**
 * Seeks to a position within the stream
 * @param nBytes            Number of bytes from specified origin
 * @param origin            Origin to do offset from
 * @returns                 The absolute current position in bytes
 */
int RMDataStream::seek(int nBytes, RMDSPos origin) {
	switch (origin) {
	case CUR:
		break;

	case START:
		_pos = 0;
		break;

	case END:
		if (_length == SIZENOTKNOWN)
			return _pos;
		_pos = _length;
		break;
	}

	_pos += nBytes;
	return _pos;
}

/**
 * Returns the current position of the stream
 * @returns                 The current position
 */
int RMDataStream::pos() {
	return _pos;
}

/**
 * Check if an error occurred during reading the stream
 * @returns                 true if there was an error, false otherwise
 */
bool RMDataStream::isError() {
	return _bError;
}

/**
 * Sets an error code for the stream
 * @param code              Error code
 */
void RMDataStream::setError(int code) {
	_bError = true;
	_ecode = code;
}

/**
 * Returns the error code for the stream
 * @returns                 Error code
 */
int RMDataStream::getError() {
	return _ecode;
}

/****************************************************************************\
*       RMPoint methods
\****************************************************************************/

/**
 * Constructor
 */
RMPoint::RMPoint() {
	_x = _y = 0;
}

/**
 * Copy constructor
 */
RMPoint::RMPoint(const RMPoint &p) {
	_x = p._x;
	_y = p._y;
}

/**
 * Constructor with integer parameters
 */
RMPoint::RMPoint(int x1, int y1) {
	_x = x1;
	_y = y1;
}

/**
 * Copy operator
 */
RMPoint &RMPoint::operator=(RMPoint p) {
	_x = p._x;
	_y = p._y;

	return *this;
}

/**
 * Offsets the point by another point
 */
void RMPoint::offset(const RMPoint &p) {
	_x += p._x;
	_y += p._y;
}

/**
 * Offsets the point by a specified offset
 */
void RMPoint::offset(int xOff, int yOff) {
	_x += xOff;
	_y += yOff;
}

/**
 * Sums together two points
 */
RMPoint operator+(RMPoint p1, RMPoint p2) {
	RMPoint p(p1);

	return (p += p2);
}

/**
 * Subtracts two points
 */
RMPoint operator-(RMPoint p1, RMPoint p2) {
	RMPoint p(p1);

	return (p -= p2);
}

/**
 * Sum (offset) of a point
 */
RMPoint &RMPoint::operator+=(RMPoint p) {
	offset(p);
	return *this;
}

/**
 * Subtract (offset) of a point
 */
RMPoint &RMPoint::operator-=(RMPoint p) {
	offset(-p);
	return *this;
}

/**
 * Inverts a point
 */
RMPoint RMPoint::operator-() {
	RMPoint p;

	p._x = -_x;
	p._y = -_y;

	return p;
}

/**
 * Equality operator
 */
bool RMPoint::operator==(RMPoint p) {
	return ((_x == p._x) && (_y == p._y));
}

/**
 * Not equal operator
 */
bool RMPoint::operator!=(RMPoint p) {
	return ((_x != p._x) || (_y != p._y));
}

/**
 * Reads a point from a stream
 */
RMDataStream &operator>>(RMDataStream &ds, RMPoint &p) {
	ds >> p._x >> p._y;
	return ds;
}

/****************************************************************************\
*       RMPointReference methods
\****************************************************************************/

RMPointReference &RMPointReference::operator=(const RMPoint &p) { 
	_x = p._x; _y = p._y; 
	return *this;
}

RMPointReference &RMPointReference::operator-=(const RMPoint &p) {
	_x -= p._x; _y -= p._y; 
	return *this;
}

/****************************************************************************\
*       RMRect methods
\****************************************************************************/

RMRect::RMRect(): _topLeft(_x1, _y1), _bottomRight(_x2, _y2) {
	setEmpty();
}

void RMRect::setEmpty() {
	_x1 = _y1 = _x2 = _y2 = 0;
}

RMRect::RMRect(const RMPoint &p1, const RMPoint &p2): _topLeft(_x1, _y1), _bottomRight(_x2, _y2) {
	setRect(p1, p2);
}

RMRect::RMRect(int X1, int Y1, int X2, int Y2): _topLeft(_x1, _y1), _bottomRight(_x2, _y2) {
	setRect(X1, Y1, X2, Y2);
}

RMRect::RMRect(const RMRect &rc): _topLeft(_x1, _y1), _bottomRight(_x2, _y2) {
	copyRect(rc);
}

void RMRect::setRect(const RMPoint &p1, const RMPoint &p2) {
	_x1 = p1._x;
	_y1 = p1._y;
	_x2 = p2._x;
	_y2 = p2._y;
}

void RMRect::setRect(int X1, int Y1, int X2, int Y2) {
	_x1 = X1;
	_y1 = Y1;
	_x2 = X2;
	_y2 = Y2;
}

void RMRect::setRect(const RMRect &rc) {
	copyRect(rc);
}

void RMRect::copyRect(const RMRect &rc) {
	_x1 = rc._x1;
	_y1 = rc._y1;
	_x2 = rc._x2;
	_y2 = rc._y2;
}

RMPoint RMRect::center() {
	return RMPoint((_x2 - _x1) / 2, (_y2 - _y1) / 2);
}

int RMRect::width() const {
	return _x2 - _x1;
}

int RMRect::height() const {
	return _y2 - _y1;
}

int RMRect::size() const {
	return width() * height();
}

RMRect::operator Common::Rect() const {
	return Common::Rect(_x1, _y1, _x2, _y2);
}

bool RMRect::isEmpty() const {
	return (_x1 == 0 && _y1 == 0 && _x2 == 0 && _y2 == 0);
}

const RMRect &RMRect::operator=(const RMRect &rc) {
	copyRect(rc);
	return *this;
}

void RMRect::offset(int xOff, int yOff) {
	_x1 += xOff;
	_y1 += yOff;
	_x2 += xOff;
	_y2 += yOff;
}

void RMRect::offset(const RMPoint &p) {
	_x1 += p._x;
	_y1 += p._y;
	_x2 += p._x;
	_y2 += p._y;
}

const RMRect &RMRect::operator+=(RMPoint p) {
	offset(p);
	return *this;
}

const RMRect &RMRect::operator-=(RMPoint p) {
	offset(-p);
	return *this;
}

RMRect operator+(const RMRect &rc, RMPoint p) {
	RMRect r(rc);
	return (r += p);
}

RMRect operator-(const RMRect &rc, RMPoint p) {
	RMRect r(rc);

	return (r -= p);
}

RMRect operator+(RMPoint p, const RMRect &rc) {
	RMRect r(rc);

	return (r += p);
}

RMRect operator-(RMPoint p, const RMRect &rc) {
	RMRect r(rc);

	return (r += p);
}

bool RMRect::operator==(const RMRect &rc) {
	return ((_x1 == rc._x1) && (_y1 == rc._y1) && (_x2 == rc._x2) && (_y2 == rc._y2));
}

bool RMRect::operator!=(const RMRect &rc) {
	return ((_x1 != rc._x1) || (_y1 != rc._y1) || (_x2 != rc._x2) || (_y2 != rc._y2));
}

void RMRect::normalizeRect() {
	setRect(MIN(_x1, _x2), MIN(_y1, _y2), MAX(_x1, _x2), MAX(_y1, _y2));
}

RMDataStream &operator>>(RMDataStream &ds, RMRect &rc) {
	ds >> rc._x1 >> rc._y1 >> rc._x2 >> rc._y2;
	return ds;
}

/****************************************************************************\
*       Resource Update
\****************************************************************************/

RMResUpdate::RMResUpdate() {
	_infos = NULL;
	_numUpd = 0;
}

RMResUpdate::~RMResUpdate() {
	if (_infos) {
		delete[] _infos;
		_infos = NULL;
	}

	if (_hFile.isOpen())
		_hFile.close();
}

void RMResUpdate::init(const Common::String &fileName) {
	// Open the resource update file
	if (!_hFile.open(fileName))
		// It doesn't exist, so exit immediately
		return;

	uint8 version;
	uint32 i;

	version = _hFile.readByte();
	_numUpd = _hFile.readUint32LE();

	_infos = new ResUpdInfo[_numUpd];

	// Load the index of the resources in the file
	for (i = 0; i < _numUpd; ++i) {
		ResUpdInfo &info = _infos[i];

		info._dwRes = _hFile.readUint32LE();
		info._offset = _hFile.readUint32LE();
		info._size = _hFile.readUint32LE();
		info._cmpSize = _hFile.readUint32LE();
	}
}

HGLOBAL RMResUpdate::queryResource(uint32 dwRes) {
	// If there isn't an update file, return NULL
	if (!_hFile.isOpen())
		return NULL;

	uint32 i;
	for (i = 0; i < _numUpd; ++i)
		if (_infos[i]._dwRes == dwRes)
			// Found the index
			break;

	if (i == _numUpd)
		// Couldn't find a matching resource, so return NULL
		return NULL;

	const ResUpdInfo &info = _infos[i];
	byte *cmpBuf = new byte[info._cmpSize];
	uint32 dwRead;

	// Move to the correct offset and read in the compressed data
	_hFile.seek(info._offset);
	dwRead = _hFile.read(cmpBuf, info._cmpSize);

	if (info._cmpSize > dwRead) {
		// Error occurred reading data, so return NULL
		delete[] cmpBuf;
		return NULL;
	}

	// Allocate space for the output resource
	HGLOBAL destBuf = globalAllocate(0, info._size);
	byte *lpDestBuf = (byte *)globalLock(destBuf);
	uint32 dwSize;

	// Decompress the data
	lzo1x_decompress(cmpBuf, info._cmpSize, lpDestBuf, &dwSize);

	// Delete buffer for compressed data
	delete [] cmpBuf;

	// Return the resource
	globalUnlock(destBuf);
	return destBuf;
}

} // End of namespace Tony
