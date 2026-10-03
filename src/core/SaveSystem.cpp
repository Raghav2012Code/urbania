#include "core/SaveSystem.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/Camera.h"
#include "simulation/BusRoute.h"
#include "simulation/BusStop.h"
#include "simulation/Citizen.h"
#include "simulation/CitizenManager.h"
#include "simulation/Economy.h"
#include "simulation/Employment.h"
#include "simulation/Pollution.h"
#include "simulation/Population.h"
#include "simulation/Simulation.h"
#include "simulation/SimulationClock.h"
#include "simulation/Transit.h"
#include "simulation/Utilities.h"
#include "world/Tile.h"
#include "world/World.h"

namespace urbania {
namespace {

constexpr int MAX_CITIZENS = 100000;
constexpr int MAX_PATH_LEN = 20000;
constexpr int MAX_POLLUTION_ENTRIES = 20000;
constexpr int MAX_STOPS = 10000;

char encodeTile(TileType type)
{
    switch (type)
    {
    case TileType::Grass: return 'G';
    case TileType::Road: return 'R';
    case TileType::Residential: return 'H';
    case TileType::Commercial: return 'C';
    case TileType::Industrial: return 'I';
    case TileType::Park: return 'P';
    }
    return 'G';
}

bool decodeTile(char c, TileType& out)
{
    switch (c)
    {
    case 'G': out = TileType::Grass; return true;
    case 'R': out = TileType::Road; return true;
    case 'H': out = TileType::Residential; return true;
    case 'C': out = TileType::Commercial; return true;
    case 'I': out = TileType::Industrial; return true;
    case 'P': out = TileType::Park; return true;
    default: return false;
    }
}

bool parseInt(const std::string& s, int& out)
{
    try
    {
        size_t pos = 0;
        const long v = std::stol(s, &pos);
        if (pos != s.size() || v < std::numeric_limits<int>::min() ||
            v > std::numeric_limits<int>::max())
        {
            return false;
        }
        out = static_cast<int>(v);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool parseFloat(const std::string& s, float& out)
{
    try
    {
        size_t pos = 0;
        const double v = std::stod(s, &pos);
        if (pos != s.size() || !std::isfinite(v) || v < -1.0e12 || v > 1.0e12)
        {
            return false;
        }
        out = static_cast<float>(v);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool inBounds(int x, int y, int w, int h)
{
    return x >= 0 && y >= 0 && x < w && y < h;
}

// Parsed file content. Only applied to the live city after full validation.
struct SaveData {
    int width = 0;
    int height = 0;
    std::vector<std::string> tileRows;
    float clockTime = 0.0f;
    float clockScale = 1.0f;
    bool clockPaused = false;
    int money = 0;
    float totalTax = 0.0f;
    float totalMaint = 0.0f;
    float economySeconds = 0.0f;
    float simElapsed = 0.0f;
    float hourLandValue = 0.0f;
    float hourHousing = 0.0f;
    float hourHappiness = 0.0f;
    float hourDemand = 0.0f;
    int utilElec = 100;
    int utilWater = 100;
    int utilSewage = 100;
    std::vector<Citizen> citizens;
    int nextCitizenId = 1;
    std::vector<std::tuple<int, int, float>> growth;  // x, y, progress
    std::map<TileCoordinate, float> pollution;
    float pollutionSeconds = 0.0f;
    float pollutionAvg = 0.0f;
    float pollutionMax = 0.0f;
    std::vector<BusStop> stops;
    int nextStopId = 1;
    std::vector<BusRoute> routes;
    int nextRouteId = 1;
    int nextBusId = 1;
    float camX = 0.0f;
    float camY = 0.0f;
    float camZoom = 1.0f;
};

class LineReader {
public:
    explicit LineReader(std::istream& in) : in_(in) {}
    bool next(std::string& line, std::string& err)
    {
        if (!std::getline(in_, line))
        {
            err = "unexpected end of file";
            return false;
        }
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        ++lineNo_;
        return true;
    }
    int lineNo() const { return lineNo_; }

private:
    std::istream& in_;
    int lineNo_ = 0;
};

bool expectTokens(std::istringstream& ss, const std::string& tag, std::vector<std::string>& out,
                  size_t count, std::string& err)
{
    std::string head;
    ss >> head;
    if (head != tag)
    {
        err = "expected " + tag;
        return false;
    }
    out.clear();
    std::string tok;
    while (ss >> tok)
    {
        out.push_back(tok);
    }
    if (out.size() != count)
    {
        err = tag + ": expected " + std::to_string(count) + " values";
        return false;
    }
    return true;
}

}  // namespace

#ifdef URBANIA_ENABLE_LOAD_FAILPOINT
namespace {

bool g_loadFailpointArmed = false;

bool consumeLoadFailpoint()
{
    const bool armed = g_loadFailpointArmed;
    g_loadFailpointArmed = false;
    return armed;
}

}  // namespace

void SaveSystem::setLoadFailpointForTests(bool armed)
{
    g_loadFailpointArmed = armed;
}
#endif

SaveSystem::Result SaveSystem::save(const World& world, const Simulation& simulation,
                                    const SimulationClock& clock, const Camera& camera,
                                    const std::string& path)
{
    Result result;
    try
    {
        const std::filesystem::path p(path);
        if (p.has_parent_path())
        {
            std::error_code ec;
            std::filesystem::create_directories(p.parent_path(), ec);
        }

        const std::string tmpPath = path + ".tmp";
        std::ofstream out(tmpPath, std::ios::trunc);
        if (!out)
        {
            result.message = "cannot open file for writing";
            return result;
        }
        out << std::setprecision(9);

        const int w = world.getWidth();
        const int h = world.getHeight();
        const Population& pop = simulation.getPopulation();
        const CitizenManager& citizens = pop.getCitizens();
        const Economy& economy = simulation.getEconomy();
        const Transit& transit = simulation.getTransit();
        const Pollution& pollution = simulation.getPollution();
        const Utilities& utilities = simulation.getUtilities();

        out << "URBANIA_SAVE " << SAVE_VERSION << "\n";
        out << "WORLD " << w << " " << h << "\n";
        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                out << encodeTile(world.getTile(x, y).type);
            }
            out << "\n";
        }

        out << "CLOCK " << clock.getSimulationTime() << " " << clock.getTimeScale() << " "
            << (clock.isPaused() ? 1 : 0) << "\n";
        out << "ECONOMY " << economy.getMoney() << " " << economy.getTotalTaxCollected() << " "
            << economy.getTotalMaintenancePaid() << " " << economy.getSecondsTowardNextDay() << "\n";
        out << "SIM " << simulation.getElapsedSimulationSeconds() << "\n";
        out << "HOURCLOCKS " << simulation.getLandValue().getSecondsTowardNextHour() << " "
            << simulation.getHousing().getSecondsTowardNextHour() << " "
            << simulation.getHappiness().getSecondsTowardNextHour() << " "
            << simulation.getDemand().getSecondsTowardNextHour() << "\n";
        out << "UTILCAPS " << utilities.getElectricityCapacity() << " "
            << utilities.getWaterCapacity() << " " << utilities.getSewageCapacity() << "\n";

        out << "CITIZENS " << citizens.getCitizenCount() << " " << citizens.getNextId() << "\n";
        for (const Citizen& c : citizens.getCitizens())
        {
            out << "CIT " << c.id << " " << c.home.x << " " << c.home.y << " "
                << (c.home.valid ? 1 : 0) << " " << c.workplace.x << " " << c.workplace.y << " "
                << (c.workplace.valid ? 1 : 0) << " " << c.currentTile.x << " " << c.currentTile.y
                << " " << (c.currentTile.valid ? 1 : 0) << " " << c.pathIndex << " "
                << c.movementProgress << " " << c.income << " " << c.happiness << " "
                << (c.employed ? 1 : 0) << " " << (c.commutingToWork ? 1 : 0) << "\n";
            out << "CPATH " << c.commutePath.size() << "\n";
            for (const TileCoordinate& t : c.commutePath)
            {
                out << t.x << " " << t.y << " " << (t.valid ? 1 : 0) << "\n";
            }
        }

        // Growth progress for residential tiles only.
        std::vector<std::tuple<int, int, float>> growthRows;
        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                if (world.getTile(x, y).type == TileType::Residential)
                {
                    growthRows.emplace_back(x, y, pop.getGrowthProgress(x, y));
                }
            }
        }
        out << "POPGROWTH " << growthRows.size() << "\n";
        for (const auto& g : growthRows)
        {
            out << "PG " << std::get<0>(g) << " " << std::get<1>(g) << " " << std::get<2>(g)
                << "\n";
        }

        const auto& polGrid = pollution.getPollutionGrid();
        out << "POLLUTION " << polGrid.size() << " " << pollution.getSecondsTowardNextHour()
            << " " << pollution.getAveragePollution() << " " << pollution.getMaxPollution()
            << "\n";
        for (const auto& entry : polGrid)
        {
            out << "POL " << entry.first.x << " " << entry.first.y << " " << entry.second
                << "\n";
        }

        out << "STOPS " << transit.getBusStopCount() << " " << transit.getNextStopId() << "\n";
        for (const BusStop& s : transit.getBusStops())
        {
            out << "STOP " << s.id << " " << s.tile.x << " " << s.tile.y << " "
                << (s.tile.valid ? 1 : 0) << "\n";
        }

        out << "ROUTES " << transit.getRouteCount() << " " << transit.getNextRouteId() << " "
            << transit.getNextBusId() << "\n";
        for (const BusRoute& r : transit.getRoutes())
        {
            out << "ROUTE " << r.id << " " << r.stopIds.size();
            for (int sid : r.stopIds)
            {
                out << " " << sid;
            }
            out << "\n";
        }

        const Vector2 target = camera.getTarget();
        out << "CAMERA " << target.x << " " << target.y << " " << camera.getZoom() << "\n";
        out << "END\n";
        out.flush();
        if (!out)
        {
            result.message = "write failed";
            return result;
        }
        out.close();

        std::error_code ec;
        std::filesystem::rename(tmpPath, p, ec);
        if (ec)
        {
            result.message = "atomic rename failed";
            return result;
        }
        result.ok = true;
        result.message = "City saved";
        return result;
    }
    catch (const std::exception& e)
    {
        result.message = std::string("save failed: ") + e.what();
        return result;
    }
}

SaveSystem::Result SaveSystem::load(World& world, Simulation& simulation, SimulationClock& clock,
                                    Camera& camera, const std::string& path)
{
    Result result;
    SaveData data;

    // ---- Phase 1: parse + validate into memory (live city untouched). ----
    try
    {
        std::ifstream in(path);
        if (!in)
        {
            result.message = "save file not found";
            return result;
        }
        LineReader reader(in);
        std::string line, err;
        std::vector<std::string> toks;
        std::istringstream ss;

        // Header
        if (!reader.next(line, err))
        {
            result.message = "empty save file";
            return result;
        }
        ss.clear();
        ss.str(line);
        if (!expectTokens(ss, "URBANIA_SAVE", toks, 1, err))
        {
            result.message = "not a save file";
            return result;
        }
        int version = 0;
        if (!parseInt(toks[0], version) || version < 1 || version > SAVE_VERSION)
        {
            result.message = "unsupported version (got " + toks[0] + ")";
            return result;
        }

        // World dims
        if (!reader.next(line, err) || (ss.clear(), ss.str(line), !expectTokens(ss, "WORLD", toks, 2, err)))
        {
            result.message = "missing WORLD";
            return result;
        }
        if (!parseInt(toks[0], data.width) || !parseInt(toks[1], data.height) ||
            data.width != world.getWidth() || data.height != world.getHeight())
        {
            result.message = "unsupported world dimensions";
            return result;
        }
        const int w = data.width;
        const int h = data.height;
        data.tileRows.reserve(h);
        std::vector<std::vector<TileType>> tiles(h, std::vector<TileType>(w, TileType::Grass));
        for (int y = 0; y < h; ++y)
        {
            if (!reader.next(line, err))
            {
                result.message = "truncated tile data";
                return result;
            }
            if (static_cast<int>(line.size()) != w)
            {
                result.message = "bad tile row " + std::to_string(y);
                return result;
            }
            for (int x = 0; x < w; ++x)
            {
                if (!decodeTile(line[x], tiles[y][x]))
                {
                    result.message = "bad tile at row " + std::to_string(y);
                    return result;
                }
            }
            data.tileRows.push_back(line);
        }
        auto tileAt = [&](int x, int y) -> TileType { return tiles[y][x]; };

        // Clock
        if (!reader.next(line, err) || (ss.clear(), ss.str(line), !expectTokens(ss, "CLOCK", toks, 3, err)))
        {
            result.message = "missing CLOCK";
            return result;
        }
        int pausedInt = 0;
        if (!parseFloat(toks[0], data.clockTime) || !parseFloat(toks[1], data.clockScale) ||
            !parseInt(toks[2], pausedInt) || (pausedInt != 0 && pausedInt != 1) ||
            data.clockTime < 0.0f || !SimulationClock::isSupportedSpeed(data.clockScale))
        {
            result.message = "bad CLOCK values";
            return result;
        }
        data.clockPaused = pausedInt == 1;

        // Economy
        if (!reader.next(line, err) || (ss.clear(), ss.str(line), !expectTokens(ss, "ECONOMY", toks, 4, err)))
        {
            result.message = "missing ECONOMY";
            return result;
        }
        if (!parseInt(toks[0], data.money) || !parseFloat(toks[1], data.totalTax) ||
            !parseFloat(toks[2], data.totalMaint) || !parseFloat(toks[3], data.economySeconds) ||
            data.money < 0 || data.totalTax < 0.0f || data.totalMaint < 0.0f ||
            data.economySeconds < 0.0f)
        {
            result.message = "bad ECONOMY values";
            return result;
        }

        // Sim elapsed
        if (!reader.next(line, err) || (ss.clear(), ss.str(line), !expectTokens(ss, "SIM", toks, 1, err)))
        {
            result.message = "missing SIM";
            return result;
        }
        if (!parseFloat(toks[0], data.simElapsed) || data.simElapsed < 0.0f)
        {
            result.message = "bad SIM value";
            return result;
        }

        // v2+ hourly accumulators. v1 files simply omit the record, leaving
        // these at zero.
        if (version >= 2)
        {
            if (!reader.next(line, err) ||
                (ss.clear(), ss.str(line), !expectTokens(ss, "HOURCLOCKS", toks, 4, err)))
            {
                result.message = "missing HOURCLOCKS";
                return result;
            }
            const float hourLimit = LandValue::SIM_SECONDS_PER_HOUR;
            if (!parseFloat(toks[0], data.hourLandValue) ||
                !parseFloat(toks[1], data.hourHousing) ||
                !parseFloat(toks[2], data.hourHappiness) ||
                !parseFloat(toks[3], data.hourDemand) || data.hourLandValue < 0.0f ||
                data.hourLandValue >= hourLimit || data.hourHousing < 0.0f ||
                data.hourHousing >= hourLimit || data.hourHappiness < 0.0f ||
                data.hourHappiness >= hourLimit || data.hourDemand < 0.0f ||
                data.hourDemand >= hourLimit)
            {
                result.message = "bad HOURCLOCKS values";
                return result;
            }
        }

        // Utility caps
        if (!reader.next(line, err) || (ss.clear(), ss.str(line), !expectTokens(ss, "UTILCAPS", toks, 3, err)))
        {
            result.message = "missing UTILCAPS";
            return result;
        }
        if (!parseInt(toks[0], data.utilElec) || !parseInt(toks[1], data.utilWater) ||
            !parseInt(toks[2], data.utilSewage) || data.utilElec < 0 || data.utilWater < 0 ||
            data.utilSewage < 0 || data.utilElec > 1000000 || data.utilWater > 1000000 ||
            data.utilSewage > 1000000)
        {
            result.message = "bad UTILCAPS values";
            return result;
        }

        // Citizens
        if (!reader.next(line, err) || (ss.clear(), ss.str(line), !expectTokens(ss, "CITIZENS", toks, 2, err)))
        {
            result.message = "missing CITIZENS";
            return result;
        }
        int citizenCount = 0;
        if (!parseInt(toks[0], citizenCount) || !parseInt(toks[1], data.nextCitizenId) ||
            citizenCount < 0 || citizenCount > MAX_CITIZENS || data.nextCitizenId < 1)
        {
            result.message = "bad CITIZENS header";
            return result;
        }
        data.citizens.reserve(citizenCount);
        std::unordered_set<int> citizenIds;
        for (int i = 0; i < citizenCount; ++i)
        {
            if (!reader.next(line, err) || (ss.clear(), ss.str(line), !expectTokens(ss, "CIT", toks, 16, err)))
            {
                result.message = "truncated CITIZENS";
                return result;
            }
            // CIT layout (16 tokens): id hx hy hv wx wy wv cx cy cv pathIdx
            //   progress income happiness employed commuting
            Citizen c;
            int id, hx, hy, hv, wx, wy, wv, cx, cy, cv, pathIdx;
            float progress, income, happiness;
            int employed, commuting;
            if (!parseInt(toks[0], id) || !parseInt(toks[1], hx) || !parseInt(toks[2], hy) ||
                !parseInt(toks[3], hv) || !parseInt(toks[4], wx) || !parseInt(toks[5], wy) ||
                !parseInt(toks[6], wv) || !parseInt(toks[7], cx) || !parseInt(toks[8], cy) ||
                !parseInt(toks[9], cv) || !parseInt(toks[10], pathIdx) ||
                !parseFloat(toks[11], progress) || !parseFloat(toks[12], income) ||
                !parseFloat(toks[13], happiness) || !parseInt(toks[14], employed) ||
                !parseInt(toks[15], commuting))
            {
                result.message = "bad CIT record";
                return result;
            }
            if (id <= 0 || !citizenIds.insert(id).second || hv < 0 || hv > 1 || wv < 0 ||
                wv > 1 || cv < 0 || cv > 1 || employed < 0 || employed > 1 || commuting < 0 ||
                commuting > 1 || pathIdx < 0 || pathIdx > MAX_PATH_LEN || progress < -0.001f ||
                progress > 1.001f || happiness < -0.001f || happiness > 100.001f)
            {
                result.message = "bad CIT record";
                return result;
            }
            const bool homeV = hv == 1;
            const bool workV = wv == 1;
            const bool curV = cv == 1;
            if (!homeV || !inBounds(hx, hy, w, h) || tileAt(hx, hy) != TileType::Residential)
            {
                result.message = "CIT references invalid home";
                return result;
            }
            if (workV)
            {
                if (!inBounds(wx, wy, w, h) ||
                    (tileAt(wx, wy) != TileType::Commercial &&
                     tileAt(wx, wy) != TileType::Industrial))
                {
                    result.message = "CIT references invalid workplace";
                    return result;
                }
            }
            if ((employed == 1) != workV)
            {
                result.message = "CIT employment/workplace mismatch";
                return result;
            }
            if (curV && !inBounds(cx, cy, w, h))
            {
                result.message = "CIT references invalid tile";
                return result;
            }
            c.id = id;
            c.home = homeV ? TileCoordinate::validCoord(hx, hy) : TileCoordinate{};
            c.workplace = workV ? TileCoordinate::validCoord(wx, wy) : TileCoordinate{};
            c.currentTile = curV ? TileCoordinate::validCoord(cx, cy) : TileCoordinate{};
            c.pathIndex = pathIdx;
            c.movementProgress = std::clamp(progress, 0.0f, 1.0f);
            c.income = income;
            c.happiness = std::clamp(happiness, 0.0f, 100.0f);
            c.employed = employed == 1;
            c.commutingToWork = commuting == 1;

            if (!reader.next(line, err) || (ss.clear(), ss.str(line), !expectTokens(ss, "CPATH", toks, 1, err)))
            {
                result.message = "missing CPATH";
                return result;
            }
            int pathLen = 0;
            if (!parseInt(toks[0], pathLen) || pathLen < 0 || pathLen > MAX_PATH_LEN)
            {
                result.message = "bad CPATH length";
                return result;
            }
            c.commutePath.reserve(pathLen);
            for (int k = 0; k < pathLen; ++k)
            {
                if (!reader.next(line, err))
                {
                    result.message = "truncated commute path";
                    return result;
                }
                std::istringstream ps(line);
                std::string a, b, d;
                int px, py, pv;
                if (!(ps >> a >> b >> d) || !parseInt(a, px) || !parseInt(b, py) ||
                    !parseInt(d, pv) || pv < 0 || pv > 1 || !inBounds(px, py, w, h))
                {
                    result.message = "bad commute path node";
                    return result;
                }
                c.commutePath.push_back(pv == 1 ? TileCoordinate::validCoord(px, py)
                                                : TileCoordinate{});
            }
            if (c.pathIndex > static_cast<int>(c.commutePath.size()))
            {
                result.message = "CIT path index out of range";
                return result;
            }
            data.citizens.push_back(std::move(c));
        }

        // Population growth
        if (!reader.next(line, err) || (ss.clear(), ss.str(line), !expectTokens(ss, "POPGROWTH", toks, 1, err)))
        {
            result.message = "missing POPGROWTH";
            return result;
        }
        int growthCount = 0;
        if (!parseInt(toks[0], growthCount) || growthCount < 0 || growthCount > w * h)
        {
            result.message = "bad POPGROWTH header";
            return result;
        }
        for (int i = 0; i < growthCount; ++i)
        {
            if (!reader.next(line, err) || (ss.clear(), ss.str(line), !expectTokens(ss, "PG", toks, 3, err)))
            {
                result.message = "truncated POPGROWTH";
                return result;
            }
            int gx, gy;
            float gp;
            if (!parseInt(toks[0], gx) || !parseInt(toks[1], gy) || !parseFloat(toks[2], gp) ||
                !inBounds(gx, gy, w, h) || tileAt(gx, gy) != TileType::Residential || gp < 0.0f ||
                gp > 1.0f)
            {
                result.message = "bad PG record";
                return result;
            }
            data.growth.emplace_back(gx, gy, gp);
        }

        // Pollution
        if (!reader.next(line, err) || (ss.clear(), ss.str(line), !expectTokens(ss, "POLLUTION", toks, 4, err)))
        {
            result.message = "missing POLLUTION";
            return result;
        }
        int polCount = 0;
        if (!parseInt(toks[0], polCount) || !parseFloat(toks[1], data.pollutionSeconds) ||
            !parseFloat(toks[2], data.pollutionAvg) || !parseFloat(toks[3], data.pollutionMax) ||
            polCount < 0 || polCount > MAX_POLLUTION_ENTRIES || data.pollutionSeconds < 0.0f ||
            data.pollutionAvg < 0.0f || data.pollutionMax < 0.0f)
        {
            result.message = "bad POLLUTION header";
            return result;
        }
        for (int i = 0; i < polCount; ++i)
        {
            if (!reader.next(line, err) || (ss.clear(), ss.str(line), !expectTokens(ss, "POL", toks, 3, err)))
            {
                result.message = "truncated POLLUTION";
                return result;
            }
            int px, py;
            float pv;
            if (!parseInt(toks[0], px) || !parseInt(toks[1], py) || !parseFloat(toks[2], pv) ||
                !inBounds(px, py, w, h) || pv < 0.0f || pv > 100.0f)
            {
                result.message = "bad POL record";
                return result;
            }
            data.pollution[TileCoordinate::validCoord(px, py)] = pv;
        }

        // Transit stops
        if (!reader.next(line, err) || (ss.clear(), ss.str(line), !expectTokens(ss, "STOPS", toks, 2, err)))
        {
            result.message = "missing STOPS";
            return result;
        }
        int stopCount = 0;
        if (!parseInt(toks[0], stopCount) || !parseInt(toks[1], data.nextStopId) ||
            stopCount < 0 || stopCount > MAX_STOPS || data.nextStopId < 1)
        {
            result.message = "bad STOPS header";
            return result;
        }
        std::unordered_set<int> stopIds;
        std::unordered_set<int> stopTiles;
        for (int i = 0; i < stopCount; ++i)
        {
            if (!reader.next(line, err) || (ss.clear(), ss.str(line), !expectTokens(ss, "STOP", toks, 4, err)))
            {
                result.message = "truncated STOPS";
                return result;
            }
            int sid, sx, sy, sv;
            if (!parseInt(toks[0], sid) || !parseInt(toks[1], sx) || !parseInt(toks[2], sy) ||
                !parseInt(toks[3], sv) || sid <= 0 || !stopIds.insert(sid).second || sv != 1 ||
                !inBounds(sx, sy, w, h) || tileAt(sx, sy) != TileType::Road ||
                !stopTiles.insert(sy * w + sx).second)
            {
                result.message = "bad STOP record";
                return result;
            }
            data.stops.push_back({ sid, TileCoordinate::validCoord(sx, sy) });
        }

        // Transit routes
        if (!reader.next(line, err) || (ss.clear(), ss.str(line), !expectTokens(ss, "ROUTES", toks, 3, err)))
        {
            result.message = "missing ROUTES";
            return result;
        }
        int routeCount = 0;
        if (!parseInt(toks[0], routeCount) || !parseInt(toks[1], data.nextRouteId) ||
            !parseInt(toks[2], data.nextBusId) || routeCount < 0 ||
            static_cast<size_t>(routeCount) > Transit::MAX_ROUTES ||
            data.nextRouteId < 1 || data.nextBusId < 1)
        {
            result.message = "bad ROUTES header";
            return result;
        }
        std::unordered_set<int> routeIds;
        for (int i = 0; i < routeCount; ++i)
        {
            if (!reader.next(line, err))
            {
                result.message = "truncated ROUTES";
                return result;
            }
            std::istringstream rs(line);
            std::string head;
            rs >> head;
            if (head != "ROUTE")
            {
                result.message = "expected ROUTE";
                return result;
            }
            int rid, n;
            if (!(rs >> rid >> n) || rid <= 0 || !routeIds.insert(rid).second || n < 2 ||
                static_cast<size_t>(n) > Transit::MAX_STOPS_PER_ROUTE)
            {
                result.message = "bad ROUTE header";
                return result;
            }
            BusRoute route;
            route.id = rid;
            route.stopIds.reserve(n);
            std::unordered_set<int> seenInRoute;
            for (int k = 0; k < n; ++k)
            {
                int sid;
                if (!(rs >> sid) || stopIds.find(sid) == stopIds.end())
                {
                    result.message = "ROUTE references unknown stop";
                    return result;
                }
                // Mirror Transit duplicate rule: repeats only allowed as a
                // first/last loop closure on routes with 3+ stops.
                if (!seenInRoute.insert(sid).second &&
                    !(k == n - 1 && sid == route.stopIds.front() && n >= 3))
                {
                    result.message = "ROUTE has duplicate stops";
                    return result;
                }
                route.stopIds.push_back(sid);
            }
            std::string extra;
            if (rs >> extra)
            {
                result.message = "bad ROUTE record";
                return result;
            }
            data.routes.push_back(std::move(route));
        }

        // Camera
        if (!reader.next(line, err) || (ss.clear(), ss.str(line), !expectTokens(ss, "CAMERA", toks, 3, err)))
        {
            result.message = "missing CAMERA";
            return result;
        }
        if (!parseFloat(toks[0], data.camX) || !parseFloat(toks[1], data.camY) ||
            !parseFloat(toks[2], data.camZoom))
        {
            result.message = "bad CAMERA values";
            return result;
        }

        // End marker
        if (!reader.next(line, err))
        {
            result.message = "missing END";
            return result;
        }
        std::istringstream es(line);
        std::string endTok;
        es >> endTok;
        if (endTok != "END")
        {
            result.message = "missing END";
            return result;
        }
    }
    catch (const std::exception& e)
    {
        result.message = std::string("parse failed: ") + e.what();
        return result;
    }

    // ---- Phase 2: apply to the live city (all data already validated). ----
    // Strong exception guarantee: snapshot the mutable live state before the
    // first mutation and restore it (via noexcept moves) if any step throws.
    // This upholds the transactional contract in SaveSystem.h even for an
    // allocation failure mid-apply.
    Simulation simulationSnapshot;
    std::vector<Tile> tileSnapshot;
    urbania::Camera cameraSnapshot;
    bool haveSnapshot = false;
    try
    {
        simulationSnapshot = simulation;
        tileSnapshot = world.snapshotTiles();
        cameraSnapshot = camera;
        haveSnapshot = true;

        const int w = data.width;
        const int h = data.height;

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                TileType type = TileType::Grass;
                decodeTile(data.tileRows[y][x], type);
                world.getTile(x, y).type = type;
            }
        }

#ifdef URBANIA_ENABLE_LOAD_FAILPOINT
        if (consumeLoadFailpoint())
        {
            throw std::runtime_error("test load failpoint");
        }
#endif

