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
 * This file is based on WME Lite.
 * http://dead-code.org/redir.php?target=wmelite
 * Copyright (c) 2011 Jan Nedoma
 */

#include "engines/wintermute/dcgf.h"
#include "engines/wintermute/Base/BGame.h"
#include "engines/wintermute/Base/BFader.h"
#include "engines/wintermute/Base/BFileManager.h"
#include "engines/wintermute/Base/BFont.h"
#include "engines/wintermute/Base/BFontStorage.h"
#include "engines/wintermute/Base/BImage.h"
#include "engines/wintermute/Base/BKeyboardState.h"
#include "engines/wintermute/Base/BParser.h"
#include "engines/wintermute/Base/BQuickMsg.h"
#include "engines/wintermute/Base/BRegistry.h"
#include "engines/wintermute/Base/BRenderSDL.h"
#include "engines/wintermute/Base/BSound.h"
#include "engines/wintermute/Base/BSoundMgr.h"
#include "engines/wintermute/Base/BSprite.h"
#include "engines/wintermute/Base/BSubFrame.h"
#include "engines/wintermute/Base/BSurfaceSDL.h"
#include "engines/wintermute/Base/BTransitionMgr.h"
#include "engines/wintermute/Base/BViewport.h"
#include "engines/wintermute/Base/BStringTable.h"
#include "engines/wintermute/Base/BRegion.h"
#include "engines/wintermute/Base/BSaveThumbHelper.h"
#include "engines/wintermute/Base/BSurfaceStorage.h"
#include "engines/wintermute/utils/crc.h"
#include "engines/wintermute/utils/PathUtil.h"
#include "engines/wintermute/utils/StringUtil.h"
#include "engines/wintermute/UI/UIWindow.h"
#include "engines/wintermute/Base/scriptables/ScValue.h"
#include "engines/wintermute/Base/scriptables/ScEngine.h"
#include "engines/wintermute/Base/scriptables/ScStack.h"
#include "engines/wintermute/Base/scriptables/ScScript.h"
#include "engines/wintermute/Base/scriptables/SXMath.h"
#include "engines/wintermute/Base/scriptables/SXStore.h"
#include "engines/wintermute/video/VidPlayer.h"
#include "engines/wintermute/video/VidTheoraPlayer.h"
#include "engines/wintermute/wintermute.h"
#include "common/savefile.h"
#include "common/textconsole.h"
#include "common/util.h"
#include "common/keyboard.h"
#include "common/system.h"
#include "common/file.h"

#ifdef __IPHONEOS__
#   include "ios_utils.h"
#endif

