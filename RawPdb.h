#pragma once
#include <cinttypes>
#include <stdint.h>

namespace PDB
{
#define PDB_PRAGMA(_x)									__pragma(_x)
#define PDB_PUSH_WARNING_MSVC							PDB_PRAGMA(warning(push))
#define PDB_SUPPRESS_WARNING_MSVC(_number)				PDB_PRAGMA(warning(suppress : _number))
#define PDB_DISABLE_WARNING_MSVC(_number)				PDB_PRAGMA(warning(disable : _number))
#define PDB_POP_WARNING_MSVC							PDB_PRAGMA(warning(pop))

// Defines a C-like flexible array member.
#define PDB_FLEXIBLE_ARRAY_MEMBER(_type, _name)				\
	PDB_PUSH_WARNING_MSVC									\
	PDB_DISABLE_WARNING_MSVC(4200)							\
	_type _name[0];											\
	PDB_POP_WARNING_MSVC									\

	static constexpr const uint32_t InfoStreamIndex = 1u;

	enum class Byte : unsigned char
	{
	};

#pragma pack(push, 1)
	// https://llvm.org/docs/PDB/MsfFile.html#msf-superblock
	struct SuperBlock
	{
		static const char MAGIC[30u];

		char fileMagic[30u];
		char padding[2u];
		uint32_t blockSize;
		uint32_t freeBlockMapIndex; // index of the free block map
		uint32_t blockCount; // number of blocks in the file
		uint32_t directorySize; // size of the stream directory in bytes
		uint32_t unknown;
		PDB_FLEXIBLE_ARRAY_MEMBER(uint32_t, directoryBlockIndices) // indices of the blocks that make up the directory indices
	};

	// https://github.com/Microsoft/microsoft-pdb/blob/master/PDB/msf/msf.cpp#L962
	const char SuperBlock::MAGIC[30u] = "Microsoft C/C++ MSF 7.00\r\n\x1a\x44\x53";

	// this matches the definition in guiddef.h, but we don't want to pull that in
	struct GUID
	{
		uint32_t Data1;
		uint16_t Data2;
		uint16_t Data3;
		uint8_t Data4[8];
	};

	// https://llvm.org/docs/PDB/PdbStream.html#stream-header
	struct Header
	{
		enum class Version : uint32_t
		{
			VC2 = 19941610u,
			VC4 = 19950623u,
			VC41 = 19950814u,
			VC50 = 19960307u,
			VC98 = 19970604u,
			VC70Dep = 19990604u,
			VC70 = 20000404u,
			VC80 = 20030901u,
			VC110 = 20091201u,
			VC140 = 20140508u
		};

		Version version;
		uint32_t signature;
		uint32_t age;
		GUID guid;
	};

	// https://llvm.org/docs/PDB/PdbStream.html
	struct NamedStreamMap
	{
		uint32_t length;
		PDB_FLEXIBLE_ARRAY_MEMBER(char, stringTable)

		struct HashTableEntry
		{
			uint32_t stringTableOffset;
			uint32_t streamIndex;
		};
	};

	// https://llvm.org/docs/PDB/HashTable.html
	struct SerializedHashTable
	{
		struct Header
		{
			uint32_t size;
			uint32_t capacity;
		};

		struct BitVector
		{
			uint32_t wordCount;
			PDB_FLEXIBLE_ARRAY_MEMBER(uint32_t, words)
		};
	};

	// https://llvm.org/docs/PDB/PdbStream.html#pdb-feature-codes
	enum class FeatureCode : uint32_t
	{
		VC110 = 20091201,
		VC140 = 20140508,

		// https://github.com/microsoft/microsoft-pdb/blob/master/PDB/include/pdbcommon.h#L23
		NoTypeMerge = 0x4D544F4E,				// "NOTM"
		MinimalDebugInfo = 0x494E494D			// "MINI", i.e. executable was linked with /DEBUG:FASTLINK
	};
#pragma pack(pop)

