#pragma once

namespace Iridium
{
    class BinaryTransform
    {
    public:
        const u8* NextIn {nullptr};
        usize AvailIn {0};

        u8* NextOut {nullptr};
        usize AvailOut {0};

        bool Finished {false};

        virtual ~BinaryTransform() = default;

        virtual bool Reset() = 0;
        virtual bool Update() = 0;
    };
} // namespace Iridium
