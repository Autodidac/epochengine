module; // REQUIRED global module fragment

// ============================================================
// Named module
// ============================================================

export module aengine.context.commandqueue;

// ------------------------------------------------------------
// Standard library
// ------------------------------------------------------------
import <functional>;
import <mutex>;
import <queue>;

// ============================================================
// Command queue
// ============================================================

export namespace almondnamespace::core
{
    struct CommandQueue
    {
        using RenderCommand = std::function<void()>;

        std::queue<RenderCommand>& get_queue()
        {
            return commands;
        }

        std::mutex& get_mutex()
        {
            return mutex;
        }

        void enqueue(RenderCommand cmd)
        {
            std::scoped_lock lock(mutex);
            commands.push(std::move(cmd));
        }

        void clear()
        {
            std::scoped_lock lock(mutex);
            std::queue<RenderCommand> empty;
            commands.swap(empty);
        }

        bool drain()
        {
            std::queue<RenderCommand> localCommands;
            {
                std::scoped_lock lock(mutex);
                if (commands.empty())
                    return false;
                localCommands.swap(commands);
            }

            while (!localCommands.empty()) {
                auto cmd = std::move(localCommands.front());
                localCommands.pop();
                if (cmd)
                    cmd();
            }
            return true;
        }

    private:
        std::mutex mutex;
        std::queue<RenderCommand> commands;
    };
}
