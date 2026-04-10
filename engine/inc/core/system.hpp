#pragma once

namespace Apex
{
    class System
    {
    public:
        virtual ~System() = default;
        System(const System&) = delete;
        System& operator=(const System&) = delete;
        System(System&&) = delete;
        System& operator=(System&&) = delete;

        virtual void Init() {}
        virtual void Update(float) {}
        virtual void FixedUpdate(float) {}
        virtual void BeginFrame() {}
        virtual void EndFrame() {}
        virtual void Shutdown() {}

        template <typename T, typename... Args>
        static Ref<T> CreateSystem(Args&&... args)
        {
            return std::make_shared<T>(std::forward<Args>(args)...);
        }

        bool bCanBePaused = true;

    protected:
        friend class EngineClass;
        System() = default;
    };
}  // namespace Apex