/*
Copyright (c) 2006-2017 by Jakob Schröter <js@camaya.net>
This file is part of the gloox library. http://camaya.net/gloox

This software is distributed under a license. The full license
agreement can be found in the file LICENSE in this distribution.
This software may not be copied, modified, sold or distributed
other than expressed in the named license agreement.

This software is distributed without any warranty.
*/

#ifndef SHA_H__
#define SHA_H__

#include <string>

namespace gloox
{

	/**
	* @brief An implementation of SHA1.
	*
	* @author Jakob Schröter <js@camaya.net>
	* @since 0.9
	*/
	class SHA
	{

	public:
		/**
		* Constructs a new SHA object.
		*/
		SHA();

		/**
		* Virtual Destructor.
		*/
		virtual ~SHA();

		/**
		* Resets the internal state.
		*/
		void reset();

		/**
		* Finalizes the hash computation.
		*/
		void finalize();

		/**
		* Returns the message digest in hex notation. Finalizes the hash if finalize()
		* has not been called before.
		* @return The message digest.
		*/
		const std::string hex();

		/**
		* Returns the raw binary message digest. Finalizes the hash if finalize()
		* has not been called before.
		* @return The message raw binary digest.
		*/
		const std::string binary();

		/**
		* Provide input to SHA1.
		* @param data The data to compute the digest of.
		* @param length The size of the data in bytes.
		*/
		void feed(const unsigned char* data, unsigned length);

		/**
		* Provide input to SHA1.
		* @param data The data to compute the digest of.
		*/
		void feed(const std::string& data);

	private:
		void process();
		void pad();
		inline unsigned shift(int bits, unsigned word);
		void init();

		unsigned H[5];
		unsigned Length_Low;
		unsigned Length_High;
		unsigned char Message_Block[64];
		int Message_Block_Index;
		bool m_finished;
		bool m_corrupted;

	};

}


/*
Copyright (c) 2006-2017 by Jakob Schröter <js@camaya.net>
This file is part of the gloox library. http://camaya.net/gloox

This software is distributed under a license. The full license
agreement can be found in the file LICENSE in this distribution.
This software may not be copied, modified, sold or distributed
other than expressed in the named license agreement.

This software is distributed without any warranty.
*/

#include <cstdio>

namespace gloox
{

	SHA::SHA()
	{
		init();
	}

	SHA::~SHA()
	{
	}

	void SHA::init()
	{
		Length_Low = 0;
		Length_High = 0;
		Message_Block_Index = 0;

		H[0] = 0x67452301;
		H[1] = 0xEFCDAB89;
		H[2] = 0x98BADCFE;
		H[3] = 0x10325476;
		H[4] = 0xC3D2E1F0;

		m_finished = false;
		m_corrupted = false;
	}

	void SHA::reset()
	{
		init();
	}

	const std::string SHA::hex()
	{
		if (m_corrupted)
			return "";

		if (!m_finished)
			finalize();

		char buf[41];
		for (int i = 0; i < 20; ++i)
			sprintf(buf + i * 2, "%02x", static_cast<unsigned char>(H[i >> 2] >> ((3 - (i & 3)) << 3)));

		return std::string(buf, 40);
	}

	const std::string SHA::binary()
	{
		if (!m_finished)
			finalize();

		unsigned char digest[20];
		for (int i = 0; i < 20; ++i)
			digest[i] = static_cast<unsigned char>(H[i >> 2] >> ((3 - (i & 3)) << 3));

		return std::string(reinterpret_cast<char*>(digest), 20);
	}

	void SHA::finalize()
	{
		if (!m_finished)
		{
			pad();
			m_finished = true;
		}
	}

	void SHA::feed(const unsigned char* data, unsigned length)
	{
		if (!length)
			return;

		if (m_finished || m_corrupted)
		{
			m_corrupted = true;
			return;
		}

		while (length-- && !m_corrupted)
		{
			Message_Block[Message_Block_Index++] = (*data & 0xFF);

			Length_Low += 8;
			Length_Low &= 0xFFFFFFFF;
			if (Length_Low == 0)
			{
				Length_High++;
				Length_High &= 0xFFFFFFFF;
				if (Length_High == 0)
				{
					m_corrupted = true;
				}
			}

			if (Message_Block_Index == 64)
			{
				process();
			}

			++data;
		}
	}

