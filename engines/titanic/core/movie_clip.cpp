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
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "titanic/core/movie_clip.h"

namespace Titanic {

CMovieClip::CMovieClip() {
}

void CMovieClip::save(SimpleFile *file, int indent) const {
	file->writeNumberLine(2, indent);
	file->writeQuotedLine("Clip", indent);
	file->writeQuotedLine(_name, indent);
	file->writeNumberLine(_startFrame, indent);
	file->writeNumberLine(_endFrame, indent);

	ListItem::save(file, indent);
}

void CMovieClip::load(SimpleFile *file) {
	int val = file->readNumber();

	switch (val) {
	case 1:
		_name = file->readString();
		_startFrame = file->readNumber();
		_endFrame = file->readNumber();
		_field20 = file->readNumber();
		_field24 = file->readNumber();
		_field28 = file->readNumber();
		_field2C = file->readNumber();
		_field30 = file->readNumber();
		break;

	case 2:
		file->readString();
		_name = file->readString();
		_startFrame = file->readNumber();
		_endFrame = file->readNumber();
		break;

	default:
		break;
	}

	ListItem::load(file);
}

CMovieClip *CMovieClipList::findByName(const Common::String &name) const {
	for (const_iterator i = begin(); i != end(); ++i) {
		CMovieClip *clip = *i;
		if (clip->_name == name)
			return clip;
	}

	return nullptr;
}

} // End of namespace Titanic
