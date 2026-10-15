#pragma once

#include "transport_catalogue.h"
#include <optional>
#include <set>
#include <string_view>
#include <map>

namespace transport {

class RequestHandler {
public:
    explicit RequestHandler(const TransportCatalogue& db);

    std::optional<BusInfo> GetBusStat(const std::string_view& bus_name) const;
    const std::set<std::string_view>* GetBusesByStop(const std::string_view& stop_name) const;
    std::map<std::string_view, const domain::Bus*> GetSortedBuses() const;

private:
    const TransportCatalogue& db_;
};

} // namespace transport