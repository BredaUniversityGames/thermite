#pragma once
#include <steam/steam_api.h>
namespace tmt {

constexpr auto APP_ID = 4515200u;
class SteamAPI {
   public:
    SteamAPI();
    ~SteamAPI();
    void update();
    bool get_success_init() const { return success_init; }

   private:
    bool success_init { true };
};

}  // namespace tmt