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
 * $URL$
 * $Id$
 *
 */


#include "common/config-manager.h"
#include "common/endian.h"
#include "common/file.h"
#include "common/fs.h"
#include "common/func.h"

#include "gui/message.h"

#include "kyra/resource.h"

#define INS_CACHE_THRESHOLD		300000	// all files with file size greater than this will be cached

namespace Kyra {

Resource::Resource(KyraEngine_v1 *vm) : _loaders(), _map(), _vm(vm) {
	initializeLoaders();
}

Resource::~Resource() {
	_map.clear();
	_loaders.clear();
}

bool Resource::reset() {
	unloadAllPakFiles();

	FilesystemNode dir(ConfMan.get("path"));

	if (!dir.exists() || !dir.isDirectory())
		error("invalid game path '%s'", dir.getPath().c_str());

	if (!loadPakFile(StaticResource::staticDataFilename()) || !StaticResource::checkKyraDat()) {
		Common::String errorMessage = "You're missing the '" + StaticResource::staticDataFilename() + "' file or it got corrupted, (re)get it from the ScummVM website";
		::GUI::MessageDialog errorMsg(errorMessage);
		errorMsg.runModal();
		error(errorMessage.c_str());
	}

	if (_vm->game() == GI_KYRA1) {
		// We only need kyra.dat for the demo.
		if (_vm->gameFlags().isDemo)
			return true;

		// only VRM file we need in the *whole* game for kyra1
		if (_vm->gameFlags().isTalkie)
			loadPakFile("CHAPTER1.VRM");
	} else if (_vm->game() == GI_KYRA2) {
		if (_vm->gameFlags().useInstallerPackage)
			loadPakFile("WESTWOOD.001");

		// mouse pointer, fonts, etc. required for initializing
		if (_vm->gameFlags().isDemo && !_vm->gameFlags().isTalkie) {
			loadPakFile("GENERAL.PAK");
		} else {
			if (_vm->gameFlags().isTalkie) {
				// Add default file directories
				Common::File::addDefaultDirectory(ConfMan.get("path") + "hof_cd");
				Common::File::addDefaultDirectory(ConfMan.get("path") + "HOF_CD");
			}

			loadPakFile("INTROGEN.PAK");
			loadPakFile("OTHER.PAK");
		}

		return true;
	} else if (_vm->game() == GI_KYRA3) {
		if (_vm->gameFlags().useInstallerPackage)
			loadPakFile("WESTWOOD.001");
		// Add default file directories
		Common::File::addDefaultDirectory(ConfMan.get("path") + "malcolm");
		Common::File::addDefaultDirectory(ConfMan.get("path") + "MALCOLM");

		loadFileList("FILEDATA.FDT");

		return true;
	}

	FSList fslist;
	if (!dir.getChildren(fslist, FilesystemNode::kListFilesOnly))
		error("can't list files inside game path '%s'", dir.getPath().c_str());

	if (_vm->game() == GI_KYRA1 && _vm->gameFlags().isTalkie) {
		static const char *list[] = {
			"ADL.PAK", "CHAPTER1.VRM", "COL.PAK", "FINALE.PAK", "INTRO1.PAK", "INTRO2.PAK",
			"INTRO3.PAK", "INTRO4.PAK", "MISC.PAK",	"SND.PAK", "STARTUP.PAK", "XMI.PAK",
			"CAVE.APK", "DRAGON1.APK", "DRAGON2.APK", "LAGOON.APK"
		};

		Common::for_each(list, list + ARRAYSIZE(list), Common::bind1st(Common::mem_fun(&Resource::loadPakFile), this));

		for (int i = 0; i < ARRAYSIZE(list); ++i) {
			ResFileMap::iterator iterator = _map.find(list[i]);
			if (iterator != _map.end())
				iterator->_value.prot = true;
		}
	} else {
		for (FSList::const_iterator file = fslist.begin(); file != fslist.end(); ++file) {
			Common::String filename = file->getName();
			filename.toUppercase();

			// No real PAK file!
			if (filename == "TWMUSIC.PAK")
				continue;

			if (filename == ((_vm->gameFlags().lang == Common::EN_ANY) ? "JMC.PAK" : "EMC.PAK"))
				continue;

			if (filename.hasSuffix(".PAK") || filename.hasSuffix(".APK")) {
				if (!loadPakFile(file->getName()))
					error("couldn't open pakfile '%s'", file->getName().c_str());
			}
		}
	}

	return true;
}

bool Resource::loadPakFile(const Common::String &filename) {
	if (!isAccessable(filename))
		return false;

	ResFileMap::iterator iter = _map.find(filename);
	if (iter == _map.end())
		return false;

	if (iter->_value.preload) {
		iter->_value.mounted = true;
		return true;
	}

	const ResArchiveLoader *loader = getLoader(iter->_value.type);
	if (!loader) {
		error("no archive loader for file '%s' found which is of type %d", filename.c_str(), iter->_value.type);
		return false;
	}

	Common::SeekableReadStream *stream = getFileStream(filename);
	if (!stream) {
		error("archive file '%s' not found", filename.c_str());
		return false;
	}

	iter->_value.mounted = true;
	iter->_value.preload = true;
	ResArchiveLoader::FileList files;
	loader->loadFile(filename, *stream, files);
	delete stream;
	stream = 0;
	
	for (ResArchiveLoader::FileList::iterator i = files.begin(); i != files.end(); ++i) {
		iter = _map.find(i->filename);
		if (iter == _map.end()) {
			// A new file entry, so we just insert it into the file map.
			_map[i->filename] = i->entry;
		} else if (!iter->_value.parent.empty()) {
			if (!iter->_value.parent.equalsIgnoreCase(filename)) {
				ResFileMap::iterator oldParent = _map.find(iter->_value.parent);
				if (oldParent != _map.end()) {
					// Protected files and their embedded file entries do not get overwritten.
					if (!oldParent->_value.prot) {
						// If the old parent is not protected we mark it as not preload anymore,
						// since now no longer all of its embedded files are in the filemap.
						oldParent->_value.preload = false;
						_map[i->filename] = i->entry;
					}
				} else {
					// Old parent not found? That's strange... But we just overwrite the old
					// entry.
					_map[i->filename] = i->entry;
				}
			} else {
				// The old parent has the same filenames as the new archive, we are sure and overwrite the
				// old file entry, could be afterall that the preload flag of the new archive was
				// just unflagged.
				_map[i->filename] = i->entry;
			}
		}
		// 'else' case would mean here overwriting an existing file entry in the map without parent.
		// We don't support that though, so one can overwrite files from archives by putting
		// them in the gamepath.
	}

	detectFileTypes();
	return true;
}

bool Resource::loadFileList(const Common::String &filedata) {
	Common::File f;

	if (!f.open(filedata))
		return false;

	uint32 filenameOffset = 0;
	while ((filenameOffset = f.readUint32LE()) != 0) {
		uint32 offset = f.pos();
		f.seek(filenameOffset, SEEK_SET);

		uint8 buffer[13];
		f.read(buffer, sizeof(buffer)-1);
		buffer[12] = 0;
		f.seek(offset + 16, SEEK_SET);

		Common::String filename = (char*)buffer;
		filename.toUppercase();

		if (filename.hasSuffix(".PAK")) {
			if (!isAccessable(filename) && _vm->gameFlags().isDemo) {
				// the demo version supplied with Kyra3 does not 
				// contain all pak files listed in filedata.fdt
				// so we don't do anything here if they are non
				// existant.
			} else if (!loadPakFile(filename)) {
				error("couldn't load file '%s'", filename.c_str());
				return false;
			}
		}
	}

	return true;
}

bool Resource::loadFileList(const char * const *filelist, uint32 numFiles) {
	if (!filelist)
		return false;

	while (numFiles--) {
		if (!loadPakFile(filelist[numFiles])) {
			error("couldn't load file '%s'", filelist[numFiles]);
			return false;
		}
	}

	return true;
}

void Resource::unloadPakFile(const Common::String &filename) {
	ResFileMap::iterator iter = _map.find(filename);
	if (iter != _map.end()) {
		if (!iter->_value.prot)
			iter->_value.mounted = false;
	}
}

bool Resource::isInPakList(const Common::String &filename) {
	if (!isAccessable(filename))
		return false;
	ResFileMap::iterator iter = _map.find(filename);
	if (iter == _map.end())
		return false;
	return (iter->_value.type != ResFileEntry::kRaw);
}

void Resource::unloadAllPakFiles() {
	// remove all entries
	_map.clear();
}

uint8 *Resource::fileData(const char *file, uint32 *size) {
	Common::SeekableReadStream *stream = getFileStream(file);
	if (!stream)
		return 0;

	uint32 bufferSize = stream->size();
	uint8 *buffer = new uint8[bufferSize];
	assert(buffer);
	if (size)
		*size = bufferSize;
	stream->read(buffer, bufferSize);
	delete stream;
	return buffer;
}

bool Resource::exists(const char *file, bool errorOutOnFail) {
	if (Common::File::exists(file))
		return true;
	else if (isAccessable(file))
		return true;
	else if (errorOutOnFail)
		error("File '%s' can't be found", file);
	return false;
}

uint32 Resource::getFileSize(const char *file) {
	if (Common::File::exists(file)) {
		Common::File f;
		if (f.open(file))
			return f.size();
	} else {
		if (!isAccessable(file))
			return 0;

		ResFileMap::const_iterator iter = _map.find(file);
		if (iter != _map.end())
			return iter->_value.size;
	}
	return 0;
}

bool Resource::loadFileToBuf(const char *file, void *buf, uint32 maxSize) {
	Common::SeekableReadStream *stream = getFileStream(file);
	if (!stream)
		return false;

	memset(buf, 0, maxSize);
	stream->read(buf, stream->size());
	delete stream;
	return true;
}

Common::SeekableReadStream *Resource::getFileStream(const Common::String &file) {
	if (Common::File::exists(file)) {
		Common::File *stream = new Common::File();
		if (!stream->open(file)) {
			delete stream;
			stream = 0;
			error("Couldn't open file '%s'", file.c_str());
		}
		return stream;
	} else {
		if (!isAccessable(file))
			return 0;

		ResFileMap::const_iterator iter = _map.find(file);
		if (iter == _map.end())
			return 0;

		if (!iter->_value.parent.empty()) {
			Common::SeekableReadStream *parent = getFileStream(iter->_value.parent);
			assert(parent);

			ResFileMap::const_iterator parentIter = _map.find(iter->_value.parent);
			const ResArchiveLoader *loader = getLoader(parentIter->_value.type);
			assert(loader);

			return loader->loadFileFromArchive(file, parent, iter->_value);
		} else {
			error("Couldn't open file '%s'", file.c_str());
		}
	}

	return 0;
}

bool Resource::isAccessable(const Common::String &file) {
	checkFile(file);

	ResFileMap::const_iterator iter = _map.find(file);
	while (iter != _map.end()) {
		if (!iter->_value.parent.empty()) {
			iter = _map.find(iter->_value.parent);
			if (iter != _map.end()) {
				// parent can never be a non archive file
				if (iter->_value.type == ResFileEntry::kRaw)
					return false;
				// not mounted parent means not accessable
				else if (!iter->_value.mounted)
					return false;
			}
		} else {
			return true;
		}
	}
	return false;
}

void Resource::checkFile(const Common::String &file) {
	if (_map.find(file) == _map.end() && Common::File::exists(file)) {
		Common::File temp;
		if (temp.open(file)) {
			ResFileEntry entry;
			entry.parent = "";
			entry.size = temp.size();
			entry.mounted = file.compareToIgnoreCase(StaticResource::staticDataFilename()) != 0;
			entry.preload = false;
			entry.prot = false;
			entry.type = ResFileEntry::kAutoDetect;
			entry.offset = 0;
			_map[file] = entry;
			temp.close();

			detectFileTypes();
		}
	}
}

void Resource::detectFileTypes() {
	for (ResFileMap::iterator i = _map.begin(); i != _map.end(); ++i) {
		if (!isAccessable(i->_key))
			continue;

		if (i->_value.type == ResFileEntry::kAutoDetect) {
			Common::SeekableReadStream *stream = 0;
			for (LoaderIterator l = _loaders.begin(); l != _loaders.end(); ++l) {
				if (!(*l)->checkFilename(i->_key))
					continue;
				
				if (!stream)
					stream = getFileStream(i->_key);

				if ((*l)->isLoadable(i->_key, *stream)) {
					i->_value.type = (*l)->getType();
					i->_value.mounted = false;
					i->_value.preload = false;
					break;
				}
			}
			delete stream;
			stream = 0;

			if (i->_value.type == ResFileEntry::kAutoDetect)
				i->_value.type = ResFileEntry::kRaw;
		}
	}
}

#pragma mark -
#pragma mark - ResFileLodaer
#pragma mark -

class ResLoaderPak : public ResArchiveLoader {
public:
	bool checkFilename(Common::String filename) const;
	bool isLoadable(const Common::String &filename, Common::SeekableReadStream &stream) const;
	bool loadFile(const Common::String &filename, Common::SeekableReadStream &stream, FileList &files) const;
	Common::SeekableReadStream *loadFileFromArchive(const Common::String &file, Common::SeekableReadStream *archive, const ResFileEntry entry) const;

