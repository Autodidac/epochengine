module; // REQUIRED global module fragment

// ============================================================
// Named module
// ============================================================

export module aengine.context.commandqueue;

// ------------------------------------------------------------
// Standard library
// ------------------------------------------------------------
import <cstddef>;
import <functional>;
import <mutex>;
import <queue>;
import <utility>;

// ============================================================
// Command queue (thread-safe, no raw mutex access)
// ============================================================

export namespace almondnamespace::core
{
    struct CommandQueue
    {
        using RenderCommand = std::function<void()>;

        // Push a command (thread-safe)
        void enqueue(RenderCommand cmd)
        {
            if (!cmd) return;
            std::lock_guard<std::mutex> lock(mutex_);
            commands_.push(std::move(cmd));
        }

        // Remove all queued commands (thread-safe)
        void clear() noexcept
        {
            std::lock_guard<std::mutex> lock(mutex_);
            std::queue<RenderCommand> empty;
            commands_.swap(empty);
        }

        // Execute all queued commands. Returns true if anything ran.
        bool drain()
        {
            std::queue<RenderCommand> local;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (commands_.empty())
                    return false;
                local.swap(commands_);
            }

            while (!local.empty())
            {
                auto cmd = std::move(local.front());
                local.pop();
                cmd(); // cmd is guaranteed non-empty from enqueue(), but safe anyway.
            }
            return true;
        }

        // Depth snapshot for telemetry (thread-safe)
        [[nodiscard]] std::size_t depth() const noexcept
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return commands_.size();
        }

        // Optional: run at most one command (useful for budgeted pumping)
        bool try_run_one()
        {
            RenderCommand cmd;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (commands_.empty())
                    return false;
                cmd = std::move(commands_.front());
                commands_.pop();
            }
            if (cmd) cmd();
            return true;
        }

    private:
        mutable std::mutex mutex_;
        std::queue<RenderCommand> commands_;
    };
}
