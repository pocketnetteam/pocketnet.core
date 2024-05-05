// Copyright (c) 2015-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "logging.h"
#include "rpc/blockchain.h"
#include <httpserver.h>
#include <interfaces/chain.h>

#include <event2/bufferevent.h>
#include <event2/bufferevent_ssl.h>

#include <chainparamsbase.h>
#include <openssl/ssl.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <util/translation.h>
#include <netbase.h>
#include <sync.h>
#include <node/ui_interface.h>
#include <memory>
#include <cstdlib>
#include <deque>
#include <future>
#include <rpc/register.h>
#include <walletinitinterface.h>
#include "eventloop.h"

#include "rpcapi/rpcapi.h"

#ifdef EVENT__HAVE_NETINET_IN_H
#include <netinet/in.h>
#ifdef _XOPEN_SOURCE_EXTENDED
#include <arpa/inet.h>
#endif
#endif

#if ENABLE_ZMQ
#include <zmq/zmqrpc.h>
#endif

/** Maximum size of http request (request line + headers) */
static const size_t MAX_HEADERS_SIZE = 8192;

/* Stored RPC timer interface (for unregistration) */
static std::unique_ptr<HTTPRPCTimerInterface> httpRPCTimerInterface;

/** HTTP module state */

//! libevent event loop
static struct event_base *eventBase = nullptr;
//! HTTP server
static struct evhttp *eventHTTP = nullptr;
//! List of subnets to allow RPC connections from
static std::vector<CSubNet> rpc_allow_subnets;

//! HTTP socket objects to handle requests on different routes
HTTPSocket *g_socket;
HTTPWebSocket *g_webSocket;
HTTPWebSocket *g_webSocketHttps;
HTTPSocket *g_staticSocket;
HTTPSocket *g_restSocket;

static std::thread g_thread_http;

/** Check if a network address is allowed to access the HTTP server */
static bool ClientAllowed(const CNetAddr &netaddr)
{
    if (!netaddr.IsValid())
        return false;
    for (const CSubNet &subnet : rpc_allow_subnets)
        if (subnet.Match(netaddr))
            return true;
    return false;
}

/** Initialize ACL list for HTTP server */
static bool InitHTTPAllowList()
{
    rpc_allow_subnets.clear();
    CNetAddr localv4;
    CNetAddr localv6;
    LookupHost("127.0.0.1", localv4, false);
    LookupHost("::1", localv6, false);
    rpc_allow_subnets.push_back(CSubNet(localv4, 8));      // always allow IPv4 local subnet
    rpc_allow_subnets.push_back(CSubNet(localv6));         // always allow IPv6 localhost
    for (const std::string &strAllow : gArgs.GetArgs("-rpcallowip"))
    {
        CSubNet subnet;
        LookupSubNet(strAllow, subnet);
        if (!subnet.IsValid())
        {
            uiInterface.ThreadSafeMessageBox(
                strprintf(
                    Untranslated("Invalid -rpcallowip subnet specification: %s. Valid are a single IP (e.g. 1.2.3.4), a network/netmask (e.g. 1.2.3.4/255.255.255.0) or a network/CIDR (e.g. 1.2.3.4/24)."),
                    strAllow),
                "", CClientUIInterface::MSG_ERROR);
            return false;
        }
        rpc_allow_subnets.push_back(subnet);
    }
    std::string strAllowed;
    for (const CSubNet &subnet : rpc_allow_subnets)
        strAllowed += subnet.ToString() + " ";
    LogPrint(BCLog::HTTP, "Allowing HTTP connections from: %s\n", strAllowed);
    return true;
}

/** HTTP request method as string - use for logging only */
std::string RequestMethodString(HTTPRequest::RequestMethod m)
{
    switch (m)
    {
        case HTTPRequest::GET:
            return "GET";
        case HTTPRequest::POST:
            return "POST";
        case HTTPRequest::HEAD:
            return "HEAD";
        case HTTPRequest::PUT:
            return "PUT";
        case HTTPRequest::OPTIONS:
            return "OPTIONS";
        default:
            return "unknown";
    }
}


/**
 * This callback is responsible for creating a new SSL connection
 * and wrapping it in an OpenSSL bufferevent.  This is the way
 * we implement an https server instead of a plain old http server.
 */
static struct bufferevent* bevcb (struct event_base *base, void *arg)
{ struct bufferevent* r;
  SSL_CTX *ctx = (SSL_CTX *) arg;

