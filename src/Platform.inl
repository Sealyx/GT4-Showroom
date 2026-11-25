
namespace
{
    inline constexpr int platformModelId = 18117;
    inline constexpr uintptr_t platformClockHours = 0xB70153;
    inline constexpr uintptr_t platformPlayer = 0xB7CD98;
    inline constexpr uintptr_t fnRequestModel = 0x4087E0;
    inline constexpr uintptr_t fnLoadAllRequested = 0x40EA10;
    inline constexpr uintptr_t fnObjectCreate = 0x5A1F60;
    inline constexpr uintptr_t fnWorldAdd = 0x563220;
    inline constexpr uintptr_t fnWorldRemove = 0x563280;
    inline constexpr uintptr_t fnSetPosn = 0x420B80;
    inline constexpr uintptr_t fnSetHeading = 0x43E0C0;
    inline constexpr uintptr_t fnUpdateRwFrame = 0x532B00;
    inline constexpr uintptr_t fnUpdateRwMatrix = 0x446F90;
    inline constexpr uintptr_t fnGetColModel = 0x535300;
    inline constexpr uintptr_t fnGroundZ = 0x5696C0;
    inline constexpr uintptr_t fnColNew = 0x40FC30;
    inline constexpr uintptr_t fnColCtor = 0x40FB60;
    inline constexpr uintptr_t fnLoadCol3 = 0x537CE0;
    inline constexpr uintptr_t fnSetColModel = 0x4C4BC0;
    inline constexpr uintptr_t fnCentreToBase = 0x536BE0;

    inline int platformModelFromName(const std::string& name)
    {
        static const std::pair<const char*, int> known[]{
            {"elegy", 562}, {"stratum", 561}, {"flash", 565}, {"savanna", 567},
            {"jester", 559}, {"sultan", 560}, {"feltzer", 533},
        };
        for (const auto& item : known)
        {
            if (_stricmp(name.c_str(), item.first) == 0)
            {
                return item.second;
            }
        }
        return atoi(name.c_str());
    }

    inline std::vector<int> platformVehicleList(gt4::Gallery gallery)
    {
        std::vector<int> models;
        const auto section = gt4::galleries[int(gallery)].section;
        wchar_t value[512]{};
        GetPrivateProfileStringW(section, L"Platform", L"", value, 512, gt4::ini.c_str());
        std::wstringstream stream{std::wstring(value)};
        std::wstring token;
        while (std::getline(stream, token, L','))
        {
            const int id = platformModelFromName(std::string(token.begin(), token.end()));
            if (id > 0)
            {
                models.push_back(id);
            }
        }
        if (models.empty())
        {
            models = gallery == gt4::Gallery::Wang ? std::vector<int>{562, 561, 565, 567}
                                                   : std::vector<int>{559, 562, 560, 533};
            gt4::log("Platform: Platform= missing, using default vehicle list");
        }
        return models;
    }

    struct PlatformDisplay
    {
        gt4::Gallery gallery;
        gt4::V3 position;
        float heading;
        void* object = nullptr;
        void* vehicle = nullptr;
        void* carObject = nullptr;
        int vehicleModel = 0;
        bool active = false;
        bool grounded = false;
        float carLift = 0.f;
        float ground = 0.f;
        double applied = 0.0;
        double angle = 0;
        int objectHandle = -1;
        int vehicleHandle = -1;
    };

    inline PlatformDisplay platformDisplays[2]{
        {gt4::Gallery::Wang, {-1954.3f, 299.f, 41.f}, 0.f, nullptr, nullptr, nullptr, 0, false, false, 0.f, 0.f, 0.0, 0, -1, -1},
        {gt4::Gallery::Ottos, {-1656.f, 1213.f, 21.f}, 0.f, nullptr, nullptr, nullptr, 0, false, false, 0.f, 0.f, 0.0, 0, -1, -1},
    };

    inline void* platformModelInfo(int model)
    {
        uint32_t table = 0, count = 0;
        if (!::gta::read(0x40CD67, table) || !table)
        {
            return nullptr;
        }
        if (!::gta::read(0x40CD58, count) || count < 18118 || count > 1000000)
        {
            count = 20000;
        }
        if (model < 0 || uint32_t(model) >= count)
        {
            return nullptr;
        }
        void* info = nullptr;
        return ::gta::read(uintptr_t(table) + uintptr_t(model) * 4, info) ? info : nullptr;
    }

