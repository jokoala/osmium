/*
 * The code in this example file is released into the Public Domain.
*/

#define OSMIUM_WITH_PBF_INPUT
#define OSMIUM_WITH_XML_INPUT

#include <osmium.hpp>
#include <osmium/handler/progress.hpp>

class CityFinderHandler : public Osmium::Handler::Progress {

public:

    void node(const shared_ptr<Osmium::OSM::Node const>& node) {
        Osmium::Handler::Progress::node(node);
        const char *place = node->tags().get_value_by_key("place");
        if ((place != 0) && (::strncmp(place, "city",5) == 0)) {
            const char *name = node->tags().get_value_by_key("name");
            if (name != 0) {
                std::cout <<name <<std::endl;
            }
        }
    }

    void after_nodes() {
        Osmium::Handler::Progress::after_nodes();
        throw Osmium::Handler::StopReading();
    }
}; // class CityFinderHandler

/* ================================================== */

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " OSMFILE\n\n";
        exit(1);
    }

    Osmium::OSMFile infile(argv[1]);
    CityFinderHandler handler;
    Osmium::Input::read(infile, handler);

    google::protobuf::ShutdownProtobufLibrary();
}