  r = bufferevent_openssl_socket_new (base,
                                      -1,
                                      SSL_new (ctx),
                                      BUFFEREVENT_SSL_ACCEPTING,
                                      BEV_OPT_CLOSE_ON_FREE);
  return r;
}

/** HTTP request callback */
static void http_request_cb(struct evhttp_request *req, void *arg)
{
    auto *httpSock = (HTTPSocket*) arg;
    // Disable reading to work around a libevent bug, fixed in 2.2.0.
    if (event_get_version_number() >= 0x02010600 && event_get_version_number() < 0x02020001)
    {
        evhttp_connection *conn = evhttp_request_get_connection(req);
        if (conn)
        {
            bufferevent *bev = evhttp_connection_get_bufferevent(conn);
            if (bev)
            {
                bufferevent_disable(bev, EV_READ);
            }
        }
    }
    std::shared_ptr<HTTPRequest> hreq = std::make_shared<HTTPRequest>(req);

    LogPrint(BCLog::HTTP, "Received a %s request for %s from %s\n",
        RequestMethodString(hreq->GetRequestMethod()), hreq->GetURI(), hreq->GetPeer().ToString());

    // Early address-based allow check
    if (!httpSock->m_publicAccess && !ClientAllowed(hreq->GetPeer()))
    {
        LogPrint(BCLog::HTTP, "Request from %s not allowed\n", hreq ? hreq->GetPeer().ToString() : "unknown");
        hreq->WriteReply(HTTP_FORBIDDEN);
        return;
    }

    // Early reject unknown HTTP methods
    if (hreq->GetRequestMethod() == HTTPRequest::UNKNOWN)
    {
        hreq->WriteReply(HTTP_BADMETHOD);
        return;
    }
    
    hreq->WriteHeader("Access-Control-Allow-Origin", "*");
    hreq->WriteHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
    hreq->WriteHeader("Access-Control-Allow-Headers", "*");

    if (hreq->GetRequestMethod() == HTTPRequest::OPTIONS)
    {
        hreq->WriteReply(HTTP_NOCONTENT);
        return;
    }

    // Find registered handler for prefix
    std::string strURI = hreq->GetURI();
    std::string body = hreq->ReadBody();


    // Dispatch to worker thread
    // TODO (losty-nat): errors should be processed inside
    httpSock->m_requestProcessor->Process(strURI, body, hreq);
}

/** Callback to reject HTTP requests after shutdown. */
static void http_reject_request_cb(struct evhttp_request *req, void *)
{
    LogPrint(BCLog::HTTP, "Rejecting request while shutting down\n");
    evhttp_send_error(req, HTTP_SERVUNAVAIL, nullptr);
}

/** Event dispatcher thread */
static bool ThreadHTTP(struct event_base *base)
{
    util::ThreadRename("pocketcoin-http");
    LogPrint(BCLog::HTTP, "Entering http event loop\n");
    event_base_dispatch(base);
    // Event loop will be interrupted by InterruptHTTPServer()
    LogPrint(BCLog::HTTP, "Exited http event loop\n");
    return event_base_got_break(base) == 0;
}

