#include "networked_multiplayer_webrtc.h"

#include "os/os.h"
#include "io/marshalls.h"

void NetworkedMultiplayerWebRTC::_bind_methods()
{
  ClassDB::bind_method(D_METHOD("create_server", "max_clients"), &NetworkedMultiplayerWebRTC::create_server);
  ClassDB::bind_method(D_METHOD("create_client"), &NetworkedMultiplayerWebRTC::create_client);
  ClassDB::bind_method(D_METHOD("create_offer"), &NetworkedMultiplayerWebRTC::create_offer);
  ClassDB::bind_method(D_METHOD("accept_client"), &NetworkedMultiplayerWebRTC::accept_client);
  ClassDB::bind_method(D_METHOD("set_unique_id", "peer_id"), &NetworkedMultiplayerWebRTC::set_unique_id);
  ClassDB::bind_method(D_METHOD("set_local_description", "sdp", "isOffer", "peer_id"), &NetworkedMultiplayerWebRTC::set_local_description);
  ClassDB::bind_method(D_METHOD("set_remote_description", "sdp", "isOffer", "peer_id"), &NetworkedMultiplayerWebRTC::set_remote_description);
  ClassDB::bind_method( // @TODO rename arguments: give them shorter names
    D_METHOD( "add_ice_candidate",
              "sdp_mid_name",
              "sdp_mline_index_name",
              "sdp_name",
              "peer_id"
    ), &NetworkedMultiplayerWebRTC::add_ice_candidate
  );
  ClassDB::bind_method(D_METHOD("_notify"), &_notify);
  ClassDB::bind_method(D_METHOD("_offer_created"), &_offer_created);
  ClassDB::bind_method(D_METHOD("_new_ice_candidate"), &_new_ice_candidate);

  ADD_SIGNAL(MethodInfo("notify",
                        PropertyInfo(Variant::STRING, "message"),
                        PropertyInfo(Variant::INT, "peer_id")
  ));
  ADD_SIGNAL(MethodInfo("offer_created",
                        PropertyInfo(Variant::STRING, "sdp"),
                        PropertyInfo(Variant::BOOL, "is_offer"),
                        PropertyInfo(Variant::INT, "peer_id")
  ));
  ADD_SIGNAL(MethodInfo("new_ice_candidate",
                        PropertyInfo(Variant::STRING, "sdp_mid_name"),
                        PropertyInfo(Variant::INT, "sdp_mline_index_name"),
                        PropertyInfo(Variant::STRING, "sdp_name"),
                        PropertyInfo(Variant::INT, "peer_id")
  ));
  ADD_SIGNAL(MethodInfo("client_accepted", PropertyInfo(Variant::INT, "peer_id")));
}

void NetworkedMultiplayerWebRTC::set_transfer_mode(TransferMode p_mode)
{
  transfer_mode = p_mode;
}

NetworkedMultiplayerPeer::TransferMode NetworkedMultiplayerWebRTC::get_transfer_mode() const
{
  return transfer_mode;
}

void NetworkedMultiplayerWebRTC::set_target_peer(int p_peer_id)
{
	target_peer = p_peer_id;
}

/* Returns the ID of the NetworkedMultiplayerPeer who sent the most recent packet: */
int NetworkedMultiplayerWebRTC::get_packet_peer() const
{
	// ERR_FAIL_COND_V(!active, 1);
	ERR_FAIL_COND_V(incoming_packets.size() == 0, 1);

	return incoming_packets.front()->get().from;
}

bool NetworkedMultiplayerWebRTC::is_server() const {
	// ERR_FAIL_COND_V(!active, false);

	return _is_server;
}

void NetworkedMultiplayerWebRTC::poll()
{
  if (connection_status == CONNECTION_CONNECTING && !_is_server && peer_map[1]->_is_active())
    connection_status = CONNECTION_CONNECTED;

	for (Map<int, WebRTCPeer *>::Element *E = peer_map.front(); E; E = E->next()) { // for each peer in peer_map:
    E->value()->poll(); // call peer.poll()
    if (E->value()->_is_active()) // if peer is active (found using peer.get_state_peer_connection(), but that function needs to be completed ):
      while(E->value()->get_available_packet_count() != 0)
      {
        const uint8_t *r_buffer;
        int r_buffer_size;
        E->value()->get_packet(&r_buffer, r_buffer_size);
        // const uint8_t *copied_buffer = (r_buffer); // hopefully this copies r_buffer

        incoming_packets.push_back(webrtc_packet_create(r_buffer, r_buffer_size, E->key())); //   add any packets from the peer to: incoming_packets (using incoming_packets.push_back())
      }
  }
}

