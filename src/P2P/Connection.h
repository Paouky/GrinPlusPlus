#pragma once

#include "Messages/Message.h"

#include <Core/Enums/ProtocolVersion.h>
#include <Common/ConcurrentQueue.h>
#include <BlockChain/BlockChainServer.h>
#include <Net/Socket.h>
#include <P2P/ConnectedPeer.h>
#include <Config/Config.h>
#include <atomic>
#include <queue>

// Forward Declarations
class IMessage;
class ConnectionManager;
class HandShake;
class MessageProcessor;
class MessageRetriever;

//
// A Connection will be created for each ConnectedPeer.
// Each Connection will run on its own thread, and will watch the socket for messages,
// and will ping the peer when it hasn't been heard from in a while.
//
class Connection : public Traits::IPrintable
{
public:
	Connection(
		const Config& config,
		const SocketPtr& pSocket,
		const uint64_t connectionId,
		ConnectionManager& connectionManager,
		const ConnectedPeer& connectedPeer,
		SyncStatusConstPtr pSyncStatus,
		std::shared_ptr<HandShake> pHandShake,
		const std::weak_ptr<MessageProcessor>& pMessageProcessor
	);
	Connection(const Connection&) = delete;
	Connection& operator=(const Connection&) = delete;
	Connection(Connection&&) = delete;
	~Connection();

	static std::shared_ptr<Connection> Create(
		const SocketPtr& pSocket,
		const uint64_t connectionId,
		const Config& config,
		ConnectionManager& connectionManager,
		IBlockChainServerPtr pBlockChainServer,
		const ConnectedPeer& connectedPeer,
		const std::weak_ptr<MessageProcessor>& pMessageProcessor,
		SyncStatusConstPtr pSyncStatus
	);

	void Disconnect(const bool wait = false);

	uint64_t GetId() const { return m_connectionId; }
	bool IsConnectionActive() const;

	void AddToSendQueue(const IMessage& message);
	bool SendMsg(const IMessage& message);
	bool ExceedsRateLimit() const;
	void BanPeer(const EBanReason reason);

	SocketPtr GetSocket() const { return m_pSocket; }
	PeerPtr GetPeer() { return m_connectedPeer.GetPeer(); }
	PeerConstPtr GetPeer() const { return m_connectedPeer.GetPeer(); }
	const ConnectedPeer& GetConnectedPeer() const { return m_connectedPeer; }
	const IPAddress& GetIPAddress() const { return GetPeer()->GetIPAddress(); }
	uint64_t GetTotalDifficulty() const { return m_connectedPeer.GetTotalDifficulty(); }
	uint64_t GetHeight() const { return m_connectedPeer.GetHeight(); }
	Capabilities GetCapabilities() const { return m_connectedPeer.GetPeer()->GetCapabilities(); }
	EProtocolVersion GetProtocolVersion() const noexcept { return GetPeer()->GetVersion() > 1 ? EProtocolVersion::V2 : EProtocolVersion::V1; }
	void UpdateTotals(const uint64_t totalDifficulty, const uint64_t height) { m_connectedPeer.UpdateTotals(totalDifficulty, height); }

	std::string Format() const final { return "Connection{" + GetIPAddress().Format() + "}"; }

private:
	static void Thread_ProcessConnection(std::shared_ptr<Connection> pConnection);

	const Config& m_config;
	ConnectionManager& m_connectionManager;
	SyncStatusConstPtr m_pSyncStatus;

	std::shared_ptr<HandShake> m_pHandShake;
	std::weak_ptr<MessageProcessor> m_pMessageProcessor;

	std::atomic<bool> m_terminate;
	std::thread m_connectionThread;
	const uint64_t m_connectionId;

	ConnectedPeer m_connectedPeer;

	std::shared_ptr<asio::io_context> m_pContext;
	mutable SocketPtr m_pSocket;

	ConcurrentQueue<IMessagePtr> m_sendQueue;
};

typedef std::shared_ptr<Connection> ConnectionPtr;