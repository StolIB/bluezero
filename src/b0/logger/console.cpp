#include <iostream>
#include <string>

#include <b0/node.h>
#include <b0/subscriber.h>
#include <b0/logger/logger.h>
#include <b0/message/log_entry.h>

namespace b0
{

namespace logger
{

class Console : public Node
{
public:
    Console()
        : Node("logger_console"),
          sub_(this, "log", &Console::onLogMessage, this),
          dummy_logger_(this)
    {
    }

    ~Console()
    {
    }

    std::string getName() const
    {
        return "console";
    }

    void onLogMessage(const b0::message::LogEntry &entry)
    {
        LocalLogger::LevelInfo info = dummy_logger_.levelInfo(entry.level);
        std::cout << info.ansiEscape() << "[" << entry.node_name << "] " << info.levelStr << ": " << entry.message << info.ansiReset() << std::endl;
    }

protected:
    b0::Subscriber sub_;

private:
    // A dummy logger to get formatting information from:
    b0::logger::LocalLogger dummy_logger_;
};

} // namespace logger

} // namespace b0

int main()
{
    b0::logger::Console console;
    console.init();
    console.spin();
    return 0;
}