// Returns the ID of the NetworkedMultiplayerPeer who sent the most recent packet:
int NetworkedMultiplayerWebRTC::get_unique_id() const
{
  return unique_id;
}

void NetworkedMultiplayerWebRTC::_notify(String message, int p_id)
{
  emit_signal("notify", message, p_id);
}

void NetworkedMultiplayerWebRTC::_offer_created(String sdp, bool isOffer, int p_id)
{
  emit_signal("offer_created", sdp, isOffer, p_id);
}

void NetworkedMultiplayerWebRTC::_new_ice_candidate(String candidateSdpMidName, int candidateSdpMlineIndexName, String candidateSdpName, int p_id)
{
  emit_signal("new_ice_candidate", candidateSdpMidName, candidateSdpMlineIndexName, candidateSdpName, p_id);
}

void NetworkedMultiplayerWebRTC::set_refuse_new_connections(bool p_enable)
{
  refuse_connections = p_enable;
}

bool NetworkedMultiplayerWebRTC::is_refusing_new_connections() const
{
  return refuse_connections;
}

NetworkedMultiplayerPeer::ConnectionStatus NetworkedMultiplayerWebRTC::get_connection_status() const
{
  return connection_status;
}

Error NetworkedMultiplayerWebRTC::create_server(int max_clients)
{
  connection_status = CONNECTION_CONNECTED;
  ERR_FAIL_COND_V(max_clients < 0, ERR_INVALID_PARAMETER);
  this->max_clients = max_clients;

  unique_id = 1;
  _is_server = true;
  return OK;
}

void NetworkedMultiplayerWebRTC::create_client()
{
	connection_status = CONNECTION_CONNECTING;
  _is_server = false;
  peer_map[1] = memnew(WebRTCPeer);
  connect_signals(peer_map[1], 1);
  // unique_id = _gen_unique_id();
  // return OK;
}

int NetworkedMultiplayerWebRTC::accept_client()
{
  // called by the user in response to a signal saying there's a new client
  int id = _gen_unique_id();
  ERR_FAIL_COND_V(client_count > max_clients, ERR_ALREADY_EXISTS);
  ERR_FAIL_COND_V(refuse_connections, ERR_UNAUTHORIZED);
  ++client_count;

  // uint32_t id = _gen_unique_id();
  WebRTCPeer* peer = memnew(WebRTCPeer);  // generate a new peer_connection
  peer_map[id] = peer;                    // add the new peer connection to the peer_map

  // connect all of the signals from the new WebRTCPeer to this NetworkedMultiplayerWebRTC:
  connect_signals(peer, id);

  emit_signal("client_accepted", id);
  return OK;
}

void NetworkedMultiplayerWebRTC::set_unique_id(int id)
{
  unique_id = id;
  emit_signal("notify", "unique_id set", id);
  std::cout << "setting id. id: " << unique_id << std::endl;
};

Error NetworkedMultiplayerWebRTC::set_remote_description(String sdp, bool isOffer, int peer_id)
{
  ERR_FAIL_COND_V(!peer_map.has(peer_id), ERR_DOES_NOT_EXIST)
  //call the NetworkedMultiplayerWebRTC function of the same name on the NetworkedMultiplayerWebRTC in the map with the given peer_id
  peer_map[peer_id]->set_remote_description(sdp, isOffer);
  return OK;
}

Error NetworkedMultiplayerWebRTC::set_local_description(String sdp, bool isOffer, int peer_id)
{
  ERR_FAIL_COND_V(!peer_map.has(peer_id), ERR_DOES_NOT_EXIST)
  //call the NetworkedMultiplayerWebRTC function of the same name on the NetworkedMultiplayerWebRTC in the map with the given peer_id
  peer_map[peer_id]->set_local_description(sdp, isOffer);
  return OK;
}

Error NetworkedMultiplayerWebRTC::add_ice_candidate(String sdpMidName, int sdpMlineIndexName, String sdpName, int peer_id)
{
  //call the NetworkedMultiplayerWebRTC function of the same name on the NetworkedMultiplayerWebRTC in the map with the given peer_id
  ERR_FAIL_COND_V(!peer_map.has(peer_id), ERR_DOES_NOT_EXIST)
  peer_map[peer_id]->add_ice_candidate(sdpMidName, sdpMlineIndexName, sdpName);
  return OK;
}

NetworkedMultiplayerWebRTC::Packet NetworkedMultiplayerWebRTC::webrtc_packet_create(const uint8_t* buffer, int buffer_size, int from)
{
  Packet packet;
  packet.buffer = buffer;
  packet.buffer_size = buffer_size;
  packet.from = from;
  return packet;
}

