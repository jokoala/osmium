#define BOOST_TEST_DYN_LINK
#ifdef STAND_ALONE
# define BOOST_TEST_MODULE Main
#endif
#include <boost/test/unit_test.hpp>

#define OSMIUM_WITH_XML_INPUT 1
#define OSMIUM_WITH_PBF_INPUT 1

#include <osmium/handler/enumerator.hpp>

BOOST_AUTO_TEST_SUITE(Enumerator_Handler)

BOOST_AUTO_TEST_CASE( create_instance ) {
    Osmium::Handler::Enumerator();
}

BOOST_AUTO_TEST_SUITE_END()
