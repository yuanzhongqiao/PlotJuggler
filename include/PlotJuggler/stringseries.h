#ifndef STRINGSERIES_H
#define STRINGSERIES_H

#include "timeseries.h"
#include <algorithm>

namespace PJ {

class StringSeries: public TimeseriesBase<std::string_view>
{
public:
    using TimeseriesBase<std::string_view>::_points;

    StringSeries(const std::string& name):
        TimeseriesBase<std::string_view>(name)
    { }

    virtual void clear() override
    {
        _storage.clear();
        TimeseriesBase<std::string_view>::clear();
    }

    void pushBack(const Point &p)
    {
        auto temp = p;
        pushBack(std::move(temp));
    }

    virtual void pushBack(Point&& p) override
    {
        _tmp_str.assign( p.y.data(), p.y.size() );

        auto it = _storage.find( _tmp_str );
        if( it == _storage.end() ) {
            it = _storage.insert( _tmp_str ).first;
        }
        // new string_view should point at local storage
        TimeseriesBase<std::string_view>::pushBack( { p.x, std::string_view ( *it ) } );
    }

private:
    thread_local static std::string _tmp_str;
    std::set<std::string> _storage;
};

inline thread_local std::string StringSeries::_tmp_str;

} // end namespace

#endif
