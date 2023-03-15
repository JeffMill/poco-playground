// Adapted from: https://gist.github.com/sekia/d249b44104cf89653b404f1c9cf597f4
//
// usage: SimpleWatcher [options] [--] command ...
// /target=<path>  Target directory.
// /help           Show help message.
//
// e.g. SimpleWatcher /target=d:\temp ping 127.0.0.1 -n 1 -w 1
//
// Will launch "ping 127.0.0.1 -n 1 -w 1" everytime a change in d:\temp is detected.

#include <iostream>
#include <memory>
#include <string>

#include <Poco/Delegate.h>
#include <Poco/DirectoryWatcher.h>
#include <Poco/Event.h>
#include <Poco/Exception.h>
#include <Poco/File.h>
#include <Poco/Mutex.h>
#include <Poco/Path.h>
#include <Poco/Process.h>
#include <Poco/ScopedLock.h>
#include <Poco/Thread.h>
#include <Poco/Util/Application.h>
#include <Poco/Util/HelpFormatter.h>
#include <Poco/Util/Option.h>
#include <Poco/Util/OptionCallback.h>
#include <Poco/Util/OptionException.h>
#include <Poco/Util/OptionSet.h>
#include <Poco/Util/Validator.h>

using Poco::Delegate;
using Poco::DirectoryWatcher;
using Poco::Event;
using Poco::Exception;
using Poco::FastMutex;
using Poco::File;
using Poco::Path;
using Poco::Process;
using Poco::Thread;
using Poco::Util::Application;
using Poco::Util::HelpFormatter;
using Poco::Util::Option;
using Poco::Util::OptionCallback;
using Poco::Util::OptionException;
using Poco::Util::OptionSet;
using Poco::Util::Validator;

using DirectoryEvent = DirectoryWatcher::DirectoryEvent;
using ScopedLock = FastMutex::ScopedLock;

namespace
{
    // Given a folder, return the absolute path to that folder.
    File GetDirectoryFor(const std::string &path_str)
    {
        Path path = Path::forDirectory(path_str);
        if (path.isRelative())
        {
            path = Path::forDirectory(Path::current()).resolve(path);
        }
        return File(path);
    }
}

// Validator implementation for directory argument.
// Resolves directory argument into absolute path and ensures it exists
// and it's a directory.
class DirectoryOptionValidator : public Validator
{
public:
    void validate(const Option &, const std::string &path) override
    {
        try
        {
            File dir = GetDirectoryFor(path);
            if (!(dir.exists() && dir.isDirectory()))
            {
                throw OptionException("Specified directory does not exist.");
            }
        }
        catch (const OptionException &e)
        {
            e.rethrow();
        }
        catch (const Exception &e)
        {
            throw OptionException(
                "Error occured during directory path parsing.", e);
        }
    }
};

class SimpleWatcher : public Application
{
protected:
    void defineOptions(OptionSet &options) override
    {
        Application::defineOptions(options);

        options.addOption(
            Option("target", "t", "Target directory.")
                .argument("<path>", true)
                .callback(
                    OptionCallback<SimpleWatcher>(
                        this, &SimpleWatcher::HandleTargetOption))
                .required(true)
                .validator(new DirectoryOptionValidator()));

        options.addOption(
            Option("help", "h", "Show help message.")
                .callback(
                    OptionCallback<SimpleWatcher>(
                        this, &SimpleWatcher::HandleHelpOption)));
    }

    int main(const std::vector<std::string> &args) override
    {
        if (help_ || args.empty())
        {
            HelpFormatter formatter(options());
            formatter.setCommand(commandName());
            formatter.setUsage("[options] [--] command ...");
            formatter.format(std::cout);

            return EXIT_OK;
        }

        const std::string &command = args[0];
        const std::vector<std::string> command_args(args.begin() + 1, args.end());

        FastMutex lock;
        bool stop_launcher = false;

        Thread launcher;
        launcher.startFunc(
            [&]()
            {
                while (true)
                {
                    // Wait for signal from directory change.
                    awake_launcher_.wait();

                    ScopedLock guard(lock);
                    if (stop_launcher)
                    {
                        break;
                    }

                    ExecuteCommand(command, command_args);
                }
            });

        // Have the launcher thread do an initial iteration.
        awake_launcher_.set();

        std::cout << "Press enter to quit" << std::endl;
        std::string in;
        std::getline(std::cin, in);
        std::cout << "Shutting down" << std::endl;

        // Stop the launcher thread.
        lock.lock();
        stop_launcher = true;
        lock.unlock();
        awake_launcher_.set();
        launcher.join();

        return EXIT_OK;
    }

private:
    void ExecuteCommand(
        const std::string &command,
        const std::vector<std::string> &args) const
    {
        std::cout << "Launching " << command << std::endl;
        Process::wait(Process::launch(command, args));
    }

    // Delegate for the DirectoryWatcher.
    void HandleDirectoryEvent(const DirectoryEvent &de)
    {
        std::cout << "Directory changed. File: " << de.item.path() << "; Type: " << de.event << std::endl;
        awake_launcher_.set();
    }

    // OptionCallback for the "help" commandline option.
    void HandleHelpOption(const std::string &, const std::string &) noexcept
    {
        help_ = true;
        stopOptionsProcessing();
    }

    // OptionCallback for the "directory" commandline option.
    void HandleTargetOption(const std::string &, const std::string &dir_path)
    {
        std::cout << "Monitoring " << dir_path << std::endl;

        auto watcher = new DirectoryWatcher(GetDirectoryFor(dir_path));
        watcher_ = std::unique_ptr<DirectoryWatcher>(watcher);

        auto event_handler =
            Delegate<SimpleWatcher, const DirectoryEvent, false>(
                this, &SimpleWatcher::HandleDirectoryEvent);
        watcher_->itemAdded += event_handler;
        watcher_->itemModified += event_handler;
        watcher_->itemMovedFrom += event_handler;
        watcher_->itemMovedTo += event_handler;
        watcher_->itemRemoved += event_handler;
    }

    Event awake_launcher_;
    bool help_ = false;
    std::unique_ptr<DirectoryWatcher> watcher_;
};

POCO_APP_MAIN(SimpleWatcher)
