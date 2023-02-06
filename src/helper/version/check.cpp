#include "check.hpp"
#include <fancy.hpp>
#include <json.hpp>
#include <optional>
#include <semver.hpp>

httplib::Client VersionCheck::client("https://api.github.com");

std::optional<Soundux::Objects::VersionStatus> VersionCheck::getStatus()
{
    Fancy::fancy.logTime().warning() << "Request DISABLED!" << std::endl;
    return std::nullopt;

    auto githubTags = client.Get("/repos/Soundux/Soundux/tags");

    if (githubTags && githubTags->status == 200)
    {
        auto parsed = nlohmann::json::parse(githubTags->body, nullptr, false);

        if (!parsed.is_discarded())
        {
            auto latestTag = parsed[0]["name"];
            if (!latestTag.is_null())
            {
                auto latestTagStr = latestTag.get<std::string>();

                try
                {
                    auto remote = semver::from_string(latestTagStr);
                    auto local = semver::from_string(SOUNDUX_VERSION);

                    return Soundux::Objects::VersionStatus{SOUNDUX_VERSION, latestTagStr, remote > local};
                }
                catch (const std::exception &e)
                {
                    Fancy::fancy.logTime().warning() << "Could not fetch version" << std::endl;
                }
            }
            Fancy::fancy.logTime().warning() << "Failed to find latest tag" << std::endl;
        }
        else
        {
            Fancy::fancy.logTime().warning() << "Failed to parse github response" << std::endl;
        }
    }
    else
    {
        Fancy::fancy.logTime().warning() << "Request failed!" << std::endl;
    }
    return std::nullopt;
}
