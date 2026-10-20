#pragma once
#include "Showroom.hpp"
#include <cctype>
namespace gt4 {
enum class SortBy{Id,Name,Class,TopSpeed,Country};
struct SortSettings{SortBy by=SortBy::Id;bool descending=false;};
inline std::string folded(std::string text){for(char& c:text)c=char(std::tolower(static_cast<unsigned char>(c)));return text;}
inline void sortCatalogue(std::vector<Entry>& cars,SortSettings order){
 std::stable_sort(cars.begin(),cars.end(),[&](const Entry&a,const Entry&b){
  int result=0;
  if(order.by==SortBy::TopSpeed){const bool av=std::isfinite(a.topSpeed)&&a.topSpeed>=0,bv=std::isfinite(b.topSpeed)&&b.topSpeed>=0;if(av!=bv)return av;if(av)result=a.topSpeed<b.topSpeed?-1:a.topSpeed>b.topSpeed?1:0;}
  else if(order.by==SortBy::Name){const auto an=folded(a.name),bn=folded(b.name);result=an<bn?-1:an>bn?1:0;}
  else if(order.by==SortBy::Country){const auto an=std::string(countryKeys[size_t(a.country)]),bn=std::string(countryKeys[size_t(b.country)]);const bool av=a.country!=Country::Unknown,bv=b.country!=Country::Unknown;if(av!=bv)return av;result=an<bn?-1:an>bn?1:0;}
  else if(order.by==SortBy::Class)result=a.classId<b.classId?-1:a.classId>b.classId?1:0;
  else result=a.id<b.id?-1:a.id>b.id?1:0;
  if(result)return order.descending?result>0:result<0;
  const auto an=folded(a.name),bn=folded(b.name);if(order.by!=SortBy::Id&&an!=bn)return an<bn;return a.id<b.id;
 });
}
template<class Hidden,class IsFavorite>void buildCatalogueView(Model&state,std::vector<Entry>&out,const std::vector<Entry>&all,SortSettings order,Hidden hidden,IsFavorite favorite){
 const size_t previous=state.selected;const int id=out.empty()?-1:out[std::min(previous,out.size()-1)].id;
 std::array<int,countryCount> countryTotals{};out.clear();for(auto e:all){if(hidden(e))continue;e.favorite=favorite(e);if(state.ui.favoritesOnly&&!e.favorite)continue;++countryTotals[size_t(e.country)];if(state.ui.countryFilter<0||int(e.country)==state.ui.countryFilter)out.push_back(std::move(e));}state.ui.menu.setCountries(countryTotals);
 sortCatalogue(out,order);state.selected=out.empty()?0:std::min(previous,out.size()-1);bool retained=false;for(size_t i=0;i<out.size();++i)if(out[i].id==id){state.selected=i;retained=true;break;}
 if(id>=0&&!retained){state.page=Page::Cars;state.paint=0;state.spawn=false;state.changed=state.clock;}state.count=out.size();state.scroll=state.scrollTarget();
}
}
