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

#ifndef MADS_ANIMATION_H
#define MADS_ANIMATION_H

#include "common/scummsys.h"
#include "common/array.h"
#include "common/rect.h"
#include "mads/msurface.h"
#include "mads/scene_data.h"
#include "mads/font.h"
#include "mads/user_interface.h"

namespace MADS {

enum AnimFlag { 
	ANIMFLAG_DITHER				= 0x0001,	// Dither to 16 colors
	ANIMFLAG_CUSTOM_FONT		= 0x0020,	// Load ccustom font 
	ANIMFLAG_LOAD_BACKGROUND	= 0x0100,	// Load background
	ANIMFLAG_LOAD_BACKGROUND_ONLY = 0x0200	// Load background only
};

class MADSEngine;
class Scene;

class AnimMessage {
public:
	int16 _soundId;
	Common::String _msg;
	Common::Point _pos;
	byte _rgb1[3], _rgb2[3];
	int _flags;
	int _startFrame, _endFrame;
	int _kernelMsgIndex;

	/**
	 * Loads data for the message from a stream
	 */
	void load(Common::SeekableReadStream *f);
};

class AnimFrameEntry {
public:
	int _frameNumber;
	int _seqIndex;
	SpriteSlotSubset _spriteSlot;

	/**
	 * Loads data for the record
	 */
	void load(Common::SeekableReadStream *f, bool uiFlag);
};

class AnimMiscEntry {
public:
	int _soundId;
	int _msgIndex;
	int _numTicks;
	Common::Point _posAdjust;
	int _field8;

	/**
	* Loads data for the record
	*/
	void load(Common::SeekableReadStream *f);
};

#define ANIM_SPAWN_COUNT 2

class AnimUIEntry {
public:
	int _probability;
	int _imageCount;
	int _firstImage;
	int _lastImage;
	int _counter;
	int _spawn[ANIM_SPAWN_COUNT];
	int _spawnFrame[ANIM_SPAWN_COUNT];
	int _sound;
	int _soundFrame;

	/**
	 * Loads the data for the record
	 */
	void load(Common::SeekableReadStream *f);
};

class AAHeader {
public:
	int _spriteSetsCount;
	int _miscEntriesCount;
	int _frameEntriesCount;
	int _messagesCount;
	byte _flags;
	int _animMode;
	int _roomNumber;
	bool _manualFlag;
	int _spritesIndex;
	Common::Point _scrollPosition;
	uint32 _scrollTicks;
	Common::String _interfaceFile;
	Common::StringArray _spriteSetNames;
	Common::String _lbmFilename;
	Common::String _spritesFilename;
	Common::String _soundName;
	Common::String _dsrName;
	Common::String _fontResource;

	/**
	 * Loads the data for a animation file header
	 */
	void load(Common::SeekableReadStream *f);
};

class Animation {
private:
	MADSEngine *_vm;
	Scene *_scene;

	Common::Array<AnimMessage> _messages;
	Common::Array<AnimMiscEntry> _miscEntries;
	Common::Array<SpriteAsset *> _spriteSets;
	Font *_font;

	int _currentFrame;
	bool _freeFlag;
	bool _skipLoad;
	int _unkIndex;
	Common::Point _unkList[2];
	uint32 _nextFrameTimer;
	uint32 _nextScrollTimer;
	int _messageCtr;
	int _trigger;
	TriggerMode _triggerMode;
	ActionDetails _actionDetails;

	/**
	 * Load data for a given frame
	 * @param frameNumber	Frame number
	 */
	void loadFrame(int frameNumber);

	bool drawFrame(SpriteAsset &spriteSet, const Common::Point &pt, int frameNumber);

	/**
	 * Load the user interface display for an animation
	 */
	void loadInterface(UserInterface &interfaceSurface, MSurface &depthSurface,
		AAHeader &header, int flags, Common::Array<RGB4> *palAnimData, SceneInfo *sceneInfo);

	/**
	 * Returns true if there is a scroll required
	 */
	bool hasScroll() const;
protected:
	Animation(MADSEngine *vm, Scene *scene);
public:
	AAHeader _header;
	Common::Array<int> _spriteListIndexes;
	Common::Array<AnimFrameEntry> _frameEntries;
	Common::Array<AnimUIEntry> _uiEntries;
	bool _resetFlag;
	int _oldFrameEntry;

	static Animation *init(MADSEngine *vm, Scene *scene);
	/*
	 * Destructor
	 */
	~Animation();

	/**
	 * Releases scene resources used by the animation, and then deletes it
	 */
	void free();

	/**
	 * Loads animation data
	 */
	void load(UserInterface &interfaceSurface, MSurface &depthSurface, const Common::String &resName,
		int flags, Common::Array<RGB4> *palAnimData, SceneInfo *sceneInfo);

	/**
	 * Preload animation data for the scene
	 */
	void preLoad(const Common::String &resName, int level);

	/**
	 * Setups up a loaded animation for playback
	 */
	void startAnimation(int abortTimers);

	/**
	 * Update the animation
	 */
	void update();

	void setNextFrameTimer(int frameNumber);
	void setCurrentFrame(int frameNumber);
	int getCurrentFrame() const { return _currentFrame; }

	bool freeFlag() const { return _freeFlag; }
	bool getAnimMode() const { return _header._animMode; }
	int roomNumber() const { return _header._roomNumber; }
};

} // End of namespace MADS

#endif /* MADS_ANIMATION_H */