	// Converts a block index into a file offset, based on the block size of the PDB file
	inline size_t ConvertBlockIndexToFileOffset(uint32_t blockIndex, uint32_t blockSize)
	{
		// cast to size_t to avoid potential overflow in 64-bit
		return static_cast<size_t>(blockIndex) * static_cast<size_t>(blockSize);
	}

	// Calculates how many blocks are needed for a certain number of bytes
	inline uint32_t ConvertSizeToBlockCount(uint32_t sizeInBytes, uint32_t blockSize)
	{
		// integer ceil to account for non-full blocks
		return static_cast<uint32_t>((static_cast<size_t>(sizeInBytes) + blockSize - 1u) / blockSize);
	}
	
	bool AreBlockIndicesContiguous(const uint32_t* blockIndices, uint32_t blockSize, uint32_t streamSize)
	{
		const uint32_t blockCount = ConvertSizeToBlockCount(streamSize, blockSize);
	
		// start with the first index, checking if all following indices are contiguous (N, N+1, N+2, ...)
		uint32_t expectedIndex = blockIndices[0];
		for (uint32_t i = 1u; i < blockCount; ++i)
		{
			++expectedIndex;
			if (blockIndices[i] != expectedIndex)
			{
				return false;
			}
		}
	
		return true;
	}

	namespace Pointer
	{
		// Offsets any pointer by a given number of bytes.
		template <typename T, typename U, typename V>
		inline T Offset(U* anyPointer, V howManyBytes)
		{
			static_assert(std::is_pointer<T>::value == true, "Type T must be a pointer type.");
			static_assert(std::is_const<typename std::remove_pointer<T>::type>::value == std::is_const<U>::value, "Wrong constness.");

			union
			{
				T as_T;
				U* as_U_ptr;
				char* as_char_ptr;
			};

			as_U_ptr = anyPointer;
			as_char_ptr += howManyBytes;

			return as_T;
		}
	}

	class RawPdb
	{
	public:
		RawPdb()
		{
		}

		~RawPdb()
		{
			Close();
		}

		class CoalescedMSFStream
		{
		public:
			CoalescedMSFStream() : ownedData_(nullptr) , data_(nullptr) , size_(0u)
			{
			}

			explicit CoalescedMSFStream(const void* data, uint32_t blockSize, const uint32_t* blockIndices,
			                            uint32_t streamSize) : ownedData_(nullptr) , data_(nullptr) , size_(streamSize)
			{
				if (AreBlockIndicesContiguous(blockIndices, blockSize, streamSize))
				{
					// fast path, all block indices are contiguous, so we don't have to copy any data at all.
					// instead, we directly point into the memory-mapped file at the correct offset.
					const uint32_t index = blockIndices[0];
					const size_t fileOffset = PDB::ConvertBlockIndexToFileOffset(index, blockSize);
					data_ = Pointer::Offset<const Byte*>(data, fileOffset);
				}
				else
				{
					// slower path, we need to copy disjunct blocks into our own data array, block by block
					ownedData_ = new(Byte[streamSize]);
					data_ = ownedData_;

					Byte* destination = ownedData_;

					// copy full blocks first
					const uint32_t fullBlockCount = streamSize / blockSize;
					for (uint32_t i = 0u; i < fullBlockCount; ++i)
					{
						const uint32_t index = blockIndices[i];

						// read one single block at the correct offset in the stream
						const size_t fileOffset = PDB::ConvertBlockIndexToFileOffset(index, blockSize);
						const void* sourceData = Pointer::Offset<const void*>(data, fileOffset);
						std::memcpy(destination, sourceData, blockSize);

						destination += blockSize;
					}

					// account for non-full blocks
					const uint32_t remainingBytes = streamSize - (fullBlockCount * blockSize);
					if (remainingBytes != 0u)
					{
						const uint32_t index = blockIndices[fullBlockCount];

						// read remaining bytes at correct offset in the stream
						const size_t fileOffset = PDB::ConvertBlockIndexToFileOffset(index, blockSize);
						const void* sourceData = Pointer::Offset<const void*>(data, fileOffset);
						std::memcpy(destination, sourceData, remainingBytes);
					}
				}
			}

