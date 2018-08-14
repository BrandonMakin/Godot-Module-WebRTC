#include "webrtc_peer.h"

WebRTCPeer::GD_PCO::GD_PCO(WebRTCPeer* parent)
{
  this->parent = parent;
}

void WebRTCPeer::GD_PCO::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state)
{
  parent->queue_signal("notify", "PeerConnectionObserver::OnSignalingChange - doing nothing");
  // std::cout << "OnSignalingChange" << std::endl;
}

void WebRTCPeer::GD_PCO::OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream)
{
  parent->queue_signal("notify", "PeerConnectionObserver::OnAddStream");
  // std::cout << "OnAddStream " << std::endl;
}

void WebRTCPeer::GD_PCO::OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream)
{
  parent->queue_signal("notify", "PeerConnectionObserver::OnRemoveStream");
  // std::cout << "PeerConnectionObserver::OnRemoveStream " << std::endl;
}

void WebRTCPeer::GD_PCO::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel)
{
  parent->queue_signal("notify", "PeerConnectionObserver::OnDataChannel");
  // std::cout << "PeerConnectionObserver::OnDataChannel " << std::endl;
}

void WebRTCPeer::GD_PCO::OnRenegotiationNeeded()
{
  parent->queue_signal("notify", "PeerConnectionObserver::OnRenegotiationNeeded");
  // std::cout << "PeerConnectionObserver::OnRenegotiationNeeded" << std::endl;

  // parent->peer_connection->CreateOffer(
  //   parent->ptr_csdo, // CreateSessionDescriptionObserver* observer,
  //   webrtc::PeerConnectionInterface::RTCOfferAnswerOptions() // const MediaConstraintsInterface* constraints
  // );
  // std::cout << "Trying to create offer from GD_PCO\n";

}

void WebRTCPeer::GD_PCO::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state)
{
  parent->queue_signal("notify", "PeerConnectionObserver::OnIceConnectionChange");
  // std::cout << "OnIceConnectionChange " << std::endl;
}

void WebRTCPeer::GD_PCO::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state)
{
  parent->queue_signal("notify", "PeerConnectionObserver::OnIceGatheringChange");
  // std::cout << "OnIceGatheringChange " << std::endl;
}

void WebRTCPeer::GD_PCO::OnIceCandidate(const webrtc::IceCandidateInterface* candidate)
{
  // std::cout << "OnIceCandidate " << std::endl;

  // 5. Once an ICE candidate has been gathered, the PeerConnection will call the
  // observer function OnIceCandidate. The candidates must also be serialized and
  // sent to the remote peer. That peer will add the new ice candidate by calling
  // PeerConnection->AddIceCandidate(candidate);


  // TODO Put the following three strings in a Godot dictionary
  // and emit it as a signal with that dictionary as the argument
  Dictionary candidateSDP;

  String candidateSdpMidName = candidate->sdp_mid().c_str();
  int candidateSdpMlineIndexName = candidate->sdp_mline_index();
  std::string sdp;
  candidate->ToString(&sdp);
  String candidateSdpName = sdp.c_str();

  // message += "SDP MidName = " + candidateSdpMidName;
  // message += ", SDP MlineIndexName = " + candidateSdpMlineIndexName;
  // message += ", SDP name = " + candidateSdpName;
  parent->queue_signal("new_ice_candidate",
                      candidateSdpMidName,
                      candidateSdpMlineIndexName,
                      candidateSdpName
  );

  ////////////////////////////////////////////////////////////////////////////
  // RTC_LOG(INFO) << __FUNCTION__ << " " << candidate->sdp_mline_index();
  // // For loopback test. To save some connecting delay.
  // if (loopback_) {
  //   if (!peer_connection->AddIceCandidate(candidate)) {
  //     RTC_LOG(WARNING) << "Failed to apply the received candidate";
  //   }
  //   return;
  // }
  // Json::StyledWriter writer;
  // Json::Value jmessage;
  // jmessage[kCandidateSdpMidName] = candidate->sdp_mid();
  // jmessage[kCandidateSdpMlineIndexName] = candidate->sdp_mline_index();
  // std::string sdp;
  // if (!candidate->ToString(&sdp)) {
  //   RTC_LOG(LS_ERROR) << "Failed to serialize candidate";
  //   return;
  // }
  // jmessage[kCandidateSdpName] = sdp;
  //----------------------------------------------------------------------
  // SendMessage(writer.write(jmessage));

}