namespace WinterMute {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_PERSISTENT(CBGame, true)


//////////////////////////////////////////////////////////////////////
CBGame::CBGame(): CBObject(this) {
	_shuttingDown = false;

	_state = GAME_RUNNING;
	_origState = GAME_RUNNING;
	_freezeLevel = 0;

	_interactive = true;
	_origInteractive = false;

	_surfaceStorage = NULL;
	_fontStorage = NULL;
	_renderer = NULL;
	_soundMgr = NULL;
	_fileManager = NULL;
	_transMgr = NULL;
	_debugMgr = NULL;
	_scEngine = NULL;
	_keyboardState = NULL;

	_mathClass = NULL;

	_dEBUG_LogFile = NULL;
	_dEBUG_DebugMode = false;
	_dEBUG_AbsolutePathWarning = true;
	_dEBUG_ShowFPS = false;

	_systemFont = NULL;
	_videoFont = NULL;

	_videoPlayer = NULL;
	_theoraPlayer = NULL;

	_mainObject = NULL;
	_activeObject = NULL;

	_fader = NULL;

	_offsetX = _offsetY = 0;
	_offsetPercentX = _offsetPercentY = 0.0f;

	_subtitles = true;
	_videoSubtitles = true;

	_timer = 0;
	_timerDelta = 0;
	_timerLast = 0;

	_liveTimer = 0;
	_liveTimerDelta = 0;
	_liveTimerLast = 0;

	_sequence = 0;

	_mousePos.x = _mousePos.y = 0;
	_mouseLeftDown = _mouseRightDown = _mouseMidlleDown = false;
	_capturedObject = NULL;

	// FPS counters
	_lastTime = _fpsTime = _deltaTime = _framesRendered = _fps = 0;

	_cursorNoninteractive = NULL;

	_useD3D = false;

	_registry = new CBRegistry(this);
	_stringTable = new CBStringTable(this);

	for (int i = 0; i < NUM_MUSIC_CHANNELS; i++) {
		_music[i] = NULL;
		_musicStartTime[i] = 0;
	}

	_settingsResWidth = 800;
	_settingsResHeight = 600;
	_settingsRequireAcceleration = false;
	_settingsRequireSound = false;
	_settingsTLMode = 0;
	_settingsAllowWindowed = true;
	_settingsGameFile = NULL;
	_settingsAllowAdvanced = false;
	_settingsAllowAccessTab = true;
	_settingsAllowAboutTab = true;
	_settingsAllowDesktopRes = false;

	_editorForceScripts = false;
	_editorAlwaysRegister = false;

	_focusedWindow = NULL;

	_loadInProgress = false;

	_quitting = false;
	_loading = false;
	_scheduledLoadSlot = -1;

	_personalizedSave = false;
	_compressedSavegames = true;

	_editorMode = false;
	_doNotExpandStrings = false;

	_engineLogCallback = NULL;
	_engineLogCallbackData = NULL;

	_smartCache = false;
	_surfaceGCCycleTime = 10000;

	_reportTextureFormat = false;

	_viewportSP = -1;

	_subtitlesSpeed = 70;

	_resourceModule = 0;

	_forceNonStreamedSounds = false;

	_thumbnailWidth = _thumbnailHeight = 0;

	_indicatorDisplay = false;
	_indicatorColor = DRGBA(255, 0, 0, 128);
	_indicatorProgress = 0;
	_indicatorX = -1;
	_indicatorY = -1;
	_indicatorWidth = -1;
	_indicatorHeight = 8;
	_richSavedGames = false;
	_savedGameExt = NULL;
	CBUtils::SetString(&_savedGameExt, "dsv");

	_musicCrossfadeRunning = false;
	_musicCrossfadeStartTime = 0;
	_musicCrossfadeLength = 0;
	_musicCrossfadeChannel1 = -1;
	_musicCrossfadeChannel2 = -1;
	_musicCrossfadeSwap = false;

	_loadImageName = NULL;
	_saveImageName = NULL;
	_saveLoadImage = NULL;

	_saveImageX = _saveImageY = 0;
	_loadImageX = _loadImageY = 0;

	_localSaveDir = NULL;
	CBUtils::SetString(&_localSaveDir, "saves");
	_saveDirChecked = false;

	_loadingIcon = NULL;
	_loadingIconX = _loadingIconY = 0;
	_loadingIconPersistent = false;

	_textEncoding = TEXT_ANSI;
	_textRTL = false;

	_soundBufferSizeSec = 3;
	_suspendedRendering = false;

	_lastCursor = NULL;


	CBPlatform::SetRectEmpty(&_mouseLockRect);

	_suppressScriptErrors = false;
	_lastMiniUpdate = 0;
	_miniUpdateEnabled = false;

	_cachedThumbnail = NULL;

	_autorunDisabled = false;

	// compatibility bits
	_compatKillMethodThreads = false;

	_usedMem = 0;


	_autoSaveOnExit = true;
	_autoSaveSlot = 999;
	_cursorHidden = false;

#ifdef __IPHONEOS__
	_touchInterface = true;
	_constrainedMemory = true; // TODO differentiate old and new iOS devices
#else
	_touchInterface = false;
	_constrainedMemory = false;
#endif

	_store = NULL;
}


//////////////////////////////////////////////////////////////////////
CBGame::~CBGame() {
	_shuttingDown = true;

	LOG(0, "");
	LOG(0, "Shutting down...");

	GetDebugMgr()->OnGameShutdown();

	_registry->WriteBool("System", "LastRun", true);

	cleanup();

	delete[] _localSaveDir;
	delete[] _settingsGameFile;
	delete[] _savedGameExt;

	delete _cachedThumbnail;

	delete _saveLoadImage;
	delete _mathClass;

	delete _transMgr;
	delete _scEngine;
	delete _fontStorage;
	delete _surfaceStorage;
	delete _videoPlayer;
	delete _theoraPlayer;
	delete _soundMgr;
	delete _debugMgr;
	//SAFE_DELETE(_keyboardState);

	delete _renderer;
	delete _fileManager;
	delete _registry;
	delete _stringTable;

	_localSaveDir = NULL;
	_settingsGameFile = NULL;
	_savedGameExt = NULL;

	_cachedThumbnail = NULL;

	_saveLoadImage = NULL;
	_mathClass = NULL;

	_transMgr = NULL;
	_scEngine = NULL;
	_fontStorage = NULL;
	_surfaceStorage = NULL;
	_videoPlayer = NULL;
	_theoraPlayer = NULL;
	_soundMgr = NULL;
	_debugMgr = NULL;

	_renderer = NULL;
	_fileManager = NULL;
	_registry = NULL;
	_stringTable = NULL;

	DEBUG_DebugDisable();
	CBPlatform::OutputDebugString("--- shutting down normally ---\n");
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::cleanup() {
	delete _loadingIcon;
	_loadingIcon = NULL;

	_engineLogCallback = NULL;
	_engineLogCallbackData = NULL;

	for (int i = 0; i < NUM_MUSIC_CHANNELS; i++) {
		delete _music[i];
		_music[i] = NULL;
		_musicStartTime[i] = 0;
	}

	UnregisterObject(_store);
	_store = NULL;

	UnregisterObject(_fader);
	_fader = NULL;

	for (int i = 0; i < _regObjects.GetSize(); i++) {
		delete _regObjects[i];
		_regObjects[i] = NULL;
	}
	_regObjects.RemoveAll();

	_windows.RemoveAll(); // refs only
	_focusedWindow = NULL; // ref only

	delete[] _saveImageName;
	delete[] _loadImageName;
	_saveImageName = NULL;
	_loadImageName = NULL;

	delete _cursorNoninteractive;
	delete _cursor;
	delete _activeCursor;
	_cursorNoninteractive = NULL;
	_cursor = NULL;
	_activeCursor = NULL;

	delete _scValue;
	delete _sFX;
	_scValue = NULL;
	_sFX = NULL;

	for (int i = 0; i < _scripts.GetSize(); i++) {
		_scripts[i]->_owner = NULL;
		_scripts[i]->finish();
	}
	_scripts.RemoveAll();

	_fontStorage->RemoveFont(_systemFont);
	_systemFont = NULL;

	_fontStorage->RemoveFont(_videoFont);
	_videoFont = NULL;

	for (int i = 0; i < _quickMessages.GetSize(); i++) delete _quickMessages[i];
	_quickMessages.RemoveAll();

	_viewportStack.RemoveAll();
	_viewportSP = -1;

	delete[] _name;
	delete[] _filename;
	_name = NULL;
	_filename = NULL;
	for (int i = 0; i < 7; i++) {
		delete[] _caption[i];
		_caption[i] = NULL;
	}

	_lastCursor = NULL;

	delete _keyboardState;
	_keyboardState = NULL;

	return S_OK;
}


//////////////////////////////////////////////////////////////////////
HRESULT CBGame::Initialize1() {
	_surfaceStorage = new CBSurfaceStorage(this);
	if (_surfaceStorage == NULL) goto init_fail;

	_fontStorage = new CBFontStorage(this);
	if (_fontStorage == NULL) goto init_fail;

	_fileManager = new CBFileManager(this);
	if (_fileManager == NULL) goto init_fail;

	_soundMgr = new CBSoundMgr(this);
	if (_soundMgr == NULL) goto init_fail;

	_debugMgr = new CBDebugger(this);
	if (_debugMgr == NULL) goto init_fail;

	_mathClass = new CSXMath(this);
	if (_mathClass == NULL) goto init_fail;

	_scEngine = new CScEngine(this);
	if (_scEngine == NULL) goto init_fail;

	_videoPlayer = new CVidPlayer(this);
	if (_videoPlayer == NULL) goto init_fail;

	_transMgr = new CBTransitionMgr(this);
	if (_transMgr == NULL) goto init_fail;

	_keyboardState = new CBKeyboardState(this);
	if (_keyboardState == NULL) goto init_fail;

	_fader = new CBFader(this);
	if (_fader == NULL) goto init_fail;
	RegisterObject(_fader);

	_store = new CSXStore(this);
	if (_store == NULL) goto init_fail;
	RegisterObject(_store);

	return S_OK;

init_fail:
	if (_mathClass) delete _mathClass;
	if (_store) delete _store;
	if (_keyboardState) delete _keyboardState;
	if (_transMgr) delete _transMgr;
	if (_debugMgr) delete _debugMgr;
	if (_surfaceStorage) delete _surfaceStorage;
	if (_fontStorage) delete _fontStorage;
	if (_soundMgr) delete _soundMgr;
	if (_fileManager) delete _fileManager;
	if (_scEngine) delete _scEngine;
	if (_videoPlayer) delete _videoPlayer;
	return E_FAIL;
}


//////////////////////////////////////////////////////////////////////
HRESULT CBGame::Initialize2() { // we know whether we are going to be accelerated
	_renderer = new CBRenderSDL(this);
	if (_renderer == NULL) goto init_fail;

	return S_OK;

init_fail:
	if (_renderer) delete _renderer;
	return E_FAIL;
}


//////////////////////////////////////////////////////////////////////
HRESULT CBGame::Initialize3() { // renderer is initialized
	_posX = _renderer->_width / 2;
	_posY = _renderer->_height / 2;

	if (_indicatorY == -1) _indicatorY = _renderer->_height - _indicatorHeight;
	if (_indicatorX == -1) _indicatorX = 0;
	if (_indicatorWidth == -1) _indicatorWidth = _renderer->_width;

	return S_OK;
}


//////////////////////////////////////////////////////////////////////
void CBGame::DEBUG_DebugEnable(const char *Filename) {
	_dEBUG_DebugMode = true;

#ifndef __IPHONEOS__
	//if (Filename)_dEBUG_LogFile = fopen(Filename, "a+");
	//else _dEBUG_LogFile = fopen("./zz_debug.log", "a+");

	if (!_dEBUG_LogFile) {
		AnsiString safeLogFileName = PathUtil::GetSafeLogFileName();
		//_dEBUG_LogFile = fopen(safeLogFileName.c_str(), "a+");
	}

	//if (_dEBUG_LogFile != NULL) fprintf((FILE *)_dEBUG_LogFile, "\n");
	warning("BGame::DEBUG_DebugEnable - No logfile is currently created"); //TODO: Use a dumpfile?
#endif

	/*  time_t timeNow;
	    time(&timeNow);
	    struct tm *tm = localtime(&timeNow);

	#ifdef _DEBUG
	    LOG(0, "********** DEBUG LOG OPENED %02d-%02d-%04d (Debug Build) *******************", tm->tm_mday, tm->tm_mon, tm->tm_year + 1900);
	#else
	    LOG(0, "********** DEBUG LOG OPENED %02d-%02d-%04d (Release Build) *****************", tm->tm_mday, tm->tm_mon, tm->tm_year + 1900);
	#endif*/
	int secs = g_system->getMillis() / 1000;
	int hours = secs / 3600;
	secs = secs % 3600;
	int mins = secs / 60;
	secs = secs % 60;

#ifdef _DEBUG
	LOG(0, "********** DEBUG LOG OPENED %02d-%02d-%02d (Debug Build) *******************", hours, mins, secs);
#else
	LOG(0, "********** DEBUG LOG OPENED %02d-%02d-%02d (Release Build) *****************", hours, mins, secs);
#endif

	LOG(0, "%s ver %d.%d.%d%s, Compiled on " __DATE__ ", " __TIME__, DCGF_NAME, DCGF_VER_MAJOR, DCGF_VER_MINOR, DCGF_VER_BUILD, DCGF_VER_SUFFIX);
	//LOG(0, "Extensions: %s ver %d.%02d", EXT_NAME, EXT_VER_MAJOR, EXT_VER_MINOR);

	AnsiString platform = CBPlatform::GetPlatformName();
	LOG(0, "Platform: %s", platform.c_str());
	LOG(0, "");
}


//////////////////////////////////////////////////////////////////////
void CBGame::DEBUG_DebugDisable() {
	if (_dEBUG_LogFile != NULL) {
		LOG(0, "********** DEBUG LOG CLOSED ********************************************");
		//fclose((FILE *)_dEBUG_LogFile);
		_dEBUG_LogFile = NULL;
	}
	_dEBUG_DebugMode = false;
}


//////////////////////////////////////////////////////////////////////
void CBGame::LOG(HRESULT res, LPCSTR fmt, ...) {
	uint32 secs = g_system->getMillis() / 1000;
	uint32 hours = secs / 3600;
	secs = secs % 3600;
	uint32 mins = secs / 60;
	secs = secs % 60;

	char buff[512];
	va_list va;

	va_start(va, fmt);
	vsprintf(buff, fmt, va);
	va_end(va);

	// redirect to an engine's own callback
	if (_engineLogCallback) {
		_engineLogCallback(buff, res, _engineLogCallbackData);
	}
	if (_debugMgr) _debugMgr->OnLog(res, buff);

	debugCN(kWinterMuteDebugLog, "%02d:%02d:%02d: %s\n", hours, mins, secs, buff);

	//fprintf((FILE *)_dEBUG_LogFile, "%02d:%02d:%02d: %s\n", hours, mins, secs, buff);
	//fflush((FILE *)_dEBUG_LogFile);

	//QuickMessage(buff);
}


//////////////////////////////////////////////////////////////////////////
void CBGame::SetEngineLogCallback(ENGINE_LOG_CALLBACK Callback, void *Data) {
	_engineLogCallback = Callback;
	_engineLogCallbackData = Data;
}


//////////////////////////////////////////////////////////////////////
HRESULT CBGame::InitLoop() {
	_viewportSP = -1;

	_currentTime = CBPlatform::GetTime();

	GetDebugMgr()->OnGameTick();
	_renderer->initLoop();
	_soundMgr->initLoop();
	UpdateMusicCrossfade();

	_surfaceStorage->initLoop();
	_fontStorage->InitLoop();


	//_activeObject = NULL;

	// count FPS
	_deltaTime = _currentTime - _lastTime;
	_lastTime  = _currentTime;
	_fpsTime += _deltaTime;

	_liveTimerDelta = _liveTimer - _liveTimerLast;
	_liveTimerLast = _liveTimer;
	_liveTimer += MIN((uint32)1000, _deltaTime);

	if (_state != GAME_FROZEN) {
		_timerDelta = _timer - _timerLast;
		_timerLast = _timer;
		_timer += MIN((uint32)1000, _deltaTime);
	} else _timerDelta = 0;

	_framesRendered++;
	if (_fpsTime > 1000) {
		_fps = _framesRendered;
		_framesRendered  = 0;
		_fpsTime = 0;
	}
	//Game->LOG(0, "%d", _fps);

	GetMousePos(&_mousePos);

	_focusedWindow = NULL;
	for (int i = _windows.GetSize() - 1; i >= 0; i--) {
		if (_windows[i]->_visible) {
			_focusedWindow = _windows[i];
			break;
		}
	}

	updateSounds();

	if (_fader) _fader->update();

	return S_OK;
}


//////////////////////////////////////////////////////////////////////
HRESULT CBGame::InitInput(HINSTANCE hInst, HWND hWnd) {
	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
int CBGame::GetSequence() {
	return ++_sequence;
}


//////////////////////////////////////////////////////////////////////////
void CBGame::SetOffset(int OffsetX, int OffsetY) {
	_offsetX = OffsetX;
	_offsetY = OffsetY;
}

//////////////////////////////////////////////////////////////////////////
void CBGame::GetOffset(int *OffsetX, int *OffsetY) {
	if (OffsetX != NULL) *OffsetX = _offsetX;
	if (OffsetY != NULL) *OffsetY = _offsetY;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::loadFile(const char *Filename) {
	byte *Buffer = Game->_fileManager->readWholeFile(Filename);
	if (Buffer == NULL) {
		Game->LOG(0, "CBGame::LoadFile failed for file '%s'", Filename);
		return E_FAIL;
	}

	HRESULT ret;

	_filename = new char [strlen(Filename) + 1];
	strcpy(_filename, Filename);

	if (FAILED(ret = loadBuffer(Buffer, true))) Game->LOG(0, "Error parsing GAME file '%s'", Filename);


	delete [] Buffer;

	return ret;
}


TOKEN_DEF_START
TOKEN_DEF(GAME)
TOKEN_DEF(TEMPLATE)
TOKEN_DEF(NAME)
TOKEN_DEF(SYSTEM_FONT)
TOKEN_DEF(VIDEO_FONT)
TOKEN_DEF(EVENTS)
TOKEN_DEF(CURSOR)
TOKEN_DEF(ACTIVE_CURSOR)
TOKEN_DEF(NONINTERACTIVE_CURSOR)
TOKEN_DEF(STRING_TABLE)
TOKEN_DEF(RESOLUTION)
TOKEN_DEF(SETTINGS)
TOKEN_DEF(REQUIRE_3D_ACCELERATION)
TOKEN_DEF(REQUIRE_SOUND)
TOKEN_DEF(HWTL_MODE)
TOKEN_DEF(ALLOW_WINDOWED_MODE)
TOKEN_DEF(ALLOW_ACCESSIBILITY_TAB)
TOKEN_DEF(ALLOW_ABOUT_TAB)
TOKEN_DEF(ALLOW_ADVANCED)
TOKEN_DEF(ALLOW_DESKTOP_RES)
TOKEN_DEF(REGISTRY_PATH)
TOKEN_DEF(PERSONAL_SAVEGAMES)
TOKEN_DEF(SCRIPT)
TOKEN_DEF(CAPTION)
TOKEN_DEF(PROPERTY)
TOKEN_DEF(SUBTITLES_SPEED)
TOKEN_DEF(SUBTITLES)
TOKEN_DEF(VIDEO_SUBTITLES)
TOKEN_DEF(EDITOR_PROPERTY)
TOKEN_DEF(THUMBNAIL_WIDTH)
TOKEN_DEF(THUMBNAIL_HEIGHT)
TOKEN_DEF(INDICATOR_X)
TOKEN_DEF(INDICATOR_Y)
TOKEN_DEF(INDICATOR_WIDTH)
TOKEN_DEF(INDICATOR_HEIGHT)
TOKEN_DEF(INDICATOR_COLOR)
TOKEN_DEF(SAVE_IMAGE_X)
TOKEN_DEF(SAVE_IMAGE_Y)
TOKEN_DEF(SAVE_IMAGE)
TOKEN_DEF(LOAD_IMAGE_X)
TOKEN_DEF(LOAD_IMAGE_Y)
TOKEN_DEF(LOAD_IMAGE)
TOKEN_DEF(LOCAL_SAVE_DIR)
TOKEN_DEF(RICH_SAVED_GAMES)
TOKEN_DEF(SAVED_GAME_EXT)
TOKEN_DEF(GUID)
TOKEN_DEF(COMPAT_KILL_METHOD_THREADS)
TOKEN_DEF_END
//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::loadBuffer(byte  *Buffer, bool Complete) {
	TOKEN_TABLE_START(commands)
	TOKEN_TABLE(GAME)
	TOKEN_TABLE(TEMPLATE)
	TOKEN_TABLE(NAME)
	TOKEN_TABLE(SYSTEM_FONT)
	TOKEN_TABLE(VIDEO_FONT)
	TOKEN_TABLE(EVENTS)
	TOKEN_TABLE(CURSOR)
	TOKEN_TABLE(ACTIVE_CURSOR)
	TOKEN_TABLE(NONINTERACTIVE_CURSOR)
	TOKEN_TABLE(PERSONAL_SAVEGAMES)
	TOKEN_TABLE(SCRIPT)
	TOKEN_TABLE(CAPTION)
	TOKEN_TABLE(PROPERTY)
	TOKEN_TABLE(SUBTITLES_SPEED)
	TOKEN_TABLE(SUBTITLES)
	TOKEN_TABLE(VIDEO_SUBTITLES)
	TOKEN_TABLE(EDITOR_PROPERTY)
	TOKEN_TABLE(THUMBNAIL_WIDTH)
	TOKEN_TABLE(THUMBNAIL_HEIGHT)
	TOKEN_TABLE(INDICATOR_X)
	TOKEN_TABLE(INDICATOR_Y)
	TOKEN_TABLE(INDICATOR_WIDTH)
	TOKEN_TABLE(INDICATOR_HEIGHT)
	TOKEN_TABLE(INDICATOR_COLOR)
	TOKEN_TABLE(SAVE_IMAGE_X)
	TOKEN_TABLE(SAVE_IMAGE_Y)
	TOKEN_TABLE(SAVE_IMAGE)
	TOKEN_TABLE(LOAD_IMAGE_X)
	TOKEN_TABLE(LOAD_IMAGE_Y)
	TOKEN_TABLE(LOAD_IMAGE)
	TOKEN_TABLE(LOCAL_SAVE_DIR)
	TOKEN_TABLE(COMPAT_KILL_METHOD_THREADS)
	TOKEN_TABLE_END

	byte *params;
	int cmd;
	CBParser parser(Game);

	if (Complete) {
		if (parser.GetCommand((char **)&Buffer, commands, (char **)&params) != TOKEN_GAME) {
			Game->LOG(0, "'GAME' keyword expected.");
			return E_FAIL;
		}
		Buffer = params;
	}

	while ((cmd = parser.GetCommand((char **)&Buffer, commands, (char **)&params)) > 0) {
		switch (cmd) {
		case TOKEN_TEMPLATE:
			if (FAILED(loadFile((char *)params))) cmd = PARSERR_GENERIC;
			break;

		case TOKEN_NAME:
			setName((char *)params);
			break;

		case TOKEN_CAPTION:
			setCaption((char *)params);
			break;

		case TOKEN_SYSTEM_FONT:
			if (_systemFont) _fontStorage->RemoveFont(_systemFont);
			_systemFont = NULL;

			_systemFont = Game->_fontStorage->AddFont((char *)params);
			break;

		case TOKEN_VIDEO_FONT:
			if (_videoFont) _fontStorage->RemoveFont(_videoFont);
			_videoFont = NULL;

			_videoFont = Game->_fontStorage->AddFont((char *)params);
			break;


		case TOKEN_CURSOR:
			delete _cursor;
			_cursor = new CBSprite(Game);
			if (!_cursor || FAILED(_cursor->loadFile((char *)params))) {
				delete _cursor;
				_cursor = NULL;
				cmd = PARSERR_GENERIC;
			}
			break;

		case TOKEN_ACTIVE_CURSOR:
			delete _activeCursor;
			_activeCursor = NULL;
			_activeCursor = new CBSprite(Game);
			if (!_activeCursor || FAILED(_activeCursor->loadFile((char *)params))) {
				delete _activeCursor;
				_activeCursor = NULL;
				cmd = PARSERR_GENERIC;
			}
			break;

		case TOKEN_NONINTERACTIVE_CURSOR:
			delete _cursorNoninteractive;
			_cursorNoninteractive = new CBSprite(Game);
			if (!_cursorNoninteractive || FAILED(_cursorNoninteractive->loadFile((char *)params))) {
				delete _cursorNoninteractive;
				_cursorNoninteractive = NULL;
				cmd = PARSERR_GENERIC;
			}
			break;

		case TOKEN_SCRIPT:
			addScript((char *)params);
			break;

		case TOKEN_PERSONAL_SAVEGAMES:
			parser.ScanStr((char *)params, "%b", &_personalizedSave);
			break;

		case TOKEN_SUBTITLES:
			parser.ScanStr((char *)params, "%b", &_subtitles);
			break;

		case TOKEN_SUBTITLES_SPEED:
			parser.ScanStr((char *)params, "%d", &_subtitlesSpeed);
			break;

		case TOKEN_VIDEO_SUBTITLES:
			parser.ScanStr((char *)params, "%b", &_videoSubtitles);
			break;

		case TOKEN_PROPERTY:
			parseProperty(params, false);
			break;

		case TOKEN_EDITOR_PROPERTY:
			parseEditorProperty(params, false);
			break;

		case TOKEN_THUMBNAIL_WIDTH:
			parser.ScanStr((char *)params, "%d", &_thumbnailWidth);
			break;

		case TOKEN_THUMBNAIL_HEIGHT:
			parser.ScanStr((char *)params, "%d", &_thumbnailHeight);
			break;

		case TOKEN_INDICATOR_X:
			parser.ScanStr((char *)params, "%d", &_indicatorX);
			break;

		case TOKEN_INDICATOR_Y:
			parser.ScanStr((char *)params, "%d", &_indicatorY);
			break;

		case TOKEN_INDICATOR_COLOR: {
			int r, g, b, a;
			parser.ScanStr((char *)params, "%d,%d,%d,%d", &r, &g, &b, &a);
			_indicatorColor = DRGBA(r, g, b, a);
		}
		break;

		case TOKEN_INDICATOR_WIDTH:
			parser.ScanStr((char *)params, "%d", &_indicatorWidth);
			break;

		case TOKEN_INDICATOR_HEIGHT:
			parser.ScanStr((char *)params, "%d", &_indicatorHeight);
			break;

		case TOKEN_SAVE_IMAGE:
			CBUtils::SetString(&_saveImageName, (char *)params);
			break;

		case TOKEN_SAVE_IMAGE_X:
			parser.ScanStr((char *)params, "%d", &_saveImageX);
			break;

		case TOKEN_SAVE_IMAGE_Y:
			parser.ScanStr((char *)params, "%d", &_saveImageY);
			break;

		case TOKEN_LOAD_IMAGE:
			CBUtils::SetString(&_loadImageName, (char *)params);
			break;

		case TOKEN_LOAD_IMAGE_X:
			parser.ScanStr((char *)params, "%d", &_loadImageX);
			break;

		case TOKEN_LOAD_IMAGE_Y:
			parser.ScanStr((char *)params, "%d", &_loadImageY);
			break;

		case TOKEN_LOCAL_SAVE_DIR:
			CBUtils::SetString(&_localSaveDir, (char *)params);
			break;

		case TOKEN_COMPAT_KILL_METHOD_THREADS:
			parser.ScanStr((char *)params, "%b", &_compatKillMethodThreads);
			break;
		}
	}

	if (!_systemFont) _systemFont = Game->_fontStorage->AddFont("system_font.fnt");


	if (cmd == PARSERR_TOKENNOTFOUND) {
		Game->LOG(0, "Syntax error in GAME definition");
		return E_FAIL;
	}
	if (cmd == PARSERR_GENERIC) {
		Game->LOG(0, "Error loading GAME definition");
		return E_FAIL;
	}

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
// high level scripting interface
//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::scCallMethod(CScScript *script, CScStack *stack, CScStack *thisStack, const char *name) {
	//////////////////////////////////////////////////////////////////////////
	// LOG
	//////////////////////////////////////////////////////////////////////////
	if (strcmp(name, "LOG") == 0) {
		stack->CorrectParams(1);
		LOG(0, stack->Pop()->GetString());
		stack->PushNULL();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// Caption
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Caption") == 0) {
		HRESULT res = CBObject::scCallMethod(script, stack, thisStack, name);
		SetWindowTitle();
		return res;
	}

	//////////////////////////////////////////////////////////////////////////
	// Msg
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Msg") == 0) {
		stack->CorrectParams(1);
		QuickMessage(stack->Pop()->GetString());
		stack->PushNULL();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// RunScript
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "RunScript") == 0) {
		Game->LOG(0, "**Warning** The 'RunScript' method is now obsolete. Use 'AttachScript' instead (same syntax)");
		stack->CorrectParams(1);
		if (FAILED(addScript(stack->Pop()->GetString())))
			stack->PushBool(false);
		else
			stack->PushBool(true);

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// LoadStringTable
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "LoadStringTable") == 0) {
		stack->CorrectParams(2);
		const char *Filename = stack->Pop()->GetString();
		CScValue *Val = stack->Pop();

		bool ClearOld;
		if (Val->IsNULL()) ClearOld = true;
		else ClearOld = Val->GetBool();

		if (FAILED(_stringTable->loadFile(Filename, ClearOld)))
			stack->PushBool(false);
		else
			stack->PushBool(true);

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// ValidObject
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "ValidObject") == 0) {
		stack->CorrectParams(1);
		CBScriptable *obj = stack->Pop()->GetNative();
		if (ValidObject((CBObject *) obj)) stack->PushBool(true);
		else stack->PushBool(false);

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// Reset
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Reset") == 0) {
		stack->CorrectParams(0);
		ResetContent();
		stack->PushNULL();

		return S_OK;
	}


	//////////////////////////////////////////////////////////////////////////
	// UnloadObject
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "UnloadObject") == 0) {
		stack->CorrectParams(1);
		CScValue *val = stack->Pop();
		CBObject *obj = (CBObject *)val->GetNative();
		UnregisterObject(obj);
		if (val->GetType() == VAL_VARIABLE_REF) val->SetNULL();

		stack->PushNULL();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// LoadWindow
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "LoadWindow") == 0) {
		stack->CorrectParams(1);
		CUIWindow *win = new CUIWindow(Game);
		if (win && SUCCEEDED(win->loadFile(stack->Pop()->GetString()))) {
			_windows.Add(win);
			RegisterObject(win);
			stack->PushNative(win, true);
		} else {
			delete win;
			win = NULL;
			stack->PushNULL();
		}
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// ExpandString
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "ExpandString") == 0) {
		stack->CorrectParams(1);
		CScValue *val = stack->Pop();
		char *str = new char[strlen(val->GetString()) + 1];
		strcpy(str, val->GetString());
		_stringTable->Expand(&str);
		stack->PushString(str);
		delete [] str;
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// PlayMusic / PlayMusicChannel
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "PlayMusic") == 0 || strcmp(name, "PlayMusicChannel") == 0) {
		int Channel = 0;
		if (strcmp(name, "PlayMusic") == 0) stack->CorrectParams(3);
		else {
			stack->CorrectParams(4);
			Channel = stack->Pop()->GetInt();
		}

		const char *Filename = stack->Pop()->GetString();
		CScValue *ValLooping = stack->Pop();
		bool Looping = ValLooping->IsNULL() ? true : ValLooping->GetBool();

		CScValue *ValLoopStart = stack->Pop();
		uint32 LoopStart = (uint32)(ValLoopStart->IsNULL() ? 0 : ValLoopStart->GetInt());


		if (FAILED(PlayMusic(Channel, Filename, Looping, LoopStart))) stack->PushBool(false);
		else stack->PushBool(true);
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// StopMusic / StopMusicChannel
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "StopMusic") == 0 || strcmp(name, "StopMusicChannel") == 0) {
		int Channel = 0;

		if (strcmp(name, "StopMusic") == 0) stack->CorrectParams(0);
		else {
			stack->CorrectParams(1);
			Channel = stack->Pop()->GetInt();
		}

		if (FAILED(StopMusic(Channel))) stack->PushBool(false);
		else stack->PushBool(true);
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// PauseMusic / PauseMusicChannel
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "PauseMusic") == 0 || strcmp(name, "PauseMusicChannel") == 0) {
		int Channel = 0;

		if (strcmp(name, "PauseMusic") == 0) stack->CorrectParams(0);
		else {
			stack->CorrectParams(1);
			Channel = stack->Pop()->GetInt();
		}

		if (FAILED(PauseMusic(Channel))) stack->PushBool(false);
		else stack->PushBool(true);
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// ResumeMusic / ResumeMusicChannel
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "ResumeMusic") == 0 || strcmp(name, "ResumeMusicChannel") == 0) {
		int Channel = 0;
		if (strcmp(name, "ResumeMusic") == 0) stack->CorrectParams(0);
		else {
			stack->CorrectParams(1);
			Channel = stack->Pop()->GetInt();
		}

		if (FAILED(ResumeMusic(Channel))) stack->PushBool(false);
		else stack->PushBool(true);
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// GetMusic / GetMusicChannel
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "GetMusic") == 0 || strcmp(name, "GetMusicChannel") == 0) {
		int Channel = 0;
		if (strcmp(name, "GetMusic") == 0) stack->CorrectParams(0);
		else {
			stack->CorrectParams(1);
			Channel = stack->Pop()->GetInt();
		}
		if (Channel < 0 || Channel >= NUM_MUSIC_CHANNELS) stack->PushNULL();
		else {
			if (!_music[Channel] || !_music[Channel]->_soundFilename) stack->PushNULL();
			else stack->PushString(_music[Channel]->_soundFilename);
		}
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SetMusicPosition / SetMusicChannelPosition
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SetMusicPosition") == 0 || strcmp(name, "SetMusicChannelPosition") == 0 || strcmp(name, "SetMusicPositionChannel") == 0) {
		int Channel = 0;
		if (strcmp(name, "SetMusicPosition") == 0) stack->CorrectParams(1);
		else {
			stack->CorrectParams(2);
			Channel = stack->Pop()->GetInt();
		}

		uint32 Time = stack->Pop()->GetInt();

		if (FAILED(SetMusicStartTime(Channel, Time))) stack->PushBool(false);
		else stack->PushBool(true);

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// GetMusicPosition / GetMusicChannelPosition
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "GetMusicPosition") == 0 || strcmp(name, "GetMusicChannelPosition") == 0) {
		int Channel = 0;
		if (strcmp(name, "GetMusicPosition") == 0) stack->CorrectParams(0);
		else {
			stack->CorrectParams(1);
			Channel = stack->Pop()->GetInt();
		}

		if (Channel < 0 || Channel >= NUM_MUSIC_CHANNELS || !_music[Channel]) stack->PushInt(0);
		else stack->PushInt(_music[Channel]->getPositionTime());
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// IsMusicPlaying / IsMusicChannelPlaying
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "IsMusicPlaying") == 0 || strcmp(name, "IsMusicChannelPlaying") == 0) {
		int Channel = 0;
		if (strcmp(name, "IsMusicPlaying") == 0) stack->CorrectParams(0);
		else {
			stack->CorrectParams(1);
			Channel = stack->Pop()->GetInt();
		}

		if (Channel < 0 || Channel >= NUM_MUSIC_CHANNELS || !_music[Channel]) stack->PushBool(false);
		else stack->PushBool(_music[Channel]->isPlaying());
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SetMusicVolume / SetMusicChannelVolume
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SetMusicVolume") == 0 || strcmp(name, "SetMusicChannelVolume") == 0) {
		int Channel = 0;
		if (strcmp(name, "SetMusicVolume") == 0) stack->CorrectParams(1);
		else {
			stack->CorrectParams(2);
			Channel = stack->Pop()->GetInt();
		}

		int Volume = stack->Pop()->GetInt();
		if (Channel < 0 || Channel >= NUM_MUSIC_CHANNELS || !_music[Channel]) stack->PushBool(false);
		else {
			if (FAILED(_music[Channel]->setVolume(Volume))) stack->PushBool(false);
			else stack->PushBool(true);
		}
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// GetMusicVolume / GetMusicChannelVolume
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "GetMusicVolume") == 0 || strcmp(name, "GetMusicChannelVolume") == 0) {
		int Channel = 0;
		if (strcmp(name, "GetMusicVolume") == 0) stack->CorrectParams(0);
		else {
			stack->CorrectParams(1);
			Channel = stack->Pop()->GetInt();
		}

		if (Channel < 0 || Channel >= NUM_MUSIC_CHANNELS || !_music[Channel]) stack->PushInt(0);
		else stack->PushInt(_music[Channel]->getVolume());

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// MusicCrossfade
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "MusicCrossfade") == 0) {
		stack->CorrectParams(4);
		int Channel1 = stack->Pop()->GetInt(0);
		int Channel2 = stack->Pop()->GetInt(0);
		uint32 FadeLength = (uint32)stack->Pop()->GetInt(0);
		bool Swap = stack->Pop()->GetBool(true);

		if (_musicCrossfadeRunning) {
			script->RuntimeError("Game.MusicCrossfade: Music crossfade is already in progress.");
			stack->PushBool(false);
			return S_OK;
		}

		_musicCrossfadeStartTime = _liveTimer;
		_musicCrossfadeChannel1 = Channel1;
		_musicCrossfadeChannel2 = Channel2;
		_musicCrossfadeLength = FadeLength;
		_musicCrossfadeSwap = Swap;

		_musicCrossfadeRunning = true;

		stack->PushBool(true);
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// GetSoundLength
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "GetSoundLength") == 0) {
		stack->CorrectParams(1);

		int Length = 0;
		const char *Filename = stack->Pop()->GetString();

		CBSound *Sound = new CBSound(Game);
		if (Sound && SUCCEEDED(Sound->setSound(Filename, SOUND_MUSIC, true))) {
			Length = Sound->getLength();
			delete Sound;
			Sound = NULL;
		}
		stack->PushInt(Length);
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SetMousePos
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SetMousePos") == 0) {
		stack->CorrectParams(2);
		int x = stack->Pop()->GetInt();
		int y = stack->Pop()->GetInt();
		x = MAX(x, 0);
		x = MIN(x, _renderer->_width);
		y = MAX(y, 0);
		y = MIN(y, _renderer->_height);
		POINT p;
		p.x = x + _renderer->_drawOffsetX;
		p.y = y + _renderer->_drawOffsetY;

		CBPlatform::SetCursorPos(p.x, p.y);

		stack->PushNULL();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// LockMouseRect
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "LockMouseRect") == 0) {
		stack->CorrectParams(4);
		int left = stack->Pop()->GetInt();
		int top = stack->Pop()->GetInt();
		int right = stack->Pop()->GetInt();
		int bottom = stack->Pop()->GetInt();

		if (right < left) CBUtils::Swap(&left, &right);
		if (bottom < top) CBUtils::Swap(&top, &bottom);

		CBPlatform::SetRect(&_mouseLockRect, left, top, right, bottom);

		stack->PushNULL();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// PlayVideo
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "PlayVideo") == 0) {
		/*      stack->CorrectParams(0);
		        stack->PushBool(false);

		        return S_OK;
		        // TODO: ADDVIDEO
		        */

		Game->LOG(0, "Warning: Game.PlayVideo() is now deprecated. Use Game.PlayTheora() instead.");

		stack->CorrectParams(6);
		const char *Filename = stack->Pop()->GetString();
		warning("PlayVideo: %s - not implemented yet", Filename);
		CScValue *valType = stack->Pop();
		int Type;
		if (valType->IsNULL()) Type = (int)VID_PLAY_STRETCH;
		else Type = valType->GetInt();

		int X = stack->Pop()->GetInt();
		int Y = stack->Pop()->GetInt();
		bool FreezeMusic = stack->Pop()->GetBool(true);

		CScValue *valSub = stack->Pop();
		const char *SubtitleFile = valSub->IsNULL() ? NULL : valSub->GetString();

		if (Type < (int)VID_PLAY_POS || Type > (int)VID_PLAY_CENTER)
			Type = (int)VID_PLAY_STRETCH;

		if (SUCCEEDED(Game->_videoPlayer->initialize(Filename, SubtitleFile))) {
			if (SUCCEEDED(Game->_videoPlayer->play((TVideoPlayback)Type, X, Y, FreezeMusic))) {
				stack->PushBool(true);
				script->Sleep(0);
			} else stack->PushBool(false);
		} else stack->PushBool(false);

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// PlayTheora
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "PlayTheora") == 0) {
		/*      stack->CorrectParams(0);
		        stack->PushBool(false);

		        return S_OK;*/
		// TODO: ADDVIDEO

		stack->CorrectParams(7);
		const char *Filename = stack->Pop()->GetString();
		CScValue *valType = stack->Pop();
		int Type;
		if (valType->IsNULL())
			Type = (int)VID_PLAY_STRETCH;
		else Type = valType->GetInt();

		int X = stack->Pop()->GetInt();
		int Y = stack->Pop()->GetInt();
		bool FreezeMusic = stack->Pop()->GetBool(true);
		bool DropFrames = stack->Pop()->GetBool(true);

		CScValue *valSub = stack->Pop();
		const char *SubtitleFile = valSub->IsNULL() ? NULL : valSub->GetString();

		if (Type < (int)VID_PLAY_POS || Type > (int)VID_PLAY_CENTER) Type = (int)VID_PLAY_STRETCH;

		delete _theoraPlayer;
		_theoraPlayer = new CVidTheoraPlayer(this);
		if (_theoraPlayer && SUCCEEDED(_theoraPlayer->initialize(Filename, SubtitleFile))) {
			_theoraPlayer->_dontDropFrames = !DropFrames;
			if (SUCCEEDED(_theoraPlayer->play((TVideoPlayback)Type, X, Y, true, FreezeMusic))) {
				stack->PushBool(true);
				script->Sleep(0);
			} else stack->PushBool(false);
		} else {
			stack->PushBool(false);
			delete _theoraPlayer;
			_theoraPlayer = NULL;
		}

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// QuitGame
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "QuitGame") == 0) {
		stack->CorrectParams(0);
		stack->PushNULL();
		_quitting = true;
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// RegWriteNumber
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "RegWriteNumber") == 0) {
		stack->CorrectParams(2);
		const char *Key = stack->Pop()->GetString();
		int Val = stack->Pop()->GetInt();
		_registry->WriteInt("PrivateSettings", Key, Val);
		stack->PushNULL();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// RegReadNumber
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "RegReadNumber") == 0) {
		stack->CorrectParams(2);
		const char *Key = stack->Pop()->GetString();
		int InitVal = stack->Pop()->GetInt();
		stack->PushInt(_registry->ReadInt("PrivateSettings", Key, InitVal));
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// RegWriteString
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "RegWriteString") == 0) {
		stack->CorrectParams(2);
		const char *Key = stack->Pop()->GetString();
		const char *Val = stack->Pop()->GetString();
		_registry->WriteString("PrivateSettings", Key, Val);
		stack->PushNULL();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// RegReadString
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "RegReadString") == 0) {
		stack->CorrectParams(2);
		const char *Key = stack->Pop()->GetString();
		const char *InitVal = stack->Pop()->GetString();
		AnsiString val = _registry->ReadString("PrivateSettings", Key, InitVal);
		stack->PushString(val.c_str());
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SaveGame
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SaveGame") == 0) {
		stack->CorrectParams(3);
		int Slot = stack->Pop()->GetInt();
		const char *xdesc = stack->Pop()->GetString();
		bool quick = stack->Pop()->GetBool(false);

		char *Desc = new char[strlen(xdesc) + 1];
		strcpy(Desc, xdesc);
		stack->PushBool(true);
		if (FAILED(SaveGame(Slot, Desc, quick))) {
			stack->Pop();
			stack->PushBool(false);
		}
		delete [] Desc;
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// LoadGame
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "LoadGame") == 0) {
		stack->CorrectParams(1);
		_scheduledLoadSlot = stack->Pop()->GetInt();
		_loading = true;
		stack->PushBool(false);
		script->Sleep(0);
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// IsSaveSlotUsed
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "IsSaveSlotUsed") == 0) {
		stack->CorrectParams(1);
		int Slot = stack->Pop()->GetInt();
		stack->PushBool(IsSaveSlotUsed(Slot));
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// GetSaveSlotDescription
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "GetSaveSlotDescription") == 0) {
		stack->CorrectParams(1);
		int Slot = stack->Pop()->GetInt();
		char Desc[512];
		Desc[0] = '\0';
		GetSaveSlotDescription(Slot, Desc);
		stack->PushString(Desc);
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// EmptySaveSlot
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "EmptySaveSlot") == 0) {
		stack->CorrectParams(1);
		int Slot = stack->Pop()->GetInt();
		EmptySaveSlot(Slot);
		stack->PushNULL();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SetGlobalSFXVolume
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SetGlobalSFXVolume") == 0) {
		stack->CorrectParams(1);
		Game->_soundMgr->setVolumePercent(SOUND_SFX, (byte)stack->Pop()->GetInt());
		stack->PushNULL();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SetGlobalSpeechVolume
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SetGlobalSpeechVolume") == 0) {
		stack->CorrectParams(1);
		Game->_soundMgr->setVolumePercent(SOUND_SPEECH, (byte)stack->Pop()->GetInt());
		stack->PushNULL();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SetGlobalMusicVolume
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SetGlobalMusicVolume") == 0) {
		stack->CorrectParams(1);
		Game->_soundMgr->setVolumePercent(SOUND_MUSIC, (byte)stack->Pop()->GetInt());
		stack->PushNULL();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SetGlobalMasterVolume
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SetGlobalMasterVolume") == 0) {
		stack->CorrectParams(1);
		Game->_soundMgr->setMasterVolumePercent((byte)stack->Pop()->GetInt());
		stack->PushNULL();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// GetGlobalSFXVolume
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "GetGlobalSFXVolume") == 0) {
		stack->CorrectParams(0);
		stack->PushInt(_soundMgr->getVolumePercent(SOUND_SFX));
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// GetGlobalSpeechVolume
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "GetGlobalSpeechVolume") == 0) {
		stack->CorrectParams(0);
		stack->PushInt(_soundMgr->getVolumePercent(SOUND_SPEECH));
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// GetGlobalMusicVolume
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "GetGlobalMusicVolume") == 0) {
		stack->CorrectParams(0);
		stack->PushInt(_soundMgr->getVolumePercent(SOUND_MUSIC));
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// GetGlobalMasterVolume
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "GetGlobalMasterVolume") == 0) {
		stack->CorrectParams(0);
		stack->PushInt(_soundMgr->getMasterVolumePercent());
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SetActiveCursor
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SetActiveCursor") == 0) {
		stack->CorrectParams(1);
		if (SUCCEEDED(setActiveCursor(stack->Pop()->GetString()))) stack->PushBool(true);
		else stack->PushBool(false);

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// GetActiveCursor
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "GetActiveCursor") == 0) {
		stack->CorrectParams(0);
		if (!_activeCursor || !_activeCursor->_filename) stack->PushNULL();
		else stack->PushString(_activeCursor->_filename);

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// GetActiveCursorObject
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "GetActiveCursorObject") == 0) {
		stack->CorrectParams(0);
		if (!_activeCursor) stack->PushNULL();
		else stack->PushNative(_activeCursor, true);

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// RemoveActiveCursor
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "RemoveActiveCursor") == 0) {
		stack->CorrectParams(0);
		delete _activeCursor;
		_activeCursor = NULL;
		stack->PushNULL();

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// HasActiveCursor
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "HasActiveCursor") == 0) {
		stack->CorrectParams(0);

		if (_activeCursor) stack->PushBool(true);
		else stack->PushBool(false);

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// FileExists
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "FileExists") == 0) {
		stack->CorrectParams(1);
		const char *Filename = stack->Pop()->GetString();

		Common::SeekableReadStream *File = _fileManager->openFile(Filename, false);
		if (!File) stack->PushBool(false);
		else {
			_fileManager->closeFile(File);
			stack->PushBool(true);
		}
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// FadeOut / FadeOutAsync / SystemFadeOut / SystemFadeOutAsync
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "FadeOut") == 0 || strcmp(name, "FadeOutAsync") == 0 || strcmp(name, "SystemFadeOut") == 0 || strcmp(name, "SystemFadeOutAsync") == 0) {
		stack->CorrectParams(5);
		uint32 Duration = stack->Pop()->GetInt(500);
		byte Red = stack->Pop()->GetInt(0);
		byte Green = stack->Pop()->GetInt(0);
		byte Blue = stack->Pop()->GetInt(0);
		byte Alpha = stack->Pop()->GetInt(0xFF);

		bool System = (strcmp(name, "SystemFadeOut") == 0 || strcmp(name, "SystemFadeOutAsync") == 0);

		_fader->fadeOut(DRGBA(Red, Green, Blue, Alpha), Duration, System);
		if (strcmp(name, "FadeOutAsync") != 0 && strcmp(name, "SystemFadeOutAsync") != 0) script->WaitFor(_fader);

		stack->PushNULL();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// FadeIn / FadeInAsync / SystemFadeIn / SystemFadeInAsync
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "FadeIn") == 0 || strcmp(name, "FadeInAsync") == 0 || strcmp(name, "SystemFadeIn") == 0 || strcmp(name, "SystemFadeInAsync") == 0) {
		stack->CorrectParams(5);
		uint32 Duration = stack->Pop()->GetInt(500);
		byte Red = stack->Pop()->GetInt(0);
		byte Green = stack->Pop()->GetInt(0);
		byte Blue = stack->Pop()->GetInt(0);
		byte Alpha = stack->Pop()->GetInt(0xFF);

		bool System = (strcmp(name, "SystemFadeIn") == 0 || strcmp(name, "SystemFadeInAsync") == 0);

		_fader->fadeIn(DRGBA(Red, Green, Blue, Alpha), Duration, System);
		if (strcmp(name, "FadeInAsync") != 0 && strcmp(name, "SystemFadeInAsync") != 0) script->WaitFor(_fader);

		stack->PushNULL();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// GetFadeColor
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "GetFadeColor") == 0) {
		stack->CorrectParams(0);
		stack->PushInt(_fader->getCurrentColor());
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// Screenshot
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Screenshot") == 0) {
		stack->CorrectParams(1);
		char Filename[MAX_PATH];

		CScValue *Val = stack->Pop();

		warning("BGame::ScCallMethod - Screenshot not reimplemented"); //TODO
		int FileNum = 0;

		while (true) {
			sprintf(Filename, "%s%03d.bmp", Val->IsNULL() ? _name : Val->GetString(), FileNum);
			if (!Common::File::exists(Filename))
				break;
			FileNum++;
		}

		bool ret = false;
		CBImage *Image = Game->_renderer->takeScreenshot();
		if (Image) {
			ret = SUCCEEDED(Image->SaveBMPFile(Filename));
			delete Image;
		} else ret = false;

		stack->PushBool(ret);
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// ScreenshotEx
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "ScreenshotEx") == 0) {
		stack->CorrectParams(3);
		const char *Filename = stack->Pop()->GetString();
		int SizeX = stack->Pop()->GetInt(_renderer->_width);
		int SizeY = stack->Pop()->GetInt(_renderer->_height);

		bool ret = false;
		CBImage *Image = Game->_renderer->takeScreenshot();
		if (Image) {
			ret = SUCCEEDED(Image->Resize(SizeX, SizeY));
			if (ret) ret = SUCCEEDED(Image->SaveBMPFile(Filename));
			delete Image;
		} else ret = false;

		stack->PushBool(ret);
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// CreateWindow
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "CreateWindow") == 0) {
		stack->CorrectParams(1);
		CScValue *Val = stack->Pop();

		CUIWindow *Win = new CUIWindow(Game);
		_windows.Add(Win);
		RegisterObject(Win);
		if (!Val->IsNULL()) Win->setName(Val->GetString());
		stack->PushNative(Win, true);
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// DeleteWindow
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "DeleteWindow") == 0) {
		stack->CorrectParams(1);
		CBObject *Obj = (CBObject *)stack->Pop()->GetNative();
		for (int i = 0; i < _windows.GetSize(); i++) {
			if (_windows[i] == Obj) {
				UnregisterObject(_windows[i]);
				stack->PushBool(true);
				return S_OK;
			}
		}
		stack->PushBool(false);
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// OpenDocument
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "OpenDocument") == 0) {
		stack->CorrectParams(0);
		stack->PushNULL();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// DEBUG_DumpClassRegistry
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "DEBUG_DumpClassRegistry") == 0) {
		stack->CorrectParams(0);
		DEBUG_DumpClassRegistry();
		stack->PushNULL();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SetLoadingScreen
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SetLoadingScreen") == 0) {
		stack->CorrectParams(3);
		CScValue *Val = stack->Pop();
		_loadImageX = stack->Pop()->GetInt();
		_loadImageY = stack->Pop()->GetInt();

		if (Val->IsNULL()) {
			delete[] _loadImageName;
			_loadImageName = NULL;
		} else {
			CBUtils::SetString(&_loadImageName, Val->GetString());
		}
		stack->PushNULL();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SetSavingScreen
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SetSavingScreen") == 0) {
		stack->CorrectParams(3);
		CScValue *Val = stack->Pop();
		_saveImageX = stack->Pop()->GetInt();
		_saveImageY = stack->Pop()->GetInt();

		if (Val->IsNULL()) {
			delete[] _saveImageName;
			_saveImageName = NULL;
		} else {
			CBUtils::SetString(&_saveImageName, Val->GetString());
		}
		stack->PushNULL();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SetWaitCursor
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SetWaitCursor") == 0) {
		stack->CorrectParams(1);
		if (SUCCEEDED(SetWaitCursor(stack->Pop()->GetString()))) stack->PushBool(true);
		else stack->PushBool(false);

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// RemoveWaitCursor
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "RemoveWaitCursor") == 0) {
		stack->CorrectParams(0);
		delete _cursorNoninteractive;
		_cursorNoninteractive = NULL;

		stack->PushNULL();

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// GetWaitCursor
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "GetWaitCursor") == 0) {
		stack->CorrectParams(0);
		if (!_cursorNoninteractive || !_cursorNoninteractive->_filename) stack->PushNULL();
		else stack->PushString(_cursorNoninteractive->_filename);

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// GetWaitCursorObject
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "GetWaitCursorObject") == 0) {
		stack->CorrectParams(0);
		if (!_cursorNoninteractive) stack->PushNULL();
		else stack->PushNative(_cursorNoninteractive, true);

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// ClearScriptCache
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "ClearScriptCache") == 0) {
		stack->CorrectParams(0);
		stack->PushBool(SUCCEEDED(_scEngine->EmptyScriptCache()));
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// DisplayLoadingIcon
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "DisplayLoadingIcon") == 0) {
		stack->CorrectParams(4);

		const char *Filename = stack->Pop()->GetString();
		_loadingIconX = stack->Pop()->GetInt();
		_loadingIconY = stack->Pop()->GetInt();
		_loadingIconPersistent = stack->Pop()->GetBool();

		delete _loadingIcon;
		_loadingIcon = new CBSprite(this);
		if (!_loadingIcon || FAILED(_loadingIcon->loadFile(Filename))) {
			delete _loadingIcon;
			_loadingIcon = NULL;
		} else {
			DisplayContent(false, true);
			Game->_renderer->flip();
			Game->_renderer->initLoop();
		}
		stack->PushNULL();

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// HideLoadingIcon
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "HideLoadingIcon") == 0) {
		stack->CorrectParams(0);
		delete _loadingIcon;
		_loadingIcon = NULL;
		stack->PushNULL();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// DumpTextureStats
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "DumpTextureStats") == 0) {
		stack->CorrectParams(1);
		const char *Filename = stack->Pop()->GetString();

		_renderer->dumpData(Filename);

		stack->PushNULL();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// AccOutputText
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "AccOutputText") == 0) {
		stack->CorrectParams(2);
		/* const char *Str = */
		stack->Pop()->GetString();
		/* int Type = */
		stack->Pop()->GetInt();
		// do nothing
		stack->PushNULL();

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// StoreSaveThumbnail
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "StoreSaveThumbnail") == 0) {
		stack->CorrectParams(0);
		delete _cachedThumbnail;
		_cachedThumbnail = new CBSaveThumbHelper(this);
		if (FAILED(_cachedThumbnail->StoreThumbnail())) {
			delete _cachedThumbnail;
			_cachedThumbnail = NULL;
			stack->PushBool(false);
		} else stack->PushBool(true);

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// DeleteSaveThumbnail
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "DeleteSaveThumbnail") == 0) {
		stack->CorrectParams(0);
		delete _cachedThumbnail;
		_cachedThumbnail = NULL;
		stack->PushNULL();

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// GetFileChecksum
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "GetFileChecksum") == 0) {
		stack->CorrectParams(2);
		const char *Filename = stack->Pop()->GetString();
		bool AsHex = stack->Pop()->GetBool(false);

		Common::SeekableReadStream *File = _fileManager->openFile(Filename, false);
		if (File) {
			crc remainder = crc_initialize();
			byte Buf[1024];
			int BytesRead = 0;

			while (BytesRead < File->size()) {
				int BufSize = MIN((uint32)1024, (uint32)(File->size() - BytesRead));
				BytesRead += File->read(Buf, BufSize);

				for (int i = 0; i < BufSize; i++) {
					remainder = crc_process_byte(Buf[i], remainder);
				}
			}
			crc checksum = crc_finalize(remainder);

			if (AsHex) {
				char Hex[100];
				sprintf(Hex, "%x", checksum);
				stack->PushString(Hex);
			} else
				stack->PushInt(checksum);

			_fileManager->closeFile(File);
			File = NULL;
		} else stack->PushNULL();

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// EnableScriptProfiling
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "EnableScriptProfiling") == 0) {
		stack->CorrectParams(0);
		_scEngine->EnableProfiling();
		stack->PushNULL();

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// DisableScriptProfiling
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "DisableScriptProfiling") == 0) {
		stack->CorrectParams(0);
		_scEngine->DisableProfiling();
		stack->PushNULL();

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// ShowStatusLine
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "ShowStatusLine") == 0) {
		stack->CorrectParams(0);
#ifdef __IPHONEOS__
		IOS_ShowStatusLine(TRUE);
#endif
		stack->PushNULL();

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// HideStatusLine
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "HideStatusLine") == 0) {
		stack->CorrectParams(0);
#ifdef __IPHONEOS__
		IOS_ShowStatusLine(FALSE);
#endif
		stack->PushNULL();

		return S_OK;
	}

	else return CBObject::scCallMethod(script, stack, thisStack, name);
}


