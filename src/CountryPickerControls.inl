bool assignPickerCountry(int country) {
    if(state.selected>=entries.size()||entries[state.selected].country!=Country::Unknown)return false;
    const auto& entry=entries[state.selected];const auto name=entry.modelName.empty()?std::to_string(entry.id):entry.modelName;
    if(!updateCountryIni(ini,country,name,vanillaVehicle(entry.id))){state.status="Could not save country assignment";state.cue=Cue::Error;return false;}
    countryConfig=readCountries(ini);
    for(auto& vehicle:catalog)vehicle.country=countryConfig.resolve(vehicle);
    for(auto& vehicle:entries)vehicle.country=countryConfig.resolve(vehicle);
    std::array<int,countryCount> counts{};for(const auto& vehicle:catalog)if(allowedHere(vehicle))++counts[size_t(vehicle.country)];state.ui.menu.setCountries(counts);
    state.countryPicker.open=false;state.ui.keyboard.open=false;state.cue=Cue::Accept;blockActions();return true;
}
void openCountryPicker() {
    loadCountryRegistry(std::filesystem::path(base)/L"GT4Showroom/languages");updateCountryIni(ini);
    const float x=state.countryPicker.x,y=state.countryPicker.y;state.countryPicker={};state.countryPicker.x=x;state.countryPicker.y=y;state.countryPicker.open=true;
    state.countryPicker.clock=state.clock;state.countryPicker.openedAt=state.clock;state.countryPicker.changedAt=state.clock;
    countrySearchInput.quarantine();state.cue=Cue::Open;blockActions();
}