/** Bind HTTP server to specified addresses */
static bool HTTPBindAddresses()
{
    int securePort = gArgs.GetArg("-rpcport", BaseParams().RPCPort());
    int publicPort = gArgs.GetArg("-publicrpcport", BaseParams().PublicRPCPort());
    int publicTlsPort = gArgs.GetArg("-publictlsrpcport", BaseParams().PublicTlsRPCPort());
    int staticPort = gArgs.GetArg("-staticrpcport", BaseParams().StaticRPCPort());
    int restPort = gArgs.GetArg("-restport", BaseParams().RestPort());
    int bindAddresses = 0;

    // Determine what addresses to bind to
    if (g_socket)
    {
        if (!(gArgs.IsArgSet("-rpcallowip") && gArgs.IsArgSet("-rpcbind")))
        { // Default to loopback if not allowing external IPs
            g_socket->BindAddress("::1", securePort);
            g_socket->BindAddress("127.0.0.1", securePort);
            if (gArgs.IsArgSet("-rpcallowip"))
            {
                LogPrintf("WARNING: option -rpcallowip was specified without -rpcbind; this doesn't usually make sense\n");
            }
            if (gArgs.IsArgSet("-rpcbind"))
            {
                LogPrintf("WARNING: option -rpcbind was ignored because -rpcallowip was not specified, refusing to allow everyone to connect\n");
            }
        }
        else if (gArgs.IsArgSet("-rpcbind"))
        { // Specific bind address
            for (const std::string& strRPCBind: gArgs.GetArgs("-rpcbind"))
            {
                std::string host;
                int port = securePort;
                SplitHostPort(strRPCBind, port, host);
                g_socket->BindAddress(host, port);
            }
        }

        bindAddresses += g_socket->GetAddressCount();
    }

    // Public sockets always bind to any IPs
    if (g_webSocket)
    {
        g_webSocket->BindAddress("::", publicPort);
        g_webSocket->BindAddress("0.0.0.0", publicPort);
        if (g_webSocketHttps)
        {
            g_webSocketHttps->BindAddress("::", publicTlsPort);
            g_webSocketHttps->BindAddress("0.0.0.0", publicTlsPort);
        }
    }
    if (g_staticSocket)
    {
        g_staticSocket->BindAddress("::", staticPort);
        g_staticSocket->BindAddress("0.0.0.0", staticPort);
    }
    if (g_restSocket)
    {
        g_restSocket->BindAddress("::", restPort);
        g_restSocket->BindAddress("0.0.0.0", restPort);
    }

    return bindAddresses;
}

/** libevent event log callback */
static void libevent_log_cb(int severity, const char *msg)
{
#ifndef EVENT_LOG_WARN
    // EVENT_LOG_WARN was added in 2.0.19; but before then _EVENT_LOG_WARN existed.
# define EVENT_LOG_WARN _EVENT_LOG_WARN
#endif
    if (severity >= EVENT_LOG_WARN) // Log warn messages and higher without debug category
        LogPrintf("libevent: %s\n", msg);
    else
        LogPrint(BCLog::LIBEVENT, "libevent: %s\n", msg);
}

using namespace std::chrono;

bool InitHTTPServer(const util::Ref& context, const std::shared_ptr<IRequestProcessor>& privateHandler, const std::shared_ptr<IRequestProcessor>& webHandler, const std::shared_ptr<IRequestProcessor>& restHandler, const std::shared_ptr<IRequestProcessor>& staticHandler)
{
    if (!InitHTTPAllowList())
        return false;

    // Redirect libevent's logging to our own log
    event_set_log_callback(&libevent_log_cb);
    // Update libevent's log handling. Returns false if our version of
    // libevent doesn't support debug logging, in which case we should
    // clear the BCLog::LIBEVENT flag.
    if (!UpdateHTTPServerLogging(LogInstance().WillLogCategory(BCLog::LIBEVENT)))
    {
        LogInstance().DisableCategory(BCLog::LIBEVENT);
    }

#ifdef WIN32
    evthread_use_windows_threads();
#else
    evthread_use_pthreads();
#endif
    
    int timeout = gArgs.GetArg("-rpcservertimeout", DEFAULT_HTTP_SERVER_TIMEOUT);

    raii_event_base base_ctr = obtain_event_base();
    eventBase = base_ctr.get();

    // General private socket
    g_socket = new HTTPSocket(eventBase, timeout, false);
    g_socket->RegisterRequestProcessor(privateHandler);

    // Additional pocketnet seocket
    if (gArgs.GetBoolArg("-api", DEFAULT_API_ENABLE))
    {
        g_webSocket = new HTTPWebSocket(eventBase, timeout, true);
        g_webSocket->RegisterRequestProcessor(webHandler);
        g_webSocketHttps = new HTTPWebSocket(eventBase, timeout, true, /* tls */ true);
        // TODO (losty-nat): register request processor?
    }

    if (gArgs.GetBoolArg("-rest", DEFAULT_REST_ENABLE))
    {
        g_restSocket = new HTTPSocket(eventBase, timeout, true);
        g_restSocket->RegisterRequestProcessor(restHandler);
    }

    if (gArgs.GetBoolArg("-static", DEFAULT_STATIC_ENABLE))
    {
        g_staticSocket = new HTTPSocket(eventBase, timeout, true);
        g_staticSocket->RegisterRequestProcessor(staticHandler);
    }
 
    if (!HTTPBindAddresses())
    {
        LogPrintf("Unable to bind any endpoint for RPC server\n");
        return false;
    }

    LogPrint(BCLog::HTTP, "Initialized HTTP server\n");

    // transfer ownership to eventBase/HTTP via .release()
    eventBase = base_ctr.release();
    return true;
}

