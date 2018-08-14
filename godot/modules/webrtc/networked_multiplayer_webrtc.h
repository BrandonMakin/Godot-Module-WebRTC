#ifndef NETWORKED_MULTIPLAYER_WEBRTC_H
#define NETWORKED_MULTIPLAYER_WEBRTC_H

#include "io/networked_multiplayer_peer.h"
#include "webrtc_peer.h"

class NetworkedMultiplayerWebRTC : public NetworkedMultiplayerPeer {

	GDCLASS(NetworkedMultiplayerWebRTC, NetworkedMultiplayerPeer);

protected:
  static void _bind_methods();

private:
  // bool active;
	bool _is_server = false;
	uint32_t unique_id;
	int target_peer;
  int max_clients;
  int client_count = 0;
	TransferMode transfer_mode;

  std::queue< uint8_t* > packet_queue;
  std::queue< int > packet_sizes_queue;

  struct Packet {
    const uint8_t* buffer;
    int buffer_size;
    int from;
  };

  List<Packet> incoming_packets;

  Packet current_packet;

  bool refuse_connections = false;
  ConnectionStatus connection_status;

	// int transfer_channel;
	// int channel_count;
	// bool always_ordered;
  //
	// ENetEvent event;
	// ENetPeer *peer;
	// ENetHost *host;

	Map<int, WebRTCPeer *> peer_map;

public:
  ~NetworkedMultiplayerWebRTC();

  void _notify(String message, int p_id);
  void _offer_created(String sdp, bool isOffer, int p_id);
  void _new_ice_candidate(String candidateSdpMidName, int candidateSdpMlineIndexName, String candidateSdpName, int p_id);

  // NetworkedMultiplayerWebRTC();
  Error create_server( int max_clients=32 );
  void create_client();
  int accept_client();
  void set_unique_id(int id);
  void connect_signals(WebRTCPeer* peer, int id);

  Error set_remote_description(String sdp, bool isOffer, int peer_id);
  Error set_local_description(String sdp, bool isOffer, int peer_id);
  Error add_ice_candidate(String sdpMidName, int sdpMlineIndexName, String sdpName, int peer_id);

  uint32_t _gen_unique_id() const;
  Packet webrtc_packet_create(const uint8_t* buffer, int buffer_size, int from);

  void _pop_current_packet();

  // WebRTC peer stuff:
  void create_offer();


  // Overridden from PacketPeer:
  Error get_packet(const uint8_t **r_buffer, int &r_buffer_size); ///< buffer is GONE after next get_packet
  Error put_packet(const uint8_t *p_buffer, int p_buffer_size);
  int get_available_packet_count() const;
  int get_max_packet_size() const;



  // Overridden from NetworkedMultiplayerPeer:

  void set_transfer_mode(TransferMode p_mode);
  TransferMode get_transfer_mode() const;
  void set_target_peer(int p_peer_id);

  int get_packet_peer() const;

  bool is_server() const;

  void poll();

  int get_unique_id() const;

  void set_refuse_new_connections(bool p_enable);
  bool is_refusing_new_connections() const;

  ConnectionStatus get_connection_status() const;


};

#endif