	ResFileEntry::kType getType() const {
		return ResFileEntry::kPak;
	}
};

bool ResLoaderPak::checkFilename(Common::String filename) const {
	filename.toUppercase();
	return (filename.hasSuffix(".PAK") || filename.hasSuffix(".APK") || filename.hasSuffix(".VRM") || filename.hasSuffix(".TLK") || filename.equalsIgnoreCase(StaticResource::staticDataFilename()));
}

bool ResLoaderPak::isLoadable(const Common::String &filename, Common::SeekableReadStream &stream) const {
	uint32 filesize = stream.size();
	uint32 offset = 0;
	bool switchEndian = false;
	bool firstFile = true;

	offset = stream.readUint32LE();
	if (offset > filesize) {
		switchEndian = true;
		offset = SWAP_BYTES_32(offset);
	}

	Common::String file = "";
	while (!stream.eos()) {
		// The start offset of a file should never be in the filelist
		if (offset < stream.pos() || offset > filesize)
			return false;

		byte c = 0;

		file = "";

		while (!stream.eos() && (c = stream.readByte()) != 0)
			file += c;

		if (stream.eos())
			return false;

		// Quit now if we encounter an empty string
		if (file.empty()) {
			if (firstFile)
				return false;
			else
				break;
		}

		firstFile = false;
		offset = switchEndian ? stream.readUint32BE() : stream.readUint32LE();

		if (!offset || offset == filesize)
			break;
	}

	return true;
}

bool ResLoaderPak::loadFile(const Common::String &filename, Common::SeekableReadStream &stream, FileList &files) const {
	uint32 filesize = stream.size();
	
	uint32 startoffset = 0, endoffset = 0;
	bool switchEndian = false;
	bool firstFile = true;

	startoffset = stream.readUint32LE();
	if (startoffset > filesize) {
		switchEndian = true;
		startoffset = SWAP_BYTES_32(startoffset);
	}

	Common::String file = "";
	while (!stream.eos()) {
		// The start offset of a file should never be in the filelist
		if (startoffset < stream.pos() || startoffset > filesize) {
			warning("PAK file '%s' is corrupted", filename.c_str());
			return false;
		}

		file = "";
		byte c = 0;

		while (!stream.eos() && (c = stream.readByte()) != 0)
			file += c;

		if (stream.eos()) {
			warning("PAK file '%s' is corrupted", filename.c_str());
			return false;
		}

		// Quit now if we encounter an empty string
		if (file.empty()) {
			if (firstFile) {
				warning("PAK file '%s' is corrupted", filename.c_str());
				return false;
			} else {
				break;
			}
		}

		firstFile = false;
		endoffset = switchEndian ? stream.readUint32BE() : stream.readUint32LE();

		if (!endoffset)
			endoffset = filesize;

		if (startoffset != endoffset) {
			ResFileEntry entry;
			entry.size = endoffset - startoffset;
			entry.offset = startoffset;
			entry.parent = filename;
			entry.type = ResFileEntry::kAutoDetect;
			entry.mounted = false;
			entry.prot = false;
			entry.preload = false;

			files.push_back(File(file, entry));
		}

		if (endoffset == filesize)
			break;

		startoffset = endoffset;
	}

	return true;
}

Common::SeekableReadStream *ResLoaderPak::loadFileFromArchive(const Common::String &file, Common::SeekableReadStream *archive, const ResFileEntry entry) const {
	assert(archive);

	archive->seek(entry.offset, SEEK_SET);
	Common::SeekableSubReadStream *stream = new Common::SeekableSubReadStream(archive, entry.offset, entry.offset + entry.size, true);
	assert(stream);
	return stream;
}

class ResLoaderInsKyra : public ResArchiveLoader {
public:
	bool checkFilename(Common::String filename) const;
	bool isLoadable(const Common::String &filename, Common::SeekableReadStream &stream) const;
	bool loadFile(const Common::String &filename, Common::SeekableReadStream &stream, FileList &files) const;
	Common::SeekableReadStream *loadFileFromArchive(const Common::String &file, Common::SeekableReadStream *archive, const ResFileEntry entry) const;

