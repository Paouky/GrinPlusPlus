#include "NodeRestServer.h"
#include "civetweb/include/civetweb.h"

#include "API/HeaderAPI.h"
#include "API/BlockAPI.h"
#include "API/ServerAPI.h"
#include "API/ChainAPI.h"
#include "API/PeersAPI.h"
#include "API/TxHashSetAPI.h"

#include "API/Explorer/BlockInfoAPI.h"

#include <P2P/P2PServer.h>
#include <BlockChain/BlockChainServer.h>
#include <Database/Database.h>

NodeRestServer::NodeRestServer(const Config& config, IDatabase* pDatabase, TxHashSetManager* pTxHashSetManager, IBlockChainServer* pBlockChainServer, IP2PServer* pP2PServer)
	: m_config(config), m_pDatabase(pDatabase), m_pTxHashSetManager(pTxHashSetManager), m_pBlockChainServer(pBlockChainServer), m_pP2PServer(pP2PServer)
{
	m_serverContainer.m_pDatabase = m_pDatabase;
	m_serverContainer.m_pBlockChainServer = m_pBlockChainServer;
	m_serverContainer.m_pP2PServer = m_pP2PServer;
	m_serverContainer.m_pTxHashSetManager = m_pTxHashSetManager;
}

bool NodeRestServer::Start()
{
	/* Initialize the library */
	mg_init_library(0);

	/* Start the server */

	const uint32_t port = m_config.GetServerConfig().GetRestAPIPort();
	const char* mg_options[] = {
		"num_threads", "5",
		"listening_ports", std::to_string(port).c_str(),
		NULL
	};
	ctx = mg_start(NULL, 0, mg_options);

	/* Add handlers */
	mg_set_request_handler(ctx, "/v1/explorer/blockinfo/", BlockInfoAPI::GetBlockInfo_Handler, &m_serverContainer);

	mg_set_request_handler(ctx, "/v1/status", ServerAPI::GetStatus_Handler, &m_serverContainer);
	mg_set_request_handler(ctx, "/v1/headers/", HeaderAPI::GetHeader_Handler, m_pBlockChainServer);
	mg_set_request_handler(ctx, "/v1/blocks/", BlockAPI::GetBlock_Handler, m_pBlockChainServer);
	mg_set_request_handler(ctx, "/v1/chain/outputs/byids", ChainAPI::GetChainOutputsByIds_Handler, &m_serverContainer);
	mg_set_request_handler(ctx, "/v1/chain/outputs/byheight", ChainAPI::GetChainOutputsByHeight_Handler, &m_serverContainer);
	mg_set_request_handler(ctx, "/v1/chain", ChainAPI::GetChain_Handler, m_pBlockChainServer);
	mg_set_request_handler(ctx, "/v1/peers/all", PeersAPI::GetAllPeers_Handler, &m_serverContainer);
	mg_set_request_handler(ctx, "/v1/peers/connected", PeersAPI::GetConnectedPeers_Handler, &m_serverContainer);
	mg_set_request_handler(ctx, "/v1/peers/", PeersAPI::Peer_Handler, &m_serverContainer);
	mg_set_request_handler(ctx, "/v1/txhashset/roots", TxHashSetAPI::GetRoots_Handler, &m_serverContainer);
	mg_set_request_handler(ctx, "/v1/txhashset/lastkernels", TxHashSetAPI::GetLastKernels_Handler, &m_serverContainer);
	mg_set_request_handler(ctx, "/v1/txhashset/lastoutputs", TxHashSetAPI::GetLastOutputs_Handler, &m_serverContainer);
	mg_set_request_handler(ctx, "/v1/txhashset/lastrangeproofs", TxHashSetAPI::GetLastRangeproofs_Handler, &m_serverContainer);
	mg_set_request_handler(ctx, "/v1/txhashset/outputs", TxHashSetAPI::GetOutputs_Handler, &m_serverContainer);
	mg_set_request_handler(ctx, "/v1/", ServerAPI::V1_Handler, &m_serverContainer);

	/*
	// TODO: Still missing
		"get pool",
		"post pool/push"
		"post chain/compact"
		"post chain/validate"
	*/

	return true;
}

bool NodeRestServer::Shutdown()
{
	/* Stop the server */
	mg_stop(ctx);

	/* Un-initialize the library */
	mg_exit_library();

	return true;
}