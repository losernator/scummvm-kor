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

#include "common/scummsys.h"
#include "mads/mads.h"
#include "mads/player.h"

namespace MADS {

#define PLAYER_SEQ_INDEX -2

const int Player::_directionListIndexes[32] = {
	0, 7, 4, 3, 6, 0, 2, 5, 0, 1, 9, 4, 1, 2, 7, 9, 3, 8, 9, 6, 7, 2, 3, 6, 1, 7, 9, 4, 7, 8, 0, 0
};

Player::Player(MADSEngine *vm): _vm(vm) {
	_action = nullptr;
	_facing = FACING_NORTH;
	_turnToFacing = FACING_NORTH;
	_targetFacing = FACING_NORTH;
	_mirror = false;
	_spritesLoaded = false;
	_spritesStart = 0;
	_spritesIdx = 0;
	_numSprites = 0;
	_stepEnabled = false;
	_visible = false;
	_priorVisible = false;
	_visible3 = false;
	_loadsFirst = false;
	_loadedFirst = false;
	_walkAnywhere = false;
	_special = 0;
	_ticksAmount = 0;
	_priorTimer = 0;
	_trigger = 0;
	_scalingVelocity = false;
	_spritesChanged = false;
	_forceRefresh = false;
	_highSprites = false;
	_currentDepth = 0;
	_currentScale = 0;
	_frameNumber = 0;
	_centerOfGravity = 0;
	_frameCount = 0;
	_velocity = 0;
	_upcomingTrigger = 0;
	_trigger = 0;
	_frameListIndex = 0;
	_stopWalkerIndex = 0;
	_totalDistance = 0;
	
	Common::fill(&_stopWalkerList[0], &_stopWalkerList[12], 0);
	Common::fill(&_stopWalkerTrigger[0], &_stopWalkerTrigger[12], 0);
	Common::fill(&_spriteSetsPresent[0], &_spriteSetsPresent[PLAYER_SPRITES_FILE_COUNT], false);
}

void Player::cancelWalk() {
	_action = &_vm->_game->_scene._action;
	_targetPos = _playerPos;
	_targetFacing = FACING_NONE;
	_turnToFacing = _facing;
	_moving = false;
	_walkOffScreen = _walkOffScreenSceneId = 0;
	_next = 0;
	_routeCount = 0;
	_walkAnywhere = false;

	_action->_startWalkFlag = false;
	_action->_walkFlag = false;
}

bool Player::loadSprites(const Common::String &prefix) {
	Common::String suffixList = "89632741";

	Common::String newPrefix;
	if (prefix.empty()) {
		newPrefix = _spritesPrefix;
	} else {
		_spritesPrefix = prefix;
		newPrefix = prefix;
	}

	_numSprites = 0;
	if (!_spritesPrefix.empty()) {
		for (int fileIndex = 0; fileIndex < PLAYER_SPRITES_FILE_COUNT; ++fileIndex) {
			Common::String setName = Common::String::format("*%s_%c.SS",
				newPrefix.c_str(), suffixList[fileIndex]);
			if (fileIndex >= 5)
				_highSprites = true;

			_spriteSetsPresent[fileIndex] = true;

			int setIndex = -1;
			if (Common::File::exists(setName)) {
				setIndex = _vm->_game->_scene._sprites.addSprites(setName, 4);
				++_numSprites;
			}  else if (fileIndex < 5) {
				_highSprites = 0;
				return true;
			} else {
				_spriteSetsPresent[fileIndex] = false;
			}

			if (fileIndex == 0)
				_spritesStart = setIndex;
		}

		_spritesLoaded = true;
		_spritesChanged = false;
		_highSprites = false;
		return false;
	} else {
		Common::fill(&_spriteSetsPresent[0], &_spriteSetsPresent[PLAYER_SPRITES_FILE_COUNT], false);
		_highSprites = false;
		return true;
	}
}

void Player::setFinalFacing() {
	if (_targetFacing != FACING_NONE)
		_turnToFacing = _targetFacing;
}

void Player::changeFacing() {
	int dirIndex = 0, dirIndex2 = 0;
	int newDir = 0, newDir2 = 0;

	if (_facing != _turnToFacing) {
		// Find the index for the given direction in the player direction list
		int tempDir = _facing;
		do {
			++dirIndex;
			newDir += tempDir;
			tempDir = _directionListIndexes[tempDir + 10];
		} while (tempDir != _turnToFacing);
	}


	if (_facing != _turnToFacing) {
		// Find the index for the given direction in the player direction list
		int tempDir = _facing;
		do {
			++dirIndex2;
			newDir2 += tempDir;
			tempDir = _directionListIndexes[tempDir + 20];
		} while (tempDir != _turnToFacing);
	}

	int diff = dirIndex - dirIndex2;
	if (diff == 0)
		diff = newDir - newDir2;

	_facing = (diff >= 0) ? (Facing)_directionListIndexes[_facing + 20] :
		(Facing)_directionListIndexes[_facing + 10];
	selectSeries();

	if ((_facing == _turnToFacing) && !_moving)
		updateFrame();

	_priorTimer += 1;
}

void Player::cancelCommand() {
	cancelWalk();
	_action->_inProgress = false;
}

void Player::selectSeries() {
	Scene &scene = _vm->_game->_scene;

	clearStopList();
	_mirror = false;

	_spritesIdx = _directionListIndexes[_facing];
	if (!_spriteSetsPresent[_spritesIdx]) {
		// Direction isn't present, so use alternate direction, with entries flipped
		_spritesIdx -= 4;
		_mirror = true;
	}

	SpriteAsset &spriteSet = *scene._sprites[_spritesStart + _spritesIdx];
	assert(spriteSet._charInfo);
	_velocity = MAX(spriteSet._charInfo->_velocity, 100);
	setBaseFrameRate();

	_frameCount = spriteSet._charInfo->_totalFrames;
	if (_frameCount == 0)
		_frameCount = spriteSet.getCount();

	_centerOfGravity = spriteSet._charInfo->_centerOfGravity;

	if ((_frameNumber <= 0) || (_frameNumber > _frameCount))
		_frameNumber = 1;

	_forceRefresh = true;
}

void Player::updateFrame() {
	Scene &scene = _vm->_game->_scene;
	SpriteAsset &spriteSet = *scene._sprites[_spritesStart + _spritesIdx];
	assert(spriteSet._charInfo);

	if (!spriteSet._charInfo->_numEntries) {
		_frameNumber = 1;
	} else {
		_frameListIndex = _stopWalkerList[_stopWalkerIndex];

		if (!_visible) {
			_upcomingTrigger = 0;
		}
		else {
			_upcomingTrigger = _stopWalkerTrigger[_stopWalkerIndex];

			if (_stopWalkerIndex > 0)
				--_stopWalkerIndex;
		}

		// Set the player frame number
		int listIndex = ABS(_frameListIndex);
		_frameNumber = (_frameListIndex >= 0) ? spriteSet._charInfo->_startFrames[listIndex] :
			spriteSet._charInfo->_stopFrames[listIndex];

		// Set next waiting period in ticks
		if (listIndex == 0) {
			setBaseFrameRate();
		} else {
			_ticksAmount = spriteSet._charInfo->_ticksList[listIndex];
		}
	}

	_forceRefresh = true;
}

void Player::update() {
	Scene &scene = _vm->_game->_scene;

	if (_forceRefresh || (_visible != _priorVisible)) {
		int slotIndex = getSpriteSlot();
		if (slotIndex >= 0)
			scene._spriteSlots[slotIndex]._SlotType = ST_EXPIRED;

		int newDepth = 1;
		int yp = MAX(_playerPos.y, (int16)(MADS_SCENE_HEIGHT - 1));
		
		for (int idx = 1; idx < 15; ++idx) {
			if (scene._sceneInfo->_depthList[newDepth] >= yp)
				newDepth = idx + 1;
		}
		_currentDepth = newDepth;

		// Get the scale
		int newScale = getScale(_playerPos.y);
		_currentScale = MIN(newScale, 100);

		if (_visible) {
			// Player sprite needs to be rendered
			SpriteSlot slot;
			slot._SlotType = ST_FOREGROUND;
			slot._seqIndex = PLAYER_SEQ_INDEX;
			slot._spritesIndex = _spritesStart + _spritesIdx;
			slot._frameNumber = _mirror ? -_frameNumber : _frameNumber;
			slot._position.x = _playerPos.x;
			slot._position.y = _playerPos.y + (_centerOfGravity * newScale) / 100;
			slot._depth = newDepth;
			slot._scale = newScale;

			if (slotIndex >= 0) {
				// Check if the existing player slot has the same details, and can be re-used
				SpriteSlot &s2 = scene._spriteSlots[slotIndex];
				bool equal = (s2._seqIndex == slot._seqIndex) 
					&& (s2._spritesIndex == slot._spritesIndex)
					&& (s2._frameNumber == slot._frameNumber) 
					&& (s2._position == slot._position)
					&& (s2._depth == slot._depth) 
					&& (s2._scale == slot._scale);

				if (equal)
					// Undo the prior expiry of the player sprite
					s2._SlotType = ST_NONE;
				else
					slotIndex = -1;
			}

			if (slotIndex < 0) {
				// New slot needed, so allocate one and copy the slot data
				slotIndex = scene._spriteSlots.add();
				scene._spriteSlots[slotIndex] = slot;
			}

			// If changing a scene, check to change the scene when the player 
			// has moved off-screen
			if (_walkOffScreen) {
				SpriteAsset *asset = scene._sprites[slot._spritesIndex];
				MSprite *frame = asset->getFrame(_frameNumber - 1);
				int xScale = frame->w * newScale / 200;
				int yScale = frame->h * newScale / 100;
				int playerX = slot._position.x;
				int playerY = slot._position.y;

				if ((playerX + xScale) < 0 || (playerX + xScale) >= MADS_SCREEN_WIDTH ||
						playerY < 0 || (playerY + yScale) >= MADS_SCENE_HEIGHT) {
					scene._nextSceneId = _walkOffScreen;
					_walkOffScreen = 0;
					_walkAnywhere = false;
				}
			}

		}
	}

	_visible3 = _visible;
	_priorVisible = _visible;
	_forceRefresh = false;
}

void Player::clearStopList() {
	_stopWalkerList[0] = 0;
	_stopWalkerTrigger[0] = 0;
	_stopWalkerIndex = 0;
	_upcomingTrigger = 0;
	_trigger = 0;
}

void Player::setDest(const Common::Point &pt, Facing facing) {
	Scene &scene = _vm->_game->_scene;

	clearStopList();
	setBaseFrameRate();
	_moving = true;
	_targetFacing = facing;

	scene._sceneInfo->setRouteNode(scene._sceneInfo->_nodes.size() - 2,
		_playerPos, scene._depthSurface);
	scene._sceneInfo->setRouteNode(scene._sceneInfo->_nodes.size() - 1,
		pt, scene._depthSurface);

	bool v = scene._depthSurface.getDepthHighBit(pt);
	setupRoute(v);
	_next = 0;

	if (_routeCount > 0) {
		Common::Point srcPos = _playerPos;
		for (int routeCtr = _routeCount - 1; (routeCtr >= 0) && (_next == 0); --routeCtr) {
			int idx = _routeIndexes[routeCtr];
			const Common::Point &pt =scene._sceneInfo->_nodes[idx]._walkPos;

			_next = scanPath(scene._depthSurface, srcPos, pt);
			srcPos = pt;
		}
	}
}

void Player::startWalking(const Common::Point &pos, Facing direction) {
	Scene &scene = _vm->_game->_scene;

	cancelWalk();
	scene._action._startWalkFlag = true;
	scene._action._walkFlag = true;
	scene._destPos = pos;
	scene._targetFacing = direction;
}

void Player::nextFrame() {
	Scene &scene = _vm->_game->_scene;

	uint32 newTime = _priorTimer + _ticksAmount;
	if (scene._frameStartTime >= newTime) {
		_priorTimer = scene._frameStartTime;
		if (_moving) {
			move();
		} else {
			idle();
		}

		postUpdate();
		update();
	}
}

void Player::move() {
	Scene &scene = _vm->_game->_scene;
	bool newFacing = false;

	if (_moving) {
		int idx = _routeCount;
		while (!_walkOffScreen && _playerPos == _targetPos) {
			if (idx != 0) {
				--idx;
				SceneNode &node = scene._sceneInfo->_nodes[_routeIndexes[idx]];
				_targetPos = node._walkPos;
				newFacing = true;
			} else if (_walkOffScreenSceneId == idx) {
				// End of walking path
				_routeCount = 0;
				_moving = false;
				setFinalFacing();
				newFacing = true;
				idx = _routeCount;
			} else {
				_walkOffScreen = _walkOffScreenSceneId;
				_walkAnywhere = true;
				_walkOffScreenSceneId = 0;
				_stepEnabled = true;
				newFacing = false;
			}

			if (!_moving)
				break;
		}
		_routeCount = idx;
	}

	if (newFacing && _moving)
		startMovement();

	if (_turnToFacing != _facing)
		changeFacing();
	else if (!_moving)
		updateFrame();

	int velocity = _velocity;
	if (_scalingVelocity && (_totalDistance > 0)) {
		int angleRange = 100 - _currentScale;
		int angleScale = angleRange * (_posDiff.x - 1) / _totalDistance + _currentScale;
		velocity = MAX(1, 10000 / (angleScale * _currentScale * velocity));
	}

	if (!_moving || (_facing != _turnToFacing))
		return;

	Common::Point newPos = _playerPos;

	if (_distAccum < velocity) {
		do {
			if (_pixelAccum < _posDiff.x)
				_pixelAccum += _posDiff.y;
			if (_pixelAccum >= _posDiff.x) {
				if ((_posChange.y > 0) || (_walkOffScreen != 0))
					newPos.y += _yDirection;
				--_posChange.y;
				_pixelAccum -= _posDiff.x;
			}

			if (_pixelAccum < _posDiff.x) {
				if ((_posChange.x > 0) || (_walkOffScreen != 0))
					newPos.x += _xDirection;
				--_posChange.x;
			}

			if (!_walkAnywhere && !_walkOffScreen && (_walkOffScreenSceneId == 0)) {
				newFacing = scene._depthSurface.getDepthHighBit(newPos);

				if (_special == 0)
					_special = scene.getDepthHighBits(newPos);
			}

			_distAccum += _deltaDistance;

		} while ((_distAccum < velocity) && !newFacing && ((_posChange.x > 0) || (_posChange.y > 0) || (_walkOffScreen != 0)));
	}

	_distAccum -= velocity;

	if (newFacing) {
		cancelCommand();
	} else {
		if (!_walkOffScreen) {
			// If the move is complete, make sure the position is exactly on the given destination
			if (_posChange.x == 0)
				newPos.x = _targetPos.x;
			if (_posChange.y == 0)
				newPos.y = _targetPos.y;
		}

		_playerPos = newPos;
	}
}

void Player::idle() {
	Scene &scene = _vm->_game->_scene;

	if (_facing != _turnToFacing) {
		// The direction has changed, so reset for new direction
		changeFacing();
		return;
	}

	SpriteAsset &spriteSet = *scene._sprites[_spritesStart + _spritesIdx];
	assert(spriteSet._charInfo);
	if (spriteSet._charInfo->_numEntries == 0)
		// No entries, so exit immediately
		return;

	int frameIndex = ABS(_frameListIndex);
	int direction = (_frameListIndex < 0) ? -1 : 1;

	if (frameIndex >= spriteSet._charInfo->_numEntries)
		// Reset back to the start of the list
		_frameListIndex = 0;
	else {
		_frameNumber += direction;
		_forceRefresh = true;

		if (spriteSet._charInfo->_stopFrames[frameIndex] < _frameNumber) {
			_trigger = _upcomingTrigger;
			updateFrame();
		}
		if (spriteSet._charInfo->_startFrames[frameIndex] < _frameNumber) {
			_trigger = _upcomingTrigger;
			updateFrame();
		}
	}
}

void Player::postUpdate() {
	if (_moving) {
		if (++_frameNumber > _frameCount)
			_frameNumber = 1;
		_forceRefresh = true;
	} else {
		if (!_forceRefresh)
			idle();
	}
}

int Player::getSpriteSlot() {
	SpriteSlots &spriteSlots = _vm->_game->_scene._spriteSlots;

	for (uint idx = 0; idx < spriteSlots.size(); ++idx) {
		if (spriteSlots[idx]._seqIndex == PLAYER_SEQ_INDEX && 
				spriteSlots[idx]._SlotType >= ST_NONE)
			return idx;
	}

	return - 1;
}

int Player::getScale(int yp) {
	Scene &scene = _vm->_game->_scene;

	int scale = (scene._bandsRange == 0) ? scene._sceneInfo->_maxScale :
		(yp - scene._sceneInfo->_yBandsStart) * scene._scaleRange / scene._bandsRange +
		scene._sceneInfo->_minScale;

	return MIN(scale, 100);
}

void Player::setBaseFrameRate() {
	Scene &scene = _vm->_game->_scene;

	SpriteAsset &spriteSet = *scene._sprites[_spritesStart + _spritesIdx];
	assert(spriteSet._charInfo);

	_ticksAmount = spriteSet._charInfo->_ticksAmount;
	if (_ticksAmount == 0)
		_ticksAmount = 6;
}

void Player::setupRoute(bool bitFlag) {
	Scene &scene = _vm->_game->_scene;

	// Reset the flag set of nodes in use
	SceneNodeList &nodes = scene._sceneInfo->_nodes;
	for (uint i = 0; i < nodes.size(); ++i)
		nodes[i]._active = false;

	// Start constructing route node list
	_routeLength = 0x3FFF;
	_routeCount = 0;

	setupRouteNode(_tempRoute, nodes.size() - 1, bitFlag ? 0xC000 : 0x8000, 0);
}

void Player::setupRouteNode(int *routeIndexP, int nodeIndex, int flags, int routeLength) {
	Scene &scene = _vm->_game->_scene;
	SceneNodeList &nodes = scene._sceneInfo->_nodes;
	SceneNode &currentNode = nodes[nodeIndex];
	currentNode._active = true;

	*routeIndexP++ = nodeIndex;

	int subIndex = nodes.size() - 2;
	int indexVal = nodes[nodeIndex]._indexes[subIndex];
	if (indexVal & flags) {
		routeLength += indexVal & 0x3FFF;
		if (routeLength < _routeLength) {
			// Found a new shorter route to destination, so set up the route with the found one
			Common::copy(_tempRoute, routeIndexP, _routeIndexes);
			_routeCount = routeIndexP - _tempRoute;
			_routeLength = indexVal & 0x3FFF;
		}
	} else {
		for (int idx = nodes.size() - 2; idx > 0; --idx) {
			int nodePos = idx - 1;
			if (!nodes[nodePos]._active && ((currentNode._indexes[nodePos] & flags) != 0))
				setupRouteNode(routeIndexP, nodePos, 0x8000, indexVal & 0x3fff);
		}
	}

	currentNode._active = false;
}

int Player::scanPath(MSurface &depthSurface, const Common::Point &srcPos, const Common::Point &destPos) {
	Scene &scene = _vm->_game->_scene;
	
	// For compressed depth surfaces, always return 0
	if (scene._sceneInfo->_depthStyle != 2)
		return 0;

	int yDiff = destPos.y - srcPos.y;
	int yAmount = MADS_SCREEN_WIDTH;

	if (yDiff < 0) {
		yDiff = -yDiff;
		yAmount = -yAmount;
	}

	int xDiff = destPos.x - srcPos.y;
	int xDirection = 1;
	int xAmount = 0;
	if (xDiff < 0) {
		xDiff = -xDiff;
		xDirection = -xDirection;
		xAmount = MIN(yDiff, xDiff);
	}

	++xDiff;
	++yDiff;

	const byte *srcP = depthSurface.getBasePtr(srcPos.x, srcPos.y);
	int index = xAmount;

	// Outer horizontal movement loop
	for (int yIndex = 0; yIndex < yDiff; ++yIndex) {
		index += yDiff;
		int v = (*srcP & 0x7F) >> 4;
		if (v)
			return v;

		// Inner loop for handling vertical movement
		while (index >= xDiff) {
			index -= xDiff;

			v = (*srcP & 0x7F) >> 4;
			if (v)
				return v;

			srcP += yAmount;
		}

		srcP += xDirection;
	}

	return 0;
}

void Player::startMovement() {
	int xDiff = _targetPos.x - _playerPos.x;
	int yDiff = _targetPos.y - _playerPos.y;
	int srcScale = getScale(_playerPos.y);
	int destScale = getScale(_targetPos.y);

	// Sets the X direction
	if (xDiff > 0)
		_xDirection = 1;
	else if (xDiff < 0)
		_xDirection = -1;
	else
		_xDirection = 0;

	// Sets the Y direction
	if (yDiff > 0)
		_yDirection = 1;
	else if (yDiff < 0)
		_yDirection = -1;
	else
		_yDirection = 0;

	xDiff = ABS(xDiff);
	yDiff = ABS(yDiff);
	int scaleDiff = ABS(srcScale - destScale);

	int xAmt100 = xDiff * 100;
	int yAmt100 = yDiff * 100;
	int xAmt33 = xDiff * 33;

	int scaleAmount = (_scalingVelocity ? scaleDiff * 3 : 0) + 100 * yDiff / 100;
	int scaleAmount100 = scaleAmount * 100;

	// Figure out direction that will need to be moved in
	int majorDir;
	if (xDiff == 0)
		majorDir = 1;
	else if (yDiff == 0)
		majorDir = 3;
	else {
		if ((scaleAmount < xDiff) && ((xAmt33 / scaleAmount) >= 141))
			majorDir = 3;
		else if (yDiff <= xDiff)
			majorDir = 2;
		else if ((scaleAmount100 / xDiff) >= 141)
			majorDir = 1;
		else
			majorDir = 2;
	}

	switch (majorDir) {
	case 1:
		_turnToFacing = (_yDirection <= 0) ? FACING_NORTH : FACING_SOUTH;
		break;
	case 2: {
		_turnToFacing = (Facing)(((_yDirection <= 0) ? 9 : 3) - ((_xDirection <= 0) ? 2 : 0));
		break;
	}
	case 3:
		_turnToFacing = (_xDirection <= 0) ? FACING_WEST : FACING_EAST;
		break;
	default:
		break;
	}

	_totalDistance = sqrt((double)(xAmt100 * xAmt100 + yAmt100 * yAmt100));
	_posDiff.x = xDiff + 1;
	_posDiff.y = yDiff + 1;
	_posChange.x = xDiff;
	_posChange.y = yDiff;

	int majorChange = MAX(xDiff, yDiff);
	_deltaDistance = (majorChange == 0) ? 0 : _totalDistance / majorChange;

	if (_playerPos.x > _targetPos.x)
		_pixelAccum = MAX(_posChange.x, _posChange.y);
	else
		_pixelAccum = 0;

	_totalDistance /= 100;
	_distAccum = -_deltaDistance;
}

void Player::step() {
	warning("TODO: Player::step");
}

} // End of namespace MADS