//////////////////////////////////////////////////////////////////////////
CScValue *CBGame::scGetProperty(const char *name) {
	_scValue->SetNULL();

	//////////////////////////////////////////////////////////////////////////
	// Type
	//////////////////////////////////////////////////////////////////////////
	if (strcmp(name, "Type") == 0) {
		_scValue->SetString("game");
		return _scValue;
	}
	//////////////////////////////////////////////////////////////////////////
	// Name
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Name") == 0) {
		_scValue->SetString(_name);
		return _scValue;
	}
	//////////////////////////////////////////////////////////////////////////
	// Hwnd (RO)
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Hwnd") == 0) {
		_scValue->SetInt((int)_renderer->_window);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// CurrentTime (RO)
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "CurrentTime") == 0) {
		_scValue->SetInt((int)_timer);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// WindowsTime (RO)
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "WindowsTime") == 0) {
		_scValue->SetInt((int)CBPlatform::GetTime());
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// WindowedMode (RO)
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "WindowedMode") == 0) {
		_scValue->SetBool(_renderer->_windowed);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// MouseX
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "MouseX") == 0) {
		_scValue->SetInt(_mousePos.x);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// MouseY
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "MouseY") == 0) {
		_scValue->SetInt(_mousePos.y);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// MainObject
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "MainObject") == 0) {
		_scValue->SetNative(_mainObject, true);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// ActiveObject (RO)
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "ActiveObject") == 0) {
		_scValue->SetNative(_activeObject, true);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// ScreenWidth (RO)
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "ScreenWidth") == 0) {
		_scValue->SetInt(_renderer->_width);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// ScreenHeight (RO)
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "ScreenHeight") == 0) {
		_scValue->SetInt(_renderer->_height);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// Interactive
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Interactive") == 0) {
		_scValue->SetBool(_interactive);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// DebugMode (RO)
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "DebugMode") == 0) {
		_scValue->SetBool(_dEBUG_DebugMode);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// SoundAvailable (RO)
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SoundAvailable") == 0) {
		_scValue->SetBool(_soundMgr->_soundAvailable);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// SFXVolume
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SFXVolume") == 0) {
		Game->LOG(0, "**Warning** The SFXVolume attribute is obsolete");
		_scValue->SetInt(_soundMgr->getVolumePercent(SOUND_SFX));
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// SpeechVolume
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SpeechVolume") == 0) {
		Game->LOG(0, "**Warning** The SpeechVolume attribute is obsolete");
		_scValue->SetInt(_soundMgr->getVolumePercent(SOUND_SPEECH));
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// MusicVolume
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "MusicVolume") == 0) {
		Game->LOG(0, "**Warning** The MusicVolume attribute is obsolete");
		_scValue->SetInt(_soundMgr->getVolumePercent(SOUND_MUSIC));
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// MasterVolume
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "MasterVolume") == 0) {
		Game->LOG(0, "**Warning** The MasterVolume attribute is obsolete");
		_scValue->SetInt(_soundMgr->getMasterVolumePercent());
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// Keyboard (RO)
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Keyboard") == 0) {
		if (_keyboardState) _scValue->SetNative(_keyboardState, true);
		else _scValue->SetNULL();

		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// Subtitles
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Subtitles") == 0) {
		_scValue->SetBool(_subtitles);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// SubtitlesSpeed
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SubtitlesSpeed") == 0) {
		_scValue->SetInt(_subtitlesSpeed);
		return _scValue;
	}
	//////////////////////////////////////////////////////////////////////////
	// VideoSubtitles
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "VideoSubtitles") == 0) {
		_scValue->SetBool(_videoSubtitles);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// FPS (RO)
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "FPS") == 0) {
		_scValue->SetInt(_fps);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// AcceleratedMode / Accelerated (RO)
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "AcceleratedMode") == 0 || strcmp(name, "Accelerated") == 0) {
		_scValue->SetBool(_useD3D);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// TextEncoding
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "TextEncoding") == 0) {
		_scValue->SetInt(_textEncoding);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// TextRTL
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "TextRTL") == 0) {
		_scValue->SetBool(_textRTL);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// SoundBufferSize
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SoundBufferSize") == 0) {
		_scValue->SetInt(_soundBufferSizeSec);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// SuspendedRendering
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SuspendedRendering") == 0) {
		_scValue->SetBool(_suspendedRendering);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// SuppressScriptErrors
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SuppressScriptErrors") == 0) {
		_scValue->SetBool(_suppressScriptErrors);
		return _scValue;
	}


	//////////////////////////////////////////////////////////////////////////
	// Frozen
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Frozen") == 0) {
		_scValue->SetBool(_state == GAME_FROZEN);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// AccTTSEnabled
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "AccTTSEnabled") == 0) {
		_scValue->SetBool(false);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// AccTTSTalk
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "AccTTSTalk") == 0) {
		_scValue->SetBool(false);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// AccTTSCaptions
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "AccTTSCaptions") == 0) {
		_scValue->SetBool(false);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// AccTTSKeypress
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "AccTTSKeypress") == 0) {
		_scValue->SetBool(false);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// AccKeyboardEnabled
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "AccKeyboardEnabled") == 0) {
		_scValue->SetBool(false);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// AccKeyboardCursorSkip
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "AccKeyboardCursorSkip") == 0) {
		_scValue->SetBool(false);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// AccKeyboardPause
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "AccKeyboardPause") == 0) {
		_scValue->SetBool(false);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// AutorunDisabled
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "AutorunDisabled") == 0) {
		_scValue->SetBool(_autorunDisabled);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// SaveDirectory (RO)
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SaveDirectory") == 0) {
		AnsiString dataDir = GetDataDir();
		_scValue->SetString(dataDir.c_str());
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// AutoSaveOnExit
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "AutoSaveOnExit") == 0) {
		_scValue->SetBool(_autoSaveOnExit);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// AutoSaveSlot
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "AutoSaveSlot") == 0) {
		_scValue->SetInt(_autoSaveSlot);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// CursorHidden
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "CursorHidden") == 0) {
		_scValue->SetBool(_cursorHidden);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// Platform (RO)
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Platform") == 0) {
		_scValue->SetString(CBPlatform::GetPlatformName().c_str());
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// DeviceType (RO)
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "DeviceType") == 0) {
		_scValue->SetString(GetDeviceType().c_str());
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// MostRecentSaveSlot (RO)
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "MostRecentSaveSlot") == 0) {
		_scValue->SetInt(_registry->ReadInt("System", "MostRecentSaveSlot", -1));
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// Store (RO)
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Store") == 0) {
		if (_store) _scValue->SetNative(_store, true);
		else _scValue->SetNULL();

		return _scValue;
	}

	else return CBObject::scGetProperty(name);
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::scSetProperty(const char *name, CScValue *Value) {
	//////////////////////////////////////////////////////////////////////////
	// Name
	//////////////////////////////////////////////////////////////////////////
	if (strcmp(name, "Name") == 0) {
		setName(Value->GetString());

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// MouseX
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "MouseX") == 0) {
		_mousePos.x = Value->GetInt();
		ResetMousePos();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// MouseY
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "MouseY") == 0) {
		_mousePos.y = Value->GetInt();
		ResetMousePos();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// Caption
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Name") == 0) {
		HRESULT res = CBObject::scSetProperty(name, Value);
		SetWindowTitle();
		return res;
	}

	//////////////////////////////////////////////////////////////////////////
	// MainObject
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "MainObject") == 0) {
		CBScriptable *obj = Value->GetNative();
		if (obj == NULL || ValidObject((CBObject *)obj)) _mainObject = (CBObject *)obj;
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// Interactive
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Interactive") == 0) {
		SetInteractive(Value->GetBool());
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SFXVolume
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SFXVolume") == 0) {
		Game->LOG(0, "**Warning** The SFXVolume attribute is obsolete");
		Game->_soundMgr->setVolumePercent(SOUND_SFX, (byte)Value->GetInt());
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SpeechVolume
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SpeechVolume") == 0) {
		Game->LOG(0, "**Warning** The SpeechVolume attribute is obsolete");
		Game->_soundMgr->setVolumePercent(SOUND_SPEECH, (byte)Value->GetInt());
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// MusicVolume
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "MusicVolume") == 0) {
		Game->LOG(0, "**Warning** The MusicVolume attribute is obsolete");
		Game->_soundMgr->setVolumePercent(SOUND_MUSIC, (byte)Value->GetInt());
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// MasterVolume
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "MasterVolume") == 0) {
		Game->LOG(0, "**Warning** The MasterVolume attribute is obsolete");
		Game->_soundMgr->setMasterVolumePercent((byte)Value->GetInt());
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// Subtitles
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Subtitles") == 0) {
		_subtitles = Value->GetBool();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SubtitlesSpeed
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SubtitlesSpeed") == 0) {
		_subtitlesSpeed = Value->GetInt();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// VideoSubtitles
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "VideoSubtitles") == 0) {
		_videoSubtitles = Value->GetBool();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// TextEncoding
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "TextEncoding") == 0) {
		int Enc = Value->GetInt();
		if (Enc < 0) Enc = 0;
		if (Enc >= NUM_TEXT_ENCODINGS) Enc = NUM_TEXT_ENCODINGS - 1;
		_textEncoding = (TTextEncoding)Enc;
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// TextRTL
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "TextRTL") == 0) {
		_textRTL = Value->GetBool();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SoundBufferSize
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SoundBufferSize") == 0) {
		_soundBufferSizeSec = Value->GetInt();
		_soundBufferSizeSec = MAX(3, _soundBufferSizeSec);
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SuspendedRendering
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SuspendedRendering") == 0) {
		_suspendedRendering = Value->GetBool();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SuppressScriptErrors
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SuppressScriptErrors") == 0) {
		_suppressScriptErrors = Value->GetBool();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// AutorunDisabled
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "AutorunDisabled") == 0) {
		_autorunDisabled = Value->GetBool();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// AutoSaveOnExit
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "AutoSaveOnExit") == 0) {
		_autoSaveOnExit = Value->GetBool();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// AutoSaveSlot
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "AutoSaveSlot") == 0) {
		_autoSaveSlot = Value->GetInt();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// CursorHidden
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "CursorHidden") == 0) {
		_cursorHidden = Value->GetBool();
		return S_OK;
	}

	else return CBObject::scSetProperty(name, Value);
}