uint32_t NetworkedMultiplayerWebRTC::_gen_unique_id() const
{
  uint32_t hash = 0;

	while (hash == 0 || hash == 1) {

		hash = hash_djb2_one_32(
				(uint32_t)OS::get_singleton()->get_ticks_usec());
		hash = hash_djb2_one_32(
				(uint32_t)OS::get_singleton()->get_unix_time(), hash);
		hash = hash_djb2_one_32(
				(uint32_t)OS::get_singleton()->get_user_data_dir().hash64(), hash);
		hash = hash_djb2_one_32(
				(uint32_t)((uint64_t)this), hash); // Rely on ASLR heap
		hash = hash_djb2_one_32(
				(uint32_t)((uint64_t)&hash), hash); // Rely on ASLR stack

    hash = hash & 0x7FFFFFFF; // Make it compatible with unsigned, since negative ID is used for exclusion
	}

	return hash;
}

Error NetworkedMultiplayerWebRTC::get_packet(const uint8_t **r_buffer, int &r_buffer_size) {

	ERR_FAIL_COND_V(incoming_packets.size() == 0, ERR_UNAVAILABLE);

	_pop_current_packet();

	current_packet = incoming_packets.front()->get();
	incoming_packets.pop_front();

	*r_buffer = current_packet.buffer;
	r_buffer_size = current_packet.buffer_size;

	return OK;
}

Error NetworkedMultiplayerWebRTC::put_packet(const uint8_t *p_buffer, int p_buffer_size)
{
	// ERR_FAIL_COND_V(!active, ERR_UNCONFIGURED);
	// ERR_FAIL_COND_V(connection_status != CONNECTION_CONNECTED, ERR_UNCONFIGURED);

	// if (transfer_channel > SYSCH_CONFIG)
	// 	channel = transfer_channel;

	Map<int, WebRTCPeer *>::Element *E = NULL;

	if (target_peer != 0) {

		E = peer_map.find(ABS(target_peer));
		if (!E) {
			ERR_EXPLAIN("Invalid Target Peer: " + itos(target_peer));
			ERR_FAIL_V(ERR_INVALID_PARAMETER);
		}
	}

  int packet_flags = 0;

	// ENetPacket *packet = enet_packet_create(NULL, p_buffer_size + 12, packet_flags);

  // p_buffer_size += 12;
  uint8_t* put_buffer = new uint8_t[p_buffer_size];
	// encode_uint32(unique_id, &put_buffer[0]); // Source ID
	// encode_uint32(target_peer, &put_buffer[4]); // Dest ID
	// encode_uint32(packet_flags, &put_buffer[8]); // Dest ID - @TODO Check if this is even necessary for WebRTC (b/c not Enet)
  // copymem(&put_buffer[12], p_buffer, p_buffer_size);
	// copymem(&put_buffer, p_buffer, p_buffer_size);

	if (_is_server) {

    if (target_peer > 0) {
      E->value()->put_packet(p_buffer, p_buffer_size); // Send to server for broadcast
    }
    else {
			int exclude = -target_peer;

			for (Map<int, WebRTCPeer *>::Element *F = peer_map.front(); F; F = F->next()) {

				if (target_peer != 0 && F->key() == exclude) // Exclude packet.  If target_peer == 0 then don't exclude any packets
					continue;

				F->value()->put_packet(p_buffer, p_buffer_size);
			}

			// delete put_buffer;
		}
	}
  else {

		ERR_FAIL_COND_V(!peer_map.has(1), ERR_BUG);
    peer_map[1]->put_packet(p_buffer, p_buffer_size); // Send to server for broadcast
	}

	return OK;
}

int NetworkedMultiplayerWebRTC::get_available_packet_count() const
{
  return incoming_packets.size();
}

int NetworkedMultiplayerWebRTC::get_max_packet_size() const
{
  return 1;
}

void NetworkedMultiplayerWebRTC::_pop_current_packet() {
  // Should I have this?  @TODO Check if I actually need this
	if (current_packet.buffer) {
		delete current_packet.buffer;
		current_packet.buffer = NULL;
    current_packet.buffer_size = 0;
		current_packet.from = 0;
	}
}

void NetworkedMultiplayerWebRTC::create_offer()
{
  ERR_FAIL_COND(_is_server);
  peer_map[1]->create_offer();
}

void NetworkedMultiplayerWebRTC::connect_signals(WebRTCPeer* peer, int id)
{
  peer->connect("offer_created", this, "_offer_created", varray(id));
  peer->connect("new_ice_candidate", this, "_new_ice_candidate", varray(id));
  peer->connect("notify", this, "_notify", varray(id));
}

NetworkedMultiplayerWebRTC::~NetworkedMultiplayerWebRTC()
{
}