    inline bool platformGroundZ(const gt4::V3& point, float& out)
    {
        bool ok = false;
        const float found = reinterpret_cast<float(__cdecl*)(float, float, float, bool*, void**)>(fnGroundZ)(
            point.x, point.y, point.z + 2.f, &ok, nullptr);
        if (!ok)
        {
            return false;
        }
        out = found;
        return true;
    }

    // CEntity::IsEntityPointerValid
    inline bool platformEntityValid(void* entity)
    {
        return entity && reinterpret_cast<bool(__cdecl*)(void*)>(0x533310)(entity);
    }

    inline int platformObjectRef(void* object)
    {
        return object ? reinterpret_cast<int(__cdecl*)(void*)>(0x550020)(object) : -1;
    }

    inline void* platformResolveObject(int handle)
    {
        return handle >= 0 ? reinterpret_cast<void*(__cdecl*)(int)>(0x550050)(handle) : nullptr;
    }

    inline int platformVehicleRef(void* vehicle)
    {
        return vehicle ? reinterpret_cast<int(__cdecl*)(void*)>(0x54FFC0)(vehicle) : -1;
    }

    inline void* platformResolveVehicle(int handle)
    {
        return handle >= 0 ? reinterpret_cast<void*(__cdecl*)(int)>(0x54FFF0)(handle) : nullptr;
    }

    inline uint16_t platformEntityModel(void* entity)
    {
        return entity ? *reinterpret_cast<uint16_t*>(static_cast<uint8_t*>(entity) + 0x22) : uint16_t(0xFFFF);
    }

    inline const char* platformGalleryName(gt4::Gallery gallery)
    {
        return gallery == gt4::Gallery::Wang ? "WangCars" : "OttosAutos";
    }

    // Attaches the shipped COL3 to the model and disables back-face culling, so the
    // disc is solid to walk on and never vanishes as it turns.
    inline void platformPrepareModel()
    {
        static bool prepared = false;
        if (prepared)
        {
            return;
        }
        auto* info = static_cast<uint8_t*>(platformModelInfo(platformModelId));
        if (!info)
        {
            return;
        }
        prepared = true;

        // force back-face culling so the disc is not drawn double-sided
        // Make sure the disc behaves like ordinary geometry: no draw-last, no additive pass and
        // it must write depth, otherwise it is composited over whatever stands on it.
        auto* flags = reinterpret_cast<uint16_t*>(info + 0x12);
        *flags &= uint16_t(~(0x02 | 0x04 | 0x08));
        // keep backface culling on: a double-sided disc shows its dark underside through the top

        const auto file = std::filesystem::path(gt4::base) / L"GT4Showroom/platform/gt4_platform.col";
        std::ifstream stream(file, std::ios::binary);
        if (!stream)
        {
            gt4::log("Platform: COL file missing");
            return;
        }
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        if (data.size() < 40 || std::memcmp(data.data(), "COL3", 4) != 0)
        {
            gt4::log("Platform: COL header invalid");
            return;
        }
        if (*reinterpret_cast<void**>(info + 0x14))
        {
            gt4::log("Platform: collision already registered by Mod Loader");
            return;
        }
        auto* model = reinterpret_cast<void*(__cdecl*)(unsigned)>(fnColNew)(0x30);
        if (!model)
        {
            gt4::log("Platform: COL allocation failed");
            return;
        }
        reinterpret_cast<void(__thiscall*)(void*)>(fnColCtor)(model);
        reinterpret_cast<void(__cdecl*)(uint8_t*, unsigned, void*, const char*)>(fnLoadCol3)(
            data.data() + 32, unsigned(data.size() - 32), model, "gt4_platform");
        reinterpret_cast<void(__thiscall*)(void*, void*, bool)>(fnSetColModel)(info, model, true);
        gt4::log("Platform: collision attached");
    }

    inline bool platformModelReady()
    {
        static bool reported = false;
        const bool ready = platformModelInfo(platformModelId) != nullptr;
        if (!reported)
        {
            reported = true;
            uint32_t table = 0, count = 0;
            ::gta::read(0x40CD67, table);
            ::gta::read(0x40CD58, count);
            char detail[224];
            auto* byName = reinterpret_cast<void*(__cdecl*)(const char*, int*)>(0x4C5940)("gt4_platform", nullptr);
            std::snprintf(detail, sizeof(detail), "Platform: table=%p count=%u model400=%p model18117=%p byName=%p",
                          reinterpret_cast<void*>(uintptr_t(table)), count,
                          platformModelInfo(400), platformModelInfo(platformModelId), byName);
            gt4::log(detail);
        }
        return ready;
    }

