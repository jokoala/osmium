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
    Osmium::Handler::Enumerator example();
}

BOOST_AUTO_TEST_CASE( read_example_file ) {
    Osmium::Handler::Enumerator enumerator;
    Osmium::OSMFile infile("data/example.osm");

    Osmium::Handler::Enumerator::Iterator it = enumerator(infile, enumerator);

    for (int i=1; i<=9; i++) {
        BOOST_TEST_MESSAGE("i=" <<i);
        BOOST_REQUIRE(it);
        BOOST_CHECK_EQUAL((*it)->id(), i);
        BOOST_CHECK_EQUAL((*it)->uid(), 31337);
        BOOST_CHECK_EQUAL((*it)->user(), "haxor");
        ++it;
    }
    BOOST_REQUIRE(!it);
}

BOOST_AUTO_TEST_SUITE_END()
