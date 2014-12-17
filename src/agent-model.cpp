/*
 * Copyright (C) 2014 INRA
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <vle/extension/decision/Agent.hpp>
#include <vle/extension/decision/Activity.hpp>
#include <vle/devs/Executive.hpp>
#include <vle/devs/ExecutiveDbg.hpp>
#include <vle/utils/Package.hpp>
#include <vle/utils/i18n.hpp>
#include <fstream>
#include "global.hpp"
#include "strategic.hpp"
#include "lu.hpp"

namespace safihr {

class Farmer : public vle::devs::Executive,
               public vle::extension::decision::KnowledgeBase
{
    typedef vle::extension::decision::Activities::result_t ActivityList;

    CropRotation m_rotation;
    LandUnits m_lus;


    void farm_initialize()
    {
        for (size_t i = 0, e = m_lus.lus.size(); i != e; ++i) {
            char buffer[std::numeric_limits<int>::digits10 + 2];

            std::snprintf(buffer, sizeof(buffer), "p%d", m_lus.lus[i].id);

            createModelFromClass("CropSoil", buffer);
        }
    }

public:
    Farmer(const vle::devs::ExecutiveInit& mdl, const vle::devs::InitEventList& evts)
        : vle::devs::Executive(mdl, evts)
    {
        vle::utils::Package pack("safihr");

        {
            std::ifstream ifs(pack.getDataFile("Assolement-test.txt"));
            ifs >> m_rotation;
        }

        {
            std::ifstream ifs(pack.getDataFile("Farm.txt"));
            ifs >> m_lus;
        }
    }

    virtual ~Farmer()
    {
    }

    virtual vle::devs::Time init(const vle::devs::Time& time)
    {
        mState = Output;
        mCurrentTime = time;
        mNextChangeTime = processChanges(time);

        return 0.0;
    }

    virtual void output(const vle::devs::Time& time,
                        vle::devs::ExternalEventList& output) const
    {
        (void)time;

        if (mState == Output) {

            {
                const Farmer::ActivityList& lst = latestStartedActivities();
                Farmer::ActivityList::const_iterator it = lst.begin();
                for (; it != lst.end(); ++it) {
                    (*it)->second.output((*it)->first, output);
                }
            }
            {
                const Farmer::ActivityList& lst = latestFailedActivities();
                Farmer::ActivityList::const_iterator it = lst.begin();
                for (; it != lst.end(); ++it) {
                    (*it)->second.output((*it)->first, output);
                }
            }
            {
                const Farmer::ActivityList& lst = latestDoneActivities();
                Farmer::ActivityList::const_iterator it = lst.begin();
                for (; it != lst.end(); ++it) {
                    (*it)->second.output((*it)->first, output);
                }
            }
            {
                const Farmer::ActivityList& lst = latestEndedActivities();
                Farmer::ActivityList::const_iterator it = lst.begin();
                for (; it != lst.end(); ++it) {
                    (*it)->second.output((*it)->first, output);
                }
            }
        }
    }

    virtual vle::devs::Time timeAdvance() const
    {
        switch (mState) {
        case Init:
        case Output:
        case UpdateFact:
            return 0.0;
        case Process:
            if (mNextChangeTime.second == vle::devs::negativeInfinity or
                mNextChangeTime.first == true or
                haveActivityInLatestActivitiesLists()) {
                return 0.0;
            } else {
                return mNextChangeTime.second - mCurrentTime;
            }
        }

        throw std::logic_error("unknown state");
    }

    virtual void internalTransition(const vle::devs::Time& time)
    {
        mCurrentTime = time;

        switch (mState) {
        case Output:
            clearLatestActivitiesLists();
        case Init:
        case UpdateFact:
            mNextChangeTime = processChanges(time);
            mState = Process;
            break;
        case Process:
            mState = Output;
            break;
        }
    }

    virtual void externalTransition(const vle::devs::ExternalEventList& events,
                                    const vle::devs::Time& time)
    {
        mCurrentTime = time;

        for (vle::devs::ExternalEventList::const_iterator it = events.begin();
             it != events.end(); ++it) {
            const std::string& port((*it)->getPortName());
            const vle::value::Map& atts = (*it)->getAttributes();

            if (port == "ack") {
                const std::string& activity(atts.getString("name"));
                const std::string& order(atts.getString("value"));

                if (order == "done") {
                    setActivityDone(activity, time);
                } else if (order == "fail") {
                    setActivityFailed(activity, time);
                } else {
                    throw vle::utils::ModellingError(
                        vle::fmt(_("Decision: unknown order `%1%'")) % order);
                }
            } else {
                vle::value::Map::const_iterator jt = atts.value().find("value");
                if (jt == atts.end()) {
                    jt = atts.value().find("init");
                }

                if (jt == atts.end() or not jt->second) {
                    throw vle::utils::ModellingError(
                        vle::fmt(_("Decision: no value in this message: `%1%'")) %
                        (*it));
                }

                applyFact(port, *jt->second);
            }
        }

        mState = UpdateFact;
    }

    virtual void confluentTransitions(const vle::devs::Time& time,
                                      const vle::devs::ExternalEventList& lst)
    {
        internalTransition(time);
        externalTransition(lst, time);
    }

    virtual vle::value::Value* observation(const vle::devs::ObservationEvent& event) const
    {
        const std::string port = event.getPortName();

        if (port == "KnowledgeBase") {
            std::stringstream out;
            out << *this;
            return new vle::value::String(out.str());
        } else if (port == "Activities") {
            std::stringstream out;
            out << activities();
            return new vle::value::String(out.str());
        } else if ((port.compare(0, 9, "Activity_") == 0) and port.size() > 9) {
            std::string activity(port, 9, std::string::npos);
            const vle::extension::decision::Activity& act(activities().get(activity)->second);
            std::stringstream out;
            out << act.state();
            return new vle::value::String(out.str());
        } else if ((port.compare(0, 6, "Rules_") == 0) and port.size() > 6) {
            std::string rule(port, 6, std::string::npos);
            const vle::extension::decision::Rule& ru(rules().get(rule));
            return new vle::value::Boolean(ru.isAvailable());
        }
        return 0;
    }

    virtual void finish()
    {
    }

private:
    enum State
    {
        Init,
        Process,
        UpdateFact,
        Output
    };

    vle::extension::decision::KnowledgeBase::Result mNextChangeTime;
    vle::devs::Time mCurrentTime;
    State mState;
};

} // namespace safihr

DECLARE_EXECUTIVE_DBG(safihr::Farmer)
