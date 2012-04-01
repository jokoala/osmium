#ifndef OSMIUM_INPUT_XML_HPP
#define OSMIUM_INPUT_XML_HPP

/*

Copyright 2011 Jochen Topf <jochen@topf.org> and others (see README).

This file is part of Osmium (https://github.com/joto/osmium).

Osmium is free software: you can redistribute it and/or modify it under the
terms of the GNU Lesser General Public License or (at your option) the GNU
General Public License as published by the Free Software Foundation, either
version 3 of the Licenses, or (at your option) any later version.

Osmium is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU Lesser General Public License and the GNU
General Public License for more details.

You should have received a copy of the Licenses along with Osmium. If not, see
<http://www.gnu.org/licenses/>.

*/

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>
#include <cstring>
#include <sstream>

#include <expat.h>

namespace Osmium {

    namespace Input {

        /**
        * Class for parsing OSM XML files.
        *
        * Generally you are not supposed to instantiate this class yourself.
        * Instead create an OSMFile object and call its read() method.
        *
        * @tparam THandler A handler class (subclass of Osmium::Handler::Base).
        */
        template <class THandler>
        class XML : public Base<THandler> {

        public:

            /**
            * Instantiate XML Parser.
            *
            * @param file OSMFile instance.
            * @param handler Instance of THandler.
            */
            XML(Osmium::OSMFile& file, THandler& handler) : Base<THandler>(file, handler), m_current_object(NULL), m_in_delete_section(false) {
            }

            void parse() {
                int done;

                XML_Parser parser = XML_ParserCreate(0);
                if (!parser) {
                    throw std::runtime_error("Error creating parser");
                }

                XML_SetUserData(parser, this);

                XML_SetElementHandler(parser, Osmium::Input::XML<THandler>::start_element_wrapper, Osmium::Input::XML<THandler>::end_element_wrapper);

                try {
                    do {
                        void* buffer = XML_GetBuffer(parser, c_buffer_size);
                        if (buffer == 0) {
                            throw std::runtime_error("out of memory");
                        }

                        int result = read(this->get_fd(), buffer, c_buffer_size);
                        if (result < 0) {
                            exit(1);
                        }
                        this->m_filepos += result;
                        this->call_progress_handler();
                        done = (result == 0);
                        if (XML_ParseBuffer(parser, result, done) == XML_STATUS_ERROR) {
                            XML_Error errorCode = XML_GetErrorCode(parser);
                            long errorLine = XML_GetCurrentLineNumber(parser);
                            long errorCol = XML_GetCurrentColumnNumber(parser);
                            const XML_LChar *errorString = XML_ErrorString(errorCode);

                            std::stringstream errorDesc;
                            errorDesc << "XML parsing error at line " << errorLine << ":" << errorCol;
                            errorDesc << ": " << errorString;
                            throw std::runtime_error(errorDesc.str());
                        }
                    } while (!done);
                    XML_ParserFree(parser);

                    this->call_after_and_before_on_handler(UNKNOWN);
                } catch (Osmium::Input::StopReading) {
                    // if a handler says to stop reading, we do
                }
                this->call_final_on_handler();
            }

        private:

            static const int c_buffer_size = 10240;

            Osmium::OSM::Object* m_current_object;

            /**
             * This is used only for change files which contain create, modify,
             * and delete sections.
             */
            bool m_in_delete_section;

            static void XMLCALL start_element_wrapper(void* data, const XML_Char* element, const XML_Char** attrs) {
                ((Osmium::Input::XML<THandler> *)data)->start_element(element, attrs);
            }

            static void XMLCALL end_element_wrapper(void* data, const XML_Char* element) {
                ((Osmium::Input::XML<THandler> *)data)->end_element(element);
            }

