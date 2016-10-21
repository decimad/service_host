#include <iostream>
#include <type_traits>
#include "service_host.hpp"

struct lwip_service
{
    template< typename Host >
    struct service_impl : public util::service_impl_base<service_impl<Host>>
    {
        void some_call()
        {
            std::cout << "Called!\n";
        }
    };
};

struct ptp_service
{
    template< typename Host >
    struct service_impl : public util::service_impl_base<service_impl<Host>>
    {
    };
};

struct phy_service
{
    template< typename Host >
    struct service_impl : public util::service_impl_base< service_impl<Host> >
    {
        void on_event() {
            // get service we rely on inside our host
            this->template get_service<lwip_service>().some_call();
            std::cout << "Event fired!\n";
        }

        using event  = util::service_event_impl< service_impl, &service_impl::on_event >;
        using events = util::type_list< event >;
    };

    using event = util::service_event< phy_service, 0 >;
};

int main()
{
    using ethernet_host_t = util::service_host< 4096, 100, phy_service, lwip_service, ptp_service >;
    ethernet_host_t host;

    host.template trigger< phy_service::event >();
    host.template get_service< lwip_service >().some_call();

    constexpr size_t size = sizeof( host );

    return 0;
}