    // Uses the game's own placement calls so the entity matrix *and* its RenderWare frame
    // stay in sync (a raw matrix write rotates the collision but not the model).
    inline void platformSetTransform(void* object, gt4::V3 position, float heading)
    {
        if (!object)
        {
            return;
        }
        reinterpret_cast<void(__thiscall*)(void*, float, float, float)>(fnSetPosn)(object, position.x, position.y, position.z);
        reinterpret_cast<void(__thiscall*)(void*, float)>(fnSetHeading)(object, heading);
        reinterpret_cast<void(__thiscall*)(void*)>(fnUpdateRwMatrix)(object);
        reinterpret_cast<void(__thiscall*)(void*)>(fnUpdateRwFrame)(object);
    }

    // Kinematic display vehicle: no gravity, no collision/turn/move forces and no applied
    // speed, so physics cannot fight the transform the disc applies every frame.
    inline void platformProtectVehicle(void* vehicle)
    {
        if (!vehicle)
        {
            return;
        }
        auto* body = static_cast<uint8_t*>(vehicle);
        auto* flags = reinterpret_cast<uint32_t*>(body + 0x40);
        *flags &= ~uint32_t(0x2);                                   // bApplyGravity off
        *flags |= uint32_t(0x4 | 0x10 | 0x20 | 0x2000);             // disable collision/turn/move force, dont apply speed
        // DOORLOCK_LOCKED (2); game::spawn writes 1 here so the player can drive the car away
        *reinterpret_cast<uint32_t*>(body + 0x4F8) = 2;
        auto* move = reinterpret_cast<float*>(body + 0x44);
        auto* turn = reinterpret_cast<float*>(body + 0x50);
        for (int i = 0; i < 3; ++i)
        {
            move[i] = 0.f;
            turn[i] = 0.f;
        }
    }

