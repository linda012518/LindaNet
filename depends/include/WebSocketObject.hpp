#ifndef _WebSocketObject_HPP_
#define _WebSocketObject_HPP_

#include <cstdint>

namespace linda {
	namespace io {
		//操作码
		enum WebsocketOpcode {
			/*

			%x0：表示一个延续帧。当 Opcode 为 0 时，表示本次数据传输采用了数据分片，当前收到的数据帧为其中一个数据分片。
			%x1：表示这是一个文本帧（frame）
			%x2：表示这是一个二进制帧（frame）
			%x3-7：保留的操作代码，用于后续定义的非控制帧。
			%x8：表示连接断开。
			%x8：表示这是一个 ping 操作。
			%xA：表示这是一个 pong 操作。
			%xB-F：保留的操作代码，用于后续定义的控制帧。

			*/
			Continuation = 0x0,
			Text = 0x1,
			Binary = 0x2,

			Close = 0x8,
			Ping = 0x9,
			Pong = 0xA,
		};

		struct WebsocketHeader {
			uint64_t len;//真实长度	 可变长度2或8个字节		这些长度不包括 mask-key
			WebsocketOpcode opcode;//操作码
			uint8_t masking_key[4];//Mask-key 1是4字节 0是0字节

			//x 为 0~126：数据的长度为 x 字节。
			//x 为 126：后续 2 个字节代表一个 16 位的无符号整数，该无符号整数的值为数据的长度。
			//x 为 127：后续 8 个字节代表一个 64 位的无符号整数（最高位为 0），该无符号整数的值为数据的长度。
			//此外，如果 payload length 占用了多个字节的话，payload length 的二进制表达采用 网络序（big endian，重要的位在前）。
			//这些长度不包括 mask-key
			uint8_t len0;
			uint8_t header_size;//头的大小      有可能2字节，有可能4字节，有可能10字节
			bool fin;//第一个bit 是否为消息的最后一个分片
			bool mask;//是否要掩码操作
		};
	}
}
#endif // !_WebSocketObject_HPP_