			~CoalescedMSFStream()
			{
				delete[] ownedData_;
			}

			// Returns the size of the stream.
			inline size_t GetSize() const
			{
				return size_;
			}

			// Provides read-only access to the data.
			template <typename T>
			inline const T* GetDataAtOffset(size_t offset) const
			{
				return reinterpret_cast<const T*>(data_ + offset);
			}

			template <typename T>
			inline size_t GetPointerOffset(const T* pointer) const
			{
				const Byte* bytePointer = reinterpret_cast<const Byte*>(pointer);
				const Byte* dataEnd = data_ + size_;

				//_ASSERT(bytePointer >= data_ && bytePointer <= dataEnd,
				//           "Pointer 0x%016" PRIXPTR " not within stream range [0x%016" PRIXPTR ":0x%016" PRIXPTR "]",
				//           reinterpret_cast<uintptr_t>(bytePointer), reinterpret_cast<uintptr_t>(m_data),
				//           reinterpret_cast<uintptr_t>(dataEnd));

				return static_cast<size_t>(bytePointer - data_);
			}

		private:
			// contiguous, coalesced data, can be null
			Byte* ownedData_;

			// either points to the owned data that has been copied from disjunct blocks, or points to the
			// memory-mapped data directly in case all stream blocks are contiguous.
			const Byte* data_;
			size_t size_;
		};

		template <typename T>
		inline const T* GetDataAtOffset(size_t offset) const
		{
			return reinterpret_cast<const T*>(data_ + offset);
		}

		// Returns the size of the stream with the given index, taking into account nil page sizes.
		inline uint32_t GetStreamSize(uint32_t streamIndex) const 
		{
			const uint32_t streamSize = stream_sizes_[streamIndex];

			return (streamSize == 0xffffffffu) ? 0u : streamSize;
		}

		template <typename T>
		T CreateMSFStream(uint32_t streamIndex)
		{
			return T(data_, super_block_->blockSize, stream_blocks_[streamIndex], GetStreamSize(streamIndex));
		}

		BOOL Open(const CString& path)
		{
			BOOL bRet = FALSE;
			do
			{
				if (!file_.Open(path))
				{
					break;
				}

				if (0 == file_.GetSize())
				{
					break;
				}

				data_ = new Byte[file_.GetSize()];
				if (NULL == data_)
				{
					break;
				}
				if (!file_.Read((LPVOID)data_, file_.GetSize()))
				{
					break;
				}

				super_block_ = Pointer::Offset<const SuperBlock*>(data_, 0u);
				if (std::memcmp(super_block_->fileMagic, SuperBlock::MAGIC, sizeof(SuperBlock::MAGIC)) != 0)
				{
					break;
				}
				if (super_block_->freeBlockMapIndex != 1u && super_block_->freeBlockMapIndex != 2u)
				{
					break;
				}

				// the SuperBlock stores an array of indices of blocks that make up the indices of directory blocks, which need to be stitched together to form the directory.
				// the blocks holding the indices of directory blocks are not necessarily contiguous, so they need to be coalesced first.
				const uint32_t directoryBlockCount = ConvertSizeToBlockCount(
					super_block_->directorySize, super_block_->blockSize);
				// the directory is made up of directoryBlockCount blocks, so we need that many indices to be read from the blocks that make up the indices
				CoalescedMSFStream directoryIndicesStream(data_, super_block_->blockSize, super_block_->directoryBlockIndices, directoryBlockCount * sizeof(uint32_t));
				// these are the indices of blocks making up the directory stream, now guaranteed to be contiguous
				const uint32_t* directoryIndices = directoryIndicesStream.GetDataAtOffset<uint32_t>(0u);
				directory_stream_ = CoalescedMSFStream(data_, super_block_->blockSize, directoryIndices, super_block_->directorySize);
				// https://llvm.org/docs/PDB/MsfFile.html#the-stream-directory
				// parse the directory from its contiguous version. the directory matches the following struct:
				//	struct StreamDirectory
				//	{
				//		uint32_t streamCount;
				//		uint32_t streamSizes[streamCount];
				//		uint32_t streamBlocks[streamCount][];
				//	};
				stream_count_ = *directory_stream_.GetDataAtOffset<uint32_t>(0u);

				// we can assign pointers into the stream directly, since the RawFile keeps ownership of the directory stream
				stream_sizes_ = directory_stream_.GetDataAtOffset<uint32_t>(sizeof(uint32_t));
				const uint32_t* directoryStreamBlocks = directory_stream_.GetDataAtOffset<uint32_t>(sizeof(uint32_t) + sizeof(uint32_t) * stream_count_);

				// prepare indices for directly accessing individual streams
				stream_blocks_ = new const uint32_t*[stream_count_];
				const uint32_t* indicesForCurrentBlock = directoryStreamBlocks;
				for (uint32_t i = 0u; i < stream_count_; ++i)
				{
					const uint32_t sizeInBytes = GetStreamSize(i);
					const uint32_t blockCount = ConvertSizeToBlockCount(sizeInBytes, super_block_->blockSize);
					stream_blocks_[i] = indicesForCurrentBlock;
					
					indicesForCurrentBlock += blockCount;
				}

				info_stream_ = CreateMSFStream<CoalescedMSFStream>(InfoStreamIndex);
				header_ = info_stream_.GetDataAtOffset<const Header>(0u);
				bRet = TRUE;
			}while (false);

			return bRet;
		}

