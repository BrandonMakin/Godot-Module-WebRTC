#include "webrtc_peer.h"

WebRTCPeer::GD_SSDO::GD_SSDO(WebRTCPeer* parent)
{
  this->parent = parent;
}

void WebRTCPeer::GD_SSDO::OnSuccess() {
  std::string message = "SetSessionDescriptionObserver::OnSuccess - doing nothing";
  parent->queue_signal("notify", message.c_str());
  // std::cout << message << std::endl;
  std::cout << parent->name << " state: " << parent->peer_connection->signaling_state() << std::endl;
};

void WebRTCPeer::GD_SSDO::OnFailure(const std::string& error) {
  std::string message = "SetSessionDescriptionObserver::OnFailure: error = ";
  message += error;
  parent->queue_signal("notify", message.c_str());
  // std::cout << message << std::endl;
};