//////////////////////////////////////////////////////////////////////////
const char *CBGame::scToString() {
	return "[game object]";
}



#define QUICK_MSG_DURATION 3000
//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::DisplayQuickMsg() {
	if (_quickMessages.GetSize() == 0 || !_systemFont) return S_OK;

	int i;

	// update
	for (i = 0; i < _quickMessages.GetSize(); i++) {
		if (_currentTime - _quickMessages[i]->_startTime >= QUICK_MSG_DURATION) {
			delete _quickMessages[i];
			_quickMessages.RemoveAt(i);
			i--;
		}
	}

	int PosY = 20;

	// display
	for (i = 0; i < _quickMessages.GetSize(); i++) {
		_systemFont->drawText((byte *)_quickMessages[i]->GetText(), 0, PosY, _renderer->_width);
		PosY += _systemFont->getTextHeight((byte *)_quickMessages[i]->GetText(), _renderer->_width);
	}
	return S_OK;
}


#define MAX_QUICK_MSG 5
//////////////////////////////////////////////////////////////////////////
void CBGame::QuickMessage(const char *Text) {
	if (_quickMessages.GetSize() >= MAX_QUICK_MSG) {
		delete _quickMessages[0];
		_quickMessages.RemoveAt(0);
	}
	_quickMessages.Add(new CBQuickMsg(Game, Text));
}


