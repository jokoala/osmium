#ifndef OSMIUM_HANDLER_ITERATOR_HPP
#define OSMIUM_HANDLER_ITERATOR_HPP

/*

Copyright 2012 Jochen Topf <jochen@topf.org> and others (see README).

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

#include <boost/context/all.hpp>

namespace Osmium {

    namespace Handler {

        /**
         * @brief handler with iterator interface
         *
         * This handler allows to iterate over all objects processed.
         * It uses the boost::context library for coroutine calling.
         *
         * @todo Klasse fertigschreiben
         */
        class Enumerator : public Osmium::Handler::Base {
            public:

                /** 
                 * @brief Iterator for Enumerator classes
                 */
                class Iterator {
                    public:
                        /// continue the handler until next yield
                        Iterator& operator++() {
                            m_parent->call(HANDLER_CONTINUE);
                            return *this;
                        }

                        /// is the iterator already finished
                        operator bool() const {
                            return (m_parent->m_state == RUNNING);
                        }

                        /// current object
                        shared_ptr<Osmium::OSM::Object const>& operator*() {
                            return m_parent->m_cur_object;
                        }

                    private:
                        Iterator(Enumerator *parent) :
                            m_parent(parent) {
                        }
                };

                Enumerator() :
                    Osmium::Handler::Base(),
                    m_state(IDLE),
                    m_allocator() {

                    m_stack_size = m_alloc.minimum_stacksize();
                    m_handler_stack = m_alloc.allocate(m_stack_size);
                }

                ~Enumerator() {
                    if (m_state == RUNNING) {
                    }
                }


                /** @name inherited methods
                 * Now the inherited methods of Handler
                 */
                ///@{

                void node(const shared_ptr<Osmium::OSM::Node const>& node) const {
                    m_cur_object = node;
                    yield();
                    m_cur_object.reset();
                }

                void way(const shared_ptr<Osmium::OSM::Way const>& way) const {
                }

                ///@}


            private:

                /// possible types for internal state
                enum iterator_state_t {
                    IDLE = 0,
                    RUNNING 1
                };

                /// magic constants for communication with coroutine
                enum handler_notification_t {
                    /// coroutine->main: coroutine reached a yield statement
                    HANDLER_YIELD = 0,
                    /// coroutine->main: coroutine done
                    HANDLER_FINISHED = 1,
                    /// main->coroutine: continue
                    HANDLER_CONTINUE = 2,
                    /// main->coroutine: unroll the stack at once
                    HANDLER_STOP = 3
                };

                /// current iterator state
                iterator_state_t m_state;

                /// context of main program
                boost::context::fcontext_t m_ctx_main;

                /// context of the coroutine for handler
                boost::context::fcontext_t *m_ctx_coroutine;

                /// allocator for context objects
                boost::context::guarded_stack_allocator m_allocator;

                /// stack for coroutine
                void *m_coroutine_stack;
                std::size_t m_stack_size;

                /// temporary buffer of current OSM::Object
                mutable shared_ptr<Osmium::OSM::Object const> m_cur_object;

                /// parameters for coroutine
                void *m_coroutine_file;
                void *m_coroutine_handler;

        }; // class Enumerator

    } // namespace Handler

} // namespace Osmium

#endif // OSMIUM_HANDLER_ITERATOR_HPP
