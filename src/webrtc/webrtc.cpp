
#include "webrtc/webrtc.h"
#include "eventloop.h"
#include "protectedmap.h"


class PCClearer : public IQueueProcessor<std::shared_ptr<webrtc::WebRTCConnection>>
{
public:
    void Process(std::shared_ptr<webrtc::WebRTCConnection> pc) override
    {
        // Do nothing. This is just needed to guarantee that memory will be freed in different thread
    }
};


webrtc::WebRTC::WebRTC(std::shared_ptr<IRequestProcessor> requestProcessor, std::shared_ptr<notifications::INotificationProtocol> notificationProtocol, int port)
    : m_port(port),
      m_queue(std::make_shared<Queue<std::shared_ptr<WebRTCConnection>>>())
{
    m_protocol = std::make_shared<WebRTCProtocol>(requestProcessor, notificationProtocol, m_queue),
    m_eventLoop = std::make_shared<QueueEventLoopThread<std::shared_ptr<WebRTCConnection>>>(m_queue, std::make_shared<PCClearer>());
    m_wsConnections = std::make_shared<ProtectedMap<std::string, std::shared_ptr<webrtc::WsConnectionHandler>>>();
}


void webrtc::WebRTC::InitiateNewSignalingConnection(const std::string& ip)
{
    if (!m_fRunning) {
        // TODO (losty-rtc): error
        return;
    }
    if (m_wsConnections->has(ip)) {
        // TODO (losty-rtc): error
        return;
    }
    auto ws = std::make_shared<rtc::WebSocket>();
    auto wsHandler = std::make_shared<webrtc::WsConnectionHandler>(ip, ws, m_wsConnections);
    // TODO (losty-rtc): better memory handling in callbacks
    ws->onOpen([ws = std::weak_ptr(ws)]() {
        UniValue registermeMsg(UniValue::VOBJ);
        registermeMsg.pushKV("type", "registerasnode");
        if (auto lock = ws.lock()) {
            lock->send(registermeMsg.write());
        }
    });
    ws->onMessage([ws = std::weak_ptr(ws), protocol = m_protocol](rtc::message_variant data) {
        if (!std::holds_alternative<std::string>(data))
            return; // TODO (losty-rtc): error
        UniValue message(UniValue::VOBJ);
        message.read(std::get<std::string>(data));
        if (!message.exists("type")) {
            // TODO (losty-rtc): error
            return;
        }
        auto type = message["type"].get_str();
        if (type != "protocol") {
            // TODO (losty-rtc): error
            return;
        }

        if (!message.exists("id")) {
            // TODO (losty-rtc): error
            return;
        }
        auto id = message["id"].get_str();
        if (!message.exists("message")) {
            // TODO (losty-rtc): error
            return;
        }

        if (auto lock = ws.lock()) {
            protocol->Process(message["message"], id, lock);
        }
    });
    ws->onClosed([wsHandler = std::weak_ptr(wsHandler)]() {
        if (auto lock = wsHandler.lock()) {
            lock->onClosed();
        }
    });

    std::string url = "ws://" + ip + ":" + std::to_string(m_port) + "/signaling";
    ws->open(url);

    m_wsConnections->insert(ip, std::move(wsHandler));
}

void webrtc::WebRTC::DropConnection(const std::string &id)
{
    m_wsConnections->erase(id);
}

void webrtc::WebRTC::Start()
{
    m_fRunning = true;
    m_eventLoop->Start();
}

void webrtc::WebRTC::Stop()
{
    m_fRunning = false;
    m_eventLoop->Stop();
    m_wsConnections->clear();
    m_protocol->StopAll();
    // TODO (losty-rtc): queue is not cleared
}