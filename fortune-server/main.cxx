// Application
#include <Poco/Util/ServerApplication.h>

// Crypt
#include <Poco/Random.h>

// HTTPServer
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>

// Streams
#include <Poco/FileStream.h>

class FortuneRequestHandler : public Poco::Net::HTTPRequestHandler
{
public:
    FortuneRequestHandler()
    {
        const std::vector<std::string> fortunes{
            {"The first principle is that you must not fool yourself -- and you are the easiest person to fool. -- Richard Feynman",
             "The greater danger for most of us lies not in setting our aim too high and falling short; but in setting our aim too low, and achieving our mark. -- Michelangelo",
             "A friend is someone who understands your past, believes in your future, and accepts you just the way you are.",
             "Be kind, for everyone you meet is fighting a hard battle. -- Plato",
             "Happiness is when what you think, what you say, and what you do are in harmony. -- Gandhi",
             "We must accept finite disappointment, but never lose infinite hope. -- Martin Luther King Jr."}};

        Poco::Random random;
        fortune_ = {fortunes.at(random.next(fortunes.size()))};
    }

    void handleRequest(Poco::Net::HTTPServerRequest &request, Poco::Net::HTTPServerResponse &response)
    {
        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        std::ostream &ostr = response.send();
        ostr << "<html>";
        ostr << "<head>";
        ostr << "<title>Fortunes</title>";
        ostr << "</head>";
        ostr << "<body>";
        ostr << "<p style=\"text-align: center; font-size: 48px;\">";
        ostr << fortune_;
        ostr << "</p>";
        ostr << "<p style=\"text-align: left; font-size: 12px;\">";
        ostr << "Client address: " << request.clientAddress().toString();
        ostr << "</p>";
        ostr << "</body>";
        ostr << "</html>";
    }

    std::string fortune_;
};

class FortuneRequestHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory
{
public:
    Poco::Net::HTTPRequestHandler *createRequestHandler(const Poco::Net::HTTPServerRequest &request)
    {
        if (request.getURI() == "/")
        {
            return new FortuneRequestHandler;
        }

        return 0;
    }
};

class FortuneServerApplication : public Poco::Util::ServerApplication
{
public:
    FortuneServerApplication()
    {
    }

protected:
    void initialize(Application &self)
    {
        loadConfiguration();

        ServerApplication::initialize(self);
    }

    void uninitialize()
    {
        ServerApplication::uninitialize();
    }

    int main(const std::vector<std::string> &args)
    {
        unsigned short port = config().getUInt("HTTPTimeServer.port", 9999);

        // The server takes ownership of the HTTPRequstHandlerFactory
        Poco::Net::HTTPServer server(new FortuneRequestHandlerFactory, port);
        server.start();
        waitForTerminationRequest();
        server.stop();

        return Application::EXIT_OK;
    }
};

int main(int argc, char **argv)
{
    FortuneServerApplication app;
    return app.run(argc, argv);
}
