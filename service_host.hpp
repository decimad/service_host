#ifndef SERVICE_HOST_HPP__
#define SERVICE_HOST_HPP__

#include "type_list.hpp"

namespace util {

namespace detail {

template< typename Service, typename EventList >
struct collect_events_body
{
    using type = util::concat_t< EventList, typename Service::events >;
};

template< typename service_host, size_t Index = 0, size_t End = util::list_size< typename service_host::event_list >::value >
struct handle_event {
    static void _( service_host& host, size_t mask )
    {
        if( mask == (1ul << Index) ) {
            using handler = util::get_t< typename service_host::event_list, Index >;
            (host.template get_service< typename handler::service_impl_type >().*handler::handler_ptr)();
        }
        else {
            handle_event< service_host, Index + 1, End >::_( host, mask );
        }
    }
};

template<typename service_host, size_t End >
struct handle_event< service_host, End, End >
{
    static void _( service_host& host, size_t mask )
    {}
};

}

template< size_t StackSize, size_t IdleTimeout, typename... Services >
class service_host : private Services::template service_impl< service_host<StackSize, IdleTimeout, Services...> >...
{
private:
    template< typename Host, size_t Index, size_t End >
    friend struct detail::handle_event;

    using service_list = util::type_list< Services... >;
    using service_impl_list = util::type_list< typename Services::template service_impl< service_host >... >;

    using event_list = util::for_each_t< service_impl_list, detail::collect_events_body, util::type_list< > >;

    static_assert(util::list_size< event_list >::value <= 32, "Too many events!");

    template< typename ServiceImpl >
    static constexpr size_t get_service_impl_index()
    {
        return util::find_index_v< service_impl_list, ServiceImpl >;
    }

    template< typename Service >
    static constexpr size_t get_service_index()
    {
        return get_service_impl_index< typename Service::template service_impl<service_host> >();
    }

    template< typename ServiceEvent >
    static constexpr size_t get_event_index()
    {
        using event_type = util::get_t< typename ServiceEvent::service_type::template service_impl<service_host>::events, ServiceEvent::index >;
        return util::find_index_v< event_list, event_type >;
    }

    template< typename ServiceEvent >
    static constexpr size_t get_event_mask()
    {
        return size_t( 1 ) << get_event_index<ServiceEvent>();
    }

public:
    template< typename Service >
    void send_message( typename Service::message_type msg )
    {}

    template< typename ServiceEvent >
    void trigger()
    {
        detail::handle_event< service_host >::_( *this, get_event_mask<ServiceEvent>() );
    }

    template< typename Service >
    typename Service::template service_impl<service_host>& get_service()
    {
        return *this;
    }

private:
    template< typename Host >
    friend struct service_impl_base;
};

template< typename ServiceImpl >
struct service_impl_base;

template< template< typename > class ServiceImplType, typename Host >
struct service_impl_base< ServiceImplType< Host > >
{
    using events = util::type_list< >;

    Host& get_host()
    {
        return *static_cast<Host*>(this);
    }

    template< typename Service >
    typename Service::template service_impl<Host>& get_service()
    {
        return get_host();
    }
};

template< typename ServiceImpl, void (ServiceImpl::*HandlerPtr)() >
struct service_event_impl
{
    using service_impl_type = ServiceImpl;
    static constexpr void (ServiceImpl::*handler_ptr)() = HandlerPtr;
};

template< typename Service, size_t Index >
struct service_event
{
    using service_type = Service;
    static constexpr size_t index = Index;
};

}


#endif