bool UpdateHTTPServerLogging(bool enable)
{
#if LIBEVENT_VERSION_NUMBER >= 0x02010100
    if (enable)
    {
        event_enable_debug_logging(EVENT_DBG_ALL);
    } else
    {
        event_enable_debug_logging(EVENT_DBG_NONE);
    }
    return true;
#else
    // Can't update libevent logging if version < 02010100
    return false;
#endif
}

void StartHTTPServer()
{
    g_thread_http = std::thread(ThreadHTTP, eventBase);
    struct event_base* eventBase = EventBase();
    assert(eventBase);
    httpRPCTimerInterface = MakeUnique<HTTPRPCTimerInterface>(eventBase);
    RPCSetTimerInterface(httpRPCTimerInterface.get());
    // TODO (losty-nat): call StartHTTPSocket() here
}

void InterruptHTTPServer()
{
    LogPrint(BCLog::HTTP, "Interrupting HTTP server\n");
    if (g_socket) g_socket->InterruptHTTPSocket();
    if (g_webSocket) g_webSocket->InterruptHTTPSocket();
    if (g_webSocketHttps) g_webSocketHttps->InterruptHTTPSocket();
    if (g_staticSocket) g_staticSocket->InterruptHTTPSocket();
    if (g_restSocket) g_restSocket->InterruptHTTPSocket();
}

void StopHTTPServer()
{
    LogPrint(BCLog::HTTP, "Stopping HTTP server\n");

    LogPrint(BCLog::HTTP, "Waiting for HTTP worker threads to exit\n");
    if (g_socket) g_socket->StopHTTPSocket();
    if (g_webSocket) g_webSocket->StopHTTPSocket();
    if (g_webSocketHttps) g_webSocketHttps->StopHTTPSocket();
    if (g_staticSocket) g_staticSocket->StopHTTPSocket();
    if (g_restSocket) g_restSocket->StopHTTPSocket();

    if (eventBase)
    {
        LogPrint(BCLog::HTTP, "Waiting for HTTP event thread to exit\n");
        if (g_thread_http.joinable()) g_thread_http.join();
    }

    delete g_socket;
    g_socket = nullptr;

    delete g_webSocket;
    g_webSocket = nullptr;

    delete g_webSocketHttps;
    g_webSocketHttps = nullptr;

    delete g_staticSocket;
    g_staticSocket = nullptr;

    delete g_restSocket;
    g_restSocket = nullptr;

    if (eventBase)
    {
        event_base_free(eventBase);
        eventBase = nullptr;
    }
    LogPrint(BCLog::HTTP, "Stopped HTTP server\n");

    if (httpRPCTimerInterface) {
        RPCUnsetTimerInterface(httpRPCTimerInterface.get());
        httpRPCTimerInterface.reset();
    }
}

struct event_base *EventBase()
{
    return eventBase;
}

static void httpevent_callback_fn(evutil_socket_t, short, void *data)
{
    // Static handler: simply call inner handler
    HTTPEvent *self = static_cast<HTTPEvent *>(data);
    self->handler();
    if (self->deleteWhenTriggered)
        delete self;
}

SSLContext SetupTls()
{
    x509 cert;
    cert.Generate();
    SSLContext sslCtx(std::move(cert));
    sslCtx.Setup();
    return sslCtx;
}

HTTPSocket::HTTPSocket(struct event_base *base, int timeout, bool publicAccess, bool fUseTls):
    m_http(nullptr), m_eventHTTP(nullptr), m_publicAccess(publicAccess)
{
    /* Create a new evhttp object to handle requests. */
    raii_evhttp http_ctr = obtain_evhttp(base);
    m_http = http_ctr.get();
    if (!m_http)
    {
        LogPrintf("couldn't create evhttp. Exiting.\n");
        return;
    }

    evhttp_set_timeout(m_http, gArgs.GetArg("-rpcservertimeout", DEFAULT_HTTP_SERVER_TIMEOUT));
    evhttp_set_max_headers_size(m_http, MAX_HEADERS_SIZE);
    evhttp_set_max_body_size(m_http, MAX_SIZE);
    evhttp_set_gencb(m_http, http_request_cb, (void*) this);
    if (fUseTls)
    {
        m_sslCtx = SetupTls();
        evhttp_set_bevcb(m_http, bevcb, (void*)m_sslCtx->GetCtx());
    }
    evhttp_set_allowed_methods(m_http,
        evhttp_cmd_type::EVHTTP_REQ_GET |
        evhttp_cmd_type::EVHTTP_REQ_POST |
        evhttp_cmd_type::EVHTTP_REQ_HEAD |
        evhttp_cmd_type::EVHTTP_REQ_PUT |
        evhttp_cmd_type::EVHTTP_REQ_DELETE |
        evhttp_cmd_type::EVHTTP_REQ_OPTIONS
    );

    // transfer ownership to eventBase/HTTP via .release()
    m_eventHTTP = http_ctr.release(); 
}

