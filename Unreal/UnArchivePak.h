#ifndef __UNARCHIVE_PAK_H__
#define __UNARCHIVE_PAK_H__

#if UNREAL4


// NOTE: this implementation has a lot of common things with FObbFile. If there'll be another
// one virtual file system with similar implementation, it's worth to make some parent class
// for all of them which will differs perhaps only with AttachReader() method.

#define PAK_FILE_MAGIC		0x5A6F12E1

// Pak file versions
enum
{
	PAK_INITIAL = 1,
	PAK_NO_TIMESTAMPS,
	PAK_COMPRESSION_ENCRYPTION,
	PAK_INDEX_ENCRYPTION,				// UE4.17+

	PAK_LATEST_PLUS_ONE,
	PAK_LATEST = PAK_LATEST_PLUS_ONE - 1
};

// hack: use ArLicenseeVer to not pass FPakInfo.Version to serializer
#define PakVer		ArLicenseeVer

struct FPakInfo
{
	int			Magic;
	int			Version;
	int64		IndexOffset;
	int64		IndexSize;
	byte		IndexHash[20];
	// When new fields are added to FPakInfo, they're serialized before 'Magic' to keep compatibility
	// with older pak file versions. At the same time, structure size grows.
	byte		bEncryptedIndex;

	enum { Size = sizeof(int) * 2 + sizeof(int64) * 2 + 20 + /* new fields */ 1 };

	friend FArchive& operator<<(FArchive& Ar, FPakInfo& P)
	{
		// New FPakInfo fields.
		Ar << P.bEncryptedIndex;

		// Old FPakInfo fields.
		Ar << P.Magic << P.Version << P.IndexOffset << P.IndexSize;
		Ar.Serialize(ARRAY_ARG(P.IndexHash));

		// Reset new fields to their default states when seralizing older pak format.
		if (P.Version < PAK_INDEX_ENCRYPTION)
		{
			P.bEncryptedIndex = false;
		}

		if (P.Version > PAK_LATEST)
		{
			appError("Pak file has unsupported version %d", P.Version);
		}
		return Ar;
	}
};

struct FPakCompressedBlock
{
	int64		CompressedStart;
	int64		CompressedEnd;

	friend FArchive& operator<<(FArchive& Ar, FPakCompressedBlock& B)
	{
		return Ar << B.CompressedStart << B.CompressedEnd;
	}
};

struct FPakEntry
{
	const char*	Name;
	int64		Pos;
	int64		Size;
	int64		UncompressedSize;
	int32		CompressionMethod;
	byte		Hash[20];
	byte		bEncrypted;
	TArray<FPakCompressedBlock> CompressionBlocks;
	int32		CompressionBlockSize;

	int32		StructSize;					// computed value
	FPakEntry*	HashNext;					// computed value

	friend FArchive& operator<<(FArchive& Ar, FPakEntry& P)
	{
		guard(FPakEntry<<);

		// FPakEntry is duplicated before each stored file, without a filename. So,
		// remember the serialized size of this structure to avoid recomputation later.
		int64 StartOffset = Ar.Tell64();

		Ar << P.Pos << P.Size << P.UncompressedSize << P.CompressionMethod;

		if (Ar.PakVer < PAK_NO_TIMESTAMPS)
		{
			int64	timestamp;
			Ar << timestamp;
		}

		Ar.Serialize(ARRAY_ARG(P.Hash));

		if (Ar.PakVer >= PAK_COMPRESSION_ENCRYPTION)
		{
			if (P.CompressionMethod != 0)
				Ar << P.CompressionBlocks;
			Ar << P.bEncrypted << P.CompressionBlockSize;
		}
#if TEKKEN7
		if (GForceGame == GAME_Tekken7)
			P.bEncrypted = false;		// Tekken 7 has 'bEncrypted' flag set, but actually there's no encryption
#endif

		P.StructSize = Ar.Tell64() - StartOffset;

		return Ar;

		unguard;
	}
};

class FPakFile : public FArchive
{
	DECLARE_ARCHIVE(FPakFile, FArchive);
public:
	FPakFile(const FPakEntry* info, FArchive* reader)
	:	Info(info)
	,	Reader(reader)
	,	UncompressedBuffer(NULL)
	{}

	virtual ~FPakFile()
	{
		if (UncompressedBuffer)
			appFree(UncompressedBuffer);
	}