	ResFileEntry::kType getType() const {
		return ResFileEntry::kInsKyra;
	}
};

bool ResLoaderInsKyra::checkFilename(Common::String filename) const {
	return false;
}

bool ResLoaderInsKyra::isLoadable(const Common::String &filename, Common::SeekableReadStream &stream) const {
	return true;
}

bool ResLoaderInsKyra::loadFile(const Common::String &filename, Common::SeekableReadStream &stream, FileList &files) const {
	return true;
}

Common::SeekableReadStream *ResLoaderInsKyra::loadFileFromArchive(const Common::String &file, Common::SeekableReadStream *archive, const ResFileEntry entry) const {
	return 0;
}

class FileExpanderSource {
public:
	FileExpanderSource(const uint8 *data) : _dataPtr(data), _bitsLeft(8), _key(0), _index(0) {}
	~FileExpanderSource() {}

	void advSrcRefresh();
	void advSrcBitsBy1();
	void advSrcBitsByIndex(uint8 newIndex);

	uint8 getKeyLower() { return _key & 0xff; }
	void setIndex(uint8 index) { _index = index; }
	uint16 getKeyMasked(uint8 newIndex);
	uint16 keyMaskedAlign(uint16 val);

	void copyBytes(uint8 *& dst);

private:
	const uint8 *_dataPtr;
	uint16 _key;
	int8 _bitsLeft;
	uint8 _index;
};

void FileExpanderSource::advSrcBitsBy1() {
	_key >>= 1;		
	if (!--_bitsLeft) {
		_key = ((*_dataPtr++) << 8 ) | (_key & 0xff);
		_bitsLeft = 8;
	}
}

void FileExpanderSource::advSrcBitsByIndex(uint8 newIndex) {
	_index = newIndex;
	_bitsLeft -= _index;
	if (_bitsLeft <= 0) {
		_key >>= (_index + _bitsLeft);
		_index = -_bitsLeft;
		_bitsLeft = 8 - _index;
		_key = (*_dataPtr++ << 8) | (_key & 0xff);
	}
	_key >>= _index;
}

uint16 FileExpanderSource::getKeyMasked(uint8 newIndex) {
	static const uint8 mskTable[] = { 0x0F, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF };
	_index = newIndex;
	uint16 res = 0;

	if (_index > 8) {
		newIndex = _index - 8;
		res = (_key & 0xff) & mskTable[8];		
		advSrcBitsByIndex(8);
		_index = newIndex;
		res |= (((_key & 0xff) & mskTable[_index]) << 8);
		advSrcBitsByIndex(_index);
	} else {
		res = (_key & 0xff) & mskTable[_index];
		advSrcBitsByIndex(_index);
	}

	return res;
}

void FileExpanderSource::copyBytes(uint8 *& dst) {
	advSrcBitsByIndex(_bitsLeft);
	uint16 r = (READ_LE_UINT16(_dataPtr) ^ _key) + 1;
	_dataPtr += 2;

	if (r)
		error("decompression failure");

	memcpy(dst, _dataPtr, _key);
	_dataPtr += _key;
	dst += _key;
}

uint16 FileExpanderSource::keyMaskedAlign(uint16 val) {
	val -= 0x101;
	_index = (val & 0xff) >> 2;
	int16 b = ((_bitsLeft << 8) | _index) - 1;
	_bitsLeft = b >> 8;
	_index = b & 0xff;
	return (((val & 3) + 4) << _index) + 0x101 + getKeyMasked(_index);
}

void FileExpanderSource::advSrcRefresh() {
	_key = READ_LE_UINT16(_dataPtr);
	_dataPtr += 2;		
	_bitsLeft = 8;
}

class FileExpander {
public:
	FileExpander();
	~FileExpander();

