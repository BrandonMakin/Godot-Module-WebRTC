#ifndef WEBRTC_PEER_H
#define WEBRTC_PEER_H

// #include "queued_signal.h"
#include <functional>         // std::function
#include <iostream>           // remove eventually
#include "os/mutex.h"         // mutex // necessary to include this? it seemed to work find without including
#include "ustring.h"          // String
#include "reference.h"
#include "io/packet_peer.h"
#include "ring_buffer.h"      // RingBuffer
#include "api/peerconnectioninterface.h"
#include "media/base/mediaengine.h"       // needed for CreateModularPeerConnectionFactory

// class WebRTCPeer : public Reference {
//   GDCLASS(WebRTCPeer, Reference);

class WebRTCPeer : public PacketPeer {
  GDCLASS(WebRTCPeer, PacketPeer);

protected:
    static void _bind_methods();

public:

  Mutex *mutex_signal_queue;
  Mutex *mutex_packet_queue;

  std::string name = "receiver";
  std::queue< std::function<void()> > signal_queue;
  std::queue< uint8_t* > packet_queue;
  std::queue< int > packet_sizes_queue;
  // RingBuffer< rtc::CopyOnWriteBuffer > packet_rbuffer;
  // RingBuffer< uint8_t* > packet_rbuffer;
  int packet_queue_size;

  int create_offer();
  void set_remote_description(String sdp, bool isOffer);
  void set_local_description(String sdp, bool isOffer);
  void set_description(String sdp, bool isOffer, bool isLocal);
  void add_ice_candidate(String sdpMidName, int sdpMlineIndexName, String sdpName);
  void poll();
  void queue_signal(StringName p_name, VARIANT_ARG_LIST);
  // void queue_packet(const webrtc::DataBuffer&);
  // void queue_packet(const rtc::CopyOnWriteBuffer*);
  void queue_packet(uint8_t*, int);
  // webrtc::PeerConnectionInterface::SignalingState _get_state_peer_connection();
  bool _is_active();

  WebRTCPeer();
  ~WebRTCPeer();


  /** Inherited from PacketPeer: **/
  // @TODO add "override" keyword to all of these inherited methods.
  int get_available_packet_count() const;
  Error get_packet(const uint8_t **r_buffer, int &r_buffer_size); ///< buffer is GONE after next get_packet
  Error put_packet(const uint8_t *p_buffer, int p_buffer_size);

  int get_max_packet_size() const;

  /** PeerConnectionObserver callback functions **/
  class GD_PCO : public webrtc::PeerConnectionObserver {
  public:
    WebRTCPeer* parent;

    GD_PCO(WebRTCPeer* parent);

    // Triggered when the SignalingState changes.
    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override;

    // Triggered when media is received on a new stream from remote peer.
    void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;

    // Triggered when a remote peer closes a stream.
    void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;

    // Triggered when a remote peer opens a data channel.
    void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override;

    // Triggered when renegotiation is needed. For example, an ICE restart
    // has begun.
    void OnRenegotiationNeeded() override;

    // Called any time the IceConnectionState changes.
    void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override;

    // Called any time the IceGatheringState changes.
    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override;

    // A new ICE candidate has been gathered.
    void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
  };

  /** CreateSessionDescriptionObserver callback functions **/
  class GD_CSDO : public webrtc::CreateSessionDescriptionObserver {
  public:
    WebRTCPeer* parent;

    GD_CSDO(WebRTCPeer* parent);
    void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;
    void OnFailure(const std::string& error) override;
  };

  /** DataChannelObserver callback functions **/
  class GD_DCO : public webrtc::DataChannelObserver {
  public:
    WebRTCPeer* parent;

    GD_DCO(WebRTCPeer* parent);
    void OnStateChange() override;
    void OnMessage(const webrtc::DataBuffer& buffer) override;
    void OnBufferedAmountChange(uint64_t previous_amount) override;
  };

  /** SetSessionDescriptionObserver callback functions **/
  class GD_SSDO : public webrtc::SetSessionDescriptionObserver {
  public:
    WebRTCPeer* parent;
    GD_SSDO(WebRTCPeer* parent);
    void OnSuccess() override;
    void OnFailure(const std::string& error) override;
  };

  rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection;
  rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel;

  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> pc_factory;

  // std::unique_ptr<rtc::Thread> signaling_thread;
  rtc::Thread* signaling_thread;


  GD_PCO pco;
  GD_DCO dco;
  rtc::scoped_refptr<GD_SSDO> ptr_ssdo;
  rtc::scoped_refptr<GD_CSDO> ptr_csdo;


};
#endif //WEBRTC_PEER_CONNECTION_CREATOR
