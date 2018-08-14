#include "webrtc_peer.h"

// #include "api/audio_codecs/builtin_audio_decoder_factory.h"
// #include "api/audio_codecs/builtin_audio_encoder_factory.h"
// #include "test/mock_audio_decoder_factory.h"
// #include "test/mock_audio_encoder_factory.h"
// #include "api/video_codecs/builtin_video_decoder_factory.h"
// #include "api/video_codecs/builtin_video_encoder_factory.h"


void WebRTCPeer::_bind_methods()
{

  ClassDB::bind_method(D_METHOD("create_offer"), &WebRTCPeer::create_offer);
  ClassDB::bind_method(D_METHOD("set_local_description", "sdp", "isOffer"), &WebRTCPeer::set_local_description);
  ClassDB::bind_method(D_METHOD("set_remote_description", "sdp", "isOffer"), &WebRTCPeer::set_remote_description);
  // ClassDB::bind_method(D_METHOD("send_message", "message"), &WebRTCPeer::send_message);
  // ClassDB::bind_method(D_METHOD("_get_state_peer_connection"), &WebRTCPeer::get_state_peer_connection);
  ClassDB::bind_method(D_METHOD("poll"), &WebRTCPeer::poll);
  ClassDB::bind_method( // @TODO rename arguments: give them shorter names
    D_METHOD( "add_ice_candidate",
              "sdp_mid_name",
              "sdp_mline_index_name",
              "sdp_name"
    ), &WebRTCPeer::add_ice_candidate
  );

  ADD_SIGNAL(MethodInfo("notify", PropertyInfo(Variant::STRING, "message")));
  // ADD_SIGNAL(MethodInfo("new_peer_message", PropertyInfo(Variant::STRING, "message")));
  ADD_SIGNAL(MethodInfo("offer_created",
                        PropertyInfo(Variant::STRING, "sdp"),
                        PropertyInfo(Variant::BOOL, "is_offer")
  ));
  ADD_SIGNAL(MethodInfo("new_ice_candidate",
                        PropertyInfo(Variant::STRING, "sdp_mid_name"),
                        PropertyInfo(Variant::INT, "sdp_mline_index_name"),
                        PropertyInfo(Variant::STRING, "sdp_name")
  ));
}

WebRTCPeer::WebRTCPeer() :  pco(this)
                            , ptr_csdo(new rtc::RefCountedObject<GD_CSDO>(this))
                            , ptr_ssdo(new rtc::RefCountedObject<GD_SSDO>(this))
                            , dco(this)
{
  mutex_signal_queue = Mutex::create(true);
  mutex_packet_queue = Mutex::create(true);

  packet_queue_size = 0;

  // packet_rbuffer.resize(16);

  // 1. Create a PeerConnectionFactoryInterface.
  signaling_thread = new rtc::Thread;
  signaling_thread->Start();
  pc_factory = webrtc::CreateModularPeerConnectionFactory(
    nullptr, // rtc::Thread* network_thread,
    nullptr, // rtc::Thread* worker_thread,
    signaling_thread,
    nullptr, // std::unique_ptr<cricket::MediaEngineInterface> media_engine,
    nullptr, // std::unique_ptr<CallFactoryInterface> call_factory,
    nullptr  // std::unique_ptr<RtcEventLogFactoryInterface> event_log_factory
  );
  if (pc_factory.get() == nullptr)
    queue_signal("notify", "[FAILURE]: peer connection factory");


  else
    queue_signal("notify", "[success]: peer connection factory");

    // return 0xBADFAC; // "bad factory" -> the factory isn't created correctly

  // 2. Create a PeerConnection object. Provide a configuration struct which
  // points to STUN and/or TURN servers used to generate ICE candidates, and
  // provide an object that implements the PeerConnectionObserver interface,
  // which is used to receive callbacks from the PeerConnection.

  webrtc::PeerConnectionInterface::RTCConfiguration configuration; // default configuration

  webrtc::PeerConnectionInterface::IceServer ice_server;

  // configuration.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;  // Temporary @TODO look this line up
  // configuration.enable_dtls_srtp = true;                             // Temporary @TODO look this line up

  ice_server.uri = "stun:stun.l.google.com:19302";
  configuration.servers.push_back(ice_server); // add ice_server to configuration

  peer_connection = pc_factory->CreatePeerConnection(
    configuration, nullptr, nullptr, &pco);

  if (peer_connection.get() == nullptr)
    queue_signal("notify", "[FAILURE]: peer connection");

  else
    queue_signal("notify", "[success]: peer connection");

  // return 0xBADC; // "bad connection" -> the peer connection isn't created correctly

  // 3. Create local MediaStreamTracks using the PeerConnectionFactory and add
  // them to PeerConnection by calling AddTrack (or legacy method, AddStream).
  webrtc::DataChannelInit data_channel_config;
  data_channel_config.negotiated = true; // True if the channel has been externally negotiated
  data_channel_config.id = 0;

  data_channel = peer_connection->CreateDataChannel("channel", &data_channel_config);
  data_channel->RegisterObserver(&dco);
}

