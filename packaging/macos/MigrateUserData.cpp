// Repair literal Windows path names left by early native builds. This helper
// ships in the app so migration needs neither Python nor developer tools.
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
namespace fs = std::filesystem;

static fs::file_status Status(const fs::path &path) {
    std::error_code error;
    auto status = fs::symlink_status(path, error);
    if (error && error != std::errc::no_such_file_or_directory)
        throw fs::filesystem_error("inspect migration path", path, error);
    return status;
}
static bool Present(const fs::path &path) { return fs::exists(Status(path)); }

static fs::path Destination(const fs::path &relative) {
    std::string name = relative.generic_string();
    if (name.find('\\') == std::string::npos) return {};
    // This exact legacy default was a literal directory name, never a
    // request to traverse out of the application support directory.
    const std::string oldTas = "..\\ModLoader\\TAS\\";
    if (name.compare(0, oldTas.size(), oldTas) == 0) name.erase(0, 3);
    std::replace(name.begin(), name.end(), '\\', '/');
    fs::path result;
    for (const auto &part : fs::path(name)) {
        if (part.empty() || part == ".") continue;
        if (part == ".." || part == "/") return {};
        result /= part;
    }
    if (result.empty() || *result.begin() != "ModLoader") return {};
    return result;
}

class Migration {
    fs::path m_Root;
    std::ofstream m_Log;
    void Log(const char *action, const fs::path &source, const fs::path &target) {
        if (!m_Log.is_open()) {
            auto path = m_Root / "PathMigration.log";
            if (fs::is_symlink(Status(path))) throw std::runtime_error("migration log is a symlink");
            m_Log.open(path, std::ios::app);
            if (!m_Log) throw std::runtime_error("cannot open migration log");
        }
        m_Log << action << ' ' << std::quoted(source.lexically_relative(m_Root).generic_string())
              << " -> " << std::quoted(target.lexically_relative(m_Root).generic_string()) << '\n';
        m_Log.flush();
        if (!m_Log) throw std::runtime_error("cannot write migration log");
    }
    // Never follow an existing symlink in a destination's parent chain.
    bool Directories(const fs::path &relative) {
        fs::path current = m_Root;
        for (const auto &part : relative) {
            if (part.empty() || part == ".") continue;
            if (part == ".." || part == "/") return false;
            current /= part;
            auto status = Status(current);
            if (!fs::exists(status)) fs::create_directory(current);
            else if (!fs::is_directory(status)) return false;
        }
        return true;
    }
    static bool Identical(const fs::path &a, const fs::path &b) {
        if (!fs::is_regular_file(Status(a)) || !fs::is_regular_file(Status(b)) ||
            fs::file_size(a) != fs::file_size(b)) return false;
        std::ifstream left(a, std::ios::binary), right(b, std::ios::binary);
        if (!left || !right) throw std::runtime_error("cannot compare migration files");
        std::array<char, 65536> x{}, y{};
        do {
            left.read(x.data(), x.size()); right.read(y.data(), y.size());
            if (left.bad() || right.bad()) throw std::runtime_error("migration comparison read failed");
            if (left.gcount() != right.gcount() ||
                !std::equal(x.begin(), x.begin() + left.gcount(), y.begin())) return false;
        } while (left.gcount());
        return true;
    }
    void Archive(const fs::path &source, const fs::path &relative) {
        fs::path base = fs::path("LegacyPathConflicts") / relative;
        if (!Directories(base.parent_path())) throw std::runtime_error("conflict backup path is blocked");
        fs::path target = m_Root / base;
        for (unsigned suffix = 1; Present(target); ++suffix) {
            if (Identical(source, target)) {
                Log("duplicate-of-backup", source, target); fs::remove(source); return;
            }
            target = m_Root / fs::path(base.string() + ".legacy-" + std::to_string(suffix));
        }
        Log("preserve-conflict", source, target);
        fs::rename(source, target);
    }
    void Move(const fs::path &source, const fs::path &relative) {
        if (!Present(source)) return;
        auto status = Status(source);
        // Keep links intact as backups; do not install them as live loader paths.
        if (fs::is_symlink(status) || !Directories(relative.parent_path())) {
            Archive(source, relative); return;
        }
        fs::path target = m_Root / relative;
        auto targetStatus = Status(target);
        if (!fs::exists(targetStatus)) {
            Log("move", source, target); fs::rename(source, target); return;
        }
        if (fs::is_directory(status) && fs::is_directory(targetStatus) && fs::is_empty(source)) {
            Log("merge-empty-directory", source, target); fs::remove(source); return;
        }
        if (Identical(source, target)) {
            Log("duplicate", source, target); fs::remove(source); return;
        }
        // Native destination wins; preserve differing legacy content verbatim.
        Archive(source, relative);
    }
public:
    explicit Migration(fs::path root) : m_Root(std::move(root)) {}
    void Run() {
        std::vector<std::pair<fs::path, fs::path>> entries;
        for (const auto &top : fs::directory_iterator(m_Root)) {
            const auto rel = top.path().lexically_relative(m_Root);
            if (rel != "ModLoader" && Destination(rel).empty()) continue;
            auto add = [&](const fs::path &path) {
                auto destination = Destination(path.lexically_relative(m_Root));
                if (!destination.empty()) entries.emplace_back(path, destination);
            };
            add(top.path());
            if (fs::is_directory(top.symlink_status()))
                for (const auto &entry : fs::recursive_directory_iterator(top.path())) add(entry.path());
        }
        // Move children first so merging a malformed parent cannot hide a
        // still-unmigrated nested name. The iterator never follows links.
        std::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) {
            auto depth = [](const fs::path &p) { return std::distance(p.begin(), p.end()); };
            auto x = depth(a.first), y = depth(b.first);
            return x != y ? x > y : a.first.generic_string() < b.first.generic_string();
        });
        for (const auto &entry : entries) Move(entry.first, entry.second);
    }
};
int main(int argc, char **argv) {
    if (argc != 2) { std::cerr << "usage: MigrateUserData USER_DATA_DIRECTORY\n"; return 2; }
    int lock = -1;
    try {
        fs::create_directories(argv[1]);
        fs::path root = fs::canonical(argv[1]);
        lock = open((root / ".path-migration.lock").c_str(), O_CREAT | O_RDWR | O_NOFOLLOW, 0600);
        if (lock < 0 || flock(lock, LOCK_EX) != 0) throw std::runtime_error("cannot lock user-data migration");
        Migration(root).Run();
        close(lock); return 0;
    } catch (const std::exception &error) {
        if (lock >= 0) close(lock);
        std::cerr << "Ballance data migration: " << error.what() << '\n'; return 1;
    }
}
