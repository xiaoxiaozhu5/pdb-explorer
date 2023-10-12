#pragma once
#include <stdint.h>

namespace PDB
{
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
		uint32_t freeBlockMapIndex;						// index of the free block map
		uint32_t blockCount;							// number of blocks in the file
		uint32_t directorySize;							// size of the stream directory in bytes
		uint32_t unknown;
		uint32_t directoryBlockIndices[0];				// indices of the blocks that make up the directory indices
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
#pragma pack(pop)

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

		// Offsets any pointer by a given number of bytes.
		template <typename T, typename U, typename V>
		inline T Offset(U* anyPointer, V howManyBytes)
		{
			static_assert(std::is_pointer<T>::value == true, "Type T must be a pointer type.");
			static_assert(std::is_const<typename std::remove_pointer<T>::type>::value == std::is_const<U>::value,
				"Wrong constness.");

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

		template <typename T>
		inline const T* GetDataAtOffset(size_t offset) const
		{
			return reinterpret_cast<const T*>(data_ + offset);
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

				const SuperBlock* superBlock = Offset<const SuperBlock*>(data_, 0u);
				if (std::memcmp(superBlock->fileMagic, SuperBlock::MAGIC, sizeof(SuperBlock::MAGIC)) != 0)
				{
					break;
				}
				if (superBlock->freeBlockMapIndex != 1u && superBlock->freeBlockMapIndex != 2u)
				{
					break;
				}
				bRet = TRUE;
			}
			while (false);
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


	private:
		const Header* header_;
		CFile file_;
		const Byte* data_;
	};
}
