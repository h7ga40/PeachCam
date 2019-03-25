//#define _DEBUG_ALL

#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#ifdef _DEBUG_ALL
#include <stdio.h>
#include <stdarg.h>
static inline void http_server_debug_print(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}
#else
static inline void http_server_debug_print(const char *format, ...) {}
#endif

#include <string>
using std::string;

#include <map>
using std::map;

#include "HTTPRequestHandler.h"
#include "rtos/rtos.h"
#include "mbed.h"

struct handlersComp { //Used to order handlers in the right way
    bool operator() (const string& handler1, const string& handler2) const {
        //The first handler is longer than the second one
        if (handler1.length() > handler2.length())
            return true; //Returns true if handler1 is to appear before handler2
        else if (handler1.length() < handler2.length())
            return false;
        else //To avoid the == case, sort now by address
            return ((&handler1)>(&handler2));
    }
};

map< string, HTTPRequestHandler*(*)(const char*, const char* , TCPSocket* ), handlersComp > m_lpHandlers;
template<typename T>
void HTTPServerAddHandler(const char* path)  //Template decl in header
{
    m_lpHandlers[path] = &T::inst;
}

void ListenThread(void const *args);
enum HTTP_METH {
    HTTP_GET,
    HTTP_POST,
    HTTP_HEAD
};

bool getRequest(TCPSocket* client, string* path, string* meth)
{
    char req[128];
    char c_path[128];
    char c_meth[128];
    const int maxLen = 128;
    char* p = req;
    //Read Line
    int ret;
    int len = 0;
    for(int i = 0; i < maxLen - 1; i++) {
        ret = client->recv(p, 1);
        if(!ret) {
            break;
        }
        if( (len > 1) && *(p-1)=='\r' && *p=='\n' ) {
            p--;
            len-=2;
            break;
        } else if( *p=='\n' ) {
            len--;
            break;
        }
        p++;
        len++;
    }
    *p = 0;
    http_server_debug_print("Parsing request : %s\r\n", req);
    ret = sscanf(req, "%s %s HTTP/%*d.%*d", c_meth, c_path);
    if(ret !=2)        return false;
    *meth = string(c_meth);
    *path = string(c_path);
    return true;
}

void dispatchRequest(TCPSocket* client)
{
    string path;
    string meth;
    HTTP_METH methCode;
    http_server_debug_print("Dispatching req\r\n");
    if( !getRequest(client,&path, &meth ) ) {
        http_server_debug_print("dispatchRequest Invalid request\r\n");
        return; //Invalid request
    }
    if( !meth.compare("GET") ) {
        http_server_debug_print("dispatchRequest HTTP_GET\r\n");
        methCode = HTTP_GET;
    } else if( !meth.compare("POST") ) {
        http_server_debug_print("dispatchRequest HTTP_POST\r\n");
        methCode = HTTP_POST;
    } else if( !meth.compare("HEAD") ) {
        http_server_debug_print("dispatchRequest HTTP_HEAD\r\n");
        methCode = HTTP_HEAD;
    } else {
        http_server_debug_print("dispatchRequest() Parse error\r\n");
        return;
    }
    http_server_debug_print("Looking for a handler\r\n");
    map< string, HTTPRequestHandler*(*)(const char*, const char*, TCPSocket*), handlersComp >::iterator it;
    int root_len = 0;
    for (it = m_lpHandlers.begin(); it != m_lpHandlers.end(); it++) {
        http_server_debug_print("Checking %s...\r\n", (*it).first.c_str());
        root_len = (*it).first.length();
        if ( root_len &&
                !path.compare( 0, root_len, (*it).first ) &&
                (path[root_len] == '/' || path[root_len] == '\0')) {
            http_server_debug_print("Found (%s)\r\n", (*it).first.c_str());
            // Found!
            break;  // for
        }
    }
    if((it == m_lpHandlers.end()) && !(m_lpHandlers.empty())) {
        http_server_debug_print("Using default handler\r\n");
        it = m_lpHandlers.end();
        it--; //Get the last element
        if( ! (((*it).first.length() == 0) || !(*it).first.compare("/")) ) //This is not the default handler
            it = m_lpHandlers.end();
        root_len = 0;
    }
    if(it == m_lpHandlers.end()) {
        http_server_debug_print("No handler found\r\n");
        return;
    }
    http_server_debug_print("Handler found.\r\n");
    HTTPRequestHandler* pHdlr = (*it).second((*it).first.c_str(), path.c_str() + root_len, client);
    //****  client = NULL; //We don't own it anymore
    switch(methCode) {
        case HTTP_GET:
            pHdlr->doGet();
            break;
        case HTTP_POST:
            pHdlr->doPost();
            break;
        case HTTP_HEAD:
            pHdlr->doHead();
            break;
    }
    delete pHdlr;
    http_server_debug_print("(dispatcherRequest)return\r\n");
    return ;
}

