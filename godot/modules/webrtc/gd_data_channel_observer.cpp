#include "webrtc_peer.h"


WebRTCPeer::GD_DCO::GD_DCO(WebRTCPeer* parent)
{
  this->parent = parent;
}

void WebRTCPeer::GD_DCO::OnStateChange() {
  parent->queue_signal("notify", "DataChannelObserver::OnStateChange");
  //@TODO notify what the new DataChannel state is.
};

void WebRTCPeer::GD_DCO::OnMessage(const webrtc::DataBuffer& buffer) {
  // String message = std::string(buffer.data.data<char>(), buffer.data.size()).c_str();
  // parent->queue_signal("notify", message);
  // parent->queue_packet(&buffer.data);

  const uint8_t* data = buffer.data.data<uint8_t>();

  uint8_t* memory_controlled_buffer = new uint8_t[buffer.data.size()];
  std::copy(data, data + buffer.data.size(), memory_controlled_buffer);

  parent->queue_packet(memory_controlled_buffer, buffer.data.size());
};

void WebRTCPeer::GD_DCO::OnBufferedAmountChange(uint64_t previous_amount) {
  std::string message = "DataChannelObserver::OnBufferedAmountChange - ";
  // @TODO find a way to send the uint64_t - previous_amount - in the message

  // std::cout << "DataChannelObserver::OnBufferedAmountChange(" << previous_amount << ")" << std::endl;
};
