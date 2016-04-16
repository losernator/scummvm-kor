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

#ifndef TITANIC_GAME_OBJECT_H
#define TITANIC_GAME_OBJECT_H

#include "titanic/support/mouse_cursor.h"
#include "titanic/support/proximity.h"
#include "titanic/support/rect.h"
#include "titanic/core/movie_clip.h"
#include "titanic/core/named_item.h"
#include "titanic/pet_control/pet_section.h"

namespace Titanic {

enum Find { FIND_GLOBAL = 1, FIND_ROOM = 2, FIND_PET = 4, FIND_MAILMAN = 8 };
enum Found { FOUND_NONE = 0, FOUND_GLOBAL = 1, FOUND_ROOM = 2, FOUND_PET = 3, FOUND_MAILMAN = 4 };

class CVideoSurface;
class CMouseDragStartMsg;
class OSMovie;

class CGameObject : public CNamedItem {
	friend class OSMovie;
	DECLARE_MESSAGE_MAP
public:
	static void *_v1;
private:
	/**
	 * Load a visual resource for the object
	 */
	void loadResource(const CString &name);

	/**
	 * Loads a movie
	 */
	void loadMovie(const CString &name, bool pendingFlag = true);

	/**
	 * Loads an image
	 */
	void loadImage(const CString &name, bool pendingFlag = true);

	void processClipList2();
protected:
	Rect _bounds;
	double _field34;
	double _field38;
	double _field3C;
	int _field40;
	int _field44;
	int _field48;
	int _field4C;
	int _field50;
	int _field54;
	int _field58;
	bool _visible;
	CMovieClipList _clipList1;
	int _initialFrame;
	CMovieClipList _clipList2;
	int _frameNumber;
	int _field90;
	int _field94;
	int _field98;
	int _field9C;
	Common::Point _savedPos;
	CVideoSurface *_surface;
	CString _resource;
	int _fieldB8;
protected:
	/**
	 * Saves the current position the object is located at
	 */
	void savePosition();

	/**
	 * Resets the object back to the previously saved starting position
	 */
	void resetPosition();

	/**
	 * Check for starting to drag the object
	 */
	bool checkStartDragging(CMouseDragStartMsg *msg);

	/**
	 * Marks the area in the passed rect as dirty, and requiring re-rendering
	 */
	void makeDirty(const Rect &r);

	/**
	 * Marks the area occupied by the object as dirty, requiring re-rendering
	 */
	void makeDirty();

	/**
	 * Sets a new area in the PET
	 */
	void setPetArea(PetArea newArea) const;

	/**
	 * Goto a new view
	 */
	void gotoView(const CString &viewName, const CString &clipName);

	/**
	 * Parses a view into it's components of room, node, and view,
	 * and locates the designated view
	 */
	CViewItem * parseView(const CString &viewString);

	bool soundFn1(int val);
	void soundFn2(int val, int val2);
	void petFn2(int val);
	void petFn3(CTreeItem *item);
	void incState38();

	/**
	 * Plays a sound
	 */
	bool playSound(const CString &name, int val2, int val3, int val4);

	/**
	 * Plays a sound
	 */
	bool playSound(const CString &name, CProximity &prox);

	/**
	 * Adds a timer
	 */
	int addTimer(int endVal, uint firstDuration, uint duration);

	/**
	 * Adds a timer
	 */
	int addTimer(uint firstDuration, uint duration);

	/**
	 * Stops a timer
	 */
	void stopTimer(int id);

	/**
	 * Causes the game to sleep for the specified time
	 */
	void sleep(uint milli);

	/**
	 * Get the current mouse cursor position
	 */
	Point getMousePos() const;

	/*
	 * Compares the current view's name in a Room.Node.View tuplet
	 * string form to the passed string
	 */
	bool compareViewNameTo(const CString &name) const;

	/**
	* Display a message in the PET
	*/
	void petDisplayMsg(const CString &msg) const;

	/**
	 * Gets the first object under the system MailMan
	 */
	CGameObject *getMailManFirstObject() const;

	/**
	 * Gets the next object under the system MailMan
	 */
	CGameObject *getMailManNextObject(CGameObject *prior) const;

	/**
	 * Finds an object by name within the object's room
	 */
	CGameObject *findRoomObject(const CString &name) const;

	/**
	 * Finds an item in various system areas
	 */
	Found find(const CString &name, CGameObject **item, int findAreas);

	/**
	 * Scan the specified room for an item by name
	 */
	static CGameObject *findUnder(CTreeItem *parent, const CString &name);

	/**
	 * Moves the item from it's original position to be under the hidden room
	 */
	void moveToHiddenRoom();

	/**
	 * Moves the item from it's original position to be under the current view
	 */
	void moveToView();

	void trueTalkFn1(CTreeItem *item, int val2, int val3);

	/**
	 * Load the surface
	 */
	void loadSurface();
public:
	int _field60;
	CursorId _cursorId;
public:
	CLASSDEF
	CGameObject();

	/**
	 * Save the data for the class to file
	 */
	virtual void save(SimpleFile *file, int indent) const;

	/**
	 * Load the data for the class from file
	 */
	virtual void load(SimpleFile *file);

	/**
	 * Allows the item to draw itself
	 */
	virtual void draw(CScreenManager *screenManager);

	/**
	 * Allows the item to draw itself
	 */
	virtual void draw(CScreenManager *screenManager, const Common::Point &destPos);

	/**
	 * Returns true if the item is the PET control
	 */
	virtual bool isPet() const;

	/**
	 * Stops any movie currently playing for the object
	 */
	void stopMovie();

	/**
	 * Checks the passed point is validly in the object,
	 * with extra checking of object flags status
	 */
	bool checkPoint(const Point &pt, bool ignore40 = false, bool visibleOnly = false);

	/**
	 * Set the position of the object
	 */
	void setPosition(const Point &newPos);

	/**
	 * Returns true if the object has a currently active movie
	 */
	bool hasActiveMovie() const;

	int getMovie19() const;
	int getSurface45() const;
	void sound8(bool flag) const;

	/**
	 * Loads a frame
	 */
	void loadFrame(int frameNumber);

	/**
	 * Change the object's status
	 */
	void playMovie(uint flags);

	/**
	 * Play the movie specified in _resource
	 */
	void playMovie(uint startFrame, uint endFrame, int val3);

	/**
	 * Play an arbitrary clip
	 */
	void playClip(const CString &name, uint flags);

	/**
	 * Return the current view/node/room as a single string
	 */
	CString getViewFullName() const;

	/**
	 * Sets whether the object is visible
	 */
	void setVisible(bool val);

	
};

} // End of namespace Titanic

#endif /* TITANIC_GAME_OBJECT_H */
