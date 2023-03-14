#include <Poco/Foundation.h>

// Package: Application
#include <Poco/Util/Application.h>

// Package: Process
#include <Poco/Process.h>
#include <Poco/PipeStream.h>

class Application : public Poco::Util::Application
{
private:
    int main(const std::vector<std::string> &arguments) override;
};

int Application::main(const std::vector<std::string> &arguments)
{
#ifdef POCO_OS_FAMILY_WINDOWS
    const std::string path("ping.exe");
#else
    const std::string path("/usr/bin/ping");
#endif

    std::vector<std::string> args{{"-n", "1", "-w", "1", "microsoft.com"}};
    Poco::Pipe outPipe;

    try
    {
        Poco::ProcessHandle handle{Poco::Process::launch(path, args, nullptr /*inPipe*/, &outPipe, nullptr /*errPipe*/)};
        handle.wait();
    }
    catch (const Poco::SystemException &e)
    {
        std::cerr << "[Failure] " << e.displayText() << std::endl;
        return EXIT_FAILURE;
    }

    Poco::PipeInputStream processStdOut(outPipe);
    std::cout << processStdOut.rdbuf() << std::endl;

    return EXIT_OK;
}

POCO_APP_MAIN(Application)