	bool process(const uint8 * dst, const uint8 * src, uint32 outsize, uint32 insize);

private:
	void generateTables(uint8 srcIndex, uint8 dstIndex, uint8 dstIndex2, int cnt);
	uint8 calcCmdAndIndex(const uint8 *tbl, int16 &para);

	FileExpanderSource *_src;
	uint8 *_tables[9];
	uint16 *_tables16[3];
};

FileExpander::FileExpander() : _src(0) {
	_tables[0] = new uint8[3914];
	assert(_tables[0]);

	_tables[1]	= _tables[0] + 320;
	_tables[2]	= _tables[0] + 352;
	_tables[3]	= _tables[0] + 864;
	_tables[4]	= _tables[0] + 2016;
	_tables[5]	= _tables[0] + 2528;
	_tables[6]	= _tables[0] + 2656;
	_tables[7]	= _tables[0] + 2736;
	_tables[8]	= _tables[0] + 2756;

	_tables16[0] = (uint16*) (_tables[0] + 3268);
	_tables16[1] = (uint16*) (_tables[0] + 3302);
	_tables16[2] = (uint16*) (_tables[0] + 3338);
}

FileExpander::~FileExpander() {
	delete _src;
	delete [] _tables[0];
}

bool FileExpander::process(const uint8 * dst, const uint8 * src, uint32 outsize, uint32 compressedSize) {
	static const uint8 indexTable[] = {
		0x10, 0x11, 0x12, 0x00, 0x08, 0x07, 0x09, 0x06, 0x0A,
		0x05, 0x0B, 0x04, 0x0C, 0x03, 0x0D, 0x02, 0x0E, 0x01, 0x0F
	};
	
	memset(_tables[0], 0, 3914);

	uint8 *d = (uint8*) dst;
	uint16 tableSize0 = 0;
	uint16 tableSize1 = 0;
	bool needrefresh = true;
	bool postprocess = false;

	_src = new FileExpanderSource(src);

	while (d < dst + outsize) {

		if (needrefresh) {
			needrefresh = false;
			_src->advSrcRefresh();
		}

		_src->advSrcBitsBy1();

		int mode = _src->getKeyMasked(2) - 1;
		if (mode == 1) {
			tableSize0 = _src->getKeyMasked(5) + 257;
			tableSize1 = _src->getKeyMasked(5) + 1;
			memset(_tables[7], 0, 19);
				
			const uint8 *itbl = (uint8*) indexTable;
			int numbytes = _src->getKeyMasked(4) + 4;
			
			while (numbytes--)
				_tables[7][*itbl++] = _src->getKeyMasked(3);

			generateTables(7, 8, 255, 19);

			int cnt = tableSize0 + tableSize1;
			uint8 *tmp = _tables[0];

			while (cnt) {
				uint16 cmd = _src->getKeyLower();
				cmd = READ_LE_UINT16(&_tables[8][cmd << 1]);
				_src->advSrcBitsByIndex(_tables[7][cmd]);

				if (cmd < 16) {
					*tmp++ = cmd;
					cnt--;
				} else {
					uint8 tmpI = 0;
					if (cmd == 16) {
						cmd = _src->getKeyMasked(2) + 3;
						tmpI = *(tmp - 1);							
					} else if (cmd == 17) {
						cmd = _src->getKeyMasked(3) + 3;
					} else {
						cmd = _src->getKeyMasked(7) + 11;
					}
					_src->setIndex(tmpI);
					memset(tmp, tmpI, cmd);
					tmp += cmd;

					cnt -= cmd;
					if (cnt < 0)
						error("decompression failure");
				}
			}
				
			memcpy(_tables[1], _tables[0] + tableSize0, tableSize1);
			generateTables(0, 2, 3, tableSize0);
			generateTables(1, 4, 5, tableSize1);
			postprocess = true;
		} else if (mode < 0) {
			_src->copyBytes(d);
			postprocess = false;
			needrefresh = true;
		} else if (mode == 0){
			uint16 cnt = 144;
			uint8 *d2 = (uint8*) _tables[0];			
			memset(d2, 8, 144);
			memset(d2 + 144, 9, 112);
			memset(d2 + 256, 7, 24);
			memset(d2 + 280, 8, 8);
			d2 = (uint8*) _tables[1];
			memset(d2, 5, 32);
			tableSize0 = 288;
			tableSize1 = 32;

			generateTables(0, 2, 3, tableSize0);
			generateTables(1, 4, 5, tableSize1);
			postprocess = true;
		} else {
			error("decompression failure");
		}

		if (!postprocess)
			continue;
		
		int16 cmd = 0;
		
		do  {
			cmd = ((int16*) _tables[2])[_src->getKeyLower()];
			_src->advSrcBitsByIndex(cmd < 0 ? calcCmdAndIndex(_tables[3], cmd) : _tables[0][cmd]);

			if (cmd == 0x11d) {
				cmd = 0x200;
			} else if (cmd > 0x108) {
				cmd = _src->keyMaskedAlign(cmd);
			}

			if (!(cmd >> 8)) {
				*d++ = cmd & 0xff;
			} else if (cmd != 0x100) {
				cmd -= 0xfe;
				int16 offset = ((int16*) _tables[4])[_src->getKeyLower()];
				_src->advSrcBitsByIndex(offset < 0 ? calcCmdAndIndex(_tables[5], offset) : _tables[1][offset]);
				if ((offset & 0xff) >= 4) {
					uint8 newIndex = ((offset & 0xff) >> 1) - 1;
					offset = (((offset & 1) + 2) << newIndex) + _src->getKeyMasked(newIndex);
				}

				uint8 *s2 = d - 1 - offset;
				if (s2 >= dst) {
					while (cmd--)
						*d++ = *s2++;
				} else {
					uint32 pos = dst - s2;
					s2 += (d - dst);

					if (pos < (uint32) cmd) {
						cmd -= pos;
						while (pos--)
							*d++ = *s2++;
						s2 = (uint8*) dst;
					}
					while (cmd--)
						*d++ = *s2++;
				}
			}
		} while (cmd != 0x100);
	}

	delete _src;
	_src = 0;

	return true;
}

void FileExpander::generateTables(uint8 srcIndex, uint8 dstIndex, uint8 dstIndex2, int cnt) {
	const uint8 *tbl1 = _tables[srcIndex];
	const uint8 *tbl2 = _tables[dstIndex];
	const uint8 *tbl3 = dstIndex2 == 0xff ? 0 : _tables[dstIndex2];

	if (!cnt)
		return;

	uint8 *s = (uint8*) tbl1;
	memset(_tables16[0], 0, 32);
	
	for (int i = 0; i < cnt; i++) 
		_tables16[0][(*s++)]++;

	_tables16[1][1] = 0;

	for (uint16 i = 1, r = 0; i < 16; i++) {
		r = (r + _tables16[0][i]) << 1;
		_tables16[1][i + 1] = r;
	}

	if (_tables16[1][16]) {
		uint16 r = 0;
		for (uint16 i = 1; i < 16; i++)
			r += _tables16[0][i];
		if (r > 1)
			error("decompression failure");
	}

	s = (uint8*) tbl1;
	uint16 *d = _tables16[2];
	for (int i = 0; i < cnt; i++) {
		uint16 t = *s++;
		if (t) {
			_tables16[1][t]++;
			t = _tables16[1][t] - 1;
		}
		*d++ = t;
	}

	s = (uint8*) tbl1;
	d = _tables16[2];
	for (int i = 0; i < cnt; i++) {
		int8 t = ((int8)(*s++)) - 1;
		if (t > 0) {
			uint16 v1 = *d;
			uint16 v2 = 0;
			
			do {
				v2 = (v2 << 1) | (v1 & 1);
				v1 >>= 1;
			} while (--t && v1);
			
			t++;
			uint8 c1 = (v1 & 1);
			while (t--) {
				uint8 c2 = v2 >> 15;
				v2 = (v2 << 1) | c1;
				c1 = c2;
			};

			*d++ = v2;
		} else {
			d++;
		}		
	}

	memset((void*) tbl2, 0, 512);

	cnt--;
	s = (uint8*) tbl1 + cnt;
	d = &_tables16[2][cnt];
	uint16 * bt = (uint16*) tbl3;
	uint16 inc = 0;
	uint16 cnt2 = 0;

	do {
		uint8 t = *s--;
		uint16 *s2 = (uint16*) tbl2;

		if (t && t < 9) {
			inc = 1 << t;
			uint16 o = *d;
			
			do {
				s2[o] = cnt;
				o += inc;
			} while (!(o & 0xf00));

		} else if (t > 8) {
			if (!bt)
				error("decompression failure");

			t -= 8;
			uint8 shiftCnt = 1;
			uint8 v = (*d) >> 8;
			s2 = &((uint16*) tbl2)[*d & 0xff];

			do {
				if (!*s2) {
					*s2 = (uint16)(~cnt2);
					*(uint32*)&bt[cnt2] = 0;
					cnt2 += 2;
				}

				s2 = &bt[(uint16)(~*s2)];
				if (v & shiftCnt)
					s2++;

				shiftCnt <<= 1;
			} while (--t);
			*s2 = cnt;
		}
		d--;		
	} while (--cnt >= 0);
}

uint8 FileExpander::calcCmdAndIndex(const uint8 *tbl, int16 &para) {
	const uint16 *t = (uint16*) tbl;
	_src->advSrcBitsByIndex(8);
	uint8 newIndex = 0;
	uint16 v = _src->getKeyLower();

	do {
		newIndex++;
		para = t[((~para) & 0xfffe) | (v & 1)];
		v >>= 1;
	} while (para < 0);

	return newIndex;
}

class FileCache {
public:
	FileCache() : _size(0) {}
	~FileCache() {}

