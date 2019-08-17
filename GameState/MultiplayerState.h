#ifndef MULTIPLAYER_STATE_INCLUDED
#define MULTIPLAYER_STATE_INCLUDED


#include "MultiplayerGameState.h"
#include "BaseState.h"

#include "..\coreutilities\containers.h"
#include "..\coreutilities\EngineCore.h"
#include "..\coreutilities\GameFramework.h"
#include "..\coreutilities\memoryutilities.h"
#include "..\coreutilities\Components.h"
#include "..\graphicsutilities\GuiUtilities.h"

#include <functional>
#include <steam\isteamnetworkingsockets.h>
#include <steam\isteamnetworkingutils.h>

using FlexKit::EngineCore;
using FlexKit::UpdateDispatcher;
using FlexKit::GameFramework;


/************************************************************************************************/

// Manages the lifespan of the packet
class Packet
{
public:
    Packet(void* IN_data = nullptr, size_t IN_size = 0, void* IN_sender = nullptr, iAllocator* IN_allocator = nullptr) :
        dataSize    { IN_size       },
        data        { IN_data       },
        sender      { IN_sender     },
        allocator   { IN_allocator  } {}


    ~Packet() { Release(); }

    // No Copy!
    Packet(const Packet&)                   = delete;
    const Packet operator = (const Packet&) = delete;


    Packet(Packet&& rhs) noexcept
    {
        if (data)
            Release();

        data        = rhs.data;
        sender      = rhs.sender;
        allocator   = rhs.allocator;

        rhs.Clear();
    }


    Packet& operator = (Packet&& rhs) noexcept// Move
    {
        if (data)
            Release();

        data        = rhs.data;
        sender      = rhs.sender;
        allocator   = rhs.allocator;

        rhs.Clear();

        return *this;
    }

    void Release()
    {
        if (data)
        {
            allocator->free(data);
            allocator->free(sender);
            Clear();
        }
    }


    void Clear()
    {
        data        = nullptr;
        sender      = nullptr;
        allocator   = nullptr;
    }


    static Packet CopyCreate(void* data, size_t data_size, void* sender, iAllocator* allocator)
    {
        auto buffer = (char*)allocator->malloc(data_size);
        memcpy(buffer, data, data_size);

        return { data, data_size, sender, allocator };
    }

    void*       data        = nullptr;
    size_t      dataSize    = 0;
    void*       sender      = nullptr;
    iAllocator* allocator   = nullptr;
};


/************************************************************************************************/
 

typedef size_t PacketID_t;


enum EBasePacketIDs : unsigned char
{
	EBP_USERPACKET,
	EBP_COUNT,
};


class UserPacketHeader
{
public:
	UserPacketHeader(const size_t IN_size, PacketID_t IN_id) :
		EBasePacketIDs	{ EBP_USERPACKET	},
		packetSize		{ IN_size			},
		id				{ IN_id				} {}

	PacketID_t GetID() 
	{ 
		return id;
	}

	unsigned char		EBasePacketIDs;
	const size_t		packetSize;
	const PacketID_t	id;
};


/************************************************************************************************/


class NetworkState;

class PacketHandler
{
public:
	PacketHandler(PacketID_t IN_id) :
		packetTypeID{ IN_id } {}

	const PacketID_t packetTypeID;

	virtual ~PacketHandler() {};
	virtual void HandlePacket(UserPacketHeader* incomingPacket, void* packet, NetworkState* network) = 0;
};


/************************************************************************************************/


template<typename FN_TY>
class LambdaPacketHandler : public PacketHandler
{
public:
	LambdaPacketHandler(
		PacketID_t IN_id, 
		FN_TY&& IN_FN) :
			PacketHandler	{IN_id}, 
			_FN				{std::move(IN_FN)}{}

	void HandlePacket(
			UserPacketHeader*	header, 
			void*				packet, 
			NetworkState*		network) override
	{
		_FN(header, packet, network);
	}

	FN_TY _FN;
};

template<typename FN_TY>
LambdaPacketHandler<FN_TY>* CreatePacketHandler(PacketID_t IN_id, FN_TY&& FN, FlexKit::iAllocator* allocator)
{
	return &allocator->allocate_aligned<LambdaPacketHandler<FN_TY>>(IN_id, std::move(FN));
}


/************************************************************************************************/


class NetworkState : public FlexKit::FrameworkState
{
protected:
    void PushIncomingPacket(Packet&& packet)
    {
       incomingPackets.emplace_back(std::move(packet));
    }