//////////////////////////////////////////////////////////////////////////
void CBGame::QuickMessageForm(LPSTR fmt, ...) {
	char buff[256];
	va_list va;

	va_start(va, fmt);
	vsprintf(buff, fmt, va);
	va_end(va);

	QuickMessage(buff);
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::RegisterObject(CBObject *Object) {
	_regObjects.Add(Object);
	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::UnregisterObject(CBObject *Object) {
	if (!Object) return S_OK;

	int i;

	// is it a window?
	for (i = 0; i < _windows.GetSize(); i++) {
		if ((CBObject *)_windows[i] == Object) {
			_windows.RemoveAt(i);

			// get new focused window
			if (_focusedWindow == Object) _focusedWindow = NULL;

			break;
		}
	}

	// is it active object?
	if (_activeObject == Object) _activeObject = NULL;

	// is it main object?
	if (_mainObject == Object) _mainObject = NULL;

	if (_store) _store->OnObjectDestroyed(Object);

	// destroy object
	for (i = 0; i < _regObjects.GetSize(); i++) {
		if (_regObjects[i] == Object) {
			_regObjects.RemoveAt(i);
			if (!_loadInProgress) CSysClassRegistry::getInstance()->enumInstances(InvalidateValues, "CScValue", (void *)Object);
			delete Object;
			return S_OK;
		}
	}

	return E_FAIL;
}


//////////////////////////////////////////////////////////////////////////
void CBGame::InvalidateValues(void *Value, void *Data) {
	CScValue *val = (CScValue *)Value;
	if (val->IsNative() && val->GetNative() == Data) {
		if (!val->_persistent && ((CBScriptable *)Data)->_refCount == 1) {
			((CBScriptable *)Data)->_refCount++;
		}
		val->SetNative(NULL);
		val->SetNULL();
	}
}



//////////////////////////////////////////////////////////////////////////
bool CBGame::ValidObject(CBObject *Object) {
	if (!Object) return false;
	if (Object == this) return true;

	for (int i = 0; i < _regObjects.GetSize(); i++) {
		if (_regObjects[i] == Object) return true;
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
void CBGame::PublishNatives() {
	if (!_scEngine || !_scEngine->_compilerAvailable) return;

	_scEngine->ExtDefineFunction("LOG");
	_scEngine->ExtDefineFunction("String");
	_scEngine->ExtDefineFunction("MemBuffer");
	_scEngine->ExtDefineFunction("File");
	_scEngine->ExtDefineFunction("Date");
	_scEngine->ExtDefineFunction("Array");
	_scEngine->ExtDefineFunction("TcpClient");
	_scEngine->ExtDefineFunction("Object");
	//_scEngine->ExtDefineFunction("Game");
	_scEngine->ExtDefineFunction("Sleep");
	_scEngine->ExtDefineFunction("WaitFor");
	_scEngine->ExtDefineFunction("Random");
	_scEngine->ExtDefineFunction("SetScriptTimeSlice");
	_scEngine->ExtDefineFunction("MakeRGBA");
	_scEngine->ExtDefineFunction("MakeRGB");
	_scEngine->ExtDefineFunction("MakeHSL");
	_scEngine->ExtDefineFunction("RGB");
	_scEngine->ExtDefineFunction("GetRValue");
	_scEngine->ExtDefineFunction("GetGValue");
	_scEngine->ExtDefineFunction("GetBValue");
	_scEngine->ExtDefineFunction("GetAValue");
	_scEngine->ExtDefineFunction("GetHValue");
	_scEngine->ExtDefineFunction("GetSValue");
	_scEngine->ExtDefineFunction("GetLValue");
	_scEngine->ExtDefineFunction("Debug");

	_scEngine->ExtDefineFunction("ToString");
	_scEngine->ExtDefineFunction("ToInt");
	_scEngine->ExtDefineFunction("ToBool");
	_scEngine->ExtDefineFunction("ToFloat");

	_scEngine->ExtDefineVariable("Game");
	_scEngine->ExtDefineVariable("Math");
	_scEngine->ExtDefineVariable("Directory");
	_scEngine->ExtDefineVariable("self");
	_scEngine->ExtDefineVariable("this");
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::ExternalCall(CScScript *script, CScStack *stack, CScStack *thisStack, char *name) {
	CScValue *this_obj;

	//////////////////////////////////////////////////////////////////////////
	// LOG
	//////////////////////////////////////////////////////////////////////////
	if (strcmp(name, "LOG") == 0) {
		stack->CorrectParams(1);
		Game->LOG(0, "sc: %s", stack->Pop()->GetString());
		stack->PushNULL();
	}

	//////////////////////////////////////////////////////////////////////////
	// String
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "String") == 0) {
		this_obj = thisStack->GetTop();

		this_obj->SetNative(makeSXString(Game, stack));
		stack->PushNULL();
	}

	//////////////////////////////////////////////////////////////////////////
	// MemBuffer
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "MemBuffer") == 0) {
		this_obj = thisStack->GetTop();

		this_obj->SetNative(makeSXMemBuffer(Game, stack));
		stack->PushNULL();
	}

	//////////////////////////////////////////////////////////////////////////
	// File
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "File") == 0) {
		this_obj = thisStack->GetTop();

		this_obj->SetNative(makeSXFile(Game, stack));
		stack->PushNULL();
	}

	//////////////////////////////////////////////////////////////////////////
	// Date
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Date") == 0) {
		this_obj = thisStack->GetTop();

		this_obj->SetNative(makeSXDate(Game, stack));
		stack->PushNULL();
	}

	//////////////////////////////////////////////////////////////////////////
	// Array
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Array") == 0) {
		this_obj = thisStack->GetTop();

		this_obj->SetNative(makeSXArray(Game, stack));
		stack->PushNULL();
	}

	//////////////////////////////////////////////////////////////////////////
	// Object
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Object") == 0) {
		this_obj = thisStack->GetTop();

		this_obj->SetNative(makeSXObject(Game, stack));
		stack->PushNULL();
	}

	//////////////////////////////////////////////////////////////////////////
	// Sleep
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Sleep") == 0) {
		stack->CorrectParams(1);

		script->Sleep((uint32)stack->Pop()->GetInt());
		stack->PushNULL();
	}

	//////////////////////////////////////////////////////////////////////////
	// WaitFor
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "WaitFor") == 0) {
		stack->CorrectParams(1);

		CBScriptable *obj = stack->Pop()->GetNative();
		if (ValidObject((CBObject *)obj)) script->WaitForExclusive((CBObject *)obj);
		stack->PushNULL();
	}

	//////////////////////////////////////////////////////////////////////////
	// Random
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Random") == 0) {
		stack->CorrectParams(2);

		int from = stack->Pop()->GetInt();
		int to   = stack->Pop()->GetInt();

		stack->PushInt(CBUtils::RandomInt(from, to));
	}

	//////////////////////////////////////////////////////////////////////////
	// SetScriptTimeSlice
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SetScriptTimeSlice") == 0) {
		stack->CorrectParams(1);

		script->_timeSlice = (uint32)stack->Pop()->GetInt();
		stack->PushNULL();
	}

	//////////////////////////////////////////////////////////////////////////
	// MakeRGBA / MakeRGB / RGB
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "MakeRGBA") == 0 || strcmp(name, "MakeRGB") == 0 || strcmp(name, "RGB") == 0) {
		stack->CorrectParams(4);
		int r = stack->Pop()->GetInt();
		int g = stack->Pop()->GetInt();
		int b = stack->Pop()->GetInt();
		int a;
		CScValue *val = stack->Pop();
		if (val->IsNULL()) a = 255;
		else a = val->GetInt();

		stack->PushInt(DRGBA(r, g, b, a));
	}

	//////////////////////////////////////////////////////////////////////////
	// MakeHSL
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "MakeHSL") == 0) {
		stack->CorrectParams(3);
		int h = stack->Pop()->GetInt();
		int s = stack->Pop()->GetInt();
		int l = stack->Pop()->GetInt();

		stack->PushInt(CBUtils::HSLtoRGB(h, s, l));
	}

	//////////////////////////////////////////////////////////////////////////
	// GetRValue
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "GetRValue") == 0) {
		stack->CorrectParams(1);

		uint32 rgba = (uint32)stack->Pop()->GetInt();
		stack->PushInt(D3DCOLGetR(rgba));
	}

	//////////////////////////////////////////////////////////////////////////
	// GetGValue
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "GetGValue") == 0) {
		stack->CorrectParams(1);

		uint32 rgba = (uint32)stack->Pop()->GetInt();
		stack->PushInt(D3DCOLGetG(rgba));
	}

	//////////////////////////////////////////////////////////////////////////
	// GetBValue
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "GetBValue") == 0) {
		stack->CorrectParams(1);

		uint32 rgba = (uint32)stack->Pop()->GetInt();
		stack->PushInt(D3DCOLGetB(rgba));
	}

	//////////////////////////////////////////////////////////////////////////
	// GetAValue
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "GetAValue") == 0) {
		stack->CorrectParams(1);

		uint32 rgba = (uint32)stack->Pop()->GetInt();
		stack->PushInt(D3DCOLGetA(rgba));
	}

	//////////////////////////////////////////////////////////////////////////
	// GetHValue
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "GetHValue") == 0) {
		stack->CorrectParams(1);
		uint32 rgb = (uint32)stack->Pop()->GetInt();

		byte H, S, L;
		CBUtils::RGBtoHSL(rgb, &H, &S, &L);
		stack->PushInt(H);
	}

	//////////////////////////////////////////////////////////////////////////
	// GetSValue
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "GetSValue") == 0) {
		stack->CorrectParams(1);
		uint32 rgb = (uint32)stack->Pop()->GetInt();

		byte H, S, L;
		CBUtils::RGBtoHSL(rgb, &H, &S, &L);
		stack->PushInt(S);
	}

	//////////////////////////////////////////////////////////////////////////
	// GetLValue
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "GetLValue") == 0) {
		stack->CorrectParams(1);
		uint32 rgb = (uint32)stack->Pop()->GetInt();

		byte H, S, L;
		CBUtils::RGBtoHSL(rgb, &H, &S, &L);
		stack->PushInt(L);
	}

	//////////////////////////////////////////////////////////////////////////
	// Debug
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Debug") == 0) {
		stack->CorrectParams(0);

		if (Game->GetDebugMgr()->_enabled) {
			Game->GetDebugMgr()->OnScriptHitBreakpoint(script);
			script->Sleep(0);
		}
		stack->PushNULL();
	}

	//////////////////////////////////////////////////////////////////////////
	// ToString
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "ToString") == 0) {
		stack->CorrectParams(1);
		const char *Str = stack->Pop()->GetString();
		char *Str2 = new char[strlen(Str) + 1];
		strcpy(Str2, Str);
		stack->PushString(Str2);
		delete [] Str2;
	}

	//////////////////////////////////////////////////////////////////////////
	// ToInt
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "ToInt") == 0) {
		stack->CorrectParams(1);
		int Val = stack->Pop()->GetInt();
		stack->PushInt(Val);
	}

	//////////////////////////////////////////////////////////////////////////
	// ToFloat
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "ToFloat") == 0) {
		stack->CorrectParams(1);
		double Val = stack->Pop()->GetFloat();
		stack->PushFloat(Val);
	}

	//////////////////////////////////////////////////////////////////////////
	// ToBool
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "ToBool") == 0) {
		stack->CorrectParams(1);
		bool Val = stack->Pop()->GetBool();
		stack->PushBool(Val);
	}

	//////////////////////////////////////////////////////////////////////////
	// failure
	else {
		script->RuntimeError("Call to undefined function '%s'. Ignored.", name);
		stack->CorrectParams(0);
		stack->PushNULL();
	}

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::showCursor() {
	if (_cursorHidden) return S_OK;

	if (!_interactive && Game->_state == GAME_RUNNING) {
		if (_cursorNoninteractive) return DrawCursor(_cursorNoninteractive);
	} else {
		if (_activeObject && !FAILED(_activeObject->showCursor())) return S_OK;
		else {
			if (_activeObject && _activeCursor && _activeObject->getExtendedFlag("usable")) return DrawCursor(_activeCursor);
			else if (_cursor) return DrawCursor(_cursor);
		}
	}
	return E_FAIL;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::SaveGame(int slot, const char *desc, bool quickSave) {
	char Filename[MAX_PATH + 1];
	GetSaveSlotFilename(slot, Filename);

	LOG(0, "Saving game '%s'...", Filename);

	Game->applyEvent("BeforeSave", true);

	HRESULT ret;

	_indicatorDisplay = true;
	_indicatorProgress = 0;
	CBPersistMgr *pm = new CBPersistMgr(Game);
	if (FAILED(ret = pm->initSave(desc))) goto save_finish;

	if (!quickSave) {
		delete _saveLoadImage;
		_saveLoadImage = NULL;
		if (_saveImageName) {
			_saveLoadImage = new CBSurfaceSDL(this);

			if (!_saveLoadImage || FAILED(_saveLoadImage->create(_saveImageName, true, 0, 0, 0))) {
				delete _saveLoadImage;
				_saveLoadImage = NULL;
			}
		}
	}

	if (FAILED(ret = CSysClassRegistry::getInstance()->saveTable(Game, pm, quickSave))) goto save_finish;
	if (FAILED(ret = CSysClassRegistry::getInstance()->saveInstances(Game, pm, quickSave))) goto save_finish;
	if (FAILED(ret = pm->saveFile(Filename))) goto save_finish;

	_registry->WriteInt("System", "MostRecentSaveSlot", slot);

save_finish:
	delete pm;
	_indicatorDisplay = false;

	delete _saveLoadImage;
	_saveLoadImage = NULL;

	return ret;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::LoadGame(int Slot) {
	//Game->LOG(0, "Load start %d", CBUtils::GetUsedMemMB());

	_loading = false;
	_scheduledLoadSlot = -1;

	char Filename[MAX_PATH + 1];
	GetSaveSlotFilename(Slot, Filename);

	return LoadGame(Filename);
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::LoadGame(const char *Filename) {
	LOG(0, "Loading game '%s'...", Filename);
	GetDebugMgr()->OnGameShutdown();

	HRESULT ret;

	delete _saveLoadImage;
	_saveLoadImage = NULL;
	if (_loadImageName) {
		_saveLoadImage = new CBSurfaceSDL(this);

		if (!_saveLoadImage || FAILED(_saveLoadImage->create(_loadImageName, true, 0, 0, 0))) {
			delete _saveLoadImage;
			_saveLoadImage = NULL;
		}
	}


	_loadInProgress = true;
	_indicatorDisplay = true;
	_indicatorProgress = 0;
	CBPersistMgr *pm = new CBPersistMgr(Game);
	_dEBUG_AbsolutePathWarning = false;
	if (FAILED(ret = pm->initLoad(Filename))) goto load_finish;

	//if(FAILED(ret = cleanup())) goto load_finish;
	if (FAILED(ret = CSysClassRegistry::getInstance()->loadTable(Game, pm))) goto load_finish;
	if (FAILED(ret = CSysClassRegistry::getInstance()->loadInstances(Game, pm))) goto load_finish;

	// data initialization after load
	InitAfterLoad();

	Game->applyEvent("AfterLoad", true);

	DisplayContent(true, false);
	//_renderer->flip();

	GetDebugMgr()->OnGameInit();

load_finish:
	_dEBUG_AbsolutePathWarning = true;

	_indicatorDisplay = false;
	delete pm;
	_loadInProgress = false;

	delete _saveLoadImage;
	_saveLoadImage = NULL;

	//Game->LOG(0, "Load end %d", CBUtils::GetUsedMemMB());

	return ret;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::InitAfterLoad() {
	CSysClassRegistry::getInstance()->enumInstances(AfterLoadRegion,   "CBRegion",   NULL);
	CSysClassRegistry::getInstance()->enumInstances(AfterLoadSubFrame, "CBSubFrame", NULL);
	CSysClassRegistry::getInstance()->enumInstances(AfterLoadSound,    "CBSound",    NULL);
	CSysClassRegistry::getInstance()->enumInstances(AfterLoadFont,     "CBFontTT",   NULL);
	CSysClassRegistry::getInstance()->enumInstances(AfterLoadScript,   "CScScript",  NULL);

	_scEngine->RefreshScriptBreakpoints();
	if (_store) _store->afterLoad();

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
void CBGame::AfterLoadRegion(void *Region, void *Data) {
	((CBRegion *)Region)->CreateRegion();
}


//////////////////////////////////////////////////////////////////////////
void CBGame::AfterLoadSubFrame(void *Subframe, void *Data) {
	((CBSubFrame *)Subframe)->setSurfaceSimple();
}


//////////////////////////////////////////////////////////////////////////
void CBGame::AfterLoadSound(void *Sound, void *Data) {
	((CBSound *)Sound)->setSoundSimple();
}

//////////////////////////////////////////////////////////////////////////
void CBGame::AfterLoadFont(void *Font, void *Data) {
	((CBFont *)Font)->afterLoad();
}

//////////////////////////////////////////////////////////////////////////
void CBGame::AfterLoadScript(void *script, void *data) {
	((CScScript *)script)->afterLoad();
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::DisplayWindows(bool InGame) {
	HRESULT res;

	int i;

	// did we lose focus? focus topmost window
	if (_focusedWindow == NULL || !_focusedWindow->_visible || _focusedWindow->_disable) {
		_focusedWindow = NULL;
		for (i = _windows.GetSize() - 1; i >= 0; i--) {
			if (_windows[i]->_visible && !_windows[i]->_disable) {
				_focusedWindow = _windows[i];
				break;
			}
		}
	}

	// display all windows
	for (i = 0; i < _windows.GetSize(); i++) {
		if (_windows[i]->_visible && _windows[i]->_inGame == InGame) {

			res = _windows[i]->display();
			if (FAILED(res)) return res;
		}
	}

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::PlayMusic(int Channel, const char *Filename, bool Looping, uint32 LoopStart) {
	if (Channel >= NUM_MUSIC_CHANNELS) {
		Game->LOG(0, "**Error** Attempting to use music channel %d (max num channels: %d)", Channel, NUM_MUSIC_CHANNELS);
		return E_FAIL;
	}

	delete _music[Channel];
	_music[Channel] = NULL;

	_music[Channel] = new CBSound(Game);
	if (_music[Channel] && SUCCEEDED(_music[Channel]->setSound(Filename, SOUND_MUSIC, true))) {
		if (_musicStartTime[Channel]) {
			_music[Channel]->setPositionTime(_musicStartTime[Channel]);
			_musicStartTime[Channel] = 0;
		}
		if (LoopStart) _music[Channel]->setLoopStart(LoopStart);
		return _music[Channel]->play(Looping);
	} else {
		delete _music[Channel];
		_music[Channel] = NULL;
		return E_FAIL;
	}
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::StopMusic(int Channel) {
	if (Channel >= NUM_MUSIC_CHANNELS) {
		Game->LOG(0, "**Error** Attempting to use music channel %d (max num channels: %d)", Channel, NUM_MUSIC_CHANNELS);
		return E_FAIL;
	}

	if (_music[Channel]) {
		_music[Channel]->stop();
		delete _music[Channel];
		_music[Channel] = NULL;
		return S_OK;
	} else return E_FAIL;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::PauseMusic(int Channel) {
	if (Channel >= NUM_MUSIC_CHANNELS) {
		Game->LOG(0, "**Error** Attempting to use music channel %d (max num channels: %d)", Channel, NUM_MUSIC_CHANNELS);
		return E_FAIL;
	}

	if (_music[Channel]) return _music[Channel]->pause();
	else return E_FAIL;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::ResumeMusic(int Channel) {
	if (Channel >= NUM_MUSIC_CHANNELS) {
		Game->LOG(0, "**Error** Attempting to use music channel %d (max num channels: %d)", Channel, NUM_MUSIC_CHANNELS);
		return E_FAIL;
	}

	if (_music[Channel]) return _music[Channel]->resume();
	else return E_FAIL;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::SetMusicStartTime(int Channel, uint32 Time) {

	if (Channel >= NUM_MUSIC_CHANNELS) {
		Game->LOG(0, "**Error** Attempting to use music channel %d (max num channels: %d)", Channel, NUM_MUSIC_CHANNELS);
		return E_FAIL;
	}

	_musicStartTime[Channel] = Time;
	if (_music[Channel] && _music[Channel]->isPlaying()) return _music[Channel]->setPositionTime(Time);
	else return S_OK;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::LoadSettings(const char *Filename) {
	TOKEN_TABLE_START(commands)
	TOKEN_TABLE(SETTINGS)
	TOKEN_TABLE(GAME)
	TOKEN_TABLE(STRING_TABLE)
	TOKEN_TABLE(RESOLUTION)
	TOKEN_TABLE(REQUIRE_3D_ACCELERATION)
	TOKEN_TABLE(REQUIRE_SOUND)
	TOKEN_TABLE(HWTL_MODE)
	TOKEN_TABLE(ALLOW_WINDOWED_MODE)
	TOKEN_TABLE(ALLOW_ACCESSIBILITY_TAB)
	TOKEN_TABLE(ALLOW_ABOUT_TAB)
	TOKEN_TABLE(ALLOW_ADVANCED)
	TOKEN_TABLE(ALLOW_DESKTOP_RES)
	TOKEN_TABLE(REGISTRY_PATH)
	TOKEN_TABLE(RICH_SAVED_GAMES)
	TOKEN_TABLE(SAVED_GAME_EXT)
	TOKEN_TABLE(GUID)
	TOKEN_TABLE_END


	byte *OrigBuffer = Game->_fileManager->readWholeFile(Filename);
	if (OrigBuffer == NULL) {
		Game->LOG(0, "CBGame::LoadSettings failed for file '%s'", Filename);
		return E_FAIL;
	}

	HRESULT ret = S_OK;

	byte *Buffer = OrigBuffer;
	byte *params;
	int cmd;
	CBParser parser(Game);

	if (parser.GetCommand((char **)&Buffer, commands, (char **)&params) != TOKEN_SETTINGS) {
		Game->LOG(0, "'SETTINGS' keyword expected in game settings file.");
		return E_FAIL;
	}
	Buffer = params;
	while ((cmd = parser.GetCommand((char **)&Buffer, commands, (char **)&params)) > 0) {
		switch (cmd) {
		case TOKEN_GAME:
			delete[] _settingsGameFile;
			_settingsGameFile = new char[strlen((char *)params) + 1];
			if (_settingsGameFile) strcpy(_settingsGameFile, (char *)params);
			break;

		case TOKEN_STRING_TABLE:
			if (FAILED(_stringTable->loadFile((char *)params))) cmd = PARSERR_GENERIC;
			break;

		case TOKEN_RESOLUTION:
			parser.ScanStr((char *)params, "%d,%d", &_settingsResWidth, &_settingsResHeight);
			break;

		case TOKEN_REQUIRE_3D_ACCELERATION:
			parser.ScanStr((char *)params, "%b", &_settingsRequireAcceleration);
			break;

		case TOKEN_REQUIRE_SOUND:
			parser.ScanStr((char *)params, "%b", &_settingsRequireSound);
			break;

		case TOKEN_HWTL_MODE:
			parser.ScanStr((char *)params, "%d", &_settingsTLMode);
			break;

		case TOKEN_ALLOW_WINDOWED_MODE:
			parser.ScanStr((char *)params, "%b", &_settingsAllowWindowed);
			break;

		case TOKEN_ALLOW_DESKTOP_RES:
			parser.ScanStr((char *)params, "%b", &_settingsAllowDesktopRes);
			break;

		case TOKEN_ALLOW_ADVANCED:
			parser.ScanStr((char *)params, "%b", &_settingsAllowAdvanced);
			break;

		case TOKEN_ALLOW_ACCESSIBILITY_TAB:
			parser.ScanStr((char *)params, "%b", &_settingsAllowAccessTab);
			break;

		case TOKEN_ALLOW_ABOUT_TAB:
			parser.ScanStr((char *)params, "%b", &_settingsAllowAboutTab);
			break;

		case TOKEN_REGISTRY_PATH:
			_registry->SetBasePath((char *)params);
			break;

		case TOKEN_RICH_SAVED_GAMES:
			parser.ScanStr((char *)params, "%b", &_richSavedGames);
			break;

		case TOKEN_SAVED_GAME_EXT:
			CBUtils::SetString(&_savedGameExt, (char *)params);
			break;

		case TOKEN_GUID:
			break;
		}
	}
	if (cmd == PARSERR_TOKENNOTFOUND) {
		Game->LOG(0, "Syntax error in game settings '%s'", Filename);
		ret = E_FAIL;
	}
	if (cmd == PARSERR_GENERIC) {
		Game->LOG(0, "Error loading game settings '%s'", Filename);
		ret = E_FAIL;
	}

	_settingsAllowWindowed = _registry->ReadBool("Debug", "AllowWindowed", _settingsAllowWindowed);
	_compressedSavegames = _registry->ReadBool("Debug", "CompressedSavegames", _compressedSavegames);
	//_compressedSavegames = false;

	delete [] OrigBuffer;

	return ret;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::persist(CBPersistMgr *persistMgr) {
	if (!persistMgr->_saving) cleanup();

	CBObject::persist(persistMgr);

	persistMgr->transfer(TMEMBER(_activeObject));
	persistMgr->transfer(TMEMBER(_capturedObject));
	persistMgr->transfer(TMEMBER(_cursorNoninteractive));
	persistMgr->transfer(TMEMBER(_doNotExpandStrings));
	persistMgr->transfer(TMEMBER(_editorMode));
	persistMgr->transfer(TMEMBER(_fader));
	persistMgr->transfer(TMEMBER(_freezeLevel));
	persistMgr->transfer(TMEMBER(_focusedWindow));
	persistMgr->transfer(TMEMBER(_fontStorage));
	persistMgr->transfer(TMEMBER(_interactive));
	persistMgr->transfer(TMEMBER(_keyboardState));
	persistMgr->transfer(TMEMBER(_lastTime));
	persistMgr->transfer(TMEMBER(_mainObject));
	for (int i = 0; i < NUM_MUSIC_CHANNELS; i++) {
		persistMgr->transfer(TMEMBER(_music[i]));
		persistMgr->transfer(TMEMBER(_musicStartTime[i]));
	}

	persistMgr->transfer(TMEMBER(_offsetX));
	persistMgr->transfer(TMEMBER(_offsetY));
	persistMgr->transfer(TMEMBER(_offsetPercentX));
	persistMgr->transfer(TMEMBER(_offsetPercentY));

	persistMgr->transfer(TMEMBER(_origInteractive));
	persistMgr->transfer(TMEMBER_INT(_origState));
	persistMgr->transfer(TMEMBER(_personalizedSave));
	persistMgr->transfer(TMEMBER(_quitting));

	_regObjects.persist(persistMgr);

	persistMgr->transfer(TMEMBER(_scEngine));
	//persistMgr->transfer(TMEMBER(_soundMgr));
	persistMgr->transfer(TMEMBER_INT(_state));
	//persistMgr->transfer(TMEMBER(_surfaceStorage));
	persistMgr->transfer(TMEMBER(_subtitles));
	persistMgr->transfer(TMEMBER(_subtitlesSpeed));
	persistMgr->transfer(TMEMBER(_systemFont));
	persistMgr->transfer(TMEMBER(_videoFont));
	persistMgr->transfer(TMEMBER(_videoSubtitles));

	persistMgr->transfer(TMEMBER(_timer));
	persistMgr->transfer(TMEMBER(_timerDelta));
	persistMgr->transfer(TMEMBER(_timerLast));

	persistMgr->transfer(TMEMBER(_liveTimer));
	persistMgr->transfer(TMEMBER(_liveTimerDelta));
	persistMgr->transfer(TMEMBER(_liveTimerLast));

	persistMgr->transfer(TMEMBER(_musicCrossfadeRunning));
	persistMgr->transfer(TMEMBER(_musicCrossfadeStartTime));
	persistMgr->transfer(TMEMBER(_musicCrossfadeLength));
	persistMgr->transfer(TMEMBER(_musicCrossfadeChannel1));
	persistMgr->transfer(TMEMBER(_musicCrossfadeChannel2));
	persistMgr->transfer(TMEMBER(_musicCrossfadeSwap));

	persistMgr->transfer(TMEMBER(_loadImageName));
	persistMgr->transfer(TMEMBER(_saveImageName));
	persistMgr->transfer(TMEMBER(_saveImageX));
	persistMgr->transfer(TMEMBER(_saveImageY));
	persistMgr->transfer(TMEMBER(_loadImageX));
	persistMgr->transfer(TMEMBER(_loadImageY));

	persistMgr->transfer(TMEMBER_INT(_textEncoding));
	persistMgr->transfer(TMEMBER(_textRTL));

	persistMgr->transfer(TMEMBER(_soundBufferSizeSec));
	persistMgr->transfer(TMEMBER(_suspendedRendering));

	persistMgr->transfer(TMEMBER(_mouseLockRect));

	_windows.persist(persistMgr);

	persistMgr->transfer(TMEMBER(_suppressScriptErrors));
	persistMgr->transfer(TMEMBER(_autorunDisabled));

	persistMgr->transfer(TMEMBER(_autoSaveOnExit));
	persistMgr->transfer(TMEMBER(_autoSaveSlot));
	persistMgr->transfer(TMEMBER(_cursorHidden));

	if (persistMgr->checkVersion(1, 0, 1))
		persistMgr->transfer(TMEMBER(_store));
	else
		_store = NULL;

	if (!persistMgr->_saving) _quitting = false;

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::FocusWindow(CUIWindow *Window) {
	CUIWindow *Prev = _focusedWindow;

	int i;
	for (i = 0; i < _windows.GetSize(); i++) {
		if (_windows[i] == Window) {
			if (i < _windows.GetSize() - 1) {
				_windows.RemoveAt(i);
				_windows.Add(Window);

				Game->_focusedWindow = Window;
			}

			if (Window->_mode == WINDOW_NORMAL && Prev != Window && Game->ValidObject(Prev) && (Prev->_mode == WINDOW_EXCLUSIVE || Prev->_mode == WINDOW_SYSTEM_EXCLUSIVE))
				return FocusWindow(Prev);
			else return S_OK;
		}
	}
	return E_FAIL;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::Freeze(bool IncludingMusic) {
	if (_freezeLevel == 0) {
		_scEngine->PauseAll();
		_soundMgr->pauseAll(IncludingMusic);
		_origState = _state;
		_origInteractive = _interactive;
		_interactive = true;
	}
	_state = GAME_FROZEN;
	_freezeLevel++;

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::Unfreeze() {
	if (_freezeLevel == 0) return S_OK;

	_freezeLevel--;
	if (_freezeLevel == 0) {
		_state = _origState;
		_interactive = _origInteractive;
		_scEngine->ResumeAll();
		_soundMgr->resumeAll();
	}

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
bool CBGame::handleKeypress(Common::Event *event, bool printable) {
	if (IsVideoPlaying()) {
		if (event->kbd.keycode == Common::KEYCODE_ESCAPE)
			StopVideo();
		return true;
	}
#ifdef __WIN32__
	// TODO: Do we really need to handle this in-engine?
	// handle Alt+F4 on windows
	if (event->type == Common::EVENT_KEYDOWN && event->kbd.keycode == Common::KEYCODE_F4 && (event->kbd.flags == Common::KBD_ALT)) {
		OnWindowClose();
		return true;
		//TODO
	}
#endif

	if (event->type == Common::EVENT_KEYDOWN && event->kbd.keycode == Common::KEYCODE_RETURN && (event->kbd.flags == Common::KBD_ALT)) {
		// TODO: Handle alt-enter as well as alt-return.
		_renderer->switchFullscreen();
		return true;
	}


	_keyboardState->handleKeyPress(event);
	_keyboardState->ReadKey(event);
// TODO

	if (_focusedWindow) {
		if (!Game->_focusedWindow->handleKeypress(event, _keyboardState->_currentPrintable)) {
			/*if (event->type != SDL_TEXTINPUT) {*/
			if (Game->_focusedWindow->canHandleEvent("Keypress"))
				Game->_focusedWindow->applyEvent("Keypress");
			else
				applyEvent("Keypress");
			/*}*/
		}
		return true;
	} else { /*if (event->type != SDL_TEXTINPUT)*/
		applyEvent("Keypress");
		return true;
	} //else return true;

	return false;
}

void CBGame::handleKeyRelease(Common::Event *event) {
	_keyboardState->handleKeyRelease(event);
}


//////////////////////////////////////////////////////////////////////////
bool CBGame::handleMouseWheel(int Delta) {
	bool Handled = false;
	if (_focusedWindow) {
		Handled = Game->_focusedWindow->handleMouseWheel(Delta);

		if (!Handled) {
			if (Delta < 0 && Game->_focusedWindow->canHandleEvent("MouseWheelDown")) {
				Game->_focusedWindow->applyEvent("MouseWheelDown");
				Handled = true;
			} else if (Game->_focusedWindow->canHandleEvent("MouseWheelUp")) {
				Game->_focusedWindow->applyEvent("MouseWheelUp");
				Handled = true;
			}

		}
	}

	if (!Handled) {
		if (Delta < 0) {
			applyEvent("MouseWheelDown");
		} else {
			applyEvent("MouseWheelUp");
		}
	}

	return true;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::GetVersion(byte  *VerMajor, byte *VerMinor, byte *ExtMajor, byte *ExtMinor) {
	if (VerMajor) *VerMajor = DCGF_VER_MAJOR;
	if (VerMinor) *VerMinor = DCGF_VER_MINOR;

	if (ExtMajor) *ExtMajor = 0;
	if (ExtMinor) *ExtMinor = 0;

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
void CBGame::SetWindowTitle() {
	if (_renderer) {
		char Title[512];
		strcpy(Title, _caption[0]);
		if (Title[0] != '\0') strcat(Title, " - ");
		strcat(Title, "WME Lite");


		Utf8String title;
		if (_textEncoding == TEXT_UTF8) {
			title = Utf8String(Title);
		} else {
			warning("CBGame::SetWindowTitle -Ignoring textencoding");
			title = Utf8String(Title);
			/*          WideString wstr = StringUtil::AnsiToWide(Title);
			            title = StringUtil::WideToUtf8(wstr);*/
		}
#if 0
		CBRenderSDL *renderer = static_cast<CBRenderSDL *>(_renderer);
		// TODO

		SDL_SetWindowTitle(renderer->GetSdlWindow(), title.c_str());
#endif
	}
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::GetSaveSlotFilename(int Slot, char *Buffer) {
	AnsiString dataDir = GetDataDir();
	//sprintf(Buffer, "%s/save%03d.%s", dataDir.c_str(), Slot, _savedGameExt);
	sprintf(Buffer, "save%03d.%s", Slot, _savedGameExt);
	warning("Saving %s - we really should prefix these things to avoid collisions.", Buffer);
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
AnsiString CBGame::GetDataDir() {
	AnsiString userDir = PathUtil::GetUserDirectory();
#ifdef __IPHONEOS__
	return userDir;
#else
	AnsiString baseDir = _registry->GetBasePath();
	return PathUtil::Combine(userDir, baseDir);
#endif
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::GetSaveSlotDescription(int Slot, char *Buffer) {
	Buffer[0] = '\0';

	char Filename[MAX_PATH + 1];
	GetSaveSlotFilename(Slot, Filename);
	CBPersistMgr *pm = new CBPersistMgr(Game);
	if (!pm) return E_FAIL;

	_dEBUG_AbsolutePathWarning = false;
	if (FAILED(pm->initLoad(Filename))) {
		_dEBUG_AbsolutePathWarning = true;
		delete pm;
		return E_FAIL;
	}

	_dEBUG_AbsolutePathWarning = true;
	strcpy(Buffer, pm->_savedDescription);
	delete pm;

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
bool CBGame::IsSaveSlotUsed(int Slot) {
	char Filename[MAX_PATH + 1];
	GetSaveSlotFilename(Slot, Filename);

	warning("CBGame::IsSaveSlotUsed(%d) - FIXME, ugly solution", Slot);
	Common::SeekableReadStream *File = g_wintermute->getSaveFileMan()->openForLoading(Filename);
	if (!File) return false;
	delete File;
	return true;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::EmptySaveSlot(int Slot) {
	char Filename[MAX_PATH + 1];
	GetSaveSlotFilename(Slot, Filename);

	CBPlatform::DeleteFile(Filename);

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::SetActiveObject(CBObject *Obj) {
	// not-active when game is frozen
	if (Obj && !Game->_interactive && !Obj->_nonIntMouseEvents) {
		Obj = NULL;
	}

	if (Obj == _activeObject) return S_OK;

	if (_activeObject) _activeObject->applyEvent("MouseLeave");
	//if(ValidObject(_activeObject)) _activeObject->applyEvent("MouseLeave");
	_activeObject = Obj;
	if (_activeObject) {
		_activeObject->applyEvent("MouseEntry");
	}

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::PushViewport(CBViewport *Viewport) {
	_viewportSP++;
	if (_viewportSP >= _viewportStack.GetSize()) _viewportStack.Add(Viewport);
	else _viewportStack[_viewportSP] = Viewport;

	_renderer->setViewport(Viewport->getRect());

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::PopViewport() {
	_viewportSP--;
	if (_viewportSP < -1) Game->LOG(0, "Fatal: Viewport stack underflow!");

	if (_viewportSP >= 0 && _viewportSP < _viewportStack.GetSize()) _renderer->setViewport(_viewportStack[_viewportSP]->getRect());
	else _renderer->setViewport(_renderer->_drawOffsetX,
		                            _renderer->_drawOffsetY,
		                            _renderer->_width + _renderer->_drawOffsetX,
		                            _renderer->_height + _renderer->_drawOffsetY);

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::GetCurrentViewportRect(RECT *Rect, bool *Custom) {
	if (Rect == NULL) return E_FAIL;
	else {
		if (_viewportSP >= 0) {
			CBPlatform::CopyRect(Rect, _viewportStack[_viewportSP]->getRect());
			if (Custom) *Custom = true;
		} else {
			CBPlatform::SetRect(Rect,   _renderer->_drawOffsetX,
			                    _renderer->_drawOffsetY,
			                    _renderer->_width + _renderer->_drawOffsetX,
			                    _renderer->_height + _renderer->_drawOffsetY);
			if (Custom) *Custom = false;
		}

		return S_OK;
	}
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::GetCurrentViewportOffset(int *OffsetX, int *OffsetY) {
	if (_viewportSP >= 0) {
		if (OffsetX) *OffsetX = _viewportStack[_viewportSP]->_offsetX;
		if (OffsetY) *OffsetY = _viewportStack[_viewportSP]->_offsetY;
	} else {
		if (OffsetX) *OffsetX = 0;
		if (OffsetY) *OffsetY = 0;
	}

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::WindowLoadHook(CUIWindow *Win, char **Buf, char **Params) {
	return E_FAIL;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::WindowScriptMethodHook(CUIWindow *Win, CScScript *script, CScStack *stack, const char *name) {
	return E_FAIL;
}


//////////////////////////////////////////////////////////////////////////
void CBGame::SetInteractive(bool State) {
	_interactive = State;
	if (_transMgr) _transMgr->_origInteractive = State;
}


//////////////////////////////////////////////////////////////////////////
void CBGame::ResetMousePos() {
	POINT p;
	p.x = _mousePos.x + _renderer->_drawOffsetX;
	p.y = _mousePos.y + _renderer->_drawOffsetY;

	CBPlatform::SetCursorPos(p.x, p.y);
}


//////////////////////////////////////////////////////////////////////////
void CBGame::SetResourceModule(HMODULE ResModule) {
	_resourceModule = ResModule;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::DisplayContent(bool update, bool displayAll) {
	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::DisplayContentSimple() {
	// fill black
	_renderer->fill(0, 0, 0);
	if (_indicatorDisplay) DisplayIndicator();

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::DisplayIndicator() {
	if (_saveLoadImage) {
		RECT rc;
		CBPlatform::SetRect(&rc, 0, 0, _saveLoadImage->getWidth(), _saveLoadImage->getHeight());
		if (_loadInProgress) _saveLoadImage->displayTrans(_loadImageX, _loadImageY, rc);
		else _saveLoadImage->displayTrans(_saveImageX, _saveImageY, rc);
	}

	if ((!_indicatorDisplay && _indicatorWidth <= 0) || _indicatorHeight <= 0) return S_OK;
	_renderer->setupLines();
	for (int i = 0; i < _indicatorHeight; i++)
		_renderer->drawLine(_indicatorX, _indicatorY + i, _indicatorX + (int)(_indicatorWidth * (float)((float)_indicatorProgress / 100.0f)), _indicatorY + i, _indicatorColor);

	_renderer->setup2D();
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::UpdateMusicCrossfade() {
	/* byte GlobMusicVol = _soundMgr->getVolumePercent(SOUND_MUSIC); */

	if (!_musicCrossfadeRunning) return S_OK;
	if (_state == GAME_FROZEN) return S_OK;

	if (_musicCrossfadeChannel1 < 0 || _musicCrossfadeChannel1 >= NUM_MUSIC_CHANNELS || !_music[_musicCrossfadeChannel1]) {
		_musicCrossfadeRunning = false;
		return S_OK;
	}
	if (_musicCrossfadeChannel2 < 0 || _musicCrossfadeChannel2 >= NUM_MUSIC_CHANNELS || !_music[_musicCrossfadeChannel2]) {
		_musicCrossfadeRunning = false;
		return S_OK;
	}

	if (!_music[_musicCrossfadeChannel1]->isPlaying()) _music[_musicCrossfadeChannel1]->play();
	if (!_music[_musicCrossfadeChannel2]->isPlaying()) _music[_musicCrossfadeChannel2]->play();

	uint32 CurrentTime = Game->_liveTimer - _musicCrossfadeStartTime;

	if (CurrentTime >= _musicCrossfadeLength) {
		_musicCrossfadeRunning = false;
		//_music[_musicCrossfadeChannel2]->setVolume(GlobMusicVol);
		_music[_musicCrossfadeChannel2]->setVolume(100);

		_music[_musicCrossfadeChannel1]->stop();
		//_music[_musicCrossfadeChannel1]->setVolume(GlobMusicVol);
		_music[_musicCrossfadeChannel1]->setVolume(100);


		if (_musicCrossfadeSwap) {
			// swap channels
			CBSound *Dummy = _music[_musicCrossfadeChannel1];
			int DummyInt = _musicStartTime[_musicCrossfadeChannel1];

			_music[_musicCrossfadeChannel1] = _music[_musicCrossfadeChannel2];
			_musicStartTime[_musicCrossfadeChannel1] = _musicStartTime[_musicCrossfadeChannel2];

			_music[_musicCrossfadeChannel2] = Dummy;
			_musicStartTime[_musicCrossfadeChannel2] = DummyInt;
		}
	} else {
		//_music[_musicCrossfadeChannel1]->setVolume(GlobMusicVol - (float)CurrentTime / (float)_musicCrossfadeLength * GlobMusicVol);
		//_music[_musicCrossfadeChannel2]->setVolume((float)CurrentTime / (float)_musicCrossfadeLength * GlobMusicVol);
		_music[_musicCrossfadeChannel1]->setVolume(100 - (float)CurrentTime / (float)_musicCrossfadeLength * 100);
		_music[_musicCrossfadeChannel2]->setVolume((float)CurrentTime / (float)_musicCrossfadeLength * 100);

		//Game->QuickMessageForm("%d %d", _music[_musicCrossfadeChannel1]->GetVolume(), _music[_musicCrossfadeChannel2]->GetVolume());
	}

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::ResetContent() {
	_scEngine->ClearGlobals();
	//_timer = 0;
	//_liveTimer = 0;

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
void CBGame::DEBUG_DumpClassRegistry() {
	warning("DEBUG_DumpClassRegistry - untested");
	Common::DumpFile *f = new Common::DumpFile;
	f->open("zz_class_reg_dump.log");

	CSysClassRegistry::getInstance()->dumpClasses(f);

	f->close();
	delete f;
	Game->QuickMessage("Classes dump completed.");
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::invalidateDeviceObjects() {
	for (int i = 0; i < _regObjects.GetSize(); i++) {
		_regObjects[i]->invalidateDeviceObjects();
	}
	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::restoreDeviceObjects() {
	for (int i = 0; i < _regObjects.GetSize(); i++) {
		_regObjects[i]->restoreDeviceObjects();
	}
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::SetWaitCursor(const char *Filename) {
	delete _cursorNoninteractive;
	_cursorNoninteractive = NULL;

	_cursorNoninteractive = new CBSprite(Game);
	if (!_cursorNoninteractive || FAILED(_cursorNoninteractive->loadFile(Filename))) {
		delete _cursorNoninteractive;
		_cursorNoninteractive = NULL;
		return E_FAIL;
	} else return S_OK;
}

//////////////////////////////////////////////////////////////////////////
bool CBGame::IsVideoPlaying() {
	if (_videoPlayer->isPlaying()) return true;
	if (_theoraPlayer && _theoraPlayer->isPlaying()) return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::StopVideo() {
	if (_videoPlayer->isPlaying()) _videoPlayer->stop();
	if (_theoraPlayer && _theoraPlayer->isPlaying()) {
		_theoraPlayer->stop();
		delete _theoraPlayer;
		_theoraPlayer = NULL;
	}
	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::DrawCursor(CBSprite *Cursor) {
	if (!Cursor) return E_FAIL;
	if (Cursor != _lastCursor) {
		Cursor->Reset();
		_lastCursor = Cursor;
	}
	return Cursor->Draw(_mousePos.x, _mousePos.y);
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::OnActivate(bool Activate, bool RefreshMouse) {
	if (_shuttingDown || !_renderer) return S_OK;

	_renderer->_active = Activate;

	if (RefreshMouse) {
		POINT p;
		GetMousePos(&p);
		SetActiveObject(_renderer->getObjectAt(p.x, p.y));
	}

	if (Activate) _soundMgr->resumeAll();
	else _soundMgr->pauseAll();

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::OnMouseLeftDown() {
	if (_activeObject) _activeObject->handleMouse(MOUSE_CLICK, MOUSE_BUTTON_LEFT);

	bool Handled = _state == GAME_RUNNING && SUCCEEDED(applyEvent("LeftClick"));
	if (!Handled) {
		if (_activeObject != NULL) {
			_activeObject->applyEvent("LeftClick");
		}
	}

	if (_activeObject != NULL) _capturedObject = _activeObject;
	_mouseLeftDown = true;
	CBPlatform::SetCapture(_renderer->_window);

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::OnMouseLeftUp() {
	if (_activeObject) _activeObject->handleMouse(MOUSE_RELEASE, MOUSE_BUTTON_LEFT);

	CBPlatform::ReleaseCapture();
	_capturedObject = NULL;
	_mouseLeftDown = false;

	bool Handled = _state == GAME_RUNNING && SUCCEEDED(applyEvent("LeftRelease"));
	if (!Handled) {
		if (_activeObject != NULL) {
			_activeObject->applyEvent("LeftRelease");
		}
	}
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::OnMouseLeftDblClick() {
	if (_state == GAME_RUNNING && !_interactive) return S_OK;

	if (_activeObject) _activeObject->handleMouse(MOUSE_DBLCLICK, MOUSE_BUTTON_LEFT);

	bool Handled = _state == GAME_RUNNING && SUCCEEDED(applyEvent("LeftDoubleClick"));
	if (!Handled) {
		if (_activeObject != NULL) {
			_activeObject->applyEvent("LeftDoubleClick");
		}
	}
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::OnMouseRightDblClick() {
	if (_state == GAME_RUNNING && !_interactive) return S_OK;

	if (_activeObject) _activeObject->handleMouse(MOUSE_DBLCLICK, MOUSE_BUTTON_RIGHT);

	bool Handled = _state == GAME_RUNNING && SUCCEEDED(applyEvent("RightDoubleClick"));
	if (!Handled) {
		if (_activeObject != NULL) {
			_activeObject->applyEvent("RightDoubleClick");
		}
	}
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::OnMouseRightDown() {
	if (_activeObject) _activeObject->handleMouse(MOUSE_CLICK, MOUSE_BUTTON_RIGHT);

	bool Handled = _state == GAME_RUNNING && SUCCEEDED(applyEvent("RightClick"));
	if (!Handled) {
		if (_activeObject != NULL) {
			_activeObject->applyEvent("RightClick");
		}
	}
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::OnMouseRightUp() {
	if (_activeObject) _activeObject->handleMouse(MOUSE_RELEASE, MOUSE_BUTTON_RIGHT);

	bool Handled = _state == GAME_RUNNING && SUCCEEDED(applyEvent("RightRelease"));
	if (!Handled) {
		if (_activeObject != NULL) {
			_activeObject->applyEvent("RightRelease");
		}
	}
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::OnMouseMiddleDown() {
	if (_state == GAME_RUNNING && !_interactive) return S_OK;

	if (_activeObject) _activeObject->handleMouse(MOUSE_CLICK, MOUSE_BUTTON_MIDDLE);

	bool Handled = _state == GAME_RUNNING && SUCCEEDED(applyEvent("MiddleClick"));
	if (!Handled) {
		if (_activeObject != NULL) {
			_activeObject->applyEvent("MiddleClick");
		}
	}
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::OnMouseMiddleUp() {
	if (_activeObject) _activeObject->handleMouse(MOUSE_RELEASE, MOUSE_BUTTON_MIDDLE);

	bool Handled = _state == GAME_RUNNING && SUCCEEDED(applyEvent("MiddleRelease"));
	if (!Handled) {
		if (_activeObject != NULL) {
			_activeObject->applyEvent("MiddleRelease");
		}
	}
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::OnPaint() {
	if (_renderer && _renderer->_windowed && _renderer->_ready) {
		_renderer->initLoop();
		DisplayContent(false, true);
		DisplayDebugInfo();
		_renderer->windowedBlt();
	}
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::OnWindowClose() {
	if (canHandleEvent("QuitGame")) {
		if (_state != GAME_FROZEN) Game->applyEvent("QuitGame");
		return S_OK;
	} else return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::DisplayDebugInfo() {
	char str[100];

	if (_dEBUG_ShowFPS) {
		sprintf(str, "FPS: %d", Game->_fps);
		_systemFont->drawText((byte *)str, 0, 0, 100, TAL_LEFT);
	}

	if (Game->_dEBUG_DebugMode) {
		if (!Game->_renderer->_windowed)
			sprintf(str, "Mode: %dx%dx%d", _renderer->_width, _renderer->_height, _renderer->_bPP);
		else
			sprintf(str, "Mode: %dx%d windowed", _renderer->_width, _renderer->_height);

		strcat(str, " (");
		strcat(str, _renderer->getName());
		strcat(str, ")");
		_systemFont->drawText((byte *)str, 0, 0, _renderer->_width, TAL_RIGHT);

		_renderer->displayDebugInfo();

		int ScrTotal, ScrRunning, ScrWaiting, ScrPersistent;
		ScrTotal = _scEngine->GetNumScripts(&ScrRunning, &ScrWaiting, &ScrPersistent);
		sprintf(str, "Running scripts: %d (r:%d w:%d p:%d)", ScrTotal, ScrRunning, ScrWaiting, ScrPersistent);
		_systemFont->drawText((byte *)str, 0, 70, _renderer->_width, TAL_RIGHT);


		sprintf(str, "Timer: %d", _timer);
		Game->_systemFont->drawText((byte *)str, 0, 130, _renderer->_width, TAL_RIGHT);

		if (_activeObject != NULL) _systemFont->drawText((byte *)_activeObject->_name, 0, 150, _renderer->_width, TAL_RIGHT);

		sprintf(str, "GfxMem: %dMB", _usedMem / (1024 * 1024));
		_systemFont->drawText((byte *)str, 0, 170, _renderer->_width, TAL_RIGHT);

	}

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
CBDebugger *CBGame::GetDebugMgr() {
	if (!_debugMgr) _debugMgr = new CBDebugger(this);
	return _debugMgr;
}


//////////////////////////////////////////////////////////////////////////
void CBGame::GetMousePos(POINT *Pos) {
	CBPlatform::GetCursorPos(Pos);

	Pos->x -= _renderer->_drawOffsetX;
	Pos->y -= _renderer->_drawOffsetY;

	/*
	// Windows can squish maximized window if it's larger than desktop
	// so we need to modify mouse position appropriately (tnx mRax)
	if (_renderer->_windowed && ::IsZoomed(_renderer->_window)) {
	    RECT rc;
	    ::GetClientRect(_renderer->_window, &rc);
	    Pos->x *= Game->_renderer->_realWidth;
	    Pos->x /= (rc.right - rc.left);
	    Pos->y *= Game->_renderer->_realHeight;
	    Pos->y /= (rc.bottom - rc.top);
	}
	*/

	if (_mouseLockRect.left != 0 && _mouseLockRect.right != 0 && _mouseLockRect.top != 0 && _mouseLockRect.bottom != 0) {
		if (!CBPlatform::PtInRect(&_mouseLockRect, *Pos)) {
			Pos->x = MAX(_mouseLockRect.left, Pos->x);
			Pos->y = MAX(_mouseLockRect.top, Pos->y);

			Pos->x = MIN(_mouseLockRect.right, Pos->x);
			Pos->y = MIN(_mouseLockRect.bottom, Pos->y);

			POINT NewPos = *Pos;

			NewPos.x += _renderer->_drawOffsetX;
			NewPos.y += _renderer->_drawOffsetY;

			CBPlatform::SetCursorPos(NewPos.x, NewPos.y);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::MiniUpdate() {
	if (!_miniUpdateEnabled) return S_OK;

	if (CBPlatform::GetTime() - _lastMiniUpdate > 200) {
		if (_soundMgr) _soundMgr->initLoop();
		_lastMiniUpdate = CBPlatform::GetTime();
	}
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
HRESULT CBGame::OnScriptShutdown(CScScript *script) {
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
bool CBGame::IsLeftDoubleClick() {
	return IsDoubleClick(0);
}

//////////////////////////////////////////////////////////////////////////
bool CBGame::IsRightDoubleClick() {
	return IsDoubleClick(1);
}

//////////////////////////////////////////////////////////////////////////
bool CBGame::IsDoubleClick(int buttonIndex) {
	uint32 maxDoubleCLickTime = 500;
	int maxMoveX = 4;
	int maxMoveY = 4;

#if __IPHONEOS__
	maxMoveX = 16;
	maxMoveY = 16;
#endif

	POINT pos;
	CBPlatform::GetCursorPos(&pos);

	int moveX = abs(pos.x - _lastClick[buttonIndex].PosX);
	int moveY = abs(pos.y - _lastClick[buttonIndex].PosY);


	if (_lastClick[buttonIndex].Time == 0 || CBPlatform::GetTime() - _lastClick[buttonIndex].Time > maxDoubleCLickTime || moveX > maxMoveX || moveY > maxMoveY) {
		_lastClick[buttonIndex].Time = CBPlatform::GetTime();
		_lastClick[buttonIndex].PosX = pos.x;
		_lastClick[buttonIndex].PosY = pos.y;
		return false;
	} else {
		_lastClick[buttonIndex].Time = 0;
		return true;
	}
}

//////////////////////////////////////////////////////////////////////////
void CBGame::AutoSaveOnExit() {
	_soundMgr->saveSettings();
	_registry->SaveValues();

	if (!_autoSaveOnExit) return;
	if (_state == GAME_FROZEN) return;

	SaveGame(_autoSaveSlot, "autosave", true);
}

//////////////////////////////////////////////////////////////////////////
void CBGame::AddMem(int bytes) {
	_usedMem += bytes;
}

//////////////////////////////////////////////////////////////////////////
AnsiString CBGame::GetDeviceType() const {
#ifdef __IPHONEOS__
	char devType[128];
	IOS_GetDeviceType(devType);
	return AnsiString(devType);
#else
	return "computer";
#endif
}

} // end of namespace WinterMute