	void add(ResFileEntry entry, const uint8 *data);
	bool getData(ResFileEntry entry, uint8 *dst);
	void flush();

private:
	struct FileCacheEntry {
		ResFileEntry entry;
		const uint8 *data;
	};

	Common::List<FileCacheEntry> _cachedFileList;
	uint32 _size;
};

void FileCache::add(ResFileEntry entry, const uint8 *data) {
	FileCacheEntry fileCacheEntry;
	fileCacheEntry.entry.compressedSize = entry.compressedSize;
	fileCacheEntry.entry.mounted = entry.mounted;
	fileCacheEntry.entry.offset = entry.offset;
	fileCacheEntry.entry.parent = entry.parent;
	fileCacheEntry.entry.preload = entry.preload;
	fileCacheEntry.entry.prot = entry.prot;
	fileCacheEntry.entry.size = entry.size;
	fileCacheEntry.entry.type = entry.type;
	fileCacheEntry.entry.fileIndex = entry.fileIndex;
	uint8 *dst = new uint8[entry.size];
	assert(dst);
	memcpy(dst, data, entry.size);
	fileCacheEntry.data = dst;
	_cachedFileList.push_back(fileCacheEntry);
	_size += entry.size;
}

bool FileCache::getData(ResFileEntry entry, uint8 *dst) {
	for (Common::List<FileCacheEntry>::const_iterator c = _cachedFileList.begin(); c != _cachedFileList.end(); ++c) {
		if (c->entry.offset == entry.offset && c->entry.compressedSize == entry.compressedSize &&
			c->entry.fileIndex == entry.fileIndex && c->entry.size == entry.size) {
				memcpy(dst, c->data, c->entry.size);
				return true;
		}
	}
	return false;
}

void FileCache::flush() {
	debug("total amount of cache memory used: %d", _size);
	for (Common::List<FileCacheEntry>::const_iterator c = _cachedFileList.begin(); c != _cachedFileList.end(); ++c)
		delete [] c->data;
	_cachedFileList.clear();
	_size = 0;
}

class ResLoaderInsHof : public ResArchiveLoader {
public:
	ResLoaderInsHof() {}
	~ResLoaderInsHof() { _fileCache.flush(); }