	void SHA::feed(const std::string& data)
	{
		feed(reinterpret_cast<const unsigned char*>(data.c_str()), static_cast<int>(data.length()));
	}

	void SHA::process()
	{
		const unsigned K[] = { 0x5A827999,
			0x6ED9EBA1,
			0x8F1BBCDC,
			0xCA62C1D6
		};
		int t;
		unsigned temp;
		unsigned W[80];
		unsigned A, B, C, D, E;

		for (t = 0; t < 16; t++)
		{
			W[t] = static_cast<unsigned int>(Message_Block[t * 4]) << 24;
			W[t] |= static_cast<unsigned int>(Message_Block[t * 4 + 1]) << 16;
			W[t] |= static_cast<unsigned int>(Message_Block[t * 4 + 2]) << 8;
			W[t] |= static_cast<unsigned int>(Message_Block[t * 4 + 3]);
		}

		for (t = 16; t < 80; ++t)
		{
			W[t] = shift(1, W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16]);
		}

		A = H[0];
		B = H[1];
		C = H[2];
		D = H[3];
		E = H[4];

		for (t = 0; t < 20; ++t)
		{
			temp = shift(5, A) + ((B & C) | ((~B) & D)) + E + W[t] + K[0];
			temp &= 0xFFFFFFFF;
			E = D;
			D = C;
			C = shift(30, B);
			B = A;
			A = temp;
		}

		for (t = 20; t < 40; ++t)
		{
			temp = shift(5, A) + (B ^ C ^ D) + E + W[t] + K[1];
			temp &= 0xFFFFFFFF;
			E = D;
			D = C;
			C = shift(30, B);
			B = A;
			A = temp;
		}

		for (t = 40; t < 60; ++t)
		{
			temp = shift(5, A) + ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
			temp &= 0xFFFFFFFF;
			E = D;
			D = C;
			C = shift(30, B);
			B = A;
			A = temp;
		}

		for (t = 60; t < 80; ++t)
		{
			temp = shift(5, A) + (B ^ C ^ D) + E + W[t] + K[3];
			temp &= 0xFFFFFFFF;
			E = D;
			D = C;
			C = shift(30, B);
			B = A;
			A = temp;
		}

		H[0] = (H[0] + A) & 0xFFFFFFFF;
		H[1] = (H[1] + B) & 0xFFFFFFFF;
		H[2] = (H[2] + C) & 0xFFFFFFFF;
		H[3] = (H[3] + D) & 0xFFFFFFFF;
		H[4] = (H[4] + E) & 0xFFFFFFFF;

		Message_Block_Index = 0;
	}

	void SHA::pad()
	{
		Message_Block[Message_Block_Index++] = 0x80;

		if (Message_Block_Index > 56)
		{
			while (Message_Block_Index < 64)
			{
				Message_Block[Message_Block_Index++] = 0;
			}

			process();
		}

		while (Message_Block_Index < 56)
		{
			Message_Block[Message_Block_Index++] = 0;
		}

		Message_Block[56] = static_cast<unsigned char>((Length_High >> 24) & 0xFF);
		Message_Block[57] = static_cast<unsigned char>((Length_High >> 16) & 0xFF);
		Message_Block[58] = static_cast<unsigned char>((Length_High >> 8) & 0xFF);
		Message_Block[59] = static_cast<unsigned char>((Length_High) & 0xFF);
		Message_Block[60] = static_cast<unsigned char>((Length_Low >> 24) & 0xFF);
		Message_Block[61] = static_cast<unsigned char>((Length_Low >> 16) & 0xFF);
		Message_Block[62] = static_cast<unsigned char>((Length_Low >> 8) & 0xFF);
		Message_Block[63] = static_cast<unsigned char>((Length_Low) & 0xFF);

		process();
	}


	unsigned SHA::shift(int bits, unsigned word)
	{
		return ((word << bits) & 0xFFFFFFFF) | ((word & 0xFFFFFFFF) >> (32 - bits));
	}

}
namespace linda {
	namespace io {
		static int SHA1_String(const unsigned char* inputString, unsigned long len, unsigned char* pOutSHA1Buf)
		{
			if (!inputString || !pOutSHA1Buf)
				return -1;

			gloox::SHA sha1;
			sha1.feed(inputString, len);
			std::string s1 = sha1.binary();
			if (s1.length() != 20)
				return -2;

			memcpy(pOutSHA1Buf, s1.c_str(), 20);

			return 1;
		}
	}
}
#endif // SHA_H__