    inline void updatePlatformDisplays(double dt)
    {
        static bool notified = false;
        if (!platformModelReady())
        {
            if (!notified)
            {
                notified = true;
                gt4::log("Platform: model 18117 not registered (check Mod Loader IDE)");
            }
            return;
        }
        platformPrepareModel();

        const int hour = gt4::game::at<uint8_t>(platformClockHours);
        const bool rotating = hour >= 8 && hour < 23;
        void* playerPed = gt4::game::player();
        const gt4::V3 player = playerPed ? gt4::game::position(playerPed) : gt4::V3{0.f, 0.f, 0.f};

        for (auto& display : platformDisplays)
        {
            const char* name = platformGalleryName(display.gallery);
            const bool available = gt4::galleryStoryAvailable(display.gallery);
            const float dx = player.x - display.position.x, dy = player.y - display.position.y;
            const float distance = std::sqrt(dx * dx + dy * dy);

            // Resolve through the pools; the stored handle, not the raw pointer, is the
            // authority on whether the entity still exists.
            if (display.objectHandle >= 0)
            {
                void* resolved = platformResolveObject(display.objectHandle);
                if (!resolved)
                {
                    char detail[224];
                    std::snprintf(detail, sizeof(detail), "Platform[%s]: object handle %d no longer resolves",
                                  name, display.objectHandle);
                    gt4::log(detail);
                    display.object = nullptr;
                    display.objectHandle = -1;
                    display.active = false;
                    display.angle = 0.0;
                    display.applied = 0.0;
                }
                else if (platformEntityModel(resolved) != uint16_t(platformModelId))
                {
                    char detail[224];
                    std::snprintf(detail, sizeof(detail), "Platform[%s]: handle %d resolved to model %u, dropping",
                                  name, display.objectHandle, unsigned(platformEntityModel(resolved)));
                    gt4::log(detail);
                    display.object = nullptr;
                    display.objectHandle = -1;
                    display.active = false;
                }
                else
                {
                    display.object = resolved;
                }
            }
            else
            {
                display.object = nullptr;
            }
            if (display.vehicleHandle >= 0)
            {
                void* resolved = platformResolveVehicle(display.vehicleHandle);
                if (!resolved)
                {
                    display.vehicle = nullptr;
                    display.vehicleHandle = -1;
                    display.grounded = false;
                }
                else if (platformEntityModel(resolved) != uint16_t(display.vehicleModel))
                {
                    display.vehicle = nullptr;
                    display.vehicleHandle = -1;
                    display.grounded = false;
                }
                else
                {
                    display.vehicle = resolved;
                }
            }
            else
            {
                display.vehicle = nullptr;
            }

            if (!available || distance > 340.f)
            {
                if (display.object && display.active)
                {
                    reinterpret_cast<void(__cdecl*)(void*)>(fnWorldRemove)(display.object);
                    display.active = false;
                }
                continue;
            }

            if (!display.object)
            {
                if (distance > 300.f)
                {
                    continue;
                }
                float ground = 0.f;
                if (!platformGroundZ(display.position, ground))
                {
                    char detail[192];
                    std::snprintf(detail, sizeof(detail), "Platform[%s]: ground query failed at %.1f %.1f", name,
                                  display.position.x, display.position.y);
                    gt4::log(detail);
                    continue;
                }
                display.ground = ground;

                reinterpret_cast<void(__cdecl*)(int, int)>(fnRequestModel)(platformModelId, 0);
                reinterpret_cast<void(__cdecl*)(bool)>(fnLoadAllRequested)(false);
                auto* info = static_cast<uint8_t*>(platformModelInfo(platformModelId));
                if (!info || !*reinterpret_cast<void**>(info + 0x1C))
                {
                    continue;
                }

                gt4::V3 place = display.position;
                place.z = ground + 0.16f;
                display.object = reinterpret_cast<void*(__cdecl*)(int, bool)>(fnObjectCreate)(platformModelId, true);
                if (display.object)
                {
                    display.objectHandle = platformObjectRef(display.object);
                    char audit[256];
                    std::snprintf(audit, sizeof(audit), "Platform[%s]: created model=%u ptr=%p handle=%d",
                                  name, unsigned(platformEntityModel(display.object)), display.object, display.objectHandle);
                    gt4::log(audit);
                }
                if (!display.object)
                {
                    char detail[192];
                    std::snprintf(detail, sizeof(detail), "Platform[%s]: object creation failed", name);
                    gt4::log(detail);
                    continue;
                }

                // position and frame are final before the entity enters the world
                platformSetTransform(display.object, place, display.heading + float(display.angle));
                reinterpret_cast<void(__cdecl*)(void*)>(fnWorldAdd)(display.object);
                display.active = true;
                char detail[256];
                std::snprintf(detail, sizeof(detail),
                              "Platform[%s]: created at %.1f %.1f %.1f (requested %.1f %.1f %.1f, ground %.2f)", name,
                              place.x, place.y, place.z, display.position.x, display.position.y, display.position.z,
                              ground);
                gt4::log(detail);
            }

            if (!display.object)
            {
                continue;
            }
            if (!display.active)
            {
                // re-entering range: only put the entity back if the game still owns it
                if (!platformEntityValid(display.object))
                {
                    display.object = nullptr;
                    continue;
                }
                if (distance <= 300.f)
                {
                    reinterpret_cast<void(__cdecl*)(void*)>(fnWorldAdd)(display.object);
                    display.active = true;
                }
                continue;
            }

            if (rotating)
            {
                display.angle += 5.f * 3.14159265f / 180.f * float(dt);
            }
            const bool turning = rotating && (display.angle - display.applied) > 0.0087;

            gt4::V3 place = display.position;
            place.z = display.ground + 0.16f;
            if (turning)
            {
                reinterpret_cast<void(__cdecl*)(void*)>(fnWorldRemove)(display.object);
            }
            platformSetTransform(display.object, place, display.heading + float(display.angle));
            if (turning)
            {
                reinterpret_cast<void(__cdecl*)(void*)>(fnWorldAdd)(display.object);
                display.applied = display.angle;
            }

            if (display.vehicleModel <= 0)
            {
                const auto models = platformVehicleList(display.gallery);
                display.vehicleModel = models.empty() ? 0 : models.front();
            }
            if (!display.vehicle && display.vehicleModel > 0)
            {
                gt4::Entry entry;
                entry.id = display.vehicleModel;
                std::string error;
                display.vehicle = gt4::game::spawn(entry, 0, error, 2.f, false, &display.position, display.heading);
                display.grounded = display.vehicle != nullptr;
                if (display.vehicle)
                {
                    display.vehicleHandle = platformVehicleRef(display.vehicle);
                    display.carLift = reinterpret_cast<float(__thiscall*)(void*)>(fnCentreToBase)(display.vehicle);
                }
                char detail[224];
                std::snprintf(detail, sizeof(detail), "Platform[%s]: vehicle=%p model=%d %s", name,
                              display.vehicle, display.vehicleModel, error.c_str());
                gt4::log(detail);
            }
            if (display.vehicle)
            {
                gt4::V3 carPlace = place;
                carPlace.z = display.ground + 0.32f + display.carLift;
                platformSetTransform(display.vehicle, carPlace, display.heading + float(display.angle));
                platformProtectVehicle(display.vehicle);
            }
        }
    }
}