HTTPSocket::~HTTPSocket()
{
    if (m_eventHTTP)
    {
        evhttp_free(m_eventHTTP);
        m_eventHTTP = nullptr;
    }
}


void HTTPSocket::StopHTTPSocket()
{
    // Unlisten sockets, these are what make the event loop running, which means
    // that after this and all connections are closed the event loop will quit.
    for (evhttp_bound_socket *socket : m_boundSockets)
    {
        evhttp_del_accept_socket(m_eventHTTP, socket);
    }
    m_boundSockets.clear();
}

void HTTPSocket::InterruptHTTPSocket()
{
    // TODO (losty-nat): socket is not usable after this call.
    //                   Need to set this callback back to http_request_cb in StartHTTPSocket()
    if (m_eventHTTP)
    {
        // Reject requests on current connections
        evhttp_set_gencb(m_eventHTTP, http_reject_request_cb, nullptr);
    }
}

void HTTPSocket::BindAddress(std::string ipAddr, int port)
{ 
    LogPrint(BCLog::HTTP, "Binding RPC on address %s port %i\n", ipAddr, port);
    evhttp_bound_socket *bind_handle = evhttp_bind_socket_with_handle(m_eventHTTP, ipAddr.empty() ? nullptr : ipAddr.c_str(), port);
    if (bind_handle)
    {
        CNetAddr addr;
        if (ipAddr.empty() || (LookupHost(ipAddr, addr, false) && addr.IsBindAny())) {
            // TODO (aok, lostystyg): only for private ports
            LogPrintf("WARNING: the RPC server is not safe to expose to untrusted networks such as the public internet. %s:%d\n", ipAddr, port);
        }
        m_boundSockets.push_back(bind_handle);
    }
    else
    {
        LogPrint(BCLog::HTTP,"Binding RPC on address %s port %i failed.\n", ipAddr, port);
    }
}

int HTTPSocket::GetAddressCount()
{
    return (int)m_boundSockets.size();
}

/** WebSocket for public API */
HTTPWebSocket::HTTPWebSocket(struct event_base* base, int timeout, bool publicAccess, bool fUseTls)
    : HTTPSocket(base, timeout, publicAccess, fUseTls)
{}

HTTPWebSocket::~HTTPWebSocket() = default;

void HTTPWebSocket::StopHTTPSocket()
{   
    // Interrupting socket here because stop without interrupting is illegal.
    HTTPSocket::InterruptHTTPSocket();
    HTTPSocket::StopHTTPSocket();
}

void HTTPWebSocket::InterruptHTTPSocket()
{
    HTTPSocket::InterruptHTTPSocket();
}

HTTPEvent::HTTPEvent(struct event_base *base, bool _deleteWhenTriggered, std::function<void()> _handler) :
    deleteWhenTriggered(_deleteWhenTriggered), handler(std::move(_handler))
{
    ev = event_new(base, -1, 0, httpevent_callback_fn, this);
    assert(ev);
}

HTTPEvent::~HTTPEvent()
{
    event_free(ev);
}

void HTTPEvent::trigger(struct timeval *tv)
{
    if (tv == nullptr)
        event_active(ev, 0, 0); // immediately trigger event in main thread
    else
        evtimer_add(ev, tv); // trigger after timeval passed
}

HTTPRequest::HTTPRequest(struct evhttp_request *_req, bool _replySent) : req(_req),
                                                        replySent(_replySent)
{
    Created = gStatEngineInstance.GetCurrentSystemTime();
    m_peerStr = GetPeer().ToString();
}

Statistic::RequestTime HTTPRequest::GetCreated() const
{
    return Created;
}

