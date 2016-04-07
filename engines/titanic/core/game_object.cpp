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

#include "titanic/support/files_manager.h"
#include "titanic/game_manager.h"
#include "titanic/support/screen_manager.h"
#include "titanic/titanic.h"
#include "titanic/support/video_surface.h"
#include "titanic/core/game_object.h"
#include "titanic/core/resource_key.h"
#include "titanic/pet_control/pet_control.h"

namespace Titanic {

BEGIN_MESSAGE_MAP(CGameObject, CNamedItem)
END_MESSAGE_MAP()

void *CGameObject::_v1 = nullptr;

CGameObject::CGameObject(): CNamedItem() {
	_bounds = Rect(0, 0, 15, 15);
	_field34 = 0;
	_field38 = 0;
	_field3C = 0;
	_field40 = 0;
	_field44 = 0xF0;
	_field48 = 0xF0;
	_field4C = 0xFF;
	_field50 = 0;
	_field54 = 0;
	_field58 = 0;
	_visible = true;
	_field60 = 0;
	_cursorId = CURSOR_ARROW;
	_field78 = 0;
	_frameNumber = -1;
	_field90 = 0;
	_field94 = 0;
	_field98 = 0;
	_field9C = 0;
	_surface = nullptr;
	_fieldB8 = 0;
}

void CGameObject::save(SimpleFile *file, int indent) const {
	file->writeNumberLine(7, indent);
	error("TODO: CGameObject::save");

	CNamedItem::save(file, indent);
}

void CGameObject::load(SimpleFile *file) {
	int val = file->readNumber();
	CResourceKey resourceKey;

	switch (val) {
	case 7:
		_clipList2.load(file);
		_frameNumber = file->readNumber();
		// Deliberate fall-through

	case 6:
		_cursorId = (CursorId)file->readNumber();
		// Deliberate fall-through

	case 5:
		_clipList1.load(file);
		// Deliberate fall-through

	case 4:
		_field60 = file->readNumber();
		// Deliberate fall-through

	case 3:
		_field40 = file->readNumber();
		// Deliberate fall-through

	case 2:
		_resource = file->readString();
		// Deliberate fall-through

	case 1:
		_bounds = file->readBounds();
		_field34 = file->readFloat();
		_field38 = file->readFloat();
		_field3C = file->readFloat();
		_field44 = file->readNumber();
		_field48 = file->readNumber();
		_field4C = file->readNumber();
		_fieldB8 = file->readNumber();
		_visible = file->readNumber() != 0;
		_field50 = file->readNumber();
		_field54 = file->readNumber();
		_field58 = file->readNumber();

		resourceKey.load(file);		
		_surface = nullptr;
		val = file->readNumber();
		if (val) {
			_resource = resourceKey.getString();
		}
		break;

	default:
		break;
	}

	CNamedItem::load(file);
}

void CGameObject::stopMovie() {
	if (_surface)
		_surface->stopMovie();
}

bool CGameObject::checkPoint(const Point &pt, bool ignore40, bool visibleOnly) {
	if ((!_visible && visibleOnly) || !_bounds.contains(pt))
		return false;

	if (ignore40 || _field40)
		return true;

	if (!_surface) {
		if (_frameNumber == -1)
			return true;
		loadFrame(_frameNumber);
		if (!_surface)
			return true;
	}

	Common::Point pixelPos = pt - _bounds;
	if (_surface->_blitStyleFlag) {
		pixelPos.y = ((_bounds.height() - _bounds.top) / 2) * 2 - pixelPos.y;
	}

	uint transColor = _surface->getTransparencyColor();
	uint pixel = _surface->getPixel(pixelPos);
	return pixel != transColor;
}

void CGameObject::draw(CScreenManager *screenManager) {
	if (!_visible)
		return;
	if (_v1) {
		error("TODO: Block in CGameObject::draw");
	}

	if (_field40) {
		if (_field90) {
			if (_bounds.intersects(getGameManager()->_bounds))
				warning("TODO: _field90(screenManager);");
		}
	} else {
		if (!_surface) {
			if (!_resource.empty()) {
				loadResource(_resource);
				_resource = "";
			}
		}

		if (_surface) {
			_bounds.right = _surface->getWidth();
			_bounds.bottom = _surface->getHeight();

			if (!_bounds.right || !_bounds.bottom)
				return;

			if (_frameNumber >= 0) {
				loadFrame(_frameNumber);
				_frameNumber = -1;
			}

			if (!_clipList2.empty())
				processClipList2();

			if (_bounds.intersects(getGameManager()->_bounds)) {
				if (_surface) {
					Point destPos(_bounds.left, _bounds.top);
					screenManager->blitFrom(SURFACE_BACKBUFFER, _surface, &destPos);
				}

				if (_field90)
					warning("TODO: sub_415f80(screenManager);");
			}
		}
	}
}

void CGameObject::draw(CScreenManager *screenManager, const Common::Point &destPos) {
	if (!_surface && !_resource.empty()) {
		loadResource(_resource);
		_resource.clear();
	}

	if (_surface) {
		int xSize = _surface->getWidth();
		int ySize = _surface->getHeight();

		if (xSize > 0 && ySize > 0) {
			screenManager->blitFrom(SURFACE_BACKBUFFER, _surface, &destPos);
		}
	}
}

void CGameObject::loadResource(const CString &name) {
	switch (name.imageTypeSuffix()) {
	case FILETYPE_IMAGE:
		loadImage(name);
		break;
	case FILETYPE_MOVIE:
		loadMovie(name);
		break;
	}
}

void CGameObject::loadMovie(const CString &name, bool pendingFlag) {
	warning("TODO: CGameObject::loadMovie");
}

void CGameObject::loadImage(const CString &name, bool pendingFlag) {
	// Get a refernce to the game and screen managers
	CGameManager *gameManager = getGameManager();
	CScreenManager *screenManager;

	if (gameManager && (screenManager = CScreenManager::setCurrent()) != nullptr) {
		// Destroy the object's surface if it already had one
		if (_surface) {
			delete _surface;
			_surface = nullptr;
		}

		g_vm->_filesManager.fn5(name);

		if (!name.empty()) {
			_surface = new OSVideoSurface(screenManager, CResourceKey(name), pendingFlag);
		}

		if (_surface && !pendingFlag) {
			_bounds.right = _surface->getWidth();
			_bounds.bottom = _surface->getHeight();
		}

		// Mark the object's area as dirty, so that on the next frame rendering
		// this object will be redrawn
		makeDirty();
	}

	_field78 = 0;
}

void CGameObject::loadFrame(int frameNumber) {
	if (frameNumber != -1 && !_resource.empty())
		loadResource(_resource);
	
	if (_surface)
		_surface->setMovieFrame(frameNumber);

	makeDirty();
}

void CGameObject::processClipList2() {
	warning("CGameObject::processClipList2");
}

void CGameObject::makeDirty(const Rect &r) {
	CGameManager *gameManager = getGameManager();
	if (gameManager)
		gameManager->extendBounds(r);
}

void CGameObject::makeDirty() {
	makeDirty(_bounds);
}

bool CGameObject::soundFn1(int val) {
	if (val != 0 && val != -1) {
		CGameManager *gameManager = getGameManager();
		if (gameManager)
			return gameManager->_sound.fn1(val);
	}

	return false;
}

void CGameObject::soundFn2(int val, int val2) {
	if (val != 0 && val != -1) {
		CGameManager *gameManager = getGameManager();
		if (gameManager) {
			if (val2)
				gameManager->_sound.fn3(val, 0, val2);
			else
				gameManager->_sound.fn2(val);
		}
	}
}

void CGameObject::setVisible(bool val) {
	if (val != _visible) {
		_visible = val;
		makeDirty();
	}
}

void CGameObject::petFn2(int val) {
	CPetControl *pet = getPetControl();
	if (pet)
		pet->fn2(val);
}

void CGameObject::petFn3(CTreeItem *item) {
	CPetControl *pet = getPetControl();
	if (pet)
		pet->fn3(item);
}

void CGameObject::fn1(int val1, int val2, int val3) {
	_frameNumber = -1;
	if (!_surface) {
		if (!_resource.empty())
			loadResource(_resource);
		_resource.clear();
	}

	if (_surface) {
		_surface->proc34(val1, val2, val3, val3 != 0);

		if (val3 & 0x10)
			getGameManager()->_gameState.addMovie(_surface->_movie);
	}
}

void CGameObject::changeStatus(int newStatus) {
	if (_frameNumber == -1 && !_resource.empty()) {
		loadResource(_resource);
		_resource.clear();
	}

	CVideoSurface *surface = (newStatus & 4) ? _surface : nullptr;
	if (_surface) {
		_surface->proc32(newStatus, surface);
		
		if (newStatus & 0x10) {
			getGameManager()->_gameState.addMovie(_surface->_movie);
		}
	}
}

void CGameObject::savePosition() {
	_savedPos = _bounds;
}

void CGameObject::resetPosition() {
	setPosition(_savedPos);
}

void CGameObject::setPosition(const Common::Point &newPos) {
	makeDirty();
	_bounds.moveTo(newPos);
	makeDirty();
}

bool CGameObject::checkStartDragging(CMouseDragStartMsg *msg) {
	if (_visible && checkPoint(msg->_mousePos, msg->_field14, 1)) {
		savePosition();
		msg->_dragItem = this;
		return true;
	} else {
		return false;
	}
}

void CGameObject::setPetArea(PetArea newArea) const {
	CPetControl *pet = getPetControl();
	if (pet)
		pet->setArea(newArea);
}

bool CGameObject::hasActiveMovie() const {
	if (_surface && _surface->_movie)
		return _surface->_movie->isActive();
	return false;
}

int CGameObject::getMovie19() const {
	if (_surface && _surface->_movie)
		return _surface->_movie->proc19();
	return _field78;
}

int CGameObject::getSurface45() const {
	return _surface ? _surface->proc45() : 0;
}

void CGameObject::soundProximity(const CString &name, int val2, int val3, int val4) {
	CProximity prox;
	prox._field8 = val2;
	prox._fieldC = val3;
	prox._field20 = val4;
	soundProximity(name, prox);
}

void CGameObject::soundProximity(const CString &name, CProximity &prox) {
	if (prox._field28 == 2) {
		// TODO
	}
}

} // End of namespace Titanic