	bool checkFilename(Common::String filename) const;
	bool isLoadable(const Common::String &filename, Common::SeekableReadStream &stream) const;
	bool loadFile(const Common::String &filename, Common::SeekableReadStream &stream, FileList &files) const;
	Common::SeekableReadStream *loadFileFromArchive(const Common::String &file, Common::SeekableReadStream *archive, const ResFileEntry entry) const;

	ResFileEntry::kType getType() const {
		return ResFileEntry::kInsHof;
	}
private:
	mutable FileCache _fileCache;
};

bool ResLoaderInsHof::checkFilename(Common::String filename) const {
	filename.toUppercase();
	if (!filename.hasSuffix(".001"))
		return false;
	filename.insertChar('2', filename.size() - 1);
	filename.deleteLastChar();
	if (!Common::File::exists(filename))
		return false;
	return true;
}

bool ResLoaderInsHof::isLoadable(const Common::String &filename, Common::SeekableReadStream &stream) const {
	uint8 fileId = stream.readByte();
	uint32 size = stream.readUint32LE();
	
	if (size < stream.size() + 1)
		return false;

	return (fileId == 1);
}

bool ResLoaderInsHof::loadFile(const Common::String &filename, Common::SeekableReadStream &stream, FileList &files) const {
	Common::File tmpFile;

	uint32 pos = 0;
	uint32 bytesleft = 0;
	bool startFile = true;

	Common::String fn_base(filename.c_str(), 10);
	Common::String fn_tmp;
	char fn_ext[4];

	int i = filename.size() - 1;
	while (fn_base.lastChar() != '.')
		fn_base.deleteLastChar();

	struct InsHofArchive {
		Common::String filename;
		uint32 firstFile;
		uint32 startOffset;
		uint32 lastFile;
		uint32 endOffset;
		uint32 totalSize;
	} newArchive;

	Common::List<InsHofArchive> archives;

	for (int8 currentFile = 1; currentFile; currentFile++) {	
		sprintf(fn_ext, "%03d", currentFile);
		fn_tmp = fn_base + Common::String(fn_ext);

		Common::File tmpFile;
		if (!tmpFile.open(fn_tmp)) {
			debug(3, "couldn't open file '%s'\n", fn_tmp);
			break;
		}

		tmpFile.seek(pos);
		uint8 fileId = tmpFile.readByte();
		pos++;

		uint32 size = tmpFile.size() - 1;
		if (startFile) {
			size -= 4;
			if (fileId == currentFile) {
				size -= 6;
				pos += 6;
				tmpFile.seek(6, SEEK_CUR);
			} else {
				size = ++size - pos;
			}
			newArchive.filename = fn_base;
			bytesleft = newArchive.totalSize = tmpFile.readUint32LE();
			pos += 4;
			newArchive.firstFile = currentFile;
			newArchive.startOffset = pos;
			startFile = false;
		}

		uint32 cs = MIN(size, bytesleft);
		bytesleft -= cs;

		tmpFile.close();
		
		pos += cs;
		if (cs == size) {
			if (!bytesleft) {
				newArchive.lastFile = currentFile;
				newArchive.endOffset = --pos;
				archives.push_back(newArchive);
				currentFile = -1;
			} else {
				pos = 0;
			}
		} else {
			startFile = true;
			bytesleft = size - cs;
			newArchive.lastFile = currentFile--;
			newArchive.endOffset = --pos;
			archives.push_back(newArchive);
		}
	}

	ResFileEntry newEntry;
	pos = 0;

	const uint32 kExecSize = 0x0bba;
	const uint32 kHeaderSize = 30;
	const uint32 kHeaderSize2 = 46;

	for (Common::List<InsHofArchive>::iterator a = archives.begin(); a != archives.end(); ++a) {
		bool startFile = true;
		for (uint32 i = a->firstFile; i != (a->lastFile + 1); i++) {
			sprintf(fn_ext, "%03d", i);
			fn_tmp = a->filename + Common::String(fn_ext);

			if (!tmpFile.open(fn_tmp)) {
				debug(3, "couldn't open file '%s'\n", fn_tmp);
				break;
			}

			uint32 size = (i == a->lastFile) ? a->endOffset : tmpFile.size();
			
			if (startFile) {
				startFile = false;
				pos = a->startOffset + kExecSize;
				if (pos > size) {
					pos -= size;
					tmpFile.close();
					continue;
				}
			} else {
				pos ++;
			}

			while (pos < size) {
				uint8 hdr[43];
				uint32 m = 0;
				tmpFile.seek(pos);

				if (pos + 42 > size) {
					m = size - pos;
					uint32 b = 42 - m;

					if (m >= 4) {
						uint32 id = tmpFile.readUint32LE();
						if (id == 0x06054B50) {
							startFile = true;
							break;
						} else {
							tmpFile.seek(pos);
						}
					}

					
					sprintf(fn_ext, "%03d", i + 1);
					fn_tmp = a->filename + Common::String(fn_ext);

					Common::File tmpFile2;
					tmpFile2.open(fn_tmp);
					tmpFile.read(hdr, m);
					tmpFile2.read(hdr + m, b);
					tmpFile2.close();

				} else {
					tmpFile.read(hdr, 42);
				}

				uint32 id = READ_LE_UINT32(hdr);
				
				if (id == 0x04034B50) {
					if (hdr[8] != 8)
						error("compression type not implemented");
					newEntry.compressedSize = READ_LE_UINT32(hdr + 18);
					newEntry.size = READ_LE_UINT32(hdr + 22);
			
					uint16 filestrlen = READ_LE_UINT16(hdr + 26);
					*(hdr + 30 + filestrlen) = 0;
					pos += (kHeaderSize + filestrlen - m);

					newEntry.parent = filename;
					newEntry.offset = pos;
					newEntry.type = ResFileEntry::kAutoDetect;
					newEntry.mounted = false;
					newEntry.preload = false;
					newEntry.prot = false;
					newEntry.fileIndex = i;
					files.push_back(File(Common::String((const char*) (hdr + 30)), newEntry));

					pos += newEntry.compressedSize;
					if (pos > size) {
						pos -= size;
						break;
					}
				} else {
					uint32 filestrlen = READ_LE_UINT32(hdr + 28);
					pos += (kHeaderSize2 + filestrlen - m);
				}
			}
			tmpFile.close();
		}
	}

	archives.clear();
	
	return true;
}

Common::SeekableReadStream *ResLoaderInsHof::loadFileFromArchive(const Common::String &file, Common::SeekableReadStream *archive, const ResFileEntry entry) const {
	if (archive) {
		delete archive;
		archive = 0;
	}

	bool resident = false;
	uint8 * outbuffer = (uint8*) malloc(entry.size);
	assert(outbuffer);

	if (!_fileCache.getData(entry, outbuffer)) {
		Common::File tmpFile;
		char filename[13];
		sprintf(filename, "WESTWOOD.%03d", entry.fileIndex);

		if (!tmpFile.open(filename)) {
			delete [] outbuffer;
			return 0;
		}

		tmpFile.seek(entry.offset);

		uint8 *inbuffer = new uint8[entry.compressedSize];
		assert(inbuffer);

		if ((entry.offset + entry.compressedSize) > tmpFile.size()) {
			// this is for files that are split between two archive files
			uint32 a = tmpFile.size() - entry.offset;
			uint32 b = entry.compressedSize - a;
				
			tmpFile.read(inbuffer, a);
			tmpFile.close();

			filename[strlen(filename) - 1]++;

			if (!tmpFile.open(filename))
				return 0;
			tmpFile.seek(1);
			tmpFile.read(inbuffer + a, b);

		} else {
			tmpFile.read(inbuffer, entry.compressedSize);
		}

		tmpFile.close();

		FileExpander().process(outbuffer, inbuffer, entry.size, entry.compressedSize);
		delete [] inbuffer;

		if (entry.size > INS_CACHE_THRESHOLD)
			_fileCache.add(entry, outbuffer);
	}
	
	Common::MemoryReadStream *stream = new Common::MemoryReadStream(outbuffer, entry.size, true);
	assert(stream);	
	
	return stream;
}

class ResLoaderInsMalcolm : public ResArchiveLoader {
public:
	bool checkFilename(Common::String filename) const;
	bool isLoadable(const Common::String &filename, Common::SeekableReadStream &stream) const;
	bool loadFile(const Common::String &filename, Common::SeekableReadStream &stream, FileList &files) const;
	Common::SeekableReadStream *loadFileFromArchive(const Common::String &file, Common::SeekableReadStream *archive, const ResFileEntry entry) const;

