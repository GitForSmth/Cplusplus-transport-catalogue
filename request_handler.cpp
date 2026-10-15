#include "request_handler.h"

namespace transport {

RequestHandler::RequestHandler(const TransportCatalogue& db) : db_(db) {}

std::optional<BusInfo> RequestHandler::GetBusStat(const std::string_view& bus_name) const {
    return db_.GetBusInfo(bus_name);
}

const std::set<std::string_view>* RequestHandler::GetBusesByStop(const std::string_view& stop_name) const {
    return db_.GetBusesForStop(stop_name);
}

std::map<std::string_view, const domain::Bus*> RequestHandler::GetSortedBuses() const {
    return db_.GetSortedBuses();
}

} // namespace transport