int WebRTCPeer::create_offer() {
  name = "caller";
  queue_signal("notify", "WebRTCPeer::create_offer");


  // 4. Create an offer, call SetLocalDescription with it, serialize it, and send
  // it to the remote peer

  peer_connection->CreateOffer(
    ptr_csdo, // CreateSessionDescriptionObserver* observer,
    nullptr // webrtc::PeerConnectionInterface::RTCOfferAnswerOptions() // const MediaConstraintsInterface* constraints
  );
  return 0;
}

void WebRTCPeer::set_local_description(String sdp, bool isOffer)
{
  set_description(sdp, isOffer, true); // isLocal == true
}

void WebRTCPeer::set_remote_description(String sdp, bool isOffer)
{
  set_description(sdp, isOffer, false); //false meaning !isLocal because it is remote

  if (isOffer)
  {
      // 4. Generate an answer to the remote offer by calling CreateAnswer and send it
      // back to the remote peer.
      /*FOR THE PEER RECEIVING THE CALL*/
      peer_connection->CreateAnswer(ptr_csdo, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
  }
}

void WebRTCPeer::set_description(String sdp, bool isOffer, bool isLocal)
{
  std::cout << name << " state: " << peer_connection->signaling_state() << std::endl;
  // std::string message = "state: " + (peer_connection->signaling_state());
  // queue_signal("notify", message.c_str());


  std::string string_sdp = sdp.utf8().get_data();
  webrtc::SdpType type = (isOffer) ? webrtc::SdpType::kOffer : webrtc::SdpType::kAnswer;
  // FOR THE PEER MAKING THE CALL
  // SetRemoteDescription with the remote ANSWER.

  // FOR THE PEER RECEIVING THE CALL
  // SetRemoteDescription with the remote OFFER.

  std::unique_ptr<webrtc::SessionDescriptionInterface> desc =
    webrtc::CreateSessionDescription(type, string_sdp);

  if (isLocal)
  {
    peer_connection->SetLocalDescription(
      ptr_ssdo, // if an ssdo isn't needed, you can use DummySetSessionDescriptionObserver::Create()
      desc.release()
    );
  } else
  {
    peer_connection->SetRemoteDescription(
      ptr_ssdo, // if an ssdo isn't needed, you can use DummySetSessionDescriptionObserver::Create()
      desc.release()
    );
  }
  ////////////////////////////////////////////////////////////////////

  std::string message = "WebRTCPeer::SetRemoteDescription - setting description to ";
  message += (isOffer) ? "Offer" : "Answer";
  queue_signal("notify", message.c_str());

}

void WebRTCPeer::add_ice_candidate(String sdpMidName, int sdpMlineIndexName, String sdpName)
{
  // 7. Once a remote candidate is received from the remote peer, provide it to
  // the PeerConnection by calling AddIceCandidate.

  queue_signal("notify", "WebRTCPeer::add_ice_candidate - adding candidate");


  webrtc::SdpParseError *error = nullptr;
  webrtc::IceCandidateInterface *candidate = webrtc::CreateIceCandidate(
    sdpMidName.utf8().get_data(),
    sdpMlineIndexName,
    sdpName.utf8().get_data(),
    error
  );
  // @TODO do something if there's an error (if error, or if !candidate)
  if (error || !candidate)
    std::cout << "ERROR with creating ICE candidate (" << error << ")\n";

  if (!peer_connection->AddIceCandidate(candidate))
    queue_signal("notify", "error with adding ICE candidate");

  // @TODO do something if there's an error adding the candidate [if (!peer_connection->AddIceCandidate(candidate))]
}

// void WebRTCPeer::send_message(String msg)
// {
//   std::string string_msg = msg.utf8().get_data();
//   put_packet((uint8_t *)(string_msg.c_str()), string_msg.size());
//   // webrtc::DataBuffer buffer(rtc::CopyOnWriteBuffer(string_msg.c_str(), string_msg.size()), true);
//   // std::cout << "Send(" << data_channel->state() << ")" << std::endl;
//   // data_channel->Send(buffer);
// }

int WebRTCPeer::get_available_packet_count() const
{
  return packet_queue_size;
}

Error WebRTCPeer::get_packet(const uint8_t **r_buffer, int &r_buffer_size)
{
  /*
  Hold a current_packet buffer (a reference to the last popped packet)
  and set *r_buffer = current_packet (try to get it to uint8_t without copying)
  (may need to copy because WebRTC deletes the reference)
  and r_size to the size of current_packet
  ---
  on every get_packet call:
  pop front from queue into current_buffer,
  set r_buffer to buffer start (*r_buffer = current_packet)
  set r_size accordingly. You have a PacketPeer :)
  */

  if (packet_queue_size == 0)
    return ERR_UNAVAILABLE;
    mutex_packet_queue->lock();
    uint8_t* current_packet = packet_queue.front();
    *r_buffer = current_packet;
    r_buffer_size = packet_sizes_queue.front();

    packet_queue.pop();
    packet_sizes_queue.pop();
    mutex_packet_queue->unlock();

  // rtc::CopyOnWriteBuffer current_packet; // packet_queue.front();
  // uint8_t* current_packet;
  // packet_rbuffer.read(&current_packet, 1);
  //
  // *r_buffer = current_packet.data<uint8_t>();  // .data returns the Buffer's data as a uint8_t*
  // *r_buffer = current_packet;
  // r_buffer_size = current_packet.size();

  --packet_queue_size;
  return OK;
}


Error WebRTCPeer::put_packet(const uint8_t *p_buffer, int p_buffer_size)
{
  webrtc::DataBuffer webrtc_buffer(rtc::CopyOnWriteBuffer(p_buffer, p_buffer_size), true);
  data_channel->Send(webrtc_buffer);

  return OK; // @TODO properly return any Error we may get.
}

int WebRTCPeer::get_max_packet_size() const
{
  return 1;
}

void WebRTCPeer::poll()
{
  std::function<void()> signal;
  while (!signal_queue.empty())
  {
    mutex_signal_queue->lock();
    signal = signal_queue.front();
    signal_queue.pop();
    mutex_signal_queue->unlock();

    signal();

  }
}

void WebRTCPeer::queue_signal(StringName p_name, VARIANT_ARG_DECLARE)
{
  mutex_signal_queue->lock();
  signal_queue.push(
    [this, p_name, VARIANT_ARG_PASS]{
      emit_signal(p_name, VARIANT_ARG_PASS);
    }
  );
  mutex_signal_queue->unlock();
}

void WebRTCPeer::queue_packet(uint8_t* buffer, int buffer_size)
{
  mutex_packet_queue->lock();
  packet_queue.push(buffer);
  packet_sizes_queue.push(buffer_size);
  // packet_rbuffer.write(buffer, 1);
  ++packet_queue_size;
  mutex_packet_queue->unlock();
}


// webrtc::PeerConnectionInterface::SignalingState WebRTCPeer::_get_state_peer_connection()
// {
//   // queue_signal("notify", "state: " + peer_connection->signaling_state());
//   // std::cout << name << "- peer connection state: " << peer_connection->signaling_state() << std::endl;
//   return peer_connection->signaling_state();
//
//   // String statename = ":(";
//   // switch (state)
//   // {
//   //   case webrtc::PeerConnectionInterface::SignalingState.kStable:
//   //     statename = "kStable";
//   //   case kHaveLocalOffer:
//   //     statename = "kHaveLocalOffer";
//   //   case kHaveLocalPrAnswer:
//   //     statename = "kHaveLocalPrAnswer";
//   //   case kHaveRemoteOffer:
//   //     statename = "kHaveRemoteOffer";
//   //   case kHaveRemotePrAnswer:
//   //     statename = "kHaveRemotePrAnswer";
//   //   case kClosed:
//   //     statename = "kClosed";
//   // }
//   // return "" + state;
// }

bool WebRTCPeer::_is_active()
{
  return data_channel->state() == webrtc::DataChannelInterface::DataState::kOpen;
}

WebRTCPeer::~WebRTCPeer()
{
  // delete peerConnectionFactory;
  memdelete(mutex_signal_queue);
  mutex_signal_queue = NULL;

  memdelete(mutex_packet_queue);
  mutex_packet_queue = NULL;
}