	ResFileEntry::kType getType() const {
		return ResFileEntry::kInsMal;
	}
};

bool ResLoaderInsMalcolm::checkFilename(Common::String filename) const {
	filename.toUppercase();
	if (!filename.hasSuffix(".001"))
		return false;
	filename.insertChar('2', filename.size() - 1);
	filename.deleteLastChar();
	if (Common::File::exists(filename))
		return false;
	return true;
}

bool ResLoaderInsMalcolm::isLoadable(const Common::String &filename, Common::SeekableReadStream &stream) const {
	stream.seek(3);
	uint32 size = stream.readUint32LE();

	if (size+7 > stream.size())
		return false;

	stream.seek(size+5, SEEK_SET);
	uint8 buffer[2];
	stream.read(&buffer, 2);

	return (buffer[0] == 0x0D && buffer[1] == 0x0A);
}

bool ResLoaderInsMalcolm::loadFile(const Common::String &filename, Common::SeekableReadStream &stream, FileList &files) const {
	Common::List<Common::String> filenames;

	// thanks to eriktorbjorn for this code (a bit modified though)
	stream.seek(3, SEEK_SET);

	// first file is the index table
	uint32 size = stream.readUint32LE();
	Common::String temp = "";

	for (uint32 i = 0; i < size; ++i) {
		byte c = stream.readByte();

		if (c == '\\') {
			temp = "";
		} else if (c == 0x0D) {
			// line endings are CRLF
			c = stream.readByte();
			assert(c == 0x0A);
			++i;

			filenames.push_back(temp);
		} else {
			temp += (char)c;
		}
	}

	stream.seek(3, SEEK_SET);

	for (Common::List<Common::String>::iterator file = filenames.begin(); file != filenames.end(); ++file) {
		ResFileEntry entry;
		entry.parent = filename;
		entry.type = ResFileEntry::kAutoDetect;
		entry.mounted = false;
		entry.preload = false;
		entry.prot = false;
		entry.size = stream.readUint32LE();
		entry.offset = stream.pos();
		stream.seek(entry.size, SEEK_CUR);
		files.push_back(File(*file, entry));
	}

	return true;
}

Common::SeekableReadStream *ResLoaderInsMalcolm::loadFileFromArchive(const Common::String &file, Common::SeekableReadStream *archive, const ResFileEntry entry) const {
	assert(archive);

	archive->seek(entry.offset, SEEK_SET);
	Common::SeekableSubReadStream *stream = new Common::SeekableSubReadStream(archive, entry.offset, entry.offset + entry.size, true);
	assert(stream);
	return stream;
}

class ResLoaderTlk : public ResArchiveLoader {
public:
	bool checkFilename(Common::String filename) const;
	bool isLoadable(const Common::String &filename, Common::SeekableReadStream &stream) const;
	bool loadFile(const Common::String &filename, Common::SeekableReadStream &stream, FileList &files) const;
	Common::SeekableReadStream *loadFileFromArchive(const Common::String &file, Common::SeekableReadStream *archive, const ResFileEntry entry) const;