            void init_object(Osmium::OSM::Object& obj, const XML_Char** attrs) {
                if (m_in_delete_section) {
                    obj.visible(false);
                }
                m_current_object = &obj;
                for (int count = 0; attrs[count]; count += 2) {
                    if (!strcmp(attrs[count], "lon")) {
                        if (this->m_node) {
                            this->m_node->set_x(atof(attrs[count+1]));
                        }
                    } else if (!strcmp(attrs[count], "lat")) {
                        if (this->m_node) {
                            this->m_node->set_y(atof(attrs[count+1]));
                        }
                    } else {
                        m_current_object->set_attribute(attrs[count], attrs[count+1]);
                    }
                }
            }

            void start_element(const XML_Char* element, const XML_Char** attrs) {
                // order in the following "if" statements is based on frequency of tags in planet file
                if (!strcmp(element, "nd")) {
                    for (int count = 0; attrs[count]; count += 2) {
                        if (!strcmp(attrs[count], "ref")) {
                            this->m_way->add_node(atoll(attrs[count+1]));
                        }
                    }
                } else if (!strcmp(element, "node")) {
                    this->call_after_and_before_on_handler(NODE);
                    init_object(this->prepare_node(), attrs);
                } else if (!strcmp(element, "tag")) {
                    const char *key = "", *value = "";
                    for (int count = 0; attrs[count]; count += 2) {
                        if (attrs[count][0] == 'k' && attrs[count][1] == 0) {
                            key = attrs[count+1];
                        }
                        if (attrs[count][0] == 'v' && attrs[count][1] == 0) {
                            value = attrs[count+1];
                        }
                    }
                    // XXX assert key, value exist
                    if (m_current_object) {
                        m_current_object->tags().add(key, value);
                    }
                } else if (!strcmp(element, "way")) {
                    this->call_after_and_before_on_handler(WAY);
                    init_object(this->prepare_way(), attrs);
                } else if (!strcmp(element, "member")) {
                    char        type = 'x';
                    uint64_t    ref  = 0;
                    const char *role = "";
                    for (int count = 0; attrs[count]; count += 2) {
                        if (!strcmp(attrs[count], "type")) {
                            type = (char)attrs[count+1][0];
                        } else if (!strcmp(attrs[count], "ref")) {
                            ref = atoll(attrs[count+1]);
                        } else if (!strcmp(attrs[count], "role")) {
                            role = (char *)attrs[count+1];
                        }
                    }
                    // XXX assert type, ref, role are set
                    if (m_current_object && this->m_relation) {
                        this->m_relation->add_member(type, ref, role);
                    }
                } else if (!strcmp(element, "relation")) {
                    this->call_after_and_before_on_handler(RELATION);
                    init_object(this->prepare_relation(), attrs);
                } else if (!strcmp(element, "bounds")) {
                    Osmium::OSM::Position min;
                    Osmium::OSM::Position max;
                    for (int count = 0; attrs[count]; count += 2) {
                        if (!strcmp(attrs[count], "minlon")) {
                            min.lon(atof(attrs[count+1]));
                        } else if (!strcmp(attrs[count], "minlat")) {
                            min.lat(atof(attrs[count+1]));
                        } else if (!strcmp(attrs[count], "maxlon")) {
                            max.lon(atof(attrs[count+1]));
                        } else if (!strcmp(attrs[count], "maxlat")) {
                            max.lat(atof(attrs[count+1]));
                        }
                    }
                    this->meta().bounds().extend(min).extend(max);
                } else if (!strcmp(element, "delete")) {
                    m_in_delete_section = true;
                }
            }

            void end_element(const XML_Char* element) {
                if (!strcmp(element, "node")) {
                    this->call_node_on_handler();
                    m_current_object = NULL;
                } else if (!strcmp(element, "way")) {
                    this->call_way_on_handler();
                    m_current_object = NULL;
                } else if (!strcmp(element, "relation")) {
                    this->call_relation_on_handler();
                    m_current_object = NULL;
                } else if (!strcmp(element, "delete")) {
                    m_in_delete_section = false;
                }
            }

        }; // class XML

    } // namespace Input

} // namespace Osmium

#endif // OSMIUM_INPUT_XML_HPP
