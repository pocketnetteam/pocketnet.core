// Copyright (c) 2015-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef POCKETCOIN_HTTPSERVER_H
#define POCKETCOIN_HTTPSERVER_H

#include <string>
#include <cstdint>
#include <functional>
#include <future>
#include <rpc/protocol.h> // For HTTP status codes
#include <event2/thread.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>
#include <support/events.h>
#include "crypto/x509.h"
#include "rpc/server.h"
#include "init.h"
#include "pocketdb/SQLiteConnection.h"
#include <eventloop.h>
#include "rpcapi/rpcapi.h"

static const int DEFAULT_HTTP_SERVER_TIMEOUT = 30;

static const bool DEFAULT_API_ENABLE = true;
static const bool DEFAULT_REST_ENABLE = false;
static const bool DEFAULT_STATIC_ENABLE = false;

struct evhttp_request;

class CService;

class HTTPRequest;

template<typename WorkItem>
class WorkQueue;

namespace util {
class Ref;
}

/** Initialize HTTP server.
 * Call this before RegisterHTTPHandler or EventBase().
 */
bool InitHTTPServer(const util::Ref& context, const std::shared_ptr<IRequestProcessor>& privateHandler, const std::shared_ptr<IRequestProcessor>& webHandler, const std::shared_ptr<IRequestProcessor>& restHandler, const std::shared_ptr<IRequestProcessor>& staticHandler);
/** Start HTTP server.
 * This is separate from InitHTTPServer to give users race-condition-free time
 * to register their handlers between InitHTTPServer and StartHTTPServer.
 */
void StartHTTPServer();
/** Interrupt HTTP server threads */
void InterruptHTTPServer();
/** Stop HTTP server */
void StopHTTPServer();

/** Change logging level for libevent. Removes BCLog::LIBEVENT from log categories if
 * libevent doesn't support debug logging.*/
bool UpdateHTTPServerLogging(bool enable);

/** Handler for requests to a certain HTTP path */
typedef std::function<bool(HTTPRequest* req, const std::string&)> HTTPRequestHandler;

/** Return evhttp event base. This can be used by submodules to
 * queue timers or custom events.
 */
struct event_base* EventBase();

/** In-flight HTTP request.
 * Thin C++ wrapper around evhttp_request.
 */
class HTTPRequest : public IReplier
{
private:
    struct evhttp_request* req;
    bool replySent;
    std::string m_peerStr;

    DbConnectionRef dbConnection;

public:
    explicit HTTPRequest(struct evhttp_request* req, bool _replySent = false);
    ~HTTPRequest();

    Statistic::RequestTime Created;

    enum RequestMethod
    {
        UNKNOWN,
        GET,
        POST,
        HEAD,
        PUT,
        OPTIONS
    };

    Statistic::RequestTime GetCreated() const override;

    /** Get requested URI.
     */
    std::string GetURI() const;

    const std::string& GetPeerStr() const override;

    /** Get CService (address:ip) for the origin of the http request.
     */
    CService GetPeer() const;

    /** Get request method.
     */
    RequestMethod GetRequestMethod() const;

    /**
     * Get the request header specified by hdr, or an empty string.
     * Return a pair (isPresent,string).
     */
    std::pair<bool, std::string> GetHeader(const std::string& hdr) const;

    /**
     * Read request body.
     *
     * @note As this consumes the underlying buffer, call this only once.
     * Repeated calls will return an empty string.
     */
    std::string ReadBody();

    /**
     * Write output header.
     *
     * @note call this before calling WriteErrorReply or Reply.
     */
    void WriteHeader(const std::string& hdr, const std::string& value) override;

    /**
     * Write HTTP reply.
     * nStatus is the HTTP status code to send.
     * strReply is the body of the reply. Keep it empty to send a standard message.
     *
     * @note Can be called only once. As this will give the request back to the
     * main thread, do not call any other HTTPRequest methods after calling this.
     */
    void WriteReply(int nStatus, const std::string& strReply = "") override;