		void Close()
		{
			file_.Close();
			delete data_;
		}

		inline const Header* GetHeader() const
		{
			return header_;
		}

		inline const uint32_t GetStremCount() const
		{
			return stream_count_;
		}

		inline const uint32_t GetBlockSize() const
		{
			return super_block_->blockSize;
		}

		inline const uint32_t GetBlockCount() const
		{
			return super_block_->blockCount;
		}

		// the info stream starts with the header, followed by the named stream map, followed by the feature codes
		// https://llvm.org/docs/PDB/PdbStream.html#named-stream-map
		inline const uint32_t* GetFeature(size_t* count) const
		{
			size_t streamOffset = sizeof(Header);
			const NamedStreamMap* namedStreamMap = info_stream_.GetDataAtOffset<const NamedStreamMap>(streamOffset);
			streamOffset += sizeof(NamedStreamMap) + namedStreamMap->length;
			const SerializedHashTable::Header* hashTableHeader = info_stream_.GetDataAtOffset<const SerializedHashTable::Header>(streamOffset);
			streamOffset += sizeof(SerializedHashTable::Header);
			const SerializedHashTable::BitVector* presentBitVector = info_stream_.GetDataAtOffset<const SerializedHashTable::BitVector>(streamOffset);
			streamOffset += sizeof(SerializedHashTable::BitVector) + sizeof(uint32_t) * presentBitVector->wordCount;
			const SerializedHashTable::BitVector* deletedBitVector = info_stream_.GetDataAtOffset<const SerializedHashTable::BitVector>(streamOffset);
			streamOffset += sizeof(SerializedHashTable::BitVector) + sizeof(uint32_t) * deletedBitVector->wordCount;

			streamOffset += sizeof(NamedStreamMap::HashTableEntry) * hashTableHeader->size;

			// read feature codes by consuming remaining bytes
			// https://llvm.org/docs/PDB/PdbStream.html#pdb-feature-codes
			const FeatureCode* featureCodes = info_stream_.GetDataAtOffset<const FeatureCode>(streamOffset);
			const size_t remainingBytes = info_stream_.GetSize() - streamOffset;
			const size_t cnt = remainingBytes / sizeof(FeatureCode);
			*count = cnt;
			return reinterpret_cast<const uint32_t*>(featureCodes);
		}

	private:
		const Header* header_;
		CFile file_;
		const Byte* data_;
		const SuperBlock* super_block_;
		CoalescedMSFStream info_stream_;
		CoalescedMSFStream directory_stream_;

		// stream directory
		uint32_t stream_count_;
		const uint32_t* stream_sizes_;
		const uint32_t** stream_blocks_;
	};
}
