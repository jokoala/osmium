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

#ifndef OSMIUM_WITH_PBF_INPUT
    #ifndef OSMIUM_WITH_XML_INPUT
        #error no file input activated, please define OSMIUM_WITH_PBF_INPUT or OSMIUM_WITH_XML_INPUT
    #endif 
#endif

#include <osmium.hpp>
#include <osmium/handler.hpp>
#include <osmium/osmfile.hpp>

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
            private:
                /// possible types for internal state
                enum iterator_state_t {
                    IDLE = 0,
                    RUNNING = 1
                };

                /// magic constants for communication with coroutine
                enum handler_notification_t {
                    /// coroutine->main: coroutine reached a yield statement
                    COROUTINE_YIELD = 0,
                    /// coroutine->main: coroutine done
                    COROUTINE_FINISHED = 1,
                    /// main->coroutine: continue
                    MAIN_CONTINUE = 2,
                    /// main->coroutine: unroll the stack at once
                    MAIN_STOP = 3
                };

            public:

                /** 
                 * @brief Iterator for Enumerator classes
                 */
                class Iterator {
                    friend class Enumerator;

                    public:
                        /// continue the handler until next yield
                        Iterator& operator++() {
                            m_parent->switch_to_coroutine(MAIN_CONTINUE);
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

                        /// parent class
                        Enumerator *m_parent;
                };

                Enumerator() :
                    Osmium::Handler::Base(),
                    m_state(IDLE),
                    m_allocator(),
                    m_iterator(this) {

                    m_stack_size = m_allocator.minimum_stacksize();
                    m_coroutine_stack = m_allocator.allocate(m_stack_size);
                }

                ~Enumerator() {
                    if (m_state == RUNNING) {
                        switch_to_coroutine(MAIN_STOP);
                        BOOST_ASSERT_MSG( m_state == IDLE, "stack unrolling failed");
                    }
                    m_allocator.deallocate(m_coroutine_stack, m_stack_size);
                }

                /**
                 * @brief start coroutine
                 *
                 * Starts a coroutine running Osmium::read with supplied parameters.
                 *
                 * @returns Enumerator::Iterator instance
                 */
                template <class T>
                Enumerator::Iterator& operator()(Osmium::OSMFile &file, T &handler) {
                    BOOST_ASSERT_MSG( m_state == IDLE, "handler is running, can't run twice");

                    m_coroutine_parameter_file = &file;
                    m_coroutine_parameter_handler = &handler;

                    m_ctx_coroutine = boost::context::make_fcontext(m_coroutine_stack, m_stack_size, coroutine<T>);

                    m_state = RUNNING;
                    switch_to_coroutine(reinterpret_cast<intptr_t>(this));

                    return m_iterator;
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
                    m_cur_object = way;
                    yield();
                    m_cur_object.reset();
                }

                void relation(const shared_ptr<Osmium::OSM::Relation const>& rel) const {
                    m_cur_object = rel;
                    yield();
                    m_cur_object.reset();
                }

                ///@}


            private:

                /** 
                 * @brief starting point of coroutine
                 *
                 * The coroutine starts here. Call semantics is dictated by boost::coroutine.
                 * The real parameters are therefore put in the instance variables
                 * coroutine_parameter_*
                 *
                 * The parameter is interpreted as pointer to the Enumerator instance.
                 */
                template <class T>
                static void coroutine(intptr_t param) {
                    Enumerator *self = reinterpret_cast<Enumerator*>(param);
                    Osmium::OSMFile *infile = reinterpret_cast<Osmium::OSMFile*>(self->m_coroutine_parameter_file);
                    T *handler = reinterpret_cast<T*>(self->m_coroutine_parameter_handler);

                    Osmium::Input::read(*infile, *handler);

                    boost::context::jump_fcontext(self->m_ctx_coroutine, &self->m_ctx_main, COROUTINE_FINISHED);

                    // should never return here
                    BOOST_ASSERT_MSG(false, "tried to continue a finished coroutine");
                }

                /**
                 * @brief switch to the coroutine
                 *
                 * Switches context to the coroutine and passes the parameter.
                 * After the coroutine switches back it interprets the yield value.
                 */
                void switch_to_coroutine(intptr_t parm) {
                    BOOST_ASSERT_MSG( m_state == RUNNING, "tried to switch to a dead coroutine");

                    intptr_t res = boost::context::jump_fcontext(&m_ctx_main, m_ctx_coroutine, parm);
                    
                    switch(res) {
                        case COROUTINE_YIELD:
                            return;

                        case COROUTINE_FINISHED:
                            m_state = IDLE;
                            return;

                        default:
                            BOOST_ASSERT_MSG(false, "invalid value yielded from coroutine");
                    }
                }

                /**
                 * @brief switch back to main routine
                 *
                 * The yielded value is in m_???.
                 */
                void yield() const {
                    BOOST_ASSERT_MSG( m_state == RUNNING, "can't yield to stopped handler");

                    intptr_t res = boost::context::jump_fcontext(m_ctx_coroutine, &m_ctx_main, COROUTINE_YIELD);
                    switch(res) {
                        case MAIN_CONTINUE:
                            return;

                        case MAIN_STOP:
                            throw Osmium::Handler::StopReading();

                        default:
                            BOOST_ASSERT_MSG(false, "invalid value passed to coroutine");
                    }
                }


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
                void *m_coroutine_parameter_file;
                void *m_coroutine_parameter_handler;

                /// instance of an iterator
                Enumerator::Iterator m_iterator;

        }; // class Enumerator

    } // namespace Handler

} // namespace Osmium

#endif // OSMIUM_HANDLER_ITERATOR_HPP