HTTPRequest::~HTTPRequest()
{
    if (!replySent)
    {
        // Keep track of whether reply was sent to avoid request leaks
        LogPrintf("%s: Unhandled request\n", __func__);
        WriteReply(HTTP_INTERNAL_SERVER_ERROR, "Unhandled request");
    }
    // evhttpd cleans up the request, as long as a reply was sent.
}

std::pair<bool, std::string> HTTPRequest::GetHeader(const std::string &hdr) const
{
    const struct evkeyvalq *headers = evhttp_request_get_input_headers(req);
    assert(headers);
    const char *val = evhttp_find_header(headers, hdr.c_str());
    if (val)
        return std::make_pair(true, val);
    else
        return std::make_pair(false, "");
}

std::string HTTPRequest::ReadBody()
{
    struct evbuffer *buf = evhttp_request_get_input_buffer(req);
    if (!buf)
        return "";
    size_t size = evbuffer_get_length(buf);
    /** Trivial implementation: if this is ever a performance bottleneck,
     * internal copying can be avoided in multi-segment buffers by using
     * evbuffer_peek and an awkward loop. Though in that case, it'd be even
     * better to not copy into an intermediate string but use a stream
     * abstraction to consume the evbuffer on the fly in the parsing algorithm.
     */
    const char *data = (const char *) evbuffer_pullup(buf, size);
    if (!data) // returns nullptr in case of empty buffer
        return "";
    std::string rv(data, size);
    evbuffer_drain(buf, size);
    return rv;
}

void HTTPRequest::WriteHeader(const std::string &hdr, const std::string &value)
{
    struct evkeyvalq *headers = evhttp_request_get_output_headers(req);
    assert(headers);
    evhttp_add_header(headers, hdr.c_str(), value.c_str());
}

/** Closure sent to main thread to request a reply to be sent to
 * a HTTP request.
 * Replies must be sent in the main loop in the main http thread,
 * this cannot be done from worker threads.
 */
void HTTPRequest::WriteReply(int nStatus, const std::string &strReply)
{
    assert(!replySent && req);
    if (ShutdownRequested())
    {
        WriteHeader("Connection", "close");
    }
    // Send event to main http thread to send reply message
    struct evbuffer *evb = evhttp_request_get_output_buffer(req);
    assert(evb);
    evbuffer_add(evb, strReply.data(), strReply.size());
    auto req_copy = req;
    auto *ev = new HTTPEvent(eventBase, true, [req_copy, nStatus]
    {
        evhttp_send_reply(req_copy, nStatus, nullptr, nullptr);
        // Re-enable reading from the socket. This is the second part of the libevent
        // workaround above.
        if (event_get_version_number() >= 0x02010600 && event_get_version_number() < 0x02020001)
        {
            evhttp_connection *conn = evhttp_request_get_connection(req_copy);
            if (conn)
            {
                bufferevent *bev = evhttp_connection_get_bufferevent(conn);
                if (bev)
                {
                    bufferevent_enable(bev, EV_READ | EV_WRITE);
                }
            }
        }
    });
    ev->trigger(nullptr);
    replySent = true;
    req = nullptr; // transferred back to main thread
}

bool HTTPRequest::GetAuthCredentials(std::string& out)
{
    auto[ok, cred] = this->GetHeader("authorization");
    out = cred;
    return ok;
}

const std::string& HTTPRequest::GetPeerStr() const
{
    return m_peerStr;
}

CService HTTPRequest::GetPeer() const
{
    evhttp_connection *con = evhttp_request_get_connection(req);
    CService peer;
    if (con)
    {
        // evhttp retains ownership over returned address string
        const char *address = "";
        uint16_t port = 0;
        evhttp_connection_get_peer(con, (char **) &address, &port);
        peer = LookupNumeric(address, port);
    }
    return peer;
}

std::string HTTPRequest::GetURI() const
{
    return evhttp_request_get_uri(req);
}

HTTPRequest::RequestMethod HTTPRequest::GetRequestMethod() const
{
    switch (evhttp_request_get_command(req))
    {
        case EVHTTP_REQ_GET:
            return GET;
            break;
        case EVHTTP_REQ_POST:
            return POST;
            break;
        case EVHTTP_REQ_HEAD:
            return HEAD;
            break;
        case EVHTTP_REQ_PUT:
            return PUT;
            break;
        case EVHTTP_REQ_OPTIONS:
            return OPTIONS;
            break;
        default:
            return UNKNOWN;
            break;
    }
}
