#ifndef OSMIUM_JAVASCRIPT_TEMPLATE_HPP
#define OSMIUM_JAVASCRIPT_TEMPLATE_HPP

namespace Osmium {

    namespace Javascript {

        namespace Template {

            /**
            * Base class for all Javascript templates.
            */
            class Base {

              protected:

                v8::Persistent<v8::ObjectTemplate> js_template;

                Base(int field_count=1) {
                    js_template = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());
                    js_template->SetInternalFieldCount(field_count);
                }

                ~Base() {
                    js_template.Dispose();
                }

              public:

                v8::Local<v8::Object> create_instance(void *wrapper) {
                    v8::Local<v8::Object> instance = js_template->NewInstance();
                    instance->SetInternalField(0, v8::External::New(wrapper));
                    return instance;
                }

                /*
                   These magic helper function are used to connect Javascript
                   methods to C++ methods. They are given to the SetAccessor,
                   SetIndexedPropertyHandler and SetNamedPropertyHandler
                   functions of a v8::ObjectTemplate object, respectively.

                   The first template argument is the class of the object we
                   want to access, for instance Osmium::OSM::Node.

                   The second template argument is the member function on the
                   object that we want to call when this function is called
                   from Javascript.

                */
                template<class TObject, v8::Handle<v8::Value> (TObject::*func)() const>
                static v8::Handle<v8::Value> accessor_getter(v8::Local<v8::String>, const v8::AccessorInfo &info) {
                    return (( reinterpret_cast<TObject *>(v8::Local<v8::External>::Cast(info.Holder()->GetInternalField(0))->Value()) )->*(func))();
                }

                template<class TObject, v8::Handle<v8::Value> (TObject::*func)(v8::Local<v8::String> property) const>
                static v8::Handle<v8::Value> named_property_getter(v8::Local<v8::String> property, const v8::AccessorInfo &info) {
                    return (( reinterpret_cast<TObject *>(v8::Local<v8::External>::Cast(info.Holder()->GetInternalField(0))->Value()) )->*(func))(property);
                }

                template<class TObject, v8::Handle<v8::Value> (TObject::*func)(uint32_t index) const>
                static v8::Handle<v8::Value> indexed_property_getter(uint32_t index, const v8::AccessorInfo &info) {
                    return (( reinterpret_cast<TObject *>(v8::Local<v8::External>::Cast(info.Holder()->GetInternalField(0))->Value()) )->*(func))(index);
                }

                template<class TObject, v8::Handle<v8::Value> (TObject::*func)(uint32_t index)>
                static v8::Handle<v8::Value> indexed_property_getter(uint32_t index, const v8::AccessorInfo &info) {
                    return (( reinterpret_cast<TObject *>(v8::Local<v8::External>::Cast(info.Holder()->GetInternalField(0))->Value()) )->*(func))(index);
                }

                template<class TObject, v8::Handle<v8::Array> (TObject::*func)() const>
                static v8::Handle<v8::Array> property_enumerator(const v8::AccessorInfo &info) {
                    return (( reinterpret_cast<TObject *>(v8::Local<v8::External>::Cast(info.Holder()->GetInternalField(0))->Value()) )->*(func))();
                }

            }; //class Base

            v8::Local<v8::Object> create_tags_instance(void *wrapper);

            v8::Local<v8::Object> create_node_instance(void *wrapper);

            v8::Local<v8::Object> create_node_geom_instance(void *wrapper);

            v8::Local<v8::Object> create_way_instance(void *wrapper);

            v8::Local<v8::Object> create_way_nodes_instance(void *wrapper);

            v8::Local<v8::Object> create_way_geom_instance(void *wrapper);

            v8::Local<v8::Object> create_relation_member_instance(void *wrapper);

            v8::Local<v8::Object> create_relation_instance(void *wrapper);

            v8::Local<v8::Object> create_relation_members_instance(void *wrapper);

            v8::Local<v8::Object> create_multipolygon_instance(void *wrapper);

            v8::Local<v8::Object> create_output_csv_instance(void *wrapper);

            v8::Local<v8::Object> create_output_shapefile_instance(void *wrapper);

            void init();

            void cleanup();

        } // namespace Template

    } // namespace Javascript

} // namespace Osmium

#endif // OSMIUM_JAVASCRIPT_TEMPLATE_HPP
