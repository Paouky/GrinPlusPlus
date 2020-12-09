#include "TorControl.h"
#include "TorControlClient.h"

#include <Net/Tor/TorException.h>
#include <Net/Tor/TorAddressParser.h>
#include <Common/Util/StringUtil.h>
#include <cppcodec/base64_rfc4648.hpp>
#include <Crypto/ED25519.h>
#include <filesystem.h>

TorControl::TorControl(const TorConfig& config, std::unique_ptr<TorControlClient>&& pClient, ChildProcess::UCPtr&& pProcess)
	: m_torConfig(config), m_pClient(std::move(pClient)), m_pProcess(std::move(pProcess))
{

}

TorControl::~TorControl()
{
	if (m_pClient != nullptr) {
		try
		{
			m_pClient->Invoke("SIGNAL DUMP\n");
		} catch (std::exception&) { }
	}
}

std::shared_ptr<TorControl> TorControl::Create(const TorConfig& torConfig) noexcept
{
	try
	{
		auto pClient = TorControlClient::Connect(torConfig.GetControlPort(), torConfig.GetControlPassword());
		if (pClient != nullptr) {
			return std::make_unique<TorControl>(torConfig, std::move(pClient), nullptr);
		}
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Exception caught: {}", e.what());
	}

	return nullptr;
}

std::string TorControl::AddOnion(const ed25519_secret_key_t& secretKey, const uint16_t externalPort, const uint16_t internalPort)
{
	TorAddress tor_address = TorAddressParser::FromPubKey(ED25519::CalculatePubKey(secretKey));
	std::vector<std::string> hidden_services = QueryHiddenServices();
	for (const std::string& service : hidden_services)
	{
		if (service == tor_address.ToString()) {
			LOG_INFO_F("Hidden service already running for onion address {}", tor_address.ToString());
			return service;
		}
	}

	// "ED25519-V3" key is the Base64 encoding of the concatenation of
	// the 32-byte ed25519 secret scalar in little-endian
	// and the 32-byte ed25519 PRF secret.
	std::string serializedKey = cppcodec::base64_rfc4648::encode(ED25519::CalculateTorKey(secretKey).GetVec());

	// ADD_ONION ED25519-V3:<SERIALIZED_KEY> PORT=External,Internal
	std::string command = StringUtil::Format(
		"ADD_ONION ED25519-V3:{} Flags=DiscardPK,Detach Port={},{}\n",
		serializedKey,
		externalPort,
		internalPort
	);

	std::vector<std::string> response = m_pClient->Invoke(command);
	for (GrinStr line : response)
	{
		if (line.StartsWith("250-ServiceID=")) {
			const size_t prefix = std::string("250-ServiceID=").size();
			return line.substr(prefix);
		}
	}

	throw TOR_EXCEPTION("Address not returned");
}

bool TorControl::DelOnion(const TorAddress& torAddress)
{
	// DEL_ONION ServiceId
	std::string command = StringUtil::Format("DEL_ONION {}\n", torAddress.ToString());

	m_pClient->Invoke(command);

	return true;
}

bool TorControl::CheckHeartbeat()
{
	// 250-status/bootstrap-phase=NOTICE BOOTSTRAP PROGRESS=100 TAG=done SUMMARY="Done"

	// GETINFO status/clients-seen
	// GETINFO current-time/local

	try
	{
		m_pClient->Invoke("SIGNAL DUMP\n");
		m_pClient->Invoke("SIGNAL HEARTBEAT\n");
	}
	catch (std::exception&)
	{
		return false;
	}

	// SIGNAL HEARTBEAT
	//std::string command = "GETINFO onions/detached\n";
	return true;
}

std::vector<std::string> TorControl::QueryHiddenServices()
{
	// GETINFO onions/detached
	std::string command = "GETINFO onions/detached\n";

	std::vector<std::string> response = m_pClient->Invoke(command);
	
	std::vector<std::string> addresses;
	bool listing_addresses = false;
	for (GrinStr line : response)
	{
		if (line.Trim() == "250 OK" || line.Trim() == ".") {
			break;
		} else if (line.StartsWith("250+onions/detached=")) {
			listing_addresses = true;

			if (line.Trim() != "250+onions/detached=") {
				auto parts = line.Split("250+onions/detached=");
				if (parts.size() == 2) {
					addresses.push_back(parts[1].Trim());
				}
			}
		} else if (listing_addresses) {
			addresses.push_back(line.Trim());
		}
	}

	return addresses;
}