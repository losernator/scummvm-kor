/* ScummVM - Scumm Interpreter
 * Copyright (C) 2004 The ScummVM project
 *
 * The ReInherit Engine is (C)2000-2003 by Daniel Balsom.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 *
 */

#include "saga/saga.h"
#include "saga/gfx.h"

#include "saga/game_mod.h"
#include "saga/console.h"
#include "saga/rscfile_mod.h"
#include "saga/script.h"
#include "saga/sndres.h"
#include "saga/sprite.h"
#include "saga/font.h"
#include "saga/text.h"
#include "saga/sound.h"
#include "saga/scene.h"
#include "saga/actionmap.h"

#include "saga/actor.h"
#include "saga/actordata.h"
#include "saga/stream.h"

namespace Saga {

static int actorCompare(const ACTOR& actor1, const ACTOR& actor2) {
	if (actor1.a_pt.y == actor2.a_pt.y) {
		return 0;
	} else if (actor1.a_pt.y < actor2.a_pt.y) {
		return -1;
	} else {
		return 1;
	}
}
static ActorList::iterator zeroActorIterator;

ACTIONTIMES ActionTDeltas[] = {
	{ ACTION_IDLE, 80 },
	{ ACTION_WALK, 80 },
	{ ACTION_SPEAK, 200 }
};

Actor::Actor(SagaEngine *vm) : _vm(vm), _initialized(false) {
	int i;

	// Get actor resource file context
	_actorContext = GAME_GetFileContext(GAME_RESOURCEFILE, 0);
	if (_actorContext == NULL) {
		error("Actor::Actor(): Couldn't load actor module resource context.");
	}


	// Initialize alias table so each index contains itself
	for (i = 0; i < ACTORCOUNT; i++) {
		_aliasTbl[i] = i;
	}

	_count = 0;
	_initialized = true;
}

Actor::~Actor() {
	if (!_initialized) {
		return;
	}
}

int Actor::direct(int msec) {
	ActorList::iterator actorIterator;
	ACTOR *actor;

	ActorIntentList::iterator actorIntentIterator;
	ACTORINTENT *a_intent;

	int o_idx;
	int action_tdelta;

	// Walk down the actor list and direct each actor
	for (actorIterator = _list.begin(); actorIterator != _list.end(); ++actorIterator) {
		actor =  actorIterator.operator->();

		// Process the actor intent list
		actorIntentIterator = actor->a_intentlist.begin();
		if (actorIntentIterator != actor->a_intentlist.end()) {
			a_intent = actorIntentIterator.operator->();
			switch (a_intent->a_itype) {
			case INTENT_NONE:
				// Actor doesn't really feel like doing anything at all
				break;
			case INTENT_PATH:
				// Actor intends to go somewhere. Well good for him
				{
				 WALKINTENT *a_walkint;
					a_walkint = (WALKINTENT *)a_intent->a_data;
					handleWalkIntent(actor, a_walkint, &a_intent->a_idone, msec);
				}
				break;
			case INTENT_SPEAK:
				// Actor wants to blab
				{
				 SPEAKINTENT *a_speakint;
					a_speakint = (SPEAKINTENT *)a_intent->a_data;
					handleSpeakIntent(actor, a_speakint, &a_intent->a_idone, msec);
				}
				break;

			default:
				break;
			}

			// If this actor intent was flagged as completed, remove it.
			if (a_intent->a_idone) {
				a_intent->deleteData();
				actor->a_intentlist.erase(actorIntentIterator);
				actor->action = actor->def_action;
				actor->action_flags = actor->def_action_flags;
				actor->action_frame = 0;
				actor->action_time = 0;
			}
		} else {
			// Actor has no intent, idle?
		}

		// Process actor actions
		actor->action_time += msec;

		if (actor->action >= ACTION_COUNT) {
			action_tdelta = ACTOR_ACTIONTIME;
		} else {
			action_tdelta = ActionTDeltas[actor->action].time;
		}

		if (actor->action_time >= action_tdelta) {
			actor->action_time -= action_tdelta;
			actor->action_frame++;

			o_idx = ActorOrientationLUT[actor->orient];
			if (actor->act_tbl[actor->action].dir[o_idx].frame_count <= actor->action_frame) {
				if (actor->action_flags & ACTION_LOOP) {
					actor->action_frame = 0;
				} else {
					actor->action_frame--;
				}
			}
		}
	}

	return SUCCESS;
}

int Actor::drawList() {
	ActorList::iterator actorIterator;
	ACTOR *actor;

	ActorIntentList::iterator actorIntentIterator;
	ACTORINTENT *a_intent;
	SPEAKINTENT *a_speakint;

	ActorDialogList::iterator actorDialogIterator;
	ACTORDIALOGUE *a_dialogue;

	int o_idx; //Orientation index
	int sprite_num;

	int diag_x, diag_y; // dialog coordinates

	SURFACE *back_buf;

	back_buf = _vm->_gfx->getBackBuffer();

	for (actorIterator = _list.begin(); actorIterator != _list.end(); ++actorIterator) {
		actor =  actorIterator.operator->();
		o_idx = ActorOrientationLUT[actor->orient];
		sprite_num = actor->act_tbl[actor->action].dir[o_idx].frame_index;
		sprite_num += actor->action_frame;
		_vm->_sprite->drawOccluded(back_buf, actor->sl_p, sprite_num, actor->s_pt.x, actor->s_pt.y);

		// If actor's current intent is to speak, oblige him by 
		// displaying his dialogue 
		actorIntentIterator = actor->a_intentlist.begin();
		if (actorIntentIterator != actor->a_intentlist.end()) {
			a_intent = actorIntentIterator.operator->();
			if (a_intent->a_itype == INTENT_SPEAK) {
				a_speakint = (SPEAKINTENT *)a_intent->a_data;
				actorDialogIterator = a_speakint->si_diaglist.begin();
				if (actorDialogIterator != a_speakint->si_diaglist.end()) {
					a_dialogue = actorDialogIterator.operator->();
					diag_x = actor->s_pt.x;
					diag_y = actor->s_pt.y;
					diag_y -= ACTOR_DIALOGUE_HEIGHT;
					_vm->textDraw(MEDIUM_FONT_ID, back_buf, a_dialogue->d_string, diag_x, diag_y, actor->a_dcolor, 0,
								FONT_OUTLINE | FONT_CENTERED);
				}
			}
		}
	}

	return SUCCESS;
}

// Called if the user wishes to skip a line of dialogue (spacebar in the 
// original game). Will find all actors currently talking and remove one
// dialogue entry if there is a current speak intent present.

int Actor::skipDialogue() {
	ActorList::iterator actorIterator;
	ACTOR *actor;

	ActorIntentList::iterator actorIntentIterator;
	ACTORINTENT *a_intent;
	SPEAKINTENT *a_speakint;

	ActorDialogList::iterator actorDialogIterator;
	ACTORDIALOGUE *a_dialogue;

	if (!_initialized) {
		return FAILURE;
	}

	for (actorIterator = _list.begin(); actorIterator != _list.end(); ++actorIterator) {
		actor =  actorIterator.operator->();
		// Check the actor's current intent for a speak intent
		actorIntentIterator = actor->a_intentlist.begin();
		if (actorIntentIterator != actor->a_intentlist.end()) {
			a_intent = actorIntentIterator.operator->();
			if (a_intent->a_itype == INTENT_SPEAK) {
				// Okay, found a speak intent. Remove one dialogue entry 
				// from it, releasing any semaphore */
				a_speakint = (SPEAKINTENT *)a_intent->a_data;
				actorDialogIterator = a_speakint->si_diaglist.begin();
				if (actorDialogIterator != a_speakint->si_diaglist.end()) {
					a_dialogue = actorDialogIterator.operator->();
					if (a_dialogue->d_sem != NULL) {
						_vm->_script->SThreadReleaseSem(a_dialogue->d_sem);
					}
					a_speakint->si_diaglist.erase(actorDialogIterator);
					// And stop any currently playing voices
					_vm->_sound->stopVoice();
				}
			}
		}
	}

	return SUCCESS;
}

int Actor::create(int actor_id, int x, int y) {
	ACTOR actor;

	if (actor_id == 1) {
		actor_id = 0;
	} else {
		actor_id = actor_id & ~0x2000;
	}

	actor.id = actor_id;
	actor.a_pt.x = x;
	actor.a_pt.y = y;

	if (addActor(&actor) != SUCCESS) {

		return FAILURE;
	}

	return SUCCESS;
}

int Actor::addActor(ACTOR * actor) {
	ActorList::iterator actorIterator;
	int last_frame;
	int i;

	if (!_initialized) {
		return FAILURE;
	}

	if ((actor->id < 0) || (actor->id >= ACTORCOUNT)) {
		return FAILURE;
	}

	if (_tbl[actor->id] != zeroActorIterator) {
		return FAILURE;
	}

	AtoS(&actor->s_pt, &actor->a_pt);

	i = actor->id;

	actor->sl_rn = ActorTable[i].spritelist_rn;
	actor->si_rn = ActorTable[i].spriteindex_rn;

	loadActorSpriteIndex(actor, actor->si_rn, &last_frame);

	if (_vm->_sprite->loadList(actor->sl_rn, &actor->sl_p) != SUCCESS) {
		return FAILURE;
	}

	if (last_frame >= _vm->_sprite->getListLen(actor->sl_p)) {
		debug(0, "Appending to sprite list %d.", actor->sl_rn);
		if (_vm->_sprite->appendList(actor->sl_rn + 1,
			actor->sl_p) != SUCCESS) {
			return FAILURE;
		}
	}

	actor->flags = ActorTable[i].flags;
	actor->a_dcolor = ActorTable[i].color;
	actor->orient = ACTOR_DEFAULT_ORIENT;
	actor->def_action = 0;
	actor->def_action_flags = 0;
	actor->action = 0;
	actor->action_flags = 0;
	actor->action_time = 0;
	actor->action_frame = 0;

	actorIterator = _list.pushBack(*actor, actorCompare);

	actor = actorIterator.operator->();

	_tbl[i] = actorIterator;
	_count++;

	return SUCCESS;
}

int Actor::getActorIndex(uint16 actor_id) {
	uint16 actorIdx;

	if (actor_id == 1) {
		actorIdx = 0;
	} else {
		actorIdx = actor_id & ~0x2000;
	}

	if (actorIdx >= ACTORCOUNT) {
		error("Wrong actorIdx=%i", actorIdx);
	}

	if (_tbl[actorIdx] == zeroActorIterator) {
		return -1;
	}

	return actorIdx;
}

int Actor::actorExists(uint16 actor_id) {
	uint16 actor_idx;

	if (actor_id == 1) {
		actor_idx = 0;
	} else {
		actor_idx = actor_id & ~0x2000;
	}

	if (_tbl[actor_idx] == zeroActorIterator) {
		return 0;
	}

	return 1;
}

int Actor::speak(int index, const char *d_string, uint16 d_voice_rn, SEMAPHORE *sem) {
	ActorList::iterator actorIterator;
	ACTOR *actor;
	ActorIntentList::iterator actorIntentIterator;
	ACTORINTENT *a_intent_p = NULL;
	SPEAKINTENT *a_speakint;
	ACTORINTENT a_intent;
	int use_existing_ai = 0;
	ACTORDIALOGUE a_dialogue;

	a_dialogue.d_string = d_string;
	a_dialogue.d_voice_rn = d_voice_rn;
	a_dialogue.d_time = getSpeechTime(d_string, d_voice_rn);
	a_dialogue.d_sem_held = 1;
	a_dialogue.d_sem = sem;

	actorIterator = _tbl[index];
	if (actorIterator == zeroActorIterator) { 
		return FAILURE;
	}

	actor = actorIterator.operator->();

	// If actor's last registered intent is to speak, we can queue the
	// requested dialogue on that intent context; so examine the last
	// intent

	actorIntentIterator = actor->a_intentlist.end();
	--actorIntentIterator;
	if (actorIntentIterator != actor->a_intentlist.end()) {
		a_intent_p = actorIntentIterator.operator->();
		if (a_intent_p->a_itype == INTENT_SPEAK) {
			use_existing_ai = 1;
		}
	}

	if (use_existing_ai) {
		// Store the current dialogue off the existing actor intent
		a_speakint = (SPEAKINTENT *)a_intent_p->a_data;
		a_speakint->si_diaglist.push_back(a_dialogue);
	} else {
		// Create a new actor intent
		a_intent.a_itype = INTENT_SPEAK;
		a_intent.a_idone = 0;
		a_intent.a_iflags = 0;
		a_intent.createData();

		a_speakint = (SPEAKINTENT *)a_intent.a_data;
		a_speakint->si_last_action = actor->action;
		a_speakint->si_diaglist.push_back(a_dialogue);

		actor->a_intentlist.push_back(a_intent);
	}

	if (sem != NULL) {
		_vm->_script->SThreadHoldSem(sem);
	}

	return SUCCESS;
}

int Actor::handleSpeakIntent(ACTOR *actor, SPEAKINTENT *a_speakint, int *complete_p, int msec) {
	ActorDialogList::iterator actorDialogIterator;
	ActorDialogList::iterator nextActorDialogIterator;
	ACTORDIALOGUE *a_dialogue;
	ACTORDIALOGUE *a_dialogue2;
	long carry_time;
	int intent_complete = 0;

	if (!a_speakint->si_init) {
		// Initialize speak intent by setting up action
		actor->action = ACTION_SPEAK;
		actor->action_frame = 0;
		actor->action_time = 0;
		actor->action_flags = ACTION_LOOP;
		a_speakint->si_init = 1;
	}

	// Process actor dialogue list
	actorDialogIterator = a_speakint->si_diaglist.begin();
	if (actorDialogIterator != a_speakint->si_diaglist.end()) {
		a_dialogue = actorDialogIterator.operator->();
		if (!a_dialogue->d_playing) {
			// Dialogue voice hasn't played yet - play it now
			_vm->_sndRes->playVoice(a_dialogue->d_voice_rn);
			a_dialogue->d_playing = 1;
		}

		a_dialogue->d_time -= msec;
		if (a_dialogue->d_time <= 0) {
			// Dialogue time has expired; carry negative time to next
			// dialogue entry if present, release any semaphores and
			// delete the expired entry

			//actor->action = ACTION_IDLE;

			if (a_dialogue->d_sem != NULL) {
				_vm->_script->SThreadReleaseSem(a_dialogue->d_sem);
			}

			carry_time = a_dialogue->d_time;

			nextActorDialogIterator = actorDialogIterator;
			++nextActorDialogIterator;
			if (nextActorDialogIterator != a_speakint->si_diaglist.end()) {
				a_dialogue2 = nextActorDialogIterator.operator->();
				a_dialogue2->d_time -= carry_time;
			}

			// Check if there are any dialogue nodes left. If not, 
			// flag this speech intent as complete

			actorDialogIterator = a_speakint->si_diaglist.erase(actorDialogIterator);
			if (actorDialogIterator != a_speakint->si_diaglist.end()) {
				intent_complete = 1;
			}
		}
	} else {
		intent_complete = 1;
	}

	if (intent_complete) {
		*complete_p = 1;
	}

	return SUCCESS;
}

int Actor::getSpeechTime(const char *d_string, uint16 d_voice_rn) {
	int voice_len;

	voice_len = _vm->_sndRes->getVoiceLength(d_voice_rn);

	if (voice_len < 0) {
		voice_len = strlen(d_string) * ACTOR_DIALOGUE_LETTERTIME;
	}

	return voice_len;
}

int Actor::setOrientation(int index, int orient) {
	ACTOR *actor;

	actor = lookupActor(index);
	if (actor == NULL) {
		return FAILURE;
	}

	if ((orient < 0) || (orient > 7)) {
		return FAILURE;
	}

	actor->orient = orient;

	return SUCCESS;
}

int Actor::setAction(int index, int action_n, uint16 action_flags) {
	ACTOR *actor;

	actor = lookupActor(index);
	if (actor == NULL) {
		return FAILURE;
	}

	if ((action_n < 0) || (action_n >= actor->action_ct)) {
		return FAILURE;
	}

	actor->action = action_n;
	actor->action_flags = action_flags;
	actor->action_frame = 0;
	actor->action_time = 0;

	return SUCCESS;
}

int Actor::setDefaultAction(int index, int action_n, uint16 action_flags) {
	ACTOR *actor;

	actor = lookupActor(index);
	if (actor == NULL) {
		return FAILURE;
	}

	if ((action_n < 0) || (action_n >= actor->action_ct)) {
		return FAILURE;
	}

	actor->def_action = action_n;
	actor->def_action_flags = action_flags;

	return SUCCESS;
}

ACTOR *Actor::lookupActor(int index) {
	ActorList::iterator actorIterator;
	ACTOR *actor;

	if (!_initialized) {
		return NULL;
	}

	if ((index < 0) || (index >= ACTORCOUNT)) {
		return NULL;
	}

	if (_tbl[index] == zeroActorIterator) {
		return NULL;
	}

	actorIterator = _tbl[index];
	actor = actorIterator.operator->();

	return actor;
}

int Actor::loadActorSpriteIndex(ACTOR * actor, int si_rn, int *last_frame_p) {
	byte *res_p;
	size_t res_len;
	int s_action_ct;
	ACTORACTION *action_p;
	int last_frame;
	int i, orient;
	int result;

	result = RSC_LoadResource(_actorContext, si_rn, &res_p, &res_len);
	if (result != SUCCESS) {
		warning("Couldn't load sprite action index resource");
		return FAILURE;
	}

	s_action_ct = res_len / 16;
	debug(0, "Sprite resource contains %d sprite actions.", s_action_ct);
	action_p = (ACTORACTION *)malloc(sizeof(ACTORACTION) * s_action_ct);

	MemoryReadStreamEndian readS(res_p, res_len, IS_BIG_ENDIAN);

	if (action_p == NULL) {
		warning("Couldn't allocate memory for sprite actions");
		RSC_FreeResource(res_p);
		return MEM;
	}

	last_frame = 0;

	for (i = 0; i < s_action_ct; i++) {
		for (orient = 0; orient < 4; orient++) {
			// Load all four orientations
			action_p[i].dir[orient].frame_index = readS.readUint16();
			action_p[i].dir[orient].frame_count = readS.readUint16();
			if (action_p[i].dir[orient].frame_index > last_frame) {
				last_frame = action_p[i].dir[orient].frame_index;
			}
		}
	}

	actor->act_tbl = action_p;
	actor->action_ct = s_action_ct;

	RSC_FreeResource(res_p);

	if (last_frame_p != NULL) {
		*last_frame_p = last_frame;
	}

	return SUCCESS;
}

int Actor::deleteActor(int index) {
	ActorList::iterator actorIterator;
	ACTOR *actor;

	if (!_initialized) {
		return FAILURE;
	}

	if ((index < 0) || (index >= ACTORCOUNT)) {
		return FAILURE;
	}

	actorIterator = _tbl[index];

	if (actorIterator == zeroActorIterator) {
		return FAILURE;
	}

	actor = actorIterator.operator->();

	_vm->_sprite->freeSprite(actor->sl_p);

	_list.erase(actorIterator);

	_tbl[index] = zeroActorIterator;

	return SUCCESS;
}

int Actor::walkTo(int id, const Point *walk_pt, uint16 flags, SEMAPHORE *sem) {
	ACTORINTENT actor_intent;
	WALKINTENT *walk_intent;
	ActorList::iterator actorIterator;
	ACTOR *actor;

	assert(_initialized);
	assert(walk_pt != NULL);

	if ((id < 0) || (id >= ACTORCOUNT)) {
		return FAILURE;
	}

	if (_tbl[id] == zeroActorIterator) {
		return FAILURE;
	}

	actorIterator = _tbl[id];
	actor = actorIterator.operator->();

	actor_intent.a_itype = INTENT_PATH;
	actor_intent.a_iflags = 0;
	actor_intent.createData();
		
	walk_intent = (WALKINTENT*)actor_intent.a_data;

	walk_intent->wi_flags = flags;
	walk_intent->sem_held = 1;
	walk_intent->sem = sem;

	// handleWalkIntent() will create path on initialization
	walk_intent->wi_init = 0;
	walk_intent->dst_pt = *walk_pt;

	actor->a_intentlist.push_back(actor_intent);

	if (sem != NULL) {
		_vm->_script->SThreadHoldSem(sem);
	}

	return SUCCESS;
}

int Actor::setPathNode(WALKINTENT *walk_int, Point *src_pt, Point *dst_pt, SEMAPHORE *sem) {
	WALKNODE new_node;

	walk_int->wi_active = 1;
	walk_int->org = *src_pt;

	assert((walk_int != NULL) && (src_pt != NULL) && (dst_pt != NULL));

	new_node.node_pt = *dst_pt;
	new_node.calc_flag = 0;
	
	walk_int->nodelist.push_back(new_node);
	
	return SUCCESS;
}

int Actor::handleWalkIntent(ACTOR *actor, WALKINTENT *a_walkint, int *complete_p, int delta_time) {
	WalkNodeList::iterator walkNodeIterator;
	WalkNodeList::iterator nextWalkNodeIterator;

	WALKNODE *node_p;
	int dx;
	int dy;

	double path_a;
	double path_b;
	double path_slope;

	double path_x;
	double path_y;
	int path_time;

	double new_a_x;
	double new_a_y;

	int actor_x;
	int actor_y;

	char buf[100];

	// Initialize walk intent 
	if (!a_walkint->wi_init) {
		setPathNode(a_walkint, &actor->a_pt, &a_walkint->dst_pt, a_walkint->sem);
		setDefaultAction(actor->id, ACTION_IDLE, ACTION_NONE);
		a_walkint->wi_init = 1;
	}

	assert(a_walkint->wi_active);

	walkNodeIterator = a_walkint->nodelist.begin();
	nextWalkNodeIterator = walkNodeIterator;

	node_p = walkNodeIterator.operator->();

	if (node_p->calc_flag == 0) {

		debug(2, "Calculating new path vector to point (%d, %d)", node_p->node_pt.x, node_p->node_pt.y);

		dx = a_walkint->org.x - node_p->node_pt.x;
		dy = a_walkint->org.y - node_p->node_pt.y;

		if (dx == 0) {

			debug(0, "Vertical paths not implemented.");

			a_walkint->nodelist.erase(walkNodeIterator);
			a_walkint->wi_active = 0;

			// Release path semaphore
			if ((a_walkint->sem != NULL) && a_walkint->sem_held) {
				_vm->_script->SThreadReleaseSem(a_walkint->sem);
			}

			*complete_p = 1;
			return FAILURE;
		}

		a_walkint->slope = (float)dy / dx;

		if (dx > 0) {
			a_walkint->x_dir = -1;
			if (!(a_walkint->wi_flags & WALK_NOREORIENT)) {
				if (a_walkint->slope > 1.0) {
					actor->orient = ORIENT_N;
				} else if (a_walkint->slope < -1.0) {
					actor->orient = ORIENT_S;
				} else {
					actor->orient = ORIENT_W;
				}
			}
		} else {
			a_walkint->x_dir = 1;
			if (!(a_walkint->wi_flags & WALK_NOREORIENT)) {
				if (a_walkint->slope > 1.0) {
					actor->orient = ORIENT_S;
				} else if (a_walkint->slope < -1.0) {
					actor->orient = ORIENT_N;
				} else {
					actor->orient = ORIENT_E;
				}
			}
		}

		sprintf(buf, "%f", a_walkint->slope);

		debug(2, "Path slope: %s.", buf);

		actor->action = ACTION_WALK;
		actor->action_flags = ACTION_LOOP;
		a_walkint->time = 0;
		node_p->calc_flag = 1;
	}

	a_walkint->time += delta_time;
	path_time = a_walkint->time;

	path_a = ACTOR_BASE_SPEED * path_time;
	path_b = ACTOR_BASE_SPEED * path_time * ACTOR_BASE_ZMOD;
	path_slope = a_walkint->slope * a_walkint->x_dir;

	path_x = (path_a * path_b) / sqrt((path_a * path_a) * (path_slope * path_slope) + (path_b * path_b));

	path_y = path_slope * path_x;
	path_x = path_x * a_walkint->x_dir;

	new_a_x = path_x + a_walkint->org.x;
	new_a_y = path_y + a_walkint->org.y;

	if (((a_walkint->x_dir == 1) && new_a_x >= node_p->node_pt.x) ||
		((a_walkint->x_dir != 1) && (new_a_x <= node_p->node_pt.x))) {
		Point endpoint;
		int exitNum;

		debug(2, "Path complete.");
		a_walkint->nodelist.erase(walkNodeIterator);
		a_walkint->wi_active = 0;

		// Release path semaphore
		if (a_walkint->sem != NULL) {
			_vm->_script->SThreadReleaseSem(a_walkint->sem);
		}

		actor->action_frame = 0;
		actor->action = ACTION_IDLE;

		endpoint.x = (int)new_a_x / ACTOR_LMULT;
		endpoint.y = (int)new_a_y / ACTOR_LMULT;
		if ((exitNum = _vm->_scene->_actionMap->hitTest(endpoint)) != -1) {
			if (actor->flags & kProtagonist)
				_vm->_scene->changeScene(_vm->_scene->_actionMap->getExitScene(exitNum));
		}
		*complete_p = 1;
		return FAILURE;
	}

	actor_x = (int)new_a_x;
	actor_y = (int)new_a_y;

	actor->a_pt.x = (int)new_a_x;
	actor->a_pt.y = (int)new_a_y;

	actor->s_pt.x = actor->a_pt.x >> 2;
	actor->s_pt.y = actor->a_pt.y >> 2;

	ActorList::iterator actorIterator;
	if (_list.locate(actor, actorIterator)) {
		if (path_slope < 0) {
			_list.reorderUp(actorIterator, actorCompare);
		} else {
			_list.reorderDown(actorIterator, actorCompare);
		}
	} else {
		error("Actor::handleWalkIntent() actor not found list");		
	}

	return SUCCESS;
}

int Actor::move(int index, const Point *move_pt) {
	ActorList::iterator actorIterator;
	ACTOR *actor;

	int move_up = 0;

	actorIterator = _tbl[index];
	if (actorIterator == zeroActorIterator) {
		return FAILURE;
	}

	actor = actorIterator.operator->();

	if (move_pt->y < actor->a_pt.y) {
		move_up = 1;
	}

	actor->a_pt.x = move_pt->x;
	actor->a_pt.y = move_pt->y;

	AtoS(&actor->s_pt, &actor->a_pt);

	if (move_up) {
		_list.reorderUp(actorIterator, actorCompare);
	} else {
		_list.reorderDown(actorIterator, actorCompare);
	}

	return SUCCESS;
}

int Actor::moveRelative(int index, const Point *move_pt) {
	ActorList::iterator actorIterator;
	ACTOR *actor;

	actorIterator = _tbl[index];
	if (actorIterator == zeroActorIterator) {
		return FAILURE;
	}

	actor = actorIterator.operator->();

	actor->a_pt.x += move_pt->x;
	actor->a_pt.y += move_pt->y;

	AtoS(&actor->s_pt, &actor->a_pt);

	if (actor->a_pt.y < 0) {
		_list.reorderUp(actorIterator, actorCompare);
	} else {
		_list.reorderDown(actorIterator, actorCompare);
	}

	return SUCCESS;
}


int Actor::AtoS(Point *screen, const Point *actor) {
	screen->x = (actor->x / ACTOR_LMULT);
	screen->y = (actor->y / ACTOR_LMULT);

	return SUCCESS;
}

int Actor::StoA(Point *actor, const Point screen) {
	actor->x = (screen.x * ACTOR_LMULT);
	actor->y = (screen.y * ACTOR_LMULT);

	return SUCCESS;
}

void Actor::CF_actor_add(int argc, const char **argv) {
	ACTOR actor;

	actor.id = (uint16) atoi(argv[1]);

	actor.a_pt.x = atoi(argv[2]);
	actor.a_pt.y = atoi(argv[3]);

	addActor(&actor);
}

void Actor::CF_actor_del(int argc, const char **argv) {
	int id;
 
	id = atoi(argv[1]);

	deleteActor(id);
}

void Actor::CF_actor_move(int argc, const char **argv) {
	int id;
	Point move_pt;

	id = atoi(argv[1]);

	move_pt.x = atoi(argv[2]);
	move_pt.y = atoi(argv[3]);

	move(id, &move_pt);
}

void Actor::CF_actor_moverel(int argc, const char **argv) {
	int id;
	Point move_pt;

	id = atoi(argv[1]);

	move_pt.x = atoi(argv[2]);
	move_pt.y = atoi(argv[3]);

	moveRelative(id, &move_pt);
}

void Actor::CF_actor_seto(int argc, const char **argv) {
	int id;
	int orient;

	id = atoi(argv[1]);
	orient = atoi(argv[2]);

	setOrientation(id, orient);
}

void Actor::CF_actor_setact(int argc, const char **argv) {
	int index = 0;
	int action_n = 0;

	ACTOR *actor;

	index = atoi(argv[1]);
	action_n = atoi(argv[2]);

	actor = lookupActor(index);
	if (actor == NULL) {
		_vm->_console->DebugPrintf("Invalid actor index.\n");
		return;
	}

	if ((action_n < 0) || (action_n >= actor->action_ct)) {
		_vm->_console->DebugPrintf("Invalid action number.\n");
		return;
	}

	_vm->_console->DebugPrintf("Action frame counts: %d %d %d %d.\n",
			actor->act_tbl[action_n].dir[0].frame_count,
			actor->act_tbl[action_n].dir[1].frame_count,
			actor->act_tbl[action_n].dir[2].frame_count,
			actor->act_tbl[action_n].dir[3].frame_count);

	setAction(index, action_n, ACTION_LOOP);
}

} // End of namespace Saga