#define THREAD_MAX 1

#if (THREAD_MAX > 1) 
static Thread *threads[THREAD_MAX];
static Thread *xthread;

static void HTTPServerChild (TCPSocket* client)
{
    http_server_debug_print("HTTPServerChiled Start......\r\n");

    for (;;) {
        http_server_debug_print("(HTTPServer.h<HTTPServerChild>)Connection\r\n");
        dispatchRequest(client);
        http_server_debug_print("(HTTPServer.h<HTTPServerChild>)Close\r\n");
        client->close();
        Thread::signal_wait(1);
    }
}

static void HTTPServerCloser (TCPSocket *client)
{
    for (;;) {
        client->close();
        http_server_debug_print("Close\r\n");
        Thread::signal_wait(1);
    }
}

void HTTPServerStart(NetworkInterface *net, int port = 80, osPriority priority = osPriorityNormal)
{
    int i, t = 0;
    TCPSocket clients[THREAD_MAX];
    TCPSocket xclient;

    for (i = 0; i < THREAD_MAX; i ++) {
        threads[i] = NULL;
    }
    xthread = NULL;

    TCPServer server(net);
    server.bind(port);
    server.listen();
    http_server_debug_print("Wait for new connection...\r\n");

    for (;;) {
        http_server_debug_print("**Start Loop** \r\n");
        if (t >= 0) {
            if(server.accept(&clients[t])==0) {
                // fork child process
                if (threads[t]) {
                    threads[t]->signal_set(1);
                } else {
                    threads[t] = new Thread(priority, 1024 * 3);
                    threads[t]->start(callback(HTTPServerChild, &clients[t]));
                }
                http_server_debug_print("Forked %d\r\n", t);
            } else {
                ThisThread::sleep_for(10);
            }
        } else {
            if(server.accept(&xclient)==0) {
                // closer process
                if (xthread) {
                    xthread->signal_set(1);
                } else {
                    xthread = new Thread(priority);
                    xthread->start(callback(HTTPServerCloser, &xclient));
                }
                http_server_debug_print("Connection full\r\n");
            } else {
                ThisThread::sleep_for(10);
            }
        }

        t = -1;
        for (i = 0; i < THREAD_MAX; i ++) {
            if ((threads[i] == NULL)
             || ((threads[i]->get_state() == Thread::WaitingAnd))) {
                if (t < 0) t = i; // next empty thread
            }
        }
    }
}

#else  // THREAD_MAX == 1

void HTTPServerStart(NetworkInterface *net, int port = 80, osPriority priority = osPriorityNormal)
{
    TCPSocket client;

    (void)priority;

    TCPServer server(net);
    server.bind(port);
    server.listen();
    http_server_debug_print("Wait for new connection...\r\n");

    while (1) {
        http_server_debug_print("**Start Loop** \r\n");
        if (server.accept(&client) == 0) {
            dispatchRequest(&client);
            client.close();
        }
    }
}

#endif

#include "Handler/RPCHandler.h"
#include "Handler/FSHandler.h"
#include "Handler/SimpleHandler.h"
#include "SnapshotHandler.h"

#endif