	virtual void Serialize(void *data, int size)
	{
		guard(FPakFile::Serialize);
		if (ArStopper > 0 && ArPos + size > ArStopper)
			appError("Serializing behind stopper (%X+%X > %X)", ArPos, size, ArStopper);

		if (Info->CompressionMethod)
		{
			guard(SerializeCompressed);

			while (size > 0)
			{
				if ((UncompressedBuffer == NULL) || (ArPos < UncompressedBufferPos) || (ArPos >= UncompressedBufferPos + Info->CompressionBlockSize))
				{
					// buffer is not ready
					if (UncompressedBuffer == NULL)
					{
						UncompressedBuffer = (byte*)appMalloc((int)Info->CompressionBlockSize); // size of uncompressed block
					}
					// prepare buffer
					int BlockIndex = ArPos / Info->CompressionBlockSize;
					UncompressedBufferPos = Info->CompressionBlockSize * BlockIndex;

					const FPakCompressedBlock& Block = Info->CompressionBlocks[BlockIndex];
					int CompressedBlockSize = (int)(Block.CompressedEnd - Block.CompressedStart);
					int UncompressedBlockSize = min((int)Info->CompressionBlockSize, (int)Info->UncompressedSize - UncompressedBufferPos); // don't pass file end
					byte* CompressedData = (byte*)appMalloc(CompressedBlockSize);
					Reader->Seek64(Block.CompressedStart);
					Reader->Serialize(CompressedData, CompressedBlockSize);
					appDecompress(CompressedData, CompressedBlockSize, UncompressedBuffer, UncompressedBlockSize, Info->CompressionMethod);
					appFree(CompressedData);
				}

				// data is in buffer, copy it
				int BytesToCopy = UncompressedBufferPos + Info->CompressionBlockSize - ArPos; // number of bytes until end of the buffer
				if (BytesToCopy > size) BytesToCopy = size;
				assert(BytesToCopy > 0);

				// copy uncompressed data
				int OffsetInBuffer = ArPos - UncompressedBufferPos;
				memcpy(data, UncompressedBuffer + OffsetInBuffer, BytesToCopy);

				// advance pointers
				ArPos += BytesToCopy;
				size  -= BytesToCopy;
				data  = OffsetPointer(data, BytesToCopy);
			}

			unguard;
		}
		else
		{
			guard(SerializeUncompressed);

			// seek every time in a case if the same 'Reader' was used by different FPakFile
			// (this is a lightweight operation for buffered FArchive)
			Reader->Seek64(Info->Pos + Info->StructSize + ArPos);
			Reader->Serialize(data, size);
			ArPos += size;

			unguard;
		}
		unguard;
	}

	virtual void Seek(int Pos)
	{
		guard(FPakFile::Seek);
		assert(Pos >= 0 && Pos < Info->UncompressedSize);
		ArPos = Pos;
		unguard;
	}

	virtual int GetFileSize() const
	{
		return (int)Info->UncompressedSize;
	}

protected:
	const FPakEntry* Info;
	FArchive*	Reader;
	byte*		UncompressedBuffer;
	int			UncompressedBufferPos;
};


class FPakVFS : public FVirtualFileSystem
{
public:
	FPakVFS(const char* InFilename)
	:	Filename(InFilename)
	,	Reader(NULL)
	,	LastInfo(NULL)
	,	HashTable(NULL)
	{}

	virtual ~FPakVFS()
	{
		delete Reader;
		if (HashTable) delete[] HashTable;
	}

