// Package: Application
#include <Poco/Util/Application.h>

// Package: Configuration
#include <Poco/Util/IniFileConfiguration.h>
#include <Poco/Util/JSONConfiguration.h>
#include <Poco/Util/LayeredConfiguration.h>

// Package: Core
#include <Poco/String.h>

// Package: Crypt
#include <Poco/DigestStream.h>
#include <Poco/SHA2Engine.h>

// Package: Filesystem
#include <Poco/File.h>
#include <Poco/Glob.h>

// Package: Options
#include <Poco/Util/HelpFormatter.h>
#include <Poco/Util/Option.h>
#include <Poco/Util/OptionCallback.h>
#include <Poco/Util/OptionSet.h>

// Package: Streams
#include <Poco/FileStream.h>
#include <Poco/NullStream.h>
#include <Poco/StreamCopier.h>

class Application : public Poco::Util::Application
{
private:
    void defineOptions(Poco::Util::OptionSet &options) override
    {
        Poco::Util::Application::defineOptions(options);

        options.addOption(Poco::Util::Option("binary", "b", "read in binary mode"));
        options.addOption(Poco::Util::Option("check", "c", "read SHA256 sums from the FILEs and check them"));
        options.addOption(Poco::Util::Option("tag", "", "create a BSD-style checksum"));
        options.addOption(Poco::Util::Option("help", "", "display this help and exit."));
    }

    void handleOption(const std::string &name, const std::string &value) override
    {
        Poco::Util::Application::handleOption(name, value);

        if (name == "binary")
        {
            arg_binary = true;
        }
        if (name == "check")
        {
            arg_check = true;
        }
        else if (name == "tag")
        {
            arg_tag = true;
        }
        else if (name == "help")
        {
            arg_help = true;
        }
    }

    int main(const std::vector<std::string> &arguments) override;

    // Command line args

    void ShowHelpMessage() const noexcept
    {
        Poco::Util::HelpFormatter formatter(options());
        formatter.setCommand(commandName());
        formatter.setUsage("[OPTION]... [FILE]...");
        formatter.format(std::cout);
    }

    std::vector<std::string> ExpandFileArgument(const std::string &file);
    void DisplayHash(const std::string &file);
    void ReadConfig();

    bool arg_binary = false;
    bool arg_check = false;
    bool arg_tag = false;
    bool arg_help = false;
};

std::vector<std::string> Application::ExpandFileArgument(const std::string &file)
{
    std::vector<std::string> files;

    Poco::Path path(file);
    const std::string fileName(path.getFileName());
    if (fileName.empty())
    {
        // Only a pathname was specified
        return files;
    }

    if (fileName.find('?') == std::string::npos && fileName.find('*') == std::string::npos)
    {
        // Not a wildcard
        files.push_back(path.absolute().toString());
        return files;
    }

    // Wildcard
    std::set<std::string> matches;
    Poco::Glob::glob(path.absolute().toString(), matches, Poco::Glob::GLOB_CASELESS | Poco::Glob::GLOB_DOT_SPECIAL);
    for (const std::string &fileMatch : matches)
    {
        Poco::Path path(fileMatch);
        // include directories as well. They'll be filtered later.
        files.push_back(path.toString());
    }

    return files;
}

void Application::DisplayHash(const std::string &pathName)
{
    Poco::Path path(pathName);
    Poco::File file(path);
    if (file.isDirectory())
    {

        std::cout << commandName() << ": " << path.directory(path.depth() - 1) << ": Is a directory" << std::endl;
        return;
    }

    // Calculate hash
    Poco::FileInputStream stream(path.toString());
    Poco::SHA2Engine engine(Poco::SHA2Engine::SHA_256);
    Poco::DigestInputStream digest(engine, stream);
    Poco::NullOutputStream ostream;
    Poco::StreamCopier::copyStream(digest, ostream);

    if (!arg_tag)
    {
        const std::string output(Poco::cat(
            Poco::SHA2Engine::digestToHex(engine.digest()),
            std::string(arg_binary ? " *" : "  "),
            path.getFileName()));
        std::cout << output << std::endl;
    }
    else
    {
        std::cout << "SHA256 (" << path.getFileName() << ") = " << Poco::SHA2Engine::digestToHex(engine.digest()) << std::endl;
    }
}

void Application::ReadConfig()
{
    // Default Flags can be specified in JSON configuration
    Poco::AutoPtr<Poco::Util::LayeredConfiguration> configs(new Poco::Util::LayeredConfiguration);

    // configHome:
    // On Unix systems, this is the '~/.config/'. On Windows systems,
    // this is '%APPDATA%' (typically C:\Users\user\AppData\Roaming).

    Poco::Path iniPath;
    Poco::File iniFile;

    // e.g.
    // {
    //   "config": {
    //     "binary": 1
    //   }
    // }

    iniPath = Poco::Path::configHome();
    iniPath.append(Poco::cat(commandName(), std::string(".json")));
    iniFile = iniPath;
    if (iniFile.exists())
    {
        configs->add(new Poco::Util::JSONConfiguration(iniPath.toString()));
    }

    // e.g.
    // [config]
    // binary = 1

    iniPath = Poco::Path::configHome();
    iniPath.append(Poco::cat(commandName(), std::string(".ini")));
    iniFile = iniPath;
    if (iniFile.exists())
    {
        configs->add(new Poco::Util::IniFileConfiguration(iniPath.toString()));
    }

    try
    {
        arg_binary = configs->getBool("config.binary");
    }
    catch (const Poco::NotFoundException &e)
    {
    }
}

int Application::main(const std::vector<std::string> &arguments)
{
    if (arg_help || arguments.empty())
    {
        ShowHelpMessage();
        return EXIT_USAGE;
    }

    if (arg_check)
    {
        std::cerr << "check not yet implemented." << std::endl;
        return EXIT_FAILURE;
    }

    ReadConfig();

    for (const std::string &argument : arguments)
    {
        std::vector<std::string> files(ExpandFileArgument(argument));
        if (files.empty())
        {
            std::cout << commandName() << ": '" << argument << "': No such file or directory" << std::endl;
            continue;
        }

        for (const std::string &file : files)
        {
            if (!arg_check)
            {
                DisplayHash(file);
            }
            else
            {
            }
        }
    }

    return EXIT_OK;
}

POCO_APP_MAIN(Application)
