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

#ifndef TITANIC_PET_SLIDER_H
#define TITANIC_PET_SLIDER_H

#include "titanic/support/rect.h"
#include "titanic/support/string.h"
#include "titanic/core/game_object.h"

namespace Titanic {

enum SliderOrientation { ORIENTATION_HORIZONTAL = 1, ORIENTATION_VERTICAL = 2 };

class CPetControl;

class CPetSlider {
private:
	int _flags;
	Rect _bounds;
	Rect _slidingRect;
	int _thumbWidth;
	int _thumbHeight;
	int _sliderOffset;
	bool _thumbFocused;
	Rect _dirtyArea;
private:
	/**
	 * Center the center position of the slider's thumb
	 */
	Point getThumbCentroidPos() const;

	/**
	 * Returns true if the passed point is within the thumb
	 */
	bool thumbContains(const Point &pt) const;

	/**
	 * Gets the area the slider's thumbnail covers
	 */
	Rect getThumbRect() const;

	/**
	 * Calculates the slider offset at the specificed position
	 */
	int calcSliderOffset(const Point &pt) const;
protected:
	/**
	 * Get the position to draw the background at
	 */
	Point getBackgroundDrawPos();

	/**
	 * Get the position to draw the slider thumbnail at
	 */
	Point getThumbDrawPos();

	/**
	 * Returns true if the passed point falls within the slider's bounds
	 */
	bool containsPt(const Point &pt) const { return _bounds.contains(pt); }
public:
	CPetSlider();

	/**
	 * Setup the background
	 */
	virtual void setupBackground(const CString &name, CPetControl *petControl) {}

	/**
	 * Setup the thumb
	 */
	virtual void setupThumb(const CString &name, CPetControl *petControl) {}

	/**
	 * Setup the background
	 */
	virtual void setupBackground2(const CString &name, CPetControl *petControl) {}

	/**
	 * Setup the thumb
	 */
	virtual void setupThumb2(const CString &name, CPetControl *petControl) {}

	/**
	 * Reset the slider
	 */
	virtual void reset(const CString &name) {}
	
	/**
	 * Draw the slider
	 */
	virtual void draw(CScreenManager *screenManager) {}

	/**
	 * Reset the dirty area
	 */
	virtual Rect clearDirtyArea();
	
	/**
	 * Checks whether the slider is highlighted
	 */
	virtual bool checkThumb(const Point &pt);

	/**
	 * Resets the thumb focused flag
	 */
	virtual bool resetThumbFocus();

	virtual void proc10();
	virtual void proc11();
	virtual bool proc12(const Point &pt);
	virtual void proc13();
	virtual void proc14();
	
	
	virtual bool contains(const Point &pt) const;

	/**
	 * Returns the slider offset in pixels
	 */
	virtual double getOffsetPixels() const;

	/**
	 * Sets the slider offset
	 */
	virtual void setSliderOffset(double offset);

	/**
	 * Set a new slider offset in pixels
	 */
	virtual void setOffsetPixels(int offset);
};

class CPetSoundSlider : public CPetSlider {
public:
	CGameObject *_background;
	CGameObject *_thumb;
public:
	CPetSoundSlider() : CPetSlider(), _background(nullptr),
		_thumb(0) {}

	/**
	 * Setup the background
	 */
	virtual void setupBackground(const CString &name, CPetControl *petControl);

	/**
	 * Setup the thumb
	 */
	virtual void setupThumb(const CString &name, CPetControl *petControl);

	/**
	 * Setup the background
	 */
	virtual void setupBackground2(const CString &name, CPetControl *petControl);

	/**
	 * Setup the thumb
	 */
	virtual void setupThumb2(const CString &name, CPetControl *petControl);

	/**
	 * Draw the slider
	 */
	virtual void draw(CScreenManager *screenManager);
};

} // End of namespace Titanic

#endif /* TITANIC_PET_SLIDER_H */