        Population& population = simulation.getPopulation();
        population.update(world, 0.0f);
        for (const auto& g : data.growth)
        {
            population.setGrowthProgress(std::get<0>(g), std::get<1>(g), std::get<2>(g));
        }
        population.getCitizenManager().restoreSaved(std::move(data.citizens), data.nextCitizenId);
        population.update(world, 0.0f);

        simulation.setElapsedSimulationSeconds(data.simElapsed);
        simulation.getEconomy().restoreSavedState(data.money, data.totalTax, data.totalMaint,
                                                  data.economySeconds);
        clock.restoreSavedState(data.clockTime, data.clockScale, data.clockPaused);
        simulation.getLandValue().restoreSavedState(data.hourLandValue);
        simulation.getHousing().restoreSavedState(data.hourHousing);
        simulation.getHappiness().restoreSavedState(data.hourHappiness);
        simulation.getDemand().restoreSavedState(data.hourDemand);

        simulation.getUtilities().setElectricityCapacity(data.utilElec);
        simulation.getUtilities().setWaterCapacity(data.utilWater);
        simulation.getUtilities().setSewageCapacity(data.utilSewage);

        simulation.getTransit().restoreSaved(data.stops, data.routes, data.nextStopId,
                                             data.nextRouteId, data.nextBusId);
        simulation.getPollution().restoreSavedState(data.pollution, data.pollutionSeconds,
                                                    data.pollutionAvg, data.pollutionMax);

        camera.setTargetZoom({ data.camX, data.camY }, data.camZoom);

        // Rebuild every derived cache; transient trips/buses respawn.
        // Employment job IDs continue monotonically from the live counter,
        // which restoreNextJobId() already clamped above the loaded data.
        simulation.getEmployment().restoreNextJobId(
            std::max(1, simulation.getEmployment().getNextJobId()));
        simulation.rebuildAfterLoad();

        result.ok = true;
        result.message = "City loaded";
        return result;
    }
    catch (const std::exception& e)
    {
        if (haveSnapshot)
        {
            simulation = std::move(simulationSnapshot);
            world.restoreTiles(std::move(tileSnapshot));
            camera = cameraSnapshot;
        }
        result.message = std::string("apply failed: ") + e.what();
        return result;
    }
}

}  // namespace urbania