    class SocketListenerThread
    {
    public:
        SocketListenerThread(NetworkState& IN_network) :
            network         { IN_network    } {}

        void StartListener()
        {
            if (running)
                return;

            if (!network.steamSockets) // steam sockets not inialized!
                return;

            running = true;

            // create socket
            SteamNetworkingIPAddr listeningPort;
            listeningPort.Clear();
            listeningPort.m_port = network.port;

            listeningSocket = network.steamSockets->CreateListenSocketIP(listeningPort);

            if (listeningSocket == k_HSteamNetConnection_Invalid)
            {
                FK_ASSERT(false, "Failed to create valid listening socket!");
                throw std::exception("Failed to create valid listening socket!");
            }

            workerThread = std::move(std::thread{ [&] { Listen(); } });
        }


        void Join()
        {
            if(workerThread.joinable())
                workerThread.join();
        }


        void Listen()
        {
            while (network.running)
            {
                PollIncomingMessages();
                PollConnectionStateChanges();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            running = false;
        }

        void PollIncomingMessages()
        {
            while (true)
            {
                ISteamNetworkingMessage* pIncomingMsg = nullptr;
                const size_t messageCount = network.steamSockets->ReceiveMessagesOnListenSocket(listeningSocket, &pIncomingMsg, 1);

                if (!messageCount)
                    return;

                network.PushIncomingPacket(
                    Packet::CopyCreate(
                        pIncomingMsg->m_pData,
                        pIncomingMsg->m_cbSize,
                        nullptr,
                        network.framework->core->GetBlockMemory()));

                pIncomingMsg->Release();
            }

        }

        void PollConnectionStateChanges()
        {

        }

    private:
        HSteamListenSocket      listeningSocket;
        bool                    running = false;
        NetworkState&           network;
        std::thread             workerThread;
    };

public:
	NetworkState(
		GameFramework*	IN_framework, 
		BaseState*		IN_base) :
			FrameworkState	{ IN_framework                          },
			handlerStack	{ IN_framework->core->GetBlockMemory()  },
            incomingPackets { IN_framework->core->GetBlockMemory()  },
            socketListener  { *this                                 }
    {
        SteamNetworkingSockets();
        SteamDatagramErrMsg errMsg;
        FK_ASSERT(GameNetworkingSockets_Init(nullptr, errMsg), "Unable to initialize Steam Sockets!");

        socketListener.StartListener();
    }


	~NetworkState()
    {
        while (incomingPackets.size())
        {
            auto packet = incomingPackets.pop_front();
            packet.Release();
        }

        GameNetworkingSockets_Kill();
    }


    /************************************************************************************************/


	bool Update(FlexKit::EngineCore* Engine, FlexKit::UpdateDispatcher& Dispatcher, double dT) override
	{
		// Handle incoming Packets
        while (incomingPackets.size())
        {
            auto                packet = incomingPackets.pop_front();
            UserPacketHeader*   header = reinterpret_cast<UserPacketHeader*>(packet.data);

            const auto packetID = header->GetID();
            for (auto handler : *handlerStack.back())
            {
                if (handler->packetTypeID == packetID)
                    handler->HandlePacket(header, &packet, this);
            }
        }

		return true;
	}


	/************************************************************************************************/


	void SendPacket(UserPacketHeader* packet)
	{
	}


	/************************************************************************************************/


	void PushHandler(Vector<PacketHandler*>* handler)
	{
		handlerStack.push_back(handler);
	}


	/************************************************************************************************/


	void PopHandler()
	{
		handlerStack.pop_back();
	}


	/************************************************************************************************/

    SL_list<Packet>                     incomingPackets;
    SocketListenerThread                socketListener;
    ISteamNetworkingSockets*            steamSockets = nullptr;   
	Vector<Vector<PacketHandler*>*>		handlerStack;

    uint16_t                            port;
    bool                                running;
};


/************************************************************************************************/


struct GameDescription
{
	size_t MaxPlayerCount	= 4;
	short  Port				= 1337;
};


/************************************************************************************************/


enum UserPacketIDs : PacketID_t
{
	ClientDataRequest,
	ClientDataRequestResponse,
	ClientDisconnect,
	ClientReadyEvent,
	LobbyMessage,
	UserPacketIDCount,
};


/**********************************************************************

Copyright (c) 2019 Robert May

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**********************************************************************/

#endif