    bool GetAuthCredentials(std::string& out) override;
};

/** Event handler closure.
 */
class HTTPClosure
{
public:
    virtual void operator()(DbConnectionRef& sqliteConnection) = 0;
    virtual ~HTTPClosure() {}
};

/** Event class. This can be used either as a cross-thread trigger or as a timer.
 */
class HTTPEvent
{
public:
    /** Create a new event.
     * deleteWhenTriggered deletes this event object after the event is triggered (and the handler called)
     * handler is the handler to call when the event is triggered.
     */
    HTTPEvent(struct event_base* base, bool deleteWhenTriggered, std::function<void()> handler);
    ~HTTPEvent();

    /** Trigger the event. If tv is 0, trigger it immediately. Otherwise trigger it after
     * the given time has elapsed.
     */
    void trigger(struct timeval* tv);

    bool deleteWhenTriggered;
    std::function<void()> handler;
private:
    struct event* ev;
};

/** Simple one-shot callback timer to be used by the RPC mechanism to e.g.
 * re-lock the wallet.
 */
class HTTPRPCTimer : public RPCTimerBase
{
public:
    HTTPRPCTimer(struct event_base* eventBase, std::function<void()>& func, int64_t millis) : ev(eventBase, false, func)
    {
        struct timeval tv;
        tv.tv_sec = millis / 1000;
        tv.tv_usec = (millis % 1000) * 1000;
        ev.trigger(&tv);
    }

private:
    HTTPEvent ev;
};

class HTTPRPCTimerInterface : public RPCTimerInterface
{
public:
    explicit HTTPRPCTimerInterface(struct event_base* _base) : base(_base)
    {
    }
    const char* Name() override
    {
        return "HTTP";
    }
    RPCTimerBase* NewTimer(std::function<void()>& func, int64_t millis) override
    {
        return new HTTPRPCTimer(base, func, millis);
    }

private:
    struct event_base* base;
};


class HTTPSocket
{
private:
    struct evhttp* m_http;
    struct evhttp* m_eventHTTP;
    std::vector<evhttp_bound_socket*> m_boundSockets;
    std::optional<SSLContext> m_sslCtx;

public:
    HTTPSocket(struct event_base* base, int timeout, bool publicAccess, bool fUseTls = false);
    ~HTTPSocket();

    /** Sets the need to check the request source. For public APIs,
      * we do not need to restrict the source of the request. */
    bool m_publicAccess;
    
    /** Work queue for handling longer requests off the event loop thread */
    std::shared_ptr<IRequestProcessor> m_requestProcessor;

    /** Start worker threads to listen on bound http sockets */
    void StartHTTPSocket();
    /** Stop worker threads on all bound http sockets */
    void StopHTTPSocket();

    /** Acquire a http socket handle for a provided IP address and port number */
    void BindAddress(std::string ipAddr, int port);
    /** Get number of bound IP sockets */
    int GetAddressCount();

    void InterruptHTTPSocket();
    /** Register handler for prefix.
     * If multiple handlers match a prefix, the first-registered one will
     * be invoked.
     */
    void RegisterRequestProcessor(std::shared_ptr<IRequestProcessor> reqProcessor)
    {
        m_requestProcessor = std::move(reqProcessor);
    }
};

class HTTPWebSocket: public HTTPSocket
{
public:
    CRPCTable m_table_post_rpc;

    HTTPWebSocket(struct event_base* base, int timeout, bool publicAccess, bool fUseTls = false);
    ~HTTPWebSocket();

    void StartHTTPSocket();
    void StopHTTPSocket();
    void InterruptHTTPSocket();
};

extern HTTPSocket* g_socket;
extern HTTPSocket* g_staticSocket;
extern HTTPSocket* g_restSocket;
extern HTTPWebSocket* g_webSocket;
extern HTTPWebSocket* g_webSocketHttps;

#endif // POCKETCOIN_HTTPSERVER_H
