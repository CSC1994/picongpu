/* Copyright 2013-2017 Felix Schmitt, Rene Widera, Wolfgang Hoenig,
 *                     Benjamin Worpitz
 *
 * This file is part of libPMacc.
 *
 * libPMacc is free software: you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libPMacc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with libPMacc.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "eventSystem/tasks/Factory.hpp"
#include "eventSystem/tasks/ITask.hpp"
#include "eventSystem/tasks/TaskReceive.hpp"
#include "eventSystem/tasks/TaskCopyDeviceToHost.hpp"
#include "eventSystem/EventSystem.hpp"
#include "mappings/simulation/EnvironmentController.hpp"
#include "memory/buffers/Exchange.hpp"

namespace PMacc
{



    template <class TYPE, unsigned DIM>
    class TaskSend : public MPITask
    {
    public:

        TaskSend(Exchange<TYPE, DIM> &ex) :
        exchange(&ex),
        state(Constructor)
        {
        }

        virtual void init()
        {
            state = InitDone;
            if (exchange->hasDeviceDoubleBuffer())
            {
                Environment<>::get().Factory().createTaskCopyDeviceToDevice(exchange->getDeviceBuffer(),
                                                                            exchange->getDeviceDoubleBuffer()
                                                                            );
                Environment<>::get().Factory().createTaskCopyDeviceToHost(exchange->getDeviceDoubleBuffer(),
                                                                          exchange->getHostBuffer(),
                                                                          this);
            }
            else
            {
                Environment<>::get().Factory().createTaskCopyDeviceToHost(exchange->getDeviceBuffer(),
                                                                          exchange->getHostBuffer(),
                                                                          this);
            }

        }

        bool executeIntern()
        {
            switch (state)
            {
                case InitDone:
                    break;
                case DeviceToHostFinished:
                    state = SendDone;
                    __startTransaction();
                    Environment<>::get().Factory().createTaskSendMPI(exchange, this);
                    __endTransaction();
                    break;
                case SendDone:
                    break;
                case Finish:
                    return true;
                default:
                    return false;
            }

            return false;
        }

        virtual ~TaskSend()
        {
            notify(this->myId, SENDFINISHED, nullptr);
        }

        void event(id_t, EventType type, IEventData*)
        {
            if (type == COPYDEVICE2HOST)
            {
                state = DeviceToHostFinished;
                executeIntern();

            }

            if (type == SENDFINISHED)
            {
                state = Finish;
            }

        }

        std::string toString()
        {
            std::stringstream ss;
            ss<<state;
            return std::string("TaskSend ")+ ss.str();
        }

    private:

        enum state_t
        {
            Constructor,
            InitDone,
            DeviceToHostFinished,
            SendDone,
            Finish
        };

        Exchange<TYPE, DIM> *exchange;
        state_t state;
    };

} //namespace PMacc

