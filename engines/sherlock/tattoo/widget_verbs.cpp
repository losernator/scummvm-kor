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

#include "sherlock/tattoo/widget_verbs.h"
#include "sherlock/tattoo/tattoo_scene.h"
#include "sherlock/tattoo/tattoo_user_interface.h"
#include "sherlock/tattoo/tattoo_people.h"
#include "sherlock/tattoo/tattoo.h"

namespace Sherlock {

namespace Tattoo {

WidgetVerbs::WidgetVerbs(SherlockEngine *vm) : WidgetBase(vm) {
	_selector = _oldSelector = -1;
	_outsideMenu = false;
}

void WidgetVerbs::activateVerbMenu(bool objectsOn) {
	Talk &talk = *_vm->_talk;
	FixedText &fixedText = *_vm->_fixedText;
	TattooUserInterface &ui = *(TattooUserInterface *)_vm->_ui;
	TattooPeople &people = *(TattooPeople *)_vm->_people;
	bool isWatson = false;
	Common::String strLook = fixedText.getText(kFixedText_Verb_Look);
	Common::String strTalk = fixedText.getText(kFixedText_Verb_Talk);
	Common::String strJournal = fixedText.getText(kFixedText_Verb_Journal);

	if (talk._talkToAbort)
		return;

	_outsideMenu = false;

	_verbCommands.clear();

	// Check if we need to show options for the highlighted object
	if (objectsOn) {
		// Set the verb list accordingly, depending on the target being a
		// person or an object
		if (ui._personFound) {
			TattooPerson &person = people[ui._activeObj - 1000];
			TattooPerson &npc = people[ui._activeObj - 1001];

			if (!scumm_strnicmp(npc._npcName.c_str(), "WATS", 4))
				isWatson = true;

			if (!scumm_strnicmp(person._examine.c_str(), "_EXIT", 5))
				_verbCommands.push_back(strLook);
			
			_verbCommands.push_back(strTalk);

			// Add any extra active verbs from the NPC's verb list
			// TODO
		} else {
			if (!scumm_strnicmp(ui._bgShape->_name.c_str(), "WATS", 4))
				isWatson = true;

			if (!scumm_strnicmp(ui._bgShape->_examine.c_str(), "_EXIT", 5))
				_verbCommands.push_back(strLook);

			if (ui._bgShape->_aType == PERSON)
				_verbCommands.push_back(strTalk);

			// Add any extra active verbs from the NPC's verb list
			// TODO
		}
	}

	if (isWatson)
		_verbCommands.push_back(strJournal);

	// Add the system commands
	// TODO

	// Find the widest command
	// TODO

	// TODO: Finish this
}

void WidgetVerbs::execute() {
	Events &events = *_vm->_events;
	FixedText &fixedText = *_vm->_fixedText;
	People &people = *_vm->_people;
	TattooScene &scene = *(TattooScene *)_vm->_scene;
	Talk &talk = *_vm->_talk;
	TattooUserInterface &ui = *(TattooUserInterface *)_vm->_ui;
	Common::Point mousePos = events.mousePos();
	Common::Point scenePos = mousePos + ui._currentScroll;
	bool noDesc = false;

	Common::String strLook = fixedText.getText(kFixedText_Verb_Look);
	Common::String strTalk = fixedText.getText(kFixedText_Verb_Talk);
	Common::String strJournal = fixedText.getText(kFixedText_Verb_Journal);

	checkTabbingKeys(_verbCommands.size());

	// Highlight verb display as necessary
	highlightVerbControls();

	// Flag if the user has started pressing the button with the cursor outsie the menu
	if (events._firstPress && !_bounds.contains(mousePos))
		_outsideMenu = true;

	// See if they released the mouse button
	if (events._released || events._released) {
		// See if they want to close the menu (they clicked outside of the menu)
		if (!_bounds.contains(mousePos)) {
			if (_outsideMenu) {
				// Free the current menu graphics & erase the menu
				banishWindow();

				if (events._rightReleased) {
					// Reset the selected shape to what was clicked on
					ui._bgFound = scene.findBgShape(scenePos);
					ui._personFound = ui._bgFound >= 1000;
					Object *_bgShape = ui._personFound ? nullptr : &scene._bgShapes[ui._bgFound];

					if (ui._personFound) {
						if (people[ui._bgFound - 1000]._description.empty() || people[ui._bgFound - 1000]._description.hasPrefix(" "))
							noDesc = true;
					} else if (ui._bgFound != -1) {
						if (_bgShape->_description.empty() || _bgShape->_description.hasPrefix(" "))
							noDesc = true;
					} else {
						noDesc = true;
					}

					// Call the Routine to turn on the Commands for this Object
					activateVerbMenu(!noDesc);
				} else {
					// See if we're in a Lab Table Room
					ui._menuMode = scene._labTableScene ? LAB_MODE : STD_MODE;
				}
			}
		} else if (_bounds.contains(mousePos)) {
			// Mouse is within the menu
			// Erase the menu
			banishWindow();

			// See if they are activating the Look Command
			if (!_verbCommands[_selector].compareToIgnoreCase(strLook)) {
				ui._bgFound = ui._activeObj;
				if (ui._activeObj >= 1000) {
					ui._personFound = true;
				} else {
					ui._personFound = false;
					ui._bgShape = &scene._bgShapes[ui._activeObj];
				}

				ui.lookAtObject();

			} else if (!_verbCommands[_selector].compareToIgnoreCase(strTalk)) {
				// Talk command is being activated
				talk.talk(ui._activeObj);
				ui._activeObj = -1;
			
			} else if (!_verbCommands[_selector].compareToIgnoreCase(strJournal)) {
				ui.doJournal();

				// See if we're in a Lab Table scene
				ui._menuMode = scene._labTableScene ? LAB_MODE : STD_MODE;
			} else if (_selector >= ((int)_verbCommands.size() - 2)) {
				switch (_selector - (int)_verbCommands.size() + 2) {
				case 0:
					// Inventory
					ui.doInventory(2);
					break;

				case 1:
					// Options
					ui.doControls();
					break;

				default:
					ui._menuMode = scene._labTableScene ? LAB_MODE : STD_MODE;
					break;
				}
			} else {
				// If they have selected anything else, process it
				people[HOLMES].gotoStand();

				if (ui._activeObj < 1000) {
					for (int idx = 0; idx < 6; ++idx) {
						if (!_verbCommands[_selector].compareToIgnoreCase(scene._bgShapes[ui._activeObj]._use[idx]._verb)) {
							// See if they are Picking this object up
							if (!scene._bgShapes[ui._activeObj]._use[idx]._target.compareToIgnoreCase("*PICKUP"))
								ui.pickUpObject(ui._activeObj);
							else
								ui.checkAction(scene._bgShapes[ui._activeObj]._use[idx], ui._activeObj);
						}
					}
				} else {
					for (int idx = 0; idx < 2; ++idx) {
						if (!_verbCommands[_selector].compareToIgnoreCase(people[ui._activeObj - 1000]._use[idx]._verb))
							ui.checkAction(people[ui._activeObj - 1000]._use[idx], ui._activeObj);
					}
				}

				ui._activeObj = -1;
				if (ui._menuMode != MESSAGE_MODE) {
					// See if we're in a Lab Table Room
					ui._menuMode = scene._labTableScene ? LAB_MODE : STD_MODE;
				}
			}
		}
	} else if (ui._keyState.keycode == Common::KEYCODE_ESCAPE) {
		// User closing the menu with the ESC key
		banishWindow();
		ui._menuMode = scene._labTableScene ? LAB_MODE : STD_MODE;
	}
}

void WidgetVerbs::highlightVerbControls() {
	Events &events = *_vm->_events;
	Screen &screen = *_vm->_screen;
	Common::Point mousePos = events.mousePos();

	// Get highlighted verb
	_selector = -1;
	Common::Rect bounds = _bounds;
	bounds.grow(-3);
	if (bounds.contains(mousePos))
		_selector = (mousePos.y - bounds.top) / (screen.fontHeight() + 7);

	// See if a new verb is being pointed at
	if (_selector != _oldSelector) {
		// Redraw the verb list
		for (int idx = 0; idx < (int)_verbCommands.size(); ++idx) {
			byte color = (idx == _selector) ? (byte)COMMAND_HIGHLIGHTED : (byte)INFO_TOP;
			_surface.writeString(_verbCommands[idx], Common::Point((_bounds.width() - screen.stringWidth(_verbCommands[idx])) / 2,
				(screen.fontHeight() + 7) * idx + 5), color);
		}

		_oldSelector = _selector;
	}
}

} // End of namespace Tattoo

} // End of namespace Sherlock