	virtual bool AttachReader(FArchive* reader)
	{
		guard(FPakVFS::ReadDirectory);

		// Read pak header
		FPakInfo info;
		reader->Seek64(reader->GetFileSize64() - FPakInfo::Size);
		*reader << info;
		if (info.Magic != PAK_FILE_MAGIC)		// no endian checking here
			return false;

		if (info.bEncryptedIndex)
		{
			appNotify("WARNING: Pak \"%s\" has encrypted index. Skipping.", *Filename);
			return false;
		}

		// this file looks correct, store 'reader'
		Reader = reader;

		// Read pak index

		Reader->ArLicenseeVer = info.Version;

		Reader->Seek64(info.IndexOffset);

		FStaticString<MAX_PACKAGE_PATH> MountPoint;
		*Reader << MountPoint;

		// Process MountPoint
		if (!MountPoint.RemoveFromStart("../../.."))
		{
			appNotify("WARNING: Pak \"%s\" has strange mount point \"%s\", mounting to root", *Filename, *MountPoint);
			MountPoint = "/";
		}
		if (MountPoint[0] != '/' || ( (MountPoint.Len() > 1) && (MountPoint[1] == '.') ))
		{
			appNotify("WARNING: Pak \"%s\" has strange mount point \"%s\", mounting to root", *Filename, *MountPoint);
			MountPoint = "/";
		}

		int count;
		*Reader << count;
		FileInfos.AddZeroed(count);

		int numEncryptedFiles = 0;
		for (int i = 0; i < count; i++)
		{
			FPakEntry& E = FileInfos[i];
			// serialize name, combine with MountPoint
			FStaticString<MAX_PACKAGE_PATH> Filename;
			*Reader << Filename;
			FStaticString<MAX_PACKAGE_PATH> CombinedPath;
			CombinedPath = MountPoint;
			CombinedPath += Filename;
			E.Name = appStrdupPool(*CombinedPath);
			// serialize other fields
			*Reader << E;
			if (E.bEncrypted)
			{
//				appPrintf("Encrypted file: %s\n", *Filename);
				numEncryptedFiles++;
			}
		}
		if (count >= MIN_PAK_SIZE_FOR_HASHING)
		{
			// Hash everything
			for (int i = 0; i < count; i++)
			{
				AddFileToHash(&FileInfos[i]);
			}
		}
		// Print statistics
		appPrintf("Pak %s: %d files", *Filename, count);
		if (numEncryptedFiles)
			appPrintf(" (%d encrypted)", numEncryptedFiles);
		if (strcmp(*MountPoint, "/") != 0)
			appPrintf(", mount point: \"%s\"", *MountPoint);
		appPrintf("\n");

		return true;

		unguard;
	}

	virtual int GetFileSize(const char* name)
	{
		const FPakEntry* info = FindFile(name);
		return (info) ? (int)info->UncompressedSize : 0;
	}

	// iterating over all files
	virtual int NumFiles() const
	{
		return FileInfos.Num();
	}

	virtual const char* FileName(int i)
	{
		FPakEntry* info = &FileInfos[i];
		LastInfo = info;
		return info->Name;
	}

	virtual FArchive* CreateReader(const char* name)
	{
		const FPakEntry* info = FindFile(name);
		if (!info) return NULL;
		if (info->bEncrypted)
		{
			appPrintf("pak(%s): attempt to open encrypted file %s\n", *Filename, name);
			return NULL;
		}
		return new FPakFile(info, Reader);
	}

protected:
	enum { HASH_SIZE = 1024 };
	enum { HASH_MASK = HASH_SIZE - 1 };
	enum { MIN_PAK_SIZE_FOR_HASHING = 256 };

	FString				Filename;
	FArchive*			Reader;
	TArray<FPakEntry>	FileInfos;
	FPakEntry*			LastInfo;			// cached last accessed file info, simple optimization
	FPakEntry**			HashTable;

	static uint16 GetHashForFileName(const char* FileName)
	{
		uint16 hash = 0;
		while (char c = *FileName++)
		{
			if (c >= 'A' && c <= 'Z') c += 'a' - 'A'; // lowercase a character
			hash = ROL16(hash, 5) - hash + ((c << 4) + c ^ 0x13F);	// some crazy hash function
		}
		hash &= HASH_MASK;
		return hash;
	}

	void AddFileToHash(FPakEntry* File)
	{
		if (!HashTable)
		{
			HashTable = new FPakEntry* [HASH_SIZE];
			memset(HashTable, 0, sizeof(FPakEntry*) * HASH_SIZE);
		}
		uint16 hash = GetHashForFileName(File->Name);
		File->HashNext = HashTable[hash];
		HashTable[hash] = File;
	}

	const FPakEntry* FindFile(const char* name)
	{
		if (LastInfo && !stricmp(LastInfo->Name, name))
			return LastInfo;

		if (HashTable)
		{
			// Have a hash table, use it
			uint16 hash = GetHashForFileName(name);
			for (FPakEntry* info = HashTable[hash]; info; info = info->HashNext)
			{
				if (!stricmp(info->Name, name))
				{
					LastInfo = info;
					return info;
				}
			}
			return NULL;
		}

		// Linear search without a hash table
		for (int i = 0; i < FileInfos.Num(); i++)
		{
			FPakEntry* info = &FileInfos[i];
			if (!stricmp(info->Name, name))
			{
				LastInfo = info;
				return info;
			}
		}
		return NULL;
	}
};


#endif // UNREAL4

#endif // __UNARCHIVE_PAK_H__
