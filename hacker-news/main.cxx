// NOTE: Poco doesn't have async HTTP APIs.

// Package: Application
#include <Poco/Util/Application.h>

// Package: Core
#include <Poco/SharedPtr.h>

// Package: Dynamic
#include <Poco/Dynamic/Var.h>

// Package: Foundation
#include <Poco/Timespan.h>
#include <Poco/URI.h>
#include <Poco/StreamCopier.h>

// Package: HTTP
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPMessage.h>

// Package: HTTPSClient
#include <Poco/Net/HTTPSClientSession.h>

// Package: JSON
#include <Poco/JSON/JSON.h>
#include <Poco/JSON/Parser.h>

// Package: NetCore
#include <Poco/Net/NetException.h>

// Package: SSLCore
#include <Poco/Net/Context.h>
#include <Poco/Net/SSLException.h>

// Package: Threading
#include <Poco/Runnable.h>
#include <Poco/ScopedLock.h>
#include <Poco/Thread.h>
#include <Poco/ThreadPool.h>

static std::string InvokeWebRequest(const std::string &uriString)
{
    const Poco::URI uri(uriString);

    // sendRequest may show:
    //
    // WARNING: Certificate verification failed
    // ----------------------------------------
    // Issuer Name:  C=BE,O=GlobalSign nv-sa,OU=Root CA,CN=GlobalSign Root CA
    // Subject Name: C=US,O=Google Trust Services LLC,CN=GTS Root R1
    //
    // The certificate yielded the error: unable to get local issuer certificate
    //
    // The error occurred in the certificate chain at position 2
    // Accept the certificate (y,n)?
    //
    // Why?
    // Due to intermediate certificates in its chain, so you will have to add all the intermediate CAs
    //  presented to your trusted store to get this to work. If you don't want to perform certificate
    // verification, use VERIFY_NONE by passing context object to HTTPSClientSession ctor.
    const Poco::Net::Context::Ptr context = new Poco::Net::Context(
        Poco::Net::Context::TLS_CLIENT_USE, // usage
        "",                                 // caLocation
        Poco::Net::Context::VERIFY_NONE     // verificationMode (default: VERIFY_RELAXED)
    );
    Poco::Net::HTTPSClientSession session(uri.getHost(), uri.getPort(), context);
    session.setTimeout(Poco::Timespan(10 /*seconds*/, 0 /*microseconds*/));

    Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_GET, uri.getPath(), Poco::Net::HTTPMessage::HTTP_1_1);

    (void)session.sendRequest(request);

    Poco::Net::HTTPResponse response;
    std::istream &is = session.receiveResponse(response);

    // e.g. "200 OK"
    // std::cout << response.getStatus() << " " << response.getReason() << std::endl;
    // if (response.getStatus() == Poco::Net::HTTPResponse::HTTP_OK) ...

    std::stringstream ss;
    Poco::StreamCopier::copyStream(is, ss);

    return ss.str();
}

class IdCollection
{
public:
    IdCollection(const std::string &json)
    {
        Poco::JSON::Parser parser;
        Poco::Dynamic::Var objects = parser.parse(json);
        if (!objects.isArray() || objects.size() != 1)
        {
            throw Poco::InvalidArgumentException("Expecting one JSON array");
        }

        Poco::JSON::Array::Ptr jsonArray = objects[0].extract<Poco::JSON::Array::Ptr>();

        // Convert JSON array to ids stack
        for (const auto &item : *jsonArray)
        {
            unsigned int id;
            item.convert(id);
            ids_.push(id);
        }
    }

    // Synchronized access to ids. Returns false if no more items.
    bool Next(unsigned int &id)
    {
        bool success = false;
        Poco::Mutex::ScopedLock lock(mutex_);
        if (!ids_.empty())
        {
            id = ids_.top();
            ids_.pop();
            success = true;
        }
        return success;
    }

private:
    Poco::Mutex mutex_;
    std::stack<unsigned int> ids_;
};

class Worker : public Poco::Runnable
{
public:
    Worker(IdCollection &ids) : ids_(ids) {}

    virtual void run()
    {
        std::string ITEM_URL_BASE{"https://hacker-news.firebaseio.com/v0/item/"};
        unsigned int id;
        while (ids_.Next(id))
        {
            const std::string uri{Poco::cat(ITEM_URL_BASE, std::to_string(id), std::string(".json"))};

            const std::string response{InvokeWebRequest(uri)};

            // {
            //  "by":"janniks",
            //  "descendants":297,
            //  "id":35056379,
            //   "kids":[35058025,35056380],
            //   "score":557,
            //   "time":1678202909,
            //   "title":"Hardware microphone disconnect (2021)",
            //    "type":"story",
            //    "url":"https://support.apple.com/guide/security/hardware-microphone-disconnect-secbbd20b00b/web"
            // }

            Poco::JSON::Parser parser;
            Poco::Dynamic::Var objects = parser.parse(response);
            if (objects.size() != 1)
            {
                std::cerr << "Expecting one object from item" << std::endl;
                return;
            }

            Poco::JSON::Object::Ptr object = objects[0].extract<Poco::JSON::Object::Ptr>();
            const unsigned story_id{object->getValue<unsigned int>("id")};
            const std::string story_title{object->getValue<std::string>("title")};
            std::cout << story_id << " : " << story_title << " (TID " << Poco::Thread::currentTid() << ")" << std::endl;
        }
    }

private:
    Poco::Mutex mutex;
    IdCollection &ids_;
};

class Application : public Poco::Util::Application
{
    int main(const std::vector<std::string> &arguments);
};

// Poco::Util::Application::main will catch exceptions.
int Application::main(const std::vector<std::string> &arguments)
{
    // e.g. [35056379,35060298,35062007,35060438,35060273,35055121,35056548,35056094,35060972]
    const std::string response{InvokeWebRequest("https://hacker-news.firebaseio.com/v0/topstories.json")};
    IdCollection collection(response);

    // Start threads
    std::vector<Poco::SharedPtr<Worker>> runnables;
    for (unsigned i = 0; i < 8; ++i)
    {
        Poco::SharedPtr<Worker> worker(new Worker(collection));
        runnables.push_back(worker);
        Poco::ThreadPool::defaultPool().start(*worker);
    }

    // Wait for workers to complete
    Poco::ThreadPool::defaultPool().joinAll();

    return 0;
}

POCO_APP_MAIN(Application)
