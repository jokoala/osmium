#ifdef STAND_ALONE
# define BOOST_TEST_MODULE Main
#endif
#include <boost/test/unit_test.hpp>
#include <boost/test/output_test_stream.hpp>
#include <data_file_fixture.hpp>

#include <osmium/osmfile.hpp>
#include <osmium/handler/debug.hpp>

#include <osmium/input/xml.hpp>


/* in this file are parsing tests for the xml input
 */

BOOST_AUTO_TEST_SUITE(Input_XML)

// we use private inheritance to ensure only overwritten methods are called
class FakeOSMFile { 
    public:

        FakeOSMFile(int fd, bool has_multiple_object_versions=false) {
            m_fake_fd = fd;
            m_has_multiple_object_versions = has_multiple_object_versions;
        }

        int fd() const {
            return m_fake_fd;
        }

        bool has_multiple_object_versions() {
            return m_has_multiple_object_versions;
        }

        void open_for_input() {
            return;
        }

    private:
        int m_fake_fd;
        bool m_has_multiple_object_versions;
};

BOOST_AUTO_TEST_CASE(XML_parse_correctlyParsesFile) {
    // open test.osm for input
    DataFileFixture example_osm("example.osm");
    int flags = O_RDONLY;
#ifdef WIN32
    flags |= O_BINARY;
#endif
    int fd = ::open(example_osm, flags);
    if (fd == -1) {
        ::perror("open");
        BOOST_ERROR("open failed");
    }
    FakeOSMFile fakeOsmFile(fd);

    // create an output handler
    boost::test_tools::output_test_stream output(example_osm.pattern(), true);
    Osmium::Handler::Debug debugHandler(false, output);

    // initialize the XML input handler
    Osmium::Input::XML<FakeOSMFile, Osmium::Handler::Debug> xmlInput(fakeOsmFile, debugHandler);

    xmlInput.parse();

    BOOST_CHECK(output.match_pattern());
}

BOOST_AUTO_TEST_SUITE_END()