	ResFileEntry::kType getType() const {
		return ResFileEntry::kTlk;
	}

private:
	static bool sortTlkFileList(const File &l, const File &r);
	static FileList::const_iterator nextFile(const FileList &list, FileList::const_iterator iter);
};

bool ResLoaderTlk::checkFilename(Common::String filename) const {
	filename.toUppercase();
	return (filename.hasSuffix(".TLK"));
}

bool ResLoaderTlk::isLoadable(const Common::String &filename, Common::SeekableReadStream &stream) const {
	uint16 entries = stream.readUint16LE();
	uint32 entryTableSize = (entries * 8);

	if (entryTableSize + 2 > stream.size())
		return false;

	uint32 offset = 0;

	for (uint i = 0; i < entries; ++i) {
		stream.readUint32LE();
		offset = stream.readUint32LE();

		if (offset > stream.size())
			return false;
	}

	return true;
}

bool ResLoaderTlk::loadFile(const Common::String &filename, Common::SeekableReadStream &stream, FileList &files) const {
	uint16 entries = stream.readUint16LE();
	
	for (uint i = 0; i < entries; ++i) {
		ResFileEntry entry;
		entry.parent = filename;
		entry.type = ResFileEntry::kAutoDetect;
		entry.mounted = false;
		entry.preload = false;
		entry.prot = false;

		uint32 resFilename = stream.readUint32LE();
		uint32 resOffset = stream.readUint32LE();

		entry.offset = resOffset+4;

		char realFilename[20];
		snprintf(realFilename, 20, "%u.AUD", resFilename);

		uint32 curOffset = stream.pos();
		stream.seek(resOffset, SEEK_SET);
		entry.size = stream.readUint32LE();
		stream.seek(curOffset, SEEK_SET);

		files.push_back(FileList::value_type(realFilename, entry));
	}

	return true;
}

Common::SeekableReadStream *ResLoaderTlk::loadFileFromArchive(const Common::String &file, Common::SeekableReadStream *archive, const ResFileEntry entry) const {
	assert(archive);

	archive->seek(entry.offset, SEEK_SET);
	Common::SeekableSubReadStream *stream = new Common::SeekableSubReadStream(archive, entry.offset, entry.offset + entry.size, true);
	assert(stream);
	return stream;
}

#pragma mark -

void Resource::initializeLoaders() {
	_loaders.push_back(LoaderList::value_type(new ResLoaderPak()));
	_loaders.push_back(LoaderList::value_type(new ResLoaderInsKyra()));
	_loaders.push_back(LoaderList::value_type(new ResLoaderInsHof()));
	_loaders.push_back(LoaderList::value_type(new ResLoaderInsMalcolm()));
	_loaders.push_back(LoaderList::value_type(new ResLoaderTlk()));
}

const ResArchiveLoader *Resource::getLoader(ResFileEntry::kType type) const {
	for (CLoaderIterator i = _loaders.begin(); i != _loaders.end(); ++i) {
		if ((*i)->getType() == type)
			return (*i).get();
	}
	return 0;
}

} // end of namespace Kyra


