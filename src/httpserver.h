// Copyright (c) 2015-2018 The Pocketcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef POCKETCOIN_HTTPSERVER_H
#define POCKETCOIN_HTTPSERVER_H

#include <string>
#include <stdint.h>
#include <functional>
#include <future>
#include <rpc/protocol.h> // For HTTP status codes
#include <event2/thread.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>

#include <support/events.h>

static const int DEFAULT_HTTP_THREADS=4;
static const int DEFAULT_HTTP_POST_THREADS=4;
static const int DEFAULT_HTTP_PUBLIC_THREADS=4;
static const int DEFAULT_HTTP_WORKQUEUE=16;
static const int DEFAULT_HTTP_POST_WORKQUEUE=16;
static const int DEFAULT_HTTP_PUBLIC_WORKQUEUE=16;
static const int DEFAULT_HTTP_SERVER_TIMEOUT=30;

struct evhttp_request;
//struct event_base;
class CService;
class HTTPRequest;
template<typename WorkItem> class WorkQueue;

struct HTTPPathHandler;

/** Initialize HTTP server.
 * Call this before RegisterHTTPHandler or EventBase().
 */
bool InitHTTPServer();
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
typedef std::function<bool(HTTPRequest* req, const std::string &)> HTTPRequestHandler;

/** Return evhttp event base. This can be used by submodules to
 * queue timers or custom events.
 */
struct event_base* EventBase();

/** In-flight HTTP request.
 * Thin C++ wrapper around evhttp_request.
 */
class HTTPRequest
{
private:
    struct evhttp_request* req;
    bool replySent;

public:
    explicit HTTPRequest(struct evhttp_request* req);
    ~HTTPRequest();

    enum RequestMethod {
        UNKNOWN,
        GET,
        POST,
        HEAD,
        PUT
    };

    /** Get requested URI.
     */
    std::string GetURI() const;

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
    void WriteHeader(const std::string& hdr, const std::string& value);

    /**
     * Write HTTP reply.
     * nStatus is the HTTP status code to send.
     * strReply is the body of the reply. Keep it empty to send a standard message.
     *
     * @note Can be called only once. As this will give the request back to the
     * main thread, do not call any other HTTPRequest methods after calling this.
     */
    void WriteReply(int nStatus, const std::string& strReply = "");
};

/** Event handler closure.
 */
class HTTPClosure
{
public:
    virtual void operator()() = 0;
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
    HTTPEvent(struct event_base *base, bool deleteWhenTriggered, const std::function<void()>& handler);
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

class HTTPSocket
{
private:
    struct evhttp                      *m_http;
    struct evhttp                      *m_eventHTTP;
    std::vector<evhttp_bound_socket *> m_boundSockets;
    std::vector<std::thread>           m_thread_http_workers; 

public:
    HTTPSocket(struct event_base *base, int timeout, int queueDepth);
    ~HTTPSocket();

    /** Work queue for handling longer requests off the event loop thread */
    WorkQueue<HTTPClosure> *m_workQueue;
    std::vector<HTTPPathHandler> m_pathHandlers;

    /** Start worker threads to listen on bound http sockets */
    void StartHTTPSocket(int threadCount);
    /** Stop worker threads on all bound http sockets */
    void StopHTTPSocket();
    /** Acquire a http socket handle for a provided IP address and port number */
    void BindAddress(std::string ipAddr, int port);
    /** Get number of bound IP sockets */
    int  GetAddressCount();

    void InterruptHTTPSocket();
    /** Register handler for prefix.
     * If multiple handlers match a prefix, the first-registered one will
     * be invoked.
     */
    void RegisterHTTPHandler(const std::string &prefix, bool exactMatch, const HTTPRequestHandler &handler);
    /** Unregister handler for prefix */
    void UnregisterHTTPHandler(const std::string &prefix, bool exactMatch);
};

std::string urlDecode(const std::string &urlEncoded);

extern HTTPSocket *g_socket;
extern HTTPSocket *g_pubSocket;

#endif // POCKETCOIN_HTTPSERVER_H
